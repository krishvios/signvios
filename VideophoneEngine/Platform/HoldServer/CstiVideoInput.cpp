
#include "CstiVideoInput.h"
#include "CstiExtendedEvent.h"

#define stiMAX_MESSAGES_IN_QUEUE 12
#define stiMAX_MSG_SIZE 512
#define stiTASK_NAME "CstiVideoInput"
#define stiTASK_PRIORITY 151
#define stiSTACK_SIZE 4096
#define stiDEFAULT_FRAME_DURATION 33

#ifdef COLOR_CORRECTION_MATRIX_TEST
static uint32_t g_un32CCMDefault = 4;
//static uint32_t g_un32CCMMin = 0;
//static uint32_t g_un32CCMMax = 4;
#endif

std::list<CstiVideoInput *> CstiVideoInput::m_VideoInputList;
std::recursive_mutex CstiVideoInput::m_videoInputListLock{}; // Used to secure access to VideoInputList

//
// Forward Declarations
//

stiEVENT_MAP_BEGIN (CstiVideoInput)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiVideoInput::ShutdownHandle)
stiEVENT_MAP_END (CstiVideoInput)
stiEVENT_DO_NOW (CstiVideoInput)

CstiVideoInput::CstiVideoInput ()
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
	m_bIsStarted (false)
{
	m_bPrivacy = estiFALSE;
	m_bInUse = estiFALSE;
	m_pMediaFile = new CstiMediaFile();
}

CstiVideoInput::~CstiVideoInput()
{
	if (m_pMediaFile)
	{
		delete m_pMediaFile;
	}
}

stiHResult CstiVideoInput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_bInUse = estiFALSE;
	m_eMediaServiceSupportState = EstiMediaServiceSupportStates::CALL_IS_SERVICEABLE;
	m_un32LastTimeStamp = 0;
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
stiHResult CstiVideoInput::ShutdownHandle (
	CstiEvent* poEvent)  // The event to be handled
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
}

stiHResult CstiVideoInput::HSEndService (EstiMediaServiceSupportStates eMediaServiceSupportState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(CstiVideoInput::m_videoInputListLock);
	m_bIsStarted = false;
	m_currentTimerVersion++; //Cancel any outstanding timers

	m_eMediaServiceSupportState = eMediaServiceSupportState;

	m_bIsStarted = true;
	hResult = m_pMediaFile->OpenByDescription (
		m_eMediaServiceSupportState, m_stVideoRecordSettings.unColumns,
		m_stVideoRecordSettings.unRows,
		m_stVideoRecordSettings.codec);

	m_nFrameTimeinMS = m_pMediaFile->FrameTimeMSGet ();

	//The timer itself will free this memory when its no longer active
	FrameTimerArgs *timerArgs = new FrameTimerArgs{
		this,
		m_currentTimerVersion,
	};
	//Start a new timer to process video for the new state
	m_timer.OneShot (CstiVideoInput::TimerTick, timerArgs, CstiMediaFile::VideoStartDelayGet());

	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(CstiVideoInput::m_videoInputListLock);
	VideoRecordStop ();
	m_currentTimerVersion++;

	if (m_serviceStartState)
	{
		m_eMediaServiceSupportState = *m_serviceStartState;
		m_serviceStartState = boost::none;
	}

	m_bIsStarted = true;
	hResult = m_pMediaFile->OpenByDescription (
		m_eMediaServiceSupportState,
		m_stVideoRecordSettings.unColumns,
		m_stVideoRecordSettings.unRows,
		m_stVideoRecordSettings.codec);
	m_nFrameTimeinMS = m_pMediaFile->FrameTimeMSGet ();
	
	//The timer itself will free this memory when its no longer active
	FrameTimerArgs* timerArgs = new FrameTimerArgs{
		this,
		m_currentTimerVersion,
	};
	//Start a new timer to process video for the new state
	m_timer.OneShot (CstiVideoInput::TimerTick, timerArgs, CstiMediaFile::VideoStartDelayGet());

	return hResult;
}


