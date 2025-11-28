/*!
 * \file CstiImageDownloadService.cpp
 *  \brief The purpose of the ImageDownloadService task is to manage the
 *  downloading of images from Sorenson's image servers.  Downloaded images
 *  are written to a file specified with the download request.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiImageDownloadService.h"

#include "CstiHTTPService.h"

#include "stiTools.h"

#include <fstream>
#include <cstdio>

#define stiIMAGE_DL_SERVICE_MAX_MESSAGES_IN_QUEUE 40
#define stiIMAGE_DL_SERVICE_MAX_MSG_SIZE sizeof(CstiEvent)
#define stiIMAGE_DL_SERVICE_TASK_NAME "CstiImageDownloadService"
#define stiIMAGE_DL_SERVICE_TASK_PRIORITY 151
#define stiIMAGE_DL_SERVICE_STACK_SIZE 4096

stiEVENT_MAP_BEGIN (CstiImageDownloadService)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiImageDownloadService::ShutdownHandle)
stiEVENT_DEFINE (estiDOWNLOAD_IMAGE, CstiImageDownloadService::DownloadImage)
stiEVENT_MAP_END (CstiImageDownloadService)
stiEVENT_DO_NOW (CstiImageDownloadService)

namespace
{
	struct Request
	{
		unsigned int id;
		std::string local_save_path;
		std::string url1;
		std::string url2;

		Request(
			unsigned int id,
			const std::string& local_save_path,
			const std::string& url1,
			const std::string& url2):
			id(id),
			local_save_path(local_save_path),
			url1(url1),
			url2(url2)
		{
		}
	};

	const unsigned int DOWNLOAD_BUFFER_LENGTH = 4096;
}

/*! \brief Constructor
 *
 * This is the default constructor for the CstiImageDownloadService class.
 */
CstiImageDownloadService::CstiImageDownloadService(
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam):
	CstiOsTaskMQ(
		pfnAppCallback,
		CallbackParam,
		(size_t)this,
		stiIMAGE_DL_SERVICE_MAX_MESSAGES_IN_QUEUE,
		stiIMAGE_DL_SERVICE_MAX_MSG_SIZE,
		stiIMAGE_DL_SERVICE_TASK_NAME,
		stiIMAGE_DL_SERVICE_TASK_PRIORITY,
		stiIMAGE_DL_SERVICE_STACK_SIZE)
{
	stiLOG_ENTRY_NAME("CstiImageDownloadService::CstiImageDownloadService\n");
}


/*! \brief Initialize the CstiImageDownloadService task
 *
 * \return stiHResult
 * \retval stiRESULT_SUCCESS if success
 */
stiHResult CstiImageDownloadService::Initialize ()
{
	stiLOG_ENTRY_NAME(CstiImageDownloadService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_NextRequestIdMutex = stiOSMutexCreate();

	m_bInitialized = estiTRUE;

	return hResult;
}

/*! \brief Final cleanup of the CstiImageManager task
 *
 *  \retval None
 */
void CstiImageDownloadService::Uninitialize ()
{
	stiLOG_ENTRY_NAME(CstiImageDownloadService::Uninitialize);

	stiOSMutexDestroy(m_NextRequestIdMutex);
	m_NextRequestIdMutex = nullptr;

	delete m_pDownloadService;
	m_pDownloadService = nullptr;

	m_bInitialized = estiFALSE;
}

/*! \brief Start the CstiImageDownloadService task
 *
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if success
 */
stiHResult CstiImageDownloadService::Startup()
{
	stiLOG_ENTRY_NAME(CstiImageDownloadService::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (estiFALSE == m_bInitialized)
	{
		hResult = Initialize ();
		stiTESTRESULT ();
	}

	//
	// Start the task
	//
	hResult = CstiOsTaskMQ::Startup();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}

/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 * \return int
 * \retval Always returns 0
 */
int CstiImageDownloadService::Task ()
{
	stiLOG_ENTRY_NAME (CstiImageDownloadService::Task);

	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiIMAGE_DL_SERVICE_MAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	fd_set SReadFds;

	int nMsgFd = FileDescriptorGet ();

	struct timeval *pstTimeout = nullptr;

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		//
		// Initialize select.
		//
		stiFD_ZERO (&SReadFds);

		// select on the message pipe
		stiFD_SET (nMsgFd, &SReadFds);

		// wait for data to come in on the message queue
		int nSelectRet = stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, pstTimeout);

		if (stiERROR != nSelectRet)
		{
			// Check if ready to read from message queue
			if (stiFD_ISSET (nMsgFd, &SReadFds))
			{
				// read a message from the message queue
				hResult = MsgRecv ((char *)un32Buffer, stiIMAGE_DL_SERVICE_MAX_MSG_SIZE, stiWAIT_FOREVER);

				// Was a message received?
				if (stiIS_ERROR (hResult))
				{
					Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
				}
				else
				{
					// Yes! Process the message.
					auto poEvent = (CstiEvent*)(void *)un32Buffer;

					// Lookup and handle the event
					hResult = EventDoNow (poEvent);

					if (stiIS_ERROR (hResult))
					{
						Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if

					// Is the event a "shutdown" message?
					if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
					{
						// Yes.  Time to quit.  Break out of the loop
						break;
					} // end if
				} // end if
			}
		}
	} // end for

	return (0);
}

stiHResult CstiImageDownloadService::ProxyServerSet (
	const std::string& host, uint16_t port)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!host.empty())
	{
		std::string ServerIP;

		// Look up the ip address of the proxy Server
		hResult =
			stiDNSGetHostByName(
				host.c_str(),			// The name to be resolved
				nullptr,					// An alternate domain name to be resolved
				&ServerIP);	// The result

		stiTESTRESULT();

		m_ProxyServerAddress = ServerIP;
		m_ProxyServerPort = port;
	}
	else
	{
		m_ProxyServerAddress.clear();
	}

