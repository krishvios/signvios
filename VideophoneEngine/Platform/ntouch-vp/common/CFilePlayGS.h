/*!
*  \file CFilePlayGS.h
*  \brief The Message Viewer Interface
*
*  Class declaration for the Message Viewer Interface Class.  
*   This class is responsible for communicating with GStreamer
*   to play and record files.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2021 Sorenson Communications, Inc. -- All rights reserved
*
*/
#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "CstiItemId.h"

#include "stiSVX.h"
#include "IstiMessageViewer.h"
#include "IVideoInputVP2.h"
#include "IVideoOutputVP.h"
#include "CstiCall.h"

#include <vector>

//
// Constants
//

//
// Forward Declarations
//
class CFilePlayGS;

//
// Globals
//

/*!
 * \brief 
 *  
 *  Class declaration for the Message Viewer Interface Class.  
 *	This class is responsible for playing files.
 *	These files can be local or in the process of being downloaded so
 *	this class must be able to handle asking for a frame and being told
 *	that the frame is not here yet.
 *  
 */
class CFilePlayGS : public IstiMessageViewer, public CstiEventQueue
{
public:
	
	/*!
	 * \brief File Play constructor
	 */
	CFilePlayGS ();
	
	/*!
	 * \brief Initialize
	 * 
	 * \return stiHResult 
	 */
	stiHResult Initialize ();
	
	CFilePlayGS (const CFilePlayGS &other) = delete;
	CFilePlayGS (CFilePlayGS &&other) = delete;
	CFilePlayGS &operator= (const CFilePlayGS &other) = delete;
	CFilePlayGS &operator= (CFilePlayGS &&other) = delete;

	/*!
	 * \brief destructor
	 */
	~CFilePlayGS () override;
	
	/*!
	 * \brief Starts up the event loop.
	 */
	stiHResult Startup ();

	/*!
	 * \brief Load Video Message
	 * 
	 * \param const std::string server 
	 * \param const std::string fileGUID 
	 * \param const std::string viewId 
	 * \param uint64_t un64FileSize 
	 * \param const std::string clientToken 
	 * \param bool pause 
	 * \param bool useDownloadSpeed 
	 * 
	 * \return Success or failure result  
	 */
	stiHResult LoadVideoMessage (
		const std::string &server,
		const std::string &fileGUID,
		const std::string &viewId,
		uint64_t un64FileSize,
		const std::string &clientToken,
		bool pause,
		bool useDownloadSpeed = true) override;

	/*!
	 * Opens the message identified by the ItemID.
	 * 
	 * \param pszItemId The id of the file
	 * \param pszName The name of the person who lef the message (security double-check)
	 * \param pszPhoneNumber The phone number of the person who left the message
	 */
	void Open(
		const CstiItemId &itemId,		//!< The Id of the file
		const char *pszName,		//!< The Name of person who left the message (security double-check)
		const char *pszPhoneNumber) override;   //!< The phone number of person who left the message					

	/*!
	 * Opens the message identified by the server and file name.
	 * 
	 * \param pszServer The server of the video to open
	 * \param pszFileName The file name of the video to open
	 */
	void Open(
		const std::string &server,
		const std::string &fileName,
		bool useDownloadSpeed = true) override;

	/*!
	 * \brief Closes the message that is currently playing.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if in the IstiMessageViewer::estiOPENING state, the message cannot be
	 *  	   closed untill the Message Viewer is out of this state. 
	 */
	stiHResult Close () override;

	/*!
	 * \brief Close Movie
	 */
	void CloseMovie () override;
	
	/*!
	 * \brief Clears the SignMail info used for recording.
	 */
	void SignMailInfoClear () override;

	/*! 
	 * \brief Clears conifiguration information, used when recording a point to point SignMail, 
	 * 		 from the Message Viewer. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	      stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult RecordP2PMessageInfoClear () override;

	/*!
	 * \brief Set info for point to point message
	 * 
	 * \param uint32_t un32RecordTime 
	 * \param uint32_t un32DownBitrate 
	 * \param uint32_t un32UpBitrate 
	 * \param const char* pszServer 
	 * \param const char* pszUploadGUID 
	 * \param const char* pszToPhoneNum 
	 */
	void RecordP2PMessageInfoSet (
		uint32_t un32RecordTime,         
		uint32_t un32DownBitrate,        
		uint32_t un32UpBitrate,          
		const std::string &server,       
		const std::string &uploadGUID,   
		const std::string &phoneNumber) override;
	
