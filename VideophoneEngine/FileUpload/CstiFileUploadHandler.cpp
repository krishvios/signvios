/*!
*  \file CstiFileUploadHandler.cpp
*  \brief The File Upload Handler Interface
*
*  Class declaration for the File Upload Handler Class.  
*	This class is responsible for downloading large files.
*	The download of large files can be started and stoped to prevent the
*	taking of bandwidth from other tasks such as making a call.
*
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#include <cctype>  // standard type definitions.
#include <sys/stat.h>
#include <sstream>

#include "CstiFileUploadHandler.h"

#include "stiTaskInfo.h"
#include "stiTools.h"
#include "stiDebugTools.h"
#include "stiRemoteLogEvent.h"
#include "stiHTTPServiceTools.h"
#include "stiSSLTools.h"


//
// Constants
//
#define MIN_UPLOAD_BUFFER_LENGTH (16 * 1024)
#define MAX_UPLOAD_BUFFER_LENGTH (128 * 1024)
#define REQUEST_HEADER_BUFFER_LENGTH (5 * 1024)
#define UPLOAD_BUFFER_SIZE (MAX_UPLOAD_BUFFER_LENGTH + 64) // Need additonal space for content sizes
static const int nREAD_TIMEOUT = 10;

//
// Typedefs
//
//


//
// Globals
//
static CstiFileUploadHandler *g_pFileUploadHandler {nullptr};  /// Global pointer to FileUploadHandler class instance.
static int g_nCstiFileUploadHandlerInstanceCount {0}; /// Global instance counter for CstiFileUploadHandler instance.

//
// Locals
//
 
///
/// \brief Creates CstiFileUploadHandler object, returns stiHResult based on success
///
stiHResult CstiFileUploadHandlerInitialize (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!g_pFileUploadHandler)
	{
		g_pFileUploadHandler = new CstiFileUploadHandler(pfnAppCallback, CallbackParam);
		stiTESTCOND (g_pFileUploadHandler, stiRESULT_ERROR);

		g_pFileUploadHandler->Initialize();
	}

	++g_nCstiFileUploadHandlerInstanceCount; // Increment instance counter

STI_BAIL:

	return hResult;
}


///
/// \brief Decrements global instance count and (if needed) deletes the update instance
///
stiHResult CstiFileUploadHandlerUninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	--g_nCstiFileUploadHandlerInstanceCount; // Decrement instance counter

	if (0 >= g_nCstiFileUploadHandlerInstanceCount && g_pFileUploadHandler)
	{
		delete g_pFileUploadHandler;
		g_pFileUploadHandler = nullptr;
	}

	return (hResult);

} // end CstiFileUploadHandlerUninitialize


///
/// \brief Returns instance of update object
///
IstiFileUploadHandler *IstiFileUploadHandler::InstanceGet ()
{
	return g_pFileUploadHandler;
}


//
// Forward Declarations
//

//
// Constructor
//
/*!
 * \brief  Constructor
 * 
 * \param PstiObjectCallback pAppCallback 
 * \param size_t CallbackParam 
 */
CstiFileUploadHandler::CstiFileUploadHandler (
	PstiObjectCallback /*pAppCallback*/,
	size_t /*CallbackParam*/)
	: CstiEventQueue("CstiFileUploadHandler"),
	m_eState (IstiFileUploadHandler::eUPLOAD_IDLE),
	m_eError (IstiFileUploadHandler::eERROR_NONE),
	m_bUseHTTPS (estiFALSE)
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::CstiFileUploadHandler)
}


/*!
 * \brief Initializes the object
 *
 * \return stiHResult
 * stiRESULT_SUCCESS, or on error
 *
 */
stiHResult CstiFileUploadHandler::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::Initialize\n");
	); // stiDEBUG_TOOL

	// Initialize any variables.
	m_fileName.clear();
	m_filePathName.clear();
	m_fileInfo.clear();
	m_server.clear();
	m_serverIP.clear();

	return hResult;
}


//
// Destructor
//
/*!
 * \brief Destructor
 */
CstiFileUploadHandler::~CstiFileUploadHandler ()
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::~CstiFileUploadHandler)

	clearUploadSettings();
}

