/*!
*  \file CFilePlay.cpp
*  \brief The Message Viewer Interface
*
*  Class declaration for the Message Viewer Interface Class.  
*	This class is responsible for playing files.
*	These files can be local or in the process of being downloaded so
*	this class must be able to handle asking for a frame and being told
*	that the frame is not here yet.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#include "CFilePlay.h"
#ifndef WIN32
#include <sys/time.h>
#else
#include "stiOSWin32.h"
#include "stiOSWin32Signal.h"
#define __func__ __FUNCTION__
#endif

#include "SorensonErrors.h"
#include "SMCommon.h"
#include "stiTaskInfo.h"
#include "CServerDataHandler.h"
#include "IstiVideoOutput.h"
#include "IstiNetwork.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "stiRemoteLogEvent.h"
#include "stiHTTPServiceTools.h"
#include "CstiHTTPService.h"
#include "CstiIpAddress.h"
#include "MP4Signals.h"
#include <sstream>


//
// Constants
//
static const int nMAX_VIDEO_RECORD_PACKET = 691200; // Largest frame 720 x 480 x 2

static const int nMAX_GUID_REQUEST_RETRIES = 3;

static const int USEC_PER_SECOND = 1000000;

static const int MIN_SECONDS_OF_DATA_BUFFERED = 3;
static const uint32_t un32MAX_GOP_SIZE = 60;

#if APPLICATION != APP_NTOUCH_IOS && APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_NTOUCH_MAC && APPLICATION != APP_NTOUCH_PC
static const int DEFAULT_FRAME_RATE = 30;
static const int DEFAULT_RECORD_ROWS = unstiSIF_ROWS;
static const int DEFAULT_RECORD_COLS = unstiSIF_COLS;
static const int DEFAULT_RECORD_VGA_ROWS = unsti4SIF_ROWS;
static const int DEFAULT_RECORD_VGA_COLS = unsti4SIF_COLS;
static const int DEFAULT_LOW_INTRA_FRAME_RATE = 0;
static const int DEFAULT_INTRA_FRAME_RATE = 45;
#else
#if APPLICATION == APP_NTOUCH_ANDROID
#define DEFAULT_FRAME_RATE 15;
#else
#define DEFAULT_FRAME_RATE 30;
#endif
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_PC
static const int DEFAULT_RECORD_ROWS = unstiCIF_ROWS;
static const int DEFAULT_RECORD_COLS = unstiCIF_COLS;
static const int DEFAULT_RECORD_VGA_ROWS = unstiVGA_ROWS;
static const int DEFAULT_RECORD_VGA_COLS = unstiVGA_COLS;
#else 
static const int DEFAULT_RECORD_ROWS = unstiCIF_ROWS;
static const int DEFAULT_RECORD_COLS = unstiCIF_COLS;
static const int DEFAULT_RECORD_VGA_ROWS = unstiCIF_ROWS;
static const int DEFAULT_RECORD_VGA_COLS = unstiCIF_COLS;
#endif
#if APPLICATION == APP_NTOUCH_MAC
static const int DEFAULT_INTRA_FRAME_RATE = 0;
static const int DEFAULT_LOW_INTRA_FRAME_RATE = 0;
#else
static const int DEFAULT_INTRA_FRAME_RATE = 30;
static const int DEFAULT_LOW_INTRA_FRAME_RATE = 30;
#endif
#endif

static const int BYTE_ALIGNMENT_4 = 4;
#if APPLICATION == APP_NTOUCH_MAC
static const int DEFAULT_RECORD_BITRATE = 1024000;
#else
static const int DEFAULT_RECORD_BITRATE = 768000;
#endif
static const uint32_t un32Brands[] = {MAKEFOURCC('m', 'p', '4', '2'), 0, MAKEFOURCC('i', 's', 'o', 'm')};
static const unsigned int unBrandLength = sizeof (un32Brands) / sizeof (un32Brands[0]);

#define COUNTDOWN_VIDEO_CORE_PATH	"video/countdown.h264"
#define MAX_FILE_CLOSE_WAIT 5

static const int g_nSOCKET_GREETING_IMAGE_UPLOAD_TIMEOUT = 5; ///< Socket timeout value in seconds
static const int g_nREAD_TIMEOUT = 10; ///< Readtime out
static const int g_nHEADER_BUFFER_SIZE = 4096; ///< HTTP header buffer size

static const int g_nVIDEO_RECORD_TIME_SCALE = 600; ///< Timescale used for recording video frames.
static const int g_nVIDEO_DEFAULT_PLAYBACK_SPEED = 33; ///< Default playback speed is 33 milliseconds 29.9 frames/sec
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

//#define WRITE_FILE_STREAM
#ifdef WRITE_FILE_STREAM
FILE *pFile = NULL;
int nFileCount = 0;
#endif

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
CFilePlay::CFilePlay ()
	:
	CstiEventQueue("CFilePlay"),
	m_un32CurrentRecordBitrate (DEFAULT_RECORD_BITRATE),
	m_un32MaxRecordBitrate (DEFAULT_RECORD_BITRATE),
	m_un32RecordWidth (DEFAULT_RECORD_COLS),
	m_un32RecordHeight (DEFAULT_RECORD_ROWS),
	m_un32IntraFrameRate (DEFAULT_INTRA_FRAME_RATE),
    m_videoNextFrameTimer (g_nVIDEO_DEFAULT_PLAYBACK_SPEED, this)
{
	stiLOG_ENTRY_NAME (CFilePlay::CFilePlay)

	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));

	m_videoSignalConnections.push_back(m_videoNextFrameTimer.timeoutSignal.Connect (
		[this]{
			playNextFrame();
		}));

#ifdef TIME_DEBUGGING
	m_LongestWait.tv_sec = 0;
	m_LongestWait.tv_usec = 0;
#endif // TIME_DEBUGGING
}

//
// Destructor
//
/*!
 * \brief Destructor
 */
CFilePlay::~CFilePlay ()
{
	stiLOG_ENTRY_NAME (CFilePlay::~CFilePlay)

	Shutdown ();
	
	if (m_hFrame)
	{
		SMDisposeHandle (m_hFrame);
		m_hFrame = nullptr;
	}

	if (m_hParamSetsAndFrame)
	{
		SMDisposeHandle (m_hParamSetsAndFrame);
		m_hParamSetsAndFrame = nullptr;
	}

	if (m_hSampleCompOffset)
	{
		SMDisposeHandle(m_hSampleCompOffset);
		m_hSampleCompOffset = nullptr;
	}

	if (m_hSampleData)
	{
		SMDisposeHandle(m_hSampleData);
		m_hSampleData = nullptr;
	}

	if (m_hSampleDesc)
	{
		SMDisposeHandle(m_hSampleDesc);
		m_hSampleDesc = nullptr;
	}

	if (m_hSampleDuration)
	{
		SMDisposeHandle(m_hSampleDuration);
		m_hSampleDuration = nullptr;
	}

	if (m_hSampleSize)
	{
		SMDisposeHandle(m_hSampleSize);
		m_hSampleSize = nullptr;
	}

	if (m_hSampleSync)
	{
		SMDisposeHandle(m_hSampleSync);
		m_hSampleSync = nullptr;
	}

	m_videoSignalConnections.clear();
}

stiHResult CFilePlay::Startup ()
{
	// Start the event loop.
	return (CstiEventQueue::StartEventLoop() ?
	           stiRESULT_SUCCESS :
			   stiRESULT_ERROR);
}

/*!
 * \brief Has IFrames 
 * Checks that IFrames occur within a specified number of frames. 
 * 
 * \return bool 
 * Returns true if there are sufficent IFrames.  
 *  
 */
bool CFilePlay::HasIFrames()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	SMResult_t SMResult = SMRES_SUCCESS;
	bool bResult = true;
	uint32_t un32CurrentFrameIndex = un32MAX_GOP_SIZE;
	uint32_t un32IFrameIndex = 0;

	while (un32CurrentFrameIndex < m_un32TotalSampleCount &&
		  bResult)
	{
		SMResult = MP4GetMediaSyncSampleIndex(m_videoMedia,
											  un32CurrentFrameIndex,
											  &un32IFrameIndex);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

		if (un32IFrameIndex < un32CurrentFrameIndex - un32MAX_GOP_SIZE)
		{
			bResult = false;
		}

		un32CurrentFrameIndex = un32IFrameIndex + un32MAX_GOP_SIZE + 1;
	}

STI_BAIL:
	if (SMResult != SMRES_SUCCESS)
	{
		bResult = false;
	}

	return bResult;
}

/*!
 * \brief Initializes the object  
 * 
 * \return stiHResult 
 * stiRESULT_SUCCESS, or on error 
 *  
 */
stiHResult CFilePlay::Initialize ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_hFrame = SMNewHandle (0);

	stiTESTCOND (m_hFrame != nullptr, stiRESULT_ERROR);

	m_hParamSetsAndFrame = SMNewHandle (0);

	stiTESTCOND (m_hParamSetsAndFrame != nullptr, stiRESULT_ERROR);

	//
	// Get instance of the video output device
	//
	m_pVideoOutput = IstiVideoOutput::InstanceGet ();
	stiTESTCOND(m_pVideoOutput != nullptr, stiRESULT_NO_VIDEO_DRIVER);

	//
	// Get instance of the video input device
	//
	m_pVideoInput = IstiVideoInput::InstanceGet ();
	stiTESTCOND(m_pVideoInput != nullptr, stiRESULT_NO_VIDEO_DRIVER);


#ifndef stiDISABLE_SDK_NETWORK_INTERFACE
	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->wiredNetworkDisconnectedSignalGet ().Connect (
		[this]() {
			PostEvent ([this]{eventNetworkDisconnected();});
	}));

	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->wirelessNetworkDisconnectedSignalGet ().Connect (
		[this]() {
			PostEvent ([this]{eventNetworkDisconnected();});
	}));
#endif

STI_BAIL:

	return hResult;
} // end CFilePlay::Initialize ()


/*!
 * \brief This passes frames to video output. 
 * 
 * \return int 
 */
void CFilePlay::playNextFrame ()
{
	stiLOG_ENTRY_NAME (CFilePlay::playNextFrame);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::playNextFrame\n");
	);
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	bool bDeviceReady = true;
	int nFD = -1;
	timeval timevalTimeout = {0, 0};
	fd_set SReadFds;

	stiTESTCOND(StateValidate(IstiMessageViewer::estiPLAYING), stiRESULT_ERROR);

	//
	// If we have a file descriptor from the video output device then make sure
	// that video output is ready to recieve the frame.
	//
	nFD = m_pVideoOutput->GetDeviceFD ();

	if (nFD != -1)
	{
		//
		// Setup the select to monitor the message queue and the readiness of video output
		//
		stiFD_ZERO (&SReadFds);
		stiFD_SET (nFD, &SReadFds);

		stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, nullptr);

		//
		// Set a boolean to indicate the state of the video output.
		//
		bDeviceReady = stiFD_ISSET (nFD, &SReadFds);
	}

	//
	// If video output is ready and we are in a playing state
	// then send the frame to video output.
	//
	if (bDeviceReady)
	{
		FrameSend ();

		// Get next frame for playing
		FrameGet ();

		if (m_skipFrames &&
			m_un32NextFrameIndex >= m_un32SkipToFrameIndex)
		{
			MP4TimeValue sampleTime = 0;
			MP4TimeValue sampleDuration = 0;
			MP4SampleIndexToMediaTime(m_videoMedia, m_un32SkipToFrameIndex,
									  &sampleTime, &sampleDuration);

			m_CurrentProgress.un64CurrentMediaTime = sampleTime;
			m_computeStartTime = true;
			m_skipFrames = false;
			m_unCurrentFrameDuration = 0;
			m_unNextFrameDuration = sampleDuration;
			m_un32SkipToFrameIndex = 1;
		}
	}

	if (m_skipFrames)
	{
		timevalTimeout.tv_sec = 0;
		timevalTimeout.tv_usec = 0;
	}
	else if (StateValidate (IstiMessageViewer::estiPLAYING))
	{
		SetPlayingTime (&timevalTimeout);
	}
	m_videoNextFrameTimer.timeoutSet((int)(timevalTimeout.tv_usec / 1000));
	m_videoNextFrameTimer.start();

STI_BAIL:

	return;
}

stiHResult CFilePlay::Shutdown ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Shutdown);
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::ShutdownHandle\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	if (m_pMovie)
	{
		InitiateClose();
	}

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
void CFilePlay::MaxSpeedSet (
	uint32_t un32MaxDownloadSpeed,
	uint32_t un32MaxUploadSpeed)
{
	m_un32MaxDownloadSpeed = un32MaxDownloadSpeed;
	m_un32MaxUploadSpeed = un32MaxUploadSpeed;
}


/*!
 * \brief Event: Open the movie to be played
 * 
 */
void CFilePlay::EventOpenMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventOpenMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventOpenMovie\n");
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
void CFilePlay::Open (
		const CstiItemId &itemId,
		const char *pszName,
		const char *pszPhoneNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

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

STI_BAIL:

	return;
}

/*!
 * Opens the message identified by the ItemID.
 * 
 * \param pszServer The server of the video to open
 * \param pszFileName The file name of the video to open
 */
void CFilePlay::Open (
	const std::string &server,
	const std::string &fileName,
	bool useDownloadSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	stiTESTCOND (!fileName.empty (), stiRESULT_ERROR);

	m_closedDuringGUIDRequest = false;

	//The ItemId causes a MessageListItemEdit to be sent when the playing state is reached.
	//since we are no longer playing a MessageListItem with an ItemId we should clear it.
	m_pItemId = CstiItemId ();

	m_downloadGuid = fileName;

	m_server.clear ();
	if (!server.empty ())
	{
		m_server = server;
	}

	PostEvent ([this, fileName, useDownloadSpeed]   																
	{
		eventLoadVideoMessage (m_server, 
						      fileName, 
						      "", 0, "", 
						      estiFALSE, 
						      useDownloadSpeed);
	});

STI_BAIL:

	return;
}

/*!
 * \brief Event LoadCountdownVideo
 * 
 */
