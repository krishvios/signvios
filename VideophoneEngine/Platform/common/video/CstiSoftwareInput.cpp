//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved

// Includes
//
#include "stiTrace.h"
#include "CstiSoftwareInput.h"

#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
#include "CstiH264VTEncoder.h"
#endif

//
// Constant
//
#define stiSOFTWAREINPUT_MAX_MESSAGES_IN_QUEUE 12
#define stiSOFTWAREINPUT_MAX_MSG_SIZE 512
#define stiSOFTWAREINPUT_TASK_NAME "CstiSoftwareInputTask"
#define stiSOFTWAREINPUT_TASK_PRIORITY 151
#define stiSOFTWAREINPUT_STACK_SIZE 4096

// This was set to 3. (one to be encoding into, one to be sending, and one for good measure)
// When an empty frame data is not available we send a key frame.  And when a 1920x1080 key 
// frame is encoded we are more likely to run out of these buffers.  This can cause a deluge
// of key frames.  Increasing to 12 buffers helps this issue.
#define stiSOFTWAREINPUT_NUM_FRAME_BUFFERS 12

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


stiEVENT_MAP_BEGIN (CstiSoftwareInput)
	stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiSoftwareInput::ShutdownHandle)
	stiEVENT_DEFINE (estiSOFTWARE_INPUT_RECORD_START, CstiSoftwareInput::EventRecordStart)
	stiEVENT_DEFINE (estiSOFTWARE_INPUT_RECORD_STOP, CstiSoftwareInput::EventRecordStop)
stiEVENT_MAP_END (CstiSoftwareInput)
stiEVENT_DO_NOW (CstiSoftwareInput)


CstiSoftwareInput::CstiSoftwareInput ()
	:
	CstiOsTaskMQ (
		NULL,
		0,
		(size_t)this,
		stiSOFTWAREINPUT_MAX_MESSAGES_IN_QUEUE,
		stiSOFTWAREINPUT_MAX_MSG_SIZE,
		stiSOFTWAREINPUT_TASK_NAME,
		stiSOFTWAREINPUT_TASK_PRIORITY,
		stiSOFTWAREINPUT_STACK_SIZE),
	m_bFirstFrame (estiTRUE),
	m_bInitialized (estiFALSE),
	m_bKeyFrame (estiFALSE),
	m_bPrivacy (estiFALSE),
	m_bRecording (estiFALSE),
	m_bShutdownMsgReceived (estiFALSE),
	m_eVideoCodec (estiVIDEO_NONE),
	m_pVideoEncoder (NULL),
	m_pInputBuffer(NULL),
	m_unInputBufferSize(0),
	m_discardedFrameCount(0),
	m_unFrameRate (0)
{
#ifdef ACT_FRAME_RATE_METHOD
	for (int i = 0; i < 3; i++)
	{
		m_fActualFrameRateArray[i] = 30.0;
	}
#endif
}

CstiSoftwareInput::~CstiSoftwareInput ()
{

	if (m_pVideoEncoder)
	{
		delete m_pVideoEncoder;
		m_pVideoEncoder = NULL;
	}

  while (m_FrameData.size()) {
		FrameData* pData = m_FrameData.front();
		m_FrameData.pop();
		delete pData;
	}
	while (m_FrameDataEmpty.size()) {
		FrameData* pData = m_FrameDataEmpty.front();
		m_FrameDataEmpty.pop();
		delete pData;
	}

}

uint32_t CstiSoftwareInput::BrightnessDefaultGet () const
{
	return 5;
}

stiHResult CstiSoftwareInput::BrightnessRangeGet (
	uint32_t *pun32Min,
	uint32_t *pun32Max) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Min = 0;
	*pun32Max = 10;

	return hResult;
}

stiHResult CstiSoftwareInput::BrightnessSet (
		uint32_t un32Brightness)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

stiHResult CstiSoftwareInput::CameraSet (
	EstiSwitch eSwitch)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


