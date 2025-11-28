////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CDataTransferHandler
//
//  File Name:	CDataTransferHandler.cpp
//
//	Abstract: A MP4 movie data handler which is responsible to download and
// 			  upload movie data from a server.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#if defined(stiLINUX) || defined(WIN32)
#include "stiTools.h"
#include <sstream>
#endif

#include <cctype> // for isdigit
#include <cerrno>
#include "stiTrace.h"
#include "CDataTransferHandler.h"
#include "CstiHTTPService.h"
#include "stiTaskInfo.h"
#include "CstiIpAddress.h"
#include "Poco/Net/SSLException.h"
#include "Poco/Net/HTMLForm.h"
#include "CstiRegEx.h"
#include "stiRemoteLogEvent.h"
#include "IstiNetwork.h"

#ifdef WIN32
#define close stiOSClose
#endif

//
// Constants
//
static const float fDATA_RATE_UPDATE_INTERVAL = 0.2F;
static const uint32_t un32FILE_CHUNK_SIZE = 524288;
static const uint32_t un32SKIP_FILE_CHUNK_SIZE = 1048576;
static const uint32_t un32HEADER_BUFFER_LENGTH = 1000;
static const uint32_t MAXIMUM_DOWNLOAD_READ = 48000U;
static const unsigned unDOWNLOAD_TIMEOUT = 90;  // Number of seconds to wait before determining that connection has been lost.
static const int g_nSOCKET_DOWNLOAD_TIMEOUT = 60; ///< Socket timeout value in seconds
static const unsigned unMAX_UPLOAD_SIZE = 5120; //10K
static const int DEFAULT_DATA_TIMER_TIMEOUT = 10;

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//#define stiBUFFERING_DEBUG
#ifdef stiBUFFERING_DEBUG
EstiBool g_bStartPlaying = estiFALSE;
#endif

EstiBool g_bIsATT3G = estiFALSE;

//
// Locals
//


//
// Namespace Declarations
//
using namespace Poco::Net;

///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::CDataTransferHandler
//
//  Description: Class constructor.
//
//  Abstract:
//
//  Returns: None.
//
CDataTransferHandler::CDataTransferHandler ()
	:
	CstiEventQueue("CDataTransferHandler"),
 	m_dataTransferTimer(DEFAULT_DATA_TIMER_TIMEOUT, this)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::CDataTransferHandler);

// Do I need this?
	m_dataTransferSignalConnections.push_back(m_dataTransferTimer.timeoutSignal.Connect (
		[this]{
			dataProcess();
		}));

} // end CDataTransferHandler::CDataTransferHandler


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::~CDataTransferHandler
//
//  Description: Class destructor.
//
//  Abstract:
//
//  Returns: None.
//
CDataTransferHandler::~CDataTransferHandler ()
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::~CDataTransferHandler);

	m_dataTransferSignalConnections.clear();

} // end CDataTransferHandler::~CDataTransferHandler


stiHResult CDataTransferHandler::Initialize (
	CVMCircularBuffer *pCircularBuf)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace ("<CDataTransferHandler::Init>\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	stiTESTCOND(pCircularBuf != nullptr, stiRESULT_ERROR);

	m_pCircBuf = pCircularBuf;

	m_dataTransferTimer.start();

STI_BAIL:

	return (hResult);	
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::Startup
//
// Abstract:
//
// Returns:
//
stiHResult CDataTransferHandler::Startup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool started = false;

	started = CstiEventQueue::StartEventLoop();
	stiTESTCOND (started, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::InitializeDataRateTracking
//
// Description:
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, otherwise stiRESULT_ERROR.
//
stiHResult CDataTransferHandler::InitializeDataRateTracking()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Update m_un32DataRate at each fDATA_RATE_UPDATE_INTERVAL seconds.
	//
	m_un32TicksPerSecond = stiOSSysClkRateGet ();
	m_un32LastUpdateTicks = stiOSTickGet ();
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("m_un32LastUpdateTicks = %u\n", m_un32LastUpdateTicks);
	);

	m_un32NextUpdateTicks = m_un32LastUpdateTicks + (uint32_t)((float)m_un32TicksPerSecond * fDATA_RATE_UPDATE_INTERVAL);
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("m_un32NextUpdateTicks = %u\n", m_un32NextUpdateTicks);
	);
								
	m_bytesDuringLastInterval = 0;
	m_un32DataRate = 1;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::UpdateDataRate