void CFilePlay::EventLoadCountdownVideo ()
{
	std::string videoFile;
	std::string emptySvr;
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CFilePlay::LoadCountdownVideo);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventLoadCountdownVideo\n");
	);

	stiOSStaticDataFolderGet(&videoFile);
	videoFile += COUNTDOWN_VIDEO_CORE_PATH;

	if (!m_downloadGuid.empty() &&
		m_downloadGuid != videoFile)
	{
		// We are not playing the countdown so try and close it.
		hResult = Close();
		stiTESTRESULT();

		// If the file can be closed then play the countdown.
		m_loadCountdownVideo = true;
	}
	// See if greeting is already playing, if not set it up to start.
	else if (!StateValidate(estiPLAYING | estiPLAY_WHEN_READY))
	{
		if (StateValidate(estiPAUSED))
		{
			m_loadCountdownVideo = estiTRUE;
			PostEvent ([this]{EventCloseMovie();}); 
		}
		else if (!StateValidate(estiRECORD_CLOSED | estiCLOSED | estiPLAYER_IDLE))
		{
			m_loadCountdownVideo = true;
		}
		else
		{
			// If we are recording a SignMail then make sure we have an upload GUID.
			if (RecordTypeGet() == estiRECORDING_UPLOAD_URL &&
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
stiHResult CFilePlay::LoadCountdownVideo(bool waitToPlay)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string staticFolder;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::LoadCountdownVideo\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check to see if we are already playing the countdown.  If we are don't 
	// do anything.
	stiOSStaticDataFolderGet(&staticFolder);

	std::size_t found = m_downloadGuid.find(staticFolder);
	if (m_downloadGuid.empty() ||
		found != std::string::npos)
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
stiHResult CFilePlay::LoadVideoMessage (
	const std::string &server,
	const std::string &fileGUID,
	const std::string &viewId,
	uint64_t fileSize,
	const std::string &clientToken,
	bool pause,	// Set the initial state to Pause (used when an incoming call is detected while initiating fileplay.
	bool useDownloadSpeed)
{
	stiLOG_ENTRY_NAME (CFilePlay::LoadVideoMessage);

	PostEvent ([this, server, fileGUID, viewId, fileSize, clientToken, pause, useDownloadSpeed]
	{
		stiHResult hResult = stiRESULT_SUCCESS; 
		stiTESTCOND(!m_closedDuringGUIDRequest, stiRESULT_ERROR);

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


void CFilePlay::eventLoadVideoMessage (
		const std::string &server,
		const std::string &fileGUID,
		const std::string &viewId,
		uint64_t fileSize,
		const std::string &clientToken,
		bool pause,
		bool useDownloadSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CFilePlay::eventLoadVideoMessage);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace ("CFilePlay::eventLoadVideoMessage\n");
		stiTrace ("\tserver = %s fileGUID = %s StateName = %s\n", server.c_str(), fileGUID.c_str(), StateNameGet (m_eState));
	);

	// Only open the file if we are in a state that is safe to do so.
	if ((StateValidate (IstiMessageViewer::estiPLAYING) && !fileGUID.compare (m_downloadGuid)) ||
		StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiPLAYER_IDLE | 
					   IstiMessageViewer::estiREQUESTING_GUID | IstiMessageViewer::estiRECORD_CLOSED))
	{
		// Clear the load vars so we don't call eventLoadVideoMessage() again.
		m_loadServer.clear ();
		m_loadFileGUID.clear ();
		m_loadFileSize = 0;
		m_loadClientToken.clear ();
		m_loadDownloadSpeed = false;
		m_loadPause = false;
		m_loadViewId.clear ();

		// If we are in the playing state then we are just getting a new GUID
		// because the last one has expired and it needs to be passed down
		// to the data server.
		if (m_pMovie &&
			StateValidate (IstiMessageViewer::estiPLAYING))
		{
			MP4ResetMovieDownloadGUID (fileGUID, m_pMovie);
		}
		else
		{
			StateSet (IstiMessageViewer::estiOPENING);

			//
			// Initialize
			//
			InitializePlayVariables ();

			m_pMovie = nullptr;
			m_videoMedia = nullptr;
			m_unDataType = estiDOWNLOADED_DATA;


			m_openPaused = pause;
			m_downloadFailed = false;

			m_viewId = viewId;

			// Hookup the MP4 signals
			dataTransferSignalsConnect ();

			// Open movie file
			MP4OpenMovieFileAsync (server,
								   fileGUID,
								   fileSize,
								   clientToken,
								   useDownloadSpeed ? m_un32MaxDownloadSpeed : 0,
								   &m_pMovie);
			
			stiTESTCOND (m_pMovie, stiRESULT_ERROR);

			//
			// Get maximum buffering size
			//
			MP4GetMaxBufferingSize (m_pMovie, (uint32_t*)&m_un32MaxBufferingSize);
		}
	}
	else
	{
		// If fileGUID and downloadGuid don't match we are trying to load a new file, if they are the same we are 
		// trying to load a file that is loading so don't do anything.
		if (!fileGUID.compare (m_downloadGuid))
		{
			// Store loading vars and post evetLoadVideoMessage() when current video is CLOSED.
			m_loadServer = server;
			m_loadFileGUID = fileGUID;
			m_loadFileSize = fileSize;
			m_loadClientToken = clientToken;
			m_loadDownloadSpeed = useDownloadSpeed;
			m_loadPause = pause;
			m_loadViewId = viewId;

			PostEvent ([this]{Close ();});
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// If we had an error then notify the UI that there was an error opening the requested file.
		ErrorStateSet (IstiMessageViewer::estiERROR_OPENING);
		StateSet (IstiMessageViewer::estiERROR);
	}
}


/*!
 * \brief Automatically loops the message back the beginning when the end is reached.
 *  
 * \param bool bLoop 
 *  
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::LoopSet(bool loop)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_loopVideo = loop;

	return hResult;
}


void CFilePlay::dataTransferSignalsConnect ()
{   			
	if (m_dataTransferSignalConnections.empty())
	{
		m_dataTransferSignalConnections.push_back(
			mediaSampleReadySignal.Connect(
				[this](const SMediaSampleInfo &sampleInfo){
					PostEvent ([this, sampleInfo]{EventMediaSampleReady(sampleInfo);});
				}));

		m_dataTransferSignalConnections.push_back(
			mediaSampleNotReadySignal.Connect(
				[this](int notReadyState){
					PostEvent ([this, notReadyState]{EventMediaSampleNotReady(notReadyState);});
				}));

		m_dataTransferSignalConnections.push_back(
			errorSignal.Connect(
				[this](stiHResult errorCode){
					switch (errorCode)
					{
						case stiRESULT_SRVR_DATA_HNDLR_BAD_GUID:
						{
							PostEvent ([this]{EventBadGUID();}); 
							break;
						}

						case stiRESULT_SRVR_DATA_HNDLR_EXPIRED_GUID:
						{
							// If the GUID has expired we just need to get a new one.
							// We don't want to interupt playing of the message.
							requestGUIDSignal.Emit();
							break;
						}

						case stiRESULT_SERVER_BUSY:
						{
							PostEvent ([this]{EventStateSet(IstiMessageViewer::estiERROR, estiERROR_SERVER_BUSY);});
							break;
						}

						case stiRESULT_SRVR_DATA_HNDLR_UPLOAD_ERROR:
						{
							PostEvent ([this]{EventRecordError();}); 
							break;
						}

						case stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR:
						{
							PostEvent ([this]{EventDownloadError();}); 
							break;  						   
						}

						default:
						{
							PostEvent ([this]{EventStateSet(IstiMessageViewer::estiERROR, estiERROR_NONE);});
							break;
						}
					}
				}));

		m_dataTransferSignalConnections.push_back(
			movieReadySignal.Connect(
				[this](){
					PostEvent ([this]
					{
						// Set this so that we know movie ready was signaled (splunk data).
						m_movieReadySignaled = true;
						EventMovieReady();
					}); 
				}));

		m_dataTransferSignalConnections.push_back(
			messageSizeSignal.Connect(
				[this](uint64_t messageSize){
					PostEvent ([this, messageSize]{EventMessageSize(static_cast<uint32_t>(messageSize));});
				}));

		m_dataTransferSignalConnections.push_back(
			messageIdSignal.Connect(
				[this](std::string messageId){
					PostEvent ([this, messageId]{EventUploadMessageId(messageId);});
				}));

		m_dataTransferSignalConnections.push_back(
			fileDownloadProgressSignal.Connect(
				[this](const SFileDownloadProgress &downloadProgress){
					PostEvent ([this, downloadProgress]{EventDownloadProgress(downloadProgress);});
				}));

		m_dataTransferSignalConnections.push_back(
			skipDownloadProgressSignal.Connect(
				[this](const SFileDownloadProgress &skipProgress){
					PostEvent ([this, skipProgress]{EventSkipDownloadProgress(skipProgress);});
				}));

		m_dataTransferSignalConnections.push_back(
			bufferUploadSpaceUpdateSignal.Connect(
				[this](const SFileUploadProgress &upProgress){
					PostEvent ([this, upProgress]{EventBufferUploadSpaceUpdate(upProgress);});
				}));

		m_dataTransferSignalConnections.push_back(
			uploadProgressSignal.Connect(
				[this](uint32_t uploadProgress){
					PostEvent ([this, uploadProgress]{EventUploadProgress(uploadProgress);});
				}));

		m_dataTransferSignalConnections.push_back(
			uploadErrorSignal.Connect(
				[this](){
					PostEvent ([this]{EventRecordError();});
				}));

		m_dataTransferSignalConnections.push_back(
			downloadErrorSignal.Connect(
				[this](){
					PostEvent ([this]{EventDownloadError();});
				}));

		m_dataTransferSignalConnections.push_back(
			uploadCompleteSignal.Connect(
				[this](){
					PostEvent ([this]{EventUploadComplete();});
				}));

		m_dataTransferSignalConnections.push_back(
			movieClosedSignal.Connect(
				[this](Movie_t *movie){
					PostEvent ([this, movie]{EventMovieClosed(movie);}); 
				}));
	}
}


void CFilePlay::dataTransferSignalsDisconnect()
{
	// Release all of the connections to the video input signals.
	m_dataTransferSignalConnections.clear();
}


/*!
 * \brief Network disconnected
 * 
 * \return Success or failure result 
 */
void CFilePlay::eventNetworkDisconnected ()
{
	stiLOG_ENTRY_NAME (CFilePlay::NetworkDisconnected);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::eventNetworkDisconnected\n");
	);

	if (StateValidate(IstiMessageViewer::estiRECORDING |
					  IstiMessageViewer::estiUPLOADING |
					  IstiMessageViewer::estiWAITING_TO_RECORD))
	{
		PostEvent ([this]{EventRecordError();}); 
	}
}


/*!
 * \brief Event: Bad GUID
 */
void CFilePlay::EventBadGUID ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventBadGUID);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventBadGUID\n");
	);

	BadGuidReported ();
}


/*!
 * \brief Event: Buffer Upload Space Update 
 * 
 * \param CServerDataHandler::SFileUploadProgress *pstUploadProgressoEvent 
 */
void CFilePlay::EventBufferUploadSpaceUpdate (
	const SFileUploadProgress &uploadProgress)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventBufferUploadSpaceUpdate);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventBufferUploadSpaceUpdate\n");
	);

	uint32_t un32BitsAvailable = 0;
	uint32_t un32DataRate = 0;
	uint32_t un32NewBitrate = 0;

	// Only modify video record settings if uploading a signmail (not a greeting). Fix for 32058.
	if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_UPLOAD_URL) // This is a signmail
	{
		un32BitsAvailable = static_cast<uint32_t>(uploadProgress.m_bufferBytesAvailable * 8);
		un32DataRate = uploadProgress.un32DataRate;
		un32NewBitrate = m_un32CurrentRecordBitrate;

		// If media time scale is 0 then we don't have a valid "current progress" so don't do anything.
		if (m_CurrentProgress.un64MediaTimeScale != 0)
		{
			// Check to see if we have enough space to finish.

			// Add 1 to the timeRemaining to make sure we do not end up short on the seconds
			// because of rounding.
			uint32_t un32TimeRemaining = static_cast<uint32_t>(m_un32RecordTime - (m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale)) + 1;
			if (un32TimeRemaining != 0)
			{
				if (un32DataRate < m_un32CurrentRecordBitrate &&
					(m_un32CurrentRecordBitrate - un32DataRate) * un32TimeRemaining > un32BitsAvailable)
				{
					un32NewBitrate = (un32BitsAvailable / un32TimeRemaining) + un32DataRate;
				}
				else if (un32DataRate > m_un32CurrentRecordBitrate ||
						(m_un32CurrentRecordBitrate - un32DataRate) * un32TimeRemaining < un32BitsAvailable - (un32BitsAvailable / 10))
				{
					un32NewBitrate = (un32BitsAvailable / un32TimeRemaining) + un32DataRate;
					if (un32NewBitrate > m_un32MaxRecordBitrate)
					{
						un32NewBitrate = m_un32MaxRecordBitrate;
					}
				}

				if (un32NewBitrate != m_un32CurrentRecordBitrate)
				{
					m_un32CurrentRecordBitrate = un32NewBitrate;
					VideoRecordSettingsSet();
				}
			}
		}
	}
}


/*!
 * \brief Event: Download Progress
 * 
 * \param CServerDataHandler::SFileDownloadProgress *downloadProgress 
 * 
 * \return Success or failure result 
 */
void CFilePlay::EventDownloadProgress (
	const SFileDownloadProgress &downloadProgress)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventDownloadProgress);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventDownloadProgress\n");
	);

	bool bBufferFull = false;
	int64_t bytesInBuffer = 0;

	bool bDownloadComplete = false;

	// If we get here then we are downloading successfully so clear the dwonloadFailed flag.
	m_downloadFailed = false;

	if (!StateValidate (IstiMessageViewer::estiRELOADING | IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiCLOSED))
	{
		//
		// Get the download progress

		//
		// Update the download progress
		//
		m_CurrentProgress.un64FileSizeInBytes = downloadProgress.un64FileSizeInBytes;
		m_CurrentProgress.un64BytesYetToDownload = downloadProgress.un64BytesYetToDownload;
		m_CurrentProgress.un32DataRate = downloadProgress.un32DataRate;

		bytesInBuffer = downloadProgress.m_bytesInBuffer;
		bBufferFull = downloadProgress.bBufferFull;

		if (bBufferFull)
		{
			m_CurrentProgress.un32BufferingPercentage = 100;
		}

		// Check if download is complete
		if (0 == m_CurrentProgress.un64BytesYetToDownload)
		{
			bDownloadComplete = true;
		}

		//
		// Prevent divide by zero
		//
		if (m_CurrentProgress.un32DataRate == 0)
		{
			m_CurrentProgress.un32DataRate = 1;
		}

		//
		// If we are ready to play then transition to the PLAYING state.
		//
		if (StateValidate (IstiMessageViewer::estiPLAY_WHEN_READY))
		{
			// Start playing when the buffer is full or
			// when the download is complete
			if (bBufferFull || bDownloadComplete)
			{
				stiDEBUG_TOOL (g_stiFilePlayDebug,
					stiTrace("buffer is full or file is downloaded Go to PLAYING state 1\n");
				);

				// If we are playing we need to clear BufferToEnd
				m_bufferToEnd = false;
				m_CurrentProgress.un32BufferingPercentage = 100;
				StateSet (IstiMessageViewer::estiPLAYING);
			}
			else
			{
				// Start playing when playback will not catch up to download.
				int64_t n64TimeToPlay = (m_CurrentProgress.un64MediaDuration - m_CurrentProgress.un64CurrentMediaTime) / m_CurrentProgress.un64MediaTimeScale;

				uint64_t un64TimeToDownload = m_CurrentProgress.un64BytesYetToDownload / m_CurrentProgress.un32DataRate + MIN_SECONDS_OF_DATA_BUFFERED;

				// If mediaDuration is less then mediaTimeScale then the video is less than a second, so just use the FileSize as playback rate. Otherwise
				// if the calculations are done mediaDuration / mediaTimeScale = 0 and will give a divide by 0 error (i.e. FileSizeInBytes / 0)
				int64_t playbackRate = (uint64_t)m_unMediaDuration > m_CurrentProgress.un64MediaTimeScale ?
										  static_cast<int32_t>(m_CurrentProgress.un64FileSizeInBytes / (m_unMediaDuration / m_CurrentProgress.un64MediaTimeScale)) :
										  static_cast<int32_t>(m_CurrentProgress.un64FileSizeInBytes);
				auto  neededDataCalc = n64TimeToPlay * (playbackRate - static_cast<int64_t>(m_CurrentProgress.un32DataRate)) + static_cast<int64_t>(0.5 * playbackRate);

				stiDEBUG_TOOL (g_stiFilePlayDebug,
					stiTrace("\n");
					stiTrace("un64BytesYetToDownload = %llu\n", m_CurrentProgress.un64BytesYetToDownload);
					stiTrace("Datarate = %lu\n", m_CurrentProgress.un32DataRate);
					stiTrace("PlaybackRate = %ld\n", playbackRate);
					stiTrace("TimeToPlay = %llu\n", n64TimeToPlay);
					stiTrace("un64TimeToDownload = %llu\n", un64TimeToDownload);
					stiTrace("BytesInBuffer = %lld\n", bytesInBuffer);
					stiTrace("NeededDataCalc: %lld\n", neededDataCalc);
					stiTrace("BufferToEnd: %s\n", m_bufferToEnd ? "TRUE" : "FALSE");
					stiTrace("\n");
				);

				//
				// if TimeToPlay is longer than TimeToDownload and we have a frame to play
				// then start playing
				//
				if (neededDataCalc <= bytesInBuffer &&
					!m_bufferToEnd)
				{
					stiDEBUG_TOOL (g_stiFilePlayDebug,
						stiTrace("Go to PLAYING state 2\n");
					);

					m_CurrentProgress.un32BufferingPercentage = 100;

					StateSet (IstiMessageViewer::estiPLAYING);
				}
				else
				{
					//
					// Buffering percentage calculation
					//

					// Set Initialize value for bytes yet to download
					if (0 == m_un64BytesYetToDownload)
					{
						m_un64BytesYetToDownload = downloadProgress.un64FileSizeInBytes;
					}

					// Update bytes buffered and bytes yet to download
					m_un64BytesBuffered += (m_un64BytesYetToDownload - m_CurrentProgress.un64BytesYetToDownload);
					m_un64BytesYetToDownload = m_CurrentProgress.un64BytesYetToDownload;

					// Calculate how many data we need buffering before start playing using current data rate.
					uint64_t un64SizeBufferedBeforePlaying = m_CurrentProgress.un32DataRate * (un64TimeToDownload - n64TimeToPlay) + m_un64BytesBuffered;

					stiDEBUG_TOOL (g_stiFilePlayDebug,
						stiTrace("un64SizeBufferedBeforePlaying = %llu\n", un64SizeBufferedBeforePlaying);
						stiTrace("m_un32MaxBufferingSize = %llu\n", m_un32MaxBufferingSize);
						stiTrace("m_un64BytesBuffered = %llu\n", m_un64BytesBuffered);
					);

					if (un64SizeBufferedBeforePlaying > m_un32MaxBufferingSize)
					{
						un64SizeBufferedBeforePlaying = m_un32MaxBufferingSize;
					}

					// Calculate the buffering percentage
					auto  un32BufferingPercentage = (uint32_t)((100 * m_un64BytesBuffered) / un64SizeBufferedBeforePlaying);

					if (un32BufferingPercentage > m_CurrentProgress.un32BufferingPercentage && un32BufferingPercentage < 100)
					{
						m_CurrentProgress.un32BufferingPercentage = un32BufferingPercentage;
					}

					// Calculate the time required to buffer
					m_CurrentProgress.un32TimeToBuffer = (uint32_t)((un64SizeBufferedBeforePlaying - m_un64BytesBuffered) / m_CurrentProgress.un32DataRate);
				}
			}

    		stiDEBUG_TOOL (g_stiFilePlayDebug,
				stiTrace("Buffering Percentage %lu\n", m_CurrentProgress.un32BufferingPercentage);
    		);

		} // end if (StateValidate (IstiMessageViewer::estiPLAY_WHEN_READY))
	}

	// We have finished processing a download progress message so we can
	// now receive another.
	MP4ReadyForDownloadUpdate(m_videoMedia);
}

/*!
 * \brief Event: Skip Download Progress 
 * 
 * \param CServerDataHandler::SFileDownloadProgress *downloadProgress 
 */
