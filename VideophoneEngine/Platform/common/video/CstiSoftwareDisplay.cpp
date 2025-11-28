//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved

#include "stiTrace.h"
#include "CstiSoftwareDisplay.h"

#define SLICE_TYPE_NON_IDR	1
#define SLICE_TYPE_IDR		5


#define	stiVIDEODISPLAY_MAX_MESSAGES_IN_QUEUE 35
#define	stiVIDEODISPLAY_MAX_MSG_SIZE 512
#define	stiVIDEODISPLAY_TASK_NAME "CstiDisplayTask"
#define	stiVIDEODISPLAY_TASK_PRIORITY 151
#define	stiVIDEODISPLAY_STACK_SIZE 4096

#define MAX_PLAYBACK_FRAMES 30
#define START_PLAYBACK_FRAMES 5

#define MAX_FRAME_BUFFER_SIZE 1920 * 1080 * 3 / 2

uint32_t CstiVideoPlaybackFrame::m_un32BufferCount=0;

stiEVENT_MAP_BEGIN (CstiSoftwareDisplay)
	stiEVENT_DEFINE (estiVP_DELIVER_FRAME, CstiSoftwareDisplay::EventDeliverFrame)
stiEVENT_MAP_END (CstiSoftwareDisplay)
stiEVENT_DO_NOW (CstiSoftwareDisplay)

CstiSoftwareDisplay::CstiSoftwareDisplay ()
:
	CstiOsTaskMQ (
		NULL,
		0,
		(size_t)this,
		stiVIDEODISPLAY_MAX_MESSAGES_IN_QUEUE,
		stiVIDEODISPLAY_MAX_MSG_SIZE,
		stiVIDEODISPLAY_TASK_NAME,
		stiVIDEODISPLAY_TASK_PRIORITY,
		stiVIDEODISPLAY_STACK_SIZE),
		m_fdSignal (NULL),
		m_eVideoCodec (estiVIDEO_H264),
		m_pVideoDecoder(NULL),
		m_decoding(false),
		m_convertNalUnits(true),
		m_swapSizeBytes(true),
		m_width(0),
		m_height(0)
{
	m_DecoderMutex = stiOSMutexCreate();
}

CstiSoftwareDisplay::~CstiSoftwareDisplay() {
	stiOSMutexDestroy(m_DecoderMutex);
	FreeFrames(0);
}

void CstiSoftwareDisplay::FreeFrames(int max) {
	while (m_availableFrames.size() > max) {
		IstiVideoPlaybackFrame *pFrame = m_availableFrames.front();
		m_availableFrames.pop();
		delete (CstiVideoPlaybackFrame*)pFrame;
	}
}

stiHResult CstiSoftwareDisplay::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = InitDecoder();
	stiTESTCOND(!stiIS_ERROR(hResult), stiRESULT_ERROR);

	if (NULL == m_fdSignal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_fdSignal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);

		stiOSSignalSet (m_fdSignal);
	}
	
	for (int i=0;i<START_PLAYBACK_FRAMES;++i) 
		m_availableFrames.push ( AllocateVideoPlaybackFrame() );
	
STI_BAIL:

	return hResult;
}

stiHResult CstiSoftwareDisplay::InitDecoder()
{
	return  stiRESULT_SUCCESS;
}

void CstiSoftwareDisplay::Uninitialize ()
{
	// close Signal 
	if (m_fdSignal)
	{
		stiOSSignalClose (&m_fdSignal);
		m_fdSignal = NULL;
	}
	if (m_pVideoDecoder) 
	{
		m_pVideoDecoder->AvDecoderClose();
		delete m_pVideoDecoder;
		m_pVideoDecoder = NULL;
	}
}


IstiVideoPlaybackFrame *CstiSoftwareDisplay::AllocateVideoPlaybackFrame()
{
    return new CstiVideoPlaybackFrame;
}

int CstiSoftwareDisplay::Task () {
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiVIDEODISPLAY_MAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		// read a message from the message queue
		hResult = MsgRecv (
			(char *)un32Buffer,
			stiVIDEODISPLAY_MAX_MSG_SIZE,
			stiWAIT_FOREVER);

		// Was a message received?
		if (stiIS_ERROR (hResult))
		{
			Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
		}
		else
		{
			// Yes! Process the message.
			CstiEvent *poEvent = (CstiEvent*)un32Buffer;

			if(estiVP_DELIVER_FRAME == poEvent->EventGet()) {
				// nolock method
				hResult = EventDoNow (poEvent);
				if (stiIS_ERROR (hResult))
				{
					Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
				}
			}

		} // end if
	} // end for

	return (0);
}