//
// Description:
//
// Abstract:
//
// Returns:
//	estiTRUE if data rate was updated, estiFALSE otherwise.
//
bool CDataTransferHandler::UpdateDataRate ()
{
	EstiBool bUpdate = estiTRUE;
	uint32_t un32CurrentTicks = 0;

	// Is it time to update the data rate measurement?
	//
	un32CurrentTicks = stiOSTickGet ();
	/*								
	stiDEBUG_TOOL (g_stiMP4FileDebug,
	stiTrace("nCurrentTicks = %u\n", un32CurrentTicks);
	stiTrace("m_un32NextUpdateTicks = %u\n", m_un32NextUpdateTicks);
	);
	*/								
	if (un32CurrentTicks > m_un32NextUpdateTicks)
	{
		//
		// bytes/second = bytes/ticks * ticks/second
		//
		uint32_t un32DataRate = (m_bytesDuringLastInterval * m_un32TicksPerSecond) / (un32CurrentTicks - m_un32LastUpdateTicks);
		if (1 == m_un32DataRate)
		{
			m_un32DataRate = un32DataRate;
		}

		uint32_t un32OldDataRate = m_un32DataRate;

		m_un32DataRate = (2 * m_un32DataRate + un32DataRate) / 3; // smoothing data rate variation

		// Check to make sure we didn't get a data spike if we did then pull it back so we don't jump out of
		// buffering to early.
		if (un32OldDataRate * 1.5 < m_un32DataRate)
		{
			m_un32DataRate = static_cast<uint32_t>(1.5 * un32OldDataRate);
		}

		// If the download rate is less than 512K then tell FilePlayer that it is less than it 
		// actually is so that we can try and compensate for dead spots in the download.
		if (m_un32DataRate < 64000)
		{
			m_un32DataRate = static_cast<uint32_t>(m_un32DataRate * 0.80F);
		}

		/*									
		stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("m_bytesDuringLastInterval = %u\n", m_bytesDuringLastInterval);
		stiTrace("m_un32TicksPerSecond = %u\n", m_un32TicksPerSecond);
		stiTrace("un32CurrentTicks = %u\n", un32CurrentTicks);
		stiTrace("m_un32LastUpdateTicks = %u\n", m_un32LastUpdateTicks);
		stiTrace("------------new m_un32DataRate = %u----------------\n\n\n\n", m_un32DataRate);
		);
		*/
		//
		// Reset variables
		//
		m_bytesDuringLastInterval = 0;
		m_un32LastUpdateTicks = un32CurrentTicks;
		/*
		stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("m_un32LastUpdateTicks = %u\n", m_un32LastUpdateTicks);
		);
		*/
		m_un32NextUpdateTicks = un32CurrentTicks + (uint32_t)((float)m_un32TicksPerSecond * fDATA_RATE_UPDATE_INTERVAL);
		/*
		stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("m_un32NextUpdateTicks = %u\n", m_un32NextUpdateTicks);
		);
		*/		
	}
	else
	{
		bUpdate = estiFALSE;
	}

	return bUpdate;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::UploadMovieInfoSet
//
// Description: Initialize the ServerDataHandler to upload data.
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
stiHResult CDataTransferHandler::UploadMovieInfoSet(
	const std::string &server,
	const std::string &uploadGUID,
	const std::string &uploadFileName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	stiTESTCOND (!uploadFileName.empty(), stiRESULT_ERROR);

	m_uploadFileName = uploadFileName;

	if (!server.empty() && 
		!uploadGUID.empty())
	{
		m_server = server;
		m_fileGUID = uploadGUID;

		m_eState = estiUPLOADING;

		// Set the transfer type as continuous upload.
		m_eTransferType = eUPLOAD_CONTINUOUS;
	}
	else
	{
		m_server.clear();
		m_fileGUID.clear();
		m_eState = estiHOLDING;
	}

STI_BAIL:	

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::UploadOptimizeMovieInfoSet
//
// Description: Initialize the TransferDataHandler to upload data.
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
stiHResult CDataTransferHandler::UploadOptimizedMovieInfoSet(
	const std::string &server,
	const std::string &uploadGUID,
	uint32_t un32Width,
	uint32_t un32Height,
	uint32_t un32Bitrate,
	uint32_t un32Duration)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32TotalData = 0;
	uint32_t un32Bytes = 0; 

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Reset the buffer pointers so the file can be uploaded 
	// they where modified when the MOOV atom was written out.
	m_pCircBuf->ConfigureBufferForUpload();
	m_un64CurrentOffset = 0;
	un32Bytes = m_pCircBuf->GetDataSizeInBuffer(static_cast<uint32_t>(m_un64CurrentOffset), &un32TotalData);
	stiTESTCOND (un32Bytes > 0, stiRESULT_ERROR);

	stiTESTCOND (!server.empty() && !uploadGUID.empty(), stiRESULT_ERROR);

	m_server = server;
	m_fileGUID = uploadGUID;

	m_un32Width = un32Width;
	m_un32Height = un32Height;
	m_un32Bitrate = un32Bitrate;
	m_un32Duration = un32Duration;

	m_eState = estiUPLOADING;

	// Set the transfer type as file upload.
	m_eTransferType = eUPLOAD_FILE;

STI_BAIL:	

	if (stiIS_ERROR(hResult))
	{
		uploadErrorSignal.Emit();
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::UploadProgressUpdate
//
// Description: Updates the upload procentage.
//
// Abstract:
//
// Returns: stiRESULT_SUCCESS on success.
// 			stiRESULT_ERROR on error.
//
void CDataTransferHandler::UploadProgressUpdate()
{
	uint32_t un32DataInBuffer = 0;
	uint32_t un32TotalUpload = 0;
	uint32_t uploadPercent = 0;

	m_pCircBuf->GetDataSizeInBuffer(static_cast<uint32_t>(m_un64CurrentOffset), &un32DataInBuffer);
	un32TotalUpload = static_cast<uint32_t>(m_un64CurrentOffset) + un32DataInBuffer;
	uploadPercent = static_cast<uint32_t>((m_un64CurrentOffset * 100) / un32TotalUpload);
	uploadProgressSignal.Emit(uploadPercent);
}


Poco::URI httpUploadURIGet (
	bool usingProxy,
	const std::string &server,
	const std::string &file,
	int pixelWidth,
	int pixelHeight,
	int bitrate,
	int durationSeconds)
{
	std::stringstream header;

	header << "http://" << server << "/" << file;

	if (pixelWidth > 0)
	{
		header << "&PixelWidth=" << pixelWidth;
	}

	if (pixelHeight > 0)
	{
		header << "&PixelHeight=" << pixelHeight;
	}

	if (bitrate > 0)
	{
		header << "&Bitrate=" << bitrate;
	}

	if (durationSeconds)
	{
		header << "&DurationSeconds=" << durationSeconds;
	}

	return Poco::URI (header.str ());
}


std::string httpBoundaryGet (
	int seconds)
{
	std::stringstream header;

	header << "------------" << seconds;

	return header.str ();
}


std::string httpContentDispositionGet (
	const std::string &filename)
{
	std::stringstream header;

	header << "Content-Disposition: form-data; name=\"uploadForm\"; filename=\"" << filename << "\"\r\n";

	return header.str ();
}


std::string httpContentTypeGet (
	const std::string &contentType)
{
	std::stringstream header;

	header << "Content-Type: " << contentType << "\r\n";

	return header.str ();
}


std::string httpCacheControlGet ()
{
	std::stringstream header;

	header << "Cache-Control: no-cache\r\n";

	return header.str ();
}


std::string httpTransferEncodingGet ()
{
	std::stringstream header;

	header << "Transfer-Encoding: chunked\r\n";

	return header.str ();
}


std::string httpExpectGet ()
{
	std::stringstream header;

	header << "Expect: 100-continue\r\n";

	return header.str ();
}


Poco::URI httpDownloadURIGet (
	const std::string &server,
	const std::string &file,
	int downloadSpeed)
{
	std::stringstream header;

	header << "https://" << server << "/" << file;

	if (downloadSpeed > 0)
	{
		header << "&downloadSpeed=" << downloadSpeed;
	}

	return Poco::URI (header.str ());
}


std::string httpRangeHeaderGet (
	uint64_t rangeStart,
	uint64_t rangeEnd)
{
	std::stringstream header;

	header << "bytes=" << rangeStart << "-" << rangeEnd;

	return header.str ();
}


std::string httpClientTokenGet (
	const std::string &clientToken)
{
	std::stringstream header;

	if (!clientToken.empty())
	{
		header << "ClientToken: " << clientToken << "\r\n";
	}

	return header.str ();
}


std::string httpHostGet (
	const std::string &host)
{
	std::stringstream header;

	header << "Host: " << host << "\r\n";

	return header.str ();
}


std::string httpAcceptGet ()
{
	std::stringstream header;

	header << "Accept: */*\r\n";

	return header.str ();
}


std::string httpConnectionGet (
	const std::string &connectionType)
{
	std::stringstream header;

	header << "Connection:" << connectionType << "\r\n";

	return header.str ();
}


std::string httpEndGet ()
{
	std::stringstream header;

	header << "\r\n";

	return header.str ();
}


std::unique_ptr<HTTPRequest> CDataTransferHandler::getDownloadRequest (
	uint64_t rangeStart,
	uint64_t rangeEnd)
{
	auto downloadURI = httpDownloadURIGet (m_server, m_fileGUID, m_un32DownloadSpeed);
	auto requestHeader = sci::make_unique<HTTPRequest> (HTTPRequest::HTTP_GET, downloadURI.getPathAndQuery (), HTTPMessage::HTTP_1_1);
	auto timeout = m_clientSession->getKeepAliveTimeout ();
	requestHeader->setHost (m_server);
	requestHeader->set ("Accept", "*/*");
	if (!m_clientToken.empty ())
	{
		requestHeader->set ("ClientToken", m_clientToken);
	}
	requestHeader->set ("Range", httpRangeHeaderGet (rangeStart, rangeEnd));
	return requestHeader;
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::Upload
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK if successful, otherwise estiERROR.
//
stiHResult CDataTransferHandler::Upload (bool allowDataRateUpdate)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::Upload);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace ("<CDataTransferHandler::Upload> Entry.\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Bytes;
	uint8_t *pDataBuffer;
	uint32_t un32BytesAvailable = 0;

	// Upload the next chunk of data.
	un32Bytes = m_pCircBuf->GetDataSizeInBuffer (static_cast<uint32_t>(m_un64CurrentOffset));
	un32Bytes = un32Bytes < unMAX_UPLOAD_SIZE ? un32Bytes : unMAX_UPLOAD_SIZE;

	if (un32Bytes > 0)
	{
		CVMCircularBuffer::EstiBufferResult bufResult = m_pCircBuf->GetBuffer (static_cast<uint32_t>(m_un64CurrentOffset),
			un32Bytes,
			&pDataBuffer);
		if (bufResult != CVMCircularBuffer::estiBUFFER_OK)
		{
			un32Bytes = m_pCircBuf->GetDataSizeInBuffer (static_cast<uint32_t>(m_un64CurrentOffset));
			CVMCircularBuffer::EstiBufferResult bufResult = m_pCircBuf->GetBuffer (static_cast<uint32_t>(m_un64CurrentOffset),
				un32Bytes,
				&pDataBuffer);
			stiTESTCOND (bufResult == CVMCircularBuffer::estiBUFFER_OK, stiRESULT_ERROR);
		}

		// Make sure that we have a socket.  If not we haven't started the 
		// upload yet so send the message.
		if (!m_clientSession)
		{
			hResult = StartUpload ();
			stiTESTRESULT ();
		}
		stiTESTCONDMSG (m_clientSession->networkException() == nullptr, stiRESULT_ERROR,
			"EventType=Error \nException: %s\nun32Bytes=%d",
			m_clientSession->networkException()->displayText().c_str(),
			un32Bytes
		);

		stiTESTCOND(m_clientSession->connected(), stiRESULT_ERROR);

		m_requestBody->write (reinterpret_cast<char *>(pDataBuffer), un32Bytes);

		// Release the data in the buffer.
		hResult = m_pCircBuf->ReleaseBuffer (static_cast<uint32_t>(m_un64CurrentOffset), un32Bytes);
		m_un64CurrentOffset += un32Bytes;

		m_bytesDuringLastInterval += un32Bytes;
	}

	if (m_eTransferType == eUPLOAD_FILE)
	{
		UploadProgressUpdate ();

		// If the buffer is empty then we finished uploading the file notify FilePlay.
		if (m_pCircBuf->IsBufferEmpty ())
		{
			std::string responseBody;
			hResult = getHTTPResponse (responseBody);
			stiTESTRESULT ();

			m_eState = estiUPLOAD_COMPLETE;
			uploadCompleteSignal.Emit();
		}
	}

	if (allowDataRateUpdate && UpdateDataRate ())
	{
		// To get the space available in the circular buffer we need to pass in a request
		// for at lease 1 byte (thus the 1 in this function call).
		m_pCircBuf->IsBufferSpaceAvailable (1, &un32BytesAvailable);
		SFileUploadProgress uploadProgress;
	
		uploadProgress.m_bufferBytesAvailable = un32BytesAvailable;
		uploadProgress.un32DataRate = m_un32DataRate;
	
		bufferUploadSpaceUpdateSignal.Emit(uploadProgress);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		m_abortTransfer = true;

		// Notify the FilePlayer that we had an error.
		uploadErrorSignal.Emit();

		// Stop the Task() from calling this function.
		m_eState = estiUPLOAD_ERROR;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::StartUpload
//
// Description: Initializes members and starts the process of getting data
// 				that is in the future.
//
// Abstract:
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
stiHResult CDataTransferHandler::StartUpload ()
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::EventStartUpload);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace ("<CDataTransferHandler::StartUpload> Entry\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nSelectRet = 0; stiUNUSED_ARG (nSelectRet);
	time_t seconds = time(nullptr);

	// Reset the AbortTransfer variable.
	m_abortTransfer = false;

	if (!m_clientSession)
	{
		unsigned short port = STANDARD_HTTPS_PORT;

		//// Create the post end statement.
		std::string boundary = httpBoundaryGet (static_cast<int>(seconds));
		m_uploadEndBoundary.clear ();
		m_uploadEndBoundary << boundary << httpEndGet ();

		std::stringstream contentHeaders;
		contentHeaders << boundary << httpEndGet ()
			<< httpContentDispositionGet (m_uploadFileName)
			<< httpContentTypeGet ("application/octet-stream")
			<< httpEndGet ();

		m_un64CurrentOffset = 0;
		try
		{
			Poco::URI uploadURI;
			if (m_eTransferType == eUPLOAD_FILE)
			{
				uploadURI = httpUploadURIGet (false, m_server, m_fileGUID, m_un32Width, m_un32Height, m_un32Bitrate, m_un32Duration);
			}
			else
			{
				uploadURI = httpUploadURIGet (false, m_server, m_fileGUID, 0, 0, 0, 0);
			}

			m_clientSession = sci::make_unique<HTTPSClientSession> (m_server, port);
			m_clientSession->setKeepAlive (true);
			HTTPRequest requestHeader{ HTTPRequest::HTTP_POST, uploadURI.getPathAndQuery (), HTTPMessage::HTTP_1_1 };
			requestHeader.setChunkedTransferEncoding (true);
			requestHeader.setExpectContinue (true);
			requestHeader.setKeepAlive (true);
			requestHeader.setHost (m_server);
			requestHeader.setContentType (std::string ("multipart/form-data; boundary=----------") + std::to_string (seconds));
			auto &requestBody = m_clientSession->sendRequest (requestHeader);
			m_requestBody = sci::make_unique<std::ostream> (requestBody.rdbuf ());
			stiTESTCONDMSG (m_clientSession && m_clientSession->networkException () == nullptr, stiRESULT_ERROR,
				"EventType=Error \nException: %s",
				m_clientSession ? m_clientSession->networkException()->displayText().c_str() : "No m_clientSession"
			);
			m_response = sci::make_unique<HTTPResponse> ();
			m_clientSession->peekResponse (*m_response);

			stiTESTCONDMSG (m_clientSession && m_clientSession->networkException () == nullptr, stiRESULT_ERROR,
				"EventType=Error \nException: %s",
				m_clientSession ? m_clientSession->networkException()->displayText().c_str() : "No m_clientSession"
			);
			(*m_requestBody) << std::hex << contentHeaders.str ().size () << httpEndGet () << contentHeaders.str ();

			InitializeDataRateTracking ();

		}
		catch (Poco::Net::SSLException &e)
		{
			stiTHROWMSG (stiRESULT_ERROR, "StartUpload: SSLException: %s", e.what ());
		}
		catch (std::exception &e)
		{
			stiTHROWMSG (stiRESULT_ERROR, "StartUpload: Exception: %s", e.what ());
		}
	}

STI_BAIL:	

	// If we have failed to get a good connection to the HTTP server, or if an HTTP or
	// application error has been returned, inform the FilePlay class of the error.
	if (stiIS_ERROR (hResult))
	{
		cleanupSocket();
	}  

	return hResult;
}


void CDataTransferHandler::dataProcess()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	fd_set SReadFds;
	struct timeval stTimeOut{};
	struct timeval *pSTimeOut = nullptr;

	// Initialize select.  This will allow us to respond to messages and 
	// to socket connections.  Note that FD_SET must be called on each 
	// iteration as select will change the fd_set values.  FD_SET is a macro 
	// that adds a file descriptor to a file descriptor set.
	stiFD_ZERO (&SReadFds);

	if (m_eState == estiDOWNLOADING)
	{
		pSTimeOut = nullptr;
		if (!m_abortTransfer && !m_waitingForRelease)
		{
			// select on the communication socket
			if(m_clientSession)
			{
				if (m_responseBody &&            
					m_responseBody->peek() != EOF)
				{
					hResult = Download ();
					stiTESTRESULT();
				}
				else
				{
					stTimeOut.tv_sec = unDOWNLOAD_TIMEOUT;
					stTimeOut.tv_usec = 0;
					pSTimeOut = &stTimeOut;
					stiFD_SET (m_clientSession->socket ().impl ()->sockfd (), &SReadFds);

					int nSelectRet = stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, pSTimeOut);
					if (0 < nSelectRet)
					{
						// Check if ready to read from socket
						if (m_clientSession &&
							m_responseBody &&
							m_responseBody->peek() != EOF)
						{
							// Process the incoming data.
							hResult = Download ();
							stiTESTRESULT();
						}
					}
					else if (0 == nSelectRet)
					{
						// We dropped out of select due to a timeout waiting
						stiASSERT (estiFALSE);

						// Assume that the connection has been lost.
						if (m_clientSession)
						{
							cleanupSocket();
						}

						// Do we need to send an error here?
						errorSignal.Emit(stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR);
					}
				}
			}
			else if (m_un64YetToDownload > 0)
			{
				if (m_pCircBuf->IsBufferSpaceAvailable(un32FILE_CHUNK_SIZE))
				{
					hResult = FileChunkGet();
					stiTESTRESULT();
				}
				else
				{
					// If the buffer is full and we aren't ready to play yet, its because
					// we haven't gotten the atom yet, and we won't.  Assume the file is bad
					// and send an error.
					if (!m_readyToPlay)
					{
						stiASSERT (estiFALSE);
						downloadErrorSignal.Emit();
					}
				}
			}
		}
	}
	// We are not downloading but uploading.
	else if (m_eState == estiUPLOADING)
	{
		// Do we have data to send?	If the buffer is empty wait for 1 second.
		// If we don't have data we are really just waiting for a message.
		// Otherwise don't wait at all so we can send the data.
		if (!m_pCircBuf->IsBufferEmpty ())
		{
			hResult = Upload(true);
			stiTESTRESULT();
		}
	}
	else if (m_eState == estiREADFILE)
	{
		if (m_un64YetToDownload > 0 &&
			m_pCircBuf->IsBufferSpaceAvailable(un32FILE_CHUNK_SIZE))
		{
			hResult = FileChunkGet();
			stiTESTRESULT();
		}
	}

STI_BAIL:

	m_dataTransferTimer.timeoutSet(DEFAULT_DATA_TIMER_TIMEOUT);
	m_dataTransferTimer.start();
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::downloadResume
//
// Description: Restarts the download
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS on success
//

stiHResult CDataTransferHandler::downloadResume(Movie_t *movie)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this, movie]{eventDownloadResume(movie);});

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventDownloadResume
//
// Description:
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - Always
//
void CDataTransferHandler::eventDownloadResume (Movie_t *movie)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CDataTransferHandler::EventDownloadResume);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace ("<EventDownloadResume> Entry\n");
	);

	if (m_eState == estiDOWNLOAD_ERROR)
	{
		m_eState = estiDOWNLOADING;
	}

	// If yetToDownload or fileSize are 0 then we may have errored out during the
	// authentication of the file (when these variables are set).  So try restarting 
	// the download from the begining.  Otherwise pick up from where we errored.
	if (m_un64YetToDownload == 0 ||
		m_un64FileSize == 0)
	{
		hResult = FileChunkGet(movie,
							   0,
							   un32FILE_CHUNK_SIZE);
		stiTESTRESULT()
	}
	else
	{
		hResult = FileChunkGet();
		stiTESTRESULT()
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		downloadErrorSignal.Emit();
	}
} // end CDataTransferHandler::EventDownloadResume