void CFilePlay::EventSkipDownloadProgress (
	const SFileDownloadProgress &downloadProgress)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventSkipBackDownloadProgress);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventSkipBackDownloadProgress\n");
	);

	EstiBool bBufferFull = estiFALSE;
	int64_t bytesInBuffer = 0;

	bool bDownloadComplete = false;

	//
	// Get the download progress
	//

	//
	// Update the download progress
	//

	bytesInBuffer = downloadProgress.m_bytesInBuffer;
	bBufferFull = (EstiBool) downloadProgress.bBufferFull;
	m_CurrentProgress.un32BufferingPercentage = (long)(((double)(downloadProgress.un64FileSizeInBytes - downloadProgress.un64BytesYetToDownload))/
												 (double)(downloadProgress.un64FileSizeInBytes)) * 100;
	m_CurrentProgress.un32DataRate = downloadProgress.un32DataRate;

	// Check if download is complete
	if (0 == downloadProgress.un64BytesYetToDownload)
	{
		bDownloadComplete = true;
	}

	//
	// If we are ready to play then transition to the PLAYING state.
	//
	if (StateValidate (IstiMessageViewer::estiPLAY_WHEN_READY))
	{
		int64_t n64TimeToPlay = (m_CurrentProgress.un64MediaDuration - m_CurrentProgress.un64CurrentMediaTime) / m_CurrentProgress.un64MediaTimeScale;
		auto  n32PlaybackRate = static_cast<int32_t>(m_CurrentProgress.un64FileSizeInBytes / (m_unMediaDuration / m_CurrentProgress.un64MediaTimeScale));
		auto  neededDataCalc = n64TimeToPlay * static_cast<int64_t>(n32PlaybackRate - m_CurrentProgress.un32DataRate + 0.5 * n32PlaybackRate);
		auto  bufferPercent = (static_cast<double>(downloadProgress.m_bytesInBuffer) / static_cast<double>(downloadProgress.un64FileSizeInBytes)) * 100.0F;

		// Start playing when the buffer is full, download is complete or enough data
		// has been downloaded so that we will not be starved for data.
		if (bBufferFull ||
			bDownloadComplete ||
			(neededDataCalc <= bytesInBuffer && bufferPercent > 0.0))
		{
			stiDEBUG_TOOL (g_stiFilePlayDebug,
				stiTrace("Go to PLAYING state 1\n");
			);

			if (bBufferFull || bDownloadComplete)
			{
				m_CurrentProgress.un32BufferingPercentage = 100;
			}
			StateSet (IstiMessageViewer::estiPLAYING);
		}
	}
}


/*!
 * \brief Bad Guid Reported
 */
void CFilePlay::BadGuidReported ()
{
	stiLOG_ENTRY_NAME (CFilePlay::BadGuidReported);

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

		InitiateClose ();
	}

} // end CFilePlay::BadGuidReported


/*!
 * \brief Event: Movie Ready 
 * 
 */
void CFilePlay::EventMovieReady ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventMovieReady);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventMovieReady\n");
	);

	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t unTrackCount = 0;
	Track_t *track = nullptr;
	Media_t *media = nullptr;
	MP4TimeScale unMediaTimeScale;
	MP4TimeValue unMediaDuration;
	bool bHasIFrames = false;
	uint32_t un32MediaDura = 0;
	uint32_t width = 0;
	uint32_t height = 0;

    if (StateValidate (IstiMessageViewer::estiOPENING) ||
        (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL &&
         StateValidate (IstiMessageViewer::estiRECORD_FINISHED)))
    {
        stiTESTCOND (m_pMovie != nullptr, stiRESULT_ERROR);

        SMResult = MP4GetTrackCount (m_pMovie, &unTrackCount);
		stiTESTCOND (SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

        //
        // Look for the first video track and any audio track
        //
        for (uint32_t i = 1; i <= unTrackCount; i++)
        {
			uint32_t unHandlerType;
			track = nullptr;
            SMResult = MP4GetTrackByIndex (m_pMovie, i, &track);
			stiTESTCOND (SMResult == SMRES_SUCCESS && track, stiRESULT_ERROR);

			media = nullptr;
			SMResult = MP4GetTrackMedia (track, &media);
			stiTESTCOND (SMResult == SMRES_SUCCESS && media, stiRESULT_ERROR);

            SMResult = MP4GetMediaHandlerType (media, &unHandlerType);
			stiTESTCOND (SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

            if (eMediaTypeVisual == unHandlerType &&
            	!m_videoMedia)
            {
				SMHandleType sampleDescr;
				VisualSampleDescr_t *visualSampleDescr = nullptr;
				m_videoMedia = media;

				// Get the sample description
				sampleDescr = SMHandleType(SMNewHandle (0), SMHandleDeleter); 
				stiTESTCOND (sampleDescr != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);

				SMResult = MP4GetMediaSampleDescription (m_videoMedia, 1, sampleDescr.get());
				stiTESTCOND (SMResult == SMRES_SUCCESS && track, stiRESULT_ERROR);

				visualSampleDescr = (VisualSampleDescr_t*) *sampleDescr;
				width = BigEndian2(visualSampleDescr->usWidth);
				height = BigEndian2(visualSampleDescr->usHeight);
            }
        }

		// Check to make sure we found a videoMedia in the above loop if we don't have one then 
		// there is no need to continue.
		stiTESTCOND (m_videoMedia != nullptr, stiRESULT_ERROR);

        stiDEBUG_TOOL (g_stiFilePlayDebug,
            stiTrace("\tCFilePlay::EventMovieReady: width: %d, height: %d\n", width, height);
        );

        m_largeVideoFrame = width > unstiCIF_COLS || height > unstiCIF_ROWS;

        SMResult = MP4GetMediaTimeScale (m_videoMedia, &unMediaTimeScale);
		stiTESTCOND (SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

        SMResult = MP4GetMediaDuration (m_videoMedia, &unMediaDuration);
		stiTESTCOND (SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

        m_unMediaTimeScale = unMediaTimeScale;
        m_unMediaDuration = unMediaDuration;
        MP4GetMediaSampleCount (m_videoMedia, &m_un32TotalSampleCount);

        // TEMPORARY UNTIL I GET OFFICAL COUNTDOWN VIDEO FROM MARKETING!!!!!
        m_un32TotalSampleCount--;

        // Update the media information
        m_CurrentProgress.un64MediaDuration = m_unMediaDuration;
        m_CurrentProgress.un64MediaTimeScale = m_unMediaTimeScale;

        // Set the playrate to normal.
		m_playRate = ePLAY_NORMAL;

        // Reset the splunk frame so we can track buffering.
        m_un32SplunkRebufferCount = 0;
        m_bufferToEnd = false;

        // Movie is ready to play
        if (m_openPaused)
        {
            StateSet (IstiMessageViewer::estiPAUSED);
        }
        else
        {
            StateSet (IstiMessageViewer::estiPLAY_WHEN_READY);
        }

		// If closeWhenOpened is set we tried to close the file in the OPENING state, so close it now
		if (m_closeWhenOpened)
		{
			m_closeWhenOpened = false;
			PostEvent ([this]{EventCloseMovie();}); 
		}
		else
		{
			bHasIFrames = HasIFrames();
			if (!bHasIFrames)
			{
				// Notify the Player dialog to disable the player contorls.
				disablePlayerControlsSignal.Emit();
			}

			// Because we only have a granularity of a second.  We could have been
			// at the end of the video but still show up as having time left.  So
			// add a second to the time so if we are at the end our check will fail
			// and allow the video to start at the front.
			un32MediaDura = static_cast<uint32_t>(m_unMediaDuration / m_unMediaTimeScale);
			if (m_un32StartPos != 0 &&
				un32MediaDura > (m_un32StartPos + 1) &&
				bHasIFrames)
			{
				// Make sure that the current frame is set to a valid frame.
				m_un32CurrentFrameIndex = 1;

				PostEvent([this]{EventSkipForward(m_un32StartPos);});
			}

			stiDEBUG_TOOL (g_stiFilePlayDebug,
				stiTrace("\tOK! Movie is ready to play\n");
			);
		}
    }

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		StateSet (IstiMessageViewer::estiERROR);
	}
}


/*!
 * \brief Event: Media Sample Ready
 * 
 * \param  *mediaSampleInfo
 */
void CFilePlay::EventMediaSampleReady (
	const SMediaSampleInfo &mediaSampleInfo)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventMediaSampleReady);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventMediaSampleReady\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (!StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiRELOADING))
	{
#ifdef TIME_DEBUGGING
		timeval GetMediaTimeEnd;

		gettimeofday (&GetMediaTimeEnd, NULL);

		//
		// Compute the difference.
		//
		//timeval TimeDifference;

		m_TimeDifference.tv_sec = GetMediaTimeEnd.tv_sec - m_GetMediaTimeStart.tv_sec;
		if (GetMediaTimeEnd.tv_usec >= m_GetMediaTimeStart.tv_usec)
		{
			m_TimeDifference.tv_usec = GetMediaTimeEnd.tv_usec - m_GetMediaTimeStart.tv_usec;
		}
		else
		{
			m_TimeDifference.tv_sec--;
			m_TimeDifference.tv_usec = (USEC_PER_SECOND + GetMediaTimeEnd.tv_usec) - m_GetMediaTimeStart.tv_usec;
		}

		if (m_TimeDifference.tv_sec > m_LongestWait.tv_sec
		|| (m_TimeDifference.tv_sec == m_LongestWait.tv_sec
		&& m_TimeDifference.tv_usec > m_LongestWait.tv_usec))
		{
			m_LongestWait = m_TimeDifference;

			stiTrace ("Longest Wait = %d, %d\n", m_LongestWait.tv_sec, m_LongestWait.tv_usec);
		}

		static int counter = 0;

		counter++;

		if (counter > 30 * 10)
		{
			m_LongestWait.tv_sec = 0;
			m_LongestWait.tv_usec = 0;

			counter = 0;
		}
#endif // TIME_DEBUGGING

		stiDEBUG_TOOL (g_stiFilePlayDebug,
			stiTrace("\tReady to play frame %d\n", mediaSampleInfo.un32SampleIndex);
		);

		m_un32CurrentFrameIndex = mediaSampleInfo.un32SampleIndex;
		m_unNextFrameDuration = mediaSampleInfo.un32SampleDuration;

		hResult = GetMediaSample (mediaSampleInfo, &m_VideoFrame);
		stiTESTRESULT ();
		
		m_haveFrame = true;
	}

STI_BAIL:

	if (hResult != stiRESULT_SUCCESS)
	{
	   ErrorStateSet(IstiMessageViewer::estiERROR_GENERIC);
	   StateSet(IstiMessageViewer::estiERROR);
	}
}


/*!
 * \brief Event:Media Sample Not Ready 
 * 
 * \param samplePos 
 * 
 * \return Success or failure result 
 */
void CFilePlay::EventMediaSampleNotReady (
	int32_t samplePos)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventMediaSampleNotReady);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventMediaSampleNotReady for Frame %d\n", m_un32NextFrameIndex);
	);

	if (m_closeDueToBadGuid)
	{
		StateSet (IstiMessageViewer::estiRELOADING);

		m_un32StartPos = StopPositionGet();

		InitiateClose();
	}
	
	if (!StateValidate (IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiRELOADING))
	{
		// The frame is not ready because data has not
		// been downloaded to buffer yet.
		// Stop play until buffer is full or
		// player cannot catch up the download speed

		m_un64BytesBuffered = 0;
		m_CurrentProgress.un32BufferingPercentage = 0;
		
		m_un32RequestedFrameIndex = 0;

		if (m_CurrentProgress.un64BytesYetToDownload != 0 ||
			samplePos == CServerDataHandler::eSAMPLE_IN_PAST)
		{
			//If the NextFrameIndex is 1 then we are restarting so we don't need
			//to set BufferToEnd.
			if (!m_skipFrames && 
				m_un32NextFrameIndex != 1)
			{
				// Track the number of rebuffers and since we ended up here just
				// fill the buffer.
				m_un32SplunkRebufferCount++;
			}

			// If we are here we don't have a frame, indicate it so we don't try to go to the 
			// play state before we have a frame.
			m_haveFrame = false;
			StateSet (IstiMessageViewer::estiPLAY_WHEN_READY);
		}
		else
		{
			StateSet (IstiMessageViewer::estiPLAYING);
		}
	}
}


/*!
 * \brief Event: Message Size 
 * 
 * \param uint32_t messageSize
 */
void CFilePlay::EventMessageSize (
	uint32_t messageSize)
{
	m_un32RecordSize = messageSize;
}


/*!
 * \brief Event: Movie Closed 
 *  
 */
void CFilePlay::EventCloseMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventCloseMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventCloseMovie\n");
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
		InitiateClose ();
	}

	// If we don't have a movie but we are in the closing state.  We got 
	// stuck in this state and we need to move to the Closed state.
	else if (StateValidate(IstiMessageViewer::estiCLOSING) &&
			 m_pMovie == nullptr)
	{
		PostEvent ([this]{EventMovieClosed(nullptr);}); 
	}
}


/*!
 * \brief Event: Movie Closed 
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CFilePlay::EventMovieClosed (
	Movie_t *movie)
{
	stiLOG_ENTRY_NAME (CFilePlay::EventMovieClosed);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventMovieClosed\n");
	);

	if (movie != m_pMovie)
	{
//		stiTrace ("******************* Movie (%p) but not the current movie (%p) is closed.\n", movie, m_pMovie);
		MP4CleanupMovieFileAsync(movie);
	}
	else
	{
		if (movie)
		{
			MP4CleanupMovieFileAsync(movie);
			m_pMovie = nullptr;
		}

		VideoPlaybackStop ();

		m_videoMedia = nullptr;

		// Set the data as empty.
		m_unDataType = estiEMPTY_DATA;

		// TODO: RHS - Handle error code appropriately

		// TODO - inform the ui the movie has been closed

		if (StateValidate (IstiMessageViewer::estiRELOADING | IstiMessageViewer::estiRECORD_CONFIGURE))
		{
			// The movie has been successully closed.
			// Notify application to replay the video
			if (StateValidate(IstiMessageViewer::estiRELOADING))
			{
				StateSet(IstiMessageViewer::estiREQUESTING_GUID);
				requestGUIDSignal.Emit();
			}
			else
			{
				requestUploadGUIDSignal.Emit();
			}

			// If it wasn't it closed due to a bad guid, reset the guid retry counter
			if (m_closeDueToBadGuid)
			{
				m_closeDueToBadGuid = false;
			}
			else
			{
				m_nGuidRequests = 0;
			} // end if
		}
		else
		{
			m_closeWhenOpened = false;

			// Make sure the next message will not be set to looping.
			m_loopVideo = false;

			m_downloadGuid.clear();
			m_server.clear();

			memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
			if (StateValidate (IstiMessageViewer::estiCLOSING))
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

			// Notify Splunk if we have any buffering pauses.
			if (m_un32SplunkRebufferCount > 0)
			{
				stiRemoteLogEventSend ("EventType=FilePlayer Reason=Rebuffering Count=%d", m_un32SplunkRebufferCount);
			}
		}
	}

	// Cleanup the MP4 Signals.
	dataTransferSignalsDisconnect();

#ifdef WRITE_FILE_STREAM
	if (pFile)
	{
		fclose(pFile);
		pFile = NULL;
	}
#endif
}

/*!
 * \brief Event: Upload Greeting
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CFilePlay::EventGreetingUpload (
	std::string uploadGUID,
	std::string uploadServer) 
{
	stiLOG_ENTRY_NAME (CFilePlay::EventUploadingGreeting);
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventUploadGreeting\n");
	);

	// Store the UploadGUID.
	m_uploadGUID = uploadGUID;

	// Store the UploadServer.
	m_uploadServer = uploadServer;

	StateSet(IstiMessageViewer::estiUPLOADING);

	// Transcode the Greeting.
	MP4OptimizeAndWriteMovieFile(m_pMovie);

	// Upload the Greeting.
	MP4UploadOptimizedMovieFileAsync(m_uploadServer,
									 m_uploadGUID,
									 m_un32RecordWidth,
									 m_un32RecordHeight,
									 m_un32MaxRecordBitrate,
									 static_cast<uint32_t>(m_unMediaDuration / m_unMediaTimeScale),
									 m_pMovie);

	// Upload the Greeting.
	// Set the upload progress to 0.
	m_CurrentProgress.un32BufferingPercentage = 0;
}

void CFilePlay::EventGreetingImageUpload()
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
	uint16_t port = STANDARD_HTTP_PORT;
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
	
	IpAddress = serverIP;

	for (int i = 0; i < 5; i++)
	{
		if (IpAddress.IpAddressTypeGet () == estiTYPE_IPV6)
		{
			uploadSocket = stiOSSocket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		}
		else
		{
			// Open a socket.
			uploadSocket = stiOSSocket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}

		if (proxyAddress.length () > 0 && proxyPort != 0)
		{
			serverIP = proxyAddress;
			port = proxyPort;
		}

		hResult = HttpSocketConnect (uploadSocket, serverIP.c_str(), port, g_nSOCKET_GREETING_IMAGE_UPLOAD_TIMEOUT);
		if (!stiIS_ERROR(hResult))
		{
			break;
		}

		stiOSClose (uploadSocket);
		uploadSocket = stiINVALID_SOCKET;
	}
	stiTESTCOND (!stiIS_ERROR(hResult), stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR);

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

	if (uploadSocket != stiINVALID_SOCKET)
	{
		stiOSClose (uploadSocket);
		uploadSocket = stiINVALID_SOCKET;
	}

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
void CFilePlay::EventUploadComplete ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult); 

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventUploadComplete\n");
	);

	stiTESTCOND(ErrorGet() == estiERROR_NONE, stiRESULT_ERROR);

	StateSet(IstiMessageViewer::estiUPLOAD_COMPLETE);

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL)
	{
		// Upload the greeting preview image.
		PostEvent([this]{EventGreetingImageUpload();});

		// The upload is completed and we don't have any data so reset
		// the dataType and recordType so we don't try to rerecord while closing.
		m_unDataType = estiEMPTY_DATA;
		m_eRecordType = IstiMessageViewer::estiRECORDING_IDLE;
		InitiateClose();

		// Now that we are in a record idle state clean up the recording info.
		PostEvent([this]{EventRecordP2PMessageInfoClear();});
	}

STI_BAIL:
	
	return;
}


/*!
 * \brief Event: Upload Message Id 
 * 
 * \param char *messageId
 */
