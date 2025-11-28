/*!
*  \file CstiFileDownloadHandler.cpp
*  \brief The File Download Handler Interface
*
*  Class declaration for the File Download Handler Class.  
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

#include "CstiFileDownloadHandler.h"

#include "stiTaskInfo.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "stiRemoteLogEvent.h"
#include "Poco/Net/SSLException.h"
#include "Poco/Net/HTMLForm.h"
#include "CstiRegEx.h"


//
// Constants
//
#define MIN_DOWNLOAD_BUFFER_LENGTH (16 * 1024)
#define MAX_DOWNLOAD_BUFFER_LENGTH (128 * 1024)
#define REQUEST_HEADER_BUFFER_LENGTH (5 * 1024)
static const int g_nSOCKET_DOWNLOAD_TIMEOUT = 60; ///< Socket timeout value in seconds

//
// Typedefs
//
//

//
// Globals
//
static CstiFileDownloadHandler *g_pFileDownloadHandler = nullptr;  /// Global pointer to FileDownloadHandler class instance.
static int g_nCstiFileDownloadHandlerInstanceCount = 0; /// Global instance counter for CstiFileDownloadHandler instance.

//
// Locals
//

//
// Namespace Declarations
//
using namespace Poco::Net;

///
/// \brief Creates CstiFileDownloadHandler object, returns stiHResult based on success
///
stiHResult CstiFileDownloadHandlerInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!g_pFileDownloadHandler)
	{
		g_pFileDownloadHandler = new CstiFileDownloadHandler();
		stiTESTCOND (g_pFileDownloadHandler, stiRESULT_ERROR);
	}

	++g_nCstiFileDownloadHandlerInstanceCount; // Increment instance counter

STI_BAIL:

	return hResult;
}


///
/// \brief Decrements global instance count and (if needed) deletes the update instance
///
stiHResult CstiFileDownloadHandlerUninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	--g_nCstiFileDownloadHandlerInstanceCount; // Decrement instance counter

	if (0 >= g_nCstiFileDownloadHandlerInstanceCount && g_pFileDownloadHandler)
	{
		delete g_pFileDownloadHandler;
		g_pFileDownloadHandler = nullptr;
	}

	return (hResult);

} // end CstiFileDownloadHandlerUninitialize


///
/// \brief Returns instance of update object
///
IstiFileDownloadHandler *IstiFileDownloadHandler::InstanceGet ()
{
	return g_pFileDownloadHandler;
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
CstiFileDownloadHandler::CstiFileDownloadHandler ()
	: CstiEventQueue("CstiFileDownloadHandler")
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::CstiFileDownloadHandler)
}


//
// Destructor
//
/*!
 * \brief Destructor
 */
CstiFileDownloadHandler::~CstiFileDownloadHandler ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::~CstiFileDownloadHandler)

	ClearDownloadSettings();
}

/*!
 * \brief Clears settings used for downloading a file.
 *
 */

stiHResult CstiFileDownloadHandler::ClearDownloadSettings()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_fileName.clear();
	m_URL.clear();

	socketCleanup ();

	m_downloadBuffer.clear();
	m_bUseHTTPS = estiFALSE;
	m_downloadingFile.close();
	m_bytesDownloaded = 0;
	m_downloadFileSize = 0;
	m_bytesLeftToDownload = 0;
	m_requestedChunkSize = 0;
	m_lastPercentUpdate = 0;
	return hResult;
}

/*!
 * \brief Initializes file for writing and opens download socket.
 *
 */

