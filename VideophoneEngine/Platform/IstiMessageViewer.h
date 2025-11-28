/*!
*  \file IstiMessageViewer.h
*  \brief The Message Viewer Interface
*
*  Class declaration for the Message Viewer Interface Class.  
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
#if !defined(ISTIMESSAGEVIEWER_H)
#define ISTIMESSAGEVIEWER_H

#include "stiSVX.h"
#include "ISignal.h"
#include "IstiCall.h"
#include "IstiVideoOutput.h"

/*!
 * \brief This class is used to view or record video messages.
 */
class IstiMessageViewer
{
public:

	/*! 
	 * \brief This structure contains information about message playback progress 
	 *  
	 */
	struct SstiProgress
	{
		uint64_t un64FileSizeInBytes;			//!< The file size of the whole file in bytes
		uint64_t un64BytesYetToDownload;		//!< The number of bytes yet to download
		uint32_t un32DataRate;					//!< The actual data rate of the file transfer (in bytes per second)
		uint64_t un64MediaDuration;			//!< The duration of the media
		uint64_t un64CurrentMediaTime;			//!< The current play position in the media
		uint64_t un64MediaTimeScale;			//!< The time scale of the media
		uint32_t un32BufferingPercentage;		//!< The buffering percentage when in the play when ready state
		uint32_t un32TimeToBuffer;				//!< Time required to buffer in second
	};
	
	//
	// State identifiers.
	// Each value must be a power of two.
	//
	/*! 
	*  \enum EState
	*  
	*  \brief EState lists the valid states of the Message Viewer.
	* 
	*/ 
	enum EState
	{
		estiOPENING = 			0x00001,         //!
		estiRELOADING = 		0x00002,         //!
		estiPLAY_WHEN_READY = 	0x00004,         //!
		estiPLAYING = 			0x00008,         //!
		estiPAUSED = 			0x00010,         //!
		estiCLOSING = 			0x00020,         //!
		estiCLOSED = 			0x00040,         //!
		estiERROR = 			0x00080,         //!
		estiVIDEO_END = 		0x00100,         //!
		estiREQUESTING_GUID = 	0x00200,         //!
		estiRECORD_CONFIGURE = 	0x00400,         //!
		estiWAITING_TO_RECORD = 0x00800,         //!
		estiRECORDING = 		0x01000,         //!
		estiRECORD_FINISHED = 	0x02000,         //!
		estiRECORD_CLOSING = 	0x04000,         //!
		estiRECORD_CLOSED = 	0x08000,         //!
		estiUPLOADING = 		0x10000,         //!
		estiUPLOAD_COMPLETE	=	0x20000,         //!
		estiPLAYER_IDLE		= 	0x40000          //!
	};
		
	/*! 
	* \enum EError
	*  
	*  \brief EError lists the error states of the Message Viewer.
	*/ 
	enum EError
	{
		estiERROR_NONE,                          //!
		estiERROR_GENERIC,                       //!
		estiERROR_SERVER_BUSY,                   //!
		estiERROR_OPENING,                       //!
		estiERROR_REQUESTING_GUID,               //!
		estiERROR_PARSING_GUID,                  //!
		estiERROR_PARSING_UPLOAD_GUID,           //!
		estiERROR_RECORD_CONFIG_INCOMPLETE,      //!
		estiERROR_RECORDING,                     //!
		estiERROR_NO_DATA_UPLOADED               //!
	};
	
	/*! 
	* \enum ERecordType
	*  
	*  \brief ERecordType lists the type of recording the Message Viewer is performing.
	*/ 
	enum ERecordType
	{
		estiRECORDING_IDLE = 1,
		estiRECORDING_UPLOAD_URL = 2,
		estiRECORDING_NO_URL = 3,
		estiRECORDING_REVIEW_UPLOAD_URL = 4,
	};

	/*!
	 * \brief Get the file player instance 
	 * 
	 * \return A pointer to the file player
	 */
	static IstiMessageViewer *InstanceGet ();