void CDataTransferHandler::shutdown()
{

#if APPLICATION == APP_NTOUCH_VP2
	// If we have a clientSession and the network is down abort the session so that we don't
	// get stuck waiting for data to be processed in a read() or write() function. 
	if (m_clientSession.get () &&
		(IstiNetwork::InstanceGet ()->ServiceStateGet () != estiServiceOnline) &&
		(IstiNetwork::InstanceGet ()->ServiceStateGet () != estiServiceReady))
	{
		m_clientSession->abort();
	}
#endif

	PostEvent([this]{EventShutdownHandle();});
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventShutdownHandle
//
// Description: 
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, stiRESULT_ERROR if not.
//
void CDataTransferHandler::EventShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::EventShutdownHandle);
  
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::BeginShutdownHandle> Entry\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	// If we don't have a clientSession or if it isn't connected skip to cleanup.	
	stiTESTCOND(!m_clientSession.get() || m_clientSession->connected(), stiRESULT_ERROR);

	// If we are uploading a file we need to complete the upload.
	if (m_eState == estiUPLOADING)
	{
		hResult = FinishUpload();
		stiTESTRESULT();

		// Set the state to complete so that we don't try to start
		// another upload.
		m_eState = estiUPLOAD_COMPLETE;

		// Notify the fileplayer that we are at 100% on the upload.
		if (m_eTransferType == eUPLOAD_CONTINUOUS)
		{
			uint32_t uploadPercent = 100;
			uploadProgressSignal.Emit(uploadPercent);
		}

		uploadCompleteSignal.Emit();
	}

STI_BAIL:

	cleanupSocket ();

	m_abortTransfer = true;	

	shutdownCompleteSignal.Emit();
} // end CDataTransferHandler::EventShutdownHandle


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::Download
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK if successful, otherwise estiERROR.
//
stiHResult CDataTransferHandler::Download ()
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::Download);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::Download> Entry.\n");
	);
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t nBytesReceived = 0;
	uint8_t* pWriteBuffer = nullptr;
	uint32_t writeBufferSize = 0;
	EstiBool bComplete = estiFALSE;

	// Make sure we have a valid socket
	stiTESTCOND (m_clientSession, stiRESULT_ERROR);
	if (m_useRangeRequest)
	{
		if (m_eTransferType != eDOWNLOAD_SKIP_BACK)
		{
			writeBufferSize = m_pCircBuf->GetCurrentWritePosition (&pWriteBuffer);
		}
		else
		{
			writeBufferSize = m_pCircBuf->GetSkipBackWritePosition(static_cast<uint32_t>(m_un64SkipChunkStart),
															static_cast<uint32_t>(m_un64SkipChunkEnd - m_un64SkipChunkStart),
															&pWriteBuffer);
		}
	}
	else
	{
		writeBufferSize = m_pCircBuf->GetCurrentWritePosition (&pWriteBuffer);

		// If the buffer is full and we aren't ready to play yet, its becasue
		// we haven't gotten the atom yet, and we won't.  Assume the file is bad
		// and send an error.
		if (pWriteBuffer == nullptr && 
			writeBufferSize == 0 && 
			!m_readyToPlay)
		{
			stiTHROW(stiRESULT_ERROR);
		}
	}

	if (nullptr != pWriteBuffer)
	{
		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace("Getting up to %d bytes\n", writeBufferSize);
		);
		
#ifdef stiBUFFERING_DEBUG
		if (writeBufferSize > 6400)
		{
			writeBufferSize = 6400;
		}