/*! \brief Event Handler for the estiSOFTWARE_INPUT_RECORD_START message
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
EstiResult CstiSoftwareInput::EventRecordStart (
	void *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock threadSafe(m_EncoderMutex);

	//We recreate the encoder each time because some endpoints need to change
	//which encoder they are using based on the VideoRecordSettings
	
	if (m_pVideoEncoder)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose();
			delete m_pVideoEncoder;
			m_pVideoEncoder = NULL;
			hResult = CreateVideoEncoder(&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT();
		}
	}
	else
	{
		hResult = CreateVideoEncoder(&m_pVideoEncoder, m_eVideoCodec);
		stiTESTRESULT();
	}
	
	//Make sure we created a VideoEncoder otherwise throw an error.
	stiTESTCOND(m_pVideoEncoder, stiERROR);
	hResult = SetupEncoder();

	stiTESTRESULT ();

	m_bRecording = estiTRUE;
	m_bFirstFrame = estiTRUE;

#ifdef ACT_FRAME_RATE_METHOD
	m_un32FrameCount = 0;
	m_dStartFrameTime = 0.0;
#endif

STI_BAIL:

	return stiIS_ERROR(hResult) ? estiERROR : estiOK;
}

/*! \brief Event Handler for the estiSOFTWARE_INPUT_RECORD_STOP message
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
EstiResult CstiSoftwareInput::EventRecordStop (
	void *poEvent)
{
	EstiResult eResult = estiOK;
	m_bRecording = estiFALSE;
	
	CstiOSMutexLock threadSafe ( m_EncoderMutex );
	
	while ( m_FrameData.size() )
	{
		FrameData *pData = m_FrameData.front();
		m_FrameData.pop();
		pData->Reset();
		m_FrameDataEmpty.push(pData);
	}

	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvEncoderClose();
	}
	
	return (eResult);
}

stiHResult CstiSoftwareInput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_EncoderMutex = stiOSMutexCreate(); // NOTE check valid?
	m_FrameDataMutex = stiOSMutexCreate();
	
	while (m_FrameDataEmpty.size() < stiSOFTWAREINPUT_NUM_FRAME_BUFFERS)
	{
		m_FrameDataEmpty.push(new FrameData());
	}

	m_bInitialized = estiTRUE;

	return hResult;
}

stiHResult CstiSoftwareInput::KeyFrameRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoRequestKeyFrame();
	}
	return hResult;
}

stiHResult CstiSoftwareInput::PrivacyGet (
		EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pbEnabled = m_bPrivacy;

//STI_BAIL:

	return hResult;
}

stiHResult CstiSoftwareInput::PrivacySet (
		EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
		videoPrivacySetSignalGet().Emit (bEnable);
	}

	return hResult;
}

uint32_t CstiSoftwareInput::SaturationDefaultGet () const
{
	return 5;
}

stiHResult CstiSoftwareInput::SaturationRangeGet (
	uint32_t *pun32Min,
	uint32_t *pun32Max) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Min = 0;
	*pun32Max = 10;

	return hResult;
}

stiHResult CstiSoftwareInput::SaturationSet (
		uint32_t un32Saturation)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
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
stiHResult CstiSoftwareInput::ShutdownHandle (
	IN void* poEvent)  // The event to be handled
{
	stiHResult hResult = stiRESULT_SUCCESS;

// We may need a unitiailize().
//	Uninitialize ();

	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
}

/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 *  \retval Always returns 0
 */
int CstiSoftwareInput::Task ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiSOFTWAREINPUT_MAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		// msg to handle?
		hResult = MsgRecv (
			(char *)un32Buffer,
			stiSOFTWAREINPUT_MAX_MSG_SIZE,
			m_bRecording ? stiNO_WAIT : stiWAIT_FOREVER);
		
		if (!m_bRecording && stiIS_ERROR(hResult)) {
			Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);				
		} else if (!stiIS_ERROR(hResult)) {
				// handle the message
				
				CstiEvent *poEvent = (CstiEvent*)un32Buffer;

				// Is the event a "shutdown" message?
				if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
				{
					m_bShutdownMsgReceived = estiTRUE;
				} else {
					// Lookup and handle the event
					FFM_LOCK ("Task - 1");
					hResult = EventDoNow (poEvent);
					FFM_UNLOCK ("Task - 1");
					if (stiIS_ERROR (hResult))
					{
						Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					}

				}

				if (m_bShutdownMsgReceived)
				{
					// Shutdown
					CstiEvent oEvent (estiEVENT_SHUTDOWN);

					// Lookup and handle the event
					FFM_LOCK ("Task - 2");
					hResult = EventDoNow (&oEvent);
					FFM_UNLOCK ("Task - 2");

					if (stiIS_ERROR (hResult))
					{
						Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if
                    
                    break;
				}
				
			} else {
				//stiASSERT(m_bRecording && hResult==stiRESULT_TIME_EXPIRED); // logic error if I got here not recording
				if (m_bRecording) {
					if (!ThreadDeliverFrame()) {
                        // we don't want to spin if there is no pictures to encode
                        usleep(40000); // 40 ms or 25 fps
                    }
                }
			}
		}

	return (0);

} // end CstiVPService::Task


