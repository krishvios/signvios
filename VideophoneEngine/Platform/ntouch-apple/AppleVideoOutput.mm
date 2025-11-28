/*!
 * \file AppleVideoOutput.mm
 * \brief Video Output for Apple platforms, using AVKit/Foundation and
 *        VideoToolkit. See IstiVideoOutput.h for method documentation.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AppleVideoOutput.h"
#include "AppleUtilities.h"
#include "stiDebugTools.h"
#include "CstiSyncManager.h"
#include "ntouch-Swift.h"
#include <algorithm>
#include <chrono>
#include <VideoToolbox/VideoToolbox.h>

namespace vpe
{

const size_t MAX_CONSECUTIVE_DECODER_ERRORS = 60; // About 2 seconds of failed video frames at 30fps
const size_t MIN_PLAYBACK_FRAMES = 5;
const size_t MAX_PLAYBACK_FRAMES = 30;
const auto MAX_TIME_BETWEEN_KEYFRAME_REQUESTS = std::chrono::seconds (30);
const auto MIN_TIME_BETWEEN_KEYFRAME_REQUESTS = std::chrono::seconds (2);

class VideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	explicit VideoPlaybackFrame (CFAllocatorRef allocator)
		: m_allocator (allocator)
	{
		if (m_allocator != nullptr)
		{
			CFRetain (m_allocator);
		}
	}

	virtual ~VideoPlaybackFrame ()
	{
		BufferSizeSet (0); // Deallocates
		if (m_allocator != nullptr)
		{
			CFRelease (m_allocator);
		}
	}

	stiHResult BufferSizeSet (uint32_t bufferSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		bufferSize = CFAllocatorGetPreferredSizeForSize (m_allocator, bufferSize, 0);
		void *buffer = CFAllocatorReallocate (m_allocator, m_buffer, bufferSize, 0);
		stiTESTCOND_NOLOG (buffer || bufferSize == 0, stiRESULT_MEMORY_ALLOC_ERROR);

		m_buffer = buffer;
		m_capacity = bufferSize;

	STI_BAIL:
		return hResult;
	}

	stiHResult DataSizeSet (uint32_t dataSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		stiTESTCOND_NOLOG (dataSize <= m_capacity, stiRESULT_BUFFER_TOO_SMALL);

		m_size = dataSize;

	STI_BAIL:
		return hResult;
	}

	bool FrameIsCompleteGet () { return m_isComplete; }
	void FrameIsCompleteSet (bool isComplete) { m_isComplete = isComplete; }
	bool FrameIsKeyframeGet () { return m_isKeyframe; }
	void FrameIsKeyframeSet (bool isKeyframe) { m_isKeyframe = isKeyframe; }
	uint8_t *BufferGet () { return (uint8_t *)m_buffer; }
	uint32_t BufferSizeGet () const { return m_capacity; }
	uint32_t DataSizeGet () const { return m_size; }

private:
	CFAllocatorRef m_allocator;
	void *m_buffer = nullptr;
	size_t m_size = 0;
	size_t m_capacity = 0;
	bool m_isComplete = false;
	bool m_isKeyframe = false;
};

AppleVideoOutput::AppleVideoOutput ()
{
	m_keyframeRequestTimer.TimeoutSet (MAX_TIME_BETWEEN_KEYFRAME_REQUESTS.count ());
	m_keyframeRequestTimer.TypeSet (CstiTimer::Type::Single);
	m_keyframeRequestTimer.timeoutEvent.Connect ([this]
	{
		KeyframeRequest ();
	});

	m_preferredDisplaySizeChangedToken =
		(__bridge void *)[[NSNotificationCenter defaultCenter]
						  addObserverForName:SCIDisplayController.preferredDisplaySizeChanged
						  object:SCIDisplayController.shared
						  queue:nil
						  usingBlock:^(NSNotification *) { preferredDisplaySizeChangedSignalGet ().Emit (); }];
}

AppleVideoOutput::~AppleVideoOutput ()
{
	std::lock_guard<std::recursive_mutex> lock (m_framesMutex);
	while (m_emptyFrames.size () > 0)
	{
		IstiVideoPlaybackFrame *videoFrame = m_emptyFrames.front ();
		m_emptyFrames.pop ();
		VideoPlaybackFrameDiscard (videoFrame);
	}

	if (m_formatDescription != nullptr)
	{
		CFRelease (m_formatDescription);
		m_formatDescription = nullptr;
	}

	if (m_decoder != nullptr)
	{
		VideoDecoderInvalidate ();
	}

	[[NSNotificationCenter defaultCenter] removeObserver:(__bridge id)m_preferredDisplaySizeChangedToken];
}

stiHResult AppleVideoOutput::RemoteViewPrivacySet (EstiBool privacy)
{
	m_privacy = privacy;
	if (privacy)
	{
		VideoPlaybackBlackFramePut ();
	}
	else
	{
		KeyframeRequest (true); // Force a keyframe request right now
		m_awaitingKeyframe = true;
	}

	SCIDisplayController.shared.privacy = privacy;
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::RemoteViewHoldSet (EstiBool hold)
{
	m_hold = hold;
	if (hold)
	{
		VideoPlaybackBlackFramePut ();
	}
	else
	{
		VideoPlaybackBlackFramePut ();
		KeyframeRequest (true); // Force a keyframe request right now
		m_awaitingKeyframe = true;
	}

	SCIDisplayController.shared.hold = hold;
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackStart ()
{
	m_playing = true;
	m_awaitingKeyframe = true;
	VideoPlaybackBlackFramePut ();

	m_lastKeyframeRequestTime = std::chrono::steady_clock::time_point::min ();
	KeyframeRequest (true); // Force a keyframe request right now

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackStop ()
{
	m_playing = false;

	std::lock_guard<std::recursive_mutex> lock (m_framesMutex);
	while (m_emptyFrames.size () > MIN_PLAYBACK_FRAMES)
	{
		IstiVideoPlaybackFrame *videoFrame = m_emptyFrames.front ();
		m_emptyFrames.pop ();
		VideoPlaybackFrameDiscard (videoFrame);
	}

	VideoPlaybackBlackFramePut ();

	if (m_formatDescription != nullptr)
	{
		CFRelease (m_formatDescription);
		m_formatDescription = nullptr;
	}

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackFrameGet (IstiVideoPlaybackFrame **videoFrameOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IstiVideoPlaybackFrame *videoFrame = nullptr;
	{
		std::lock_guard<std::recursive_mutex> lock (m_framesMutex);
		if (!m_emptyFrames.empty ())
		{
			videoFrame = m_emptyFrames.front ();
			m_emptyFrames.pop ();
		}
		else if (m_framesCreated < MAX_PLAYBACK_FRAMES)
		{
			videoFrame = new VideoPlaybackFrame (kCFAllocatorDefault);
			stiTESTCOND (videoFrame != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
			m_framesCreated++;
		}
	}

	if (videoFrame != nullptr)
	{
		*videoFrameOut = videoFrame;
	}
	else
	{
		// See if we can get the output layers to free up some buffers
		[SCIDisplayController.shared flush];
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *videoFrame)
{
	std::lock_guard<std::recursive_mutex> lock (m_framesMutex);

	if (m_playing || m_emptyFrames.size () < MIN_PLAYBACK_FRAMES)
	{
		m_emptyFrames.push (videoFrame);
	}
	else
	{
		m_framesCreated--;
		delete (VideoPlaybackFrame *)videoFrame;
	}

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackFramePut (IstiVideoPlaybackFrame *videoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CMSampleBufferRef sampleBuffer = nullptr;
	{
		stiTESTCOND (m_playing, stiRESULT_ERROR);
		stiTESTCOND_NOLOG (videoFrame->FrameIsCompleteGet (), stiRESULT_ERROR);

		hResult = FormatDescriptionUpdate (videoFrame);
		stiTESTRESULT ();

		stiTESTCOND (m_formatDescription != nullptr, stiRESULT_ERROR);

		if (videoFrame->FrameIsKeyframeGet ())
		{
			m_awaitingKeyframe = false;
		}
		
		// Make sure we invalidate the decoder if it's been flagged as invalid
		if (m_decoderIsInvalid)
		{
			hResult = VideoDecoderInvalidate ();
			stiTESTRESULT ();
		}

		// Try to create the decoder for this frame if it's a keyframe
		if (m_decoder == nullptr && videoFrame->FrameIsKeyframeGet ())
		{
			hResult = VideoDecoderCreate ();
			stiTESTRESULT ();
		}

		// Don't log this because if the decoder failed to initialize we logged it when it failed.
		stiTESTCOND_NOLOG (m_decoder != nullptr, stiRESULT_ERROR);

		// Wrap the video frame in a sample buffer.
		hResult = SampleBufferFromPlaybackFrameGet (
			videoFrame,
			m_formatDescription,
			&sampleBuffer);
		stiTESTRESULT ();
		videoFrame = nullptr;

		// We assume frames come in decode order (no B-frames). If they do not, they should be rearranged in a CMBufferQueue.
		OSStatus status = VTDecompressionSessionDecodeFrame (
			m_decoder,
			sampleBuffer,
			kVTDecodeFrame_1xRealTimePlayback | kVTDecodeFrame_EnableAsynchronousDecompression,
			nullptr, nullptr);
		if (status != 0)
		{
			// If we failed to send a frame to the decoder, try re-initializing the encoder.
			m_consecutiveDecoderErrorCount++;
			if (m_consecutiveDecoderErrorCount > MAX_CONSECUTIVE_DECODER_ERRORS)
			{
				VideoDecoderInvalidate ();
			}
		}
		else
		{
			m_consecutiveDecoderErrorCount = 0;
		}

		stiTESTCOND_NOLOG (status == 0, stiRESULT_ERROR);
	}

STI_BAIL:
	if (stiIS_ERROR (hResult))
	{
		if (videoFrame)
		{
			VideoPlaybackFrameDiscard (videoFrame);
			videoFrame = nullptr;
		}

		// TODO: There may be cases where it is okay to drop a frame without
		// requesting a keyframe (e.g. with Hierarchical Coding in H265)
		KeyframeRequest ();
		m_awaitingKeyframe = true;
	}

	if (sampleBuffer)
	{
		CFRelease (sampleBuffer);
		sampleBuffer = nullptr;
	}

	return hResult;
}


stiHResult AppleVideoOutput::VideoFrameDecoded (CMSampleBufferRef sampleBuffer) const
{
	[SCIDisplayController.shared enqueueSampleBuffer:sampleBuffer];
	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoDecoderInvalidate ()
{
	std::lock_guard<std::recursive_mutex> lock (m_framesMutex);
	if (m_decoder != nullptr)
	{
		VTDecompressionSessionWaitForAsynchronousFrames (m_decoder);
		CFRelease (m_decoder);
		m_decoder = nil;
	}

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoDecoderCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;
	CFDictionaryRef videoDecoderSpecification = nullptr;
	
#if APPLICATION == APP_NTOUCH_MAC
	auto dimensions = CMVideoFormatDescriptionGetDimensions (m_formatDescription);
	
	// Due to a Catalina SDK bug, we need to disable hardware accelerated decoding when below VGA resolution
	if (dimensions.width <= unstiVGA_COLS && dimensions.height <= unstiVGA_ROWS) {
		videoDecoderSpecification = (__bridge CFDictionaryRef)@{ (__bridge NSString *)kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder: @NO };
	}
#endif

	VTDecompressionOutputCallbackRecord callback;
	callback.decompressionOutputRefCon = this;
	callback.decompressionOutputCallback = [](
		void * CM_NULLABLE decompressionOutputRefCon,
		void * CM_NULLABLE sourceFrameRefCon,
		OSStatus status,
		VTDecodeInfoFlags infoFlags,
		CM_NULLABLE CVImageBufferRef imageBuffer,
		CMTime presentationTimeStamp,
		CMTime presentationDuration)
	{
		AppleVideoOutput *owner = static_cast<AppleVideoOutput *> (decompressionOutputRefCon);
		if (status == 0 && imageBuffer != nullptr)
		{
			CMVideoFormatDescriptionRef formatDesc;
			status = CMVideoFormatDescriptionCreateForImageBuffer (kCFAllocatorDefault, imageBuffer, &formatDesc);
			stiASSERTMSG (status == 0, "status=%d");

			CMSampleTimingInfo timingInfo;
			timingInfo.decodeTimeStamp = presentationTimeStamp;
			timingInfo.presentationTimeStamp = presentationTimeStamp;
			timingInfo.duration = presentationDuration;

			CMSampleBufferRef sampleBuffer;
			status = CMSampleBufferCreateForImageBuffer (
				kCFAllocatorDefault,
				imageBuffer, true, nullptr, nullptr,
				formatDesc,
				&timingInfo,
				&sampleBuffer);
			stiASSERTMSG (status == 0, "status=%d");

			owner->VideoFrameDecoded (sampleBuffer);
			CFRelease (sampleBuffer);
			CFRelease (formatDesc);
		}
		else
		{
			stiTrace ("Failed to decode video frame. (status=%d)", status);
			// We can't invalidate the decoder ourselves here because we are on the wrong thread,
			// so tell AppleVideoOutput it should invalidate the decoder before using it.
			owner->m_decoderIsInvalid = true;
			owner->KeyframeRequest ();
		}
	};

	stiTESTCOND (m_formatDescription != nullptr, stiRESULT_ERROR);

	hResult = VideoDecoderInvalidate ();
	stiTESTRESULT ();

	m_consecutiveDecoderErrorCount = 0;
	status = VTDecompressionSessionCreate (
		kCFAllocatorDefault,
		m_formatDescription,
		videoDecoderSpecification,
		(__bridge CFDictionaryRef)@{
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
			(NSString *)kCVPixelBufferMetalCompatibilityKey: @(true),
			(NSString *)kCVPixelBufferOpenGLESCompatibilityKey: @(true)
#endif
		},
		&callback,
		&m_decoder);
	stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);

	m_decoderIsInvalid = false;
	
STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::VideoPlaybackBlackFramePut ()
{
	if (m_decoder != nullptr)
	{
		// Ensure submitted asynchronous frames don't overwrite our black frame.
		m_decoderIsInvalid = true;
	}

	// The proper way to do this would be to enqueue an empty samplebuffer with
	// the kCMSampleBufferAttachmentKey_EmptyMedia attachment set to true, but
	// AVSampleBufferDisplayLayer seems to ignore this even though it is the one
	// to document this behavior...
	[SCIDisplayController.shared flushAndRemoveImage];
	return stiRESULT_SUCCESS;
}

stiHResult AppleVideoOutput::FormatDescriptionUpdate (IstiVideoPlaybackFrame *videoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CMVideoFormatDescriptionRef formatDesc = nullptr;
	CMVideoDimensions before = {0, 0};
	CMVideoDimensions after = {0, 0};

	hResult = FormatDescriptionFromFrameGet (videoFrame, m_codec, &formatDesc);
	stiTESTRESULT ();

	if (m_formatDescription)
	{
		before = CMVideoFormatDescriptionGetDimensions (m_formatDescription);
	}

	if (formatDesc)
	{
		after = CMVideoFormatDescriptionGetDimensions (formatDesc);

		if (m_formatDescription)
		{
			CFRelease (m_formatDescription);
		}
		m_formatDescription = formatDesc;

		if (m_decoder != nullptr && !VTDecompressionSessionCanAcceptFormatDescription(m_decoder, m_formatDescription))
		{
			VideoDecoderInvalidate ();
		}

		if (before.width != after.width || before.height != after.height)
		{
			decodeSizeChangedSignalGet ().Emit (after.width, after.height);
		}
	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::KeyframeRequest (bool forceNow)
{
	auto currentTime = std::chrono::steady_clock::now ();
	auto timeSinceLastRequest = currentTime - m_lastKeyframeRequestTime;

	if (forceNow || timeSinceLastRequest >= MIN_TIME_BETWEEN_KEYFRAME_REQUESTS)
	{
		m_lastKeyframeRequestTime = currentTime;
		keyframeNeededSignalGet ().Emit ();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds> (MAX_TIME_BETWEEN_KEYFRAME_REQUESTS);
		m_keyframeRequestTimer.TimeoutSet (milliseconds.count ());
	}
	else
	{
		m_keyframeRequestTimer.Stop ();
		auto nextRequestTime = m_lastKeyframeRequestTime + MIN_TIME_BETWEEN_KEYFRAME_REQUESTS;
		auto timeUntilNextRequest = nextRequestTime - currentTime;
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds> (timeUntilNextRequest);
		m_keyframeRequestTimer.TimeoutSet (std::max (milliseconds.count (), 0LL));
	}

	if (m_playing && !m_privacy && !m_hold)
	{
		m_keyframeRequestTimer.Restart ();
	}

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackSizeGet (uint32_t *width, uint32_t *height) const
{
	CMVideoDimensions size = {0, 0};
	if (m_formatDescription != nullptr)
	{
		size = CMVideoFormatDescriptionGetDimensions (m_formatDescription);
	}

	*width = size.width;
	*height = size.height;

	return stiRESULT_SUCCESS;
}


stiHResult AppleVideoOutput::VideoPlaybackCodecSet (EstiVideoCodec videoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (videoCodec)
	{
		case estiVIDEO_H263:
		case estiVIDEO_H264:
		case estiVIDEO_H265:
			m_codec = videoCodec;
			break;
		case estiVIDEO_RTX:
		case estiVIDEO_NONE:
			stiTHROW (stiRESULT_INVALID_CODEC);
	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::VideoCodecsGet (std::list<EstiVideoCodec> *codecs)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (SCIDisplayController.shared.forcedCodec != 0)
	{
		EstiVideoCodec codec = ConvertFourCharCodeToVideoCodec (SCIDisplayController.shared.forcedCodec);
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
#ifdef stiENABLE_H265_DECODE
		if (SCIDisplayController.shared.supportsHEVC && SCIDisplayController.shared.enableHEVC)
		{
			codecs->push_back (estiVIDEO_H265);
		}
#endif
		codecs->push_back (estiVIDEO_H264);
	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::CodecCapabilitiesGet (
							 EstiVideoCodec eCodec,
							 SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	SstiVideoCapabilities* pstLocalCaps = m_codecCapabilities[eCodec];

	//This next peice of code is the failover in case the CodecCapabilites aren't set.
	if (!pstLocalCaps)
	{
		switch (eCodec)
		{
			case estiVIDEO_H264:
			{
				SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
				stProfileAndConstraint.eProfile = estiH264_BASELINE;
				stProfileAndConstraint.un8Constraints = 0xa0;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
				pstH264Caps->eLevel = estiH264_LEVEL_4;
				break;
			}
			default:
			{
				stiTHROW (stiRESULT_ERROR);
			}
		}
	}
	else
	{
		switch (eCodec)
		{
			case estiVIDEO_H264:
			{
				SstiH264Capabilities* pstH264LocalCaps = (SstiH264Capabilities*)pstLocalCaps;
				SstiH264Capabilities* pstH264Caps = (SstiH264Capabilities*)pstCaps;
				pstH264Caps->eLevel = pstH264LocalCaps->eLevel;
				pstH264Caps->unCustomMaxFS = pstH264LocalCaps->unCustomMaxFS;
				pstH264Caps->unCustomMaxMBPS = pstH264LocalCaps->unCustomMaxMBPS;
				break;
			}

			case estiVIDEO_H265:
			{
				SstiH265Capabilities* pstH265LocalCaps = (SstiH265Capabilities*)pstLocalCaps;
				SstiH265Capabilities* pstH265Caps = (SstiH265Capabilities*)pstCaps;
				pstH265Caps->eLevel = pstH265LocalCaps->eLevel;
				pstH265Caps->eTier = pstH265LocalCaps->eTier;
				break;
			}

			default:
				break;
		}
		//Copy the profiles
		std::list<SstiProfileAndConstraint>::iterator iter;
		for (iter = pstLocalCaps->Profiles.begin (); iter != pstLocalCaps->Profiles.end (); iter++)
		{
			SstiProfileAndConstraint stCopyData;
			memcpy (&stCopyData, &(*iter), sizeof (SstiProfileAndConstraint));
			pstCaps->Profiles.push_back (stCopyData);
		}

	}

STI_BAIL:
	return hResult;
}


stiHResult AppleVideoOutput::CodecCapabilitiesSet (
							 EstiVideoCodec eCodec,
							 SstiVideoCapabilities *pstCaps)
{
	m_codecCapabilities[eCodec] = pstCaps;
	return stiRESULT_SUCCESS;
}

void AppleVideoOutput::PreferredDisplaySizeGet (
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	CGSize preferredDisplaySize = SCIDisplayController.shared.preferredDisplaySize;
	*displayWidth = preferredDisplaySize.width;
	*displayHeight = preferredDisplaySize.height;
}

} // namespace vpe

