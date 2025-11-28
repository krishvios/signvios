#include "CstiVideoOutput.h"
#include "VideoPlaybackFrame.h"
#include "AvVideoDecoder.h"
#include "VideoDisplayFrame.h"
#include "stiDebugTools.h"

const size_t MIN_PLAYBACK_FRAMES = 5;
const size_t MAX_PLAYBACK_FRAMES = 30;
const int MIN_IFRAME_REQUEST_DELTA = 2;
const int MAX_IFRAME_REQUEST_DELTA = 30;

CstiVideoOutput::CstiVideoOutput () :
	CstiEventQueue("VideoOutput")
{
	m_framesMutex = stiOSMutexCreate ();
	m_decoderMutex = stiOSMutexCreate ();

	auto eResult = stiOSSignalCreate (&m_fdSignal);
	stiASSERT (stiRESULT_CONVERT (eResult) == stiRESULT_SUCCESS);
	stiOSSignalSet (m_fdSignal);

	m_decoder = new AvVideoDecoder ();

	auto h263Capabilities = new SstiVideoCapabilities ();
	SstiProfileAndConstraint h263ProfileAndConstraint;
	h263ProfileAndConstraint.eProfile = estiH263_ZERO;
	h263Capabilities->Profiles.push_back (h263ProfileAndConstraint);
	CodecCapabilitiesSet (estiVIDEO_H263, h263Capabilities);

	SstiH264Capabilities* stH264Capabilities = new SstiH264Capabilities;
	//Profile and Constraint Signal Main
	SstiProfileAndConstraint stH264ProfileAndConstraintMain;
	stH264ProfileAndConstraintMain.eProfile = estiH264_MAIN;
	stH264ProfileAndConstraintMain.un8Constraints = 0xe0;
	stH264Capabilities->Profiles.push_back (stH264ProfileAndConstraintMain);
	//Profile and Constraint Signal Baseline
	SstiProfileAndConstraint stH264ProfileAndConstraintBaseLine;
	stH264ProfileAndConstraintBaseLine.eProfile = estiH264_BASELINE;
	stH264ProfileAndConstraintBaseLine.un8Constraints = 0xa0;
	stH264Capabilities->Profiles.push_back (stH264ProfileAndConstraintBaseLine);
	stH264Capabilities->eLevel = estiH264_LEVEL_4_1;
	CodecCapabilitiesSet (estiVIDEO_H264, stH264Capabilities);

	SstiH265Capabilities* stH265Capabilities = new SstiH265Capabilities ();
	stH265Capabilities->eLevel = estiH265_LEVEL_4_1;
	stH265Capabilities->eTier = estiH265_TIER_MAIN;
	//Profile for h265
	SstiProfileAndConstraint stH265ProfileAndConstraint;
	stH265ProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
	stH265Capabilities->Profiles.push_back (stH265ProfileAndConstraint);
	CodecCapabilitiesSet (estiVIDEO_H265, stH265Capabilities);

	gettimeofday (&m_lastIFrame, nullptr);
	
	PostEvent ([this]() {
		constexpr int displayFramesCount = 3;
		for (int i = 0; i < displayFramesCount; i++)
		{
			m_displayFrames.push (new VideoDisplayFrame (640, 480));
		}
	});
}


CstiVideoOutput::~CstiVideoOutput ()
{
	if (m_fdSignal)
	{
		stiOSSignalClose (&m_fdSignal);
		m_fdSignal = nullptr;
	}
	stiOSMutexDestroy (m_decoderMutex);
	stiOSMutexDestroy (m_framesMutex);
	delete m_decoder;
}