/*!
 * \brief Clears settings used for downloading a file.
 *
 */

stiHResult CstiFileUploadHandler::clearUploadSettings()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_server.clear();
	m_serverIP.clear();
	m_fileName.clear();
	m_filePathName.clear();
	m_fileInfo.clear();

	if (m_httpService)
	{
		m_httpService->SocketClose ();

		delete m_httpService;
		m_httpService = nullptr;
	}

	if (m_uploadBuffer)
	{
		delete []m_uploadBuffer;
		m_uploadBuffer = nullptr;
	}

	m_bUseHTTPS = estiFALSE;
	m_uploadFile.close();
	m_serverIP.clear();
	m_bytesUploaded = 0;
	m_uploadFileSize = 0;
	m_bytesLeftToUpload = 0;

	stateSet(eUPLOAD_IDLE);
	return hResult;
}

/*!
 * \brief Initializes file for writing and opens download socket.
 *
 */

stiHResult CstiFileUploadHandler::uploadInitialize()
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::UploadInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	time_t seconds = time(nullptr);

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::UploadInitialize\n");
	); // stiDEBUG_TOOL

	// Open the file and set up upload variables.
	stiTESTCOND(!m_fileName.empty(), stiRESULT_INVALID_PARAMETER);
	m_uploadFile.open(m_filePathName.c_str(), std::ios_base::in | std::ios_base::binary);

	if (m_uploadFile.fail())
	{
		m_uploadFile.clear();
		stiTHROW (stiRESULT_ERROR);
	}

	// How much data do we have to upload
	m_uploadFile.seekg(0, std::ios::end);
	m_uploadFileSize = m_uploadFile.tellg();
	m_uploadFile.seekg(0, std::ios::beg);

	// Initialize the HTTPService.
	if (!m_httpService)
	{
		m_httpService = new CstiHTTPService;
		stiTESTCOND(m_httpService, stiRESULT_MEMORY_ALLOC_ERROR);
	}

	// Initialize SSL
	if (m_bUseHTTPS)
	{
		if (!m_pSSLCtx)
		{
			m_pSSLCtx = stiSSLCtxNew ();
			stiTESTCOND(m_pSSLCtx, stiRESULT_MEMORY_ALLOC_ERROR);
		}
		m_httpService->SSLCtxSet (m_pSSLCtx);
	}

	// Look up the ip address of the Server
	hResult = stiDNSGetHostByName (m_server.c_str(),
								   nullptr,
								   &m_serverIP);
	stiTESTRESULT();

	m_httpService->ServerAddressSet(m_server.c_str(), m_serverIP.c_str (), 80);

	m_uploadHeader.str("");
	m_uploadHeader.clear();
	m_uploadHeader << "POST /" << m_fileInfo << " HTTP/1.1\r\n"
				   << "Content-Type: multipart/form-data; boundary=----------" << seconds << "\r\n" 
#ifdef stiMVRS_APP
				   << "Cache-Control: no-cache\r\n"
#endif
				   << "Host: " << m_server << "\r\n"
				   << "Transfer-Encoding: chunked\r\n"
#ifndef stiMVRS_APP
				   << "Expect: 100-continue\r\n"
#endif
				   << "Connection: Keep-Alive\r\n" 
				   << "\r\n";

	m_uploadContentHeader.str("");
	m_uploadContentHeader.clear();
	m_uploadContentHeader << "------------" << seconds << "\r\n"
						  << "Content-Disposition: form-data; name=\"uploadForm\"; filename=\"" << m_fileName << "\"\r\n"
						  << "Content-Type: application/octet-stream\r\n"
						  << "\r\n";

	m_uploadEnd.str("");
	m_uploadEnd.clear();
	m_uploadEnd << "------------" << seconds << "\r\n\r\n"; 

	// Iniialize upload tracking variables.
