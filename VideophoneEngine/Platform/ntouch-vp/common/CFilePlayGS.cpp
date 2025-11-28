/*!
*  \file CFilePlayGS.cpp
*  \brief The Message Viewer Interface
*
*  Class declaration for the Message Viewer Interface Class.  
*   This class is responsible for communicating with GStreamer
*   to play and record files.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#include "CFilePlayGS.h"
#include <sys/time.h>

#include "stiTaskInfo.h"
#include "IVideoOutputVP.h"
#include "IstiNetwork.h"
#include "stiTools.h"
#include "stiDebugTools.h"
#include "stiRemoteLogEvent.h"
#include "stiHTTPServiceTools.h"
#include "CstiHTTPService.h"
#include "CstiIpAddress.h"
#include "IstiFileUploadHandler.h"
#include <sstream>


//
// Constants
//
static const int nMAX_VIDEO_RECORD_PACKET = 691200; // Largest frame 720 x 480 x 2

static const int nMAX_GUID_REQUEST_RETRIES = 3;
static const int DEFAULT_FRAME_RATE = 30;
static const int DEFAULT_RECORD_ROWS = unstiSIF_ROWS;
static const int DEFAULT_RECORD_COLS = unstiSIF_COLS;
static const int DEFAULT_RECORD_VGA_ROWS = unsti4SIF_ROWS;
static const int DEFAULT_RECORD_VGA_COLS = unsti4SIF_COLS;
static const int DEFAULT_LOW_INTRA_FRAME_RATE = 0;
static const int DEFAULT_INTRA_FRAME_RATE = 45;

static const int DELAY_RECORD_START_TIME = 15;

static const int DEFAULT_RECORD_BITRATE = 768000;

#define COUNTDOWN_VIDEO_CORE_PATH	"video/countdown.h264"
#define MAX_FILE_CLOSE_WAIT 5

static const int g_nREAD_TIMEOUT = 10; ///< Readtime out
static const int g_nHEADER_BUFFER_SIZE = 4096; ///< HTTP header buffer size

static const int g_nVIDEO_RECORD_TIME_SCALE = 600; ///< Timescale used for recording video frames.

//
// Typedefs
//
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//

#ifdef stiDEBUGGING_TOOLS
static const char *StateNameGet (
	IstiMessageViewer::EState eState);
#endif

//
// Constructor
//
/*!
 * \brief  Constructor
 * 
 * \param PstiObjectCallback pAppCallback 
 * \param size_t CallbackParam 
 */
CFilePlayGS::CFilePlayGS ()
	:
	CstiEventQueue("CFilePlayGS"),
	m_un32CurrentRecordBitrate (DEFAULT_RECORD_BITRATE),
	m_un32MaxRecordBitrate (DEFAULT_RECORD_BITRATE),
	m_un32RecordWidth (DEFAULT_RECORD_COLS),
	m_un32RecordHeight (DEFAULT_RECORD_ROWS),
	m_un32IntraFrameRate (DEFAULT_INTRA_FRAME_RATE),
	m_restartRecordErrorTimer (DELAY_RECORD_START_TIME, this)
{
	stiLOG_ENTRY_NAME (CFilePlayGS::CFilePlayGS)

	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
}

//
// Destructor
//
/*!
 * \brief Destructor
 */
CFilePlayGS::~CFilePlayGS ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::~CFilePlayGS)

	Shutdown ();
}

stiHResult CFilePlayGS::Startup ()
{
	// Start the event loop.
	return (CstiEventQueue::StartEventLoop() ?
	           stiRESULT_SUCCESS :
			   stiRESULT_ERROR);
}


/*!
 * \brief Initializes the object  
 * 
 * \return stiHResult 
 * stiRESULT_SUCCESS, or on error 
 *  
 */
stiHResult CFilePlayGS::Initialize ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	CstiFileUploadHandlerInitialize(nullptr, 0);
	IstiFileUploadHandler::InstanceGet()->startup();

	//
	// Get instance of the video output device
	//
	m_pVideoOutput = IVideoOutputVP::InstanceGet ();
	stiTESTCOND(m_pVideoOutput != nullptr, stiRESULT_NO_VIDEO_DRIVER);

	//
	// Get instance of the video input device
	//
	m_pVideoInput = dynamic_cast<IVideoInputVP2 *>(IstiVideoInput::InstanceGet ());
	stiTESTCOND(m_pVideoInput != nullptr, stiRESULT_NO_VIDEO_DRIVER);

	m_signalConnections.push_back (m_restartRecordErrorTimer.timeoutSignal.Connect (
		[this]
		{
			// Reset the record settings and then restart recording.
			VideoRecordSettingsSet();

			m_eState = IstiMessageViewer::estiWAITING_TO_RECORD;
			EventRecordStart();
		}));

STI_BAIL:

	return hResult;
} // end CFilePlayGS::Initialize ()


stiHResult CFilePlayGS::Shutdown ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Shutdown);
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::Shutdown\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	//
	// Stop video driver
	//
	VideoPlaybackStop ();

	//
	// Stop video input if we were recording.
	//
	if (m_bReceivingVideo)
	{
		VideoInputSignalsDisconnect ();
		m_pVideoInput->VideoRecordStop();
		
		m_bReceivingVideo = false;
	}
	
	// Shutdown the file upload handler.
	CstiFileUploadHandlerUninitialize();

	// Release the any signal connections we are hooked to.
	m_signalConnections.clear();

	return (hResult);
}

/*!
 * \brief Sets the Maximum speed
 * 
 * \param uint32_t un32MaxDownloadSpeed 
 * \param uint32_t un32MaxUploadSpeed 
 */
void CFilePlayGS::MaxSpeedSet (
	uint32_t un32MaxDownloadSpeed,
	uint32_t un32MaxUploadSpeed)
{
	m_maxDownloadSpeed = un32MaxDownloadSpeed;
	m_maxUploadSpeed = un32MaxUploadSpeed;
}


/*!
 * \brief Event: Open the movie to be played
 * 
 */
void CFilePlayGS::EventOpenMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventOpenMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		vpe::trace("CFilePlayGS::EventOpenMovie\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure we are in a state that we can open a movie in.
	stiTESTCOND(StateValidate(IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiPLAYER_IDLE | 
							  IstiMessageViewer::estiRECORD_CLOSED), 
				stiRESULT_ERROR);

	StateSet(IstiMessageViewer::estiREQUESTING_GUID);

	requestGUIDSignal.Emit();

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		// If we had an error then notify the UI that there was an error opening the requested file.
		ErrorStateSet(IstiMessageViewer::estiERROR_OPENING);
		StateSet(IstiMessageViewer::estiERROR);
	}
}

/*!
 * Opens the message identified by the ItemID.
 * 
 * \param pItemId The id of the file
 * \param pszName The name of the person who lef the message (security double-check)
 * \param pszPhoneNumber The phone number of the person who left the message
 */
void CFilePlayGS::Open (
		const CstiItemId &itemId,
		const char *pszName,
		const char *pszPhoneNumber)
{
	PostEvent([this, itemId, pszName, pszPhoneNumber]
	{
		stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

		stiTESTCOND (itemId.IsValid (), stiRESULT_ERROR);

		m_pItemId = itemId;

		if (pszName && *pszName)
		{
			m_name = pszName;
		}

		if (pszPhoneNumber && *pszPhoneNumber)
		{
			m_phoneNumber = pszPhoneNumber;
		}

		m_closedDuringGUIDRequest = false;

		PostEvent([this]{EventOpenMovie();});

STI_BAIL:;
	});

	return;
}

/*!
 * Opens the message identified by the ItemID.
 * 
 * \param pszServer The server of the video to open
 * \param pszFileName The file name of the video to open
 */
void CFilePlayGS::Open (
	const std::string &server,
	const std::string &fileName,
	bool useDownloadSpeed)
{
	PostEvent([this, server, fileName, useDownloadSpeed]
	{
		stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

		stiTESTCOND (!fileName.empty (), stiRESULT_ERROR);

		m_closedDuringGUIDRequest = false;

		//The ItemId causes a MessageListItemEdit to be sent when the playing state is reached.
		//since we are no longer playing a MessageListItem with an ItemId we should clear it.
		m_pItemId = CstiItemId ();

		m_server.clear ();
		if (!server.empty ())
		{
			m_server = server;
		}

		eventLoadVideoMessage (m_server, 
							   fileName, 
							   "", 0, "", 
							   estiFALSE, 
							   useDownloadSpeed);

STI_BAIL:;
	});

	return;
}

/*!
 * \brief Event LoadCountdownVideo
 * 
 */
void CFilePlayGS::EventLoadCountdownVideo ()
{
	std::string videoFile;
	std::string emptySvr;
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CFilePlayGS::EventLoadCountdownVideo);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventLoadCountdownVideo\n");
	);

	stiOSStaticDataFolderGet(&videoFile);
	videoFile += COUNTDOWN_VIDEO_CORE_PATH;

	if (!m_downloadGuid.empty() &&
		m_downloadGuid != videoFile)
	{
		// We are not playing the countdown so try and close it.
		if (!StateValidate(IstiMessageViewer::estiCLOSING) && 
			!StateValidate(IstiMessageViewer::estiRECORD_CLOSING))
		{
			hResult = Close();
			stiTESTRESULT();
		}

		// If the file can be closed then play the countdown.
		m_loadCountdownVideo = true;
	}
	// See if greeting is already playing, if not set it up to start.
	else if (!StateValidate(estiPLAYING | estiPLAY_WHEN_READY))
	{
		if (StateValidate (estiPAUSED))
		{
			m_loadCountdownVideo = true;
			PostEvent ([this]{EventCloseMovie ();});
		}
		else if (!StateValidate(estiRECORD_CLOSED | estiCLOSED | estiPLAYER_IDLE))
		{
			m_loadCountdownVideo = true;
		}
		else
		{
			// If we are recording a SignMail then make sure we have an upload GUID.
			if ((RecordTypeGet() == estiRECORDING_UPLOAD_URL || 
				 RecordTypeGet () == estiRECORDING_REVIEW_UPLOAD_URL) &&
				(m_uploadGUID.empty() || m_uploadServer.empty()))
			{
				requestUploadGUIDSignal.Emit();
			}

			SetStartPosition(0);
			Open(emptySvr, videoFile);

			// The countdown has been loaded so clear the LoadCountdown flag.
			m_loadCountdownVideo = false;
		}
	}

STI_BAIL:

	return;
}