stiHResult CstiFileDownloadHandler::DownloadInitialize()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::DownloadInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	const std::string CONTENT_RANGE_TAG = "Content-Range";
	std::vector<std::string> serverIPList;
	HTTPResponse response;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::DownloadInitialize\n");
	); // stiDEBUG_TOOL


	// Open the file and set up download variables.
	stiTESTCOND(!m_fileName.empty(), stiRESULT_ERROR);
	m_downloadingFile.open(m_fileName, std::ios_base::out | std::ios_base::binary | std::ios_base::ate | std::ios_base::app);

	// If we are restarting a download see how much we have already downloaded
	m_bytesDownloaded = m_downloadingFile.tellg();

	if (m_downloadingFile.fail())
	{
		m_downloadingFile.clear();
		stiTHROW (stiRESULT_ERROR);
	}

	stiTESTCOND(!m_URL.empty(), stiRESULT_ERROR);

	hResult =  HTTPDownloadRangeRequest(&response,
										0, 15);
	stiTESTRESULT();

	
	if (m_downloadFileSize == 0)
	{
		if (response.has (CONTENT_RANGE_TAG))
		{
			CstiRegEx digitsRegex ("^bytes ([0-9]+)-([0-9]+)/([0-9]+)");
			auto contentRange = response.get (CONTENT_RANGE_TAG);
			std::vector<std::string> results;
			digitsRegex.Match (contentRange, results);
			m_downloadFileSize = std::stoi (results[3]);
		}
	}

	// Reset the client session so we can send a new request later.
	m_clientSession->reset ();

	m_bytesLeftToDownload = m_downloadFileSize - m_bytesDownloaded;
	m_downloadChunkSize = MAX_DOWNLOAD_BUFFER_LENGTH;
	m_requestedChunkSize = 0;

	m_downloadBuffer.resize(MAX_DOWNLOAD_BUFFER_LENGTH + 1);

	StateSet(IstiFileDownloadHandler::eDOWNLOAD_INITIALIZED);

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		ErrorStateSet(eERROR_INITIALIZING_DOWNLOAD);
		StateSet(eDOWNLOAD_ERROR);

		// If we have an error close the file and socket.
		ClearDownloadSettings();
	}

	return hResult;
}