stiHResult CstiVideoOutput::CodecCapabilitiesGet (EstiVideoCodec codec, SstiVideoCapabilities *videoCapabilities)
{
	SstiProfileAndConstraint stProfileAndConstraint;
	auto hResult = stiRESULT_SUCCESS;

	SstiVideoCapabilities* pstLocalCaps = m_codecCapabilities[codec];

	//This next piece of code is the failover in case the CodecCapabilites aren't set.
	if (!pstLocalCaps)
	{
		switch (codec)
		{
			case estiVIDEO_H264:
			{
				SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)videoCapabilities;
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
		switch (codec)
		{
			case estiVIDEO_H264:
			{
				SstiH264Capabilities* pstH264LocalCaps = (SstiH264Capabilities*)pstLocalCaps;
				SstiH264Capabilities* pstH264Caps = (SstiH264Capabilities*)videoCapabilities;
				pstH264Caps->eLevel = pstH264LocalCaps->eLevel;
				pstH264Caps->unCustomMaxFS = pstH264LocalCaps->unCustomMaxFS;
				pstH264Caps->unCustomMaxMBPS = pstH264LocalCaps->unCustomMaxMBPS;
				break;
			}

			case estiVIDEO_H265:
			{
				SstiH265Capabilities* pstH265LocalCaps = (SstiH265Capabilities*)pstLocalCaps;
				SstiH265Capabilities* pstH265Caps = (SstiH265Capabilities*)videoCapabilities;
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
			videoCapabilities->Profiles.push_back (stCopyData);
		}
	}

STI_BAIL:
	return hResult;
}


stiHResult CstiVideoOutput::CodecCapabilitiesSet (EstiVideoCodec codec, SstiVideoCapabilities *videoCapabilities)
{
	m_codecCapabilities[codec] = videoCapabilities;
	return stiRESULT_SUCCESS;
}


void CstiVideoOutput::DisplaySettingsGet (SstiDisplaySettings * displaySettings) const
{
	memset (displaySettings, 0, sizeof (SstiDisplaySettings));
}


stiHResult CstiVideoOutput::DisplaySettingsSet (const SstiDisplaySettings *displaySettings)
{
	return stiRESULT_SUCCESS;
}


int CstiVideoOutput::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_fdSignal);
}


void CstiVideoOutput::PreferredDisplaySizeGet (unsigned int *displayWidth, unsigned int *displayHeight) const
{
	*displayWidth = 0;
	*displayHeight = 0;
}