	/*!
	 * \brief Start recording
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult RecordStart ();

	/*!
	 * \brief Stops recording and prepares the message to be uploaded. If recording is stop because of an 
	 * 		  error the messages is closed and will no be uploaded. 
	 *  
	 * \param bRecordError indicates that recording was stopped because of an error.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult RecordStop (bool bRecordError = false) override;

	/*!
	 * \brief Deletes the currently recorded SignMail message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult DeleteRecordedMessage () override;

	/*!
	 * \brief Sends the currently recorded SignMail message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SendRecordedMessage () override;

	/*!
	 * \brief Used to notify the Message Viewer to stop recording but does not 
	 * 		  generate an error.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult RecordingInterrupted () override;

	/*!
	 * \brief Set maximum download and upload speeds
	 * 
	 * \param uint32_t un32MaxDownloadSpeed 
	 * \param uint32_t un32MaxUploadSpeed 
	 */
	void MaxSpeedSet (
		uint32_t un32MaxDownloadSpeed,
		uint32_t un32MaxUploadSpeed) override;

	/*!
	 * \brief Places the Message Viewer into fast forward mode. The message will play 
	 * 		  at 2X speed. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult FastForward () override;

	/*!
	 * \brief Places the Message Viewer into slow forward mode. The message will play 
	 * 		  at 1/2X speed. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SlowForward () override;

	/*!
	 * \brief Returns the playback speed of the current video. 
	 * 
	 * \return EPlayRate for the current playback speed.
	 */
	EPlayRate playbackSpeedGet () override
	{
		return m_playRate;
	}

	/*!
	 * \brief Plays message.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Play () override;

	/*!
	 * \brief Pauses the Message Viewer
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Pause () override;

	/*!
	 * \brief Plays the message from the begining
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Restart () override;

	/*!
	 * \brief Automatically loops the message back the beginning when the end is reached.
	 *  
	 * \param bLoop specifies whether the message should loop or not. 
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult LoopSet(bool loop) override;

	/*!
	 * \brief Skips back into the message, the number of seconds indicated by unSkipBack. 
	 * 
	 * \param unSkipBack specifies the number of seconds to skip back. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SkipBack (
		unsigned int unSkipBack) override;

	/*!
	 * \brief Skips forward into the message, the number of seconds indicated by unSkipForward.
	 * 
	 * \param unSkipForward specifies teh number of seconds to skip forward.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SkipForward (
		unsigned int unSkipForward) override;

	/*!
	 * \brief Skips to the specified point in the video.
	 * 
	 * \param unSkipTo is the point within the video, in seconds, that should be skipped to.
	 * 
	 * \return Success or failure result 
	 */
	stiHResult SkipTo (
		unsigned int unSkipTo) override;

	/*!
	 * \brief Skips to the end of the message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SkipEnd () override;
	
	/*!
	 * \brief Skips to x seconds from the end.
	 * 
	 * \param unSecondsFromEnd is the point, in seconds, from the end that should be skipped to. 
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_INVALID_MEDIA if media is not playing.
	 *  	   stiRESULT_MEMORY_ALLOC_ERROR if memory could not be allocated.
	 */
	stiHResult SkipFromEnd (unsigned int unSecondsFromEnd) override
	{
		return stiRESULT_ERROR;
	}

	/*!
	 * \brief Skips to the countdown portion of the greeting.
	 *
	 */
	void SkipSignMailGreeting () override;

	/*!
	 * \brief Rerecords a SignMail message.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult SignMailRerecord() override;

	/*!
	 * \brief Returns the current position of the  message, in seconds
	 * 
	 * \return Current position in seconds as a uint32_t 
	 */
	uint32_t StopPositionGet() override;

	/*!
	 * \brief Sets the position, in seconds, the message should start playing at.
	 * 
	 * \param un32StartPosition specifies the start position in seconds.
	 */
	void SetStartPosition(
		uint32_t un32StartPostion) override;