void CFilePlay::EventUploadMessageId (
	std::string messageId)  
{
	m_messageId = messageId;
}

/*!
 * \brief Event: Upload Progress 
 * 
 * \param uint32_t progress 
 */
void CFilePlay::EventUploadProgress (
	uint32_t progress)  // The event to be handled
{

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventUploadProgress\n");
	);

	m_CurrentProgress.un32BufferingPercentage = progress;

	if (m_CurrentProgress.un32BufferingPercentage == 100 &&
		ErrorGet() == estiERROR_NONE)
	{
		StateSet(IstiMessageViewer::estiUPLOAD_COMPLETE);
	}
}


/*!
 * \brief Set Playing Time
 * 
 * \param timeval* ptimevalTimeout 
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlay::SetPlayingTime (
	timeval *ptimevalTimeout)
{
	stiLOG_ENTRY_NAME (CFilePlay::SetPlayingTime);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::SetPlayingTime\n");
	);


	stiHResult hResult = stiRESULT_SUCCESS;

	// Calculate the medaiTimeScale.
	MP4TimeScale mediaTimeScale = m_unMediaTimeScale;
	if (m_playRate == ePLAY_FAST)
	{
		mediaTimeScale *= 2;
	}
	else if (m_playRate == ePLAY_SLOW)
	{
		mediaTimeScale /= 2;
	}

	//
	// First find out how much time has passed since the last
	// frame was passed to the driver
	//
	struct timeval timevalNow{};
	gettimeofday (&timevalNow, nullptr);

	//
	// Compute our position within the media in seconds and microseconds.
	//
	auto  unMediaTime = static_cast<unsigned int>(m_CurrentProgress.un64CurrentMediaTime + m_unCurrentFrameDuration);

	unsigned int unSeconds = unMediaTime / mediaTimeScale;
	auto  unMicroSeconds = (unsigned int)((double)(unMediaTime % mediaTimeScale) * USEC_PER_SECOND / (double)mediaTimeScale);

	//
	// If we just started playing or if we have resumed from a paused state
	// then recompute the start time of the media in real time.
	//
	if (m_computeStartTime)
	{
		m_timevalStart = timevalNow;

		m_timevalStart.tv_sec -= unSeconds;

		if (unMicroSeconds <= (unsigned)m_timevalStart.tv_usec)
		{
			m_timevalStart.tv_usec -= unMicroSeconds;
		}
		else
		{
			m_timevalStart.tv_sec--;
			m_timevalStart.tv_usec = (USEC_PER_SECOND + m_timevalStart.tv_usec) - unMicroSeconds;
		}

		m_computeStartTime = false;
	}

#ifdef TIME_DEBUGGING
	static timeval TimeVal = {0, 0};

	if (TimeVal.tv_sec != m_timevalStart.tv_sec
	 || TimeVal.tv_usec != m_timevalStart.tv_usec)
	{
		stiTrace("StartTime has changed from %d, %d to %d, %d\n",
		 TimeVal.tv_sec, TimeVal.tv_usec,
		 m_timevalStart.tv_sec, m_timevalStart.tv_usec);

		TimeVal = m_timevalStart;
	}
#endif // TIME_DEBUGGING

	//
	// Convert the current time of the media to that of real time by
	// adding the start time of the movie to the media time.
	//
	unSeconds += m_timevalStart.tv_sec;

	unMicroSeconds += m_timevalStart.tv_usec;

	if (unMicroSeconds > (unsigned)USEC_PER_SECOND)
	{
		unSeconds++;
		unMicroSeconds -= USEC_PER_SECOND;
	}

	//
	// Determine how long to wait until the next frame.  If the time the next frame should start
	// has already passed then indicate that there should be no delay.
	//
	if (unSeconds < (unsigned)timevalNow.tv_sec
	 || (unSeconds == (unsigned)timevalNow.tv_sec
	 && unMicroSeconds <= (unsigned)timevalNow.tv_usec))
	{
		//
		// Next frame should be sent as soon as possible
		//
		ptimevalTimeout->tv_sec = 0;
		ptimevalTimeout->tv_usec = 0;

#ifdef TIME_DEBUGGING
		stiTrace("Needs to catch up: %d, %d: %d, %d\n", unSeconds, unMicroSeconds, m_TimeDifference.tv_sec, m_TimeDifference.tv_usec);
#endif // TIME_DEBUGGING
	}
	else
	{
		ptimevalTimeout->tv_sec = unSeconds - timevalNow.tv_sec;

		if ((unsigned)timevalNow.tv_usec <= unMicroSeconds)
		{
			ptimevalTimeout->tv_usec = unMicroSeconds - timevalNow.tv_usec;
		}
		else
		{
			ptimevalTimeout->tv_sec--;
			ptimevalTimeout->tv_usec = (USEC_PER_SECOND + unMicroSeconds) - timevalNow.tv_usec;
		}
	}

/*
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::FrameGet\n un32mSecsSinceLastFrame = %u, n32mSecsUntilNextFrame = %u\n m_timevalTimeout.tv_sec = %u, m_timevalTimeout.tv_usec = %u\n",
		un32mSecsSinceLastFrame, n32mSecsUntilNextFrame, m_timevalTimeout.tv_sec, m_timevalTimeout.tv_usec);
	);

*/
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("\tptimevalTimeout->tv_sec = %u, ptimevalTimeout->tv_usec = %u\n", ptimevalTimeout->tv_sec, ptimevalTimeout->tv_usec);
	);

	return hResult;
}


/*!
 * \brief Initiate Close 
 *  If the state is RELOADING or RECORD_CONFIGURE don't set the state to CLOSING because ::EventMovieCosed   
 *  will look for these states to handle the closing of the movie differently.                               
 *  
 */
void CFilePlay::InitiateClose ()
{
	stiLOG_ENTRY_NAME (CFilePlay::InitiateClose);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::InitiateClose\n");
	);

	// If the state is RELOADING or RECORD_CONFIGURE don't set the state to CLOSING because ::EventMovieCosed
	// will look for these states to handle the closing of the movie differently. If we are in the RECORD_CLOSED, 
	// CLOSED or PLAYER_IDLE state we are already closed so don't do anything.
	if (!StateValidate (IstiMessageViewer::estiRELOADING | IstiMessageViewer::estiRECORD_CONFIGURE |
						IstiMessageViewer::estiRECORD_CLOSED | IstiMessageViewer::estiCLOSED | IstiMessageViewer::estiPLAYER_IDLE))
	{
		if ((StateValidate(IstiMessageViewer::estiUPLOAD_COMPLETE) && !m_pMovie) ||
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
	if (m_pMovie)
	{
		MP4CloseMovieFileAsync (m_pMovie);
	}
	else if (!StateValidate(IstiMessageViewer::estiRECORD_CLOSED))
	{
		PostEvent ([this]{EventMovieClosed(nullptr);});
	}
}


/*!
 * \brief Initiatize the Variables that are used in playing a video. 
 *  
 */
void CFilePlay::InitializePlayVariables()
{
	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));

	m_un32CurrentSampleDescIndex = 0;

	m_un32NextFrameIndex = 1;

	m_un32RequestedFrameIndex = 0;

	m_un32CurrentFrameIndex = 0;

	m_unCurrentFrameDuration = 0;
	m_unNextFrameDuration = 0;

	m_un64BytesBuffered = 0;

	m_un64BytesYetToDownload = 0;

	m_un32MaxBufferingSize = 0;

	m_playbackStarted = false;

	m_haveFrame = false;

	// Clear the skipping variables.
	m_skipFrames = false;
	m_un32SkipToFrameIndex = 1;
	m_un32SkipIFrameIndex = 0;

	m_movieReadySignaled = false;
}


/*!
 * \brief Closes the message that is currently playing.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if in the IstiMessageViewer::estiOPENING state, the message cannot be
 *  	   closed untill the Message Viewer is out of this state. 
 */
stiHResult CFilePlay::Close ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Close);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::Close\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
	{
		m_eRecordType = IstiMessageViewer::estiRECORDING_IDLE;
        m_loadCountdownVideo = false;
	}

	if (StateValidate(IstiMessageViewer::estiOPENING) ||
		(m_unDataType == estiRECORDED_DATA && 
		 m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL))
	{
		// We want to close the video once the moov atom has been downloaded.
		m_closeWhenOpened = true;

		//stiTrace ("State = %d, m_unDataType = %d, m_eRecordType = %d\n", StateGet (), m_unDataType, m_eRecordType);
		//stiBackTrace ();
		stiTHROW (stiRESULT_ERROR);
	}

	PostEvent ([this]{EventCloseMovie();}); 

STI_BAIL:

	return hResult;
}


/*!
 * \brief Close Movie
 */
void CFilePlay::CloseMovie ()
{
	stiLOG_ENTRY_NAME (CFilePlay::CloseMovie);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::CloseMovie\n");
	);

	if (m_unDataType != estiRECORDED_DATA || 
		m_eRecordType != IstiMessageViewer::estiRECORDING_NO_URL)
	{
		PostEvent ([this]{EventCloseMovie();});
	}
}


/*!
 * \brief Returns the current position of the  message, in seconds
 * 
 * \return Current position in seconds as a uint32_t 
 */
uint32_t  CFilePlay::StopPositionGet()
{
	SMResult_t SMResult = SMRES_SUCCESS; stiUNUSED_ARG (SMResult);
	int seconds = 0;
	MP4TimeValue sampleTime = 0;
	MP4TimeValue sampleDuration = 0;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// If we are in an error state see if it was at the start of the video or
	// during playback.  If it was at playback we don't want to start over so
	// try and find were we were at. (if currentFrameIndex is 1 then we are
	// at the start of the video and probably failed trying to download the video)
	// If the m_pvideoMedia does not exist the video is not loaded yet so just use the
	// current StartPos for the stop position.
	if (m_videoMedia && 
		(!StateValidate(estiERROR | estiPLAY_WHEN_READY) ||
		(StateValidate(estiERROR) && m_un32CurrentFrameIndex > 1)))
	{
		// Frame index should be the current frame index unless we are skipping.
		uint32_t un32FrameIndex = m_un32CurrentFrameIndex;

		// Check to see if we are skipping, or in the process of retreiving the last skipped frame
		// if so reset frameIndex.
		if (m_skipFrames)
		{
			un32FrameIndex = m_un32SkipToFrameIndex;
		}
		// This check ensures that we don't get caught in the window where the NextFrameIndex was requested,
		// but the ServerDataHandler hasn't sent it yet so the CurrentFrameIndex has not been updated. 
		// CurrentFrameIndex is updated in EventMediaSampleReady() could still be at 1.  
		else if (m_un32NextFrameIndex > m_un32CurrentFrameIndex + 1)
		{
			un32FrameIndex = m_un32NextFrameIndex;
		}

		// If frameIndex = 0 then seconds will be 0. If frameIndex is larger than 0 let the MP4 library figure it out.
		if (un32FrameIndex == 0)
		{
			seconds = 0;
		}
		else
		{
			SMResult = MP4SampleIndexToMediaTime(m_videoMedia, un32FrameIndex,
												 &sampleTime, &sampleDuration);
			stiASSERT (SMResult == SMRES_SUCCESS);

			seconds = static_cast<int>(sampleTime / m_unMediaTimeScale);
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
void CFilePlay::SetStartPosition(
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
stiHResult CFilePlay::SlowForward ()
{
	stiLOG_ENTRY_NAME (CFilePlay::SlowForward);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::SlowForward\n");
	);

	PostEvent([this]{EventSlowForward();});

	return hResult;
}


/*!
 * \brief Event: Slow Forward
 * 
 */
void CFilePlay::EventSlowForward () 
{
	stiLOG_ENTRY_NAME (CFilePlay::EventSlowFoward);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventSlowForward\n");
	);

	m_playRate = ePLAY_SLOW;
	m_computeStartTime = true;
	StateSet (IstiMessageViewer::estiPLAYING);
}


/*!
 * \brief Places the Message Viewer into fast forward mode. The message will play 
 * 		  at 2X speed. 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::FastForward ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{EventFastForward();});
	
	return hResult;
}


/*!
 * \brief Event: Slow Forward
 * 
 */
void CFilePlay::EventFastForward ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventFastFoward);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventFastForward\n");
	);

	m_playRate = ePLAY_FAST;
	m_computeStartTime = true;
	StateSet (IstiMessageViewer::estiPLAYING);
}


/*!
 * \brief Plays message.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::Play ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Play);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::Play\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Play should always set the playrate back to normal.

	if (m_playRate == ePLAY_FAST || m_playRate == ePLAY_SLOW)
	{
		m_playRate = ePLAY_NORMAL;
		m_computeStartTime = true;
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
void CFilePlay::EventPlay ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventPlay);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventPlay\n");
	);

	m_openPaused = false;

	//
	// If we are at the end of the video then the next
	// frame should be the first frame.
	//
	if (StateValidate (IstiMessageViewer::estiVIDEO_END))
	{
		m_un32NextFrameIndex = 1;
	}

	StateSet (IstiMessageViewer::estiPLAYING);
}


/*!
 * \brief Pauses the Message Viewer
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::Pause ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Pause);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::Pause\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (StateValidate (IstiMessageViewer::estiOPENING))
	{
		m_openPaused = true;
	}
	else if (StateValidate (IstiMessageViewer::estiPLAYING | IstiMessageViewer::estiPLAY_WHEN_READY))
	{
		PostEvent([this]{EventPause();});
	}

	return hResult;
}


/*!
 * \brief EventPause ( 
 * 
 */
void CFilePlay::EventPause ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventPause);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventPause\n");
	);

	StateSet (IstiMessageViewer::estiPAUSED);
}