#endif

		uint32_t bytestoRead{ 0U };
		if (m_useRangeRequest)
		{
			bytestoRead = writeBufferSize < m_un64ChunkLengthRemaining ? writeBufferSize : static_cast<uint32_t>(m_un64ChunkLengthRemaining);
		}
		else
		{
			bytestoRead = writeBufferSize < m_un64YetToDownload ? writeBufferSize : static_cast<uint32_t>(m_un64YetToDownload);
		}
		try
		{
			m_responseBody->readsome ((char *)pWriteBuffer, std::min (bytestoRead, MAXIMUM_DOWNLOAD_READ));
			nBytesReceived = static_cast<uint32_t>(m_responseBody->gcount ());
			stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);
		}
		catch (std::exception &e)
		{
			stiTHROWMSG (stiRESULT_ERROR, e.what());
		}

		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace("nBytesReceived = %d\n", nBytesReceived);
		);
		m_bytesDuringLastInterval += nBytesReceived;
		if (m_useRangeRequest)
		{
			if (m_eTransferType != eDOWNLOAD_SKIP_BACK)
			{
				m_un64YetToDownload -= (uint64_t)nBytesReceived;
			}
	
			// If DOWNLOAD_SKIP_BACK or DOWNLOAD_SKIP_FORWARD then it won't
			// be DOWNLOAD_NORMAL
			if (m_eTransferType != eDOWNLOAD_NORMAL)
			{
				m_un64SkipChunkStart += (uint64_t)nBytesReceived;
			}
	
			m_un64ChunkLengthRemaining -= (uint64_t)nBytesReceived;
		}
		else
		{
			m_un64YetToDownload -= (uint64_t)nBytesReceived;
		}
		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace("m_un64YetToDownload = %u\n", m_un64YetToDownload);
		);

		UpdateDataRate();

		// If we got something, update the Circular Buffer linked list and 
		// the File Level Linked list
		if (0 < nBytesReceived)
		{
			hResult = m_pCircBuf->InsertIntoCircularBufferLinkedList (pWriteBuffer, nBytesReceived,
																	  m_eTransferType == eDOWNLOAD_SKIP_BACK ? estiTRUE : estiFALSE);
			if (!stiIS_ERROR(hResult))
			{
				MoreDataAvailable (nBytesReceived);
			} // end if
			else
			{
				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("Insert into circ buffer failed.\n");
				);
			}
#ifdef stiBUFFERING_DEBUG
			if (!g_bStartPlaying)
			{
				stiOSTaskDelay (200);
			}
#endif
		} // end if

		if (!m_useRangeRequest && nBytesReceived <= 0)
		{
			bComplete = estiTRUE;

			// Update the maximum offset number in the circular buffer
			// This update only needs to occur if we didn't download the whole file as was expected.
			if (m_un64YetToDownload > 0)
			{
				m_pCircBuf->MaxFileOffsetSet (m_un64FileSize - m_un64YetToDownload);
			}

			cleanupSocket();
		}
		else if (m_useRangeRequest && (nBytesReceived == 0 || 
				 m_un64ChunkLengthRemaining == 0 ||
				 (m_eTransferType != eDOWNLOAD_NORMAL && 
				 m_un64SkipChunkStart == m_un64SkipChunkEnd)))
		{
			bComplete = estiTRUE;

			if (m_eTransferType != eDOWNLOAD_NORMAL)
			{
				m_un64SkipChunkStart = 0;
				m_un64SkipChunkEnd = 0;
				m_eTransferType = eDOWNLOAD_NORMAL;
			}
			cleanupSocket();

			EstiBool bBuffFull = m_pCircBuf->IsBufferSpaceAvailable(un32FILE_CHUNK_SIZE) ? estiFALSE : estiTRUE;
			uint32_t un32DataInBuffer = 0;
			m_pCircBuf->GetDataSizeInBuffer(static_cast<uint32_t>(m_un64CurrentOffset), &un32DataInBuffer);

			// Inform the app that the skip data download is complete so it can start playing.
			SFileDownloadProgress downloadProgress;

			downloadProgress.bBufferFull = bBuffFull;
			downloadProgress.un64FileSizeInBytes = m_un64FileSize;
			downloadProgress.un64BytesYetToDownload = m_un64YetToDownload;
			downloadProgress.m_bytesInBuffer = un32DataInBuffer;
			downloadProgress.un32DataRate = m_un32DataRate;
			fileDownloadProgressSignal.Emit(downloadProgress);
		}
	} // end if
	else
	{
		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace ("<CDataTransferHandler::Download> The write pointer has caught up to the "
			"read pointer.  Waiting ...\n");
		);
		
		uint32_t un32DataInBuffer = 0;
		m_pCircBuf->GetDataSizeInBuffer(static_cast<uint32_t>(m_un64CurrentOffset), &un32DataInBuffer);
			
		m_waitingForRelease = true;
		SFileDownloadProgress downloadProgress;

		downloadProgress.bBufferFull = true;
		downloadProgress.un64FileSizeInBytes = m_un64FileSize;
		downloadProgress.un64BytesYetToDownload = m_un64YetToDownload;
		downloadProgress.m_bytesInBuffer = un32DataInBuffer;
		downloadProgress.un32DataRate = m_un32DataRate;
		
		fileDownloadProgressSignal.Emit(downloadProgress);
	} // end else

	if (bComplete || m_abortTransfer)
	{
		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace ("Done downloading%s\n", m_abortTransfer ? " ... aborted" : ".");
		);
	}

STI_BAIL:
	if (stiIS_ERROR (hResult))
	{
		cleanupSocket();
		downloadErrorSignal.Emit();
	} // end if

	return hResult;
} // end CDataTransferHandler::Download


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::DownloadSpeedSet
//
// Abstract: Sets the download speed
//
// Returns: 
//
stiHResult CDataTransferHandler::DownloadSpeedSet(
	uint32_t un32DownloadSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_un32DownloadSpeed = un32DownloadSpeed;

	// If we are in a download error state then we are setting the download
	// speed to try the download again.
	if (m_eState == estiDOWNLOAD_ERROR)
	{
		m_eState = estiDOWNLOADING;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventFileChunkGet
//
// Description:
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, otherwise stiRESULT_ERROR.
//
void CDataTransferHandler::EventFileChunkGet (
	Movie_t *movie)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::EventFileGet);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::EventFileChunkGet> Entry\n");
	);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	const std::string CONTENT_RANGE_TAG = "Content-Range";
	const std::string ERROR_CODE_TAG = "Error Code";

	uint8_t *pWriteBuffer;

	// If ChunkStart and ChunkEnd are 0 then we are probably combining requests
	// so don't try to download anything.
	if (m_un64CurrentChunkStart != 0 || m_un64CurrentChunkEnd != 0)
	{
		if (movie)
		{
			// Initialize
			m_abortTransfer = false;
			m_readyToPlay = false;

			m_waitingForParsableCheck = false;
			m_un32MoreDataForParsableCheck = 0;

			m_un64CurrentOffset = 0;
			m_un64ParseOffset = 0;
			m_un64YetToDownload = 0;

			m_waitingForRelease = false;
			m_sendDownloadProgress = true;

			// If this is our first request we need to validate the request.
        	hResult = HTTPDownloadRangeRequest(0, 15);
			stiTESTRESULT();

			// If filesize is 0 then we don't know how big the file is so get it from 
			// the header.
			if (m_un64FileSize == 0)
			{
				if (m_response->has (CONTENT_RANGE_TAG))
				{
					CstiRegEx digitsRegex ("^bytes ([0-9]+)-([0-9]+)/([0-9]+)");
					auto contentRange = m_response->get (CONTENT_RANGE_TAG);
					std::vector<std::string> results;
					digitsRegex.Match (contentRange, results);
					m_un64FileSize = std::stoi (results[3]);
				}
			}

			//We are going to reuse the client so we are reseting it so we can send a new request later.  This needs to be done if you do not consume all of the response.
			//which in this case we don't want to consume the entire response.
			m_clientSession->reset ();

			// We may have just gotten the file size so check to make sure we aren't reqeusting
			// a file chunk that is larger than the file.
			if (m_un64FileSize < m_un64CurrentChunkEnd)
			{
				m_un64CurrentChunkEnd = m_un64FileSize - 1;
			}
		}

		stiDEBUG_TOOL (g_stiMP4FileDebug,
			stiTrace ("Creating socket\n");
		);
		
		hResult = HTTPDownloadRangeRequest (static_cast<uint32_t>(m_un64CurrentChunkStart), static_cast<uint32_t>(m_un64CurrentChunkEnd));
		stiTESTRESULT ();

		uint64_t contentLength{ 0 };
		uint32_t readBytes{ 0 };
		try
		{
			// Application Error Test
			if (m_response->has (ERROR_CODE_TAG))
			{
				auto errorCodeCharacters = m_response->get (ERROR_CODE_TAG);
				int errorCode = stoi (errorCodeCharacters);

				switch (errorCode)
				{
				case 2:	// Number of message accesses exceeded
				case 3: // Message was deleted and is no longer accessible
				case 5: // The file was not found on the system

					// Force an error to be logged
					stiTESTCOND (estiFALSE, stiRESULT_ERROR);
					break;

				case 4: // The messageID is invalid or missing

					// Force an error to be logged
					stiTESTCOND (estiFALSE, stiRESULT_SRVR_DATA_HNDLR_BAD_GUID);
					break;
				case 6: // An exception was thrown by the Message Server - Check server log
				default:
					// Force an error to be logged
					stiTESTCOND (estiFALSE, stiRESULT_ERROR);
					break;
				} // end switch
			}

			// Get the return code of the HTTP header.
			auto returnCode = m_response->getStatus ();

			// The return code from the HTTP server must be a 200 level code
			if (returnCode < HTTPResponse::HTTPStatus::HTTP_OK || returnCode >= HTTPResponse::HTTPStatus::HTTP_MULTIPLE_CHOICES)
			{
				if (returnCode == HTTPResponse::HTTPStatus::HTTP_SERVICE_UNAVAILABLE)
				{
					stiTHROW (stiRESULT_SERVER_BUSY);
				}
				else if (returnCode == HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST)
				{
					m_eState = estiGUID_ERROR;
					stiTHROW (stiRESULT_SRVR_DATA_HNDLR_BAD_GUID);
				}	
				// If we get a 404 error at this point it is probably because the GUID has expired
				// and we need a new one.  If we get a second guid error then the file has probably
				// been deleted, so we just need to error out.
   				else if (returnCode == HTTPResponse::HTTPStatus::HTTP_NOT_FOUND)
				{
					if (m_eState == estiGUID_EXPIRED)
					{
						// Second try so file is bad.
						m_eState = estiDOWNLOAD_ERROR;
						stiTHROW (stiRESULT_ERROR);
					}
					else
					{
						m_eState = estiGUID_EXPIRED;
						m_fileGUID.clear();

						stiTHROW (stiRESULT_SRVR_DATA_HNDLR_EXPIRED_GUID);
					}
				}
				else
				{
					stiTHROW (stiRESULT_ERROR);
				}
			}

			// If we made it here and we are in a expired GUID state we now have a
			// valid GUID so reset the state back to downloading.
			if (m_eState == estiGUID_EXPIRED)
			{
				m_eState = estiDOWNLOADING;
			}

			contentLength = m_response->getContentLength64 ();

		   // There must have been data returned in the content too
		   stiTESTCOND (0 < contentLength, stiRESULT_ERROR);

		   // Loop through and add to the write buffer
			if (m_eTransferType == eDOWNLOAD_SKIP_BACK)
			{
				// If looking for a prebuffer specify where to start and how big it should be.
				auto availableBytes = m_pCircBuf->GetSkipBackWritePosition(static_cast<uint32_t>(m_un64SkipChunkStart),
																		   static_cast<uint32_t>(m_un64SkipChunkEnd - m_un64SkipChunkStart),
																		   &pWriteBuffer);
				auto byteToRead = std::min (availableBytes, un32HEADER_BUFFER_LENGTH);
				// Make sure we have a valid write buffer
				stiTESTCOND (nullptr != pWriteBuffer, stiRESULT_ERROR);

				if (!m_responseBody->eof ())
				{
					readBytes = static_cast<uint32_t>(m_responseBody->readsome ((char *)pWriteBuffer, byteToRead));
					m_pCircBuf->InsertIntoCircularBufferLinkedList (pWriteBuffer, readBytes, estiTRUE);
					auto networkException = m_clientSession->networkException ();
					stiTESTCOND (networkException == nullptr, stiRESULT_ERROR);
				}
			}
			else
			{
				if (m_responseBody && !m_responseBody->eof ())
				{
					auto availableBytes = m_pCircBuf->GetCurrentWritePosition (&pWriteBuffer);
					auto byteToRead = std::min (availableBytes, un32HEADER_BUFFER_LENGTH);

					// If we didn't get a pointer back from the buffer then we have a big problem.
					stiTESTCOND (pWriteBuffer, stiRESULT_ERROR);
					readBytes = static_cast<uint32_t>(m_responseBody->readsome ((char *)pWriteBuffer, byteToRead));
					m_pCircBuf->InsertIntoCircularBufferLinkedList (pWriteBuffer, readBytes, estiFALSE);
					auto networkException = m_clientSession->networkException ();
					stiTESTCOND (networkException == nullptr, stiRESULT_ERROR);
				}

				if (0 == m_un64YetToDownload)
				{
					m_un64YetToDownload = m_un64FileSize - readBytes;
				}
				else
				{
					m_un64YetToDownload -= readBytes;
				}
			}

			MoreDataAvailable (static_cast<uint32_t>(readBytes));
			m_un64ChunkLengthRemaining = contentLength - readBytes;

			// Set the maximum offset number in the circular buffer
			m_pCircBuf->MaxFileOffsetSet (m_un64FileSize);

			// If the download type is not equal to eDOWNLOAD_NORMAL then it is skipping back or skipping forward.
			if (m_eTransferType != eDOWNLOAD_NORMAL)
			{
				m_un64SkipChunkStart += readBytes;
			}

			stiDEBUG_TOOL (g_stiMP4FileDebug,
				stiTrace("m_un64FileSize = %llu, m_un64YetToDownload = %llu\n", m_un64FileSize, m_un64YetToDownload);
			);
		}
		catch (std::exception &e)
		{
			stiTHROWMSG (stiRESULT_ERROR, e.what());
		}

		InitializeDataRateTracking();
	}