	/*!
	 * \brief Populates a SstiProgress stucture which contains inforamation that 
	 *  	  can be used to determine the messages download progress, playing time and position.
	 * 
	 * \param pProgress is a SstiProgress stucture that is populated the Message Viewer.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult ProgressGet (
		IstiMessageViewer::SstiProgress* pProgress) const override;

	/*!
	 * \brief Returns the current state of the Message Viewer.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	IstiMessageViewer::EState StateGet () const override;
	
	/*!
	 * \brief Returns the current error state of the Message Viewer.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	IstiMessageViewer::EError ErrorGet () const override;

	/*!
	 * \brief Returns the most recent error state of the Message Viewer.
	 *
	 * \return One of the errors defined in the EError enum.
	 */
	IstiMessageViewer::EError LastKnownErrorGet () const override;

	/*!
	 * \brief Log File initialize error
	 * 
	 * \param IstiMessageViewer::EError errorMsg 
	 */
	void LogFileInitializeError (
		IstiMessageViewer::EError errorMsg) override;

	/*!
	 * \brief Logs Message Viewer's state and other info to splunk..
	 *
	 */
	void logStateInfoToSplunk() override;

	/*!
	 * \brief get item ID
	 * 
	 * \return const char* 
	 */
	CstiItemId GetItemId() const override
	{
		return m_pItemId;
	}

	/*!
	 * \brief Returns the Item ID of the message.
	 * 
	 * \return A pointer to a CstiItemId which contains the messages Item ID. 
	 */
	std::string GetViewId() override
	{
		return m_viewId;
	}

	/*!
	 * \brief Returns the average frame rate of the message being played.
	 * 
	 * \return The aveage frame rate as a uint32_t .
	 */
	uint32_t GetAverageFrameRate() override
	{
		return 0;
	}

	/*!
	 * \brief Returns the current download bitrate for the message being played.
	 * 
	 * \return  The download bitrate as a uint32_t.
	 */
	uint32_t GetDownloadBitRate() override
	{
		return 0;
	}

	/*!
	 * \brief Returns the recording type defined in eRecordType.
	 * 
	 * \return Returns the recording type as a uint8_t.
	 */
	IstiMessageViewer::ERecordType RecordTypeGet() override;

	/*!
	 * \brief Sets the recording type defined in eRecordType.
	 * 
	 * \param un8RecordType is a type defined in eRecordType.  This defines the 
	 * 		  type of recording the message viewer will preform. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult RecordTypeSet(
		IstiMessageViewer::ERecordType eRecordType) override;
	
	stiHResult Shutdown ();

	/*!
	 * \brief Deletes the Greeting that is currently in the FilePlayer's record buffer.
	 *
	 *
	 * \return Success or failure result
	 */
	stiHResult GreetingDeleteRecorded() override;

	/*!
	 * \brief Configures filePlayer so that it can record a greeting and
	 * 		  begins the recording process.
	 * 
	 * \param bShowCountdown indicates if the countdown video should be played. 
	 *  	  It is defaulted to true.
	 *  
	 * \return Success or failure result
	 */
	stiHResult GreetingRecord(
		bool showCountdown = true) override;

	/*!
     * \brief Tells filePlayer to upload the recorded greeting.
     *  
	 * \return Success or failure result
	 */
	stiHResult GreetingUpload() override;

	/*!
	 * \brief Causes the Greeting to be transcoded and then uploaded to
	 * 		  supplied server, using the supplied upload GUID.
	 * 
	 * \return Success or failure result  
	 */
	stiHResult GreetingUploadInfoSet(
		const std::string &uploadGUID,
		const std::string &uploadServer) override;

	/*!
	 * \brief Used to clear the messageId when rerecording a signmail message.
	 * 		  
	 * 
	 * \return Success or failure result  
	 */
	stiHResult RecordMessageIdClear();

	/*!
	 * \brief Loads and begin to play the countdown video.
	 * 		  
	 * \param bWaitToPlay tells FilePlayer to load the countdown video now or 
	 * 		  wait for a video to play first. 
	 * 
	 * \return N/A 
	 *  
	 */
	stiHResult LoadCountdownVideo(
		bool waitToPlay = false) override;