/*! \brief Encoder function on Task Thread
 *
 */
EstiBool CstiSoftwareInput::ThreadDeliverFrame ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	static int skipCount=0;

	FrameData *pData = NULL;

	{
		// go ahead and lock encoder mutex until the message is ready to send
		// otherwise it's possible to obtain an image from the input queue
		// that doesn't match the encoder settings (width/height or other)
		CstiOSMutexLock threadSafe ( m_EncoderMutex );

		EstiColorSpace eColorSpace = estiCOLOR_SPACE_NV12;
		#if APPLICATION == APP_NTOUCH_PC
		if (!VideoInputRead(&m_pInputBuffer, &eColorSpace))
			{
				CstiOSMutexLock threadSafe ( m_LockMutex );
				stiTrace("No Input picture to Encode. %d available frames.\n", m_FrameData.size());
				return estiFALSE;
			}
		#elif APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
		if (!VideoInputRead(&m_pInputBuffer, &eColorSpace))
		{
				CstiOSMutexLock threadSafe ( m_LockMutex );
				stiTrace("No Input picture to Encode. %d available frames.\n", m_FrameData.size());
				return estiFALSE;
			
		}
		#else
			// check input buffer size
			uint32_t w,h;
			m_pVideoEncoder->AvVideoFrameSizeGet(&w,&h);
			uint32_t yuvSize=w*h*3/2;

			if (m_unInputBufferSize && m_unInputBufferSize<yuvSize)
			{
				DeleteInputBuffer(m_pInputBuffer);
				m_unInputBufferSize=0;
				m_pInputBuffer=NULL;
			}
			if (!m_pInputBuffer)
			{
				m_pInputBuffer = AllocateInputBuffer(yuvSize);
				m_unInputBufferSize = yuvSize;
			}

			if (!VideoInputRead(m_pInputBuffer))
			{
				std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );
				stiTrace("No Input picture to Encode. %d available frames.\n", m_FrameData.size());
				return estiFALSE;
			}
		#endif

		if (m_bFirstFrame)
		{
			m_bFirstFrame = estiFALSE;
			skipCount     = 12; 
		}

		if (skipCount-- > 0)
		{
			//stiTrace ( "Skip Frame... %d\n", skipCount );
			/* NOTE
			DJM: I added this slight delay because when our device calls the SIP hold server
			and is then transferred to the EX90 (Tandberg), the SIP ack packet on the 
			media start actually reaches the tandberg after the media packets get there.
			(The media packets go straight their instead of through the tandbergs VCS control
			proxy.  So in that case, the tandberg wasn't ready for the media and it ignores
			the packets at the start of the stream (which are needed for decoding of course).
			The result was black video on the tandberg for around 5 seconds.

			So yeah, this is a total hack for the time being but it appears to work thus
			far.  Better fix needed.
			*/
			
			#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
			if (eColorSpace == estiCOLOR_SPACE_CM_SAMPLE_BUFFER) {
				CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)m_pInputBuffer;
				CFRelease(sampleBuffer);
			}
			#endif
			return estiTRUE;
		}

		{	
			// scoped threadsafe
			CstiOSMutexLock threadSafe ( m_FrameDataMutex );

			if ( m_FrameDataEmpty.empty())
			{
				if (m_discardedFrameCount == 0)
				{
					stiTrace("WARN: RTP Send not keeping up with Encoder.\n");
				}
				m_discardedFrameCount++;
				pData = NULL;
				#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
				if (eColorSpace == estiCOLOR_SPACE_CM_SAMPLE_BUFFER) {
					CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)m_pInputBuffer;
					CFRelease(sampleBuffer);
				}
				#endif
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
			if (m_discardedFrameCount > 0)
			{
				stiTrace("WARN: RTP Send not keeping up with Encoder lost %d frames.\n", m_discardedFrameCount);
				m_discardedFrameCount = 0;
				// need to request new keyframe since one or more frames were skipped
				m_pVideoEncoder->AvVideoRequestKeyFrame();
			}

			// Compress the frame.
			hResult = m_pVideoEncoder->AvVideoCompressFrame(m_pInputBuffer, pData, eColorSpace);
			pData->SetFrameFormat(m_pVideoEncoder->AvFrameFormatGet());
			
			{
				CstiOSMutexLock threadSafe ( m_FrameDataMutex );
				if (stiIS_ERROR(hResult))
				{
					stiTrace("FAILED: m_pVideoEncoder->AvVideoCompressFrame(m_pInputBuffer,	pData) Err %d\n", hResult);
					m_FrameDataEmpty.push(pData);
					m_discardedFrameCount++;
					pData = NULL;
				}
				else
				{
					m_FrameData.push(pData);
				}
			}
		}
	}

	if (pData)
	{
		// Notify the app that a frame is ready.
		packetAvailableSignalGet().Emit ();
		
#ifdef ACT_FRAME_RATE_METHOD
		EncoderActualFrameRateCalcAndSet();
#endif
		
	}

	return estiTRUE;
}