//	m_un64UploadFileSize = m_pHTTPService->ContentRangeTotalGet ();

	m_bytesUploaded = 0;

	m_bytesLeftToUpload = m_uploadFileSize;

	if (!m_uploadBuffer)
	{
		// We need one additional byte for the null terminator
		m_uploadBuffer = new char[UPLOAD_BUFFER_SIZE];
		stiTESTCOND (m_uploadBuffer, stiRESULT_ERROR);
	}
	memset(m_uploadBuffer, 0xff, UPLOAD_BUFFER_SIZE);

	stateSet(IstiFileUploadHandler::eUPLOAD_INITIALIZED);

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		errorStateSet(eERROR_INITIALIZING_UPLOAD);
		stateSet(eUPLOAD_ERROR);
	}

	return hResult;
}

/*!
 * \brief EventUploadNextChunk 
 * 
 */
void CstiFileUploadHandler::eventUploadNextChunk () 
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::UploadNextChunkHandler);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = -1;
	char *uploadBuffer = m_uploadBuffer;
	uint32_t contentLength = 0;
	uint32_t dataLeftToUpload = std::min((int)(m_uploadFileSize - m_bytesUploaded), MAX_UPLOAD_BUFFER_LENGTH);
	uint32_t uploadDataSize = dataLeftToUpload < MAX_UPLOAD_BUFFER_LENGTH ? dataLeftToUpload : MAX_UPLOAD_BUFFER_LENGTH;

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::UploadNextChunkHandler\n");
	); // stiDEBUG_TOOL


	switch (m_eState)
	{
	case eUPLOAD_INITIALIZED:
		{
		// Initialize the socket.
		hResult = m_httpService->SocketOpen();
		stiTESTRESULT();

		// Get the socket to write to.
		m_uploadSocket = m_httpService->SocketGet();

		// Write the upload header
		nResult = stiOSWrite (m_uploadSocket, m_uploadHeader.str().c_str(), (size_t)m_uploadHeader.str().length());
		stiTESTCOND (nResult != -1, stiRESULT_ERROR);

		// Check to see that we are OK to continue.
#ifndef stiMVRS_APP
		// Expect: 100-continue is not received through AT&T and Sprint cellular networks.
		{
			SHTTPResult stResult;
			hResult = HttpHeaderRead(m_uploadSocket, nREAD_TIMEOUT, 
									 uploadBuffer, MAX_UPLOAD_BUFFER_LENGTH - 1, 
									 nullptr, &stResult);
			stiTESTRESULT();

			// Look for a return code of 100 to indicate that we can continue uploading.
			if (stResult.unReturnCode != 100)
			{
				stiTHROW (stiRESULT_ERROR);
			}
		}
#endif
		// This variable is needed so that this code will compile on both the VP2 and VP3/4
		long unsigned int contentHeaderLen = m_uploadContentHeader.str().length();

		// Send content request.
		sprintf(m_uploadBuffer, "%lx\r\n%s\r\n", contentHeaderLen, m_uploadContentHeader.str().c_str());
		nResult = stiOSWrite (m_uploadSocket, m_uploadBuffer, strlen (m_uploadBuffer));
		stiTESTCOND (nResult != -1, stiRESULT_ERROR);

		// Now we can start uploading the file.
		stateSet(eUPLOADING);

		PostEvent([this]{eventUploadNextChunk();});
		break;
		}

	case eUPLOADING:
		// Load the data to upload into the upload buffer.
		sprintf(m_uploadBuffer, "%x\r\n", uploadDataSize);
		contentLength = strlen(uploadBuffer);
		m_uploadFile.read(&m_uploadBuffer[contentLength], uploadDataSize);
		sprintf(&m_uploadBuffer[contentLength + uploadDataSize], "\r\n");

		// Write the data.
		nResult = stiOSWrite (m_uploadSocket, uploadBuffer, contentLength + uploadDataSize + 2);
		stiTESTCOND (nResult != -1, stiRESULT_ERROR);

		m_bytesUploaded += uploadDataSize;
		fileUploadProgressSignal.Emit((unsigned int)((double)m_bytesUploaded / (double)m_uploadFileSize * 100));

		if (m_bytesUploaded == m_uploadFileSize)
		{
			stateSet(eUPLOAD_COMPLETE);
		}

		PostEvent([this]{eventUploadNextChunk();});
		break;

	case eUPLOAD_COMPLETE:
		{
		// This variable is needed so that this code will compile on both the VP2 and VP3/4
		long unsigned int uploadEndLen = m_uploadEnd.str().length();

		sprintf(m_uploadBuffer, "%lx\r\n\r\n%s0\r\n\r\n", uploadEndLen, m_uploadEnd.str().c_str());
		nResult = stiOSWrite (m_uploadSocket, m_uploadBuffer , strlen (m_uploadBuffer));
		stiTESTCOND (nResult != -1, stiRESULT_ERROR);

		SHTTPResult stResult;
		hResult = HttpHeaderRead(m_uploadSocket, nREAD_TIMEOUT, 
								 m_uploadBuffer, MAX_UPLOAD_BUFFER_LENGTH, 
								 nullptr, &stResult);
		stiTESTRESULT();
		
		if (stResult.unReturnCode < 200 || stResult.unReturnCode >= 300)
		{
			stiTHROW (stiRESULT_ERROR);
		}

#ifdef stiMVRS_APP
		if (0 == stResult.unContentLen)
		{
			// Body length may have been calculated incorrectly.  Recalculate.
			stResult.szBody = m_uploadBuffer + stResult.unHeaderLen;
			size_t calcLen = (stResult.szBody != NULL) ? strlen (stResult.szBody) : 0;
			if (calcLen > 0)
			{
				stResult.unContentLen = calcLen;
			}
		}
#endif
		// If there was not an error then get the messageId from the body of the return.
		stiTESTCOND (0 < stResult.unContentLen, stiRESULT_ERROR);

		// Get the message ID from the returned data.
		char *headerBuffer = &m_uploadBuffer[stResult.unHeaderLen];

		char *cr = strchr(headerBuffer, '\r');
		stiTESTCOND (cr, stiRESULT_ERROR);
		*cr = '\0';

		std::string messageId = headerBuffer;

		fileUploadCompleteSignal.Emit(messageId);
		
		clearUploadSettings();
				 
		break;
		}

	case eUPLOAD_PAUSED:
	case eUPLOAD_RESUME:
	case eUPLOAD_ERROR:
	case eUPLOAD_IDLE:
	case eUPLOAD_STOP:
		break;
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		if (m_eState == eUPLOAD_INITIALIZED)
		{
			errorStateSet(eERROR_INITIALIZING_UPLOAD);
		}
		else
		{
			errorStateSet(eERROR_UPLOADING);
		}
		stateSet(eUPLOAD_ERROR);

		// If we have an error close the file and socket.
		clearUploadSettings();
	}
}