STI_BAIL:
	return hResult;
}

stiHResult CstiImageDownloadService::Schedule(
	const std::string& local_save_path,
	const std::string& remote_url,
	const std::string& alternate_remote_url,
	unsigned int* request_id)
{
	// Post a message to the message queue
	CstiEvent oEvent(
		CstiImageDownloadService::estiDOWNLOAD_IMAGE,
		(stiEVENTPARAM)new Request(
			*request_id = NextRequestId(),
			local_save_path,
			remote_url,
			alternate_remote_url));

	return MsgSend(&oEvent, sizeof(oEvent));
}

/*! \brief Starts the shutdown process of the CstiImageDownloadService task
 *
 * This method is responsible for doing all clean up necessary to gracefully
 * terminate the CstiImageDownloadService task. Note that once this method exits,
 * there will be no more message received from the message queue since the
 * message queue will be deleted and the CstiImageDownloadService task will go away.
 *
 * \param poEvent pointer to the event
 *
 * \return stiHResult
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageDownloadService::ShutdownHandle(void* poEvent)
{
	stiLOG_ENTRY_NAME (CstiImageDownloadService::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	Uninitialize ();

	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
}

/*! \brief Downloads an image from an HTTP server via a GET request.
 *
 * This method downloads an image from an HTTP server via a GET request and
 * writes the image to NVM.
 *
 * \param poEvent pointer to the event
 *
 * \return stiHResult
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageDownloadService::DownloadImage(void* poEvent)
{
	stiLOG_ENTRY_NAME (CstiImageDownloadService::DownloadImage);

	stiHResult download_status = stiMAKE_ERROR(stiRESULT_ERROR);

	auto request = (Request *)((CstiEvent *)poEvent)->ParamGet();

	std::string protocol1;
	std::string server1;
	std::string file1;

	std::string protocol2;
	std::string server2;
	std::string file2;
	
	bool parse_1_worked =
		   ParseUrl(request->url1.c_str(), &protocol1, &server1, &file1)
		&& (protocol1 == "http");

	bool parse_2_worked =
		   ParseUrl(request->url2.c_str(), &protocol2, &server2, &file2)
		&& (protocol2 == "http");

	std::string ServerIP;

	std::string url;
	std::string path;

	if (   parse_1_worked
		&& stiDNSGetHostByName(
				server1.c_str(),		// The name to be resolved
				nullptr,					// An alternate domain name to be resolved
				&ServerIP)				// The buffer to place the resolved IP address into
			== stiRESULT_SUCCESS)
	{
		url = request->url1;
		path = file1;
	}
	else if (   parse_2_worked
			 && stiDNSGetHostByName(
					server2.c_str(),		// The name to be resolved
					nullptr,					// An alternate domain name to be resolved
					&ServerIP)				// The buffer to place the resolved IP address into
				== stiRESULT_SUCCESS)
	{
		url = request->url2;
		path = file2;
	}

	if (!path.empty())
	{
		if (EstablishImageServerConnection (ServerIP.c_str ()) == stiRESULT_SUCCESS)
		{
			if (   m_pDownloadService->HTTPGetBegin(path.c_str(), -1, -1)
				== stiRESULT_SUCCESS)
			{
				std::ofstream local_file(
					request->local_save_path.c_str(),
					std::ofstream::binary | std::ofstream::trunc);

				if (local_file.is_open())
				{
					unsigned char download_buffer[DOWNLOAD_BUFFER_LENGTH];

					EHTTPServiceResult progress_result = eHTTP_TOOLS_CONTINUE;

					unsigned int bytes_downloaded = 0;

					while (progress_result == eHTTP_TOOLS_CONTINUE)
					{
						unsigned int read_byte_count = 0;
						unsigned int header_byte_count = 0;

						progress_result =
							m_pDownloadService->HTTPProgress(
								download_buffer,
								sizeof(download_buffer),
								&read_byte_count,
								&header_byte_count);

						if (((progress_result == eHTTP_TOOLS_CONTINUE) || 
							(progress_result == eHTTP_TOOLS_COMPLETE)) && 
							(read_byte_count > 0) && 
							(read_byte_count >= header_byte_count))
						{
                            if (header_byte_count > 0)
							{
								SHTTPResult stResult;
								progress_result = HeaderParse((char*)download_buffer, &stResult);

								// Check to see if we had an HTTP Error.
								if (progress_result == eHTTP_TOOLS_COMPLETE &&
									(stResult.unReturnCode >= 200 && 
									stResult.unReturnCode < 300))
								{
									//No error so continue to download the file.
									progress_result = eHTTP_TOOLS_CONTINUE;
								}
								else
								{
									progress_result = eHTTP_TOOLS_ERROR;
									stiASSERTMSG (estiFALSE, "Failed to download image HTTP Error: %d\n", stResult.unReturnCode);
								}
							}

							local_file.write(
								reinterpret_cast<char*>(&download_buffer[header_byte_count]),
								read_byte_count - header_byte_count);

							bytes_downloaded += read_byte_count - header_byte_count;
						}
					}

					local_file.close();

					if (   (progress_result == eHTTP_TOOLS_COMPLETE)
						&& (bytes_downloaded > 0))
					{
						download_status = stiRESULT_SUCCESS;
					}
					else
					{
						stiTrace(
							"Failed to download image from %s to %s\n",
							url.c_str(),
							request->local_save_path.c_str());

						stiASSERT (estiFALSE);

						// don't leave images that failed to download properly
						// on the filesystem
						std::remove(request->local_save_path.c_str());
					}
				}
				else
				{
					stiTrace(
						"Failed to open %s to write downloaded image %s\n",
						request->local_save_path.c_str(),
						url.c_str());

					stiASSERT (estiFALSE);
				}
			}
			else
			{
				stiTrace(
					"HTTPGetBegin failed when trying to download image %s\n",
					url.c_str());

				// This is a fatal error that causes the download service to
				// become inept whilst not indicating that its state is anything
				// other than happy, so use the big hammer and get rid of it so
				// a new one will be created for the next request.
				m_pDownloadService->SocketClose();
				delete m_pDownloadService;
				m_pDownloadService = nullptr;

				stiASSERT (estiFALSE);
			}
		}
		else
		{
			stiTrace(
				"SocketOpen failed when trying to download image %s\n",
				url.c_str());

			stiASSERT (estiFALSE);
		}
	}

	Callback(
		(download_status == stiRESULT_SUCCESS)
			? estiMSG_IMAGE_DOWNLOADED
			: estiMSG_IMAGE_DOWNLOAD_ERROR,
		request->id);

	delete request;

	return download_status;
}

unsigned int CstiImageDownloadService::NextRequestId()
{
	stiOSMutexLock(m_NextRequestIdMutex);
	const unsigned int ID = m_NextRequestId++;
	stiOSMutexUnlock(m_NextRequestIdMutex);

	return ID;
}

/*! \brief Tries to establish a connection with the specified image server if necessary.
 *
 * This method reuses an exiting connection in the event that the specified
 * image download server is the same as the one in use on an existing
 * connection.
 *
 * If no connection currently exists, an attempt to establish a new connection
 * with the specified server is made.
 *
 * If a connection to a different server is already active, that connection is
 * closed and a new connection is attempted with the specified server.
 *
 * \param pServerAddress pointer to the string representation of the image
 *                       download server's IP address
 *
 * \return stiHResult
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult
CstiImageDownloadService::EstablishImageServerConnection(const char* pServerAddress)
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);

	if (m_pDownloadService
		&& pServerAddress && *pServerAddress
		&& m_pDownloadService->ServerAddressGet() == pServerAddress
		&& (   (m_pDownloadService->StateGet() == eHTTP_COMPLETE)
			|| (m_pDownloadService->StateGet() == eHTTP_IDLE)))
	{
		// the download service is already connected to the appropriate server
		hResult = stiRESULT_SUCCESS;
	}
	else // download service doesn't exist or isn't connected to right server
	{
		if (m_pDownloadService)
		{
			m_pDownloadService->SocketClose();
		}

		delete m_pDownloadService;

		m_pDownloadService = new CstiHTTPService();

		m_pDownloadService->ServerAddressSet(pServerAddress, pServerAddress, STANDARD_HTTP_PORT);

		if (!m_ProxyServerAddress.empty())
		{
			hResult =
				CstiHTTPService::ProxyAddressSet(
					m_ProxyServerAddress,
					m_ProxyServerPort);

			stiTESTRESULT();
		}

		hResult = m_pDownloadService->SocketOpen();

		stiTESTRESULT();
	}

STI_BAIL:

	return hResult;
}