stiHResult CstiVideoOutput::RemoteViewHoldSet (EstiBool hold)
{
	if (hold == estiTRUE)
	{
		CstiOSMutexLock lock (m_decoderMutex);
		m_decoder->clear ();
		displayBlackFrame ();
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::RemoteViewPrivacySet (EstiBool privacy)
{
	if (privacy == estiTRUE)
	{
		CstiOSMutexLock lock (m_decoderMutex);
		m_decoder->clear ();
		displayBlackFrame ();
	}
	//PostEvent ([this, privacy]() {
	//	ntouchPC::CstiNativeVideoLink::RemoteVideoPrivacy (privacy == estiTRUE);
	//});
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::VideoCodecsGet (std::list<EstiVideoCodec> *codecs)
{
	if (estiVIDEO_NONE == m_testCodec)
	{
		if (IsHevcAllowed ())
		{
			codecs->push_back (estiVIDEO_H265);
		}
		codecs->push_back (estiVIDEO_H264);
		codecs->push_back (estiVIDEO_H263);
	}
	else
	{
		codecs->push_back (m_testCodec);
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::VideoPlaybackCodecSet (EstiVideoCodec videoCodec)
{
	auto hResult = stiRESULT_SUCCESS;
	switch (videoCodec)
	{
		case estiVIDEO_H263:
		case estiVIDEO_H264:
		case estiVIDEO_H265:
			m_codec = videoCodec;
			break;
		default:
			stiTHROW (stiRESULT_INVALID_CODEC);
	}

STI_BAIL:
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *videoFrame)
{
	CstiOSMutexLock lock (m_framesMutex);

	if (m_emptyFrames.size () < MIN_PLAYBACK_FRAMES)
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


stiHResult CstiVideoOutput::VideoPlaybackFrameGet (IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IstiVideoPlaybackFrame *videoFrame = nullptr;
	{
		CstiOSMutexLock lock (m_framesMutex);
		if (!m_emptyFrames.empty ())
		{
			videoFrame = m_emptyFrames.front ();
			m_emptyFrames.pop ();
		}
		else if (m_framesCreated < MAX_PLAYBACK_FRAMES)
		{
			// start with 7k buffers
			constexpr auto DEFAULT_PLAYBACK_FRAME_SIZE = 1024 * 7;
			videoFrame = new VideoPlaybackFrame (DEFAULT_PLAYBACK_FRAME_SIZE);
			stiTESTCOND (videoFrame != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
			m_framesCreated++;
		}
	}

	if (videoFrame != nullptr)
	{
		*ppVideoFrame = videoFrame;
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}

stiHResult CstiVideoOutput::VideoPlaybackFramePut (IstiVideoPlaybackFrame *videoFrame)
{
	CstiOSMutexLock lock (m_decoderMutex);

	uint8_t un8Flags = 0;

	auto success = m_decoder->decode (videoFrame, &un8Flags);

	if (success)
	{
		if (un8Flags & IstiVideoDecoder::FLAG_FRAME_COMPLETE)
		{
			// Check for a size change
			if (m_decoder->widthGet () != m_width || m_decoder->heightGet () != m_height)
			{
				m_width = m_decoder->widthGet ();
				m_height = m_decoder->heightGet ();
				decodeSizeChangedSignal.Emit ((uint32_t)m_width, (uint32_t)m_height);
			}

			// If we have a display frame available, fill it and send it to the UI
			if (m_displayFrames.size () && m_isDecoding)
			{
				auto frame = m_displayFrames.front ();
				m_displayFrames.pop ();
				m_decoder->fill (frame);
				//PostEvent ([this, frame]() 
				//{
				//	ntouchPC::CstiNativeVideoLink::DisplayFrame (frame);
				//});
			}
			else if (m_isDecoding)
			{
				m_discaredDisplayFrameCount++;
			}
			else
			{
				stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
					stiTrace ("CstiVideoOutput::VideoPlaybackFramePut Frame decoded after VideoPlaybackStop was called.\n");
				);
			}

			m_decodedFrameCount++;
		}
	}

	if (un8Flags & IstiVideoDecoder::FLAG_KEYFRAME)
	{
		gettimeofday (&m_lastIFrame, NULL);
	}

	timeval current_time;

	gettimeofday (&current_time, NULL);
	time_t iFrameDelta = current_time.tv_sec - m_lastIFrame.tv_sec;

	if ((un8Flags & IstiVideoDecoder::FLAG_IFRAME_REQUEST && iFrameDelta >= MIN_IFRAME_REQUEST_DELTA)
		|| iFrameDelta >= MAX_IFRAME_REQUEST_DELTA)
	{
		gettimeofday (&m_lastIFrame, NULL);

		// Notify that we need a keyframe
		keyframeNeededSignal.Emit ();

		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
			stiTrace ("\n\nIFRAME REQUEST. %d seconds.\n", iFrameDelta);
		);
	}

	VideoPlaybackFrameDiscard (videoFrame);

	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::VideoPlaybackSizeGet (uint32_t *pWidth, uint32_t *pHeight) const
{
	CstiOSMutexLock lock (m_decoderMutex);
	auto hResult = stiRESULT_SUCCESS;
	if (pWidth && pHeight)
	{
		*pWidth = m_decoder->widthGet ();
		*pHeight = m_decoder->heightGet ();
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool codecSet = false;
	m_decodedFrameCount = 0;
	m_discaredDisplayFrameCount = 0;

	CstiOSMutexLock lock (m_decoderMutex);
	m_isDecoding = true;
	switch (m_codec)
	{
		case estiVIDEO_H263:
			codecSet = m_decoder->codecSet (estiVIDEO_H263);
			break;

		case estiVIDEO_H264:
			codecSet = m_decoder->codecSet (estiVIDEO_H264);
			break;

		case estiVIDEO_H265:
			codecSet = m_decoder->codecSet (estiVIDEO_H265);
			break;

		case estiVIDEO_NONE:
			break;
	}

	stiTESTCOND (codecSet, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiOSMutexLock lock (m_decoderMutex);
	m_isDecoding = false;
	displayBlackFrame ();
	//ntouchPC::CstiNativeVideoLink::PlaybackStats (m_width, m_height, m_decodedFrameCount, m_discaredDisplayFrameCount, m_decoder->decoderDescription ());
	m_width = 0;
	m_height = 0;
	
	return hResult;
}


void CstiVideoOutput::testCodecSet (EstiVideoCodec testCodec)
{
	m_testCodec = testCodec;
}


void CstiVideoOutput::returnFrame (IVideoDisplayFrame* frame)
{
	PostEvent ([this, frame]() {
		m_displayFrames.push (frame);
	});
}


void CstiVideoOutput::displayBlackFrame ()
{
	PostEvent ([this]()
	{
		if (m_displayFrames.size ())
		{
			auto frame = m_displayFrames.front ();
			m_displayFrames.pop ();
			frame->clear ();
			//ntouchPC::CstiNativeVideoLink::DisplayFrame (frame);
		}
	});
}


bool CstiVideoOutput::IsHevcAllowed ()
{
	return false;
}