	/*!
	 * \brief Returns the frame data
	 *
	 * \param pDataBuffer - buffer that will return the frame data.
	 * \param un16BufferSize - size of the buffer.
	 * 
	 *  \return stiRESULT_SUCCESS if success
	 *  \return stiRESULT_ERROR if there is frame to return.
	 */
	stiHResult CapturedFrameGet() override;

	/*!
	 * \brief Sets the SignMail Call Index.
	 *  
	 * \param int callIndex 
	 */
	void signMailCallSet(std::shared_ptr<IstiCall> call) override;

	// Signals
	ISignal<>& requestGUIDSignalGet () override;
	ISignal<>& requestUploadGUIDSignalGet () override;
	ISignal<>& requestGreetingUploadGUIDSignalGet () override;
	ISignal<>& greetingImageUploadCompleteSignalGet () override;
	ISignal<SstiRecordedMessageInfo &>& deleteRecordedMessageSignalGet () override;
	ISignal<SstiRecordedMessageInfo &>& sendRecordedMessageSignalGet () override;
	ISignal<>& signMailGreetingSkippedSignalGet () override;
	ISignal<std::vector<uint8_t> &>& capturedFrameGetSignalGet () override;
	ISignal<EState>& stateSetSignalGet () override;
	ISignal<>& disablePlayerControlsSignalGet () override;

	CstiSignal<> requestGUIDSignal;
	CstiSignal<> requestUploadGUIDSignal;
	CstiSignal<> requestGreetingUploadGUIDSignal;
	CstiSignal<> greetingImageUploadCompleteSignal;
	CstiSignal<SstiRecordedMessageInfo &> deleteRecordedMessageSignal;
	CstiSignal<SstiRecordedMessageInfo &> sendRecordedMessageSignal;
	CstiSignal<> signMailGreetingSkippedSignal;
	CstiSignal<std::vector<uint8_t> &> capturedFrameGetSignal;
	CstiSignal<IstiMessageViewer::EState> stateSetSignal;
	CstiSignal<> disablePlayerControlsSignal;

private:

	enum EBufferDataType
	{
		estiEMPTY_DATA,
		estiDOWNLOADED_DATA,
		estiRECORDED_DATA,
	};

	struct SRecordFrameInfo
	{
		uint8_t *pun8FrameData;
		uint32_t un32FrameSize;
	}; 

	//
	// Supporting Functions
	//
	void InitiateClose ();

	stiHResult ProgressUpdate ();

	stiHResult ErrorStateSet (
		IstiMessageViewer::EError errorState);

	bool StateValidate (
		uint32_t un32States) const; //!< The state (or states ORed together) to check
	
	stiHResult StateSet (
		IstiMessageViewer::EState eNewState);

	stiHResult VideoPlaybackStop ();

	void BadGuidReported ();

	stiHResult VideoRecordSettingsSet();

	void fileUploadStart();
	void fileUploadFinish();
	void recordedFileDelete();
	void recordedMessageCleanup ();

	void VideoInputSignalsConnect ();
	void VideoInputSignalsDisconnect ();

	void videoOutputSignalsConnect ();
	void videoOutputSignalsDisconnect ();

	//
	// Event Handlers
	//
	void EventPlay ();
	void EventPause ();
	void EventRestart ();
	void EventSkipBack (
		unsigned int skipBack);
	void EventSkipForward (
		uint32_t skipForward);
	void EventSkipEnd ();
	void EventFastForward ();
	void EventSlowForward ();
	void EventBadGUID ();
//	void EventBufferUploadSpaceUpdate (
//		const SFileUploadProgress &uploadProgress);
	void EventLoadCountdownVideo ();
	void EventMovieReady ();
	void EventMediaReady ();
	void EventMessageSize (
		uint32_t messageSize);
	void EventMovieClosed ();
	void EventCloseMovie ();
	void EventRecordConfig ();
	void EventRecordP2PMessageInfoClear ();
	void EventRecordStart ();
	void EventRecordStop ();
	void EventStateSet (
		IstiMessageViewer::EState state,
		IstiMessageViewer::EError error);
	void EventGreetingUpload (
		std::string uploadGUID,
		std::string uploadServer);
	void eventGreetingImageUpload ();
	void EventUploadMessageId (
		std::string messageId);
	void EventUploadProgress (
		uint32_t progress);
	void EventOpenMovie ();
	void EventDownloadError ();
	void EventDeleteRecordedMessage ();
	void eventNetworkDisconnected();
	void eventVideoEnd();
	void eventRecordFinished();
	void eventRecordError();
	void eventFileUploadComplete ();
	void eventFileUploadError ();
	void eventCapturedFrameGet ();
	void eventLoadVideoMessage (
		const std::string &server,
		const std::string &fileGUID,
		const std::string &viewId,
		uint64_t fileSize,
		const std::string &clientToken,
		bool pause,
		bool useDownloadSpeed = true);
	void eventSendRecordedMessage ();