/*!
 * \brief Loads and begin to play the countdown video.
 * 		  
 * \param bWaitToPlay tells FilePlayer to load the countdown video now or 
 * 		  wait for a video to play first. 
 * 
 * \return N/A 
 *  
 */
stiHResult CFilePlayGS::LoadCountdownVideo(bool waitToPlay)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string staticFolder;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::LoadCountdownVideo\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check to see if we are already playing the countdown.  If we are don't 
	// do anything.
	stiOSStaticDataFolderGet(&staticFolder);

	std::size_t found = m_downloadGuid.find(staticFolder);
	if (m_downloadGuid.empty() ||
		found == std::string::npos)
	{
		if (waitToPlay)
		{
			m_loadCountdownVideo = true;
		}
		else
		{
			PostEvent([this]{EventLoadCountdownVideo();});
		}
	}

	return hResult;
}


/*!
 * \brief Load the video message
 * 
 * \param const char* pszServer 
 * \param const char* pszFileGUID 
 * \param const char* pszViewId 
 * \param uint64_t un64FileSize 
 * \param const char* pszClientToken 
 * \param EstiBool bPause 
 * Set the initial state to Pause (used when an incoming call is detected while  
 *  initiating fileplay.
 *  
 * \return Success or failure result
 */
stiHResult CFilePlayGS::LoadVideoMessage (
	const std::string &server,
	const std::string &fileGUID,
	const std::string &viewId,
	uint64_t fileSize,
	const std::string &clientToken,
	bool pause,	// Set the initial state to Pause (used when an incoming call is detected while initiating fileplay.
	bool useDownloadSpeed)
{
	stiLOG_ENTRY_NAME (CFilePlayGS::LoadVideoMessage);

	PostEvent ([this, server, fileGUID, viewId, fileSize, clientToken, pause, useDownloadSpeed]
	{
		stiHResult hResult = stiRESULT_SUCCESS; 
		stiTESTCOND(!m_closedDuringGUIDRequest, stiRESULT_ERROR);

		m_guidIsGreeting = true;

		eventLoadVideoMessage (server,
							   fileGUID,
							   viewId,
							   fileSize,
							   clientToken,
							   pause,
							   useDownloadSpeed);

STI_BAIL:
		if (stiIS_ERROR(hResult) &&
			m_closedDuringGUIDRequest)
		{
			m_closedDuringGUIDRequest = false;
			EventCloseMovie ();
		}
	});

	return stiRESULT_SUCCESS;
}

void CFilePlayGS::eventLoadVideoMessage (
	const std::string &server,
	const std::string &fileGUID,
	const std::string &viewId,
	uint64_t fileSize,
	const std::string &clientToken,
	bool pause,	
	bool useDownloadSpeed)
{
	stiLOG_ENTRY_NAME (CFilePlayGS::eventLoadVideoMessage);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::eventLoadVideoMessage\n");
		stiTrace("\tserver = %s fileGUID = %s StateName = %s\n", server.c_str(), fileGUID.c_str(), StateNameGet (m_eState));
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// If we are closing the video then don't try to open it.
	stiTESTCOND (!StateValidate(IstiMessageViewer::estiCLOSING), stiRESULT_ERROR);

	// When in the RECORD_FINSISHED state we are opeing a recorded message so don't reset
	// dataType or openPaused.
	if (!StateValidate (IstiMessageViewer::estiRECORD_FINISHED))
	{
		m_unDataType = estiDOWNLOADED_DATA;
		m_openPaused = pause;
	}

	stiTESTCOND (StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiPLAYER_IDLE |
				     		    IstiMessageViewer::estiREQUESTING_GUID | IstiMessageViewer::estiRECORD_CLOSED | IstiMessageViewer::estiRECORD_FINISHED), stiRESULT_ERROR);

	// When in the RECORD_FINSISHED state we are opeing a recorded message so don't reset
	// dataType or openPaused.
	if (!StateValidate (IstiMessageViewer::estiRECORD_FINISHED))
	{
		m_unDataType = estiDOWNLOADED_DATA;
		m_openPaused = pause;
	}

	StateSet (IstiMessageViewer::estiOPENING);

	//
	// Initialize
	//
	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
	m_playbackStarted = false;

	if (!m_guidIsGreeting)
	{
		m_downloadGuid = fileGUID;
	}
	m_useDownloadSpeed = useDownloadSpeed;

	m_viewId = viewId;

	videoOutputSignalsConnect();

	{
		std::string emptyStr;
		m_pVideoOutput->videoFileLoad(!server.empty() ? server : emptyStr,
									  fileGUID,
									  !clientToken.empty() ? clientToken : emptyStr,
									  useDownloadSpeed ? m_maxDownloadSpeed : 0,
									  m_loopVideo);
	}

	// Clear the load vars to ensure we don't try to reload the video when shutting down.
	m_loadServer.clear ();
	m_loadFileGUID.clear ();
	m_loadClientToken.clear ();
	m_loadViewId.clear ();
	m_loadFileSize = 0;
	m_loadDownloadSpeed = false;
	m_loadPause = false;
	m_loadLoop = false;

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		// If we have a new GUID then set up the variables to start the file when the current file is closed.
		if (fileGUID.compare (m_downloadGuid))
		{
			m_loadServer = server;
			m_loadFileGUID = fileGUID;
			m_loadClientToken = clientToken;
			m_loadViewId = viewId;
			m_loadFileSize = fileSize;
			m_loadDownloadSpeed = useDownloadSpeed;
			m_loadPause = pause;
			m_loadLoop = m_loopVideo;
		}
		else if (!fileGUID.compare (m_downloadGuid))
		{
			// If the Guids are the same then we are already opening the file so don't do anything.
		}
		else
		{
			// If we had an error then notify the UI that there was an error opening the requested file.
			ErrorStateSet(IstiMessageViewer::estiERROR_OPENING);
			StateSet(IstiMessageViewer::estiERROR);
		}
	}

	return;
}


/*!
 * \brief Automatically loops the message back the beginning when the end is reached.
 *  
 * \param bool bLoop 
 *  
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::LoopSet(bool loop)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_loopVideo = loop;

	return hResult;
}


/*!
 * \brief Network disconnected
 * 
 * \return Success or failure result 
 */
void CFilePlayGS::eventNetworkDisconnected ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::eventNetworkDisconnected);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::eventNetworkDisconnected\n");
	);

	if (StateValidate(IstiMessageViewer::estiRECORDING |
					  IstiMessageViewer::estiUPLOADING |
					  IstiMessageViewer::estiWAITING_TO_RECORD))
	{
		PostEvent ([this]{eventRecordError();}); 
	}
}


/*!
 * \brief Event: Bad GUID
 */
void CFilePlayGS::EventBadGUID ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventBadGUID);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventBadGUID\n");
	);

	BadGuidReported ();
}


/*!
 * \brief Bad Guid Reported
 */
void CFilePlayGS::BadGuidReported ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::BadGuidReported);

	m_closeDueToBadGuid = true;

    if (nMAX_GUID_REQUEST_RETRIES <= m_nGuidRequests)
    {
        ErrorStateSet(estiERROR_OPENING);
        StateSet(IstiMessageViewer::estiERROR);
    } 
    else if (StateValidate(estiOPENING |
					       estiRECORD_CONFIGURE |
					       estiWAITING_TO_RECORD |
					       estiRECORDING))
	{
		m_nGuidRequests++;

		if (StateValidate(estiOPENING))
		{
			StateSet (IstiMessageViewer::estiRELOADING);
		}

//		InitiateClose ();
	}

} // end CFilePlayGS::BadGuidReported


/*!
 * \brief Event: Movie Ready 
 * 
 */
void CFilePlayGS::EventMovieReady ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventMovieReady);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventMovieReady\n");
	);

    if (StateValidate (IstiMessageViewer::estiOPENING) ||
        (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL &&
         StateValidate (IstiMessageViewer::estiRECORD_FINISHED)))
    {
		// Clear the download failed bool.
		m_downloadFailed = false;

		// Set the playrate to normal.
		m_playRate = ePLAY_NORMAL;

        // Reset the splunk frame so we can track buffering.
        m_bufferToEnd = false;

		// If we have a loadServer then we are waiting for a different movie to load so we can close 
		// this file and load the one that is waiting.
		if (!m_loadServer.empty())
		{
			m_pVideoOutput->videoFilePlay(ePLAY_PAUSED);
			PostEvent ([this]{EventCloseMovie();}); 
		}
		// Movie is ready to play
        else if (m_openPaused)
        {
			m_pVideoOutput->videoFilePlay(ePLAY_PAUSED);
            StateSet (IstiMessageViewer::estiPAUSED);
        }
        else
        {
            StateSet (IstiMessageViewer::estiPLAY_WHEN_READY);
        }

        stiDEBUG_TOOL (g_stiFilePlayDebug,
            stiTrace("\tOK! Movie is ready to play\n");
        );
    }
}


/*!
 * \brief Event: Media 
 * 
 */
void CFilePlayGS::EventMediaReady ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventMediaReady);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventMediaReady\n");
	);

	if (StateValidate (IstiMessageViewer::estiPLAY_WHEN_READY))
	{
		PostEvent([this]{EventPlay();});
	}
}


/*!
 * \brief Event: Message Size 
 * 
 * \param uint32_t messageSize
 */
void CFilePlayGS::EventMessageSize (
	uint32_t messageSize)
{
	m_un32RecordSize = messageSize;
}


/*!
 * \brief Event: Movie Closed 
 *  
 */
void CFilePlayGS::EventCloseMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventCloseMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventCloseMovie\n");
	);

	// If we are requesting a GUID set closedDuringGUIDRequest so 
	// that when it comes we don't play the video.
	if (StateValidate (IstiMessageViewer::estiREQUESTING_GUID))
	{
		m_closedDuringGUIDRequest = true;
	}

	if (!StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiCLOSING | 
						IstiMessageViewer::estiPLAYER_IDLE | IstiMessageViewer::estiRECORD_CLOSING | 
						IstiMessageViewer::estiRECORD_CLOSED) ||
		m_eError == estiERROR_RECORDING)
	{
		// If we are in the UPLOAD_COMPLETE state we aren't playing so don't call VideoFileStop(), it can
		// cause a race conditiion if we are trying to rerecord.
		if (!StateValidate(IstiMessageViewer::estiUPLOAD_COMPLETE))
		{
			m_pVideoOutput->VideoFileStop();
		}
		InitiateClose ();
	}

	// If  videoOutputSignalConnections is empty then videoOutput
	// has already been shutdown so clean up.
	else if (m_videoOutputSignalConnections.empty() &&
			    StateValidate(IstiMessageViewer::estiCLOSING))
	{
		PostEvent ([this]{EventMovieClosed();}); 
	}
}