stiHResult CstiVideoInput::VideoRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Switch back to the default media support state
	m_eMediaServiceSupportState = EstiMediaServiceSupportStates::CALL_IS_SERVICEABLE;
	m_bIsStarted = false;
	return hResult;
}
	
stiHResult CstiVideoInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_stVideoRecordSettings	= *pVideoRecordSettings;
	if (m_bIsStarted)
	{
		m_pMediaFile->UpdateMediaSettings (
			m_eMediaServiceSupportState,
			m_stVideoRecordSettings.unColumns,
			m_stVideoRecordSettings.unRows,
			m_stVideoRecordSettings.codec);

		packetAvailableSignal.Emit();
	}
	return hResult;
}

stiHResult CstiVideoInput::VideoRecordFrameGet (
		SstiRecordVideoFrame * pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SStiFrameData pPacket;
	int nFrameTimeInMS = 1000;

	stiTESTCOND (pVideoFrame != NULL, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND (pVideoFrame->buffer != NULL, stiRESULT_BUFFER_TOO_SMALL);
	
	CstiMediaFile::EstiReadResult eReadResult = m_pMediaFile->PacketRead(&pPacket);
	stiTESTCOND (eReadResult == CstiMediaFile::EstiReadResult::estiREAD_OK, stiRESULT_DRIVER_ERROR);

	stiTESTCOND (pVideoFrame->unBufferMaxSize >= pPacket.n32Length, stiRESULT_BUFFER_TOO_SMALL);

	pVideoFrame->frameSize = pPacket.n32Length;
	pVideoFrame->keyFrame = pPacket.bIsKeyFrame;
	pVideoFrame->eFormat = pPacket.eEndianFormat;
	memcpy (pVideoFrame->buffer, pPacket.pData, pPacket.n32Length);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Since there was an error, reset packet information.
		pVideoFrame->frameSize = 0;
		pVideoFrame->keyFrame = estiFALSE;

		if (CstiMediaFile::EstiReadResult::estiREAD_MEDIA_COMPLETE == eReadResult)
		{
			// Notify those who registered for callback that we have completed the QuickTime video
			// (This functionality is not currently used.)
			holdserverVideoCompleteSignal.Emit ();
			// Change the hResult from an error to indicate no error.
			hResult = stiRESULT_SUCCESS;
		}
	}

	return hResult;
}

stiHResult CstiVideoInput::BrightnessRangeGet (
	uint32_t *pun32Min,
	uint32_t *pun32Max) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Min = 0;
	*pun32Max = 10;
	
	return hResult;
}

uint32_t CstiVideoInput::BrightnessDefaultGet () const
{
	return 5;
}

	
stiHResult CstiVideoInput::BrightnessSet (
		uint32_t un32Brightness)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiVideoInput::SaturationRangeGet (
	uint32_t *pun32Min,
	uint32_t *pun32Max) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*pun32Min = 0;
	*pun32Max = 10;
	
	return hResult;
}
		

uint32_t CstiVideoInput::SaturationDefaultGet () const
{
	return 5;
}


stiHResult CstiVideoInput::SaturationSet (
		uint32_t un32Saturation)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}

stiHResult CstiVideoInput::PrivacySet (
		EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
	
		//
		// Call the callbacks
		//
		videoPrivacySetSignal.Emit (bEnable);
	}
	
	return hResult;
}