STI_BAIL:

	// Reset chunk start and end so FileChunkGet() will know that it can request another chunk.
	m_un64CurrentChunkStart = 0;
	m_un64CurrentChunkEnd = 0;

	// If we have failed to get a good connection to the HTTP server, or if an HTTP or
	// application error has been returned, inform the FilePlay class of the error.
	if (stiIS_ERROR (hResult))
	{
		if (stiRESULT_CODE(hResult) == stiRESULT_ERROR)
		{
			downloadErrorSignal.Emit();

			// If YetToDownload is 0 we probably errored out before we started to download any
			// data so if we have a fileSize then set YetToDownload equal to it (which is what would have
			// happened if we hadn't gotten an error) so that if a download is retried FileChunkGet()
			// will determine the correct chunk to get.
			if (m_un64YetToDownload == 0 &&
				m_un64FileSize > 0)
			{
				m_un64YetToDownload = m_un64FileSize;
			}
		}
		else
		{
			//May not be needed.
			errorSignal.Emit(hResult);
		}

		if (m_eState == estiDOWNLOADING)
		{
			m_eState = estiDOWNLOAD_ERROR;
		}

		cleanupSocket();
	} // end if

	// Check to see if we have the entire chunk.
	if (m_responseBody && m_responseBody->eof () &&
		(m_un64FileSize - m_un64YetToDownload == m_un64CurrentChunkEnd ||
		 m_un64ChunkLengthRemaining == 0))
	{
		cleanupSocket();
		if (m_eTransferType != eDOWNLOAD_NORMAL)
		{
			m_un64SkipChunkStart = 0;
			m_un64SkipChunkEnd = 0;
			m_eTransferType = eDOWNLOAD_NORMAL;
		}
	}
} // end CDataTransferHandler::EventFileChunckGet


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventFileReadChunkGet
//
// Description:
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, otherwise stiRESULT_ERROR.
//
void CDataTransferHandler::EventFileReadChunkGet (
	Movie_t* movie)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::EventFileGet);

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::EventFileGet> Entry\n");
	);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	uint8_t *pWriteBuffer = nullptr;
	uint32_t un32BytesToRead = 0;
	uint32_t un32BytesRead = 0;
	uint32_t un32BufSize = 0;

	if (movie && m_un64FileSize == 0)
	{
		// Initialize
		m_abortTransfer = false;
		m_readyToPlay = false;

		m_waitingForParsableCheck = false;
		m_un32MoreDataForParsableCheck = 0;

		m_un64CurrentOffset = 0;
		m_un64ParseOffset = 0;
		m_un64YetToDownload = 0;

		m_waitingForRelease = false;

		// If this is our first time we need to open the file an get its length.
		m_pMovieFile = fopen(m_fileGUID.c_str(), "rb"); //Windows default is to use text-mode, resulting in unwanted translations. Explicitly choosing binary mode fixes it.
		stiTESTCOND (m_pMovieFile, stiRESULT_UNABLE_TO_OPEN_FILE);

		// Get the file size.
		fseek(m_pMovieFile, 0, SEEK_END);
		m_un64FileSize = ftell(m_pMovieFile);
		fseek(m_pMovieFile, 0, SEEK_SET);

		// We may have just gotten the file size so check to make sure we aren't requesting
		// a file chunk that is larger than the file. 
		if (m_un64FileSize < m_un64CurrentChunkEnd)
		{
			m_un64CurrentChunkEnd = m_un64FileSize;
		}

		m_un64YetToDownload = m_un64FileSize;
	}
	else
	{
		fseek(m_pMovieFile, static_cast<long>(m_un64CurrentChunkStart), SEEK_SET);
	}

	if (m_un64YetToDownload == 1 && 
		m_un64CurrentChunkEnd == m_un64CurrentChunkStart)
	{
		// We are at the end of the file so set yetToDownload to 0
		m_un64YetToDownload = 0;
	}
	else
	{
		un32BufSize = m_pCircBuf->GetCurrentWritePosition (&pWriteBuffer);
		stiTESTCOND (nullptr != pWriteBuffer, stiRESULT_VM_CIRCULAR_BUFFER_ERROR);

		un32BytesToRead = static_cast<uint32_t>(m_un64CurrentChunkEnd - m_un64CurrentChunkStart > un32BufSize ? un32BufSize : m_un64CurrentChunkEnd - m_un64CurrentChunkStart);

		un32BytesRead = fread(pWriteBuffer, 1, un32BytesToRead, m_pMovieFile);
		stiTESTCOND (ferror(m_pMovieFile) == 0, stiRESULT_ERROR);

		// Update the amount to still be downloaded.
		m_un64YetToDownload -= un32BytesRead;

		hResult = m_pCircBuf->InsertIntoCircularBufferLinkedList (pWriteBuffer, un32BytesRead,                                  
																  m_eTransferType == eDOWNLOAD_SKIP_BACK ? estiTRUE : estiFALSE);
	}

	if (!stiIS_ERROR(hResult))
	{
		if (un32BytesRead)
		{
			MoreDataAvailable (un32BytesRead);
		}
		else
		{
			DownloadProgressUpdate();
		}
	}


STI_BAIL:

	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::GetSkipBackData
//
// Description: Initializes members and starts the process of getting data
// 				that has been overwritten but is need for a skipback request.
//
// Abstract:
//
// Returns: None
//
stiHResult CDataTransferHandler::GetSkipBackData (
	uint64_t un64SampleOffset,
	uint64_t un64EndOffset)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SSkipDataOffsets skipDataOffsets = {0, 0, 0, 0};

	skipDataOffsets.un64ReleaseStartOffset = 0;
	skipDataOffsets.un64ReleaseEndOffset = 0;
	skipDataOffsets.un64SampleOffset = un64SampleOffset;
	skipDataOffsets.un64EndOffset = un64EndOffset;

	PostEvent([this, skipDataOffsets]{EventGetSkipBackData(skipDataOffsets);});

	return hResult;
}