/*!
 * \brief Event: Movie Closed 
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CFilePlayGS::EventMovieClosed ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventMovieClosed);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventMovieClosed\n");
	);

	videoOutputSignalsDisconnect();

	// Make sure the next message will not be set to looping.
	m_loopVideo = false;

	m_downloadGuid.clear();
	m_server.clear();
	m_guidIsGreeting = false;

	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
	if (StateValidate (IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiERROR))
	{
		StateSet (IstiMessageViewer::estiCLOSED);
		ErrorStateSet(IstiMessageViewer::estiERROR_NONE);
	}
	else if (StateValidate(IstiMessageViewer::estiRECORD_CLOSING) &&
			 (m_eRecordType != IstiMessageViewer::estiRECORDING_UPLOAD_URL ||
			  m_eError == IstiMessageViewer::estiERROR_RECORDING))
	{
		StateSet (IstiMessageViewer::estiRECORD_CLOSED);
	}

	// If we are in this error state than we recorded less than 1 sec. and we don't want
	// to send it.  If we have a messageID we have uploaded data so make sure we
	// notify the server to delete it.
	if (ErrorGet() == IstiMessageViewer::estiERROR_NO_DATA_UPLOADED &&
		!m_messageId.empty())
	{
		DeleteRecordedMessage();
	}

	if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_IDLE &&
		m_cleanupRecordedFile)
	{
		// Clean up the file if needed.
		m_cleanupRecordedFile = false;
		recordedFileDelete();
	}
}


/*!
 * \brief Event: Upload Greeting
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CFilePlayGS::EventGreetingUpload (
	std::string uploadGUID,
	std::string uploadServer) 
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventGreetingUpload);
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventGreetingUpload\n");
	);

	// Store the UploadServer.
	m_uploadServer = uploadServer;

	// Store the UploadGUID.
	m_uploadGUID = uploadGUID;

	m_uploadGUID += "&PixelWidth=";
	m_uploadGUID += std::to_string(m_un32RecordWidth);
	m_uploadGUID += "&PixelHeight=";
	m_uploadGUID += std::to_string(m_un32RecordHeight);
	m_uploadGUID += "&Bitrate=";
	m_uploadGUID += std::to_string(m_un32CurrentRecordBitrate);
	m_uploadGUID += "&DurationSeconds=";
	m_uploadGUID += std::to_string(m_un32RecordLength);

	ErrorStateSet(IstiMessageViewer::estiERROR_NONE);

	// Upload the Greeting.
	// Set the upload progress to 0.
	fileUploadStart();
}

void CFilePlayGS::eventGreetingImageUpload()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string proxyAddress;
	uint16_t proxyPort = 0;

    std::string protocol1;
    std::string server1;
    std::string file1;

    std::string protocol2;
    std::string server2;
    std::string file2;

	CstiIpAddress IpAddress;
	CstiHTTPService httpService;

	time_t seconds = time(nullptr);
	std::string boundry;
	std::stringstream secondsStream;
	secondsStream << seconds;
    boundry = "----------";
	boundry += secondsStream.str();

    std::stringstream payload_preamble;
    std::stringstream footer;
    std::stringstream header;

    std::string serverIP;
	int nSelectRet = 0; stiUNUSED_ARG (nSelectRet);
    char headerBuffer[g_nHEADER_BUFFER_SIZE];
    stiSocket uploadSocket = stiINVALID_SOCKET;
    char *pRequest = nullptr;

    SHTTPResult stResult;
    EHTTPServiceResult eServiceResult; stiUNUSED_ARG (eServiceResult);

	stiTESTCOND(!m_uploadGUID.empty() && !m_uploadServer.empty(), stiRESULT_INVALID_PARAMETER);

    hResult = stiDNSGetHostByName(m_uploadServer.c_str(), nullptr, &serverIP);
    stiTESTCOND (hResult == stiRESULT_SUCCESS, stiRESULT_SRVR_DATA_HNDLR_UPLOAD_ERROR);

	stiTESTCOND(!m_frameCaptureData.empty (), stiRESULT_INVALID_PARAMETER);

	// Check for a proxy address and port;
	CstiHTTPService::ProxyAddressGet(&proxyAddress, &proxyPort);

    payload_preamble << "--" << boundry << "\r\n"
                     << "Content-Disposition: form-data; name=\"uploadForm\"; filename=\"greetingImage.jpg\"\r\n"
					 << "Content-Type: image/jpeg\r\n"
					 << "\r\n";

    footer << "\r\n--" << boundry << "\r\n";

	header << "POST ";

	if (proxyAddress.length () > 0 && proxyPort != 0)
	{
		header << "http://" << m_uploadServer;
	}

	header << "/" << m_uploadGUID << "&PreviewImage=True HTTP/1.1\r\n"
           << "Content-Type: multipart/form-data; boundary=" << boundry << "\r\n"
#ifdef stiMVRS_APP
           << "Cache-Control: no-cache\r\n"
#endif
           << "Host: " << m_uploadServer << "\r\n"
		   << "Content-Length: " << (m_frameCaptureData.size () + payload_preamble.str().length() + footer.str().length())
           << "\r\nConnection: Keep-Alive\r\n"
            << "\r\n";

    char *pIndexPtr;
	unsigned long requestSize;
    requestSize = header.str().length() + payload_preamble.str().length() + m_frameCaptureData.size () + footer.str().length();
    pRequest = new char[requestSize];
    pIndexPtr = pRequest;
    memcpy(pIndexPtr, header.str().c_str(), header.str().length());
    pIndexPtr += header.str().length();

    memcpy(pIndexPtr, payload_preamble.str().c_str(), payload_preamble.str().length());
    pIndexPtr += payload_preamble.str().length();

    memcpy(pIndexPtr, m_frameCaptureData.data (), m_frameCaptureData.size ());
    pIndexPtr += m_frameCaptureData.size ();
     
    memcpy(pIndexPtr, footer.str().c_str(), footer.str().length());
	
	httpService.ServerAddressSet(m_uploadServer.c_str(), serverIP.c_str(), 80);

	// Initialize the socket.
	hResult = httpService.SocketOpen();
	stiTESTRESULT();

	// Get the socket to write to.
	uploadSocket = httpService.SocketGet();


    // Write the header request.
	{
		auto nResult = stiOSWrite(uploadSocket, pRequest, requestSize);
		stiTESTCOND (nResult != -1, stiRESULT_SRVR_DATA_HNDLR_UPLOAD_ERROR);
	}

    // Make sure the request was OK.
	hResult = HttpHeaderRead(uploadSocket, g_nREAD_TIMEOUT, 
								  headerBuffer, sizeof(headerBuffer) - 1, 
								  nullptr, &stResult);
	stiTESTRESULT();


    // Look for a return code of 100 to indicate that we can continue uploading.
    if (stResult.unReturnCode != 200)
    {
        stiTHROW (stiRESULT_SRVR_DATA_HNDLR_UPLOAD_ERROR);
    }

	greetingImageUploadCompleteSignal.Emit();

STI_BAIL:

	httpService.SocketClose();

    delete []pRequest;
    pRequest = nullptr;
}


/*!
 * \brief Event: Upload Complete 
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CFilePlayGS::eventFileUploadError ()
{
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventFileUploadError\n");
	);

	ErrorStateSet(estiERROR_RECORDING);
	StateSet(IstiMessageViewer::estiERROR);

	// Set the state to record_closed so we are in a state that can record again.
	StateSet(IstiMessageViewer::estiRECORD_CLOSED);
}


/*!
 * \brief Event: Upload Message Id 
 * 
 * \param char *messageId
 */
void CFilePlayGS::EventUploadMessageId (
	std::string messageId)  
{
	m_messageId = messageId;
}

/*!
 * \brief Event: Upload Progress 
 * 
 * \param uint32_t progress 
 */
void CFilePlayGS::EventUploadProgress (
	uint32_t progress)  // The event to be handled
{

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventUploadProgress\n");
	);

	m_CurrentProgress.un32BufferingPercentage = progress;

	if (m_CurrentProgress.un32BufferingPercentage == 100 &&
		ErrorGet() == estiERROR_NONE)
	{
		StateSet(IstiMessageViewer::estiUPLOAD_COMPLETE);
	}
}


/*!
 * \brief Initiate Close 
 *  If the state is RELOADING or RECORD_CONFIGURE don't set the state to CLOSING because ::EventMovieCosed   
 *  will look for these states to handle the closing of the movie differently.                               
 *  
 */
void CFilePlayGS::InitiateClose ()
{
	stiLOG_ENTRY_NAME (CFilePlay::InitiateClose);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::InitiateClose\n");
	);

	// If the state is RELOADING or RECORD_CONFIGURE don't set the state to CLOSING because ::EventMovieCosed
	// will look for these states to handle the closing of the movie differently. If we are in the RECORD_CLOSED, 
	// CLOSED or PLAYER_IDLE state we are already closed so don't do anything.
	if (!StateValidate (IstiMessageViewer::estiRELOADING | IstiMessageViewer::estiRECORD_CONFIGURE |
									IstiMessageViewer::estiRECORD_CLOSED | IstiMessageViewer::estiCLOSED | 
									IstiMessageViewer::estiPLAYER_IDLE))
	{
		if ((StateValidate(IstiMessageViewer::estiUPLOAD_COMPLETE) && m_videoOutputSignalConnections.empty()) ||
			(StateValidate(IstiMessageViewer::estiRECORD_CLOSING) && m_eError == estiERROR_RECORDING))                        
		{
			StateSet(IstiMessageViewer::estiRECORD_CLOSED);
		}
		else if (StateValidate(IstiMessageViewer::estiRECORD_FINISHED | IstiMessageViewer::estiUPLOADING) ||
				 m_unDataType == estiRECORDED_DATA ||
				 (StateValidate(IstiMessageViewer::estiERROR) && m_eError == estiERROR_RECORDING))
		{                       
			StateSet(IstiMessageViewer::estiRECORD_CLOSING);
		}
		else
		{
			StateSet(IstiMessageViewer::estiCLOSING);
		}
	}

	// If we have a movie then clean it up.  If we don't then it is already cleanedup and we
	// just need to close.
	if (m_videoOutputSignalConnections.empty())
	{
		PostEvent ([this]{EventMovieClosed();});
	}
}