	/*!
	 * Opens the message identified by the ItemID.
	 * 
	 * \param pItemId The id of the file
	 * \param pszName The name of the person who left the message (security double-check)
	 * \param pszPhoneNumber The phone number of the person who left the message
	 */
	virtual void Open(
		const CstiItemId &itemId,
		const char *pszName,
		const char *pszPhoneNumber) = 0;

	/*!
	 * Opens the message identified by the server and file name.
	 * 
	 * \param pszServer The server of the video to open
	 * \param pszFileName The file name of the video to open
	 */
	virtual void Open(
		const std::string &server,
		const std::string &fileName,
		bool useDownloadSpeed = true) = 0;

	/*! 
	 * \brief Clears configuration information, used when recording a point to point SignMail, 
	 * 		 from the Message Viewer. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	      stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult RecordP2PMessageInfoClear () = 0;

	/*!
	 * \brief Stops recording and prepares the message to be uploaded. If recording is stopped because of an 
	 * 		  error the message is closed and will not be uploaded. 
	 *  
	 * \param bRecordError indicates that recording was stopped because of an error.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult RecordStop (bool bRecordError = false) = 0;

	/*!
	 * \brief Used to notify the Message Viewer to stop recording but does not 
	 * 		  generate an error.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult RecordingInterrupted () = 0;

	/*!
	 * \brief Deletes the currently recorded SignMail message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult DeleteRecordedMessage () = 0;

	/*!
	 * \brief Sends the currently recorded SignMail message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SendRecordedMessage () = 0;

	/*!
	 * \brief Places the Message Viewer into fast forward mode. The message will play 
	 * 		  at 2X speed. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult FastForward () = 0;

	/*!
	 * \brief Places the Message Viewer into slow forward mode. The message will play 
	 * 		  at 1/2X speed. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SlowForward () = 0;

	/*!
	 * \brief Returns the playback speed of the current video. 
	 * 
	 * \return EPlayRate for the current playback speed.
	 */
	virtual EPlayRate playbackSpeedGet () = 0;

	/*!
	 * \brief Closes the message that is currently playing.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if in the IstiMessageViewer::estiOPENING state, the message cannot be
	 *  	   closed untill the Message Viewer is out of this state. 
	 */
	virtual stiHResult Close () = 0;
	
	/*!
	 * \brief Plays message.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Play () = 0;
	
	/*!
	 * \brief Pauses the Message Viewer
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Pause () = 0;
	
	/*!
	 * \brief Plays the message from the beginning
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Restart () = 0;

	/*!
	 * \brief Automatically loops the message back the beginning when the end is reached.
	 * 
	 * \param bLoop specifies whether the message should loop or not. 
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult LoopSet (bool loop) = 0;

	/*!
	 * \brief Skips back into the message, the number of seconds indicated by unSkipBack. 
	 * 
	 * \param unSkipBack specifies the number of seconds to skip back. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SkipBack (
		unsigned int unSkipBack) = 0;

	/*!
	 * \brief Skips forward into the message, the number of seconds indicated by unSkipForward.
	 * 
	 * \param unSkipForward specifies the number of seconds to skip forward.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SkipForward (
		unsigned int unSkipForward) = 0;

	/*!
	 * \brief Skips to the specified second, indicated by unSkipTo, in the video.
	 * 
	 * \param unSkipTo is the point, in seconds, that should be skipped to.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SkipTo (
		unsigned int unSkipTo) = 0;

	/*!
	 * \brief Skips to the end of the message
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SkipEnd () = 0;

	/*!
	 * \brief Skips to x seconds from the end.
	 * 
	 * \param unSecondsFromEnd is the point, in seconds, from the end that should be skipped to. 
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_INVALID_MEDIA if media is not playing.
	 *  	   stiRESULT_MEMORY_ALLOC_ERROR if memory could not be allocated.
	 */
	virtual stiHResult SkipFromEnd (unsigned int unSecondsFromEnd) = 0;

	/*!
	 * \brief Skips to the countdown portion of the greeting.
	 * 
	 */
	virtual void SkipSignMailGreeting () = 0;