/*!
 * \brief Places an event on the FileDownloadHandler's task queue to download the next peice of the file.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileDownloadHandler::DownloadNextChunk()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::DownloadNextChunk);

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::DownloadNextChunk\n");
	); // stiDEBUG_TOOL

	PostEvent([this]{DownloadNextChunkHandler();});

	return stiRESULT_SUCCESS;
}


/*!
 * \brief DownloadNextChunkHandler 
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CstiFileDownloadHandler::DownloadNextChunkHandler ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::DownloadNextChunkHandler);

	stiHResult hResult = stiRESULT_SUCCESS;
	char *pDownloadBuffer = m_downloadBuffer.data();
	int32_t n32DataRead = 0;
	uint32_t un32TotalDataRead = 0;
	uint32_t un32DataLeftToRead = std::min((int)(m_downloadFileSize - m_bytesDownloaded), (int)m_downloadChunkSize);
	uint32_t un32NoBytesReadCount = 0;
	timespec Time1{};

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::DownloadNextChunkHandler\n");
	); // stiDEBUG_TOOL

	if (StateGet() == eDOWNLOADING ||
		StateGet() == eDOWNLOAD_RESUME ||
		StateGet() == eDOWNLOAD_INITIALIZED)
	{

		if (StateGet() != eDOWNLOADING)
		{
			StateSet(eDOWNLOADING);
		}

		// Start the next chunk downloading.
		// Watch the time we want to keep the chunk size to 1 second blocks.
		clock_gettime (CLOCK_MONOTONIC, &Time1);

		do
		{
			n32DataRead = ReadFileData(pDownloadBuffer,  			 
									   un32DataLeftToRead); 

			if (n32DataRead > 0)
			{
				// Update the counts and positions.
				pDownloadBuffer += n32DataRead;
				m_bytesDownloaded += n32DataRead;
				m_bytesLeftToDownload -= n32DataRead;
				un32TotalDataRead += n32DataRead;
				un32DataLeftToRead -= n32DataRead;

				if (un32DataLeftToRead == 0)
				{
					break;
				}

				un32NoBytesReadCount = 0;	// reset the "zero bytes read" counter
			}
			else if (n32DataRead <= -1)
			{
				stiDEBUG_TOOL (g_stiFileDownloadDebug,
					stiTrace ("<CstiFileDownloadHandler::DownloadNextChunkHandler> UpdateRead failed, un32DataRead = %d, un16DataLeftToRead = %d\n",
						n32DataRead, un32DataLeftToRead);
				); // stiDEBUG_TOOL
				stiTHROW (stiRESULT_ERROR);
			}  // end else if
			else
			{
				if (un32NoBytesReadCount >= 99)
				{
					// For SPLUNK logging, assert so we know if we received a 0 return code
					stiASSERT(false);
					un32NoBytesReadCount = 0;
				}

				un32NoBytesReadCount++;
			}

		} while (un32DataLeftToRead > 0);

		if (un32TotalDataRead > 0)
		{
			//
			// Compute the download rate.  We are trying to achieve around
			// one second per download buffer so that the download process
			// can be put on hold fairly quickly.
			//
			timespec Time2{};
			timeval Time3{};
			timeval Time4{};
			timeval TimeDiff{};

			clock_gettime (CLOCK_MONOTONIC, &Time2);

			Time3.tv_sec = Time1.tv_sec;
			Time3.tv_usec = Time1.tv_nsec / 1000;

			Time4.tv_sec = Time2.tv_sec;
			Time4.tv_usec = Time2.tv_nsec / 1000;

			timersub (&Time4, &Time3, &TimeDiff);

			float fDataRate = (static_cast<float>(un32TotalDataRead * 8.0F / (TimeDiff.tv_sec + TimeDiff.tv_usec / 1000000.0F)));

			stiDEBUG_TOOL (g_stiFileDownloadDebug,
					stiTrace ("Data Rate = %f kbps\n", fDataRate / 1024);
			);

			if (fDataRate > m_downloadChunkSize && m_downloadChunkSize < MAX_DOWNLOAD_BUFFER_LENGTH)
			{
				m_downloadChunkSize += 64 * 1024;

				if (m_downloadChunkSize > MAX_DOWNLOAD_BUFFER_LENGTH)
				{
					m_downloadChunkSize = MAX_DOWNLOAD_BUFFER_LENGTH;
				}
			}

			// Write the data to the file
			m_downloadingFile.write(m_downloadBuffer.data(), un32TotalDataRead);
			m_downloadingFile.flush();

			if (m_downloadingFile.fail())
			{
				m_downloadingFile.clear();
				stiTHROW (stiRESULT_ERROR);
			}

			// Clear the download buffer.
			m_downloadBuffer.clear();

			stiTESTRESULT ();
		}

		if (StateGet() == eDOWNLOADING &&
			m_bytesLeftToDownload > 0)
		{
			int percent = static_cast<int>(100.0F * (static_cast<float>(m_bytesDownloaded) / static_cast<float>(m_downloadFileSize)));

			if (m_lastPercentUpdate != percent)
			{
				m_lastPercentUpdate = percent;
				fileDownloadProgressSignal.Emit(percent);
			}

			// Go get the next chunk.
			DownloadNextChunk();
		}
		else if (m_bytesLeftToDownload == 0)
		{
			// Downloading is complete.
			m_downloadingFile.close();

			// Download is at 100%
			fileDownloadProgressSignal.Emit(100);

			StateSet(eDOWNLOAD_COMPLETE);
		}
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		ErrorStateSet(eERROR_DOWNLOADING);
		StateSet(eDOWNLOAD_ERROR);

		// If we have an error close the file and socket.
		ClearDownloadSettings();
	}
}


/*!
 * \brief Returns the current error state of the File Download Handler.
 * 
 * \return One of the errors defined in the EError enum.
 */