/*!
 * \brief Closes the message that is currently playing.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if in the IstiMessageViewer::estiOPENING state, the message cannot be
 *  	   closed untill the Message Viewer is out of this state. 
 */
stiHResult CFilePlayGS::Close ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Close);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::Close\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL ||
		m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
	{
		m_eRecordType = IstiMessageViewer::estiRECORDING_IDLE;
        m_loadCountdownVideo = false;
	}

	if (StateValidate(IstiMessageViewer::estiOPENING) || 
		(m_unDataType == estiRECORDED_DATA && 
		m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL))
	{
		//stiTrace ("State = %d, m_unDataType = %d, m_eRecordType = %d\n", StateGet (), m_unDataType, m_eRecordType);
		//stiBackTrace ();
		stiTHROW (stiRESULT_ERROR);
	}

	if (StateValidate(IstiMessageViewer::estiERROR) &&
		ErrorGet() == estiERROR_OPENING)
	{
		PostEvent ([this]{EventMovieClosed();}); 
	}
	else
	{
		PostEvent ([this]{EventCloseMovie();}); 
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Close Movie
 */
void CFilePlayGS::CloseMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::CloseMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::CloseMovie\n");
	);

	if (m_unDataType != estiRECORDED_DATA || 
		(m_eRecordType != IstiMessageViewer::estiRECORDING_NO_URL &&
		m_eRecordType != IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL))
	{
		PostEvent ([this]{EventCloseMovie();});
	}
}


/*!
 * \brief Returns the current position of the  message, in seconds
 * 
 * \return Current position in seconds as a uint32_t 
 */
uint32_t  CFilePlayGS::StopPositionGet()
{
	int seconds = 0;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_CurrentProgress.un64CurrentMediaTime > 0)
	{
		seconds = (int)(m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale);
		if (seconds == (int)(m_CurrentProgress.un64MediaDuration / m_CurrentProgress.un64MediaTimeScale))
		{
			seconds = 0;
		}
	}
	else
	{
	   seconds = m_un32StartPos;
	}

	return seconds;
}


/*!
 * \brief Sets the position, in seconds, the message should start playing at.
 * 
 * \param un32StartPosition specifies the start position in seconds.
 */
void CFilePlayGS::SetStartPosition(
	uint32_t  un32StartPosition)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_un32StartPos = un32StartPosition;
}


/*!
 * \brief Places the Message Viewer into slow forward mode. The message will play 
 * 		  at 1/2X speed. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SlowForward ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::SlowForward);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::SlowForward\n");
	);

	PostEvent([this]{EventSlowForward();});

	return hResult;
}


/*!
 * \brief Event: Slow Forward
 * 
 */
void CFilePlayGS::EventSlowForward () 
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventSlowFoward);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventSlowForward\n");
	);

	m_pVideoOutput->videoFilePlay(ePLAY_SLOW);
	m_playRate = ePLAY_SLOW;
	StateSet (IstiMessageViewer::estiPLAYING);
}


/*!
 * \brief Places the Message Viewer into fast forward mode. The message will play 
 * 		  at 2X speed. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::FastForward ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{EventFastForward();});
	
	return hResult;
}


/*!
 * \brief Event: Slow Forward
 * 
 */
void CFilePlayGS::EventFastForward ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventFastFoward);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventFastForward\n");
	);

	m_pVideoOutput->videoFilePlay(ePLAY_FAST);
	m_playRate = ePLAY_FAST;
	StateSet (IstiMessageViewer::estiPLAYING);
}


/*!
 * \brief Plays message.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::Play ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Play);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::Play\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Play should always set the playrate back to normal.

	if (m_playRate == ePLAY_FAST || m_playRate == ePLAY_SLOW)
	{
		m_playRate = ePLAY_NORMAL;
	}

	if (!StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiCLOSING | 
						IstiMessageViewer::estiRELOADING | IstiMessageViewer::estiERROR))
	{
		if (StateValidate(IstiMessageViewer::estiOPENING))
		{
			m_openPaused = false;
		}
		else if (StateValidate(IstiMessageViewer::estiPAUSED) &&
				 (0 != m_CurrentProgress.un64BytesYetToDownload &&
				 m_CurrentProgress.un32BufferingPercentage < 95)) 
		{
			StateSet (IstiMessageViewer::estiPLAY_WHEN_READY);
		}
		else
		{
			PostEvent([this]{EventPlay();});
		}
	}

	return hResult;
}


/*!
 * \brief Event Play 
 * 
 */
void CFilePlayGS::EventPlay ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventPlay);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventPlay\n");
	);

	m_openPaused = false;

	m_pVideoOutput->videoFilePlay(ePLAY_NORMAL);
	StateSet (IstiMessageViewer::estiPLAYING);
}

/*!
 * \brief Event: File End 
 * 
 */
void CFilePlayGS::eventVideoEnd ()
{
	StateSet(IstiMessageViewer::estiVIDEO_END);
}


/*!
 * \brief Pauses the Message Viewer
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::Pause ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Pause);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::Pause\n");
	);

	PostEvent([this]{EventPause();});

	return hResult;
}


/*!
 * \brief EventPause ( 
 * 
 */
void CFilePlayGS::EventPause ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventPause);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventPause\n");
	);

	if (StateValidate (IstiMessageViewer::estiOPENING))
	{
		m_openPaused = true;
	}
	else if (StateValidate (IstiMessageViewer::estiPLAYING | IstiMessageViewer::estiPLAY_WHEN_READY | IstiMessageViewer::estiVIDEO_END))
	{
		m_pVideoOutput->videoFilePlay(ePLAY_PAUSED);
		StateSet (IstiMessageViewer::estiPAUSED);
	}
}


/*!
 * \brief Configures the Message Viewer so that it can record a message.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
void CFilePlayGS::EventRecordConfig ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// If we are in a closing state we are in the process of shutting down
	// a failed record.  We have to let this finish before we can begin
	// recording again.
	if (!StateValidate(IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiRECORD_CLOSING | 
					   IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiWAITING_TO_RECORD) &&
		m_eRecordType != IstiMessageViewer::estiRECORDING_IDLE)
	{
		if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL ||
			m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL )
		{
			// Make sure that the error state has been cleared.
			ErrorStateSet(IstiMessageViewer::estiERROR_NONE);
			StateSet(IstiMessageViewer::estiRECORD_CONFIGURE);

			// Make sure that we have an upload server and an upload GUID.
			if (m_uploadServer.empty() || m_uploadGUID.empty())
			{
				// If we are here (no upload server or upload GUID) we shouldn't have
				// a messageId so clear it.
				m_messageId.clear();
				requestUploadGUIDSignal.Emit();
				ErrorStateSet(IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE);

				stiTHROW(stiRESULT_ERROR);
			}

			m_un32IntraFrameRate = DEFAULT_INTRA_FRAME_RATE;
		}
		else
		{
			ErrorStateSet(IstiMessageViewer::estiERROR_NONE);

			m_un32IntraFrameRate = DEFAULT_LOW_INTRA_FRAME_RATE;

			// Set the starting record bitrate to be the lesser of the upload bitrate or DEFAULT_RECORD_BITRATE.
			m_un32MaxRecordBitrate = m_maxUploadSpeed >= DEFAULT_RECORD_BITRATE ? DEFAULT_RECORD_BITRATE : m_maxUploadSpeed;

			m_uploadServer.clear();
			m_uploadGUID.clear();
		}

		// Set the frame dimensions.
		if (m_un32MaxRecordBitrate == (uint32_t)DEFAULT_RECORD_BITRATE)
		{
			m_un32RecordHeight = DEFAULT_RECORD_VGA_ROWS; 
			m_un32RecordWidth = DEFAULT_RECORD_VGA_COLS;  
		}
		else
		{
			m_un32RecordHeight = DEFAULT_RECORD_ROWS;
			m_un32RecordWidth = DEFAULT_RECORD_COLS;
		}

		// Initialize the movie upload.
		m_unDataType = estiRECORDED_DATA;

		// Make sure that the current record bitrate is set to the max bitrate.
		m_un32CurrentRecordBitrate = m_un32MaxRecordBitrate;

		hResult = VideoRecordSettingsSet();

		StateSet(IstiMessageViewer::estiWAITING_TO_RECORD);
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		if (ErrorGet() == estiERROR_NONE)
		{
			ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
		}
		StateSet(IstiMessageViewer::estiERROR);
	}

	// If we are in the closing state then we didn't setup the recording because
	// we are waiting for the current video to close.
	else if (StateValidate(IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiRECORD_CLOSING))
	{
		hResult = stiMAKE_ERROR(stiRESULT_ERROR);
	}

}


/*!
 * \brief Used to notify the Message Viewer to stop recording but does not 
 * 		  generate an error.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::RecordingInterrupted ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (StateValidate(IstiMessageViewer::estiRECORDING))
	{
		VideoInputSignalsDisconnect ();
		hResult = m_pVideoInput->VideoRecordStop();

		m_bReceivingVideo = false;
	}

	return (hResult);
}


/*!
 * \brief Event Record Error
 * 
 */
void CFilePlayGS::eventRecordError ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiBool bCloseMovie = estiFALSE;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventRecordError\n");
	);

	if (ErrorGet() != estiERROR_RECORDING)
	{
		// Stop Recording.
		hResult = m_pVideoInput->VideoRecordStop();
		stiTESTRESULT ();
		
		m_bReceivingVideo = false;

		if (!StateValidate(IstiMessageViewer::estiRECORD_FINISHED))
		{
			bCloseMovie = estiTRUE;
		}

		// Set the error state and notify the engine of the error..
		ErrorStateSet(estiERROR_RECORDING);
		StateSet(IstiMessageViewer::estiERROR);

		// Report to splunk that we had a record error.
		stiRemoteLogEventSend ("EventType=FilePlayer Reason=RecordingErrorGeneric");

		// After setting error state set state to record closing.
		StateSet(IstiMessageViewer::estiRECORD_CLOSING);

		if (bCloseMovie)
		{
			// Shutdown the file data handling.
//			InitiateClose();
			PostEvent([this]{EventMovieClosed();});
		}
	}

STI_BAIL:

	return;
}


/*!
 * \brief Connects to signals from VideoInput so we can receive frames
 */