/*!
 * \brief Configures the Message Viewer so that it can record a message.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
void CFilePlay::EventRecordConfig ()
{
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	char fileName[256];
	clock_t clockTicks = clock();
	uint32_t frameBufferSize = 0;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// If we are in a closing state we are in the process of shutting down
	// a failed record.  We have to let this finish before we can begin
	// recording again.
	if (!StateValidate(IstiMessageViewer::estiCLOSING | IstiMessageViewer::estiRECORD_CLOSING | 
					   IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiWAITING_TO_RECORD) &&
		m_eRecordType != IstiMessageViewer::estiRECORDING_IDLE)
	{
		if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
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
			sprintf(fileName, "vidMessage_%d.h264", (int)clockTicks);
		}
		else
		{
			ErrorStateSet(IstiMessageViewer::estiERROR_NONE);

			m_un32IntraFrameRate = DEFAULT_LOW_INTRA_FRAME_RATE;

			// Set the starting record bitrate to be the lesser of the upload bitrate or DEFAULT_RECORD_BITRATE.
			m_un32MaxRecordBitrate = m_un32MaxUploadSpeed >= (uint32_t)DEFAULT_RECORD_BITRATE ? DEFAULT_RECORD_BITRATE : m_un32MaxUploadSpeed;

			m_uploadServer.clear();
			m_uploadGUID.clear();

			sprintf(fileName, "vidGreeting_%d.h264", (int)clockTicks);
		}

		// Reset the frame count to 0.
		m_un32CurrentFrameIndex = 0;

		// Initialize the data handles.
		if (m_hSampleCompOffset == nullptr)
		{
			m_hSampleCompOffset = SMNewHandle(sizeof(MP4TimeValue));
			m_hSampleData = SMNewHandle(0);
			m_hSampleDuration = SMNewHandle(sizeof(uint32_t));
			m_hSampleSize = SMNewHandle(sizeof(uint32_t));
			m_hSampleSync = SMNewHandle(sizeof(int));
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

		// Reset the size of the videoframe vectors. Multiply by 3/2 to ensure
		// space for the YUV frame data.
		frameBufferSize = (m_un32RecordWidth * m_un32RecordHeight * 3) / 2;
		m_videoFrameBuffer1.resize(frameBufferSize);
		m_videoFrameData1.buffer = m_videoFrameBuffer1.data();
		m_videoFrameData1.unBufferMaxSize = frameBufferSize;

		m_videoFrameBuffer2.resize(frameBufferSize);
		m_videoFrameData2.buffer = m_videoFrameBuffer2.data();
		m_videoFrameData2.unBufferMaxSize = frameBufferSize;

		// We don't have a frame yet so null out last frame pointer.
		m_pPreviousVideoFrame = nullptr;

		// Hookup the MP4 signals.
		dataTransferSignalsConnect();

		// Initialize the movie upload.
		m_unDataType = estiRECORDED_DATA;
		m_pMovie = nullptr;
		m_videoMedia = nullptr;
		m_pTrack = nullptr;

		SMResult = MP4UploadMovieFileAsync (m_uploadServer, m_uploadGUID, fileName, &m_pMovie);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

		// Add the trak and its mdia atom.
		SMResult = MP4NewMovieTrack(m_pMovie, m_un32RecordWidth, m_un32RecordHeight, 0, &m_pTrack);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

		SMResult = MP4NewTrackMedia(m_pTrack, eMediaTypeVisual, &m_videoMedia);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

		// Set the Brands to create the ftyp atom.
		SMResult = MP4SetMovieBrands(m_pMovie, (uint32_t*)un32Brands, unBrandLength);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_ERROR);

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
}


/*!
 * \brief Used to notify the Message Viewer to stop recording but does not 
 * 		  generate an error.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::RecordingInterrupted ()
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
void CFilePlay::EventRecordError ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiBool bCloseMovie = estiFALSE;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventRecordError\n");
	);

	if (ErrorGet() != estiERROR_RECORDING)
	{
		// Stop Recording.
		VideoInputSignalsDisconnect ();
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

		if (bCloseMovie)
		{
			// Shutdown the file data handling.
			InitiateClose();
		}
	}

STI_BAIL:

	return;
}


stiHResult CFilePlay::GetSPSAndPPS(
	EstiVideoFrameFormat eVideoFormat,
	uint8_t *pun8VideoBuffer,
	uint32_t un32FrameSize,
	uint8_t **ppun8SeqParm,
	uint32_t *pun32SeqParmSize,
	uint8_t **ppun8PictParm,
	uint32_t *pun32PictParmSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint8_t *pun8NALUnit = nullptr;
	uint8_t *pun8NextNALUnit = nullptr;
	uint32_t un32HeaderLength = 0;
	uint32_t un32NextHeaderLength = 0;
	uint32_t un32DataSize = 0;
	uint32_t un32BufIndex = 0;
	uint32_t un32SeqHeaderLength = 0;
	uint32_t un32PictHeaderLength = 0;
	uint32_t un32SeqOffset = 0;
	uint32_t un32PictOffset = 0;
	uint8_t un8NalType = 0;
	uint32_t un32NALUnitSize = 0;
	uint32_t un32NALUnitOffset = 0;

	// Make sure variables are cleard.
	*ppun8SeqParm = nullptr;
	*pun32SeqParmSize = 0;
	*ppun8PictParm = nullptr;
	*pun32PictParmSize = 0;

	if (eVideoFormat == estiBYTE_STREAM)
	{
		// Loop until we find both the SPS and PPS or reach the end of the frame.
		while ((*ppun8SeqParm == nullptr ||
			  *ppun8PictParm == nullptr) &&
			  un32BufIndex < un32FrameSize - 4)
		{
			if (pun8NALUnit == nullptr)
			{
				// Look for the first byte stream header.
				ByteStreamNALUnitFind (pun8VideoBuffer, un32FrameSize,
									   &pun8NALUnit, &un32HeaderLength);
			}
			else
			{
				pun8NALUnit = pun8NextNALUnit;
				un32HeaderLength = un32NextHeaderLength;
			}

			// Calculate the size of the NalUnit.
			ByteStreamNALUnitFind (pun8NALUnit + un32HeaderLength, un32FrameSize - un32HeaderLength,
								   &pun8NextNALUnit, &un32NextHeaderLength);

			// If nextNALUnit is null then we don't have a SPS and/or PPS and we need to error out. 
			stiTESTCOND (pun8NextNALUnit != nullptr, stiRESULT_ERROR);

			// Get the nal type (it will be in the first byte after the header).
			// 7 is a Sequence Paramater Set
			// 8 is a Picture Parameter Set
			un8NalType = (*(pun8NALUnit + un32HeaderLength) & 0x1F);
			if (un8NalType == estiH264PACKET_NAL_SPS)
			{
				*pun32SeqParmSize = pun8NextNALUnit - (pun8NALUnit + un32HeaderLength);
				stiTESTCOND (un32BufIndex + *pun32SeqParmSize + un32HeaderLength < un32FrameSize, stiRESULT_ERROR);

				// Save a pointer to the Sequence Parameter and its header length.
				un32SeqHeaderLength = un32HeaderLength;
				*ppun8SeqParm = pun8NALUnit + un32SeqHeaderLength;
			}
			else if (un8NalType == estiH264PACKET_NAL_PPS)
			{
				*pun32PictParmSize = pun8NextNALUnit - (pun8NALUnit + un32HeaderLength);
				stiTESTCOND (un32BufIndex + *pun32PictParmSize + un32HeaderLength < un32FrameSize, stiRESULT_ERROR);

				// Save a pointer to the Sequence Parameter and its header length.
				un32PictHeaderLength = un32HeaderLength;
				*ppun8PictParm = pun8NALUnit + un32PictHeaderLength;
			}

			//Upate the location in the video frame pointer
			un32BufIndex += pun8NextNALUnit - pun8VideoBuffer;
			pun8VideoBuffer = pun8NextNALUnit;
		}
	}
	else
	{
		// Loop until we find both the SPS and PPS or reach the end of the frame.
		while ((*ppun8SeqParm == nullptr ||
			  *ppun8PictParm == nullptr) &&
			  un32BufIndex < un32FrameSize - 4)
		{
			// Find the nal unit sizes and positions.
			un32DataSize = *(uint32_t *)pun8VideoBuffer;

			// For non byte stream frames the header length is 4 bytes.
			un32HeaderLength = 4;

			// Get the nal type (it will be in the first byte after the header).
			// 7 is a Sequence Paramater Set
			// 8 is a Picture Parameter Set
			un8NalType = (*(pun8VideoBuffer + 4) & 0x1F);

			if (un8NalType == estiH264PACKET_NAL_SPS)
			{
				*pun32SeqParmSize = un32DataSize;
				if (eVideoFormat == estiBIG_ENDIAN_PACKED)
				{
					*pun32SeqParmSize = stiDWORD_BYTE_SWAP(un32DataSize);
				}
				stiTESTCOND (un32BufIndex + *pun32SeqParmSize + un32HeaderLength < un32FrameSize, stiRESULT_ERROR);

				// Move in header length bytes to the begining of the nalUnit
				un32SeqHeaderLength = un32HeaderLength;
				*ppun8SeqParm = pun8VideoBuffer + un32SeqHeaderLength;

				if (eVideoFormat == estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED && 
					*pun32SeqParmSize % 4)
				{
					// Calculate the offset to add.
					un32SeqOffset = 4 - (*pun32SeqParmSize % 4);
				}

				// Upate the location of the video frame pointer
				un32BufIndex += un32SeqHeaderLength + *pun32SeqParmSize + un32SeqOffset;
				pun8VideoBuffer = *ppun8SeqParm + *pun32SeqParmSize + un32SeqOffset;
			}
			else if (un8NalType == estiH264PACKET_NAL_PPS)
			{
				*pun32PictParmSize = un32DataSize;
				if (eVideoFormat == estiBIG_ENDIAN_PACKED)
				{
					*pun32PictParmSize = stiDWORD_BYTE_SWAP(un32DataSize);
				}
				stiTESTCOND (un32BufIndex + *pun32PictParmSize + 4 < un32FrameSize, stiRESULT_ERROR);

				// Move in header length bytes to the begining of the nalUnit
				un32PictHeaderLength = un32HeaderLength;
				*ppun8PictParm = pun8VideoBuffer + un32PictHeaderLength;

				if (eVideoFormat == estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED && 
					*pun32PictParmSize % 4)
				{
					// Calculate the offset to add.
					un32PictOffset = 4 - (*pun32PictParmSize % 4);
				}

				// Upate the location of the video frame pointer
				un32BufIndex += un32PictHeaderLength + *pun32PictParmSize + un32PictOffset;
				pun8VideoBuffer = *ppun8PictParm + *pun32PictParmSize + un32PictOffset;
			}
			// All other NAL units
			else
			{
				// NAL unit size.
				un32NALUnitSize = un32DataSize;
				pun8NALUnit = pun8VideoBuffer + un32HeaderLength;

				// Adjust for offsets
				un32NALUnitOffset = 0;
				if (eVideoFormat == estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED && 
					un32NALUnitSize % 4)
				{
					// Calculate the offset to add.
					un32NALUnitOffset = 4 - (un32NALUnitSize % 4);
				}

				// Upate the location of the video frame pointer (header lengthis always 4)
				un32BufIndex += un32HeaderLength + un32NALUnitSize + un32NALUnitOffset;
				pun8VideoBuffer = pun8NALUnit + un32NALUnitSize + un32NALUnitOffset;
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Event Record Frame 
 * 
 */
void CFilePlay::EventRecordFrame ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint64_t frameDuration = 0;
    SstiRecordVideoFrame *pNewVideoFrame = nullptr; 

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::%s\n", __func__);
	);
	
	// Make sure that we are in the Recording state before we try and push the frame to the buffer.
	if (StateValidate(IstiMessageViewer::estiRECORDING))
	{
		if (!m_pPreviousVideoFrame || 
			m_pPreviousVideoFrame == &m_videoFrameData2)
		{
			pNewVideoFrame = &m_videoFrameData1;
		}
		else
		{
			pNewVideoFrame = &m_videoFrameData2;
		}

		stiTESTCOND (pNewVideoFrame->buffer != nullptr, stiRESULT_ERROR);
		
		hResult =  m_pVideoInput->VideoRecordFrameGet(pNewVideoFrame);
		stiTESTRESULT ();

		// Make sure we have data for 2 frames.  If so add the frame to the video file.
		if (m_pPreviousVideoFrame)
		{
			// If timeStamps are 0 then calculate duration the old way.
			if (m_videoFrameData1.timeStamp == 0 &&
				m_videoFrameData2.timeStamp == 0)
			{
#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC
				frameDuration = GetRecordingFrameRate();
#else
				frameDuration = g_nVIDEO_RECORD_TIME_SCALE / DEFAULT_FRAME_RATE;
#endif
			}
			else
			{
				// frameDuration is newFrame time minus oldFrame time divided by 1,000,000 (to convert from nano seconds
				// to milli seconds) multiplied by the video timescale. 500 is added to account for truncation (no floating point)
				// then divide by 1000 to change to frames per second scale. 
				frameDuration = ((((pNewVideoFrame->timeStamp - m_pPreviousVideoFrame->timeStamp) / NSEC_PER_MSEC) * g_nVIDEO_RECORD_TIME_SCALE + 500) / MSEC_PER_SEC);
			}

			// Add the frame to the file.
			hResult = VideoFrameAdd(m_pPreviousVideoFrame, static_cast<uint32_t>(frameDuration));
			stiTESTRESULT ();

		}

		// Update the previous video frame pointer.
		m_pPreviousVideoFrame = pNewVideoFrame;
	}

STI_BAIL:

	if (StateValidate(IstiMessageViewer::estiRECORDING) &&
		stiIS_ERROR(hResult))
	{
		VideoInputSignalsDisconnect ();
		m_pVideoInput->VideoRecordStop();

		ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
		StateSet(IstiMessageViewer::estiERROR);

		// Report to splunk that we had a record error.
		stiRemoteLogEventSend ("EventType=FilePlayer Reason=RecordingErrorBadFrame");
	}
}

