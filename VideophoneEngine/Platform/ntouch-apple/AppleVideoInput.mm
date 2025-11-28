/*!
 * \file AppleVideoInput.mm
 * \brief Video Input for Apple platforms, using AVKit/Foundation and
 *        VideoToolkit. See IstiVideoInput.h for method documentation.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AppleVideoInput.h"
#include "AppleUtilities.h"
#include "stiDebugTools.h"
#include "ntouch-Swift.h"
#include <VideoToolbox/VideoToolbox.h>

namespace vpe
{

static const size_t MAX_FRAMES = 3;

/*!
 * \brief Constructor
 */
AppleVideoInput::AppleVideoInput ()
{
	SCICaptureController.shared.outputFrameCallback = ^(CMSampleBufferRef sampleBuffer)
	{
		std::lock_guard<std::mutex> lock (m_framesLock);

		// Discard some frames if they don't fit in the buffer.
		while (m_frames.size () >= MAX_FRAMES)
		{
			stiTrace ("RTP Send too slow, discarding frame");

			// VideoRecord isn't asking us for frames as fast as we're receiving
			// them. Discard some old frames so we don't just leak memory
			// endlessly.
			DropFrame ();
		}

		CFRetain (sampleBuffer);
		m_frames.push (sampleBuffer);
		packetAvailableSignalGet ().Emit ();
	};

	SCICaptureController.shared.recalculateCaptureSizeCallback = ^()
	{
		captureCapabilitiesChangedSignalGet ().Emit ();
	};

	m_privacyObserver =
		(__bridge void *)[[NSNotificationCenter defaultCenter]
			addObserverForName:SCICaptureController.privacyChanged
			object:SCICaptureController.shared
			queue:nil
			usingBlock:^(NSNotification *) {
				videoPrivacySetSignalGet ().Emit (SCICaptureController.shared.privacy || SCICaptureController.shared.interrupted);
				if (SCICaptureController.shared.privacy || SCICaptureController.shared.interrupted)
				{
					ClearFrameQueue ();
				}
			}];
}


AppleVideoInput::~AppleVideoInput ()
{
	SCICaptureController.shared.outputFrameCallback = nil;
	ClearFrameQueue ();
	[[NSNotificationCenter defaultCenter] removeObserver:(__bridge id)m_privacyObserver];
}


void AppleVideoInput::DropFrame ()
{
	CFRelease (m_frames.front ());
	m_frames.pop ();

	// We're discarding frames, so request a keyframe.
	// TODO: We can check if the frame is depended on by others. If it is,
	// request a new keyframe. Otherwise, we don't need to request a keyframe.
	[SCICaptureController.shared requestKeyframe];
}


/*!
 * \brief Clears the queue of frames to be sent to VideoRecord.
 */
void AppleVideoInput::ClearFrameQueue ()
{
	std::lock_guard<std::mutex> lock (m_framesLock);

	while (!m_frames.empty ())
	{
		CFRelease (m_frames.front ());
		m_frames.pop ();
	}

	[SCICaptureController.shared requestKeyframe];
}


stiHResult AppleVideoInput::VideoRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	ClearFrameQueue ();
	dispatch_async(dispatch_get_main_queue(), ^{
		SCICaptureController.shared.recording = true;
	});

	if (SCICaptureController.shared.privacy == true) {
		stiTHROW (stiRESULT_ERROR);
	}
	else {
		return hResult;
	}
	
	STI_BAIL:
	return hResult;
	
}