void CstiSoftwareDisplay::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	memset (pDisplaySettings, 0, sizeof (SstiDisplaySettings));
}


stiHResult CstiSoftwareDisplay::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiSoftwareDisplay::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	CstiEvent oEvent(estiVP_DELIVER_FRAME, NULL);
	return MsgSend(&oEvent, sizeof(oEvent));
}


stiHResult CstiSoftwareDisplay::RemoteViewHoldSet (
	EstiBool bHold)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiSoftwareDisplay::VideoPlaybackSizeGet (
	uint32_t *punWidth,
	uint32_t *punHeight) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*punWidth = m_width;
	*punHeight = m_height;

	return hResult;
}


stiHResult CstiSoftwareDisplay::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{

	switch (eVideoCodec) 
	{
		case estiVIDEO_H263:
		case estiVIDEO_H264:
		case estiVIDEO_H265:
			m_eVideoCodec = eVideoCodec;
			break;
		default:
			return stiRESULT_INVALID_CODEC;
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiSoftwareDisplay::VideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock thread_safe(m_DecoderMutex);
	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ( "VideoPlaybackStart\n" );
	);
	
	if (m_pVideoDecoder)
	{
		m_pVideoDecoder->AvDecoderClose();
		delete m_pVideoDecoder;
		m_pVideoDecoder = NULL;
	}
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
	if (m_eVideoCodec != estiVIDEO_H265 && (VTDecompressionSessionCreate != NULL && CMVideoFormatDescriptionCreateFromH264ParameterSets != NULL))

	{
		m_convertNalUnits = false;
		CstiVTDecoder::CreateVTDecoder(&m_pVideoDecoder);
	}
	else
#endif
	{
		m_convertNalUnits = true;
		CstiFFMpegDecoder::CreateFFMpegDecoder(&m_pVideoDecoder);
	}
	hResult = m_pVideoDecoder->AvDecoderInit(m_eVideoCodec);
	
	stiTESTRESULT();
	gettimeofday(&m_lastIFrame,NULL);
	m_width = 0;
	m_height = 0;

	m_decoding=true;
STI_BAIL:
	return hResult;
}


stiHResult CstiSoftwareDisplay::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiOSMutexLock thread_safe(m_DecoderMutex);
	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
				   stiTrace ( "VideoPlaybackStop\n" );
				   );

	m_pVideoDecoder->AvDecoderClose();
	
	m_decoding=false;

    std::lock_guard<std::recursive_mutex> lock_safe ( m_LockMutex );
	FreeFrames(START_PLAYBACK_FRAMES);

	return hResult;
}

stiHResult CstiSoftwareDisplay::VideoPlaybackFrameGet (
	IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	IstiVideoPlaybackFrame *pFrame = NULL;
	{
		std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );
		if (m_availableFrames.size()>0) {
			pFrame = (IstiVideoPlaybackFrame*)m_availableFrames.front();
			*ppVideoFrame = pFrame;
			m_availableFrames.pop();
		}
	
		else if (CstiVideoPlaybackFrame::CreatedGet()<MAX_PLAYBACK_FRAMES) {
			// in this case ok to create another one
			pFrame = AllocateVideoPlaybackFrame();
			stiTESTCOND(pFrame!=NULL, stiRESULT_MEMORY_ALLOC_ERROR);
			*ppVideoFrame = pFrame;
		} 
	} // unlock mutex

	if (!pFrame) {
		hResult = stiRESULT_ERROR;
		stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
			stiTrace ( "Warning, no video playback buffers available\n" );
		);
	}
	
STI_BAIL:
	
	return hResult;
}
	