	/*!
	 * \brief Rerecords a SignMail message.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult SignMailRerecord () = 0;
	
	/*!
	 * \brief Populates a SstiProgress stucture which contains information that 
	 *  	  can be used to determine the messages download progress, playing time and position.
	 * 
	 * \param pProgress is a SstiProgress structure that is populated the Message Viewer.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult ProgressGet (
		SstiProgress *pProgress) const = 0;

	/*!
	 * \brief Returns the current state of the Message Viewer.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	virtual EState StateGet () const = 0;
	
	/*!
	 * \brief Returns the current error state of the Message Viewer.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	virtual EError ErrorGet () const = 0;

	/*!
	 * \brief Returns the most recent error state of the Message Viewer.
	 *
	 * \return One of the errors defined in the EError enum.
	 */
	virtual EError LastKnownErrorGet () const = 0;

	/*!
	 * \brief Returns the current position of the message, in seconds
	 * 
	 * \return Current position in seconds as a uint32_t 
	 */
	virtual uint32_t StopPositionGet() = 0;

	/*!
	 * \brief Sets the position, in seconds, the message should start playing at.
	 * 
	 * \param un32StartPosition specifies the start position in seconds.
	 */
	virtual void SetStartPosition(
		uint32_t un32StartPosition) = 0;

	/*!
	 * \brief Returns the Item ID of the message.
	 * 
	 * \return A pointer to a CstiItemId which contains the messages Item ID. 
	 */
	virtual std::string GetViewId() = 0;

	/*!
	 * \brief Returns the current download bitrate for the message being played.
	 * 
	 * \return  The download bitrate as a uint32_t.
	 */
	virtual uint32_t GetDownloadBitRate() = 0;

	/*!
	 * \brief Returns the average frame rate of the message being played.
	 * 
	 * \return The average frame rate as a uint32_t .
	 */
	virtual uint32_t GetAverageFrameRate() = 0;

	/*!
	 * \brief Request the Allocated buffer of the frame data.  Will send estiMSG_CAPTURED_FRAME_GET
	 * param will contain a pointer to a SstiFrameCaptureData.
	 *
	 *  \return stiRESULT_SUCCESS 
	 *  \return stiRESULT_ERROR 
	 */
	virtual stiHResult CapturedFrameGet() = 0;

	/*!
	 * \brief Deletes the Greeting that is currently in the FilePlayer's record buffer.
	 *
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult GreetingDeleteRecorded() = 0;

	/*!
	 * \brief Configures filePlayer so that it can record a greeting and
	 * 		  begins the recording process.
	 * 
	 * \param bShowCountdown indicates if the countdown video should be played. 
	 *  	  It is defaulted to true.
	 *  
	 * \return Success or failure result
	 */
	virtual stiHResult GreetingRecord(
		bool showCountdown = true) = 0;

	/*!
     * \brief Tells filePlayer to upload the recorded greeting.
     *  
	 * \return Success or failure result
	 */
	virtual stiHResult GreetingUpload() = 0;

	/*!
     * \brief Tells filePlayer load the countdown video.
     *  
	 * \return Success or failure result
	 */
	virtual stiHResult LoadCountdownVideo(
		bool waitToPlay = false) = 0;

	/*!
	 * \brief Set maximum download and upload speeds
	 * 
	 * \param uint32_t un32MaxDownloadSpeed 
	 * \param uint32_t un32MaxUploadSpeed 
	 */
	virtual void MaxSpeedSet (
		uint32_t un32MaxDownloadSpeed,
		uint32_t un32MaxUploadSpeed) = 0;

	/*!
	 * \brief Load Video Message
	 * 
	 * \param const char* pszServer 
	 * \param const char* pszFileGUID 
	 * \param const char* pszViewId 
	 * \param uint64_t un64FileSize 
	 * \param const char* pszClientToken 
	 * \param EstiBool bPause 
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult LoadVideoMessage (
		const std::string &server,
		const std::string &fileGUID,
		const std::string &viewId,
		uint64_t un64FileSize,
		const std::string &clientToken,
		bool pause,
		bool useDownloadSpeed = true) = 0;

	/*!
	 * \brief Log File initialize error
	 * 
	 * \param IstiMessageViewer::EError errorMsg 
	 */
	virtual void LogFileInitializeError (
		IstiMessageViewer::EError errorMsg) = 0;