IstiFileDownloadHandler::EError CstiFileDownloadHandler::ErrorGet () const
{
	IstiFileDownloadHandler::EError eError;

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
stiHResult CstiFileDownloadHandler::ErrorStateSet (
	IstiFileDownloadHandler::EError errorState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_eError = errorState;

	return hResult;
}


/*!
 * \brief Places an event on the FileDownloadHandler's task queue to pause the download.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileDownloadHandler::Pause ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Pause);

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Pause\n");
	); // stiDEBUG_TOOL

	PostEvent([this]{
		if (StateGet() == eDOWNLOADING)
		{
			// Pause the download.
			StateSet(eDOWNLOAD_PAUSED);
		}
	});

	return stiRESULT_SUCCESS;
}


int32_t CstiFileDownloadHandler::ReadFileData(
	char *dataBuffer, 
	uint32_t bufferSize)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::ReadFileData);

	stiHResult hResult = stiRESULT_SUCCESS;
	int32_t dataRead = 0;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::ReadFileData\n");
	); // stiDEBUG_TOOL

	if (m_requestedChunkSize == 0)
	{
		// We haven't stated a download, make sure we have buffer space.
		if (bufferSize > 0)
		{
			// Reqeust the next chunk of data.
			hResult = RequestFileData(m_bytesDownloaded, 
									  m_bytesDownloaded + bufferSize); 
			stiTESTRESULT();
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}
	else
	{
		if (bufferSize > 0 && 
			m_responseBody)
		{
			try
			{
				// Read the bytes from the HTTP connection.  Adjust the starting position by the number of bytes that were in the temporary buffer.
				m_responseBody->read(dataBuffer, bufferSize);
				dataRead = static_cast<uint32_t>(m_responseBody->gcount ());
				stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);
			}
			catch (std::exception &e)
			{
				stiTHROWMSG (stiRESULT_ERROR, e.what ());
			}

			m_requestedChunkSize -= dataRead;

			// If we have read all of the bytes for this request then close the socket.
			if (m_requestedChunkSize == 0)
			{
				socketCleanup ();
			}
		}
		else
		{
			dataRead = 0;
		}
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		dataRead = -1;
		socketCleanup ();
	}

	return dataRead;
}


stiHResult CstiFileDownloadHandler::RequestFileData(
	uint64_t start, 
	uint64_t end)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Resume);

	stiHResult hResult = stiRESULT_SUCCESS;
	HTTPResponse response;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Resume\n");
	); // stiDEBUG_TOOL

	hResult = HTTPDownloadRangeRequest (&response,
										start, end);
	stiTESTRESULT ();

	try
	{
		// Get the return code of the HTTP header.
		auto returnCode = response.getStatus ();

		// The return code from the HTTP server must be a 200 level code
		if (returnCode < HTTPResponse::HTTPStatus::HTTP_OK || 
			returnCode >= HTTPResponse::HTTPStatus::HTTP_MULTIPLE_CHOICES)
		{
			stiTHROW (stiRESULT_ERROR);
		}

		m_requestedChunkSize = response.getContentLength64 ();
	}
	catch (std::exception &e)
	{
		stiTHROWMSG (stiRESULT_ERROR, e.what ());
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		socketCleanup();
	}

	return hResult;
}


/*!
 * \brief Places an event on the FileDownloadHandler's task queue to resume the download.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileDownloadHandler::Resume ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Resume);

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Resume\n");
	); // stiDEBUG_TOOL

	PostEvent([this]{
		// Resume the download.
		if (StateGet() == eDOWNLOAD_PAUSED)
		{
			// Pause the download.
			StateSet(eDOWNLOAD_RESUME);
			DownloadNextChunk();
		}
	});

	return stiRESULT_SUCCESS;
}


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
stiHResult CstiFileDownloadHandler::Start (
	const std::string &fileName,
	const std::string &url)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Start);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Start\n");
	); // stiDEBUG_TOOL

	// If we are not in an idle state then we are already downloading so don't start
	// another download.
	stiTESTCOND (StateGet() == IstiFileDownloadHandler::eDOWNLOAD_IDLE || 
				 StateGet() == IstiFileDownloadHandler::eDOWNLOAD_COMPLETE || 
				 StateGet() == IstiFileDownloadHandler::eDOWNLOAD_ERROR ||
				 StateGet() == IstiFileDownloadHandler::eDOWNLOAD_STOP,
				 stiRESULT_ERROR);
	stiTESTCOND (!fileName.empty() && !url.empty(), stiRESULT_INVALID_PARAMETER);

	PostEvent([this, fileName, url]{StartHandler(fileName, url);});

STI_BAIL:

	return hResult;
}


/*!
 * \brief StartHandler 
 * 
 * \param poEvent 
 * 
 * \return Success or failure result 
 */
void CstiFileDownloadHandler::StartHandler (
	const std::string &fileName,
	const std::string &url)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::StartHandler);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::StartHandle\n");
	); // stiDEBUG_TOOL

	// Clear the error State.
	ErrorStateSet(eERROR_NONE);

	m_fileName = fileName;

	try
	{
		m_URL = url;
	}
	catch(Poco::SyntaxException &e)
	{
		stiTHROWMSG (stiRESULT_ERROR, e.what());
	}

	// Open file and http socket.
	hResult = DownloadInitialize();
	stiTESTRESULT ();

	// Start the download.
	hResult = DownloadNextChunk();