void CFilePlayGS::VideoInputSignalsConnect ()
{
	if (m_videoInputSignalConnections.empty())
	{
		m_videoInputSignalConnections.push_back(
			m_pVideoInput->frameCaptureAvailableSignalGet ().Connect (
				[this](const std::vector<uint8_t> &frame){
					std::lock_guard<std::recursive_mutex> lock (m_execMutex);
					m_frameCaptureData = frame;
				}));

		m_videoInputSignalConnections.push_back(
			m_pVideoInput->videoRecordPositionSignalGet ().Connect (
				[this](uint64_t position, uint64_t timeScale)
				{
					std::lock_guard<std::recursive_mutex> lock (m_execMutex);
					m_CurrentProgress.un64CurrentMediaTime = position;
					m_CurrentProgress.un64MediaTimeScale = timeScale; 
				}));

		m_videoInputSignalConnections.push_back(
			m_pVideoInput->videoRecordFinishedSignalGet ().Connect (
				[this](const std::string filePath, const std::string fileName){
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);
					m_recordFilePath = filePath;
					m_recordFileName = fileName;
					PostEvent([this]{eventRecordFinished ();});
				}));

		m_videoInputSignalConnections.push_back(
			m_pVideoInput->videoRecordErrorSignalGet ().Connect(
				[this](){
					PostEvent([this]{eventRecordError ();});
				}));

		m_videoInputSignalConnections.push_back(
			m_pVideoInput->videoRecordEncodeCameraErrorSignalGet ().Connect (
				[this](){
					std::lock_guard<std::recursive_mutex> lock (m_execMutex);

					m_encodeCameraErrorCount++;
				}));
	}
}


/*!
 * \brief Disconnects from VideoInput signals so we no longer get frames
 */
void CFilePlayGS::VideoInputSignalsDisconnect ()
{
	// Release all of the connections to the video input signals.
	m_videoInputSignalConnections.clear();
}


/*!
 * \brief Connects to signals from VideoOutput so we can play a video
 */
void CFilePlayGS::videoOutputSignalsConnect ()
{
	if (m_videoOutputSignalConnections.empty())
	{
		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileStartFailedSignalGet().Connect (
				[this]
				{
					PostEvent([this]{EventDownloadError();});
				}));

		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileCreatedSignalGet().Connect (
				[this]
				{
					PostEvent([this]{EventMovieReady();});
				}));

		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileReadyToPlaySignalGet().Connect (
				[this]
				{
					PostEvent([this]{EventMediaReady();});
				}));

		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileEndSignalGet().Connect (
				[this]
				{
					PostEvent([this]{eventVideoEnd();});
				}));

		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileClosedSignalGet().Connect (
				[this](const std::string fileGUID)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);

					// If there is a GUID then we are closing the file we are playing.  If not
					// we are proably playing a screensaver and trying to start another video, so don't 
					// close the video.  If we are in an error state we failed while opening so close the file.
    				if (!fileGUID.empty() ||
						StateValidate(IstiMessageViewer::estiERROR))
					{
						PostEvent([this]{EventMovieClosed();});
					}
				}));

		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFilePlayProgressSignalGet().Connect (
				[this](uint64_t position, uint64_t duration, uint64_t timeScale)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);   
					m_CurrentProgress.un64CurrentMediaTime = position;
					m_CurrentProgress.un64MediaDuration = duration;
					m_CurrentProgress.un64MediaTimeScale = timeScale; 
				}));
	
		m_videoOutputSignalConnections.push_back(
			m_pVideoOutput->videoFileSeekReadySignalGet().Connect (
				[this]
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);

					// If startPos is within 1 second of the end then restart at 0.
					if (m_CurrentProgress.un64MediaTimeScale > 0 &&
						m_un32StartPos >= m_CurrentProgress.un64MediaDuration / m_CurrentProgress.un64MediaTimeScale - 1)
					{
						m_un32StartPos = 0;
					}

					SkipForward(m_un32StartPos);
				}));
	}
}


/*!
 * \brief Disconnects from VideoOuput signals
 */
void CFilePlayGS::videoOutputSignalsDisconnect ()
{
	// Release all of the connections to the video input signals.
	m_videoOutputSignalConnections.clear();
}

/*!
 * \brief Event: Record Start 
 * 
 */
void CFilePlayGS::EventRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND(StateValidate(IstiMessageViewer::estiWAITING_TO_RECORD), 
				stiRESULT_ERROR);

	stiTESTCOND((IstiNetwork::InstanceGet ()->ServiceStateGet () == estiServiceOnline) ||
				(IstiNetwork::InstanceGet ()->ServiceStateGet () == estiServiceReady), 
				stiRESULT_ERROR);

	// Reset the video time.
	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
	m_CurrentProgress.un64MediaTimeScale = g_nVIDEO_RECORD_TIME_SCALE;

	//
	// Register a callback with the video input object so that we can start
	// receiving frames.
	//
	VideoInputSignalsConnect ();
	
	if (RecordTypeGet() == estiRECORDING_NO_URL)
	{
		hResult = m_pVideoInput->FrameCaptureRequest(IstiVideoInput::eCaptureOnRecordStart, nullptr);
	}

	// Start grabing frames from the camera.
	hResult =  m_pVideoInput->VideoRecordStart();
	stiTESTRESULT ();

	//
	// Note that we are now receiving video so, in case we shutdown prematurely, we can stop the flow of video.
	//
	m_bReceivingVideo = true;

	StateSet(IstiMessageViewer::estiRECORDING);

	// Clear the load countdown variable so we don't load and play the countdown
	// when we finish recording (this could happen TERP NET skips to record)
	m_loadCountdownVideo = false;

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		// See if we are in an idle state if so don't set an error.  We just
		// stopped the recording process, while this event was in the queue.
		if (!StateValidate(IstiMessageViewer::estiPLAYER_IDLE))
		{
			StateSet(IstiMessageViewer::estiERROR);
			ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);

			// Report to splunk that we had a record error.
			stiRemoteLogEventSend ("EventType=FilePlayer Reason=RecordingErrorStartFailed");
		}
	}
}


/*!
 * \brief Event: Record Stop 
 * 
 */
void CFilePlayGS::EventRecordStop ()
{
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventRecordStop\n");
	);

	if (StateValidate(IstiMessageViewer::estiRECORDING))
	{
		m_pVideoInput->VideoRecordStop();

		m_bReceivingVideo = false;
	}
}


/*!
 * \brief Event: Record Finished 
 * 
 */
void CFilePlayGS::eventRecordFinished ()
{
	if (StateValidate(IstiMessageViewer::estiRECORDING))
	{
		// Calculate the record time of the video.
		m_un32RecordLength = static_cast<uint32_t>(m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale);

		if (m_encodeCameraErrorCount == 1 &&
			m_un32RecordLength == 0)
		{
			// Clean up any bad files.
			recordedFileDelete();

			// We had a cameraError on start of recording so try to restart recording.
			m_restartRecordErrorTimer.start();
		}
		else
		{
			StateSet(IstiMessageViewer::estiRECORD_FINISHED);
		
			VideoInputSignalsDisconnect ();

			if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
			{
				// Set the upload progress to 0.
				m_CurrentProgress.un32BufferingPercentage = 0;
			
				// If our record time is less than 0 we probably haven't uploaded
				// any data so just shut down the record process and don't try
				// to send the data.
				if (m_un32RecordLength > 0 &&
					m_encodeCameraErrorCount < 2)
				{
					ErrorStateSet(IstiMessageViewer::estiERROR_NONE);
					fileUploadStart();
				}
				else
				{
					ErrorStateSet(IstiMessageViewer::estiERROR_NO_DATA_UPLOADED);
					StateSet(IstiMessageViewer::estiERROR);
				}
			}
			else if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL ||
					 m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
			{
				if (m_un32RecordLength > 0 &&
					m_encodeCameraErrorCount < 2)
				{
					if (m_waitingToUploadRecordedMsg)
					{
						m_waitingToUploadRecordedMsg = false;
						SendRecordedMessage ();
					}
					else
					{
						// Prep the recorded video for viewing.
						std::string videoFile = m_recordFilePath + m_recordFileName;
						std::string emptyServer;

						SetStartPosition(0);
						Open(emptyServer, videoFile);
						m_openPaused = true;

						// Reset DataType so we don't close the file.
						m_unDataType = estiRECORDED_DATA;
					}
				}
				else
				{
					ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
					StateSet(IstiMessageViewer::estiERROR);
				}
			}
		}
	}
}


/*!
 * \brief Used to clear the messageId when rerecording a signmail message.
 * 		  
 * 
 * \return Success or failure result  
 */
stiHResult CFilePlayGS::RecordMessageIdClear()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_messageId.clear();

	return hResult;
}


void CFilePlayGS::recordedMessageCleanup ()
{

	if (StateValidate(IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiRECORD_CONFIGURE | 
					  IstiMessageViewer::estiWAITING_TO_RECORD))
	{
		RecordStop();
	}

	m_cleanupRecordedFile = true;

	// Clear the camera error count;
	m_encodeCameraErrorCount = 0;

	EventCloseMovie();
	fileUploadFinish();
}


/*!
 * \brief Calls RecordP2PMessageInfoClear.
 * 
 */
void CFilePlayGS::EventRecordP2PMessageInfoClear ()
{
	RecordP2PMessageInfoClear();
}


void CFilePlayGS::SignMailInfoClear ()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	// Clean up the server and upload GUID so we don't send a new message
	// that overwrites this one.
	m_uploadServer.clear();
	m_uploadGUID.clear();
    m_phoneNumber.clear();
	m_messageId.clear();
	m_spSignMailCall = nullptr;
}


/*! 
 * \brief Clears conifiguration information, used when recording a point to point SignMail, 
 * 		 from the Message Viewer. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::RecordP2PMessageInfoClear ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	SignMailInfoClear ();

	// Set the state as IDLE so that we don't reset these
	// member variables with bad data.
	StateSet(IstiMessageViewer::estiPLAYER_IDLE);

	// Set the record tpye to idle.
	RecordTypeSet(IstiMessageViewer::estiRECORDING_IDLE);
	
	// Set the error state back to none.
	ErrorStateSet(IstiMessageViewer::estiERROR_NONE);

	// Clear the camera error count;
	m_encodeCameraErrorCount = 0;

	return hResult;
}


/*!
 * \brief Set Record P2P Message Info
 * 
 * \param uint32_t un32RecordTime 
 * \param uint32_t un32DownBitrate 
 * \param uint32_t un32UpBitrate 
 * \param const char* pszServer 
 * \param const char* pszUploadGUID 
 * \param const char* pszToPhoneNum 
 *  
 */