stiHResult CstiVideoInput::PrivacyGet (
		EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*pbEnabled = m_bPrivacy;
	
//STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInput::KeyFrameRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


///\brief Reports to the calling object which packetization schemes this device is capable of.
///\brief Add schemes in the order that you would like preference to be given during channel
///\brief negotiations.
///
///\brief This method is reporting the schemes we are capable of using for Video Record.
stiHResult CstiVideoInput::PacketizationSchemesGet (
	std::list<EstiPacketizationScheme> *pPacketizationSchemes)
{
	// Add the supported packetizations to the list
//SCI-ALA-H264
#ifdef stiENABLE_H264
	pPacketizationSchemes->push_back (estiH264_NON_INTERLEAVED);
	pPacketizationSchemes->push_back (estiH264_SINGLE_NAL);
#endif

	return stiRESULT_SUCCESS;
}

IstiVideoInput * IstiVideoInput::instanceCreate(uint32_t callIndex)
{
	return CstiVideoInput::InstanceCreate();
}

/*!
	* \brief Create instance
	* 
	* \return IstiVideoInput* 
	*/
IstiVideoInput * CstiVideoInput::InstanceCreate ()
{
	IstiVideoInput *pVideoInput = NULL;
	
	// Lock the mutex to protect the list while we use it.
	std::lock_guard<std::recursive_mutex> lock (CstiVideoInput::m_videoInputListLock);

	//
	// Get the list of video input structures
	//
	std::list<CstiVideoInput*> pVideoInputList = CstiVideoInput::VideoInputListGet ();

	//
	// Create constant iterator for list.
	//
	std::list<CstiVideoInput*>::const_iterator i;

	//
	// Iterate through list
	//
	for (i = pVideoInputList.begin(); i != pVideoInputList.end(); i++)
	{
		//
		// Is this object available for use?
		//
		if (!(*i)->InUseGet ())
		{
			// It is.  Keep a pointer to it, set it as IN_USE and break out of the loop.
			pVideoInput = (*i);
			pVideoInput->InUseSet ();
			break;
		}
	}

	return pVideoInput;
}

/*!
 * \brief release instance
 */
void CstiVideoInput::instanceRelease () 
{
	// Lock the mutex to protect the list while we use it.
	std::lock_guard<std::recursive_mutex> lock(CstiVideoInput::m_videoInputListLock);
	m_bInUse = estiFALSE;
}

stiHResult CstiVideoInput::InitList (int nMaxCalls)
{
	stiHResult hResult = stiRESULT_SUCCESS;


	// Allocate a list of CstiVideoInput objects (used by CstiVideoRecord to interface with QuickTime files)
	std::lock_guard<std::recursive_mutex> lock (CstiVideoInput::m_videoInputListLock);
	for (int i = 0; i < nMaxCalls; i++)
	{
		CstiVideoInput *pVideoInput = new CstiVideoInput();

		m_VideoInputList.push_back (pVideoInput);

		hResult = pVideoInput->Initialize ();
		hResult = pVideoInput->Startup();
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}

void CstiVideoInput::UninitList ()
{
	// Loop through the list of videoInput objects removing and deleting each.
	while (m_VideoInputList.size () > 0)
	{
		CstiVideoInput* pVideoInput = m_VideoInputList.front ();
		m_VideoInputList.remove (pVideoInput);
		delete pVideoInput;
	}
}

/*!
	 * \brief Get in use
	 * 
	 * \return EstiBool 
	 */
	EstiBool CstiVideoInput::InUseGet ()
	{
		return m_bInUse;
	}

	/*!
	 * \brief Set in use
	 */
	void CstiVideoInput::InUseSet ()
	{
		m_bInUse = estiTRUE;
	}

/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 *  \retval Always returns 0
 */
int CstiVideoInput::Task ()
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
			{
				std::lock_guard<std::recursive_mutex> lock(CstiVideoInput::m_videoInputListLock);
				hResult = EventDoNow(poEvent);
			}
			
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

/*!
* \brief Get video codec
*
* \param pCodecs - a std::list where the codecs will be stored.
*
* \return stiHResult
*/
stiHResult CstiVideoInput::VideoCodecsGet(
	std::list<EstiVideoCodec> *pCodecs)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Add the supported video codecs to the list
#ifdef stiENABLE_H264
	pCodecs->push_back(estiVIDEO_H264);
#endif

	return (hResult);
}


/*!
* \brief Get Codec Capability
*
* \param eCodec
* \param pstCaps
*
* \return stiHResult
*/
stiHResult CstiVideoInput::CodecCapabilitiesGet(
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
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
		pstH264Caps->eLevel = estiH264_LEVEL_4_1;
		break;
	}

	default:
		hResult = stiRESULT_ERROR;
	}

	return (hResult);
}

void CstiVideoInput::ServiceStartStateSet(EstiMediaServiceSupportStates state)
{
	m_serviceStartState = state;
}