STI_BAIL:;

}


/*!
 * \brief Returns the current state of the File Download Handler.
 * 
 * \return One of the states defined in the EState enum.
 */
IstiFileDownloadHandler::EState CstiFileDownloadHandler::StateGet () const
{
	IstiFileDownloadHandler::EState eState;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	eState = m_eState;

	return eState;
}


void CstiFileDownloadHandler::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::StateSet);

	// Update the state.
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_eState = eState;

	// Switch on the new state...
	switch (eState)
	{
	case eDOWNLOAD_IDLE:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eIDLE\n");
		); // stiDEBUG_TOOL

		fileDownloadStateSetSignal.Emit(eDOWNLOAD_IDLE);
		break;
	} // end case eIDLE

	case eDOWNLOADING:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eDOWNLOADING\n");
		); // stiDEBUG_TOOL

		fileDownloadStateSetSignal.Emit(eDOWNLOADING);
		break;
	} // end case eDOWNLOADING

	case eDOWNLOAD_PAUSED:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eDOWNLOAD_PAUSED\n");
		); // stiDEBUG_TOOL

		fileDownloadStateSetSignal.Emit(eDOWNLOAD_PAUSED);
		break;
	} // end case eDOWNLOAD_PAUSED

	case eDOWNLOAD_COMPLETE:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eDOWNLOAD_COMPLETE\n");
		); // stiDEBUG_TOOL

		// Notify the UI that download is complete
		fileDownloadStateSetSignal.Emit(eDOWNLOAD_COMPLETE);

		// Cleanup the download.
		ClearDownloadSettings();
		break;
	}

	case eDOWNLOAD_ERROR:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eDOWNLOAD_ERROR\n");
		); // stiDEBUG_TOOL

		fileDownloadStateSetSignal.Emit(eDOWNLOAD_ERROR);
		break;
	}

	case eDOWNLOAD_STOP:
	{
		stiDEBUG_TOOL (g_stiFileDownloadDebug,
			stiTrace ("CstiFileDownloadHandler::StateSet - Changing state to eDOWNLOAD_STOP\n");
		); // stiDEBUG_TOOL

		fileDownloadStateSetSignal.Emit(eDOWNLOAD_STOP);
		break;
	}

	case eDOWNLOAD_RESUME:
	case eDOWNLOAD_INITIALIZED:

		break;

	} // end switch

} 


stiHResult CstiFileDownloadHandler::Startup ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Startup\n");
	); // stiDEBUG_TOOL

	bool eventLoopStarted = CstiEventQueue::StartEventLoop();
	stiTESTCOND (eventLoopStarted, stiRESULT_ERROR);

STI_BAIL:

	return (hResult);
}


