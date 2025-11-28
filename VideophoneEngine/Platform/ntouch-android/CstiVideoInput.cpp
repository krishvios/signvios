//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

// Includes
//
#include "stiTrace.h"
#include "CstiVideoInput.h"

//
// Constant
//
#define stiVideoINPUT_MAX_MESSAGES_IN_QUEUE 12
#define stiVideoINPUT_MAX_MSG_SIZE 512
#define stiVideoINPUT_TASK_NAME "CstiVideoInputTask"
#define stiVideoINPUT_TASK_PRIORITY 151
#define stiVideoINPUT_STACK_SIZE 4096

// This was set to 3. (one to be encoding into, one to be sending, and one for good measure)
// When an empty frame data is not available we send a key frame.  And when a 1920x1080 key
// frame is encoded we are more likely to run out of these buffers.  This can cause a deluge
// of key frames.  Increasing to 12 buffers helps this issue.
#define stiVideoINPUT_NUM_FRAME_BUFFERS 12

////#define USE_LOCK_TRACKING
//#ifdef USE_LOCK_TRACKING
//int g_nLibLocks = 0;
//#define FFM_LOCK(A) \
//		stiTrace ("<SOFTWAREInput::Lock> Requesting lock from %s.  Lock = %p [%s]\n", A, m_LockMutex, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
//		Lock ();\
//		stiTrace ("<SOFTWAREInput::Lock> Locks = %d, Lock = %p.  Exiting from %s\n", ++g_nLibLocks, m_LockMutex, A);
//#define FFM_UNLOCK(A) \
//		stiTrace ("<SOFTWAREInput::Unlock> Freeing lock from %s, Lock = %p [%s]\n", A, m_LockMutex, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
//		Unlock (); \
//		stiTrace ("<SOFTWAREInput::Unlock> Locks = %d, Lock = %p.  Exiting from %s\n", --g_nLibLocks, m_LockMutex, A);
//#else
//#define FFM_LOCK(A) Lock ();
//#define FFM_UNLOCK(A) Unlock ();
//#endif

extern bool getVideoExchangeVideoOut( uint8_t *pTmp );
extern void SetVideoExchangeVideoOutRate( uint32_t rate );
extern void SetVideoExchangeVideoOutSize(uint32_t un32Width, uint32_t un32Height);
extern IstiVideoEncoder *getVideoEncoderFromJava(EstiVideoCodec eCodec);

bool CstiVideoInput::isMainSupported = false;
bool CstiVideoInput::isExtendedSupported = false;
EstiH264Level CstiVideoInput::h264Level = estiH264_LEVEL_1_3;


//stiEVENT_MAP_BEGIN(CstiVideoInput)
//				stiEVENT_DEFINE(estiVIDEO_INPUT_RECORD_START, CstiVideoInput::EventRecordStart)
//				stiEVENT_DEFINE(estiVIDEO_INPUT_RECORD_STOP, CstiVideoInput::EventRecordStop)
//stiEVENT_MAP_END(CstiVideoInput)
//stiEVENT_DO_NOW(CstiVideoInput)


CstiVideoInput::CstiVideoInput()
		:
		m_eventQueue("CstiVideoInput"),
		m_un32Width(0),
		m_un32Height(0),
		m_bInitialized(false),
		m_bKeyFrame(false),
		m_bPrivacy(false),
		m_bRecording(false),
		m_bShutdownMsgReceived(false),
		m_eVideoCodec(estiVIDEO_NONE),
		m_pVideoEncoder(nullptr),
		m_unFrameRate(0),
		m_unTargetBitrate(0),
		m_unIntraFramerate(0),
		m_unLevel(0),
		m_unConstraints(0),
		m_ePacketization(EstiPacketizationScheme::estiPktUnknown),
		m_eProfile(EstiVideoProfile::estiPROFILE_NONE),
		m_eTestVideoCodec (EstiVideoCodec::estiVIDEO_NONE)
{
	m_eventQueue.StartEventLoop();
#ifdef ACT_FRAME_RATE_METHOD
	for (int i = 0; i < 3; i++)
	{
		m_fActualFrameRateArray[i] = 30.0;
	}
#endif
}

