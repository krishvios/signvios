// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#include "CstiFileVideoInput.h"
#include "CstiExtendedEvent.h"

#define stiMAX_MESSAGES_IN_QUEUE 12
#define stiMAX_MSG_SIZE 512
#define stiTASK_NAME "CstiFileVideoInput"
#define stiTASK_PRIORITY 151
#define stiSTACK_SIZE 4096

#define DEFAULT_RED_OFFSET_VALUE 0
#define RED_OFFSET_MIN_VALUE -200 //-1024
#define RED_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_GREEN_OFFSET_VALUE 0
#define GREEN_OFFSET_MIN_VALUE -200 //-1024
#define GREEN_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_BLUE_OFFSET_VALUE 0
#define BLUE_OFFSET_MIN_VALUE -200 //-1024
#define BLUE_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_ENCODE_SLICE_SIZE 1300
#define DEFAULT_ENCODE_BIT_RATE 10000000
#define DEFAULT_ENCODE_FRAME_RATE 30
//#define DEFAULT_ENCODE_FRAME_RATE_NUM 30000
//#define DEFAULT_ENCODE_FRAME_RATE_DEN 1001
#define DEFAULT_ENCODE_WIDTH 1920
#define DEFAULT_ENCODE_HEIGHT 1072

#define stiMAX_CAPTURE_WIDTH 1920
#define stiMAX_CAPTURE_HEIGHT 1072 //because its the value under 1080 thats a multiple of 16

//#define LOG_ENTRY_NAME() printf("%s:\n", __FUNCTION__)

#define LOG_ENTRY_NAME()


stiEVENT_MAP_BEGIN (CstiFileVideoInput)
	stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiFileVideoInput::ShutdownHandle)
	stiEVENT_DEFINE (estiEVENT_TIMER, CstiFileVideoInput::TimerHandle)
stiEVENT_MAP_END (CstiFileVideoInput)
stiEVENT_DO_NOW (CstiFileVideoInput)


CstiFileVideoInput::CstiFileVideoInput ()
	:
	CstiOsTaskMQ (
		NULL,
		0,
		(size_t)this,
		stiMAX_MESSAGES_IN_QUEUE,
		stiMAX_MSG_SIZE,
		stiTASK_NAME,
		stiTASK_PRIORITY,
		stiSTACK_SIZE),
	m_bPrivacy (estiFALSE),
	m_wdFrameTimer (NULL),
	m_eVideoCodec (estiVIDEO_NONE),
	m_pInputFile264 (NULL),
	m_pInputFile263 (NULL),
	m_bVideoRecordStarted (false)
{
}



stiHResult CstiFileVideoInput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_wdFrameTimer = stiOSWdCreate ();
	stiTESTCOND (m_wdFrameTimer != NULL, stiRESULT_ERROR);
	
	m_pInputFile264 = fopen ("/etc/Test.264", "rb");

	if (!m_pInputFile264)
	{
		stiTrace ("Was not able to open /etc/Test.264\n");
	}

	m_pInputFile263 = fopen ("/etc/Test.263", "rb");

	if (!m_pInputFile263)
	{
		stiTrace ("Was not able to open /etc/Test.263\n");
	}

	m_pInputFileJpg = fopen("/etc/Test.jpg", "rb");

	if (!m_pInputFileJpg)
	{
		stiTrace ("Was not able to open /etc/Test.jpg\n");
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiFileVideoInput::Uninitialize ()
{
	LOG_ENTRY_NAME();
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


CstiFileVideoInput::~CstiFileVideoInput ()
{
	Shutdown ();
	
	WaitForShutdown ();

	if (m_pInputFile264)
	{
		fclose (m_pInputFile264);
		m_pInputFile264 = NULL;
	}

	if (m_pInputFile263)
	{
		fclose (m_pInputFile263);
		m_pInputFile263 = NULL;
	}
	
	if (m_wdFrameTimer)
	{
		stiOSWdDelete (m_wdFrameTimer);
		m_wdFrameTimer = NULL;
	}
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
stiHResult CstiFileVideoInput::ShutdownHandle (
	CstiEvent* poEvent)  // The event to be handled
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Check to see if the key frame timer is running
	if (NULL != m_wdFrameTimer)
	{
		// Clear the key frame waiting condition
		stiOSWdCancel (m_wdFrameTimer);
	} // end if
	
	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
}

stiHResult CstiFileVideoInput::VideoRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//stiASSERT (!m_bVideoRecordStarted);

	//
	// Create a timer to mimic frames coming from an encoder.
	//
	CstiOsTask::TimerStart (m_wdFrameTimer, 33, 0);
	
	//
	// Seek back to the beginning of the file to ensure we start
	// with a key frame.
	//
	if (m_pInputFile263)
	{
		fseek (m_pInputFile263, 0, SEEK_SET);
	}

	if (m_pInputFile264)
	{
		fseek (m_pInputFile264, 0, SEEK_SET);
	}

	m_bVideoRecordStarted = true;

	return hResult;
}


stiHResult CstiFileVideoInput::VideoRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//stiASSERT (m_bVideoRecordStarted);

	stiOSWdCancel (m_wdFrameTimer);
	
	m_bVideoRecordStarted = false;

	return hResult;
}

