//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

// Includes
//
#include "stiDebugTools.h"
#include "CstiVideoInput.h"

//#include "CstiNativeVideoLink.h"

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

//#define USE_LOCK_TRACKING
#ifdef USE_LOCK_TRACKING
int g_nLibLocks = 0;
#define FFM_LOCK(A) \
		stiTrace ("<SOFTWAREInput::Lock> Requesting lock from %s.  Lock = %p [%s]\n", A, m_LockMutex, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		Lock ();\
		stiTrace ("<SOFTWAREInput::Lock> Locks = %d, Lock = %p.  Exiting from %s\n", ++g_nLibLocks, m_LockMutex, A);
#define FFM_UNLOCK(A) \
		stiTrace ("<SOFTWAREInput::Unlock> Freeing lock from %s, Lock = %p [%s]\n", A, m_LockMutex, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		Unlock (); \
		stiTrace ("<SOFTWAREInput::Unlock> Locks = %d, Lock = %p.  Exiting from %s\n", --g_nLibLocks, m_LockMutex, A);
#else
#define FFM_LOCK(A) Lock ();
#define FFM_UNLOCK(A) Unlock ();
#endif


stiEVENT_MAP_BEGIN(CstiVideoInput)
	stiEVENT_DEFINE(estiVIDEO_INPUT_RECORD_START, CstiVideoInput::EventRecordStart)
	stiEVENT_DEFINE(estiVIDEO_INPUT_RECORD_STOP, CstiVideoInput::EventRecordStop)
stiEVENT_MAP_END(CstiVideoInput)
stiEVENT_DO_NOW(CstiVideoInput)


CstiVideoInput::CstiVideoInput()
:
CstiOsTaskMQ(
nullptr,
0,
(size_t)this,
stiVideoINPUT_MAX_MESSAGES_IN_QUEUE,
stiVideoINPUT_MAX_MSG_SIZE,
stiVideoINPUT_TASK_NAME,
stiVideoINPUT_TASK_PRIORITY,
stiVideoINPUT_STACK_SIZE),
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
#ifdef ACT_FRAME_RATE_METHOD
	for (int i = 0; i < 3; i++)
	{
		m_fActualFrameRateArray[i] = 30.0;
	}
#endif
	m_EncoderMutex = stiOSMutexCreate(); // NOTE check valid?
	m_FrameDataMutex = stiOSMutexCreate ();

	while (m_FrameDataEmpty.size () < stiVideoINPUT_NUM_FRAME_BUFFERS)
	{
		m_FrameDataEmpty.push (new FrameData ());
	}

	m_bInitialized = true;
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

	stiOSMutexDestroy (m_EncoderMutex);
}


/*! \brief Event Handler for the estiSOFTWARE_INPUT_RECORD_START message
*
*  \retval estiOK if success
*  \retval estiERROR otherwise
*/
EstiResult CstiVideoInput::EventRecordStart(void *poEvent)
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
			hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
	}
	else
	{
		hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
		stiTESTRESULT ();
	}

	m_bRecording = true;
	
	UpdateEncoder ();

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
EstiResult CstiVideoInput::EventRecordStop(void *poEvent)
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