CstiVideoInput::~CstiVideoInput()
{
	if (m_pVideoEncoder)
	{
		delete m_pVideoEncoder;
		m_pVideoEncoder = nullptr;
	}

	while (m_FrameData.size())
	{
		FrameData* pData = m_FrameData.front();
		m_FrameData.pop();
		delete pData;
	}

	while (m_FrameDataEmpty.size())
	{
		FrameData* pData = m_FrameDataEmpty.front();
		m_FrameDataEmpty.pop();
		delete pData;
	}
	m_eventQueue.StopEventLoop();
}

/*! \brief Register the callback used to notify the app that a frame is ready.
*
*  \retval stiRESULT_SUCCESS if success
*  \retval stiRESULT_ERROR otherwise
*/
stiHResult CstiVideoInput::CallbackRegister(PstiObjectCallback pfnCallback, size_t CallbackParam, size_t CallbackFromId)
{
	return stiRESULT_SUCCESS;
}

/*! \brief Unregister the callback used to notify the app that a frame is ready.
*
*  \retval stiRESULT_SUCCESS if success
*  \retval stiRESULT_ERROR otherwise
*/
stiHResult CstiVideoInput::CallbackUnregister(PstiObjectCallback pfnCallback, size_t CallbackParam)
{
	return stiRESULT_SUCCESS;
}

/*! \brief Event Handler for the estiSOFTWARE_INPUT_RECORD_START message
*
*  \retval estiOK if success
*  \retval estiERROR otherwise
*/
EstiResult CstiVideoInput::EventRecordStart()
{
	stiTrace("CstiVideoInput::EventRecordStart\n");
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiOSMutexLock threadSafe(m_EncoderMutex);

#ifdef ACT_FRAME_RATE_METHOD
	m_un32FrameCount = 0;
	m_dStartFrameTime = 0.0;
#endif

	if (m_pVideoEncoder)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose ();
			delete m_pVideoEncoder;
			m_pVideoEncoder = nullptr;
			hResult = CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
	}
	else
	{
		hResult = CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
		stiTESTRESULT ();
	}

	m_bRecording = true;

	hResult = UpdateEncoder ();

	STI_BAIL:

	return stiIS_ERROR(hResult) ? estiERROR : estiOK;
}

bool CstiVideoInput::IsRecording()
{
	return m_bRecording;
}

bool CstiVideoInput::AllowsFUPackets()
{
	return estiH264_NON_INTERLEAVED == m_ePacketization || estiH264_INTERLEAVED == m_ePacketization;
}

/*! \brief Event Handler for the estiSOFTWARE_INPUT_RECORD_STOP message
*
*  \retval estiOK if success
*  \retval estiERROR otherwise
*/
EstiResult CstiVideoInput::EventRecordStop()
{
	stiTrace("CstiVideoInput::EventRecordStop\n");
	EstiResult eResult = estiOK;
	CstiOSMutexLock threadSafe(m_EncoderMutex);
	m_bRecording = false;
	//--------Flush Frames----------
	while (m_FrameData.size())
	{
		FrameData *pData = m_FrameData.front();
		m_FrameData.pop();
		pData->Reset();
		m_FrameDataEmpty.push(pData);
	}

	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvEncoderClose ();
		delete m_pVideoEncoder;
		m_pVideoEncoder = nullptr;
	}

	return (eResult);
}

void CstiVideoInput::SendVideoFrame(uint8_t * pBytes, uint32_t nWidth, uint32_t nHeight, EstiColorSpace eColorSpace)
{
	// go ahead and lock encoder mutex until the message is ready to send
	// otherwise it's possible to obtain an image from the input queue
	// that doesn't match the encoder settings (width/height or other)
	CstiOSMutexLock threadSafe(m_EncoderMutex);
	if (nWidth == m_un32Width && nHeight == m_un32Height && m_bRecording)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		FrameData *pData = nullptr;
		{// scoped threadsafe

			CstiOSMutexLock threadSafe(m_FrameDataMutex);

			if (m_FrameDataEmpty.empty())
			{
				pData = nullptr;
			}
			else
			{
				pData = m_FrameDataEmpty.front();
				pData->Reset();
				m_FrameDataEmpty.pop();
			}
		}

		if (pData)
		{
			// Compress the frame.
			hResult = m_pVideoEncoder->AvVideoCompressFrame(pBytes, pData, eColorSpace);
			if (stiIS_ERROR(hResult))
			{
				CstiOSMutexLock threadSafe(m_FrameDataMutex);
				stiTrace("FAILED: m_pVideoEncoder->AvVideoCompressFrame(m_pInputBuffer,	pData) Err %d\n", hResult);
				m_FrameDataEmpty.push(pData);
				pData = nullptr;
			}
			else
			{
				{// scoped threadsafe
					CstiOSMutexLock threadSafe(m_FrameDataMutex);
					m_FrameData.push(pData);
				}
				stiEVENTPARAM param;

				//Notify CstiVideoRecord that a packet is ready.
				m_eventQueue.PostEvent([this]() {
					packetAvailableSignalGet().Emit();
				});

#ifdef ACT_FRAME_RATE_METHOD
				EncoderActualFrameRateCalcAndSet();
#endif
			}
		}
	}

	if (pBytes)
	{
		free(pBytes);
	}
}

