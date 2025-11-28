/*!
*  \file CstiFileUploadHandler.h
*  \brief The File Upload Handler Interface
*
*  Class declaration for the File Upload Handler Class.  
*	This class is responsible for Uploading large files.
*	The Upload of large files can be started and stoped to prevent the
*	taking of bandwidth from other tasks such as making a call.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
*
*/
#ifndef CSTIFILEUPLOADHANDLER_H
#define CSTIFILEUPLOADHANDLER_H

//
// Includes
//
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "CstiEventQueue.h"
//#include "CstiExtendedEvent.h"
//#include "stiEventMap.h"

#include "stiSVX.h"
#include "IstiFileUploadHandler.h"
#include "CstiHTTPService.h"   

#include <sstream>
#include <fstream>

//
// Constants
//

//
// Forward Declarations
//

//
// Globals
//

class CstiFileUploadHandler : public CstiEventQueue, public IstiFileUploadHandler 
{
	
public:
	
	/*!
	 * \brief File Upload Handler constructor
	 * 
	 * \param PstiObjectCallback pfnAppCallback 
	 * \param size_t CallbackParam 
	 */
	CstiFileUploadHandler (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam);
	
	/*!
	 * \brief destructor
	 */
	virtual ~CstiFileUploadHandler ();
	
	CstiFileUploadHandler (const CstiFileUploadHandler &) = delete;
	CstiFileUploadHandler (CstiFileUploadHandler &&) = delete;
	CstiFileUploadHandler &operator= (const CstiFileUploadHandler &) = delete;
	CstiFileUploadHandler &operator= (CstiFileUploadHandler &&) = delete;

	/*!
	 * \brief Initializes the class.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult Initialize () override;
	
	/*!
	 * \brief Starts the task.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult startup () override;

	/*!
	 * \brief Stores the file name and url in a structure and places an event on the FileUploadHandler's 
	 *  	  task queue to start the Upload
	 *  
	 * \param server ip of the upload server.
	 * \param fileName is the name of the saved file.
	 * \param filePath is the location of the saved file.
	 * \param fileUploadInfo contains info about the upload file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 * 		   stiRESULT_INVALID_PARAMETER if pFileName or pUrl is null 
	 *  	   stiRESULT_ERROR for any other error. 
	 */
	stiHResult start (
		std::string server,
		std::string fileName,
		std::string filePath, 
		std::string fileUploadInfo) override;

	/*!
	 * \brief Stops the Upload and removes the file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult stop () override;

	/*!
	 * \brief Returns the current state of the Message Viewer.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	IstiFileUploadHandler::EState stateGet () const override;

	/*!
	 * \brief Enables or disables HTTPS.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	stiHResult useHTTPS (EstiBool useHTTPS) override;

	/*!
	 * \brief Returns the current error state of the File Upload Handler.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	IstiFileUploadHandler::EError errorGet () const override;

	ISignal <>& fileUploadErrorSignalGet () override;
	ISignal <std::string>& fileUploadCompleteSignalGet () override;
	ISignal <unsigned int>& fileUploadProgressSignalGet () override;

protected:

	CstiSignal <> fileUploadErrorSignal;
	CstiSignal <std::string> fileUploadCompleteSignal;
	CstiSignal <unsigned int> fileUploadProgressSignal;

private:
	
	enum EEventType 
	{
		estiFDH_PAUSE = estiEVENT_NEXT,
		estiFDH_RESUME,
		estiFDH_START,
		estiFDH_STOP,
		estiFDH_UPLOAD_NEXT_CHUNK,
	};
	
	//
	// Supporting Functions
	//
	stiHResult clearUploadSettings();
	stiHResult uploadInitialize();
	stiHResult errorStateSet (
		IstiFileUploadHandler::EError errorState);
	
	void stateSet (
		IstiFileUploadHandler::EState eState);

	//
	// Event Handlers
	//
	void eventStartUpload(
		std::string server,
		std::string fileName, 
		std::string filePath, 
		std::string fileUploadInfo);
	void eventUploadNextChunk();
	
	IstiFileUploadHandler::EState m_eState;
	IstiFileUploadHandler::EError m_eError;
	std::fstream m_uploadFile;
	uint64_t m_bytesUploaded {0};
	uint64_t m_uploadFileSize {0};
	uint64_t m_bytesLeftToUpload {0};
	char *m_uploadBuffer {nullptr};
	SSL_CTX *m_pSSLCtx {nullptr};
	CstiHTTPService *m_httpService {nullptr};
	EstiBool m_bUseHTTPS;

	std::string m_fileName;
	std::string m_filePathName; 
	std::string m_fileInfo;
	std::string m_server;
	std::string m_serverIP;

	std::stringstream m_uploadHeader;
	std::stringstream m_uploadContentHeader;
	std::stringstream m_uploadEnd;

	stiSocket m_uploadSocket = -1;
};

#endif //CSTIFILEUPLOADHANDLER_H