/*!
 * \brief Adds a new frame to the media file.  Will also add audio if audio is enabled.
 *  
 * \param SstiRecordVideoFrame *pVideoFrame
 * \param uint32_t frameDuration
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::VideoFrameAdd(
	SstiRecordVideoFrame *pVideoFrame,
	uint32_t frameDuration)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SMResult_t SMResult = SMRES_SUCCESS;

	uint8_t *pun8SequenceHeader = nullptr;
	uint8_t *pun8InsertPos = nullptr;
	uint16_t un16NalHeaderSizeSwap = 0;
	uint32_t un32TotalHeaderSize = 0;
	uint32_t un32SliceDataSwapped = 0;
	uint8_t un8DataValue = 0;
	uint32_t un32SeqParmSize = 0;
	uint8_t *pun8SeqParm = nullptr;
	uint32_t un32SeqOffset = 0;
	uint32_t un32PictParmSize = 0;
	uint8_t *pun8PictParm = nullptr;
	uint32_t un32PictOffset = 0;
	uint8_t *pun8VideoBuffer = nullptr;
	uint8_t *pun8CurrentPos = nullptr;
	uint16_t un16NalHeaderSize = 0;
	uint32_t un32FrameSize = 0;
	uint32_t un32DataSize = 0;
	uint32_t un32BufIndex = 0;
	uint32_t un32SliceSize = 0;
	uint8_t un8Remainder = 0;
	uint8_t *pun8NALUnit = nullptr;
	uint8_t *pun8NextNALUnit = nullptr;
	uint32_t un32HeaderLength = 0;
	uint32_t un32NextHeaderLength = 0;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::VideoFrameAdd\n");
	);

	// Set up common handles.
	// Is this a keyframe?
	if (pVideoFrame->keyFrame)
	{
		SMSetHandleSize(m_hSampleSync, sizeof (int32_t));
		*(int32_t *)(*m_hSampleSync) = 1;
	}
	else
	{
		SMSetHandleSize(m_hSampleSync, 0);
	}

	// Set frame duration.
	*((uint32_t *)*m_hSampleDuration) = frameDuration; //Should be milliseconds

	// Update the current record time.
	m_CurrentProgress.un64CurrentMediaTime += frameDuration;

	// Set the buffer pointer to point to the beginning of the buffer and get the initial frame size.
	pun8VideoBuffer = pVideoFrame->buffer;
	un32FrameSize = pVideoFrame->frameSize;

	// Initialize the frame data buffer.
	SMSetHandleSize (m_hSampleData, un32FrameSize);
	pun8CurrentPos = (uint8_t *)*m_hSampleData;

	// If this is the first frame we need to setup the Sequence Parameter
	// Set and the Picture Parameter Set.
	if (m_un32CurrentFrameIndex == 0)
	{
		if (m_hSampleDesc)
		{
			SMDisposeHandle(m_hSampleDesc);
			m_hSampleDesc = nullptr;
		}

		GetSPSAndPPS(pVideoFrame->eFormat,
					 pun8VideoBuffer,
					 un32FrameSize,
					 &pun8SeqParm,
					 &un32SeqParmSize,
					 &pun8PictParm,
					 &un32PictParmSize);

		stiTESTCOND (pun8SeqParm != nullptr && pun8PictParm != nullptr, stiRESULT_ERROR);

		// Set the offsets if needed.
		if (pVideoFrame->eFormat == estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED) 
		{
			// Calculate the offset to add.
			un32SeqOffset = un32SeqParmSize % 4 ? 4 - (un32SeqParmSize % 4) : 0;
			un32PictOffset = un32PictParmSize % 4 ? 4 - (un32PictParmSize % 4) : 0;
		}

		// Update the videobuffer pointer and index counter.
		if (pun8SeqParm > pun8PictParm)
		{
			pun8VideoBuffer = pun8SeqParm + un32SeqParmSize + un32SeqOffset;
		}
		else
		{
			pun8VideoBuffer = pun8PictParm + un32PictParmSize + un32PictOffset;
		}
		un32BufIndex = pun8VideoBuffer - pVideoFrame->buffer;

		// Total header size is Param Sizes + 4 bytes for the size and 7 for other header info.
		un32TotalHeaderSize = un32SeqParmSize + un32PictParmSize + 4 + 7;

		pun8SequenceHeader = new uint8_t[un32TotalHeaderSize];
		memset(pun8SequenceHeader, 0, un32TotalHeaderSize);
		pun8InsertPos = pun8SequenceHeader;

		// Add the Sequence Header.
		// Set the configuration version to 1
		//Byte 1
		*pun8InsertPos = 0x01;
		pun8InsertPos++;

		// Profile Indication (Get this from the second byte of the sequence parameter set)
		// Byte 2
		memcpy(pun8InsertPos, pun8SeqParm + 1, 1);
		pun8InsertPos++;

		//Constraint flags '110' and 5 reserved bits '00000'
		// Byte 3
		*pun8InsertPos = 0xC0;
		pun8InsertPos++;

		// Level (Get this from the fourth byte of the sequence parameter set)
		// Byte 4
		memcpy(pun8InsertPos, pun8SeqParm + 3, 1);
		pun8InsertPos++;

		// Reserved bits first 6 = 0x3F last 3 bits are lengthSizeMinusOne '0x03'
		// Byte 5
		*pun8InsertPos = 0xFF;
		pun8InsertPos++;

		// Reserved bits first 3 = 0x7 last 5 contain the number of SequenceParameterSets '1'
		// Byte 6
		*pun8InsertPos = 0xE1;
		pun8InsertPos++;

		// Copy the Sequence parameter size and data to the sequence header buffer.
		un16NalHeaderSizeSwap = BYTESWAP((uint16_t)un32SeqParmSize);
		memcpy(pun8InsertPos, &un16NalHeaderSizeSwap, 2);
		pun8InsertPos += 2;
		memcpy(pun8InsertPos, pun8SeqParm, un32SeqParmSize);
		pun8InsertPos += un32SeqParmSize;

		// Put the number of picture Parameter sets in the buffer.
		*pun8InsertPos = 0x01;
		pun8InsertPos++;

		// Copy the Picture parameter size and data to the sequence header buffer.
		un16NalHeaderSizeSwap = BYTESWAP((uint16_t)un32PictParmSize);
		memcpy(pun8InsertPos, &un16NalHeaderSizeSwap, 2);
		pun8InsertPos += 2;
		memcpy(pun8InsertPos, pun8PictParm, un32PictParmSize);

		// Reset un16NalHeaderSize to the total header size.
		un16NalHeaderSize = (uint16_t)un32TotalHeaderSize;

		SMResult = MP4NewAVCSampleDescription(pun8SequenceHeader,
											un16NalHeaderSize,
											m_un32RecordWidth,
											m_un32RecordHeight,
											&m_hSampleDesc);
	}
	// If this is key frame then we need to remove all of the paramater sets from the begining.
	else if (pVideoFrame->keyFrame)
	{
		GetSPSAndPPS(pVideoFrame->eFormat,
					 pun8VideoBuffer,
					 un32FrameSize,
					 &pun8SeqParm,
					 &un32SeqParmSize,
					 &pun8PictParm,
					 &un32PictParmSize);

		if (pun8SeqParm != nullptr && 
			pun8PictParm != nullptr)
		{
			// Set the offsets if needed.
			if (pVideoFrame->eFormat == estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED) 
			{
				// Calculate the offset to add.
				un32SeqOffset = un32SeqParmSize % 4 ? 4 - (un32SeqParmSize % 4) : 0;
				un32PictOffset = un32PictParmSize % 4 ? 4 - (un32PictParmSize % 4) : 0;
			}

			// Update the videobuffer pointer and index counter.
			if (pun8SeqParm > pun8PictParm)
			{
				pun8VideoBuffer = pun8SeqParm + un32SeqParmSize + un32SeqOffset;
			}
			else
			{
				pun8VideoBuffer = pun8PictParm + un32PictParmSize + un32PictOffset;
			}
			un32BufIndex = pun8VideoBuffer - pVideoFrame->buffer;
		}
	}

	// Remove any extra space that may exist between slices in the frame.
	while (un32BufIndex < un32FrameSize)
	{
		// Add the slice size to the frame.  We have to reorder the bits
		// and write them 8 bits at a time so that it doesn't try to
		// order them on a 32 bit boundry.
		if (pVideoFrame->eFormat == estiBYTE_STREAM) 
		{
			if (pun8NALUnit == nullptr)
			{
				// Look for the first byte stream header.
				ByteStreamNALUnitFind (pun8VideoBuffer, un32FrameSize,
									   &pun8NALUnit, &un32HeaderLength);
			}
			else
			{
				pun8NALUnit = pun8NextNALUnit;
				un32HeaderLength = un32NextHeaderLength;
			}

			// Find the next NAL unit and calculate its size.
			ByteStreamNALUnitFind (pun8NALUnit + un32HeaderLength, 
								   un32FrameSize - un32BufIndex - un32HeaderLength,
								   &pun8NextNALUnit, &un32NextHeaderLength);
			if (pun8NextNALUnit == nullptr)
			{
				un32SliceSize = un32FrameSize - un32BufIndex - un32HeaderLength;
			}
			else
			{
				un32SliceSize = pun8NextNALUnit - (pun8NALUnit + un32HeaderLength);
			}

		}
		else
		{
			un32HeaderLength = 4;
			un32SliceSize = *(uint32_t *)pun8VideoBuffer;
			if (pVideoFrame->eFormat == estiBIG_ENDIAN_PACKED)
			{
				un32SliceSize = stiDWORD_BYTE_SWAP(un32SliceSize);
			}
		}

		un32SliceDataSwapped = BigEndian4(un32SliceSize);
		un8DataValue = (un32SliceDataSwapped & 0x000000FF);
		*pun8CurrentPos = un8DataValue;

		un8DataValue = (un32SliceDataSwapped & 0x0000FF00) >> 8;
		*(pun8CurrentPos + 1) = un8DataValue;

		un8DataValue = (un32SliceDataSwapped & 0x00FF0000) >> 16;
		*(pun8CurrentPos + 2) = un8DataValue;

		un8DataValue = (un32SliceDataSwapped & 0xFF000000) >> 24;
		*(pun8CurrentPos + 3) = un8DataValue;

		// Copy the data into the sampleData handle.  Add 4 for the size position
		memcpy(pun8CurrentPos + 4,
			pun8VideoBuffer + un32HeaderLength,
			un32SliceSize);

		switch (pVideoFrame->eFormat)
		{
			case estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED:
			{
				// Check that the slice ended on a 4 byte boundry.
				un8Remainder = un32SliceSize % BYTE_ALIGNMENT_4;
				un8Remainder = un8Remainder == 0 ? 0 : BYTE_ALIGNMENT_4 - un8Remainder;
				break;
			}
			
			case estiLITTLE_ENDIAN_PACKED:
			case estiBIG_ENDIAN_PACKED:
			case estiBYTE_STREAM:
			{
				un8Remainder = 0;
				break;
			}

			case estiPACKET_INFO_FOUR_BYTE_ALIGNED:
			case estiPACKET_INFO_PACKED:
			case estiRTP_H263_RFC2190_FOUR_BYTE_ALIGNED:
			case estiRTP_H263_RFC2190:
			case estiRTP_H263_RFC2429:
			case estiRTP_H263_MICROSOFT:

				stiTHROW (stiRESULT_ERROR);

				break;
		}
		
		// If the slice does not end on a 4 byte boundy then we need
		// to account for any extra space between the end of this slice and the
		// next slice in the frame.
		un32BufIndex += un32SliceSize + un32HeaderLength + un8Remainder;
		pun8VideoBuffer += un32SliceSize + un32HeaderLength + un8Remainder;

		// Move the temp pointer deeper into the buffer.
		pun8CurrentPos += un32SliceSize + 4;
		un32DataSize += un32SliceSize + 4;

		// If we have 4 bytes or less in the buffer then we don't have anymore data so break.
		if (un32BufIndex >= un32FrameSize - 4)
		{
			break;
		}
	}

	// Set the sample size.
	*((uint32_t *)*m_hSampleSize) = un32DataSize;

	SMResult = MP4AddMediaSamples (m_videoMedia,
								m_hSampleData,
								m_un32CurrentFrameIndex == 0 ? m_hSampleDesc : nullptr,
								1,
								m_hSampleDuration,
								m_hSampleSize,
//								   m_hSampleCompOffset,
								nullptr,
								m_hSampleSync);

	m_un32CurrentFrameIndex++;


STI_BAIL:

	delete []pun8SequenceHeader;
	pun8SequenceHeader = nullptr;

	if (StateValidate(IstiMessageViewer::estiRECORDING) &&
		(SMResult != SMRES_SUCCESS || stiIS_ERROR(hResult)))
	{
		ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
		StateSet(IstiMessageViewer::estiERROR);

		// Report to splunk that we had a record error.
		stiRemoteLogEventSend ("EventType=FilePlayer Reason=RecordingErrorBadFrame");
	}

	return hResult;
}


/*!
 * \brief Connects to signals from VideoInput so we can receive frames
 */
void CFilePlay::VideoInputSignalsConnect ()
{
	if (m_videoInputSignalConnections.empty())
	{
		m_videoInputSignalConnections.push_back(
			m_pVideoInput->packetAvailableSignalGet().Connect (
				[this](){
					PostEvent([this]{EventRecordFrame();});
				}));

		m_videoInputSignalConnections.push_back(
			m_pVideoInput->frameCaptureAvailableSignalGet().Connect (
				[this](const std::vector<uint8_t> &frame){
					m_frameCaptureData = frame;
				}));
	}
}


/*!
 * \brief Disconnects from VideoInput signals so we no longer get frames
 */
void CFilePlay::VideoInputSignalsDisconnect ()
{
	// Release all of the connections to the video input signals.
	m_videoInputSignalConnections.clear();
}


/*!
 * \brief Event: Record Start 
 * 
 */
void CFilePlay::EventRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND(StateValidate(IstiMessageViewer::estiWAITING_TO_RECORD), 
				stiRESULT_ERROR);

#if APPLICATION == APP_NTOUCH_VP2
	stiTESTCOND((IstiNetwork::InstanceGet ()->ServiceStateGet () == estiServiceOnline) ||
				(IstiNetwork::InstanceGet ()->ServiceStateGet () == estiServiceReady), 
				stiRESULT_ERROR);
#endif

	// Reset the video time.
	memset (&m_CurrentProgress, 0, sizeof (m_CurrentProgress));
	m_CurrentProgress.un64MediaTimeScale = g_nVIDEO_RECORD_TIME_SCALE;

	//
	// Register a callback with the video input object so that we can start
	// receiving frames.
	//
	VideoInputSignalsConnect ();
	
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	if (RecordTypeGet() == estiRECORDING_NO_URL)
	{
		hResult = m_pVideoInput->FrameCaptureRequest(IstiVideoInput::eCaptureOnRecordStart, nullptr);
	}
#endif

	// Start grabing frames from the camera.
	hResult =  m_pVideoInput->VideoRecordStart();
	stiTESTRESULT ();

	//
	// Note that we are now receiving video so, in case we shutdown prematurely, we can stop the flow of video.
	//
	m_bReceivingVideo = true;
#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC
	//----Initialize to Zero----
	m_timevalStart.tv_sec = 0;
	m_timevalStart.tv_usec = 0;
	//--------------------------
#endif
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
void CFilePlay::EventRecordStop ()
{
	MP4TimeValue mediaDuration = 0;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventRecordStop\n");
	);

	if (StateValidate(IstiMessageViewer::estiRECORDING))
	{
		VideoInputSignalsDisconnect ();
		m_pVideoInput->VideoRecordStop();

		m_bReceivingVideo = false;
		
		StateSet(IstiMessageViewer::estiRECORD_FINISHED);
	
		// Calculate the record time of the video.
		m_un32RecordLength = static_cast<uint32_t>(m_CurrentProgress.un64CurrentMediaTime / m_CurrentProgress.un64MediaTimeScale);
	
		// Add the media duration to the atom.
		MP4UpdateMediaDuration(m_videoMedia);
		MP4GetMediaDuration(m_videoMedia, &mediaDuration);
		MP4InsertMediaIntoTrack(m_pTrack, 0, 0, mediaDuration, 1);

		// Update the time scale.
		MP4SetMediaTimeScale(m_videoMedia, static_cast<uint32_t>(m_CurrentProgress.un64MediaTimeScale));

		if (m_eRecordType == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
		{
			// Write out the ftyp Atom to the buffer.
			MP4MovieWriteFtypAtom(m_pMovie);
		
			// Write the Moov Atom to the buffer.
			MP4MovieWriteMoovAtom(m_pMovie);
		
			// Set the upload progress to 0.
			m_CurrentProgress.un32BufferingPercentage = 0;
		
			// If our record time is less than 0 we probably haven't uploaded
			// any data so just shut down the record process and don't try
			// to send the data.
			if (m_un32RecordLength > 0)
			{
				ErrorStateSet(IstiMessageViewer::estiERROR_NONE);
				StateSet(IstiMessageViewer::estiUPLOADING);
			}
			else
			{
				ErrorStateSet(IstiMessageViewer::estiERROR_NO_DATA_UPLOADED);
				StateSet(IstiMessageViewer::estiERROR);
			}

			InitiateClose();
		}
		else if (m_eRecordType == IstiMessageViewer::estiRECORDING_NO_URL)
		{
			uint32_t  un32SampleCount = 0;
			MP4GetMediaSampleCount (m_videoMedia, &un32SampleCount);

			if (m_un32RecordLength > 0)
			{
				InitializePlayVariables();

				// Prep the recorded video for viewing.
				PostEvent([this]{EventMovieReady();});
			}
			else
			{
				ErrorStateSet(IstiMessageViewer::estiERROR_RECORDING);
				StateSet(IstiMessageViewer::estiERROR);
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
stiHResult CFilePlay::RecordMessageIdClear()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_messageId.clear();

	return hResult;
}


/*!
 * \brief Calls RecordP2PMessageInfoClear.
 * 
 */
void CFilePlay::EventRecordP2PMessageInfoClear ()
{
	RecordP2PMessageInfoClear();
}


void CFilePlay::SignMailInfoClear ()
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
stiHResult CFilePlay::RecordP2PMessageInfoClear ()
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
void CFilePlay::RecordP2PMessageInfoSet (
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
			RecordTypeSet(estiRECORDING_UPLOAD_URL);

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
stiHResult CFilePlay::RecordStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::RecordStart()\n");
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
stiHResult CFilePlay::RecordStop(bool bRecordError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::RecordStop()\n");
	);

	if (bRecordError)
	{
		PostEvent([this]{EventRecordError();});
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
IstiMessageViewer::ERecordType CFilePlay::RecordTypeGet()
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
stiHResult CFilePlay::RecordTypeSet(IstiMessageViewer::ERecordType eRecordType)
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
stiHResult CFilePlay::DeleteRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::DeleteRecordedMessage()\n");
	);

	PostEvent([this]{EventDeleteRecordedMessage();});

	return (hResult);
}


/*!
 * \brief Deletes the currently recorded SignMail message
 * 
 */
void CFilePlay::EventDeleteRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SstiRecordedMessageInfo recordedMsgInfo;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	stiTESTCONDMSG(!m_messageId.empty(), stiRESULT_ERROR, "Can't delete message, missing Message ID");

	recordedMsgInfo.messageId = m_messageId;

	// Null out the messageId.
	m_messageId.clear();

	deleteRecordedMessageSignal.Emit(recordedMsgInfo);

STI_BAIL:

	// If we had an error we just need to clean up.  We can't tell core to delete the
	// SignMail (core will do it on its own later).
	StateSet (IstiMessageViewer::estiRECORD_CLOSED);

	RecordP2PMessageInfoClear();
}


/*!
 * \brief Sends the currently recorded SignMail message
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::SendRecordedMessage ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiRecordedMessageInfo recordedMsgInfo;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

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

	return (hResult);
}


/*!
 * \brief Plays the message from the begining.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::Restart ()
{
	stiLOG_ENTRY_NAME (CFilePlay::Restart);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::Restart\n");
	);

	PostEvent([this]{EventRestart();});

	return (hResult);
}


/*!
 * \brief Event Restart
 * 
 */
void CFilePlay::EventRestart ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventRestart);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventRestart\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);

	// Do we have a valid media?
	stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

	m_unCurrentFrameDuration = 0;
	m_unNextFrameDuration = 0;
	m_un32NextFrameIndex = 1;

	StateSet (IstiMessageViewer::estiPLAYING);

STI_BAIL:

	return;
}


/*!
 * \brief Event Download Error
 * 
 */
void CFilePlay::EventDownloadError ()
{
	stiLOG_ENTRY_NAME (CFilePlay::EventDownloadError);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventDownloadError\n");
	);

	// If we have a fileSize and have downloaded the complete file then the file is corrupt and
	// we don't need to try and download it again.
	if (!m_downloadFailed &&
		(m_CurrentProgress.un64FileSizeInBytes > 0 &&
		 m_CurrentProgress.un64BytesYetToDownload != 0))
	{
		// Retry the download 1 time. If it fails again then error out.
		m_downloadFailed = true;
		MP4MovieDownloadResume(m_pMovie);
	}
	else
	{
		if (StateValidate (IstiMessageViewer::estiOPENING))
		{
			ErrorStateSet(estiERROR_OPENING);
		}
		StateSet(IstiMessageViewer::estiERROR);
	}
}


/*!
 * \brief Event: State Set
 * 
 * \param IstiMessageViewer::EState state 
 *        IstiMessageViewer::EError error
 *  
 * \return Success or failure result 
 */