	IstiMessageViewer::EState m_eState {IstiMessageViewer::estiCLOSED};
	IVideoInputVP2 *m_pVideoInput {nullptr};
	IVideoOutputVP *m_pVideoOutput {nullptr};
	vpe::VideoFrame m_VideoFrame {};
	uint32_t m_un32StartPos {0};
	bool m_loadCountdownVideo {false};
	bool m_loopVideo {false};
	bool m_cleanupRecordedFile {false};

	// Message id info.
	CstiItemId m_pItemId;
	std::string m_messageId;
	std::string m_name;
	std::string m_phoneNumber;
	std::string m_viewId;
	std::string m_downloadGuid;
	std::string m_server;
	bool m_guidIsGreeting {false};

	std::string m_loadServer;
	std::string m_loadFileGUID;
	std::string m_loadClientToken;
	std::string m_loadViewId;
	uint64_t m_loadFileSize {0};
	bool m_loadDownloadSpeed {false};
	bool m_loadPause {false};
	bool m_loadLoop {false};

	IstiMessageViewer::SstiProgress m_CurrentProgress {};

	EPlayRate m_playRate {ePLAY_NORMAL};		// Used to adjust the playrate. estiPLAY_NORMAL estiPLAY_FAST, estiPLAY_SLOW.
	unsigned int m_unDataType {estiEMPTY_DATA};	// Used to track the type of data in the data buffer.
	bool m_largeVideoFrame {false};

	bool m_playbackStarted {false};
	bool m_openPaused {false};
	bool m_useDownloadSpeed {false};
	bool m_bufferToEnd {false};
	uint32_t m_un32SplunkRebufferCount {0};
	bool m_downloadFailed {false};

	bool m_closeDueToBadGuid {false}; 
	bool m_closedDuringGUIDRequest {false};
	int m_nGuidRequests {0};

	EstiVideoCodec m_eVideoCodec {estiVIDEO_NONE};
	uint32_t m_un32CurrentSampleDescIndex {0};
	uint64_t m_un64BytesBuffered {0};
	uint64_t m_un64BytesYetToDownload {0};
	uint32_t m_un32MaxBufferingSize {0};
	
	IstiMessageViewer::EError m_eError {IstiMessageViewer::estiERROR_NONE};
	IstiMessageViewer::EError m_eLastError {estiERROR_NONE};

	// Record members
	std::string m_uploadServer;   
	std::string m_uploadGUID;
	IstiMessageViewer::ERecordType m_eRecordType {estiRECORDING_IDLE};
	uint32_t m_un32RecordTime {0};
	uint32_t m_un32CurrentRecordBitrate;
	uint32_t m_un32MaxRecordBitrate;
	uint32_t m_un32RecordWidth;
	uint32_t m_un32RecordHeight;
	uint32_t m_un32RecordSize {0};
	uint32_t m_un32RecordLength {0};
	uint32_t m_un32IntraFrameRate;
	bool m_waitingToUploadRecordedMsg {false};

	uint32_t m_encodeCameraErrorCount {0};
	vpe::EventTimer m_restartRecordErrorTimer;

	int m_maxDownloadSpeed {0};
	int m_maxUploadSpeed {0};

	CstiCallSP m_spSignMailCall {nullptr};

	bool m_bReceivingVideo {false};

	std::vector<uint8_t> m_frameCaptureData;

	std::string m_recordFileName;
	std::string m_recordFilePath;

	CstiSignalConnection::Collection m_videoInputSignalConnections;
	CstiSignalConnection::Collection m_videoOutputSignalConnections;
	CstiSignalConnection::Collection m_signalConnections;
	CstiSignalConnection::Collection m_fileUploadSignalConnections;
};