void CDataTransferHandler::EventGetSkipBackData (
	const SSkipDataOffsets &skipDataOffsets)
{
	EstiResult eResult = estiERROR; stiUNUSED_ARG (eResult);
	stiHResult hResult = stiRESULT_SUCCESS;

	uint64_t un64SampleOffsetBegin;
	uint64_t un64SampleOffsetEnd;
	uint32_t un32ReleasedBytes;

	// If m_bDownloadSkipBackData is true we are in the process of downloading some skipback
	// data so just reset the beginning and work to the same end.
	if (m_eTransferType != eDOWNLOAD_SKIP_BACK)
	{
		un64SampleOffsetBegin = skipDataOffsets.un64SampleOffset;
		un64SampleOffsetEnd = skipDataOffsets.un64EndOffset;
	}
	else
	{
		un64SampleOffsetBegin = skipDataOffsets.un64SampleOffset;
		un64SampleOffsetEnd = m_un64SkipChunkEnd;
	}
	
	if (un64SampleOffsetEnd - un64SampleOffsetBegin < CVMCircularBuffer::MaxBufferingSizeGet())
	{
		// Remember that we are working backwards so the offset is really the offset of the first
		// frame that we want to keep.
		hResult = m_pCircBuf->ReleaseSkipBackBuffer (static_cast<uint32_t>(un64SampleOffsetEnd),
													 static_cast<uint32_t>(un64SampleOffsetEnd - un64SampleOffsetBegin),
													 &un32ReleasedBytes);
		stiTESTRESULT();
		// If we removed not viewed data from the buffer we need to redownload it.  So we have to
		// update the amount we have downloaded.
		m_un64YetToDownload += un32ReleasedBytes;
		
		m_eTransferType = eDOWNLOAD_SKIP_BACK;
	}
	else 
	{
		m_pCircBuf->CleanupCircularBuffer(estiFALSE, static_cast<uint32_t>(un64SampleOffsetBegin));
	
		m_un64YetToDownload = m_un64FileSize - un64SampleOffsetBegin;

		un64SampleOffsetEnd = un64SampleOffsetBegin + un32SKIP_FILE_CHUNK_SIZE; 

		// We are now reloading the file so set the transfer type to DOWNLOAD_NORMAL
		m_eTransferType = eDOWNLOAD_NORMAL;
	}                         

	// Set CurrentChunkStart and CurrentChunkEnd to 0 so that we don't try to download
	// that chunk.
	m_un64CurrentChunkStart = 0;
	m_un64CurrentChunkEnd = 0;
	hResult = FileChunkGet(nullptr, un64SampleOffsetBegin, un64SampleOffsetEnd - 1);
	stiTESTRESULT();
	
STI_BAIL:

	return;
}

////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::GetSkipForwardData
//
// Description: Initializes members and starts the process of getting data
// 				that is in the future.
//
// Abstract:
//
// Returns: EstiResult
//	estiOK - Success
//	estiERROR - Failure
//
stiHResult CDataTransferHandler::GetSkipForwardData (
	uint64_t un64ReleaseStartOffset,
	uint64_t un64ReleaseEndOffset,
	uint64_t un64SampleOffset,
	uint64_t un64EndOffset)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SSkipDataOffsets skipDataOffsets = {0, 0, 0, 0};

	skipDataOffsets.un64ReleaseStartOffset = un64ReleaseStartOffset;
	skipDataOffsets.un64ReleaseEndOffset = un64ReleaseEndOffset;
	skipDataOffsets.un64SampleOffset = un64SampleOffset;
	skipDataOffsets.un64EndOffset = un64EndOffset;

	PostEvent([this, skipDataOffsets]{EventGetSkipForwardData(skipDataOffsets);});

	return hResult;
}


void CDataTransferHandler::EventGetSkipForwardData (
	const SSkipDataOffsets &skipDataOffsets)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint64_t sampleOffset = skipDataOffsets.un64SampleOffset;

	m_eTransferType = eDOWNLOAD_SKIP_FORWARD;

	// Set CurrentChunkStart and CurrentChunkEnd to 0 so that we can download a new chunk.
	m_un64CurrentChunkStart = 0;
	m_un64CurrentChunkEnd = 0;

	// If the release params are both 0 we need to releae the entier buffer.
	if (skipDataOffsets.un64ReleaseStartOffset == 0 &&
		skipDataOffsets.un64ReleaseEndOffset == 0)
	{
		m_pCircBuf->CleanupCircularBuffer(estiFALSE, static_cast<uint32_t>(skipDataOffsets.un64SampleOffset));

		// If we are reseting the buffer then we are really doing a
		// normal download and not skipping.
		m_eTransferType = eDOWNLOAD_NORMAL;
	}
	else if (skipDataOffsets.un64ReleaseStartOffset != skipDataOffsets.un64ReleaseEndOffset)
	{
		ReleaseBuffer(static_cast<uint32_t>(skipDataOffsets.un64ReleaseStartOffset),
					  static_cast<uint32_t>(skipDataOffsets.un64ReleaseEndOffset - skipDataOffsets.un64ReleaseStartOffset + 1));
	}


	// If the sample offset has already been downloaded reset the start to
	// the current download position.
	if (m_eTransferType == eDOWNLOAD_SKIP_FORWARD &&
		skipDataOffsets.un64SampleOffset < m_un64FileSize - m_un64YetToDownload)
	{
		sampleOffset = m_un64FileSize - m_un64YetToDownload;
	}
	else
	{
		// We need to adjust YetToDownload, because we are going to skip or redownload some data.
		m_un64YetToDownload = m_un64FileSize - skipDataOffsets.un64SampleOffset;
	}


	hResult = FileChunkGet(nullptr, sampleOffset, skipDataOffsets.un64EndOffset);
	stiTESTRESULT();

STI_BAIL:

	return;
}

////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::MovieFileInfoSet
//
// Description: Set server and file information
//
// Abstract:
//
// Returns: EstiResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
stiHResult CDataTransferHandler::MovieFileInfoSet (
	const std::string &server,
	const std::string &fileGUID,
	uint32_t un32DownloadSpeed,
	uint64_t un64FileSize,
	const std::string &clientToken)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	stiTESTCOND (!fileGUID.empty(), stiRESULT_ERROR);

	m_fileGUID = fileGUID;

	if (!server.empty())
	{
		m_server = server;
	}
	else
	{
		m_server.clear();
	}

	m_un64FileSize = un64FileSize;

	if (!clientToken.empty())
	{
		m_clientToken = clientToken;
	}
	else
	{
		m_clientToken.clear();
	}
	
	m_un32DownloadSpeed = un32DownloadSpeed;

	m_eState = !m_server.empty() ? estiDOWNLOADING : estiREADFILE;

STI_BAIL:	

	return hResult;
} // end CDataTransferHandler::MovieFileInfoSet


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::NotifyParseError
//
// Description: Notifies the DataTransferHandler that there was a parsing
//  				error, so stop the download.
//
// Abstract:
//
// Returns: EstiResult
//
//
stiHResult CDataTransferHandler::NotifyParseError()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{EventStateSet(estiDOWNLOAD_ERROR);});

	return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventStateSet
//
// Abstract: Sets the state to the state in the event.
//
// Returns:
//
void CDataTransferHandler::EventStateSet (
	EState state)
{
	m_eState = state;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::FileChunkGet
//
// Description: Get a video message from the media server.
//
// Abstract:
//
// Returns: stiHResult
//
//
stiHResult CDataTransferHandler::FileChunkGet (
	Movie_t* pMovie,
	uint64_t beginByte, 
	uint64_t endByte)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::FileGet);
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	stiHResult hResult = stiRESULT_SUCCESS;

	// If this function is called then we are using range requests.
	m_useRangeRequest = true;

	// If we are just startng a download then we need to set the transfer type.
	if (m_eTransferType == ePAUSED)
	{
		m_eTransferType = eDOWNLOAD_NORMAL;
	}

	if (m_eState != estiDOWNLOAD_ERROR)
	{
		stiTESTCOND (!m_fileGUID.empty(), stiRESULT_SRVR_DATA_HNDLR_CRITICAL_ERROR);

		// Did we get a valid pointer?
		if (m_eState == estiDOWNLOADING)
		{
			stiTESTCOND (!m_server.empty(), stiRESULT_SRVR_DATA_HNDLR_CRITICAL_ERROR);

			stiDEBUG_TOOL (g_stiMP4FileDebug,
				stiTrace("<CDataTransferHandler::FileGet> Called with \"%s\" and \"%s\".\n", m_server.c_str(), m_fileGUID.c_str());
			);
			
			// And is there something in it?
			if (m_un64CurrentChunkStart == 0 && m_un64CurrentChunkEnd == 0)
			{	
				// If we have a socket we need to close it so that we don't get
				// data from an old reqeust.
				cleanupSocket();

				// If begin and end bytes are not 0 get the specified chunk.
				// Otherwise calculate the next chunk to get.
				if (beginByte != 0 ||
					endByte != 0)
				{
					m_un64CurrentChunkStart = beginByte;
					m_un64CurrentChunkEnd = endByte;

					// If the start chunk is less than the start point of the remaining
					// file then we must be skipping back, so set up the skipback data.
					if (m_eTransferType != eDOWNLOAD_NORMAL)
					{
						m_un64SkipChunkStart = m_un64CurrentChunkStart;

						// Add 1 to the End ponit to account for final byte that is included.
						m_un64SkipChunkEnd = m_un64CurrentChunkEnd + 1;
						m_un64SkipDataSize = m_un64SkipChunkEnd - m_un64SkipChunkStart;
					}
				}
				else
				{
					m_un64CurrentChunkStart = m_un64FileSize - m_un64YetToDownload;
					m_un64CurrentChunkEnd = m_un64CurrentChunkStart + un32FILE_CHUNK_SIZE;
				}

				// If the the end of the requested chunk is larger then the end of the file
				// reset the end to be the size of the file.
				if (0 != m_un64FileSize)
				{
					if (m_un64CurrentChunkEnd > m_un64FileSize)
					{
						m_un64CurrentChunkEnd = m_un64FileSize - 1;
					}
					if (m_un64CurrentChunkStart > m_un64FileSize)
					{
						m_un64CurrentChunkStart = m_un64FileSize - 1;
					}

					m_un64DownloadDataTarget = m_un64FileSize - m_un64CurrentChunkEnd - 1;
				}

				if (m_un64CurrentChunkEnd > m_un64CurrentChunkStart)
				{
					PostEvent([this, pMovie]{EventFileChunkGet(pMovie);});
				}
				else
				{
					// Clear the Chunk variables so that we can call FileChunkGet() again and
					// get the next chunk that will work.
					m_un64CurrentChunkStart = 0;
					m_un64CurrentChunkEnd = 0;
				}
			} // end if
		}
		else if (m_eState == estiREADFILE) 
		{
			// If begin and end bytes are not 0 get the specified chunk.
			// Otherwise calculate the next chunk to get.
			if (beginByte != 0 ||
				endByte != 0)
			{
				m_un64CurrentChunkStart = beginByte;
				m_un64CurrentChunkEnd = endByte;

				// If the start chunk is less than the start point of the remaining
				// file then we must be skipping back, so set up the skipback data.
				if (m_eTransferType != eDOWNLOAD_NORMAL)
				{
					m_un64SkipChunkStart = m_un64CurrentChunkStart;

					// Add 1 to the End ponit to account for final byte that is included.
					m_un64SkipChunkEnd = m_un64CurrentChunkEnd + 1;
					m_un64SkipDataSize = m_un64SkipChunkEnd - m_un64SkipChunkStart;
				}
			}
			else
			{
				m_un64CurrentChunkStart = m_un64FileSize - m_un64YetToDownload;
				m_un64CurrentChunkEnd = m_un64CurrentChunkStart + un32FILE_CHUNK_SIZE;
			}

			// If the the end of the requested chunk is larger then the end of the file
			// reset the end to be the size of the file.
			if (0 != m_un64FileSize)
			{
				if (m_un64CurrentChunkEnd > m_un64FileSize)
				{
					m_un64CurrentChunkEnd = m_un64FileSize - 1;
				}
				if (m_un64CurrentChunkStart > m_un64FileSize)
				{
					m_un64CurrentChunkStart = m_un64FileSize - 1;
				}

				m_un64DownloadDataTarget = m_un64FileSize - m_un64CurrentChunkEnd - 1;
			}

			if (m_un64CurrentChunkEnd >= m_un64CurrentChunkStart)
			{
				PostEvent([this, pMovie]{EventFileReadChunkGet(pMovie);});
			}
			else
			{
				// Clear the Chunk variables so that we can call FileChunkGet() again and
				// get the next chunk that will work.
				m_un64CurrentChunkStart = 0;
				m_un64CurrentChunkEnd = 0;
			}
		}
	}
	else
	{
		// We are in a download error state so generate an error.
		stiTHROW(stiRESULT_ERROR);
	}

STI_BAIL:
		
	return (hResult);
} // end CDataTransferHandler::FileChunkGet


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::FinishUpload
//
// Description: Calls Upload() until the buffer is empty.
//
// Abstract:
//
// Returns: stiRESULT_SUCCESS on success.
//
//
stiHResult CDataTransferHandler::FinishUpload ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SHTTPResult stResult;
	int nSelectRet = 0; stiUNUSED_ARG (nSelectRet);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_clientSession)
	{
		// We need to get some data back from the server so make sure
		// we can wait for it.

		// Upload the remaining data in the buffer.
		while (!m_pCircBuf->IsBufferEmpty())
		{
			UploadProgressUpdate();

			hResult = Upload (false);
			stiTESTRESULT ();
		}

		if (m_eTransferType == eUPLOAD_CONTINUOUS)
		{
			// Now that the upload if finshed let the fileplayer know how big it was.
			messageSizeSignal.Emit(m_un64CurrentOffset);
		}

		std::string responseBody;
		hResult = getHTTPResponse (responseBody);
		stiTESTRESULT();

		if (m_eTransferType == eUPLOAD_CONTINUOUS)
		{
			auto responseData = (char *)stiHEAP_ALLOC (responseBody.size () + 1);
			responseBody.copy (responseData, responseBody.size (), 0);
			responseData[responseBody.size ()] = '\0';
			messageIdSignal.Emit(responseData);
		}
	}

STI_BAIL: 
  
	cleanupSocket();
  
	// If we have failed to upload teh remaining parts of the file notify the fileplayer.
	if (stiIS_ERROR (hResult))
	{
		m_eState = estiUPLOAD_ERROR;

		// Notify the FilePlayer that we had an error.
		uploadErrorSignal.Emit();
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::IsSampleDownloading
//
// Description: Checks to see if sample is within the requested download chunk.
//
// Abstract:
//
// Returns: 
//  estiTRUE or estiFALSE
//
//
bool CDataTransferHandler::IsSampleDownloading(
	uint64_t un64SampleOffset,
	uint32_t un32DataLength,
	bool skippingBack)
{
	EstiBool bRet = estiFALSE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// If we don't have a client session then we arn't downloading so return false.
	if (!m_clientSession)
	{
		bRet = estiFALSE;
	}
	else if (skippingBack)
	{
		if (m_eTransferType == eDOWNLOAD_SKIP_BACK &&
			un64SampleOffset > m_un64SkipChunkEnd - m_un64SkipDataSize)
		{
			bRet = estiTRUE;
		}
	}
	else
	{
		uint64_t un64EndingOffset = m_un64FileSize - m_un64YetToDownload + m_un64ChunkLengthRemaining;
		if (un64SampleOffset <  un64EndingOffset &&
			un64SampleOffset + un32DataLength <= un64EndingOffset)
		{
			bRet = estiTRUE;
		}
	
	}
	
	return (bRet);
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::MaxBufferingSizeGet
//
// Description: Get the maximum size for buffering
//
// Abstract:
//
// Returns: 
//  uint32_t - maximum size for buffering
//
//
uint32_t CDataTransferHandler::MaxBufferingSizeGet () const
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::MaxBufferingSizeGet);

	uint32_t un32stMaxBuffSize;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	un32stMaxBuffSize = CVMCircularBuffer::MaxBufferingSizeGet ();

	return (un32stMaxBuffSize);
} // end CDataTransferHandler::MaxBufferingSizeGet


/*!\brief Sends a message with the current "ready to play" value.
 *
 * \param bReadyToPlay A boolean indicating whether or not the video is ready to play
 */
stiHResult CDataTransferHandler::ReadyToPlaySet (
	bool readyToPlay)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Some space has been released, start downloading again
	PostEvent([this, readyToPlay]{EventReadyToPlaySet(readyToPlay);});

	return hResult;
}


/*!\brief Handles the "ready to play set" message using the value passed in with the message.
 *
 * \param poEvent The event object containing the "ready to play" state.
 */
void CDataTransferHandler::EventReadyToPlaySet (
	bool readyToPlay)
{
	if (!m_readyToPlay)
	{
		if (readyToPlay)
		{
			//
			// We are ready to play.  Set the data member
			// to indicate that we are ready to play
			// and call the download progree update method.
			//
			m_readyToPlay = readyToPlay;

			DownloadProgressUpdate ();
		}
		else if (m_un32MoreDataForParsableCheck > 0)
		{
			//
			// We are not ready to play and there has since
			// been additional data downloaded.  So, issue
			// the callback to recheck the parsable state.
			//
			parsableMoovCheckSignal.Emit(m_un32MoreDataForParsableCheck);
			m_un32MoreDataForParsableCheck = 0;
		}
		else
		{
			//
			// We are not ready to play and no new data has
			// been downloaded yet.  Just indicate that
			// we are not waiting for a parsable check.
			//
			m_waitingForParsableCheck = false;

			// If there are no more bytes to download and it is not readyToPlay the file is corrupted.
			if (m_un64YetToDownload == 0)
			{
				downloadErrorSignal.Emit();
			}

		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::MoreDataAvailable
//
// Description: This function is called to indicate more data has been received 
//  and put into the circular buffer.
//
// Abstract:
//
// Returns: stiHResult
//
//
stiHResult CDataTransferHandler::MoreDataAvailable (
	uint32_t un32Bytes)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CDataTransferHandler::MoreDataAvailable);
	
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::MoreDataAvailable> un32Bytes = %d\n", un32Bytes);
	);

	if (un32Bytes > 0)
	{
		// Start updating the progress as soon as we are ready to play.
		if (!m_readyToPlay)
		{
			if (!m_waitingForParsableCheck)
			{
				m_waitingForParsableCheck = true;
				parsableMoovCheckSignal.Emit(un32Bytes);
				m_un32MoreDataForParsableCheck = 0;
			}
			else
			{
				m_un32MoreDataForParsableCheck = un32Bytes;
			}
		}
		else
		{
			DownloadProgressUpdate ();
		}
	}

	return hResult;
} // end CDataTransferHandler::MoreDataAvailable