void CFilePlayGS::RecordP2PMessageInfoSet (
	uint32_t un32RecordTime,
	uint32_t un32DownBitrate,
	uint32_t un32UpBitrate,
	const std::string &server,
	const std::string &uploadGUID,
	const std::string &phoneNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (!StateValidate(IstiMessageViewer::estiWAITING_TO_RECORD | IstiMessageViewer::estiRECORDING))
	{
		// Check to see if we have a messageID.  If we do then we are in
		// the process of closing down and we don't wan't to record a message.
		stiTESTCOND (m_messageId.empty(), stiRESULT_ERROR);

		m_un32RecordTime = un32RecordTime;

		// Set the starting record bitrate to be the lesser of the upload bitrate or DEFAULT_RECORD_BITRATE.
		m_un32MaxRecordBitrate = un32UpBitrate >= (uint32_t)DEFAULT_RECORD_BITRATE ? DEFAULT_RECORD_BITRATE : un32UpBitrate;
		m_un32CurrentRecordBitrate = m_un32MaxRecordBitrate;

		m_uploadServer = server;

		if (!uploadGUID.empty())
		{
			// We are recording a signmail and we now have an upload GUID.
			RecordTypeSet(estiRECORDING_REVIEW_UPLOAD_URL);

			m_uploadGUID = uploadGUID;
		}

		m_phoneNumber = phoneNumber;
	}

	// Check for an error state.
	if (StateValidate(IstiMessageViewer::estiERROR) &&
		ErrorGet() == IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE)
	{
		// If we are in the record config incomplete state then call RecordConfig()
		PostEvent([this]{EventRecordConfig();});
	}

STI_BAIL:

	return;
}


/*!
 * \brief Start Recording
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlayGS::RecordStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::RecordStart()\n");
	);

	stiTESTCOND(StateValidate(IstiMessageViewer::estiWAITING_TO_RECORD), 
				stiRESULT_ERROR);

	PostEvent([this]{EventRecordStart();});

STI_BAIL:

	return (hResult);
}


/*!
 * \brief Stops recording and prepares the message to be uploaded. If recording is stop because of an 
 * 		  error the messages is closed and will no be uploaded. 
 *  
 * \param bRecordError indicates that recording was stopped because of an error.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::RecordStop(bool bRecordError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::RecordStop()\n");
	);

	if (bRecordError)
	{
		PostEvent([this]{eventRecordError();});
	}
	else
	{
		PostEvent([this]{EventRecordStop();});
	}

	return (hResult);
}


/*!
 * \brief Returns the recording type defined in eRecordType.
 * 
 * \return Returns the recording type as a uint8_t.
 */
IstiMessageViewer::ERecordType CFilePlayGS::RecordTypeGet()
{
	IstiMessageViewer::ERecordType eRecordType = IstiMessageViewer::estiRECORDING_IDLE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	eRecordType = m_eRecordType;

	return eRecordType;
}


/*!
 * \brief Sets the recording type defined in eRecordType.
 * 
 * \param un8RecordType is a type defined in eRecordType.  This defines the 
 * 		  type of recording the message viewer will preform. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::RecordTypeSet(IstiMessageViewer::ERecordType eRecordType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	m_eRecordType = eRecordType;

	return hResult;
}


/*!
 * \brief Deletes the currently recorded SignMail message
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::DeleteRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::DeleteRecordedMessage()\n");
	);

	PostEvent([this]{EventDeleteRecordedMessage();});

	return (hResult);
}


/*!
 * \brief Deletes the currently recorded SignMail message
 * 
 */
void CFilePlayGS::EventDeleteRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SstiRecordedMessageInfo recordedMsgInfo;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
	{
		stiTESTCONDMSG(!m_messageId.empty(), stiRESULT_ERROR, "Can't delete message, missing Message ID");

		recordedMsgInfo.messageId = m_messageId;

		// Null out the messageId.
		m_messageId.clear();

		deleteRecordedMessageSignal.Emit(recordedMsgInfo);
	}

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
	{
		RecordTypeSet(IstiMessageViewer::estiRECORDING_IDLE);

		recordedMessageCleanup ();
	}

STI_BAIL:

	// If we had an error we just need to clean up.  We can't tell core to delete the
	// SignMail (core will do it on its own later).
	RecordP2PMessageInfoClear();

	StateSet (IstiMessageViewer::estiRECORD_CLOSED);
}


/*!
 * \brief Sends the currently recorded SignMail message
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SendRecordedMessage ()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	if (m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
	{
		PostEvent([this]
		{
			if (StateValidate(IstiMessageViewer::estiRECORD_FINISHED | IstiMessageViewer::estiPAUSED))
			{
				//Set the upload progress to 0
				m_CurrentProgress.un32BufferingPercentage = 0;

				ErrorStateSet (IstiMessageViewer::estiERROR_NONE);
				fileUploadStart ();
			}
			else
			{
				m_waitingToUploadRecordedMsg = true;
			}
		});
	}
	else
	{
		PostEvent([this]{eventSendRecordedMessage ();});
	}

	return (stiRESULT_SUCCESS);
}

void CFilePlayGS::eventSendRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SstiRecordedMessageInfo recordedMsgInfo;

	stiTESTCONDMSG(!m_messageId.empty(), stiRESULT_ERROR, "Can't send message, missing Message ID");

	recordedMsgInfo.toPhoneNumber = m_phoneNumber;
	recordedMsgInfo.messageId = m_messageId;

	recordedMsgInfo.un32MessageSize = m_un32RecordSize;
	recordedMsgInfo.un32MessageSeconds = m_un32RecordLength;
	recordedMsgInfo.un32MessageBitrate = (m_un32RecordSize * 8) / m_un32RecordLength;

	// Clear the messageId.
	m_messageId.clear();

	sendRecordedMessageSignal.Emit(recordedMsgInfo);

STI_BAIL:

	StateSet (IstiMessageViewer::estiRECORD_CLOSED);

	RecordP2PMessageInfoClear();
}


/*!
 * \brief Plays the message from the begining.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::Restart ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::Restart);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::Restart\n");
	);

	PostEvent([this]{EventRestart();});

	return (hResult);
}


/*!
 * \brief Event Restart
 * 
 */
void CFilePlayGS::EventRestart ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventRestart);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventRestart\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);

	SetStartPosition(0);

	// If videoOutputSignalConnections is empty then we don't have a file loaded.
	stiTESTCOND(!m_videoOutputSignalConnections.empty(), stiRESULT_INVALID_MEDIA);

	hResult = m_pVideoOutput->videoFileSeek(m_un32StartPos);
	stiTESTRESULT();

STI_BAIL:

	return;
}


/*!
 * \brief Event Download Error
 * 
 */
void CFilePlayGS::EventDownloadError ()
{
	stiLOG_ENTRY_NAME (CFilePlayGS::EventDownloadError);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventDownloadError\n");
	);

	if (StateValidate (IstiMessageViewer::estiOPENING))
		{
			ErrorStateSet(estiERROR_OPENING);
		}
		StateSet(IstiMessageViewer::estiERROR);
}


/*!
 * \brief Event: State Set
 * 
 * \param IstiMessageViewer::EState state 
 *        IstiMessageViewer::EError error
 *  
 * \return Success or failure result 
 */
void CFilePlayGS::EventStateSet (
	IstiMessageViewer::EState state,
	IstiMessageViewer::EError error)
{

    if (state == IstiMessageViewer::estiERROR)
    {
        if (error != estiERROR_NONE)
        {
            ErrorStateSet(error);
        }
        else if (StateValidate(IstiMessageViewer::estiOPENING))
        {
            ErrorStateSet(estiERROR_OPENING);
        }
    }

    StateSet (state);
}


/*!
 * \brief Closes and deletes the current SignMail Message and then plays the countdown 
 * 		  video and begins recording. 
 * 		  
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SignMailRerecord()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::SignMailRerecord()\n");
	);

	if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_UPLOAD_URL ||
		RecordTypeGet () == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
	{
		if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
		{
			if (StateValidate(IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiRECORD_CONFIGURE | IstiMessageViewer::estiWAITING_TO_RECORD))
			{
				RecordStop();
			}

			// Reset the player state.
			StateSet(IstiMessageViewer::estiPLAYER_IDLE);

			RecordMessageIdClear();
		}
		else
		{
			recordedMessageCleanup();
		}

		// Reset the call substate so we can leave a message.
		stiTESTCOND(m_spSignMailCall, stiRESULT_ERROR);
		m_spSignMailCall->NextStateSet(esmiCS_DISCONNECTED, estiSUBSTATE_LEAVE_MESSAGE);

		PostEvent([this]{LoadCountdownVideo();});
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
		StateSet(IstiMessageViewer::estiERROR);
	}

	return hResult;
}


/*!
 * \brief Skips back into the message, the number of seconds indicated by unSkipBack. 
 * 
 * \param unSkipBack specifies the number of seconds to skip back. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SkipBack (
	unsigned int skipBack)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this, skipBack]{EventSkipBack(skipBack);});

	return (hResult);
}


/*!
 * \brief Event: Skip Back
 * 
 * \param skipBack specifies the number of seconds to skip back.
 */
void CFilePlayGS::EventSkipBack (
	unsigned int skipBack)
{
	uint32_t skipTo = 0;

	// Reset to normal speed.
	m_playRate = ePLAY_NORMAL;

	// Get the skipback time and find the needed frame.
	auto currentPos = (uint32_t)(m_CurrentProgress.un64MediaTimeScale > 0 ? m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale : 0);
	skipTo = currentPos > skipBack ? currentPos - skipBack : 0;

	m_pVideoOutput->videoFileSeek(skipTo);

	if (StateValidate(IstiMessageViewer::estiPAUSED | IstiMessageViewer::estiVIDEO_END))
	{
		EventPlay ();
	}
}


/*!
 * \brief Skips forward into the message, the number of seconds indicated by unSkipForward.
 * 
 * \param unSkipForward specifies teh number of seconds to skip forward.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SkipForward (
	uint32_t skipForward)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::SkipForward()\n");
	);

	PostEvent([this, skipForward]{EventSkipForward(skipForward);});

	return (hResult);
}


/*!
 * \brief Event: Skip Forward  
 * 
 * \param skipForward specifies the number of seconds to skip forward. 
 */