stiHResult AppleVideoInput::VideoRecordStop ()
{
	dispatch_async(dispatch_get_main_queue(), ^{
		SCICaptureController.shared.recording = false;
	});
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *settings)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (settings != nullptr)
	{
		m_codec = settings->codec;
		switch (m_codec)
		{
			case estiVIDEO_H263:
				SCICaptureController.shared.videoCodec = kCMVideoCodecType_H263;
				break;
			case estiVIDEO_H264:
				SCICaptureController.shared.videoCodec = kCMVideoCodecType_H264;
				break;
			case estiVIDEO_H265:
				SCICaptureController.shared.videoCodec = kCMVideoCodecType_HEVC;
				break;
			case estiVIDEO_RTX:
			case estiVIDEO_NONE:
				stiTHROW (stiRESULT_ERROR);
		}
		m_profile = settings->eProfile;
		m_level = settings->unLevel;

		SCICaptureController.shared.targetDimensions ={
			int32_t (settings->unColumns),
			int32_t (settings->unRows)
		};

		SCICaptureController.shared.targetFrameDuration = CMTimeMake (1, settings->unTargetFrameRate);
		SCICaptureController.shared.maxPacketSize = settings->unMaxPacketSize;
		SCICaptureController.shared.targetBitRate = settings->unTargetBitRate;
		SCICaptureController.shared.intraFrameRate = settings->unIntraFrameRate;
		SCICaptureController.shared.profile = SCIVideoProfile(m_profile);
		SCICaptureController.shared.level = m_level;
	}

	STI_BAIL:
	return hResult;
}


stiHResult AppleVideoInput::VideoRecordFrameGet (SstiRecordVideoFrame *videoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Get the next acceptable sample buffer
	m_framesLock.lock ();
	CMSampleBufferRef sampleBuffer = nullptr;
	while (!m_frames.empty ())
	{
		CMSampleBufferRef nextFrame = m_frames.front ();

		CMFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription (nextFrame);
		CMVideoCodecType fourCharCode = CMFormatDescriptionGetMediaSubType (formatDesc);
		EstiVideoCodec codec = ConvertFourCharCodeToVideoCodec (fourCharCode);

		if (codec == m_codec)
		{
			m_frames.pop ();
			sampleBuffer = nextFrame;
			break;
		}
		else
		{
			DropFrame ();
		}
	}
	m_framesLock.unlock ();

	stiTESTCOND (sampleBuffer != nullptr, stiRESULT_DRIVER_ERROR)

	hResult = VideoPacketFromSampleBuffer (sampleBuffer, videoPacket);
	stiTESTRESULT ();

STI_BAIL:
	if (sampleBuffer != nullptr)
	{
		CFRelease (sampleBuffer);
	}

	return hResult;
}


stiHResult AppleVideoInput::PrivacySet (EstiBool enable)
{
	dispatch_async(dispatch_get_main_queue(), ^{
		SCICaptureController.shared.privacy = enable;
	});
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoInput::PrivacyGet (EstiBool *enabledOut) const
{
	if (enabledOut != nullptr)
	{
		*enabledOut = SCICaptureController.shared.privacy ? estiTRUE : estiFALSE;
	}

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoInput::KeyFrameRequest ()
{
	[SCICaptureController.shared requestKeyframe];
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoInput::VideoCodecsGet (std::list<EstiVideoCodec> *codecs)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (SCICaptureController.shared.forcedCodec != 0)
	{
		EstiVideoCodec codec = ConvertFourCharCodeToVideoCodec (SCICaptureController.shared.forcedCodec);
		if (codec != estiVIDEO_NONE)
		{
			codecs->push_back (codec);
		}
		else
		{
			stiTHROW (stiRESULT_INVALID_CODEC);
		}
	}
	else
	{
		NSArray *availableCodecs = SCICaptureController.shared.availableCodecs;
#ifdef stiENABLE_H265_ENCODE
		if ([availableCodecs containsObject:@(kCMVideoCodecType_HEVC)] && SCICaptureController.shared.enableHEVC)
		{
			codecs->push_back (estiVIDEO_H265);
		}
#endif
		if ([availableCodecs containsObject:@(kCMVideoCodecType_H264)])
		{
			codecs->push_back (estiVIDEO_H264);
		}
	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoInput::CodecCapabilitiesGet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xa0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4;

			break;
		}
		case estiVIDEO_H265:
		{
			SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH265Caps->Profiles.push_back (stProfileAndConstraint);
			pstH265Caps->eLevel = estiH265_LEVEL_4;
			pstH265Caps->eTier = estiH265_TIER_MAIN;

			break;
		}

		default:
			stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}


} // namespace vpe