/*!
 * \brief Stops the download and removes the file.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileDownloadHandler::Stop ()
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::Stop);

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::Stop\n");
	); // stiDEBUG_TOOL

	PostEvent([this]{
		// Stop the download.
		StateSet(eDOWNLOAD_STOP);

		// Close and delete the file.
		m_downloadingFile.close();
		m_fileName.clear();

		ClearDownloadSettings();
	});

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Enables or disables HTTPS.
 * 
 * \return stiRESULT_SUCCESS if successful 
 *  	   stiRESULT_ERROR if an error occurs. 
 */
stiHResult CstiFileDownloadHandler::UseHTTPS (EstiBool useHTTPS)
{
	stiLOG_ENTRY_NAME (CstiFileDownloadHandler::UseHTTPS);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiFileDownloadDebug,
		stiTrace ("CstiFileDownloadHandler::UseHTTPS\n");
	); // stiDEBUG_TOOL

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_bUseHTTPS = useHTTPS;

	return (hResult);
}


stiHResult CstiFileDownloadHandler::HTTPDownloadRangeRequest (
	HTTPResponse *response,
	uint64_t startRange, 
	uint64_t endRange)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	try
	{
		if (!m_clientSession)
		{
			if (m_bUseHTTPS)
			{
				m_clientSession = sci::make_unique<HTTPSClientSession> (m_URL.getHost(), m_URL.getPort());
			}
			else
			{
				m_clientSession = sci::make_unique<HTTPClientSession> (m_URL.getHost(), m_URL.getPort());
			}
			m_clientSession->setKeepAlive (true);
			m_clientSession->setTimeout (Poco::Timespan (g_nSOCKET_DOWNLOAD_TIMEOUT, 0));
		}

		auto request = getDownloadRequest(startRange, 
										  endRange);
		m_clientSession->sendRequest (*request);
		stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);

		m_responseBody = sci::make_unique<std::istream> (m_clientSession->receiveResponse(*response).rdbuf ());
		stiTESTCOND (m_clientSession->networkException () == nullptr, stiRESULT_ERROR);

		auto statusCode = response->getStatus ();
		if (statusCode == HTTPResponse::HTTPStatus::HTTP_SERVICE_UNAVAILABLE)
		{
			stiTHROW (stiRESULT_SERVER_BUSY);
		}
		// The return code from the HTTP server must be a 200 level code
		stiTESTCOND (statusCode >= HTTPResponse::HTTPStatus::HTTP_OK && statusCode < HTTPResponse::HTTPStatus::HTTP_MULTIPLE_CHOICES, stiRESULT_ERROR);

		// There must have been data returned in the content too
		stiTESTCOND (response->hasContentLength (), stiRESULT_ERROR);
	}
	catch(std::exception &e)
	{
		stiTHROWMSG (stiRESULT_ERROR, e.what ());
	}

STI_BAIL:

	return hResult;
}


std::unique_ptr<HTTPRequest> CstiFileDownloadHandler::getDownloadRequest (
	uint64_t rangeStart,
	uint64_t rangeEnd)
{
	auto requestHeader = sci::make_unique<HTTPRequest> (HTTPRequest::HTTP_GET, m_URL.getPathAndQuery (), HTTPMessage::HTTP_1_1);
	auto timeout = m_clientSession->getKeepAliveTimeout ();
	requestHeader->setHost (m_URL.getHost());
	requestHeader->set ("Accept", "*/*");

	std::stringstream header;
	header << "bytes=" << rangeStart << "-" << rangeEnd;
	requestHeader->set ("Range", header.str());
	return requestHeader;
}


void CstiFileDownloadHandler::socketCleanup ()
{
	if (m_responseBody)
	{
		stiASSERT (m_responseBody->eof ());
	}

	m_responseBody = nullptr;
	m_clientSession = nullptr;
}


ISignal <int>& CstiFileDownloadHandler::fileDownloadProgressSignalGet ()
{
	return fileDownloadProgressSignal;
}


ISignal <IstiFileDownloadHandler::EState>& CstiFileDownloadHandler::fileDownloadStateSetSignalGet ()
{
	return fileDownloadStateSetSignal;
}