stiHResult CstiFileVideoInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiLOG_ENTRY_NAME (CstiFileVideoInput::VideoRecordSettingsSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiFileVideoInput::VideoRecordSettingsSet\n");
	);

	return hResult;
}

stiHResult CstiFileVideoInput::VideoRecordFrameGet (
	SstiRecordVideoFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiASSERT (m_bVideoRecordStarted);

	switch (m_eVideoCodec)
	{
		case estiVIDEO_H263:
			
			if (m_pInputFile263)
			{
				uint32_t un32FrameSize = 0;
				size_t nNumElements;

				for (;;)
				{
					nNumElements = fread (&un32FrameSize, sizeof (uint32_t), 1, m_pInputFile263);

					if (nNumElements == 1)
					{
						break;
					}

					//
					// If we didn't read anything and we are at the beginning of the file
					// then throw an error.
					// Otherwise, seek to the beginning and try again.
					//
					stiTESTCOND (ftell (m_pInputFile263) != 0, stiRESULT_ERROR);

					fseek (m_pInputFile263, 0, SEEK_SET);
				}

		//		un32FrameSize = stiDWORD_BYTE_SWAP (un32FrameSize);

				//
				// Make sure the frame size is less than or equal to the max buffer size.
				//
				stiTESTCOND (un32FrameSize <= pVideoFrame->unBufferMaxSize, stiRESULT_ERROR);

				nNumElements = fread (pVideoFrame->buffer, un32FrameSize, 1, m_pInputFile263);
				stiTESTCOND (nNumElements == 1, stiRESULT_ERROR);

				pVideoFrame->frameSize = un32FrameSize;

				SsmdPacketInfo *pPacketInfo = (SsmdPacketInfo *)pVideoFrame->buffer;

				if (pPacketInfo->i == 0)
				{
					pVideoFrame->keyFrame = estiTRUE;
				}
				else
				{
					pVideoFrame->keyFrame = estiFALSE;
				}

//				pVideoFrame->eFormat = estiPACKET_INFO_PACKED;
				pVideoFrame->keyFrame = estiTRUE;
				pVideoFrame->eFormat = estiRTP_H263_MICROSOFT;
			}

			break;
			
		case estiVIDEO_H264:
			
			if (m_pInputFile264)
			{
				uint32_t un32FrameSize = 0;
				size_t nNumElements;

				for (;;)
				{
					nNumElements = fread (&un32FrameSize, sizeof (uint32_t), 1, m_pInputFile264);

					if (nNumElements == 1)
					{
						break;
					}

					//
					// If we didn't read anything and we are at the beginning of the file
					// then throw an error.
					// Otherwise, seek to the beginning and try again.
					//
					stiTESTCOND (ftell (m_pInputFile264) != 0, stiRESULT_ERROR);

					fseek (m_pInputFile264, 0, SEEK_SET);
				}

		//		un32FrameSize = stiDWORD_BYTE_SWAP (un32FrameSize);

				//
				// Make sure the frame size is less than or equal to the max buffer size.
				//
				stiTESTCOND (un32FrameSize <= pVideoFrame->unBufferMaxSize, stiRESULT_ERROR);

				nNumElements = fread (pVideoFrame->buffer, un32FrameSize, 1, m_pInputFile264);
				stiTESTCOND (nNumElements == 1, stiRESULT_ERROR);

				pVideoFrame->frameSize = un32FrameSize;

				if ((pVideoFrame->buffer[4] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_OF_IDR
				 || (pVideoFrame->buffer[4] & 0x1F) == estiH264PACKET_NAL_SPS
				 || (pVideoFrame->buffer[4] & 0x1F) == estiH264PACKET_NAL_PPS)
				{
					pVideoFrame->keyFrame = estiTRUE;
				}
				else
				{
					pVideoFrame->keyFrame = estiFALSE;
				}

				// Convert the stream into a BYTE_STREAM
				uint32_t un32NalUnitSize = 0;
				for (uint32_t un32Index = 0; un32Index < un32FrameSize; un32Index += 4 + un32NalUnitSize)
				{
					un32NalUnitSize = *((uint32_t *)(pVideoFrame->buffer + un32Index));
					pVideoFrame->buffer[un32Index] = 0;
					pVideoFrame->buffer[un32Index + 1] = 0;
					pVideoFrame->buffer[un32Index + 2] = 0;
					pVideoFrame->buffer[un32Index + 3] = 1;
				}

		//		pVideoFrame->eFormat = estiLITTLE_ENDIAN_PACKED;
				pVideoFrame->eFormat = estiBYTE_STREAM;

			}

			break;

		case estiVIDEO_NONE:
		case estiVIDEO_H265:
		case estiVIDEO_RTX:

			break;
	}
	
STI_BAIL:

	return hResult;
}

stiHResult CstiFileVideoInput::PrivacySet (
		EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;

		//
		// Call the callbacks
		//
		videoPrivacySetSignal.Emit(bEnable);
	}

	return hResult;
}