void CFilePlay::EventStateSet (
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
stiHResult CFilePlay::SignMailRerecord()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::SignMailRerecord()\n");
	);

	if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
	{
		if (StateValidate(IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiRECORD_CONFIGURE | IstiMessageViewer::estiWAITING_TO_RECORD))
		{
			RecordStop();
		}

		RecordMessageIdClear();

		// Reset the call substate so we can leave a message.
		stiTESTCOND(m_spSignMailCall, stiRESULT_ERROR);
	   	m_spSignMailCall->NextStateSet(esmiCS_DISCONNECTED, estiSUBSTATE_LEAVE_MESSAGE);

		// Setting this tells FilePlayer to start the record process when the file player enters the RECORD_CLOSED state.
		LoadCountdownVideo();

		CloseMovie();
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
stiHResult CFilePlay::SkipBack (
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
void CFilePlay::EventSkipBack (
	unsigned int skipBack)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SMResult_t SMResult = SMRES_SUCCESS;
	MP4TimeValue sampleTime = 0;
	MP4TimeValue sampleDuration = 0;
	MP4TimeValue skipBackTime = 0;

	// Do we have a valid media?
	stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

	// Reset to normal speed.
	if (m_playRate != ePLAY_NORMAL)
	{
		m_playRate = ePLAY_NORMAL;
		m_computeStartTime = true;
	}

	// Get the skipback time and find the needed frame.
	skipBackTime = skipBack * m_unMediaTimeScale;

	// If SkipToFrameIndex != 1 then we are in the process of skipping,
	// so figure out where we want to go bassed on that frame.

	// If both SkipToFrameIndex and SkipIFrameIndex are both 1 then we
	// are already skipping back to the first frame.  So don't try to skipback
	// again.
	if (m_un32SkipToFrameIndex == 1 &&
		m_un32SkipIFrameIndex == 1)
	{
		goto STI_BAIL;
	}

	SMResult = MP4SampleIndexToMediaTime(m_videoMedia, m_un32SkipToFrameIndex != 1 ? m_un32SkipToFrameIndex : m_un32NextFrameIndex,
										 &sampleTime, &sampleDuration);
	stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

	SMResult = MP4MediaTimeToSampleIndex(m_videoMedia,
										 sampleTime < skipBackTime ? 0 : sampleTime - skipBackTime,
										 (uint32_t*)&m_un32SkipToFrameIndex);
	stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

	SMResult = MP4GetMediaSyncSampleIndex(m_videoMedia,
										  m_un32SkipToFrameIndex,
										  &m_un32SkipIFrameIndex);
	stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

	// If the SkipToFrame is equal to the SkipIFrameIndex then we don't need to skip
	// frames because our next frame is an I frame.
	if (m_un32SkipToFrameIndex > m_un32SkipIFrameIndex)
	{
		m_skipFrames = true;
	}
	else
	{
		m_un32SkipIFrameIndex = 0;
		m_un32NextFrameIndex = m_un32SkipToFrameIndex;
		m_CurrentProgress.un64CurrentMediaTime = sampleTime < skipBackTime ? 0 : sampleTime - skipBackTime;
	}

	// If we are stopped and skipping back start playing again.
	if (StateValidate(IstiMessageViewer::estiVIDEO_END))
	{
		m_unCurrentFrameDuration = 0;
		m_unNextFrameDuration = 0;
		m_un32NextFrameIndex = m_un32SkipToFrameIndex;
		m_CurrentProgress.un64CurrentMediaTime = sampleTime < skipBackTime ? 0 : sampleTime - skipBackTime;
	}
	m_un32CurrentSampleDescIndex = 0;
	
	if (StateValidate(IstiMessageViewer::estiPAUSED | IstiMessageViewer::estiVIDEO_END))
	{
		StateSet (IstiMessageViewer::estiPLAYING);
	}

STI_BAIL:

	return;
}


/*!
 * \brief Skips forward into the message, the number of seconds indicated by unSkipForward.
 * 
 * \param unSkipForward specifies teh number of seconds to skip forward.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::SkipForward (
	uint32_t skipForward)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::SkipForward()\n");
	);

	PostEvent([this, skipForward]{EventSkipForward(skipForward);});

	return (hResult);
}


/*!
 * \brief Event: Skip Forward  
 * 
 * \param skipForward specifies the number of seconds to skip forward. 
 */
void CFilePlay::EventSkipForward (
	uint32_t skipForward)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SMResult_t SMResult = SMRES_SUCCESS;
	MP4TimeValue sampleTime = 0;
	MP4TimeValue sampleDuration = 0;
	MP4TimeValue skipForwardTime = 0;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::EventSkipForward()\n");
	);

	// Do we have a valid media?
	stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

	// Reset to normal speed.
	if (m_playRate != ePLAY_NORMAL)
	{
		m_playRate = ePLAY_NORMAL;
		m_computeStartTime = true;
	}

	// Get the skipforward time and find the needed frame.
	skipForwardTime = skipForward * m_unMediaTimeScale;

	// If our next frame is less then the total sample count then we can skip forward.
	if (m_un32NextFrameIndex < m_un32TotalSampleCount)
	{
		SMResult = MP4SampleIndexToMediaTime(m_videoMedia, m_un32NextFrameIndex,
											 &sampleTime, &sampleDuration);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

		if (sampleTime + skipForwardTime < m_unMediaDuration)
		{
			SMResult = MP4MediaTimeToSampleIndex(m_videoMedia,
												 sampleTime + skipForwardTime,
												 (uint32_t*)&m_un32SkipToFrameIndex);
			stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);
		}
		else
		{
			m_un32SkipToFrameIndex = m_un32TotalSampleCount - 1;
		}

		// If the currentFrame is the same as the total sample count then
		// we are at the end of the video so don't try to skip.
		if (m_un32NextFrameIndex != m_un32TotalSampleCount)
		{
			// Get the last IFrame index before the point we are skipping to.
			SMResult = MP4GetMediaSyncSampleIndex(m_videoMedia,
												  m_un32SkipToFrameIndex,
												  &m_un32SkipIFrameIndex);
			stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

			// Get the new currentMediaTime.
            m_CurrentProgress.un64CurrentMediaTime = (sampleTime + skipForwardTime);

			if (m_un32SkipToFrameIndex >= m_un32SkipIFrameIndex)
			{
				m_skipFrames = true;
			}
			m_un32CurrentSampleDescIndex = 0;
			FrameGet();
		}

		// If we are stopped and skipping forward start playing again.
		if (StateValidate(IstiMessageViewer::estiPAUSED) &&
			!m_openPaused)
		{
			StateSet (IstiMessageViewer::estiPLAYING);
		}
	}

STI_BAIL:

	return;
}


/*!
 * \brief Skips to the end of the message
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CFilePlay::SkipEnd ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	PostEvent([this]{EventSkipEnd();});
	  
	return (hResult);
}


/*!
 * \brief Event: Skip End
 * 
 */
void CFilePlay::EventSkipEnd ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	SMResult_t SMResult = SMRES_SUCCESS;

	// Do we have a valid media?
	stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

	m_un32SkipToFrameIndex = m_un32TotalSampleCount;

	// If the currentFrame is the same as the total sample count then
	// we are at the end of the video so don't try to skip.
	if (m_un32CurrentFrameIndex != m_un32TotalSampleCount)
	{
		// Get the last IFrame index before the point we are skipping to.
		SMResult = MP4GetMediaSyncSampleIndex(m_videoMedia,
											  m_un32SkipToFrameIndex,
											  &m_un32SkipIFrameIndex);
		stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);
		m_skipFrames = true;
	}

	// If we are stopped and skipping forward start playing again.
	if (StateValidate(IstiMessageViewer::estiPAUSED))
	{
		StateSet (IstiMessageViewer::estiPLAYING);
	}

STI_BAIL:

	return;
}


/*!
 * \brief Skips to x seconds from the end.
 * 
 * \param unSecondsFromEnd is the point, in seconds, from the end that should be skipped to. 
 *  
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_INVALID_MEDIA if media is not playing.
 *  	   stiRESULT_MEMORY_ALLOC_ERROR if memory could not be allocated.
 */
stiHResult CFilePlay::SkipFromEnd (
	unsigned int unSecondsFromEnd)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SMResult_t SMResult = SMRES_SUCCESS;

	uint32_t skipForward = 0;
	MP4TimeValue sampleTime = 0;
	MP4TimeValue sampleDuration = 0;
	MP4TimeValue timeFromEnd = 0;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	// Do we have a valid media?
	stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

	// Find were where are in the time line.
	SMResult = MP4SampleIndexToMediaTime(m_videoMedia, m_un32NextFrameIndex,
										 &sampleTime, &sampleDuration);
	stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

	// Convert seconds to media time and see if we need to skip.
	timeFromEnd = unSecondsFromEnd * m_unMediaTimeScale;

	if (m_unMediaDuration - sampleTime - timeFromEnd > 0)
	{
		// Convert the skip time back to seconds (EventSkipForward() expects the skip time to be in seconds).
		skipForward = static_cast<uint32_t>((m_unMediaDuration - sampleTime - timeFromEnd) / m_unMediaTimeScale);
		PostEvent([this, skipForward]{EventSkipForward(skipForward);});

	}
	
STI_BAIL:
	
	return (hResult);
}


/*!
 * \brief Skips to the specified point in the video.
 * 
 * \param unSkipTo is the point within the video, in seconds, that should be skipped to.
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlay::SkipTo (
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
void CFilePlay::SkipSignMailGreeting ()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_eState != IstiMessageViewer::estiERROR) 
	{
		if (StateValidate(estiPLAYING | estiPLAY_WHEN_READY | estiPAUSED | estiOPENING))
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
stiHResult CFilePlay::ProgressGet (
	IstiMessageViewer::SstiProgress* pProgress) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::ProgressGet\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	*pProgress = m_CurrentProgress;

	return hResult;
}


/*!
 * \brief Get Frame
 * 
 * \return Success or failure result  
 */
stiHResult CFilePlay::FrameGet ()
{
	stiLOG_ENTRY_NAME (CFilePlay::FrameGet);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::FrameGet\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (!m_haveFrame)
	{
		stiTESTCOND(m_videoMedia != nullptr, stiRESULT_INVALID_MEDIA);

		stiDEBUG_TOOL (g_stiFilePlayDebug,
			stiTrace("CFilePlay::FrameGet - MP4GetMediaSampleAsync %d\n", m_un32NextFrameIndex);
		);

#ifdef TIME_DEBUGGING
		gettimeofday (&m_GetMediaTimeStart, NULL);
#endif // TIME_DEBUGGING

		// Ask for a media sample
		if (m_skipFrames && m_un32SkipIFrameIndex != 0)
		{
			// If we reset the next frame index to be the same as the requested frame index,
			// we have already reqeuseted the frame so we need to move to the next frame.
			m_un32NextFrameIndex = m_un32RequestedFrameIndex == m_un32SkipIFrameIndex ? m_un32SkipIFrameIndex + 1 : m_un32SkipIFrameIndex;

			m_un32SkipIFrameIndex = 0;
		}

		if (m_un32RequestedFrameIndex != m_un32NextFrameIndex)
		{
			MP4GetMediaSampleAsync (m_videoMedia, m_un32NextFrameIndex, m_hFrame);
			m_un32RequestedFrameIndex = m_un32NextFrameIndex;
		}

		if (m_un32NextFrameIndex == 1)
		{
			m_computeStartTime = true;
			m_CurrentProgress.un64CurrentMediaTime = 0;
		}
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Frame Send
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlay::FrameSend ()
{
	stiLOG_ENTRY_NAME (CFilePlay::FrameSend);

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::FrameSend\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// If we have a frame ready, send it to the driver
	//
	if (m_haveFrame)
	{
		stiDEBUG_TOOL (g_stiFilePlayDebug,
			stiTrace("CFilePlay::FrameSend: Calling m_pVideoOutput->VideoPlaybackFrameGet\n");
		);

		IstiVideoPlaybackFrame *pNextVidFrame = nullptr;
		stiHResult hResult = m_pVideoOutput->VideoPlaybackFrameGet(&pNextVidFrame);
		stiTESTRESULT ();

		stiTESTCOND (pNextVidFrame, stiRESULT_ERROR);

		if (pNextVidFrame->BufferSizeGet() < m_VideoFrame.frameSize)
		{
			pNextVidFrame->BufferSizeSet(m_VideoFrame.frameSize);

			//
			// Throw an error if the buffer size is still too small.
			//
			if (pNextVidFrame->BufferSizeGet() < m_VideoFrame.frameSize)
			{
				// Discard the Frame buffer so that we don't lose buffers.
				m_pVideoOutput->VideoPlaybackFrameDiscard(pNextVidFrame);

				stiTHROW (stiRESULT_ERROR);
			}
		}

		uint8_t *pun8FrameBuf = pNextVidFrame->BufferGet();
		memcpy(pun8FrameBuf, m_VideoFrame.buffer, m_VideoFrame.frameSize);
		pNextVidFrame->DataSizeSet(m_VideoFrame.frameSize);

		//
		// Replace all of the NAL Unit sizes with byte stream headers if the platform
		// layer is expecting byte stream headers in the video frame.
		//
		if (m_pVideoOutput->H264FrameFormatGet () == eH264FrameFormatByteStream)
		{
			uint8_t *pun8NALUnit = pun8FrameBuf;
			uint8_t *pun8FrameEnd = pun8NALUnit + m_VideoFrame.frameSize;

			while (pun8NALUnit < pun8FrameEnd)
			{
				uint32_t un32PacketSizeInBytes = *(uint32_t *)pun8NALUnit;
				un32PacketSizeInBytes = stiDWORD_BYTE_SWAP(un32PacketSizeInBytes);

				pun8NALUnit[0] = 0;
				pun8NALUnit[1] = 0;
				pun8NALUnit[2] = 0;
				pun8NALUnit[3] = 1;

				pun8NALUnit += un32PacketSizeInBytes + sizeof(uint32_t);
			}
		}

		pNextVidFrame->FrameIsKeyframeSet(m_VideoFrame.keyFrame);

		pNextVidFrame->FrameIsCompleteSet(true);
		
		hResult = m_pVideoOutput->VideoPlaybackFramePut (pNextVidFrame);
		stiTESTRESULT ();
		
		//
		// Update the last frame duration
		//
		m_CurrentProgress.un64CurrentMediaTime += m_unCurrentFrameDuration;
		m_unCurrentFrameDuration = m_unNextFrameDuration;

		//
		// Increment m_un32NextFrameIndex (auto loop)
		//
		if (m_un32NextFrameIndex < m_un32TotalSampleCount)
		{
			//
			// If the current frame index does not match the next
			// frame index then we must have requested a different fame
			// (such as the first frame in the case of a restart).
			// Do not increment the next frame index in this case.
			//
			if (m_un32CurrentFrameIndex == m_un32NextFrameIndex)
			{
				if (m_skipFrames && m_un32SkipIFrameIndex != 0)
				{
					m_un32NextFrameIndex = m_un32NextFrameIndex == m_un32SkipIFrameIndex ? m_un32NextFrameIndex + 1 : m_un32SkipIFrameIndex;
					m_un32SkipIFrameIndex = 0;
				}
				else
				{
					m_un32NextFrameIndex++;
				}
			}
		}
		else if (m_loopVideo) 
		{
			m_un32NextFrameIndex = 1;
			m_skipFrames = true;
			m_un32SkipToFrameIndex = 1;
		}
		else
		{
			StateSet (IstiMessageViewer::estiVIDEO_END);
			m_un32SkipToFrameIndex = 1;
		}

		//
		// Set a flag to trigger getting the next frame
		//
		m_haveFrame = false;
	}

STI_BAIL:
	return (hResult);
}


/*!
 * \brief Allocates the buffer and returns the frame data
 *
 * \param pDataBuffer - buffer that will return the frame data.
 * \param n32BufferSize - size of the buffer.
 * 
 *  \return stiRESULT_SUCCESS if success
 *  \return stiRESULT_ERROR if there is no frame to return.
 */
stiHResult CFilePlay::CapturedFrameGet()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{EventCapturedFrameGet();});

	return hResult;
}