/*!
 * \brief Returns the current error state of the File Upload Handler.
 * 
 * \return One of the errors defined in the EError enum.
 */
IstiFileUploadHandler::EError CstiFileUploadHandler::errorGet () const
{
	IstiFileUploadHandler::EError eError;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eError = m_eError;

	return eError;
}

/*!
 * \brief Sets the error state of the download.  
 * 
 * \return stiHResult 
 * stiRESULT_SUCCESS, or stiRESULT_ERROR on an error 
 *  
 */
stiHResult CstiFileUploadHandler::errorStateSet (
	IstiFileUploadHandler::EError errorState)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_eError = errorState;

	return hResult;
}

/*!
 * \brief Starts the the upload of fileName to URL
 *  
 * \param server is the ip address of the upload server.
 * \param fileName is the name of the file to be uploaded.
 * \param filePath is the location of the file to be uploaded.
 * \param fileUploadInfo contains info about the upload file.
 * 
 * \return stiRESULT_SUCCESS  
 */
stiHResult CstiFileUploadHandler::start (
		const std::string server,
		const std::string fileName,
		const std::string filePath,
		const std::string fileUploadInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiFileUploadHandler::Start);

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::Start\n");
	); // stiDEBUG_TOOL

	// If we are not in an idle state then we are already downloading so don't start
	// another download.
	stiTESTCOND (stateGet() == IstiFileUploadHandler::eUPLOAD_IDLE || 
				 stateGet() == IstiFileUploadHandler::eUPLOAD_COMPLETE || 
				 stateGet() == IstiFileUploadHandler::eUPLOAD_ERROR ||
				 stateGet() == IstiFileUploadHandler::eUPLOAD_STOP,
				 stiRESULT_ERROR);
	stiTESTCOND (!server.empty() && !fileName.empty() && !filePath.empty() && !fileUploadInfo.empty(), stiRESULT_INVALID_PARAMETER);

	PostEvent([this, server, fileName, filePath, fileUploadInfo]{eventStartUpload(server, fileName, filePath, fileUploadInfo);});