stiHResult CstiVideoInput::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_EncoderMutex = stiOSMutexCreate(); // NOTE check valid?
	m_FrameDataMutex = stiOSMutexCreate();

	while (m_FrameDataEmpty.size() < stiVideoINPUT_NUM_FRAME_BUFFERS)
	{
		m_FrameDataEmpty.push(new FrameData());
	}

	m_bInitialized = true;

	STI_BAIL:
	return hResult;
}

stiHResult CstiVideoInput::KeyFrameRequest()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiOSMutexLock threadSafe(m_EncoderMutex);
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoRequestKeyFrame();
	}

	return hResult;
}

stiHResult CstiVideoInput::PrivacyGet(EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	*pbEnabled = m_bPrivacy ? estiTRUE:estiFALSE;
	return hResult;
}

stiHResult CstiVideoInput::PrivacySet(EstiBool bEnable)
{
	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
		videoPrivacySetSignal.Emit (bEnable);
	}

	return stiRESULT_SUCCESS;
}


#ifdef ACT_FRAME_RATE_METHOD
/*! \brief	Called each time a frame is ready, calculates the actual frame rate the camera is delivering and sets
*			the actual frame rate member variable in CstiVideoEncoder. This is used as a ratio with the target
*			frame rate to calculate bit rate in AvUpdateEncoderSettings() which is called from AvInitializeEncoder().
*/
void CstiVideoInput::EncoderActualFrameRateCalcAndSet()
{
	CstiOSMutexLock threadSafe(m_EncoderMutex);

	timeval ts;

	m_un32FrameCount++;
	if (m_un32FrameCount % 30 == 0)
	{
		gettimeofday(&ts, nullptr);
		double dCurrentTime = (ts.tv_sec & 0xFFFF) /* 16 bits only */ + (double)ts.tv_usec / 1e6;
		if (m_dStartFrameTime != 0.0)
		{
			float fFrameRateAverage = 0.0;
			for (int i = 2; i>0; i--)
			{
				m_fActualFrameRateArray[i] = m_fActualFrameRateArray[i - 1];
				fFrameRateAverage += m_fActualFrameRateArray[i];
			}
			m_fActualFrameRateArray[0] = 30.0 / (float)(dCurrentTime - m_dStartFrameTime);
			fFrameRateAverage += m_fActualFrameRateArray[0];
			fFrameRateAverage /= 3.0;

			float fRateDiff = (fFrameRateAverage > m_fCurrentEncoderRate) ?
							  fFrameRateAverage - m_fCurrentEncoderRate : m_fCurrentEncoderRate - fFrameRateAverage;
			if (fRateDiff >= 1.5)
			{
				m_fCurrentEncoderRate = fFrameRateAverage;
				if (m_pVideoEncoder)
				{
					m_pVideoEncoder->AvVideoActualFrameRateSet(m_fCurrentEncoderRate);
					m_pVideoEncoder->AvInitializeEncoder();
				}
			}
		}

		m_dStartFrameTime = dCurrentTime;
	}
}
#endif //ACT_FRAME_RATE_METHOD


stiHResult CstiVideoInput::Uninitialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	{
		CstiOSMutexLock threadSafe(m_EncoderMutex);
		if (m_pVideoEncoder)
		{
			m_pVideoEncoder->AvEncoderClose();
			delete m_pVideoEncoder;
			m_pVideoEncoder = nullptr;
		}
	}

	stiOSMutexDestroy(m_EncoderMutex);

	return hResult;
}