#ifdef ACT_FRAME_RATE_METHOD
/*! \brief	Called each time a frame is ready, calculates the actual frame rate the camera is delivering and sets
 *			the actual frame rate member variable in CstiVideoEncoder. This is used as a ratio with the target
 *			frame rate to calculate bit rate in AvUpdateEncoderSettings() which is called from AvInitializeEncoder().
 */
void CstiSoftwareInput::EncoderActualFrameRateCalcAndSet()
{
	CstiOSMutexLock threadSafe(m_EncoderMutex);

	timeval ts;
	
	m_un32FrameCount++;
	if (m_un32FrameCount%30 == 0)
	{
		gettimeofday(&ts,NULL);
		double dCurrentTime = (ts.tv_sec & 0xFFFF) /* 16 bits only */ + (double)ts.tv_usec / 1e6;
		if (m_dStartFrameTime != 0.0)
		{
			float fFrameRateAverage = 0.0;
			for (int i=2;i>0;i--)
			{
				m_fActualFrameRateArray[i] = m_fActualFrameRateArray[i-1];
				fFrameRateAverage += m_fActualFrameRateArray[i];
			}
			m_fActualFrameRateArray[0] = 30.0f/ static_cast<float>(dCurrentTime - m_dStartFrameTime);
			fFrameRateAverage += m_fActualFrameRateArray[0];
			fFrameRateAverage /= 3.0;
			
			float fRateDiff =  (fFrameRateAverage > m_fCurrentEncoderRate) ?
								fFrameRateAverage - m_fCurrentEncoderRate : m_fCurrentEncoderRate - fFrameRateAverage;
			if (fRateDiff >= 1.5)
			{
				m_fCurrentEncoderRate = fFrameRateAverage;
				m_pVideoEncoder->AvVideoActualFrameRateSet (m_fCurrentEncoderRate);
				m_pVideoEncoder->AvInitializeEncoder();
			}
		}
		
		m_dStartFrameTime = dCurrentTime;
	}
}
#endif //ACT_FRAME_RATE_METHOD


stiHResult CstiSoftwareInput::Uninitialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvEncoderClose();
		delete m_pVideoEncoder;
		m_pVideoEncoder = NULL;
	}
	
	stiOSMutexDestroy(m_EncoderMutex);

	return hResult;
}

stiHResult CstiSoftwareInput::VideoRecordCodecSet (
		EstiVideoCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock threadSafe ( m_EncoderMutex );

	if (eVideoCodec != estiVIDEO_H264 &&
			eVideoCodec != estiVIDEO_H263 && eVideoCodec != estiVIDEO_H265)
	{
		hResult = stiRESULT_INVALID_CODEC;		
		return hResult;
	}

	m_eVideoCodec = eVideoCodec;
	
	return hResult;
}

stiHResult CstiSoftwareInput::VideoRecordFrameGet (
		SstiRecordVideoFrame *pVideoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock threadSafe ( m_FrameDataMutex );
	
  FrameData *pData = NULL;
	stiTESTCOND(m_FrameData.size() > 0, stiRESULT_DRIVER_ERROR);
	pData = m_FrameData.front();
	stiTESTCOND (pVideoPacket != NULL, stiRESULT_BUFFER_TOO_SMALL);
    stiTESTCOND (pVideoPacket->buffer != NULL, stiRESULT_BUFFER_TOO_SMALL);
    stiTESTCOND (pVideoPacket->unBufferMaxSize >= pData->GetDataLen(), stiRESULT_BUFFER_TOO_SMALL);

	pData->GetFrameData(pVideoPacket->buffer);
	pVideoPacket->frameSize = pData->GetDataLen();
	pVideoPacket->keyFrame = pData->IsKeyFrame() ? estiTRUE : estiFALSE;
	pVideoPacket->eFormat = pData->GetFrameFormat();

STI_BAIL:
	if(pData)
	{
  		m_FrameData.pop();
		m_FrameDataEmpty.push(pData);
	}
	return hResult;
}

