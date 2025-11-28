// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioInput.h"
#include "CstiExtendedEvent.h"
#include <err.h>

#define stiMAX_MESSAGES_IN_QUEUE 12
#define stiMAX_MSG_SIZE 512
#define stiTASK_NAME "CstiAudioInput"
#define stiTASK_PRIORITY 151
#define stiSTACK_SIZE 4096

#define AUDIO_TEST_FILE "/etc/Audio.raw"

stiEVENT_MAP_BEGIN (CstiAudioInput)
	stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiAudioInput::ShutdownHandle)
	stiEVENT_DEFINE (estiEVENT_TIMER, CstiAudioInput::TimerHandle)
stiEVENT_MAP_END (CstiAudioInput)
stiEVENT_DO_NOW (CstiAudioInput)


/*!
 * \brief Default Constructor
 */
CstiAudioInput::CstiAudioInput ()
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
	m_pInputFile (NULL),
	m_PacketReadySignal (NULL)
{
	m_AudioRecordSettings.un16SamplesPerPacket = 240;
}


stiHResult CstiAudioInput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	
	m_wdFrameTimer = stiOSWdCreate ();
	stiTESTCOND (m_wdFrameTimer != NULL, stiRESULT_ERROR);
	
	eResult = stiOSSignalCreate (&m_PacketReadySignal);
	stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);

	m_pInputFile = fopen (AUDIO_TEST_FILE, "rb");
	
	if (!m_pInputFile)
	{
		stiTrace ("Was not able to open %s\n", AUDIO_TEST_FILE);
	}
	
STI_BAIL:

	return hResult;
}


CstiAudioInput::~CstiAudioInput ()
{
	if (m_pInputFile)
	{
		fclose (m_pInputFile);
		m_pInputFile = NULL;
	}
	
	if (m_wdFrameTimer)
	{
		stiOSWdDelete (m_wdFrameTimer);
		m_wdFrameTimer = NULL;
	}

	if (m_PacketReadySignal)
	{
		stiOSSignalClose (&m_PacketReadySignal);
		m_PacketReadySignal = NULL;
	}
}


/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 *  \retval Always returns 0
 */
int CstiAudioInput::Task ()
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
stiHResult CstiAudioInput::ShutdownHandle (
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
stiHResult CstiAudioInput::TimerHandle (
	CstiEvent *poEvent) // Event
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoPlayback::TimerHandle);

	Lock ();

	stiWDOG_ID Timer = (stiWDOG_ID)((CstiExtendedEvent *)poEvent)->ParamGet (0);

	if (Timer == m_wdFrameTimer)
	{
		CstiOsTask::TimerStart (m_wdFrameTimer, m_AudioRecordSettings.un16SamplesPerPacket / 8, 0);
		
		// Notify the app that a frame is ready.
		stiOSSignalSet (m_PacketReadySignal);
	}

	Unlock ();

	return (hResult);
} // end CstiVideoPlayback::TimerHandle


/*!
 * \brief Starts Audio Record
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Create a timer to mimic frames coming from an encoder.
	//
	CstiOsTask::TimerStart (m_wdFrameTimer, m_AudioRecordSettings.un16SamplesPerPacket / 8, 0);
	
	return hResult;
}

/*!
 * \brief Stops Audio Record
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiOSWdCancel (m_wdFrameTimer);
	
	return hResult;
}


/*!
 * \brief Gets audio record packet
 * 
 * \param SsmdAudioFrame* pAudioFrame 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordPacketGet (
	SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pInputFile)
	{
		size_t nNumElements;
		
		for (;;)
		{
			nNumElements = fread (pAudioFrame->pun8Buffer, sizeof (uint8_t), m_AudioRecordSettings.un16SamplesPerPacket, m_pInputFile);
			
			if (nNumElements == m_AudioRecordSettings.un16SamplesPerPacket)
			{
				break;
			}
			
			//
			// If we didn't read anything and we are at the beginning of the file
			// then throw an error.
			// Otherwise, seek to the beginning and try again.
			//
			stiTESTCOND (ftell (m_pInputFile) != 0, stiRESULT_ERROR);
			
			fseek (m_pInputFile, 0, SEEK_SET);
		}
		
	}
	else
	{
		memset (pAudioFrame->pun8Buffer, 0xFF, m_AudioRecordSettings.un16SamplesPerPacket);
	}

	pAudioFrame->unFrameSizeInBytes = m_AudioRecordSettings.un16SamplesPerPacket;
	
	stiOSSignalClear (m_PacketReadySignal);

STI_BAIL:

	return hResult;
}


/*!
 * \brief Sets audio record codec 
 * This sets which of the audio record 
 * codecs to be used for recording an audio 
 * stream 
 * 
 * \param EstiAudioCodec eAudioCodec 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordCodecSet (
	EstiAudioCodec eAudioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

	
/*!
 * \brief Audio Record Setting Set 
 * uses the SstiAudioRecordSettings struct to setup 
 * the audio stream 
 * 
 * \param const SstiAudioRecordSettings* pAudioRecordSettings 
 *    contains un16SamplesPerPacket which holds
 *    samples per audio frame/packet
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordSettingsSet (
	const SstiAudioRecordSettings *pAudioRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	m_AudioRecordSettings = *pAudioRecordSettings;

	Unlock ();
	
	return hResult;
}


/*!
 * \brief Sets Echo mode
 * 
 * \param EsmdEchoMode eEchoMode 
 * enum setting echo mode to on or off 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::EchoModeSet (
	EsmdEchoMode eEchoMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

	
/*!
 * \brief Sets Privady to on or off
 * 
 * \param EstiBool bEnable 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::PrivacySet (
	EstiBool enable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (enable != m_bPrivacy)
	{
		m_bPrivacy = enable;
	
		audioPrivacySignalGet ().Emit(enable);
	}

	return hResult;
}

	
/*!
 * \brief Gets the privacy status
 * 
 * \param EstiBool* pbEnabled 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::PrivacyGet (
	EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*pbEnabled = m_bPrivacy;
	
	return hResult;
}


/*!
 * \brief Get Device File Descriptor
 * 
 * \return int 
 */
int CstiAudioInput::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_PacketReadySignal);
}