void CFilePlayGS::EventSkipForward (
	uint32_t skipForward)
{
	uint32_t skipTo = 0;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::EventSkipForward()\n");
	);

	// Reset the play rate to normal speed.
	m_playRate = ePLAY_NORMAL;

	// Get the skipforward time and find the needed frame.
	skipTo = skipForward + (m_CurrentProgress.un64MediaTimeScale != 0 ? (uint32_t)(m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale) : 0);

	m_pVideoOutput->videoFileSeek(skipTo);


	// If we are stopped and skipping forward start playing again.
	if (StateValidate(IstiMessageViewer::estiPAUSED) &&
		!m_openPaused)
	{
		EventPlay ();
	}
}


/*!
 * \brief Skips to the end of the message
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::SkipEnd ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	PostEvent([this]{EventSkipEnd();});
	  
	return (hResult);
}


/*!
 * \brief Event: Skip End
 * 
 */
void CFilePlayGS::EventSkipEnd ()
{
	// Implement skipping to end.
}


/*!
 * \brief Skips to the specified point in the video.
 * 
 * \param unSkipTo is the point within the video, in seconds, that should be skipped to.
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlayGS::SkipTo (
	unsigned int unSkipTo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int skipTo = 0;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	auto nCurrentSeconds = static_cast<unsigned int>(m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale);
	if (unSkipTo != nCurrentSeconds)
	{
		if (nCurrentSeconds < unSkipTo)
		{
			skipTo = unSkipTo - nCurrentSeconds;
			PostEvent([this, skipTo]{EventSkipForward(skipTo);});
		}
		else
		{
			skipTo = nCurrentSeconds - unSkipTo;
			PostEvent([this, skipTo]{EventSkipBack(skipTo);});
		}
	}

	return hResult;
}


/*!
 * \brief Skips to the countdown portion of the greeting.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
void CFilePlayGS::SkipSignMailGreeting ()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_eState != IstiMessageViewer::estiERROR) 
	{
		if (StateValidate(estiPLAYING | estiPLAY_WHEN_READY | estiPAUSED))
		{
			// If we are playing a greeting then close it.
			PostEvent([this]{EventCloseMovie();});
		}

		// Notify CCI that the greeting is being skipped.
		signMailGreetingSkippedSignal.Emit();
	}
}


/*!
 * \brief Populates a SstiProgress stucture which contains inforamation that 
 *  	  can be used to determine the messages download progress, playing time and position.
 * 
 * \param pProgress is a SstiProgress stucture that is populated the Message Viewer.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlayGS::ProgressGet (
	IstiMessageViewer::SstiProgress* pProgress) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::ProgressGet\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	*pProgress = m_CurrentProgress;

	return hResult;
}


/*!
 * \brief Requests the captured frame data
 *
 *  \return stiRESULT_SUCCESS if success
 */
stiHResult CFilePlayGS::CapturedFrameGet()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{eventCapturedFrameGet();});

	return hResult;
}

void CFilePlayGS::eventCapturedFrameGet()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult); 
	std::vector<uint8_t> frameCapturedData;

	stiTESTCOND(!m_frameCaptureData.empty (), stiRESULT_ERROR);

	frameCapturedData = m_frameCaptureData;

	capturedFrameGetSignal.Emit(frameCapturedData);

STI_BAIL:

    return;
}

#ifdef stiDEBUGGING_TOOLS
static const char *StateNameGet (
	IstiMessageViewer::EState eState) ///< The state identifier
{
	const char *pszState = "Unknown";

	switch (eState)
	{
		case IstiMessageViewer::estiOPENING:
			pszState = "IstiMessageViewer::estiOPENING";
			break;

		case IstiMessageViewer::estiRELOADING:
			pszState = "IstiMessageViewer::estiRELOADING";
			break;

		case IstiMessageViewer::estiPLAY_WHEN_READY:
			pszState = "IstiMessageViewer::estiPLAY_WHEN_READY";
			break;

		case IstiMessageViewer::estiPLAYING:
			pszState = "IstiMessageViewer::estiPLAYING";
			break;

		case IstiMessageViewer::estiPAUSED:
			pszState = "IstiMessageViewer::estiPAUSED";
			break;

		case IstiMessageViewer::estiCLOSING:
			pszState = "IstiMessageViewer::estiCLOSING";
			break;

		case IstiMessageViewer::estiCLOSED:
			pszState = "IstiMessageViewer::estiCLOSED";
			break;

		case IstiMessageViewer::estiERROR:
			pszState = "IstiMessageViewer::estiERROR";
			break;

		case IstiMessageViewer::estiVIDEO_END:
			pszState = "IstiMessageViewer::estiVIDEO_END";
			break;

		case IstiMessageViewer::estiREQUESTING_GUID:
			pszState = "IstiMessageViewer::estiREQUESTING_GUID";
			break;

		case IstiMessageViewer::estiRECORD_CONFIGURE:
			pszState = "IstiMessageViewer::estiRECORD_CONFIGURE";
			break;
	 
		case IstiMessageViewer::estiPLAYER_IDLE:
			pszState = "IstiMessageViewer::estiPLAYER_IDLE";
			break;
			
		case IstiMessageViewer::estiWAITING_TO_RECORD:
			pszState = "IstiMessageViewer::estiWAITING_TO_RECORD";
			break;
			
		case IstiMessageViewer::estiRECORDING:
			pszState = "IstiMessageViewer::estiRECORDING";
			break;
			
		case IstiMessageViewer::estiRECORD_FINISHED:
			pszState = "IstiMessageViewer::estiRECORD_FINISHED";
			break;

		case IstiMessageViewer::estiRECORD_CLOSING:
			pszState = "IstiMessageViewer::estiRECORD_CLOSING";
			break;

		case IstiMessageViewer::estiRECORD_CLOSED:
			pszState = "IstiMessageViewer::estiRECORD_CLOSED";
			break;
			
		case IstiMessageViewer::estiUPLOADING:
			pszState = "IstiMessageViewer::estiUPLOADING";
			break;
			
		case IstiMessageViewer::estiUPLOAD_COMPLETE:
			pszState = "IstiMessageViewer::estiUPLOAD_COMPLETE";
			break;
	}

	return pszState;
}
#endif


/*! \brief Validate the current state against the passed in states.
 *
 * Check the current state to see if it is one of the states that was passed
 * into this function.
 * 
 * \param const uint32_t un32States 
 * 
 * \return bool
 * \retval true If the current state is one of those passed in.
 * \retval false Otherwise.
 * 
 */
bool CFilePlayGS::StateValidate (
	const uint32_t un32States) const //!< The state (or states ORed together) to check
{
	stiLOG_ENTRY_NAME (CFilePlayGS::StateValidate);
	bool bResult = false;

	// Is the current state one of the states that was passed in?
	bResult = (0 != (m_eState & un32States));

	return (bResult);
} // end StateValidate


/*!
 * \brief Set State  
 * 
 * \param IstiMessageViewer::EState eNewState 
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlayGS::StateSet (
	IstiMessageViewer::EState eNewState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::StateSet\n");
		stiTrace("\tCurr State = %s\n", StateNameGet (m_eState));
		stiTrace("\tNew State = %s\n\n", StateNameGet (eNewState));
	);

	if (eNewState != m_eState)
	{
		m_eState = eNewState;

		switch (m_eState)
		{
			case estiCLOSED:
			{
				if (RecordTypeGet() != estiRECORDING_IDLE)
				{
					if (m_loadCountdownVideo)
					{
						LoadCountdownVideo();
					}
					else
					{
						// If we are in the record config incomplete state then call RecordConfig()
						PostEvent([this]{EventRecordConfig();});
					}
				}

				// If loadServer isn't empty then we are waiting to load another video.	
				if (!m_loadServer.empty ())
				{
					PostEvent ([this]
					{
						// Reset loopVideo incase it got cleared while closing.
						m_loopVideo = m_loadLoop;

						eventLoadVideoMessage (m_loadServer,
											   m_loadFileGUID,
											   m_loadViewId,
											   m_loadFileSize,
											   m_loadClientToken,
											   m_loadPause,
											   m_loadDownloadSpeed);
					});
				}
			}
			break;

			case estiPAUSED:
			{
				if (m_loadCountdownVideo)
				{
					LoadCountdownVideo();
				}
			}
			break;

			case IstiMessageViewer::estiWAITING_TO_RECORD:
			{
				RecordStart();
			}
			break;

			case estiRECORD_CLOSED:
			{
				if (RecordTypeGet() != estiRECORDING_IDLE)
				{
					if (m_loadCountdownVideo)
					{
						LoadCountdownVideo();
					}
				}
			}
			break;

			case estiERROR:
			{
				if (m_eError == estiERROR_RECORDING &&
			  		m_spSignMailCall)
				{
					m_spSignMailCall->NextStateSet(esmiCS_DISCONNECTED, estiSUBSTATE_MESSAGE_COMPLETE);
				}
			}
			break;

			case estiRECORD_FINISHED:
			{
				if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
				{
					stiTESTCOND(m_spSignMailCall, stiRESULT_ERROR);
					m_spSignMailCall->NextStateSet(esmiCS_DISCONNECTED, estiSUBSTATE_MESSAGE_COMPLETE);
				}
			}
			break;

			default:
			break;
		}

		stateSetSignal.Emit(m_eState);
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (m_eState == IstiMessageViewer::estiPLAYING ||
			m_eState == IstiMessageViewer::estiRECORD_FINISHED)
		{
			StateSet(IstiMessageViewer::estiERROR);
		}
	}

	return hResult;
}


/*!
 * \brief Returns the current state of the Message Viewer.
 * 
 * \return One of the states defined in the EState enum.
 */
IstiMessageViewer::EState CFilePlayGS::StateGet () const
{
	IstiMessageViewer::EState eState;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eState = m_eState;

	return eState;
}


/*!
 * \brief Returns the current error state of the Message Viewer.
 * 
 * \return One of the errors defined in the EError enum.
 */
IstiMessageViewer::EError CFilePlayGS::ErrorGet () const
{
	IstiMessageViewer::EError eError;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eError = m_eError;

	return eError;
}


/*!
 * \brief Set Error State 
 * Sets the error state.  
 * 
 * \param IstiMessageViewer::EError errorState 
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlayGS::ErrorStateSet (
		IstiMessageViewer::EError errorState)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_eError = errorState;

	if (m_eError != IstiMessageViewer::estiERROR_NONE)
	{
		m_eLastError = m_eError;
	}

	return hResult;
}


/*!
 * \brief Returns the most recent error state of the Message Viewer.
 *
 * \return One of the errors defined in the EError enum.
 */