void CstiVideoInput::SendVideoFrame(UINT8* pBytes, INT32 nDataLength, UINT32 nWidth, UINT32 nHeight, EstiVideoCodec eCodec)
{
	if (nWidth != m_un32Width || nHeight != m_un32Height || !m_bRecording)
	{
		return;
	}

	FrameData *pData = nullptr;
	{
		// scoped threadsafe
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
		int previousSize = 4;
		if (pBytes[2] == 0x01)
		{
			int previousSize = 3;
		}

		int StartNAL = 0;
		for (int i = previousSize; i < nDataLength - 4;)
		{
			if (pBytes[i] == 0x00)
			{
				if (pBytes[i + 1] == 0x00)
				{
					if (pBytes[i + 2] == 0x00)
					{
						if (pBytes[i + 3] == 0x01)
						{
							int32_t length = (i - StartNAL) - previousSize;
							uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
							stiTrace("Slice Type: %#x\n", SliceType);
							switch (SliceType)
							{
							case 0x5:
								pData->SetKeyFrame();
							case 0x7:
							case 0x8:
								pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
								pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
								break;
							}
							StartNAL = i;
							//+---------------+
							//|0|1|2|3|4|5|6|7|
							//+-+-+-+-+-+-+-+-+
							//|F|NRI|   Type  |
							//+---------------+
							previousSize = sizeof(uint32_t);
							i += 5; //We are skipping 8 bits here because of the nal unit header

						}
						else
						{
							i += 4;
						}
					}
					else if (pBytes[i + 2] == 0x01)
					{
						int32_t length = (i - StartNAL) - previousSize;
						uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
						stiTrace("Slice Type: %#x\n", SliceType);
						switch (SliceType)
						{
						case 0x5:
							pData->SetKeyFrame();
						case 0x7:
						case 0x8:
							pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
							pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
							break;
						}
						StartNAL = i;
						//+---------------+
						//|0|1|2|3|4|5|6|7|
						//+-+-+-+-+-+-+-+-+
						//|F|NRI|   Type  |
						//+---------------+
						previousSize = 3;
						i += 4; //We are skipping 8 bits here because of the nal unit header
					}
					else
					{
						i += 3;
					}
				}
				else
				{
					i += 2;
				}
			}
			else
			{
				i++;
			}
		}
		//Set the last nal unit
		{
			int32_t length = (nDataLength - StartNAL) - previousSize;
			uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
			stiTrace("Slice Type: %#x\n", SliceType);
			switch (SliceType)
			{
			case 0x5:
				pData->SetKeyFrame();
			case 0x7:
			case 0x8:
				pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
				pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
				break;
			}
		}

		//pData->AddFrameData(pBytes, nDataLength);
		{
			//Scoped FrameDataMutex
			CstiOSMutexLock threadSafe(m_FrameDataMutex);
			m_FrameData.push(pData);
		}

		//Notify CstiVideoRecord that a packet is ready.
		CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_PACKET_SIGNAL);
		MsgSend(&oEvent, sizeof(oEvent));
	}
}

void CstiVideoInput::SendVideoFrame(UINT8* pBytes, UINT32 nWidth, UINT32 nHeight, EstiColorSpace eColorSpace)
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
				
				//Notify CstiVideoRecord that a packet is ready.
				CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_PACKET_SIGNAL);
				MsgSend(&oEvent, sizeof(oEvent));


#ifdef ACT_FRAME_RATE_METHOD
				EncoderActualFrameRateCalcAndSet();
#endif
			}
		}
	}
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
	if ((bool)bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
		videoPrivacySetSignal.Emit (bEnable);
	}

	return stiRESULT_SUCCESS;
}

/*! \brief Starts the shutdown process of the display task
*
* This method is responsible for doing all clean up necessary to gracefully
* terminate the display task. Note that once this method exits,
* there will be no more message received from the message queue since the
* message queue will be deleted and the display task will go away.
*
*  \retval estiOK if success
*  \retval estiERROR otherwise
*/
stiHResult CstiVideoInput::ShutdownHandle(void* poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle();

	return (hResult);
}

/*! \brief Task function, executes until the task is shutdown.
*
* This task will loop continuously looking for messages in the message
* queue.  When it receives a message, it will call the appropriate message
* handling routine.
*
*  \retval Always returns 1
*/
int CstiVideoInput::Task()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiVideoINPUT_MAX_MSG_SIZE / sizeof(uint32_t)) + 1];

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		// msg to handle?
		hResult = MsgRecv((char *)un32Buffer, stiVideoINPUT_MAX_MSG_SIZE, stiWAIT_FOREVER);

		if (stiIS_ERROR(hResult))
		{
			Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
		}
		else
		{
			// handle the message

			CstiEvent *poEvent = (CstiEvent*)un32Buffer;
			switch (poEvent->EventGet())
			{
				case estiEVENT_SHUTDOWN:
				{
					m_bShutdownMsgReceived = true;
					// Shutdown
					CstiEvent oEvent(estiEVENT_SHUTDOWN);

					// Lookup and handle the event
					FFM_LOCK("Task - 2");
					hResult = EventDoNow(&oEvent);
					FFM_UNLOCK("Task - 2");

					if (stiIS_ERROR(hResult))
					{
						Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if
					break;
				}

				case estiVIDEO_INPUT_PACKET_SIGNAL:
				{
					packetAvailableSignal.Emit();
					break;
				}
				default:
				{
					// Lookup and handle the event
					FFM_LOCK("Task - 1");
					hResult = EventDoNow(poEvent);
					FFM_UNLOCK("Task - 1");
					if (stiIS_ERROR(hResult))
					{
						Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					}
					break;
				}
			}
		}
	}

	return (0);

} // end CstiVPService::Task


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
			m_fActualFrameRateArray[0] = 30.0f / static_cast<float>(dCurrentTime - m_dStartFrameTime);
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