stiHResult CstiSoftwareDisplay::VideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	
	CstiVideoPlaybackFrame *pFrame = (CstiVideoPlaybackFrame *)pVideoFrame;
	uint32_t un32BufferLength = pFrame->DataSizeGet();
	//static uint8_t *pTmp = p//new uint8_t[704*480*3/2];// TMP
	//memcpy(pTmp, pFrame->BufferGet(), un32BufferLength);
	uint8_t *pBuffer = pFrame->BufferGet();
	
	switch (m_eVideoCodec)
	{
		case estiVIDEO_H263:
		{
			CstiEvent oEvent (estiVP_DELIVER_FRAME, (stiEVENTPARAM)pVideoFrame);
			hResult = MsgSend (&oEvent, sizeof (oEvent));
			break;
		}
		case estiVIDEO_H264:
		{
			uint32_t un32SliceSize;
			//uint32_t un8SliceType;
			//stiTrace ( "Received an H264 frame buffer len: %d.\n", un32BufferLength);
			
			while (m_convertNalUnits && un32BufferLength > 5)
			{
				un32SliceSize = *(uint32_t *)pBuffer;
				if (m_swapSizeBytes) {
					un32SliceSize = stiDWORD_BYTE_SWAP (un32SliceSize);
				}
				
				pBuffer[0] = 0;
				pBuffer[1] = 0;
				pBuffer[2] = 0;
				pBuffer[3] = 1;
				
				pBuffer += sizeof (uint32_t) + un32SliceSize;
				un32BufferLength -= sizeof (uint32_t) + un32SliceSize;
				//stiTrace ( "Slice bytes: %d, keyframe: %d\n", un32SliceSize, bKeyFrame ? 1 : 0 );
			}
			CstiEvent oEvent ( estiVP_DELIVER_FRAME, (stiEVENTPARAM)pVideoFrame );
			hResult = MsgSend( &oEvent, sizeof(oEvent) );
			break;
		}
			
		case estiVIDEO_H265:
		{
			uint32_t un32SliceSize;
			//uint32_t un8SliceType;
			//stiTrace ( "Received an H264 frame buffer len: %d.\n", un32BufferLength);
			
			while (m_convertNalUnits && un32BufferLength > 5)
			{
				un32SliceSize = *(uint32_t *)pBuffer;
				if (m_swapSizeBytes) {
					un32SliceSize = stiDWORD_BYTE_SWAP (un32SliceSize);
				}
				
				pBuffer[0] = 0;
				pBuffer[1] = 0;
				pBuffer[2] = 0;
				pBuffer[3] = 1;
				
				pBuffer += sizeof (uint32_t) + un32SliceSize;
				un32BufferLength -= sizeof (uint32_t) + un32SliceSize;
				//stiTrace ( "Slice bytes: %d, keyframe: %d\n", un32SliceSize, bKeyFrame ? 1 : 0 );
			}
			CstiEvent oEvent ( estiVP_DELIVER_FRAME, (stiEVENTPARAM)pVideoFrame );
			hResult = MsgSend( &oEvent, sizeof(oEvent) );
			break;
		}

		default:
			
			stiASSERT (estiFALSE);
			
			break;
	}

	
	return hResult;
}


stiHResult CstiSoftwareDisplay::VideoPlaybackFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );
	if (m_decoding) {
		m_availableFrames.push(pVideoFrame);
	} else {
		delete (CstiVideoPlaybackFrame*)pVideoFrame;
	}
	
	return hResult;
}


int CstiSoftwareDisplay::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_fdSignal);
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.  If the preferred codec is to be returned, it should be placed in the
///\brief front of the list.
stiHResult CstiSoftwareDisplay::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
#ifdef stiENABLE_H265_DECODE
	pCodecs->push_back(estiVIDEO_H265);
#endif

	pCodecs->push_back (estiVIDEO_H264);
	pCodecs->push_back (estiVIDEO_H263);

	return stiRESULT_SUCCESS;
}



///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.