bool CstiSoftwareInput::FrameAvailableGet()
{
    return m_FrameData.size() > 0;
}

uint8_t* CstiSoftwareInput::AllocateInputBuffer(const uint32_t yuvSize)
{
    return new uint8_t[yuvSize];
}

void CstiSoftwareInput::DeleteInputBuffer(uint8_t* buffer)
{
    if (m_pInputBuffer)
    {
    	delete [] m_pInputBuffer;
        m_pInputBuffer = NULL;
        m_unInputBufferSize=0;
    }
}

bool CstiSoftwareInput::AllowsFragmentationUnits()
{
	return estiH264_NON_INTERLEAVED == m_stCurrentSettings.ePacketization || estiH264_INTERLEAVED == m_stCurrentSettings.ePacketization || estiH265_NON_INTERLEAVED == m_stCurrentSettings.ePacketization || estiPktNone == m_stCurrentSettings.ePacketization;
}

stiHResult CstiSoftwareInput::CreateVideoEncoder(IstiVideoEncoder **pEncoder, EstiVideoCodec eCodec)
{
    return IstiVideoEncoder::CreateVideoEncoder(pEncoder, eCodec);
}

stiHResult CstiSoftwareInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace ( "VideoRecordSettingsSet: %d x %d at %d fps\n", m_stCurrentSettings.unColumns, m_stCurrentSettings.unRows, m_stCurrentSettings.unTargetFrameRate );
	//Copy the values of the VideoRecordSettings into our local copy.
	memcpy(&m_stCurrentSettings, pVideoRecordSettings, sizeof(SstiVideoRecordSettings));

	m_unFrameRate = m_stCurrentSettings.unTargetFrameRate;
	
	if (m_pVideoEncoder != NULL)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose ();
			delete m_pVideoEncoder;
			m_pVideoEncoder = NULL;
			hResult = CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
		SetupEncoder();
	}

	VideoRecordSizeSet(m_stCurrentSettings.unColumns, m_stCurrentSettings.unRows);
	VideoRecordFrameRateSet ( m_stCurrentSettings.unTargetFrameRate);

STI_BAIL:

	return hResult;
}

stiHResult CstiSoftwareInput::CaptureCapabilitiesChanged ()
{
	captureCapabilitiesChangedSignalGet().Emit ();
	return stiRESULT_SUCCESS;
}

stiHResult CstiSoftwareInput::SetupEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("SetupEncoder\n");

	CstiOSMutexLock threadSafe(m_EncoderMutex);

	stiTESTCOND(m_pVideoEncoder, stiRESULT_INVALID_CODEC);
	
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
	if (m_eVideoCodec == estiVIDEO_H264) {
		((CstiH264VTEncoder *)m_pVideoEncoder)->AvEncoderAllowsFragmentationUnitsSet(AllowsFragmentationUnits());
	}
#endif

	m_pVideoEncoder->AvVideoFrameSizeSet(m_stCurrentSettings.unColumns, m_stCurrentSettings.unRows);

	m_pVideoEncoder->AvVideoBitrateSet(m_stCurrentSettings.unTargetBitRate);

	m_pVideoEncoder->AvVideoFrameRateSet((float)(m_stCurrentSettings.unTargetFrameRate));

	m_pVideoEncoder->AvVideoIFrameSet(m_stCurrentSettings.unIntraFrameRate);

	hResult = m_pVideoEncoder->AvVideoProfileSet(m_stCurrentSettings.eProfile, m_stCurrentSettings.unLevel, m_stCurrentSettings.unConstraints);
	stiTESTRESULT();

	hResult = m_pVideoEncoder->AvInitializeEncoder();
	stiTESTRESULT();

STI_BAIL:

	return hResult;
}

stiHResult CstiSoftwareInput::VideoRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent(CstiSoftwareInput::estiSOFTWARE_INPUT_RECORD_START);

	hResult = MsgSend (&oEvent, sizeof (oEvent));
	CaptureEnabledSet(estiTRUE);
	stiTrace ( "VideoRecordStart\n" );

	return hResult;
}

stiHResult CstiSoftwareInput::VideoRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace ( "VideoRecordStop\n" );

	CstiEvent oEvent (CstiSoftwareInput::estiSOFTWARE_INPUT_RECORD_STOP);

	hResult = MsgSend (&oEvent, sizeof (oEvent));
	CaptureEnabledSet(estiFALSE);

	return hResult;
}