stiHResult CstiVideoInput::VideoRecordCodecSet(EstiVideoCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiOSMutexLock threadSafe(m_EncoderMutex);
	switch (eVideoCodec)
	{
	case estiVIDEO_H263:
	case estiVIDEO_H264:
	case estiVIDEO_H265:
		m_eVideoCodec = eVideoCodec;
		//We might need to reinitialize the encoder here with the new codec.
		break;
	default:
		stiMAKE_ERROR(stiRESULT_INVALID_CODEC);
		m_eVideoCodec = estiVIDEO_NONE;
		break;
	}

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

void CstiVideoInput::UpdateEncoder()
{
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoFrameSizeSet(m_un32Width, m_un32Height);

		m_pVideoEncoder->AvVideoBitrateSet(m_unTargetBitrate);

		m_pVideoEncoder->AvVideoFrameRateSet((float)m_unFrameRate);

		m_pVideoEncoder->AvVideoIFrameSet(m_unIntraFramerate);

		m_pVideoEncoder->AvVideoProfileSet(m_eProfile, m_unLevel, m_unConstraints);

		m_pVideoEncoder->AvInitializeEncoder();
		
	}
}

stiHResult CstiVideoInput::VideoRecordSettingsSet(const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("VideoRecordSettingsSet\n");

	CstiOSMutexLock threadSafe(m_EncoderMutex);
	m_un32Width = pVideoRecordSettings->unColumns;
	m_un32Height = pVideoRecordSettings->unRows;

	//Inform NtouchPC of the new capture size
	//ntouchPC::CstiNativeVideoLink::CaptureSize(m_un32Width, m_un32Height);
	m_ePacketization = pVideoRecordSettings->ePacketization;
	m_unFrameRate = pVideoRecordSettings->unTargetFrameRate;
	m_unTargetBitrate = pVideoRecordSettings->unTargetBitRate;
	m_unIntraFramerate = pVideoRecordSettings->unIntraFrameRate;
	m_eProfile = pVideoRecordSettings->eProfile;
	m_unLevel = pVideoRecordSettings->unLevel;
	m_unConstraints = pVideoRecordSettings->unConstraints;

	if (m_pVideoEncoder != nullptr)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose ();
			delete m_pVideoEncoder;
			m_pVideoEncoder = nullptr;
			hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
		UpdateEncoder ();
	}



STI_BAIL:

	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_RECORD_START);

	CstiOSMutexLock threadSafe(m_EncoderMutex);

	stiTESTCOND(m_eVideoCodec != estiVIDEOCODEC_NONE, estiERROR);
	
	hResult = MsgSend(&oEvent, sizeof(oEvent));

	stiTrace("VideoRecordStart\n");

STI_BAIL:
	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStop()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("VideoRecordStop\n");

	CstiEvent oEvent(CstiVideoInput::estiVIDEO_INPUT_RECORD_STOP);

	hResult = MsgSend(&oEvent, sizeof(oEvent));

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
		pm->getPropertyInt ("EnableH265", &nEnableH265, 0, WillowPM::PropertyManager::Persistent);
		if (nEnableH265 == 1)
		{
			pCodecs->push_back (estiVIDEO_H265);
		}
#endif
		pCodecs->push_back (estiVIDEO_H264);
		pCodecs->push_back (estiVIDEO_H263);
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
stiHResult CstiVideoInput::CodecCapabilitiesGet(EstiVideoCodec eCodec,	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

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

		//Extended is not a profile in FFMPEG
		//stProfileAndConstraint.eProfile = estiH264_EXTENDED;
		//stProfileAndConstraint.un8Constraints = 0xf0;
		//pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		stProfileAndConstraint.eProfile = estiH264_MAIN;
		stProfileAndConstraint.un8Constraints = 0xe0;
		pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		stProfileAndConstraint.eProfile = estiH264_BASELINE;
		stProfileAndConstraint.un8Constraints = 0xa0;
		pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		pstH264Caps->eLevel = estiH264_LEVEL_4_1;

		break;
	}

	case estiVIDEO_H265:
	{
		if (m_bH265Enabled)
		{
			SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH265Caps->Profiles.push_back(stProfileAndConstraint);
			pstH265Caps->eLevel = estiH265_LEVEL_4;
			pstH265Caps->eTier = estiH265_TIER_MAIN;
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
