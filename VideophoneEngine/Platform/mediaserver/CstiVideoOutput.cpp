#include "CstiVideoOutput.h"

#define SLICE_TYPE_NON_IDR	1
#define SLICE_TYPE_IDR		5
#define SLICE_TYPE_SPS		7
#define SLICE_TYPE_PPS		8

std::unordered_map<uint32_t, std::shared_ptr<CstiVideoOutput>> CstiVideoOutput::m_videoOutputs;

CstiVideoOutput::CstiVideoOutput(uint32_t callIndex) : CstiVideoOutput ()
{
	m_videoContainer.callIndex = callIndex;
}

CstiVideoOutput::CstiVideoOutput ()
{
	m_videoLockMutex = stiOSMutexCreate();
}

CstiVideoOutput::~CstiVideoOutput ()
{
	// close Signal 
	if (m_signal)
	{
		stiOSSignalClose (&m_signal);
		m_signal = nullptr;
	}

	//m_videoContainer.videoData.clear ();

	stiOSMutexDestroy(m_videoLockMutex);
}


stiHResult CstiVideoOutput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (NULL == m_signal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_signal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);
	}

STI_BAIL:

	return hResult;
}

void CstiVideoOutput::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	memset (pDisplaySettings, 0, sizeof (SstiDisplaySettings));
}


stiHResult CstiVideoOutput::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiVideoOutput::RemoteViewHoldSet (
	EstiBool bHold)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackSizeGet (
	uint32_t *punWidth,
	uint32_t *punHeight) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*punWidth = 0;
	*punHeight = 0;

	return hResult;
}

void CstiVideoOutput::RequestKeyFrame()
{
	keyframeNeededSignal.Emit ();
}

EstiVideoCodec CstiVideoOutput::VideoPlaybackCodecGet()
{
	return m_videoContainer.codec;
}

stiHResult CstiVideoOutput::VideoPlaybackCodecSet(EstiVideoCodec videoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_videoContainer.codec = videoCodec;
	switch (m_videoContainer.codec)
	{
		case estiVIDEO_H264:
			printf("Recording H264\r\n");
		break;
		case estiVIDEO_H263:
			printf("Recording H263\r\n");
		break;
		default:
			return stiRESULT_ERROR;
	}
	
	return hResult;
}

bool CstiVideoOutput::writePacket(std::shared_ptr<SstiMSPacket> packet)
{
	if (packet->length > 0)
	{
		//Clear the packet queue if we get a new keyframe before we start recording
		if (!m_isSignMailRecording && packet->keyFrame)
		{
			m_videoContainer.videoData.clear ();
		}
		m_videoContainer.videoData.push_back (packet);
		return true;
	}
	//-----------------------------
	return false;
}

void CstiVideoOutput::BeginRecordingFrames()
{
	CstiVideoCallback::BeginWrite(m_videoContainer.callIndex);
	m_videoContainer.discardedFrames = m_videoContainer.videoData.size ();
	m_isSignMailRecording = true;

	// Notify that we need a keyframe
	keyframeNeededSignal.Emit ();
}

stiHResult CstiVideoOutput::VideoPlaybackStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_isRecording = true;
	return hResult;
}

stiHResult CstiVideoOutput::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_isRecording = false;
	CstiVideoCallback::EndWrite(m_videoContainer);
	return hResult;
}

stiHResult CstiVideoOutput::VideoPlaybackFrameGet(IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_isRecording)
	{
		*ppVideoFrame = new CstiVideoPlaybackFrame();
	}
	else
	{
		*ppVideoFrame = &m_videoParsingFrame;
	}

STI_BAIL:

	return hResult;
}
	
void CstiVideoOutput::bufferCleanup (uint8_t *pBuffer, uint32_t un32BufferLength)
{
	uint32_t un32SliceSize;
	//uint32_t un32SliceType;
	for (int i = 0; i < un32BufferLength;)
	{
		un32SliceSize = *(uint32_t*)&pBuffer[i];
		un32SliceSize = stiDWORD_BYTE_SWAP (un32SliceSize);
		//un32SliceType = (pBuffer[i + sizeof (uint32_t)] & 0x1F);
		pBuffer[i + 0] = 0x0;
		pBuffer[i + 1] = 0x0;
		pBuffer[i + 2] = 0x0;
		pBuffer[i + 3] = 0x1;
		i += sizeof (uint32_t) + un32SliceSize;
	}
}