void CFilePlay::EventCapturedFrameGet()
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
bool CFilePlay::StateValidate (
	const uint32_t un32States) const //!< The state (or states ORed together) to check
{
	stiLOG_ENTRY_NAME (StateValidate);
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
stiHResult CFilePlay::StateSet (
	IstiMessageViewer::EState eNewState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::StateSet\n");
		stiTrace("\tCurr State = %s\n", StateNameGet (m_eState));
		stiTrace("\tNew State = %s\n\n", StateNameGet (eNewState));
	);

	if (eNewState != m_eState)
	{
		m_eState = eNewState;

		if (m_eState == IstiMessageViewer::estiPLAYING)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			// Start the playback
			hResult = VideoPlaybackStart (m_videoMedia);
			stiTESTRESULT ();

			//
			// When we enter the playing state we want to compute the time at the
			// start of the video for use as a reference point.
			//

			m_computeStartTime = true;

			FrameGet ();
		}

		switch (m_eState)
		{
			case estiPLAYING:
				playNextFrame();
				break;

			case estiPAUSED:
				if (m_loadCountdownVideo)
				{
					LoadCountdownVideo();
				}
			break;

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
				else
				{
					m_videoNextFrameTimer.stop();
				}

				if (!m_loadServer.empty ())
				{
					PostEvent ([this]
					{
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
				if (RecordTypeGet() == estiRECORDING_UPLOAD_URL)
				{
					stiTESTCOND(m_spSignMailCall, stiRESULT_ERROR);
					m_spSignMailCall->NextStateSet(esmiCS_DISCONNECTED, estiSUBSTATE_MESSAGE_COMPLETE);
				}
			}
			break;

			case estiPLAYER_IDLE:
			{
				if (!m_loadServer.empty ())
				{
					PostEvent ([this]
					{
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
IstiMessageViewer::EState CFilePlay::StateGet () const
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
IstiMessageViewer::EError CFilePlay::ErrorGet () const
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
stiHResult CFilePlay::ErrorStateSet (
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
IstiMessageViewer::EError CFilePlay::LastKnownErrorGet () const
{
	IstiMessageViewer::EError eError;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eError = m_eLastError;

	return eError;
}


/*!
 * \brief Start Video Playback 
 * 
 * \param Media_t* pMedia 
 * 
 * \return stiHResult 
 * estiOK if successful, otherwise estiERROR 
 *  
 */
stiHResult CFilePlay::VideoPlaybackStart (
	Media_t *pMedia)
{
	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::VideoPlaybackStart\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	SMHandle hSampleDescr = nullptr;

	if (!m_playbackStarted)
	{
		SMResult_t SMResult = SMRES_SUCCESS;

		SampleDescr_t* pSampleDescr = nullptr;

		// Get the sample description
		hSampleDescr = SMNewHandle (0);

		SMResult = MP4GetMediaSampleDescription (pMedia, 1, hSampleDescr);

		stiTESTCOND (SMRES_SUCCESS == SMResult, stiRESULT_MP4_LIB_ERROR);

		pSampleDescr = (SampleDescr_t*) *hSampleDescr;

		// Determine codec type
		switch (BigEndian4(pSampleDescr->unDescrType))
		{
			case eH263SampleDesc:

				m_eVideoCodec = estiVIDEO_H263;
				stiDEBUG_TOOL (g_stiFilePlayDebug,
					stiTrace("\tCodec H263\n");
				);
				break;
			case eAVCSampleDesc:

				m_eVideoCodec = estiVIDEO_H264;
				stiDEBUG_TOOL (g_stiFilePlayDebug,
					stiTrace("\tCodec H264\n");
				);
				break;

			default:
				stiDEBUG_TOOL (g_stiFilePlayDebug,
					stiTrace("\tCodec NONE\n");
				);
				break;
		}

		//
		// For now we will only play videos that use H.264.
		//
		stiTESTCOND (estiVIDEO_H264 == m_eVideoCodec, stiRESULT_INVALID_CODEC);

		// Set codec
		hResult = m_pVideoOutput->VideoPlaybackCodecSet(m_eVideoCodec);
		stiTESTRESULT ();

		// Start decoder
		hResult = m_pVideoOutput->VideoPlaybackStart();
		stiTESTRESULT ();

		// Playback start
		m_playbackStarted = true;

		stiDEBUG_TOOL (g_stiFilePlayDebug,
			stiTrace("\tVideoPlaybackStart OK\n");
		);
	}

STI_BAIL:

	if (hSampleDescr)
	{
		// Dispose of the temporary handle
		SMDisposeHandle (hSampleDescr);
	}

	return hResult;
}


/*!
 * \brief Stop Video Playback
 * 

 */
stiHResult CFilePlay::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::VideoPlaybackStop\n");
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
stiHResult CFilePlay::VideoRecordSettingsSet() 
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
	stRecordSettings.unLevel = estiH264_LEVEL_3;

	//Sending Video Record Settings to the videoInput for encoder configuration
	hResult = m_pVideoInput->VideoRecordSettingsSet (&stRecordSettings);
	stiTESTRESULT ();
	
STI_BAIL:
	return hResult;
}


/*!
 * \brief Returns the average frame rate of the message being played.
 * 
 * \return The aveage frame rate as a uint32_t .
 */
uint32_t CFilePlay::GetAverageFrameRate()
{
	uint32_t un32FrameRate = 0;
	struct timeval timevalNow{};

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_timevalStart.tv_sec)
	{
		gettimeofday (&timevalNow, nullptr);

		if (timevalNow.tv_sec - m_timevalStart.tv_sec > 0)
		{
			un32FrameRate = m_un32CurrentFrameIndex / (timevalNow.tv_sec - m_timevalStart.tv_sec);
		}
	}

	return un32FrameRate;
}


/*!
 * \brief Returns the current mov frame rate of the recording.
 * 
 * \return The mov frame rate as a uint32_t .
 */
uint32_t CFilePlay::GetRecordingFrameRate()
{
	uint32_t un32FrameRate = 0;
	struct timeval timevalNow{};

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_timevalStart.tv_sec)
	{
		gettimeofday (&timevalNow, nullptr);
		uint32_t diff = (1000000 * (timevalNow.tv_sec - m_timevalStart.tv_sec)) + (timevalNow.tv_usec - m_timevalStart.tv_usec);
		if (diff > 0)
		{
			un32FrameRate = diff / 1666;
		}
		m_timevalStart = timevalNow;
	}
	else
	{
		un32FrameRate = g_nVIDEO_RECORD_TIME_SCALE / DEFAULT_FRAME_RATE;
		gettimeofday (&m_timevalStart, nullptr);
	}

	return un32FrameRate;
}


/*!
 * \brief Returns the current download bitrate for the message being played.
 * 
 * \return  The download bitrate as a uint32_t.
 */
uint32_t CFilePlay::GetDownloadBitRate()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Must convert bytes to bits.
	uint32_t un32BitRate = m_CurrentProgress.un32DataRate * 8;

	return un32BitRate;
}


/*!
 * \brief Get Media Sample
 * 
 * \param CServerDataHandler::SMediaSampleInfo* pstMediaSampleInfo 
 * \param SstiPlaybackVideoFrame* pstVideoFrame 
 * 
 * \return Success or failure result 
 */
stiHResult CFilePlay::GetMediaSample (
	const SMediaSampleInfo &mediaSampleInfo,
	vpe::VideoFrame *pstVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint8_t *pun8SampleData = nullptr;
	uint32_t un32SampleSize = 0;

	if (estiVIDEO_H264 == m_eVideoCodec)
	{
		// Check if sequence parameter and picture parameter has changed
		if (m_un32CurrentSampleDescIndex != mediaSampleInfo.un32SampleDescIndex)
		{
			//
			// The sample description is different for this sample
			// Send the sequence paramter set and the picture parameter set
			// together with the frame data for this frame
			//
			hResult = ParamSetsAndFrameBufferCreate (m_videoMedia,
													 mediaSampleInfo.pun8SampleData,
													 mediaSampleInfo.un32SampleSize,
													 mediaSampleInfo.un32SampleDescIndex);
			stiTESTRESULT ();

			pun8SampleData = (uint8_t*)*m_hParamSetsAndFrame;

			un32SampleSize = SMGetHandleSize (m_hParamSetsAndFrame);

			m_un32CurrentSampleDescIndex = mediaSampleInfo.un32SampleDescIndex;
		}
		else
		{
			pun8SampleData = mediaSampleInfo.pun8SampleData;
			un32SampleSize = mediaSampleInfo.un32SampleSize;
		}
	}

#ifdef WRITE_FILE_STREAM
	if (!pFile)
	{
		char fileName[1024];
		sprintf(fileName, "/home/dfrye/Public/VidFiles/output-%d.h264", nFileCount);
		nFileCount++;

		pFile = fopen(fileName, "wb");
	}

	uint8_t *pun8BufferPos;
	pun8BufferPos = pun8SampleData;
	uint32_t un32DataSize;
	un32DataSize = 0;
	uint32_t un32SliceSize;
	un32SliceSize = 0;

	uint8_t startCode[4];
	startCode[0] = 00;
	startCode[1] = 00;
	startCode[2] = 00;
	startCode[3] = 01;

	while (un32DataSize < un32SampleSize)
	{
		un32SliceSize = BigEndian4(*((uint32_t *)pun8BufferPos));
		pun8BufferPos += 4;
		un32DataSize += 4;
		fwrite(startCode, 1, 4, pFile);
		
		fwrite(pun8BufferPos, 1, un32SliceSize, pFile);
		pun8BufferPos += un32SliceSize;
		un32DataSize += un32SliceSize;
	}
#endif

	pstVideoFrame->buffer = pun8SampleData;
	pstVideoFrame->frameSize = un32SampleSize;
	pstVideoFrame->keyFrame = mediaSampleInfo.keyFrame;

STI_BAIL:

	return hResult;
}


/*!
 * \brief Deletes the Greeting that is currently in the FilePlayer's record buffer.
 *
 *
 * \return Success or failure result
 */
stiHResult CFilePlay::GreetingDeleteRecorded()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (RecordTypeGet() == IstiMessageViewer::estiRECORDING_NO_URL)
	{
		RecordTypeSet(IstiMessageViewer::estiRECORDING_IDLE);

		if (StateValidate(IstiMessageViewer::estiRECORDING | IstiMessageViewer::estiRECORD_CONFIGURE | 
						  IstiMessageViewer::estiWAITING_TO_RECORD))
		{
			RecordStop();
		}

		CloseMovie();
	}

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
stiHResult CFilePlay::GreetingRecord(bool showCountdown)
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
			// Just start recording.  No greeting requird.
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
stiHResult CFilePlay::GreetingUpload()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	requestGreetingUploadGUIDSignal.Emit();

	return hResult;
}


/*!
 * \brief Causes the Greeting to be transcoded and then uploaded to
 * 		  supplied server, using the supplied upload GUID.
 * 
 * \return Success or failure result  
 */
stiHResult CFilePlay::GreetingUploadInfoSet(
	const std::string &uploadGUID,
	const std::string &uploadServer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::%s\n", __func__);
	);

	stiTESTCOND(!uploadGUID.empty() && !uploadServer.empty(), stiRESULT_INVALID_PARAMETER);

	// Send an event to upload the GUID
	PostEvent ([this, uploadGUID, uploadServer]{EventGreetingUpload(uploadGUID, uploadServer);});

STI_BAIL:

	return hResult;
}


/*!
 * \brief Sets Parameters And Frame Buffer
 * Create buffer and add sequence parameter and picture parameter to media sample data.  
 *  
 * \param Media_t* pMedia 
 * \param uint8_t* pun8SampleData 
 * \param uint32_t un32SampleSize 
 * \param uint32_t un32SampleDescIndex 
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_INVALID_MEDIA if an error occurs. 
 */
stiHResult CFilePlay::ParamSetsAndFrameBufferCreate (
	Media_t* pMedia,
	uint8_t* pun8SampleData,		// media sample data
	uint32_t un32SampleSize,		// media sample size
	uint32_t un32SampleDescIndex)	// media sample description index
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SMResult_t SMResult = SMRES_SUCCESS;

	stiDEBUG_TOOL (g_stiFilePlayDebug,
		stiTrace("CFilePlay::ParamSetsAndFrameBufferCreate\n");
	);

	SMHandle hSampleDescr;
	AVCSampleDescr_t* pAVCSampleDescr;
	AVCDecoderConfigRecord_t* pAVCDecoderConfigRecord;
	uint8_t unNumOfSeqParamSets;
	uint8_t* pun8Src;
	uint8_t unNumOfPictParamSets;
	uint8_t* pun8Dest;
	uint16_t un16ParamLength[1];
	uint16_t un16ParamSetLength;
	uint32_t un32ParamSetLength;

	//
	// Initialize length to zero
	//
	uint32_t un32Size = 0;

	//
	// Get the sample description
	//
	hSampleDescr = SMNewHandle (0);

	SMResult = MP4GetMediaSampleDescription (pMedia, un32SampleDescIndex, hSampleDescr);
	stiTESTCOND(SMResult == SMRES_SUCCESS, stiRESULT_INVALID_MEDIA);

	pAVCSampleDescr = (AVCSampleDescr_t*)*hSampleDescr;

	pAVCDecoderConfigRecord = (AVCDecoderConfigRecord_t*)&(pAVCSampleDescr->aucConfig);

	//
	// Add the size of the sequence paramter sets
	//
	unNumOfSeqParamSets = pAVCDecoderConfigRecord->aucParameterSets[0] & 0x1F;
	pun8Src = &(pAVCDecoderConfigRecord->aucParameterSets[1]);

	for (int i = 0; i < unNumOfSeqParamSets; i++)
	{
		memcpy (un16ParamLength, pun8Src, 2);
		un16ParamSetLength = BYTESWAP(un16ParamLength[0]);
		un32ParamSetLength = (uint32_t)un16ParamSetLength;

		//
		// Add size of un32ParamSetLength
		//
		un32Size += sizeof (un32ParamSetLength);

		//
		// Add size of sequence paramter set
		//
		un32Size += un32ParamSetLength;
		pun8Src = &pun8Src[un32ParamSetLength + 2];
	}

	unNumOfPictParamSets = pun8Src[0];
	pun8Src = &pun8Src[1];

	for (int i = 0; i < unNumOfPictParamSets; i++)
	{
		memcpy (un16ParamLength, pun8Src, 2);
		un16ParamSetLength = BYTESWAP(un16ParamLength[0]);
		un32ParamSetLength = (uint32_t)un16ParamSetLength;

		//
		// Add size of un32ParamSetLength
		//
		un32Size += sizeof (un32ParamSetLength);

		//
		// Add size of sequence paramter set
		//
		un32Size += un32ParamSetLength;
		pun8Src = &pun8Src[un32ParamSetLength + 2];
	}

	//
	// Add the size of the frame
	//
	un32Size += un32SampleSize;

	//
	// Allocate a buffer
	//
	SMSetHandleSize (m_hParamSetsAndFrame, un32Size);
	pun8Dest = (uint8_t*)*m_hParamSetsAndFrame;

	//
	// Copy the sequence paramter sets
	//
	pun8Src = &(pAVCDecoderConfigRecord->aucParameterSets[1]);
	uint32_t un32SwappedParamSetLength;

	for (int i = 0; i < unNumOfSeqParamSets; i++)
	{
		memcpy (un16ParamLength, pun8Src, 2);
		un16ParamSetLength = BYTESWAP(un16ParamLength[0]);
		un32ParamSetLength = (uint32_t)un16ParamSetLength;

		un32SwappedParamSetLength = DWORDBYTESWAP(un32ParamSetLength);

		//
		// Copy size to m_hParamSetsAndFrame
		//
		memcpy (pun8Dest, &un32SwappedParamSetLength, sizeof(un32SwappedParamSetLength));
		pun8Dest += sizeof (un32ParamSetLength);

		//
		// Advance pointer
		//
		pun8Src = &pun8Src[2];

		//
		// Copy data to m_hParamSetsAndFrame
		//
		memcpy (pun8Dest, pun8Src, un32ParamSetLength);
		pun8Dest += un32ParamSetLength;

		//
		// Advance pointer
		//
		pun8Src = &pun8Src[un32ParamSetLength];

	}

	//
	// Copy the picture parameter sets
	//
	pun8Src = &pun8Src[1];

	for (int i = 0; i < unNumOfPictParamSets; i++)
	{
		memcpy (un16ParamLength, pun8Src, 2);
		un16ParamSetLength = BYTESWAP(un16ParamLength[0]);
		un32ParamSetLength = (uint32_t)un16ParamSetLength;

		un32SwappedParamSetLength = DWORDBYTESWAP(un32ParamSetLength);

		//
		// Copy size to m_hParamSetsAndFrame
		//
		memcpy (pun8Dest, &un32SwappedParamSetLength, sizeof (un32SwappedParamSetLength));
		pun8Dest += sizeof (un32ParamSetLength);

		//
		// Advance pointer
		//
		pun8Src = &pun8Src[2];

		//
		// Copy data to m_hParamSetsAndFrame
		//
		memcpy (pun8Dest, pun8Src, un32ParamSetLength);
		pun8Dest += un32ParamSetLength;

		//
		// Advance pointer
		//
		pun8Src = &pun8Src[un32ParamSetLength];
	}

	//
	// Add frame data to buffer
	//
	memcpy (pun8Dest, pun8SampleData, un32SampleSize);

	// Dispose of the temporary handle
	SMDisposeHandle (hSampleDescr);

STI_BAIL:

	return hResult;
}


/*!
 * \brief Log File Initialize Error
 * Sets the error state and modifies m_eError to the error type.  
 *  
 * \param IstiMessageViewer::EError errorMsg 
 */
void CFilePlay::LogFileInitializeError (
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
void CFilePlay::logStateInfoToSplunk()
{
	PostEvent([this]
	{
		std::stringstream infoString;

		infoString << "State: " << StateNameGet(m_eState); 
		infoString << " Movie ready to play: " << m_movieReadySignaled;

		if (StateValidate (estiOPENING))
		{
			if (m_pMovie)
			{
				MP4LogInfoToSplunk(m_pMovie);
			}
			else
			{
				infoString << " Missing pMovie!";
			}
		}

		if (StateValidate (estiPLAY_WHEN_READY))
		{
			infoString << " buffer %: "  <<  m_CurrentProgress.un32BufferingPercentage;
		}

		if (!StateValidate (estiERROR) && 
			StateValidate (estiPAUSED | estiPLAYING))
		{
			infoString << " bytes still to download: "  << m_CurrentProgress.un64BytesYetToDownload;
			infoString << " time to download: " << m_CurrentProgress.un32TimeToBuffer;
			infoString << " play pos: " << ((double)m_CurrentProgress.un64CurrentMediaTime / (double)m_CurrentProgress.un64MediaTimeScale);
		}

		stiRemoteLogEventSend ("CFilePlay State Info: %s\n", infoString.str().c_str());
	});
}


/*!
 * \brief Sets the SignMail Call.
 *  
 * \param IstiCall call 
 */
void CFilePlay::signMailCallSet(std::shared_ptr<IstiCall> call)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_spSignMailCall = std::static_pointer_cast<CstiCall>(call);
}

ISignal<>& CFilePlay::requestGUIDSignalGet ()
{
	return requestGUIDSignal;
}

ISignal<>& CFilePlay::requestUploadGUIDSignalGet ()
{
	return requestUploadGUIDSignal;
}

ISignal<>& CFilePlay::requestGreetingUploadGUIDSignalGet ()
{
	return requestGreetingUploadGUIDSignal;
}

ISignal<>& CFilePlay::greetingImageUploadCompleteSignalGet ()
{
	return greetingImageUploadCompleteSignal;
}

ISignal<SstiRecordedMessageInfo &>& CFilePlay::deleteRecordedMessageSignalGet ()
{
	return deleteRecordedMessageSignal;
}

ISignal<SstiRecordedMessageInfo &>& CFilePlay::sendRecordedMessageSignalGet ()
{
	return sendRecordedMessageSignal;
}

ISignal<>& CFilePlay::signMailGreetingSkippedSignalGet ()
{
	return signMailGreetingSkippedSignal;
}

ISignal<std::vector<uint8_t> &>& CFilePlay::capturedFrameGetSignalGet ()
{
	return capturedFrameGetSignal;
}

ISignal<IstiMessageViewer::EState>& CFilePlay::stateSetSignalGet ()
{
	return stateSetSignal;
}

ISignal<>& CFilePlay::disablePlayerControlsSignalGet ()
{
	return disablePlayerControlsSignal;
}