stiHResult CstiVideoInput::VideoRecordFrameGet(SstiRecordVideoFrame *pVideoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiOSMutexLock encoderLock(m_EncoderMutex);
	CstiOSMutexLock threadSafe(m_FrameDataMutex);

	FrameData *pData = nullptr;
	stiTESTCOND(m_FrameData.size() > 0, stiRESULT_DRIVER_ERROR);
	pData = m_FrameData.front();
	stiTESTCOND(pVideoPacket != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND(pVideoPacket->buffer != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND(pVideoPacket->unBufferMaxSize >= pData->GetDataLen(), stiRESULT_BUFFER_TOO_SMALL);

	pData->GetFrameData(pVideoPacket->buffer);
	pVideoPacket->frameSize = pData->GetDataLen();
	pVideoPacket->keyFrame = pData->IsKeyFrame() ? estiTRUE : estiFALSE;

	if (m_pVideoEncoder)
	{
		pVideoPacket->eFormat = m_pVideoEncoder->AvFrameFormatGet();
	}
	else
	{
		pVideoPacket->eFormat = estiLITTLE_ENDIAN_PACKED;
	}


	STI_BAIL:
	if (pData)
	{
		m_FrameData.pop();
		m_FrameDataEmpty.push(pData);
	}
	return hResult;
}

stiHResult CstiVideoInput::UpdateEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoFrameSizeSet(m_un32Width, m_un32Height);

		m_pVideoEncoder->AvVideoBitrateSet(m_unTargetBitrate);

		m_pVideoEncoder->AvVideoFrameRateSet((float) m_unFrameRate);

		m_pVideoEncoder->AvVideoIFrameSet(m_unIntraFramerate);

		m_pVideoEncoder->AvVideoProfileSet(m_eProfile, m_unLevel, m_unConstraints);

		hResult = m_pVideoEncoder->AvInitializeEncoder();

		if(hResult == stiRESULT_ERROR) {
			hResult = stiMAKE_ERROR(hResult);
		}
	}

	return hResult;
}

stiHResult CstiVideoInput::VideoRecordSettingsSet(const SstiVideoRecordSettings *videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("VideoRecordSettingsSet\n");

	CstiOSMutexLock threadSafe(m_EncoderMutex);
	m_un32Width = videoRecordSettings->unColumns;
	m_un32Height = videoRecordSettings->unRows;

	SetVideoExchangeVideoOutSize(m_un32Width, m_un32Height);
	m_ePacketization = videoRecordSettings->ePacketization;
	m_unFrameRate = videoRecordSettings->unTargetFrameRate;
	m_unTargetBitrate = videoRecordSettings->unTargetBitRate;
	m_unIntraFramerate = videoRecordSettings->unIntraFrameRate;
	m_eProfile = videoRecordSettings->eProfile;
	m_unLevel = videoRecordSettings->unLevel;
	m_unConstraints = videoRecordSettings->unConstraints;
    if (estiVIDEO_H264 == videoRecordSettings->codec ||
    	estiVIDEO_H265 == videoRecordSettings->codec)
	{
		m_eVideoCodec = videoRecordSettings->codec;
	}
    else
	{
		m_eVideoCodec = estiVIDEO_NONE;
	}

	if (m_pVideoEncoder != nullptr)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			hResult = ResetEncoder();
			stiTESTRESULT ();
		}
		hResult = UpdateEncoder ();
		if(stiIS_ERROR(hResult))
		{
			hResult = ResetEncoder();
			stiTESTRESULT();

			hResult = UpdateEncoder ();
			stiTESTRESULT();
		}
	}

	SetVideoExchangeVideoOutRate (m_unFrameRate);



	STI_BAIL:

	return hResult;
}

stiHResult CstiVideoInput::ResetEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_pVideoEncoder->AvEncoderClose ();
	delete m_pVideoEncoder;
	m_pVideoEncoder = nullptr;
	hResult = CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;
//	CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_RECORD_START);
//
//	CstiOSMutexLock threadSafe(m_EncoderMutex);
//
//	stiTESTCOND(m_eVideoCodec != estiVIDEOCODEC_NONE, estiERROR);
//
//	hResult = MsgSend(&oEvent, sizeof(oEvent));

	m_eventQueue.PostEvent(std::bind(&CstiVideoInput::EventRecordStart, this));

	stiTrace("VideoRecordStart\n");

	STI_BAIL:
	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStop()
{
	stiHResult hResult = stiRESULT_SUCCESS;

//	stiTrace("VideoRecordStop\n");
//
//	CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_RECORD_STOP);

	m_eventQueue.PostEvent(std::bind(&CstiVideoInput::EventRecordStop, this));

	return hResult;
}