STI_BAIL:

	return hResult;
}

void CstiFileUploadHandler::eventStartUpload(
	const std::string server, 
	const std::string fileName, 
	const std::string filePath, 
	const std::string fileUploadInfo)
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::EventStartUpload);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::EventStartUpload\n");
	); // stiDEBUG_TOOL

	// Clear the error State.
	errorStateSet(eERROR_NONE);
				                            
	m_server = server;
	m_fileName = fileName;
	m_filePathName = filePath;
	m_filePathName += "/" + fileName;
	m_fileInfo = fileUploadInfo;
	
	// Open file and http socket.
	hResult = uploadInitialize();
	stiTESTRESULT ();

	// Start the upload.
	PostEvent([this]{eventUploadNextChunk();});

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		fileUploadErrorSignal.Emit();
	}
}

/*!
 * \brief Returns the current state of the File Upload Handler.
 * 
 * \return One of the states defined in the EState enum.
 */
IstiFileUploadHandler::EState CstiFileUploadHandler::stateGet () const
{
	IstiFileUploadHandler::EState eState;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eState = m_eState;

	return eState;
}

void CstiFileUploadHandler::stateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::stateSet);

	// Update the state.
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	m_eState = eState;

	// Switch on the new state...
	switch (eState)
	{
	case eUPLOAD_IDLE:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eIDLE\n");
		); // stiDEBUG_TOOL

		break;
	} // end case eIDLE

	case eUPLOADING:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eUPLOADING\n");
		); // stiDEBUG_TOOL

		break;
	} // end case eUPLOADING

	case eUPLOAD_PAUSED:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eUPLOAD_PAUSED\n");
		); // stiDEBUG_TOOL

		break;
	} // end case eUPLOAD_PAUSED

	case eUPLOAD_COMPLETE:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eUPLOAD_COMPLETE\n");
		); // stiDEBUG_TOOL

		break;
	}

	case eUPLOAD_ERROR:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eUPLOAD_ERROR\n");
		); // stiDEBUG_TOOL

		fileUploadErrorSignal.Emit();

		clearUploadSettings();

		break;
	}

	case eUPLOAD_STOP:
	{
		stiDEBUG_TOOL (g_stiFileUploadDebug,
			stiTrace ("CstiFileUploadHandler::StateSet - Changing state to eUPLOAD_STOP\n");
		); // stiDEBUG_TOOL

		break;
	}

	case eUPLOAD_RESUME:
	case eUPLOAD_INITIALIZED:

		break;

	} // end switch

} 


stiHResult CstiFileUploadHandler::startup ()
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::Startup);

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::Startup\n");
	); // stiDEBUG_TOOL

	CstiEventQueue::StartEventLoop();

	Initialize();

	return stiRESULT_SUCCESS;
}

/*!
 * \brief Stops the download and removes the file.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileUploadHandler::stop ()
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::Stop);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::Stop\n");
	); // stiDEBUG_TOOL

	return hResult;
}

/*!
 * \brief Enables or disables HTTPS.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileUploadHandler::useHTTPS (EstiBool useHTTPS)
{
	stiLOG_ENTRY_NAME (CstiFileUploadHandler::UseHTTPS);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileUploadDebug,
		stiTrace ("CstiFileUploadHandler::UseHTTPS\n");
	); // stiDEBUG_TOOL

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_bUseHTTPS = useHTTPS;

	return (hResult);
}


ISignal <>& CstiFileUploadHandler::fileUploadErrorSignalGet ()
{
	return fileUploadErrorSignal;
}


ISignal <std::string>& CstiFileUploadHandler::fileUploadCompleteSignalGet ()
{
	return fileUploadCompleteSignal;
}


ISignal <unsigned int>& CstiFileUploadHandler::fileUploadProgressSignalGet ()
{
	return fileUploadProgressSignal;
}