stiHResult CstiFileVideoInput::PrivacyGet (
		EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pbEnabled = m_bPrivacy;

//STI_BAIL:

	return hResult;
}


stiHResult CstiFileVideoInput::KeyFrameRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Simply go back to the beginning of the file to send the
	// key frame there.
	//
	if (m_pInputFile263)
	{
		fseek (m_pInputFile263, 0, SEEK_SET);
	}

	if (m_pInputFile264)
	{
		fseek (m_pInputFile264, 0, SEEK_SET);
	}

	return hResult;
}


/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 *  \retval Always returns 0
 */
int CstiFileVideoInput::Task ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiMAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		// read a message from the message queue
		hResult = MsgRecv (
			(char *)un32Buffer,
			stiMAX_MSG_SIZE,
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

			// Lookup and handle the event
			Lock ();
			hResult = EventDoNow (poEvent);
			Unlock ();
			
			if (stiIS_ERROR (hResult))
			{
				Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
			}

			if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
			{
				break;
			}
		} // end if
	} // end for

	return (0);

} // end CstiVPService::Task


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::TimerHandle
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
stiHResult CstiFileVideoInput::TimerHandle (
	CstiEvent *poEvent) // Event
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoPlayback::TimerHandle);

	Lock ();

	stiWDOG_ID Timer = (stiWDOG_ID)((CstiExtendedEvent *)poEvent)->ParamGet (0);

	if (Timer == m_wdFrameTimer)
	{
//		stiTrace ("------------------------------------------------------------- Frame Timer Fired.\n");
		
		CstiOsTask::TimerStart (m_wdFrameTimer, 33, 0);
		
		// Notify the app that a frame is ready.
		packetAvailableSignal.Emit();
	}

	Unlock ();

	return (hResult);
} // end CstiVideoPlayback::TimerHandle


/*!
 * \brief Get video codec
 *
 * \param pCodecs - a std::list where the codecs will be stored.
 *
 * \return stiHResult
 */
stiHResult CstiFileVideoInput::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	pCodecs->push_back (estiVIDEO_H264);

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Get Codec Capability
 *
 * \param eCodec
 * \param pstCaps
 *
 * \return stiHResult
 */
stiHResult CstiFileVideoInput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

	#if 1
			stProfileAndConstraint.eProfile = estiH264_EXTENDED;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
	#endif
			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4;

			break;
		}

		default:
			hResult = stiRESULT_ERROR;
	}

	return hResult;
}