/*!
* \brief Get video codec
*
* \param pCodecs - a std::list where the codecs will be stored.
*
* \return stiHResult
*/
stiHResult CstiVideoInput::VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs)
{
	if (estiVIDEO_NONE == m_eTestVideoCodec)
	{
		// Add the supported video codecs to the list
#ifdef stiENABLE_H265_ENCODE
		WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance ();
		int nEnableH265 = NULL;
		pm->propertyGet ("EnableH265", &nEnableH265, 0, WillowPM::PropertyManager::Persistent);
		if (nEnableH265 == 1)
		{
			pCodecs->push_back (estiVIDEO_H265);
		}
#endif
		pCodecs->push_back (estiVIDEO_H264);
	}
	else
	{
		pCodecs->push_back (m_eTestVideoCodec);
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoInput::TestVideoCodecSet (EstiVideoCodec codec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eTestVideoCodec = codec;
	return hResult;
}

/*!
 * \brief Get Codec Capability
 *
 * \param eCodec
 * \param pstCaps
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::CodecCapabilitiesGet (
                                                 EstiVideoCodec eCodec,
                                                 SstiVideoCapabilities *pstCaps)
{
    stiHResult hResult = stiRESULT_SUCCESS;
    SstiProfileAndConstraint stProfileAndConstraint;
    
    switch (eCodec)
    {
		case estiVIDEO_H264:
		{
//			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
//
//			//Extended is not a profile in FFMPEG
//			//stProfileAndConstraint.eProfile = estiH264_EXTENDED;
//			//stProfileAndConstraint.un8Constraints = 0xf0;
//			//pstH264Caps->Profiles.push_back(stProfileAndConstraint);
//
//			stProfileAndConstraint.eProfile = estiH264_MAIN;
//			stProfileAndConstraint.un8Constraints = 0xe0;
//			pstH264Caps->Profiles.push_back(stProfileAndConstraint);
//
//			stProfileAndConstraint.eProfile = estiH264_BASELINE;
//			stProfileAndConstraint.un8Constraints = 0xa0;
//			pstH264Caps->Profiles.push_back(stProfileAndConstraint);
//
//			pstH264Caps->eLevel = estiH264_LEVEL_4_1;

			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

			if(isExtendedSupported)
			{
				stProfileAndConstraint.eProfile = estiH264_EXTENDED;
				stProfileAndConstraint.un8Constraints = 0xf0;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			}

			if(isMainSupported)
			{
				stProfileAndConstraint.eProfile = estiH264_MAIN;
				stProfileAndConstraint.un8Constraints = 0xe0;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			}

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xa0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = h264Level;

			break;
		}

		case estiVIDEO_H265:
		{
			if (m_bH265Enabled)
			{
//				SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;
//
//				stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
//				stProfileAndConstraint.un8Constraints = 0x00;
//				pstH265Caps->Profiles.push_back(stProfileAndConstraint);
//				pstH265Caps->eLevel = estiH265_LEVEL_4;
//				pstH265Caps->eTier = estiH265_TIER_MAIN;

				SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;

				stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
				stProfileAndConstraint.un8Constraints = 0x00;
				pstH265Caps->Profiles.push_back(stProfileAndConstraint);
				pstH265Caps->eLevel = estiH265_LEVEL_4;
				pstH265Caps->eTier = estiH265_TIER_MAIN;

				break;
			}
			break;
		}

		default:
			hResult = stiRESULT_ERROR;
	}

	return hResult;
}

stiHResult CstiVideoInput::CaptureCapabilitiesChanged ()
{
	captureCapabilitiesChangedSignal.Emit ();
	return stiRESULT_SUCCESS;
}

stiHResult CstiVideoInput::CreateVideoEncoder(IstiVideoEncoder **pEncoder, EstiVideoCodec eCodec)
{
	switch (eCodec)
	{
		case estiVIDEO_H265:
		case estiVIDEO_H264:
			if(AllowsFragmentationUnits()){
				*pEncoder = getVideoEncoderFromJava(eCodec);
				if (*pEncoder != NULL)
				{
					return stiRESULT_SUCCESS;
				}
			}
			break;
	}

	return IstiVideoEncoder::CreateVideoEncoder(pEncoder, eCodec);
}

bool CstiVideoInput::AllowsFragmentationUnits()
{
	return estiH264_NON_INTERLEAVED == m_ePacketization || estiH264_INTERLEAVED ==m_ePacketization || estiH265_NON_INTERLEAVED == m_ePacketization || estiPktNone == m_ePacketization;
}