IstiMessageViewer::EError CFilePlayGS::LastKnownErrorGet () const
{
	IstiMessageViewer::EError eError;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eError = m_eLastError;

	return eError;
}


/*!
 * \brief Stop Video Playback
 * 

 */
stiHResult CFilePlayGS::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::VideoPlaybackStop\n");
	);

	if (m_playbackStarted)
	{
		hResult = m_pVideoOutput->VideoPlaybackStop ();
		stiTESTRESULT ();

		m_playbackStarted = false;

		stiDEBUG_TOOL (g_stiFilePlayDebug,
			stiTrace("\tVideoPlaybackStop OK\n");
		);
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Sets the Recording settings when recording a SignMail message 
 * 
 * \param poEvent 
 *  
 * \return Success or failure result 
 */
stiHResult CFilePlayGS::VideoRecordSettingsSet() 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiH264Capabilities stVideoCapabilities;
	IstiVideoInput::SstiVideoRecordSettings stRecordSettings;

	// Initialize the codec for recording.
	stRecordSettings.unMaxPacketSize = nMAX_VIDEO_RECORD_PACKET;
	stRecordSettings.unTargetBitRate = m_un32CurrentRecordBitrate;
	stRecordSettings.unTargetFrameRate = DEFAULT_FRAME_RATE;
	stRecordSettings.unIntraRefreshRate = 0; // Must be 0.
	stRecordSettings.unIntraFrameRate = m_un32IntraFrameRate;
	stRecordSettings.unRows = m_un32RecordHeight;
	stRecordSettings.unColumns = m_un32RecordWidth;
	stRecordSettings.ePacketization = estiPktNone;
	stRecordSettings.codec = estiVIDEO_H264;
	stRecordSettings.eProfile = estiH264_BASELINE;
	stRecordSettings.unLevel = estiH264_LEVEL_4_1;

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL)
	{
		stRecordSettings.recordFastStartFile = true;
	}

	//Sending Video Record Settings to the videoInput for encoder configuration
	hResult = m_pVideoInput->VideoRecordSettingsSet (&stRecordSettings);
	stiTESTRESULT ();
	
	m_pVideoInput->videoRecordToFile();

STI_BAIL:
	return hResult;
}


/*!
 * \brief Deletes the Greeting that is currently in the FilePlayer's record buffer.
 *
 *
 * \return Success or failure result
 */
stiHResult CFilePlayGS::GreetingDeleteRecorded()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent ([this]{
		if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_NO_URL)
		{
			RecordTypeSet(IstiMessageViewer::estiRECORDING_IDLE);

			recordedMessageCleanup ();
		}
	});

	return hResult;
}


/*!
 * \brief Configures filePlayer so that it can record a greeting and
 * 		  begins the recording process.
 *
 * \param bShowCountdown indicates if the countdown video should be played. 
 *  	  It is defaulted to true.
 *  
 * \return Success or failure result
 */
stiHResult CFilePlayGS::GreetingRecord(bool showCountdown)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent ([this, showCountdown]{
		RecordTypeSet(IstiMessageViewer::estiRECORDING_NO_URL);
		if (showCountdown)
		{
				if (StateValidate(IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiRECORD_CLOSED | IstiMessageViewer::estiPLAYER_IDLE |
								  IstiMessageViewer::estiRECORD_CLOSING | IstiMessageViewer::estiPLAY_WHEN_READY |
								  IstiMessageViewer::estiPLAYING | IstiMessageViewer::estiPAUSED | IstiMessageViewer::estiVIDEO_END))
				{
					LoadCountdownVideo();
				}
		}
		else
		{
			// Just start recording.  No greeting required.
			EventRecordConfig();
		}
	});

	return hResult;
}


/*!
 * \brief Tells filePlayer to upload the recorded greeting.
 *  
 * \return Success or failure result
 */
stiHResult CFilePlayGS::GreetingUpload()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent ([this]{
		StateSet(IstiMessageViewer::estiUPLOADING);

		requestGreetingUploadGUIDSignal.Emit();
		});

	return hResult;
}


/*!
 * \brief Causes the Greeting to be transcoded and then uploaded to
 * 		  supplied server, using the supplied upload GUID.
 * 
 * \return Success or failure result  
 */
stiHResult CFilePlayGS::GreetingUploadInfoSet(
	const std::string &uploadGUID,
	const std::string &uploadServer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlayGS::%s\n", __func__);
	);

	stiTESTCOND(!uploadGUID.empty() && !uploadServer.empty(), stiRESULT_INVALID_PARAMETER);

	// Send an event to upload the GUID
	PostEvent ([this, uploadGUID, uploadServer]{EventGreetingUpload(uploadGUID, uploadServer);});

STI_BAIL:

	return hResult;
}


void CFilePlayGS::fileUploadStart()
{
	IstiFileUploadHandler *fileUpload = IstiFileUploadHandler::InstanceGet();
	IstiFileUploadHandler::EState eFileUploadState = fileUpload->stateGet();
	if (eFileUploadState != IstiFileUploadHandler::eUPLOAD_IDLE)
	{
		// We need an error here
	}

	// Hook up the signals.
	if (m_fileUploadSignalConnections.empty())
	{
		m_fileUploadSignalConnections.push_back(
			fileUpload->fileUploadProgressSignalGet().Connect (
				[this](unsigned int uploadPercent)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);
					m_CurrentProgress.un32BufferingPercentage = uploadPercent;
				}));

		m_fileUploadSignalConnections.push_back(
			fileUpload->fileUploadCompleteSignalGet().Connect (
				[this](std::string  messageId)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);
					m_messageId = messageId;
					PostEvent([this]{eventFileUploadComplete();});
				}));

		m_fileUploadSignalConnections.push_back(
			fileUpload->fileUploadErrorSignalGet().Connect (
				[this]()
				{
					PostEvent([this]{eventFileUploadError();});
				}));
	}

	m_CurrentProgress.un32BufferingPercentage = 0;

	StateSet(IstiMessageViewer::estiUPLOADING);

	fileUpload->start(m_uploadServer, 
					  m_recordFileName, 
					  m_recordFilePath, 
					  m_uploadGUID);
}


void CFilePlayGS::fileUploadFinish()
{
	// Disconnect the signals.
	m_fileUploadSignalConnections.clear();

	// Clear the camera error count;
	m_encodeCameraErrorCount = 0;

	recordedFileDelete();
}


void CFilePlayGS::eventFileUploadComplete()
{
	StateSet(IstiMessageViewer::estiUPLOAD_COMPLETE);

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL ||
		m_eRecordType == IstiMessageViewer::estiRECORDING_REVIEW_UPLOAD_URL)
	{
		if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL)
		{
			// Upload the greeting preview image.
			PostEvent([this]{eventGreetingImageUpload();});
		}
		else
		{
			eventSendRecordedMessage ();
		}

		// The upload is completed and we don't have any data so reset
		// the dataType and recordType so we don't try to rerecord while closing.
		m_unDataType = estiEMPTY_DATA;
		m_eRecordType = IstiMessageViewer::estiRECORDING_IDLE;

		StateSet(IstiMessageViewer::estiRECORD_CLOSED);

		m_pVideoOutput->VideoFileStop();

		recordedMessageCleanup ();
	}
	else
	{
		fileUploadFinish();
	}
}


void CFilePlayGS::recordedFileDelete()
{
	// Clean out the record file directory.
	std::string  command = "rm ";
	command += m_recordFilePath + "*";

	if (system(command.c_str()))
	{
		// Even by doing nothing, this gets rid of a compiler warning.
	}
}

/*!
 * \brief Log File Initialize Error
 * Sets the error state and modifies m_eError to the error type.  
 *  
 * \param IstiMessageViewer::EError errorMsg 
 */
void CFilePlayGS::LogFileInitializeError (
	IstiMessageViewer::EError errorMsg)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	StateSet(IstiMessageViewer::estiERROR);

	m_eError = errorMsg;
}

/*!
 * \brief Logs Message Viewer's state and other info to splunk..
 *
 */
void CFilePlayGS::logStateInfoToSplunk()
{
	std::stringstream infoString;

	infoString << "State: " << StateNameGet(m_eState); 

	if (StateValidate(estiPLAY_WHEN_READY))
	{
		infoString << " buffer %: "  <<  m_CurrentProgress.un32BufferingPercentage;
	}

	if (!StateValidate (estiERROR) && 
		StateValidate(estiPAUSED | estiPLAYING))
	{
		infoString << " bytes still to download: "  << m_CurrentProgress.un64BytesYetToDownload;
		infoString << " time to download: " << m_CurrentProgress.un32TimeToBuffer;
		infoString << " play pos: " << ((double)m_CurrentProgress.un64CurrentMediaTime / (double)m_CurrentProgress.un64MediaTimeScale);
	}

	stiRemoteLogEventSend ("CFilePlay State Info: %s\n", infoString.str().c_str());
}

/*!
 * \brief Sets the SignMail Call.
 *  
 * \param IstiCall call 
 */
void CFilePlayGS::signMailCallSet(std::shared_ptr<IstiCall> call)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_spSignMailCall = std::static_pointer_cast<CstiCall>(call);
}

ISignal<>& CFilePlayGS::requestGUIDSignalGet ()
{
	return requestGUIDSignal;
}

ISignal<>& CFilePlayGS::requestUploadGUIDSignalGet ()
{
	return requestUploadGUIDSignal;
}

ISignal<>& CFilePlayGS::requestGreetingUploadGUIDSignalGet ()
{
	return requestGreetingUploadGUIDSignal;
}

ISignal<>& CFilePlayGS::greetingImageUploadCompleteSignalGet ()
{
	return greetingImageUploadCompleteSignal;
}

ISignal<SstiRecordedMessageInfo &>& CFilePlayGS::deleteRecordedMessageSignalGet ()
{
	return deleteRecordedMessageSignal;
}

ISignal<SstiRecordedMessageInfo &>& CFilePlayGS::sendRecordedMessageSignalGet ()
{
	return sendRecordedMessageSignal;
}

ISignal<>& CFilePlayGS::signMailGreetingSkippedSignalGet ()
{
	return signMailGreetingSkippedSignal;
}

ISignal<std::vector<uint8_t> &>& CFilePlayGS::capturedFrameGetSignalGet ()
{
	return capturedFrameGetSignal;
}

ISignal<IstiMessageViewer::EState>& CFilePlayGS::stateSetSignalGet ()
{
	return stateSetSignal;
}

ISignal<>& CFilePlayGS::disablePlayerControlsSignalGet ()
{
	return disablePlayerControlsSignal;
}