	/*!
	 * \brief Clears the SignMail info used for recording.
	 */
	virtual void SignMailInfoClear () = 0;

	/*!
	 * \brief Returns the recording type defined in eRecordType.
	 * 
	 * \return Returns the recording type as a uint8_t.
	 */
	virtual IstiMessageViewer::ERecordType RecordTypeGet() = 0;

	/*!
	 * \brief Sets the recording type defined in eRecordType.
	 * 
	 * \param un8RecordType is a type defined in eRecordType.  This defines the 
	 * 		  type of recording the message viewer will preform. 
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult RecordTypeSet(
		IstiMessageViewer::ERecordType eRecordType) = 0;

	/*!
	 * \brief get item ID
	 * 
	 * \return const char* 
	 */
	virtual CstiItemId GetItemId() const = 0;

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
	virtual void RecordP2PMessageInfoSet (
		uint32_t un32RecordTime,         
		uint32_t un32DownBitrate,        
		uint32_t un32UpBitrate,          
		const std::string &server,       
		const std::string &uploadGUID,   
		const std::string &phoneNumber) = 0; 

	/*!
	 * \brief Causes the Greeting to be transcoded and then uploaded to
	 * 		  supplied server, using the supplied upload GUID.
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult GreetingUploadInfoSet(
		const std::string &uploadGUID,
		const std::string &uploadServer) = 0;

	/*!
	 * \brief Close Movie
	 */
	virtual void CloseMovie () = 0;
	
	/*!
	 * \brief Sets the SignMail Call Index.
	 *
	 * \param int callIndex 
	 */
	virtual void signMailCallSet(std::shared_ptr<IstiCall> call) = 0;

	/*!
	 * \brief Logs Message Viewer's state and other info to splunk..
	 *
	 */
	virtual void logStateInfoToSplunk() = 0;

	/*!
	* \brief Requests the download GUID for a video from Core.
	*/
	virtual ISignal<>& requestGUIDSignalGet () = 0;

	/*!
	* \brief Requests the upload GUID for a recorded video from Core.
	*/
	virtual ISignal<>& requestUploadGUIDSignalGet () = 0;

	/*!
	* \brief Requests the upload GUID for a recorded greeting from Core.
	*/
	virtual ISignal<>& requestGreetingUploadGUIDSignalGet () = 0;

	/*!
	* \brief Notifies the UI that the greeting image upload completed.
	*/
	virtual ISignal<>& greetingImageUploadCompleteSignalGet () = 0;

	/*!
	* \brief Notifies CCI to delete the P2P SignMail specified by SstiRecordedMessageInfo.
	*/
	virtual ISignal<SstiRecordedMessageInfo &>& deleteRecordedMessageSignalGet () = 0;

	/*!
	* \brief Notifies CCI to send the P2P SignMail specified by SstiRecordedMessageInfo.
	*/
	virtual ISignal<SstiRecordedMessageInfo &>& sendRecordedMessageSignalGet () = 0;

	/*!
	* \brief Notifies CCI that the SignMail greeting was skipped.
	*/
	virtual ISignal<>& signMailGreetingSkippedSignalGet () = 0;

	/*!
	* \brief Notifies UI that a new greeting image is available and passes the UI the data.
	*/
	virtual ISignal<std::vector<uint8_t> &>& capturedFrameGetSignalGet () = 0;

	/*!
	* \brief Notifies listeners that a new state has been set and what that state is.
	*/
	virtual ISignal<EState>& stateSetSignalGet () = 0;

	/*!
	* \brief Notifies UI to disable player controls.
	*/
	virtual ISignal<>& disablePlayerControlsSignalGet () = 0;
};

#endif // ISTIMESSAGEVIEWER_H