stiHResult CstiSoftwareDisplay::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)		// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	SstiVideoCapabilities* pstLocalCaps = CodecCapabilitiesLookup[eCodec];

	//This next peice of code is the failover in case the CodecCapabilites aren't set.
	if (!pstLocalCaps)
	{
		switch (eCodec)
		{
			case estiVIDEO_H263:
			{
				stProfileAndConstraint.eProfile = estiH263_ZERO;
				pstCaps->Profiles.push_back (stProfileAndConstraint);
				break;
			}
			case estiVIDEO_H264:
			{
				SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
				stProfileAndConstraint.eProfile = estiH264_BASELINE;
				stProfileAndConstraint.un8Constraints = 0xa0;
				pstH264Caps->Profiles.push_back(stProfileAndConstraint);
				pstH264Caps->eLevel = estiH264_LEVEL_4;
				break;
			}
			default:
			{
				stiMAKE_ERROR(stiRESULT_ERROR);
				break;
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
		}
		//Copy the profiles
		std::list<SstiProfileAndConstraint>::iterator iter;
		for (iter = pstLocalCaps->Profiles.begin(); iter != pstLocalCaps->Profiles.end(); iter++)
		{
			SstiProfileAndConstraint stCopyData;
			memcpy(&stCopyData, &(*iter), sizeof(SstiProfileAndConstraint));
			pstCaps->Profiles.push_back(stCopyData);
		}
		
	}

	
	return hResult;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiSoftwareDisplay::CodecCapabilitiesSet(
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	CodecCapabilitiesLookup[eCodec] = pstCaps;
	return stiRESULT_SUCCESS;
}


void CstiSoftwareDisplay::PreferredDisplaySizeGet (
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	*displayWidth = 0;
	*displayHeight = 0;
}


stiHResult CstiSoftwareDisplay::ConvertNalUnitsSet(bool convertNalUnits)
{
	m_convertNalUnits = convertNalUnits;
	return stiRESULT_SUCCESS;
}

stiHResult CstiSoftwareDisplay::SwapSizeBytesSet(bool swapSizeBytes)
{
	m_swapSizeBytes = swapSizeBytes;
	return stiRESULT_SUCCESS;
}


#if DEVICE == DEV_X86
void CstiSoftwareDisplay::ScreenshotTake (EstiScreenshotType eType)
{
}
#endif

//
// Event Handlers
//

EstiResult CstiSoftwareDisplay::EventDeliverFrame (IN void* poEvent) 
{
	uint8_t un8Flags=0;
	timeval current_time;	
	static uint8_t pTmp[MAX_FRAME_BUFFER_SIZE]; // NOTE fix
	stiHResult hResult = stiRESULT_INVALID_MEDIA; // not valid until retreived from decoder
	uint32_t w,h;

	CstiVideoPlaybackFrame *pFrame = (CstiVideoPlaybackFrame*)((CstiEvent*)poEvent)->ParamGet();

	if (pFrame)
	{
		CstiOSMutexLock decoder_safe(m_DecoderMutex);
		
			
		if (m_decoding) 
		{
			m_pVideoDecoder->AvDecoderDecode(pFrame, &un8Flags);
			{
				std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );
				m_availableFrames.push(pFrame);
			}
		}
		else
		{
			delete pFrame; // don't save between calls
			return estiOK;
		}

		if (un8Flags & IstiVideoDecoder::FLAG_FRAME_COMPLETE)
		{
			hResult = m_pVideoDecoder->AvDecoderPictureGet(pTmp, MAX_FRAME_BUFFER_SIZE, &w, &h); // tmp
		}
	} // unlock decoder mutex
	else
	{
		if (m_width != 0 || m_height != 0)
		{
			uint8_t * blackFrameData = pTmp;
			int frameDimensions = m_width * m_height;
			int quarterFrameDimesnions = frameDimensions >> 2;
			memset(blackFrameData, 0, frameDimensions);
			blackFrameData = blackFrameData + frameDimensions;
			memset(blackFrameData, 128, quarterFrameDimesnions);
			blackFrameData += quarterFrameDimesnions;
			memset(blackFrameData, 128, quarterFrameDimesnions);
			DisplayYUVImage(pTmp, m_width, m_height);
			return estiOK;
		}
	}
	
	if (stiRESULT_SUCCESS == hResult) 
	{
		DisplayYUVImage ( pTmp, w,h );
		if ((w != m_width) || (h != m_height))
		{
			m_width = w;
			m_height = h;

			decodeSizeChangedSignalGet().Emit (w, h);
		}
	}
			
	if (un8Flags & IstiVideoDecoder::FLAG_KEYFRAME) 
	{
		gettimeofday(&m_lastIFrame,NULL);
	}

	gettimeofday(&current_time,NULL);
	time_t iFrameDelta = current_time.tv_sec - m_lastIFrame.tv_sec;
	
	if ((un8Flags & IstiVideoDecoder::FLAG_IFRAME_REQUEST && iFrameDelta >= MIN_IFRAME_REQUEST_DELTA)
			|| iFrameDelta >= MAX_IFRAME_REQUEST_DELTA)
	{
		gettimeofday(&m_lastIFrame,NULL);

		// Notify that we need a keyframe
		keyframeNeededSignalGet().Emit ();

		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
					   stiTrace ( "\n\nIFRAME REQUEST. %d seconds.\n", iFrameDelta );
		);
	}
	
	return estiOK;
}