stiHResult CDataTransferHandler::DownloadProgressUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint32_t un32DataInBuffer = 0;
	m_pCircBuf->GetDataSizeInBuffer(static_cast<uint32_t>(m_un64CurrentOffset), &un32DataInBuffer);

	// Update download progress
	if (m_useRangeRequest)
	{
		if (m_eTransferType == eDOWNLOAD_NORMAL)
		{
			// Only send the update progress message when the other objects are ready to process it or if
			// we have finished the download (this would be the last update).
			if (m_sendDownloadProgress || m_un64YetToDownload == 0)
			{
				EstiBool bBuffFull = m_pCircBuf->IsBufferSpaceAvailable(un32FILE_CHUNK_SIZE) ? estiFALSE : estiTRUE;
				SFileDownloadProgress downloadProgress;

				// We don't want to send another update on the download progress until this update has been handled.
				m_sendDownloadProgress = false;

				// If we can't put another chunk in the buffer we need to start playing
				// (even if the buffer is not quite full).
				downloadProgress.bBufferFull = bBuffFull;
				downloadProgress.un64FileSizeInBytes = m_un64FileSize;
				downloadProgress.un64BytesYetToDownload = m_un64YetToDownload;
				downloadProgress.m_bytesInBuffer = un32DataInBuffer;
				downloadProgress.un32DataRate = m_un32DataRate;
				fileDownloadProgressSignal.Emit(downloadProgress);
			}
		}
		else
		{
			// Calculate the percentage downloaded.  Only update on the 10%s and when we have a new percent (i.e we
			// don't want to send multiple 20% updates we only need one).  If we update more than that we 
			// run the risk of overflowing the Player's message queue.
			auto  nSkipPercent = (int)((((double)(m_un64SkipDataSize - (m_un64SkipChunkEnd - m_un64SkipChunkStart))) /
								 (double)m_un64SkipDataSize) * 100.0F);

			if (m_nSkipPercent != nSkipPercent &&
				(!(nSkipPercent % 10) || nSkipPercent > 99))
			{
				m_nSkipPercent = nSkipPercent;

				SFileDownloadProgress downloadProgress;

				downloadProgress.bBufferFull = m_un64SkipChunkStart == m_un64SkipChunkEnd ? estiTRUE : estiFALSE;
				downloadProgress.un64BytesYetToDownload = m_un64SkipChunkEnd - m_un64SkipChunkStart;
				downloadProgress.un64FileSizeInBytes = m_un64SkipDataSize;
				downloadProgress.m_bytesInBuffer = m_un64SkipDataSize - downloadProgress.un64BytesYetToDownload;
				downloadProgress.un32DataRate = m_un32DataRate;
				skipDownloadProgressSignal.Emit(downloadProgress);
			}
		}
	}
	else
	{
		SFileDownloadProgress downloadProgress;

		downloadProgress.bBufferFull = estiFALSE;
		downloadProgress.un64FileSizeInBytes = m_un64FileSize;
		downloadProgress.un64BytesYetToDownload = m_un64YetToDownload;
		downloadProgress.m_bytesInBuffer = un32DataInBuffer;
		downloadProgress.un32DataRate = m_un32DataRate;
		fileDownloadProgressSignal.Emit(downloadProgress);
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::ReleaseBuffer
//
// Description: 
//
// Abstract:
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
//
stiHResult CDataTransferHandler::ReleaseBuffer (
	uint32_t un32Offset, 
	uint32_t un32NumberOfBytes)
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::ReleaseBuffer);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CDataTransferHandler::ReleaseBuffer>\n");
		stiTrace("\tun32Offset = %d un32NumberOfBytes = %d\n", un32Offset, un32NumberOfBytes);
	);

#if 1
	// Temporary fix to free the extraneous data between frames
	if (un32Offset > m_un64CurrentOffset)
	{
		auto  nBytes = static_cast<int>(un32Offset - m_un64CurrentOffset);

		hResult = m_pCircBuf->ReleaseBuffer (static_cast<uint32_t>(m_un64CurrentOffset), nBytes);
		m_un64CurrentOffset += nBytes;
		m_un32DataToRelease += nBytes;
	}
#endif

	hResult = m_pCircBuf->ReleaseBuffer (un32Offset, un32NumberOfBytes);
	m_un64CurrentOffset += un32NumberOfBytes;
	m_un32DataToRelease += un32NumberOfBytes;

	if (m_waitingForRelease && un32REUSE_BUFFER_SIZE < m_un32DataToRelease)
	{
		m_un32DataToRelease = 0;
		m_waitingForRelease = false;
	}

	return hResult;
} // end CDataTransferHandler::ReleaseBuffer


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::ResetExpiredGUID
//
// Abstract: Resets an expired GUID
//
// Returns: 
//
stiHResult CDataTransferHandler::ResetExpiredDownloadGUID(
		const std::string &fileGUID)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND(!fileGUID.empty(), stiRESULT_ERROR);

	if (m_eState == estiGUID_EXPIRED)
	{
		m_fileGUID = fileGUID;

        // Rest the state so we will start to download again.
        m_eState = estiDOWNLOADING;
	}

STI_BAIL:

	return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::ResetSendDownloadProgress
//
// Abstract: Sends an event to reset the controling member that lets the DataTransferHandler
//           notify the ServerDataHandler the current download status.
//
// Returns:
//
void CDataTransferHandler::ResetSendDownloadProgress()
{
	PostEvent([this]{EventResetSendDownloadProgress();});
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::EventResetSendDownloadProgress
//
// Abstract: Resets the controling member that lets the DataTransferHandler
//           notify the ServerDataHandler the current download status.
//
// Returns:
//
void CDataTransferHandler::EventResetSendDownloadProgress()
{
    m_sendDownloadProgress = true;
}


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::DHGetFileSize
//
// Abstract:
//
// Returns:
//
SMResult_t CDataTransferHandler::DHGetFileSize(
	uint64_t *pun64Size) const
{
	stiLOG_ENTRY_NAME (CDataTransferHandler::DHGetFileSize);

	SMResult_t SMResult = SMRES_SUCCESS;
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	*pun64Size = m_un64FileSize;

	return SMResult;
} // end CDataTransferHandler::DHGetFileSize


///////////////////////////////////////////////////////////////////////////////
//; CDataTransferHandler::HTTPDownloadRangeRequest
//
// Abstract:
//
// Returns:
//
stiHResult CDataTransferHandler::HTTPDownloadRangeRequest (uint64_t startRange, uint64_t endRange)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_response = sci::make_unique<HTTPResponse> ();

	// Create a HTTPS client session.
	try
	{
		if (!m_clientSession)
		{
			m_clientSession = sci::make_unique<HTTPSClientSession> (m_server, STANDARD_HTTPS_PORT);
			m_clientSession->setKeepAlive (true);
			m_clientSession->setTimeout (Poco::Timespan (g_nSOCKET_DOWNLOAD_TIMEOUT, 0));
		}
		auto request = getDownloadRequest (startRange, endRange);
		m_clientSession->sendRequest (*request);
		stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);
	}
	catch (std::exception &e)
	{
		m_clientSession = nullptr;
		stiASSERTMSG(false, e.what());
	}

	// If the HTTPS session fails then try a HTTP session (used by the screen saver previews).
	if (!m_clientSession)
	{
		try
		{
			m_clientSession = sci::make_unique<HTTPClientSession> (m_server, STANDARD_HTTP_PORT);
			m_clientSession->setKeepAlive (true);
			m_clientSession->setTimeout (Poco::Timespan (g_nSOCKET_DOWNLOAD_TIMEOUT, 0));

			auto request = getDownloadRequest (startRange, endRange);
			m_clientSession->sendRequest (*request);
			stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);
		}
		catch (std::exception &e)
		{
			stiTHROWMSG (stiRESULT_ERROR, e.what ());
		}
	}
	
	try
	{
			m_responseBody = sci::make_unique<std::istream> (m_clientSession->receiveResponse (*m_response).rdbuf ());
			stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);
			
			auto statusCode = m_response->getStatus ();
			if (statusCode == HTTPResponse::HTTPStatus::HTTP_SERVICE_UNAVAILABLE)
			{
				stiTHROW (stiRESULT_SERVER_BUSY);
			}
		
			// The return code from the HTTP server must be a 200 level code
			stiTESTCOND (statusCode >= HTTPResponse::HTTPStatus::HTTP_OK && statusCode < HTTPResponse::HTTPStatus::HTTP_MULTIPLE_CHOICES, stiRESULT_ERROR);

			// There must have been data returned in the content too
			stiTESTCOND (m_response->hasContentLength (), stiRESULT_ERROR);
	}
	catch (std::exception &e)
	{
		stiTHROWMSG (stiRESULT_ERROR, e.what ());
	}

STI_BAIL:

	return hResult;
}


stiHResult CDataTransferHandler::getHTTPResponse(
	std::string &responseBody)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	(*m_requestBody) << std::hex << m_uploadEndBoundary.str ().size () << httpEndGet () << httpEndGet () <<
	m_uploadEndBoundary.str () << httpEndGet () << httpEndGet ();
	std::istream &responseStream = m_clientSession->receiveResponse ((*m_response));
	auto status = m_response->getStatus ();
	auto contentLength = m_response->getContentLength ();
	std::string response (static_cast<unsigned int>(contentLength), '\0');

	stiTESTCOND (status == HTTPResponse::HTTPStatus::HTTP_OK, stiRESULT_ERROR);
	responseStream.read (&response[0], contentLength);
	response.erase (response.find_last_not_of ("\r\n") + 1);
	responseBody = response;

STI_BAIL:

	return hResult;
}


void CDataTransferHandler::cleanupSocket ()
{
	m_responseBody = nullptr;
	m_requestBody = nullptr;
	m_response = nullptr;
	m_clientSession = nullptr;
}

void CDataTransferHandler::logInfoToSplunk ()
{
	PostEvent ([this]
	{
		std::stringstream infoString;

		if (m_eState == estiDOWNLOADING)
		{
			uint32_t dataInBuffer = 0;
			m_pCircBuf->GetDataSizeInBuffer (static_cast<uint32_t>(m_un64CurrentOffset), &dataInBuffer);
			bool bufFull = m_pCircBuf->IsBufferSpaceAvailable (un32FILE_CHUNK_SIZE) ? false : true;

			infoString << "DOWNLOADING server: " << m_server;
			infoString << " GUID: " << m_fileGUID;
			infoString << " readyToPlay: " << m_readyToPlay;
			infoString << " fileSize: " << m_un64FileSize;
			infoString << " yetToDownlaod: " << m_un64YetToDownload;
			infoString << " bytesInBuffer: " << dataInBuffer;
			infoString << " bufferFull: " << bufFull;
			infoString << " dataRate: " << m_un32DataRate;
			infoString << "\n";
		}

		stiRemoteLogEventSend ("DataTransferHandler Transfer Info: %s\n", infoString.str ().c_str ());
	}); 
}