stiHResult CstiVideoOutput::VideoPlaybackFramePut(IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiVideoPlaybackFrame *pFrame = (CstiVideoPlaybackFrame *)pVideoFrame;
	uint8_t *pBuffer = pFrame->BufferGet();
	uint32_t un32BufferLength = pFrame->DataSizeGet();
	if (pFrame != &m_videoParsingFrame)
	{
		bool bKeyFrame = pVideoFrame->FrameIsKeyframeGet ();
		switch (m_videoContainer.codec)
		{
			case estiVIDEO_H264:
			{
				bufferCleanup(pBuffer, un32BufferLength);
			}
			break;
			case estiVIDEO_H263:
			{
				//bufferCleanup (pBuffer, un32BufferLength);
			}
			break;
			default:
			{
				stiASSERT(estiFALSE);
			}
			break;
		}

		if (bKeyFrame)
		{
			if (pVideoFrame->FrameIsCompleteGet ())
			{
				m_hasKeyFrame = true;
			}
		}
		if (m_hasKeyFrame)
		{
			auto packet = new SstiMSPacket ();
			std::shared_ptr<SstiMSPacket> packetData = std::shared_ptr<SstiMSPacket> (packet, [this](SstiMSPacket *packet) {deleter (packet); });
	
			packetData->buffer = pBuffer; //this will be deleted later
			packetData->length = un32BufferLength;
			packetData->keyFrame = bKeyFrame;
			packetData->clockTime = clock ();
			writePacket (packetData);
		}
		else
		{
			delete pBuffer;
		}

		delete pFrame;
	}
	return hResult;
}

stiHResult CstiVideoOutput::VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto videoFrameDiscard = static_cast<CstiVideoPlaybackFrame*>(pVideoFrame);
	if (videoFrameDiscard != (&m_videoParsingFrame))
	{
		delete videoFrameDiscard->BufferGet ();
		delete videoFrameDiscard;
	}
	return hResult;
}

int CstiVideoOutput::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_signal);
}

///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.  If the preferred codec is to be returned, it should be placed in the
///\brief front of the list.
stiHResult CstiVideoOutput::VideoCodecsGet(
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	pCodecs->push_back(estiVIDEO_H264);
	pCodecs->push_back(estiVIDEO_H263);

	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiVideoOutput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)		// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back(stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xa0;
			pstH264Caps->Profiles.push_back(stProfileAndConstraint);
			pstH264Caps->eLevel = estiH264_LEVEL_2_1;
			break;
		}

		default:
			hResult = stiRESULT_ERROR;
	}

	return hResult;
}

///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiVideoOutput::CodecCapabilitiesSet(
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	//These values do not change
	return stiRESULT_SUCCESS;
}

void CstiVideoOutput::ScreenshotTake (EstiScreenshotType eType)
{
}

void CstiVideoOutput::PreferredDisplaySizeGet(
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	*displayWidth = 0;
	*displayHeight = 0;
}

void CstiVideoOutput::instanceRelease ()
{
	m_videoOutputs.erase (m_videoContainer.callIndex);
}

bool CstiVideoOutput::hasKeyFrameGet ()
{
	return m_hasKeyFrame;
}

void CstiVideoOutput::deleter (SstiMSPacket *packet)
{
	delete packet->buffer;
	delete packet;
}

IstiVideoOutput *IstiVideoOutput::instanceCreate (uint32_t callIndex)
{
	auto outputInstance = std::make_shared<CstiVideoOutput> (callIndex);
	outputInstance->Initialize ();
	CstiVideoOutput::m_videoOutputs[callIndex] = outputInstance;
	CstiVideoCallback::AssociateOutput (callIndex, outputInstance);
	return outputInstance.get();
}

