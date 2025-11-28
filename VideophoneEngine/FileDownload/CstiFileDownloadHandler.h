/*!
*  \file CstiFileDownloadHandler.h
*  \brief The File Download Handler Interface
*
*  Class declaration for the File Download Handler Class.  
*	This class is responsible for downloading large files.
*	The download of large files can be started and stoped to prevent the
*	taking of bandwidth from other tasks such as making a call.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
*
*/
#ifndef CSTIFILEDOWNLOADHANDLER_H
#define CSTIFILEDOWNLOADHANDLER_H

//
// Includes
//
#include "CstiEventQueue.h"
#include "CstiEvent.h"

#include "stiSVX.h"
#include "IstiFileDownloadHandler.h"
#include "CstiHTTPService.h"   

#include <sstream>
#include <fstream>

//POCO
#include <Poco/URI.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/SSLManager.h>

//
// Constants
//

//
// Forward Declarations
//

//
// Globals
//

class CstiFileDownloadHandler : public IstiFileDownloadHandler, public CstiEventQueue
{
	
public:
	
	/*!
	 * \brief File Download Handler constructor
	 * 
	 * \param PstiObjectCallback pfnAppCallback 
	 * \param size_t CallbackParam 
	 */
	CstiFileDownloadHandler ();
	
	/*!
	 * \brief destructor
	 */
	~CstiFileDownloadHandler () override;
	
	CstiFileDownloadHandler (const CstiFileDownloadHandler &other) = delete;
	CstiFileDownloadHandler (CstiFileDownloadHandler &&other) = delete;
	CstiFileDownloadHandler &operator= (const CstiFileDownloadHandler &other) = delete;
	CstiFileDownloadHandler &operator= (CstiFileDownloadHandler &&other) = delete;

	/*!
	 * \brief Starts the event queue.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Startup () override;

	/*!
	 * \brief Places an event on the FileDownloadHandler's task queue to pause the download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Pause () override;
	
	/*!
	 * \brief Places an event on the FileDownloadHandler's task queue to resume the download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Resume () override;
	
	/*!
	 * \brief Stores the file name and url in a structure and places an event on the FileDownloadHandler's 
	 *  	  task queue to start the download
	 *  
	 * \param pFileName is the name and location of the saved file.
	 * \param pUrl is the name and download location of the file to download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 * 		   stiRESULT_INVALID_PARAMETER if pFileName or pUrl is null 
	 *  	   stiRESULT_ERROR for any other error. 
	 */
	stiHResult Start (
		const std::string &fileName,
		const std::string &url) override;

	/*!
	 * \brief Stops the download and removes the file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Stop () override;

	/*!
	 * \brief Returns the current state of the Message Viewer.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	IstiFileDownloadHandler::EState StateGet () const override;

	/*!
	 * \brief Enables or disables HTTPS.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult UseHTTPS (EstiBool useHTTPS) override;

	/*!
	 * \brief Returns the current error state of the File Download Handler.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	IstiFileDownloadHandler::EError ErrorGet () const override;

	ISignal <int>& fileDownloadProgressSignalGet () override;
	ISignal <IstiFileDownloadHandler::EState>& fileDownloadStateSetSignalGet () override;
	
protected:

	CstiSignal <int> fileDownloadProgressSignal;
	CstiSignal <IstiFileDownloadHandler::EState> fileDownloadStateSetSignal;

private:

	enum EEventType 
	{
		estiFDH_PAUSE = estiEVENT_NEXT,
		estiFDH_RESUME,
		estiFDH_START,
		estiFDH_STOP,
		estiFDH_DOWNLOAD_NEXT_CHUNK,
	};
	
	//
	// Supporting Functions
	//
	stiHResult ClearDownloadSettings();
	stiHResult DownloadNextChunk();
	stiHResult DownloadInitialize();
	stiHResult ErrorStateSet (
		IstiFileDownloadHandler::EError errorState);
	
	int32_t ReadFileData(
		char *pDataBuffer, 
		uint32_t bufferSize);

	stiHResult RequestFileData(
		uint64_t un64Start, 
		uint64_t un64End);

	void StateSet (
		IstiFileDownloadHandler::EState eState);

	//
	// Event Handlers
	//
	void DownloadNextChunkHandler ();
	void StartHandler (
		const std::string &fileName,
		const std::string &url);

	std::unique_ptr<Poco::Net::HTTPRequest> getDownloadRequest (
		uint64_t rangeStart,
		uint64_t rangeEnd);
	stiHResult HTTPDownloadRangeRequest (
		Poco::Net::HTTPResponse *response,
		uint64_t startRange, 
		uint64_t endRange);
	void socketCleanup ();

	IstiFileDownloadHandler::EState m_eState {IstiFileDownloadHandler::eDOWNLOAD_IDLE};
	IstiFileDownloadHandler::EError m_eError {IstiFileDownloadHandler::eERROR_NONE};
	std::fstream m_downloadingFile;
	uint64_t m_bytesDownloaded {0};
	uint64_t m_downloadFileSize {0};
	uint64_t m_bytesLeftToDownload {0};
	uint32_t m_downloadChunkSize {0};
	uint32_t m_requestedChunkSize {0};
	int m_lastPercentUpdate {0};
	std::vector<char> m_downloadBuffer;
	bool m_bUseHTTPS {false};

	std::string m_fileName;

	std::unique_ptr<Poco::Net::HTTPClientSession> m_clientSession;
	std::unique_ptr<std::istream> m_responseBody;
	
	Poco::URI m_URL;
};

#endif //CSTIFILEDOWNLOADHANDLER_H

