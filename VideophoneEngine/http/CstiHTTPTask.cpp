////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiHTTPTask
//
//  File Name:	CstiHTTPTask.cpp
//
//  Owner:		Scot L. Brooksby
//
//	Abstract:
//		The HTTP task implements the functions needed to send HTTP requests
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include <cctype>	// standard type definitions.
#include <cstring>
#include <algorithm>

#include "stiOS.h"			// OS Abstraction
#include "CstiHTTPTask.h"
#include "stiTrace.h"
#include "stiTaskInfo.h"
#include "stiTools.h"
#include "CstiURLResolve.h"
#include "CstiExtendedEvent.h"

//
// Constants
//
//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-HTTPTask: %s\n", #name);

const int nHTTP_ACTIVITY_TIMEOUT = 30000;	// 30 seconds in milliseconds
const int nHTTP_ACTIVITY_MAXCOUNT = 4;	// four consecutive returns of no activity
										// will cause socket to close.

stiEVENT_MAP_BEGIN (CstiHTTPTask)
	stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiHTTPTask::ShutdownHandle)
	stiEVENT_DEFINE (estiEVENT_HEARTBEAT, CstiHTTPTask::HeartbeatHandle)
	stiEVENT_DEFINE (estiEVENT_TIMER, CstiHTTPTask::TimerHandle)
#if 0
	stiEVENT_DEFINE (estiHTTP_TASK_FORM_POST, CstiHTTPTask::HTTPFormPostHandle)
#endif
	stiEVENT_DEFINE (estiHTTP_TASK_POST, CstiHTTPTask::HTTPPostHandle)
#if 0
	stiEVENT_DEFINE (estiHTTP_TASK_GET, CstiHTTPTask::HTTPGetHandle)
#endif
	stiEVENT_DEFINE (estiHTTP_TASK_CANCEL, CstiHTTPTask::HTTPCancelHandle)
	stiEVENT_DEFINE (estiHTTP_TASK_URL_RESOLVE_SUCCESS, CstiHTTPTask::HTTPUrlResolveSuccessHandle)
	stiEVENT_DEFINE (estiHTTP_TASK_URL_RESOLVE_FAIL, CstiHTTPTask::HTTPUrlResolveFailHandle)
stiEVENT_MAP_END (CstiHTTPTask)
stiEVENT_DO_NOW (CstiHTTPTask)


//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::CstiHTTPTask
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiHTTPTask::CstiHTTPTask (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam)
	:
	CstiOsTaskMQ (
		pfnAppCallback,
		CallbackParam,
		(size_t)this,
		stiHTTP_MAX_MESSAGES_IN_QUEUE,
		stiHTTP_MAX_MSG_SIZE,
		stiHTTP_TASK_NAME,
		stiHTTP_TASK_PRIORITY,
		stiHTTP_STACK_SIZE)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::CstiHTTPTask);
} // end CstiHTTPTask::CstiHTTPTask


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::~CstiHTTPTask
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	none
//
CstiHTTPTask::~CstiHTTPTask ()
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::~CstiHTTPTask);
	
	
	// Delete the Activitiy Timer message queue.
	if (nullptr != m_wdidActivityTimer)
	{
		stiOSWdDelete (m_wdidActivityTimer);
		m_wdidActivityTimer = nullptr;
	} // end if
	
	if (nullptr != m_ctx)
	{
		stiSSLCtxDestroy (m_ctx);
		m_ctx = nullptr;
	}
} // end CstiHTTPTask::~CstiHTTPTask

//:-----------------------------------------------------------------------------
//:	Task functions
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//	This method will call the method that spawns the HTTP task
//	and also will create the message queue that is used to receive messages
//	sent to this task by other tasks.
//
// Returns:
//
stiHResult CstiHTTPTask::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	//EstiResult eResult = estiOK;

	if (nullptr == m_wdidActivityTimer)
	{
		//
		// Create our activity timer
		//
		m_wdidActivityTimer = stiOSWdCreate ();
	}

	if (nullptr == m_ctx)
	{
		//
		// Create a new SSLCtx
		//
		m_ctx = stiSSLCtxNew ();
	}
		
//STI_BAIL:

	return (hResult);
	
} // end CstiHTTPTask::Initialize


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::Startup
//
// Description: This is called to initialize the class.
//
// Abstract:
//	This method is called soon after the object is created to initialize the 
//	message system.
//
// Returns: 
//
stiHResult CstiHTTPTask::Startup ()
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::Startup);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	
	//
	// Startup tasks that this task depends on
	//
	if (nullptr == m_poUrlResolveTask)
	{
		// Create a new task for Url Resolving, and add this task into array
		m_poUrlResolveTask = sci::make_unique<CstiURLResolve> (UrlResolveCallback, (size_t)this, this);
		stiTESTCOND (m_poUrlResolveTask != nullptr, stiRESULT_ERROR);
		
		m_poUrlResolveTask->StartEventLoop ( );
		
	}
	
	stiTESTCOND (nullptr != m_wdidActivityTimer, stiRESULT_TASK_HTTP_NO_TIMER);

	//
	// Startup the task
	//
	hResult = CstiOsTaskMQ::Startup ();
	
	stiTESTRESULT ();


	//
	//	Start activiy timer
	//
	TimerStart (m_wdidActivityTimer, nHTTP_ACTIVITY_TIMEOUT, 0);
	
	stiTESTCOND (estiOK == eResult, stiRESULT_TASK_HTTP_WATCH_DOG_START_FAILED);
	
STI_BAIL:

	return (hResult);
}// end CstiHTTPTask::Startup


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::Task
//
// Description: Task function, executes until the task is shutdown.
//
// Abstract:
//	This task will loop continuously looking for messages in the message
//	queue.  When it receives a message, it will call the appropriate message
//	handling routine.
//
// Returns:
//	Always returns 0
//
int CstiHTTPTask::Task ()
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::Task);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	EstiBool bShutdown = estiFALSE;
	struct timeval*	ptimeOut = nullptr;
	uint8_t pun8Buffer[stiHTTP_MAX_MSG_SIZE];
	const uint32_t n32RECV_WAIT = 8;

	fd_set m_readyFds;		// Ready file descriptor
	fd_set m_returnFds;		// Return file descriptor
	
	EstiBool bSocketsAdded = estiFALSE;

	// Continue executing until shutdown
	while (!bShutdown)
	{
		bSocketsAdded = estiFALSE;

		stiFD_ZERO (&m_readyFds);
		stiFD_SET (FileDescriptorGet(), &m_readyFds);

		{
			std::lock_guard<std::mutex> lock (m_serviceMutex);

			// Add all of the entries that are in the array
			for (auto &service: m_apoHTTPService)
			{
				stiSocket Fd = service->SocketGet ();
				if (Fd != stiINVALID_SOCKET)
				{
					stiFD_SET (Fd, &m_readyFds);
					bSocketsAdded = estiTRUE;
				} // end if
			} // end for

			m_returnFds = m_readyFds;
		}

		//
		// Wait for something to be signalled
		//
		if (stiOSSelect (stiFD_SETSIZE, &m_returnFds, nullptr, nullptr, ptimeOut) > 0)
		{
			// Have we added any sockets?
			if (bSocketsAdded)
			{
				std::lock_guard<std::mutex> lock (m_serviceMutex);

				// Yes! Search all of the array entries
				for (auto it = m_apoHTTPService.begin (); it != m_apoHTTPService.end (); )
				{
					// Yes! Is there a socket in this entry?
					stiSocket Fd = (*it)->SocketGet ();

					if (Fd != stiINVALID_SOCKET)
					{
						// Yes! Is this one of the items that was signalled?
						if (stiFD_ISSET (Fd, &m_returnFds))
						{
							// Yes! Read from the socket.
							EstiResult eResult = HTTPReplyReceive (*it);

							if (estiERROR == eResult)
							{
								Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
							} // end if

							// If if the process is done...
							if (!(*it)->WaitingCheck ())
							{
								(*it)->SocketClose ();
								it = m_apoHTTPService.erase (it);
								continue;
							} // end if
						} // end if
					} // end if

					++it;
				} // end for
			} // end if

			// Was our pipe signalled?
			if (stiFD_ISSET (FileDescriptorGet(), &m_returnFds))
			{
				// Yes! Read a message from the pipe
				hResult = MsgRecv ((char*) pun8Buffer, stiHTTP_MAX_MSG_SIZE, n32RECV_WAIT);
				
				if (stiIS_ERROR (hResult))
				{
					Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
				}
				else
				{
					// Yes! Process the message.
					auto poEvent = (CstiEvent*)pun8Buffer;
			
					// Lookup and handle the event
					hResult = EventDoNow (poEvent);
					
					if (stiIS_ERROR (hResult))
					{
						Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if
			
					// Is the event a "shutdown" message?
					if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
					{
						// Yes.  Time to quit.  Set the shutdown flag to TRUE
						bShutdown = estiTRUE;
					} // end if
					
				} // end if
				
			} // end if
			
		} // end if
		
	} // end while

	return (0);
	
} // end CstiHTTPTask::Task


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::ShutdownHandle
//
// Description: Shutdown all things relating to this task
//
// Abstract:
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
stiHResult CstiHTTPTask::ShutdownHandle (
	CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::ShutdownHandle);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	stiOSWdCancel (m_wdidActivityTimer);
	
	//
	// For all of the entries that are in the array
	//
	{
		std::lock_guard<std::mutex> lock (m_serviceMutex);

		for (auto &service: m_apoHTTPService)
		{
			service->Cancel ();

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace("CstiHTTPTask has been SHUTDOWN - %p Cancelling\n", service.get ());
			);
		} // end for
	}

	//
	// Shutdown m_poUrlResolveTask
	//
	m_poUrlResolveTask->StopEventLoop ();
	
	hResult = CstiOsTaskMQ::ShutdownHandle ();
	
	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPCancel
//
// Description: Sets the request cancel flag to estiTRUE
//
// Abstract:
//
// Returns:
// 	none
//
void CstiHTTPTask::HTTPCancel (
	void *pRequestor,
	unsigned int unRequestNum)
{
	EstiBool bFound = estiFALSE;
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPCancel);

	{
		std::lock_guard<std::mutex> lock (m_serviceMutex);

		// For all of the entries that are in the array...
		for (auto &service: m_apoHTTPService)
		{
			// Yes! Does it match the request?
			if (service->RequestorInfoMatch (pRequestor, unRequestNum))
			{
				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace (
						"CstiHTTPTask::HTTPCancel: Cancelling Service %p...\n",
						service.get ());
				); // stiDEBUG_TOOL

				// Yes! Cancel it.
				service->Cancel ();
				bFound = estiTRUE;
				break;
			} // end if
		} // end for
	}

	// Did we find the event that was requested?
	if (!bFound)
	{
		// No! That means that we might still have it pending in the queue.
		// Send an asynchronous cancel request.
		CstiExtendedEvent oEvent (
			(int32_t)estiHTTP_TASK_CANCEL,
			unRequestNum,
			(stiEVENTPARAM)pRequestor);

		MsgSend (&oEvent, sizeof (oEvent));
	} // end if
	
} // end CstiHTTPTas


std::string CstiHTTPTask::reuseSessionFind (
	CstiHTTPService *poHTTPService,
	const std::vector<std::string> &serverList)
{
	std::string foundIP;
	if (poHTTPService->IsSSLUsed ())
	{
		SSL_SESSION * pSession = nullptr;

		for (auto &serverIP: serverList)
		{
			auto it = m_sslSessions.find (serverIP);

			if (it != m_sslSessions.end ())
			{
				pSession = (*it).second.get ();
				foundIP = serverIP;
				stiDEBUG_TOOL (g_stiSSLDebug,
					stiTrace ("Found IP: %s, %p\n", foundIP.c_str (), pSession);
				);
				break;
			}
		}

		poHTTPService->SSLSessionReusedSet (pSession);
	}

	return foundIP;
}


void CstiHTTPTask::sslSessionReuse (
	CstiHTTPService *poHTTPService,
	const std::string &foundIP,
	const std::string &serverIP)
{
	if (poHTTPService->IsSSLUsed ())
	{
		if (estiFALSE == poHTTPService->IsSSLSessionReused ())
		{
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("a new session has been negotiated!!!\n");
			); // stiDEBUG_TOOL

			if (poHTTPService->SSLVerifyResultGet())
			{
				if (!foundIP.empty ())
				{
					m_sslSessions[foundIP] = SSLSessionType (poHTTPService->SSLSessionGet (), SessionDeleter);

					stiDEBUG_TOOL (g_stiSSLDebug,
						stiTrace ("store SSL session %p for session Resumption for server %s\n",
							m_sslSessions[foundIP].get (), foundIP.c_str());
					); // stiDEBUG_TOOL
				}
				else
				{
					m_sslSessions[serverIP] = SSLSessionType (poHTTPService->SSLSessionGet (), SessionDeleter);

					stiDEBUG_TOOL (g_stiSSLDebug,
						stiTrace ("store SSL session %p for session Resumption for server %s\n",
							m_sslSessions[serverIP].get (), serverIP.c_str());
					); // stiDEBUG_TOOL
				}
			}
			else
			{
				 if (!foundIP.empty ())
				 {
					stiDEBUG_TOOL (g_stiSSLDebug,
						stiTrace ("certificate verification failed, this session cannot be reused!!\n");
						stiTrace ("delete the entry of SSL session %p for session Resumption for server %s\n",
						m_sslSessions[foundIP].get (), foundIP.c_str ());
					); // stiDEBUG_TOOL

					m_sslSessions.erase(foundIP);
				 }
				 else
				 {
					stiDEBUG_TOOL (g_stiSSLDebug,
						stiTrace ("no current session for server %p\n", serverIP.c_str ());
					); // stiDEBUG_TOOL
				 }
			}
		}
		else
		{
			stiDEBUG_TOOL (g_stiSSLDebug,
				if (!foundIP.empty ())
				{
					stiTrace ("a session %p was reused for %s!!!\n", m_sslSessions[foundIP].get (), foundIP.c_str ());
				}
				else
				{
					stiTrace ("a session was reused but the IP was not found for server %s!!!\n", serverIP.c_str ());
				}
			); // stiDEBUG_TOOL
		}
	} // end if (poHTTPService->IsSSLUsed ())
}


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPFormPostHandle
//
// Description: Handles HTTP Post operation
//
// Abstract:
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPFormPostHandle (
	IN CstiEvent* poEvent)	// Event data
{
	EstiResult 	eResult = estiERROR;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPFormPostHandle);

	// Retrieve the HTTPPost info and URL
	SHTTPFormPost * pstPostInfo =
		(SHTTPFormPost *)((CstiEvent*)poEvent)->ParamGet ();
	char * szURL = pstPostInfo->szURL;

	// Create a service object
	CstiHTTPService * poHTTPService = ServiceAdd (
		pstPostInfo->pRequestor,
		pstPostInfo->unRequestNum,
		pstPostInfo->pfnCallback,
		pstPostInfo->pParam1,
		pstPostInfo->bUseSSL);
	if (poHTTPService != NULL)
	{
		eResult = estiOK;

		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("HTTPFormPostHandle: Starting HTTP Post...\n");
		); // stiDEBUG_TOOL

		// Attempt to resolve the URL
		EURLParseResult eURLResult = HTTPURLResolve (
			poHTTPService, szURL);

		// Could we resolve the URL?
		if (eURL_OK == eURLResult)
		{

			auto nFoundPos = reuseSessionFind (poHTTPService, m_ResolvedServerIPList);

			// Yes! Open the socket
			EstiResult eTryResult = poHTTPService->SocketOpen (
				m_ResolvedServerIPList, m_szPort);

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPFormPostHandle: Done with SocketOpen fd=%d\n",
					poHTTPService->SocketGet ());
			); // stiDEBUG_TOOL

			// Did we open the socket?
			if (estiOK == eTryResult)
			{

				sslSessionReuse (poHTTPService, nFoundPos, m_ResolvedServerIPList[0]);

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("HTTPFormPostHandle: Begining HTTP\n");
				); // stiDEBUG_TOOL

				// Yes! Start the HTTP.
				eTryResult = poHTTPService->HTTPFormPostBegin (
					m_szFile, (const SHTTPVariable*)pstPostInfo->astVariables);

				// Did it succeed?
				if (estiERROR != eTryResult)
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("HTTPFormPostHandle: HTTPBegin success\n");
					); // stiDEBUG_TOOL
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("HTTPFormPostHandle: HTTPBegin failed!\n");
					); // stiDEBUG_TOOL
				} // end else
			} // end if
			else
			{
				// No! The socket open failed.
				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("HTTPFormPostHandle: SocketOpen failed!\n");
				); // stiDEBUG_TOOL
			} // end else
		} // end if

		// If the post did not succeed...
		if (!poHTTPService->WaitingCheck ())
		{
			// Remove the service, since it is no longer needed.
			ServiceRemove (poHTTPService);
		} // end if
	} // end if

	// Free the strings allocated by the message sender
	stiHEAP_FREE (pstPostInfo->astVariables);
	stiHEAP_FREE (pstPostInfo);

	return (eResult);
} // end CstiHTTPTask::HTTPFormPostHandle
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPPostHandle
//
// Description: Handles HTTP Post operation
//
// Abstract:
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPPostHandle (
	CstiEvent* poEvent)	// Event data
{
	EstiResult 	eResult = estiERROR;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPPostHandle);
	

	// Retrieve the HTTPPost info and URL
	auto  pstPostInfo =
		(SHTTPPost *)(((CstiEvent*)poEvent)->ParamGet ());

	// Create a service object
	CstiHTTPService * poHTTPService = ServiceAdd (
		pstPostInfo->pRequestor,
		pstPostInfo->unRequestNum,
		pstPostInfo->pfnCallback,
		pstPostInfo->pParam1,
		pstPostInfo->bUseSSL);

	if (nullptr != poHTTPService)
	{
		auto pstPostServiceData = new SHTTPPostServiceData;

		if (nullptr != pstPostServiceData)
		{

			pstPostServiceData->nPort = 0;
			pstPostServiceData->postInfo = *pstPostInfo;

			pstPostServiceData->poHTTPService = poHTTPService;
			poHTTPService = nullptr;

			m_poUrlResolveTask->httpUrlResolve (pstPostServiceData);
			pstPostServiceData = nullptr;

			eResult = estiOK;
		}

		delete pstPostServiceData;
	} // end if

	// free memory
	delete pstPostInfo;
	delete poHTTPService;
	
	return (eResult);
} // end CstiHTTPTask::HTTPPostHandle


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPGetHandle
//
// Description: Handles HTTP Get operation
//
// Abstract:
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPGetHandle (
	IN CstiEvent* poEvent)	// Event data
{
	EstiResult 	eResult = estiERROR;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPGetHandle);

	// Retrieve the URL
	SHTTPGet * pstGetInfo =
		(SHTTPGet *)((CstiEvent*)poEvent)->ParamGet ();

	// Create a new service object.
	CstiHTTPService * poHTTPService = ServiceAdd (
		pstGetInfo->pRequestor,
		pstGetInfo->unRequestNum,
		pstGetInfo->pfnCallback,
		pstGetInfo->pParam1,
		pstGetInfo->bUseSSL);    

	// Could we create a service object?
	if (poHTTPService != NULL)
	{
		eResult = estiOK;

		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("HTTPGetHandle: Starting Get...\n");
		); // stiDEBUG_TOOL

		// Attempt to resolve the URL
		EURLParseResult eURLResult = HTTPURLResolve (
			poHTTPService,	pstGetInfo->szURL);

		// Could we resolve the URL?
		if (eURL_OK == eURLResult)
		{
			auto nFoundPos = reuseSessionFind (poHTTPService, m_ResolvedServerIPList);

			// Yes! Open the socket
			EstiResult eTryResult = poHTTPService->SocketOpen (
				m_ResolvedServerIPList, m_szPort);

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPGetHandle: Done with SocketOpen fd=%d\n",
					poHTTPService->SocketGet ());
			); // stiDEBUG_TOOL

			// Did we open the socket?
			if (estiOK == eTryResult)
			{

				sslSessionReuse (poHTTPService, nFoundPos, m_ResolvedServerIPList[0]);

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("HTTPGetHandle: Begining HTTP\n");
				); // stiDEBUG_TOOL

				// Yes! Start the HTTP.
				eTryResult = poHTTPService->HTTPGetBegin (m_szFile);

				// Did it succeed?
				if (estiERROR != eTryResult)
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("HTTPGetHandle: HTTPBegin success\n");
					); // stiDEBUG_TOOL
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("HTTPGetHandle: HTTPBegin failed!\n");
					); // stiDEBUG_TOOL
				} // end else
			} // end if
		} // end if

		// If the get did not succeed...
		if (!poHTTPService->WaitingCheck ())
		{
			// Remove the service since it is no longer needed.
			ServiceRemove (poHTTPService);
		} // end if
	} // end if

	// Free the string allocated by the message sender
	delete pstGetInfo;
	pstGetInfo = NULL;

	return (eResult);
} // end CstiHTTPTask::HTTPGetHandle
#endif

//:-----------------------------------------------------------------------------
//:	Other message handlers
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HeartbeatHandle
//
// Description: Handles a heartbeat message and responds
//
// Abstract:
//	This function responds to an heartbeat message from the system monitor by
//	sending a message that its pipe is OK and messages are being processed
//	normally.
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
EstiResult CstiHTTPTask::HeartbeatHandle (
	IN CstiEvent* poEvent)	// Pointer to the heartbeat event message
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HeartbeatHandle);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("HTTP: HeartbeatHandle\n");
	); // stiDEBUG_TOOL
	
	return estiOK;
} // end CstiHTTPTask::HeartbeatHandle

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPActivityTimerHandle
//
// Description: Handles check for inactivity
//
// Abstract:
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::TimerHandle (
	CstiEvent* poEvent)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPActivityTimerHandle);

	auto  Timer = (stiWDOG_ID)((CstiExtendedEvent *)poEvent)->ParamGet (0);
	//stiEVENTPARAM EventParam = ((CstiExtendedEvent *)poEvent)->ParamGet (1);

	if (Timer == m_wdidActivityTimer)
	{
		std::lock_guard<std::mutex> lock (m_serviceMutex);

		// Add all of the entries that are in the array
		for (auto &service: m_apoHTTPService)
		{
			// Check for activity on this service
			EstiBool bActivity = service->ActivityCheck (nHTTP_ACTIVITY_MAXCOUNT);

			if (!bActivity)
			{
				if (estiFALSE == service->Cancelled())
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace (
							"HTTPActivityTimerHandle: Abandoning and cancelling service %p "
							"due to inactivity!\n", service.get ());
					); // stiDEBUG_TOOL

					// Tell the service that it is being abandoned
					service->Abandon ();

					// cancel this service
					service->Cancel ();
				}
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace (
							"HTTPActivityTimerHandle: Do nothing for service %p "
							"due to inactivity because it has been cancelled!\n", service.get ());
					); // stiDEBUG_TOOL
				}
			} // end if
		} // end for

		TimerStart (m_wdidActivityTimer, nHTTP_ACTIVITY_TIMEOUT, 0);
	} // end if

	return (estiOK);
} // end CstiHTTPTask::HTTPActivityTimerHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPCancelHandle
//
// Description: Asynchronously cancels a request
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiHTTPTask::HTTPCancelHandle (
	IN CstiEvent* poEvent)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPCancelHandle);

	auto  unRequestNum =
		(unsigned int)((CstiExtendedEvent*)poEvent)->ParamGet (0);
	auto pRequestor =
		(void *)((CstiExtendedEvent*)poEvent)->ParamGet (1);

	std::lock_guard<std::mutex> lock (m_serviceMutex);

	// For all of the entries that are in the array...
	for (auto &service: m_apoHTTPService)
	{
		// Yes! Does it match the request?
		if (service->RequestorInfoMatch (pRequestor, unRequestNum))
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace (
					"CstiHTTPTask::HTTPCancelHandle: Cancelling Service %p...\n", service.get ());
			); // stiDEBUG_TOOL

			// Yes! Cancel it.
			service->Cancel ();

			break;
		} // end if
	} // end for

	return (estiOK);
} // end CstiHTTPTask::HTTPCancelHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPUrlResolveSuccessHandle
//
// Description: Handles HTTP URL resolving succes operation
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiHTTPTask::HTTPUrlResolveSuccessHandle (
	IN CstiEvent* poEvent)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPUrlResolveSuccessHandle);

		stiDEBUG_TOOL (g_stiHTTPTaskDebug,    
		stiTrace("stiHTTPTask::HTTPUrlResolveSuccessHandle\n");
	);
  
	// Retrieve the URL
	auto  pstPostServiceData =
		(SHTTPPostServiceData *)((CstiEvent*)poEvent)->ParamGet ();

	if (pstPostServiceData)
	{
		auto pstPostInfo = &pstPostServiceData->postInfo;
		auto poHTTPService = pstPostServiceData->poHTTPService;

		if (nullptr != pstPostInfo && nullptr != poHTTPService)
		{

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("\tURLSuccess: Service %p\n", poHTTPService);
				stiTrace ("\tURLSuccess: In: %s\n", pstPostInfo->url.c_str ());
				for (auto it = pstPostServiceData->ResolvedServerIPList.begin (); it != pstPostServiceData->ResolvedServerIPList.end (); ++it)
				{
						stiTrace ("\tURLSuccess: Server: %s\n", it->c_str ());
				}  // end for
				stiTrace ("\tURLSuccess: Port: %d\n", pstPostServiceData->nPort);
				stiTrace ("\tURLSuccess: File: %s\n", pstPostServiceData->File.c_str ());
			); // stiDEBUG_TOOL


			auto foundIP = reuseSessionFind (poHTTPService, pstPostServiceData->ResolvedServerIPList);

			stiHResult hTryResult;

			// Yes! Open the socket
			hTryResult = poHTTPService->SocketOpen ();

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("\tURLSuccess: Done with SocketOpen fd=%d\n",
					poHTTPService->SocketGet ());
			); // stiDEBUG_TOOL

			// Did we open the socket?
			if (!stiIS_ERROR (hTryResult))
			{
				sslSessionReuse (poHTTPService, foundIP, pstPostServiceData->ResolvedServerIPList[0]);

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("\tURLSuccess: Beginning HTTP\n");
				); // stiDEBUG_TOOL

				// Yes! Start the HTTP.
				EstiResult eTryResult = poHTTPService->HTTPPostBegin (
					pstPostServiceData->File, pstPostInfo->bodyData);

				// Did it succeed?
				if (estiERROR != eTryResult)
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("\tURLSuccess: HTTPBegin success\n");
					); // stiDEBUG_TOOL
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("\tURLSuccess: HTTPBegin failed!\n");
					); // stiDEBUG_TOOL
				} // end else
			} // end if
			else
			{
				// No! The socket open failed.
				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("\tURLSuccess: SocketOpen failed!\n");
				); // stiDEBUG_TOOL
			} // end else
		} // end if

		// If the post did not succeed...
		if (poHTTPService && !poHTTPService->WaitingCheck ())
		{
			// Remove the service, since it is no longer needed.
			ServiceRemove (poHTTPService);
		} // end if

		// NOTE: The HttpService object will be deleted when the service is removing
		pstPostServiceData->poHTTPService = nullptr;

		delete pstPostServiceData;
		pstPostServiceData = nullptr;
	}
  
	stiDEBUG_TOOL (g_stiHTTPTaskDebug,   
		stiTrace("CstiHTTPTask::HTTPUrlResolveSuccessHandle DONE\n\n");
	);
  
	return (estiOK);
} // end CstiHTTPTask::HTTPUrlResolveSuccessHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPUrlResolveFailHandle
//
// Description: Handles HTTP URL resolving succes operation
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiHTTPTask::HTTPUrlResolveFailHandle (
	IN CstiEvent* poEvent)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPUrlResolveFailHandle);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,    
		stiTrace("CstiHTTPTask::HTTPUrlResolveFailHandle\n");
	);

	// Retrieve the URL
	auto pstPostServiceData =
		(SHTTPPostServiceData *)((CstiEvent*)poEvent)->ParamGet ();

	if (pstPostServiceData)
	{
		auto poHTTPService = pstPostServiceData->poHTTPService;

		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("\tURLFail: Service %p\n", poHTTPService);
		);

		// If the post did not succeed...
		if (!poHTTPService->WaitingCheck ())
		{
			// Remove the service, since it is no longer needed.
			ServiceRemove (poHTTPService);
		} // end if

		// NOTE: The HttpService object will be deleted when the service is removing
		pstPostServiceData->poHTTPService = nullptr;

		delete pstPostServiceData;
		pstPostServiceData = nullptr;
	}
  
	stiDEBUG_TOOL (g_stiHTTPTaskDebug,  
		stiTrace("CstiHTTPTask::HTTPUrlResolveFailHandle DONE\n\n");
	);

	return (estiOK);
}


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPReplyReceive
//
// Description:	receives the request from the HTTP Server
//
// Abstract:
//	Once the request is received from the server it is parsed to find the
//	ip address.
//
// Returns:
//	estiOK - success
//	estiFALSE - failure
//
EstiResult CstiHTTPTask::HTTPReplyReceive (
	std::unique_ptr<CstiHTTPService> &service)
{
	EstiResult	eReportResult = estiOK;

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("HTTPReplyReceive: Service %p\n", service.get ());
	); // stiDEBUG_TOOL

	// Are we in the waiting state?
	if (service->WaitingCheck ())
	{
		// Yes! Ask the service provider to handle the socket read.
		EHTTPServiceResult eIPResult =
			service->HTTPProgress (nullptr, 0, nullptr, nullptr);

		// Are we done with the HTTP?
		if (eHTTP_TOOLS_COMPLETE == eIPResult)
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPReplyReceive: Complete.\n");
			); // stiDEBUG_TOOL
		} // end if
		// Was there an error?
		else if (eHTTP_TOOLS_ERROR == eIPResult)
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPReplyReceive: Error.\n");
			); // stiDEBUG_TOOL

			eReportResult = estiERROR;
		} // end else
		// Was there a cancel?
		else if (eHTTP_TOOLS_CANCELLED == eIPResult)
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPReplyReceive: Cancelled.\n");
			); // stiDEBUG_TOOL
		} // end else
		else
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTPReplyReceive: Continue\n");
			); // stiDEBUG_TOOL
		} // end else
	} // end if

	return (eReportResult);
} // end CstiHTTPTask::HTTPReplyReceive


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPURLResolve
//
// Description: Resolve the url for HTTP.
//
// Abstract:
//	This function receives the url to resolve from the message queue.  The
//	url is split into a host name and file.  The host name is then resolved.
//
// Returns:
//	EURLParseResult
//
EURLParseResult CstiHTTPTask::HTTPURLResolve (
	IN CstiHTTPService * poService,	// Pointer to service object
	IN const char * szUrl)	// URL to resolve
{
	EURLParseResult eURLResult = eINVALID_URL;
	EstiBool	bParsed = estiFALSE;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPURLResolve);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("HTTPURLResolve: Resolving URL...\n");
	); // stiDEBUG_TOOL

	// Was there a URL specified?
	if (NULL != szUrl)
	{
		// Yes! Ask the service provider to parse the URL.
		m_ResolvedServerIPList.clear ();
		m_szPort = NULL;
		m_szFile = NULL;
		eURLResult = poService->URLParse (
				szUrl,
				m_ResolvedServerIPList,
				m_szPort,
				m_szFile);

		// Did it succeed?
		if (eURL_OK == eURLResult && !m_ResolvedServerIPList.empty ())
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				for (std::vector<std::string>::iterator it = m_ResolvedServerIPList->begin (); it != m_ResolvedServerIPList->end (); ++it)
				{
					stiTrace ("HTTPURLResolve: Server from URLParse = %s\n", it->c_str ());
				}
			); // stiDEBUG_TOOL

			// Yes!
			bParsed = estiTRUE;
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				for (std::vector<std::string>::iterator it = m_ResolvedServerIPList->begin (); it != m_ResolvedServerIPList->end (); ++it)
				{
					stiTrace ("HTTPURLResolve: URLParse failed. Server = %s\n", it->c_str ());
				}
			); // stiDEBUG_TOOL

			m_ResolvedServerIPList.clear ();
			m_szPort = NULL;
			m_szFile = NULL;
		} // end else
	} // end if

	return (eURLResult);
} // end CstiHTTPTask::HTTPURLResolve
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::ServiceAdd
//
// Description: Adds a service object to the internal array
//
// Abstract:
//
// Returns:
//	CstiHTTPService * - new service object, or NULL on error
//
CstiHTTPService * CstiHTTPTask::ServiceAdd (
	void *pRequestor,
	unsigned int unRequestNum,
	PstiHTTPCallback pfnCallback,
	void * pParam1,
	EstiBool bUseSSL)
{
	CstiHTTPService * poReturn = nullptr;

	stiLOG_ENTRY_NAME (CstiHTTPTask::ServiceAdd);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("CstiHTTPTask::ServiceAdd\n");
	); // stiDEBUG_TOOL

	poReturn = new CstiHTTPService;

	if (poReturn != nullptr)
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("\tServiceAdd: Adding Service %p\n", poReturn);
		); // stiDEBUG_TOOL

		// Set the requestor's info
		poReturn->RequestorInfoSet (
			pRequestor,
			unRequestNum,
			pfnCallback,
			pParam1);

		if (bUseSSL)
		{
			poReturn->SSLCtxSet (m_ctx);
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("ServiceAdd: Using SSL - Set SSL_CTX object %p to Service %p\n", m_ctx, poReturn);
			);        
		}
		else
		{
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("ServiceAdd: NOT using SSL\n");
			);      
		}

		std::lock_guard<std::mutex> lock (m_serviceMutex);

		m_apoHTTPService.emplace_back(poReturn);
	} // end if
	else
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("\tServiceAdd: FAILED to allocate Service Object!\n");
		); // stiDEBUG_TOOL
	} // end else

	return (poReturn);
} // end CstiHTTPTask::ServiceAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::ServiceRemove
//
// Description: Removes a service object from the internal array
//
// Abstract:
//
// Returns:
//	EstiError - estiOK if removed. estiERROR if not found in the array, or the
//	object pointer is NULL.
//
EstiResult CstiHTTPTask::ServiceRemove (
	IN CstiHTTPService * poService)
{
	EstiResult eResult = estiERROR;
	stiLOG_ENTRY_NAME (CstiHTTPTask::ServiceRemove);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("ServiceRemove: Removing Service %p\n", poService);
	); // stiDEBUG_TOOL

	// Were we given a valid pointer?
	if (poService != nullptr)
	{
		// Yes! Close the socket (if possible).
		poService->SocketClose ();

		std::lock_guard<std::mutex> lock (m_serviceMutex);

		auto it = std::find_if (m_apoHTTPService.begin (), m_apoHTTPService.end (),
			[&](std::unique_ptr<CstiHTTPService> &p)
			{
				return p.get () == poService;
			});

		if (it != m_apoHTTPService.end ())
		{
			m_apoHTTPService.erase (it);
		}
	} // end if

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("ServiceRemove: %d services left in list.\n", m_apoHTTPService.size ());
	); // stiDEBUG_TOOL

	return (eResult);
} // end CstiHTTPTask::ServiceRemove


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPServiceRemove
//
// Description:  Tells the task to remove the http service object
//
// Abstract:
//  Post a message to the HTTP task requesting it remove the
//	HTTP service
//
//  estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPServiceRemove (
	IN CstiHTTPService *poHTTPService)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPRemove);

	return (ServiceRemove (poHTTPService));
} // end CstiHTTPTask::HTTPServiceRemove


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPUrlResolveSuccess
//
// Description:  Tells the task that the URL resolving has been succeeded
//
// Abstract:
//	Post a message to the HTTP requesting it begin HTTP processing based on
//  the URL resolving result
//
//  estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPUrlResolveSuccess (
	IN SHTTPPostServiceData *poServiceData)
{
	EstiResult eResult = estiOK;
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPUrlResolveSuccess);

	// Create the event
	CstiEvent oEvent (CstiHTTPTask::estiHTTP_TASK_URL_RESOLVE_SUCCESS, (stiEVENTPARAM)poServiceData);

	// Send the message
	stiHResult hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	if (stiIS_ERROR (hResult))
	{
		eResult = estiERROR;
		stiASSERT (estiFALSE);
	}

	return (eResult);

} // end CstiHTTPTask::HTTPUrlResolveSuccess

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPUrlResolveFail
//
// Description:  Tells the task that the URL resolving has been failed
//
// Abstract:
//	Post a message to the HTTP requesting it begin HTTP processing based on
//  the URL resolving result
//
//  estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPUrlResolveFail (
	IN SHTTPPostServiceData *poServiceData)
{
	EstiResult eResult = estiOK;
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPUrlResolveFail);

	// Create the event
	CstiEvent oEvent (CstiHTTPTask::estiHTTP_TASK_URL_RESOLVE_FAIL,
					   (stiEVENTPARAM)poServiceData);

	// Send the message
	stiHResult hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	if (stiIS_ERROR (hResult))
	{
		eResult = estiERROR;
		stiASSERT (estiFALSE);
	}

	return (eResult);

} // end CstiHTTPTask::HTTPUrlResolveFail


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPGet
//
// Description: Tells the task to start HTTP Get message
//
// Abstract:
//	Post a message to the HTTP requesting it begin HTTP Get
//
//	NOTE: The Callback function specified by pfnCallback MUST free the
//	returned response structure by calling HTTPResultFree. This parameter can
//	be NULL to indicate that a callback is not desired.
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPGet (
	void *pRequestor,	// Requestor of action - can be anything,
						// but should be unique to process calling
						// this function. TaskId is a good choice.
	unsigned int unRequestNum,	// Request number - indicates which request
								// this represents.
	const char * szURL,			// URL of get request.
	PstiHTTPCallback pfnCallback, // Callback function that will process the
								// result (or NULL)
	void * pParam1,				// Optional pointer param - will be returned
								// in Callback function.
	EstiBool bUseSSL)
{
	EstiResult eResult = estiOK;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPGet);

	// Allocate memory for the HTTPGet structure
	// NOTE: These will be freed in the HTTPGet function.
	SHTTPGet * pstBuffer = (SHTTPGet *)stiHEAP_ALLOC (sizeof (SHTTPGet));

	// Copy in the URL
	pstBuffer->pRequestor = pRequestor;
	pstBuffer->unRequestNum = unRequestNum;
	strcpy (pstBuffer->szURL, szURL);

	// Copy the callback function and params
	pstBuffer->pfnCallback = pfnCallback;
	pstBuffer->pParam1 = pParam1;
	pstBuffer->bUseSSL = bUseSSL;   

	CstiEvent oEvent (CstiHTTPTask::estiHTTP_TASK_GET, (stiEVENTPARAM)pstBuffer);

	// Send the message
	stiHResult hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	if (stiIS_ERROR (hResult))
	{
		eResult = estiERROR;
		stiASSERT (estiFALSE);
	}

	return (eResult);
	
} // end CstiHTTPTask::HTTPGet
#endif


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPFormPost
//
// Description: Tells the task to post HTTP message
//
// Abstract:
//	Post a message to the HTTP task requesting it begin HTTP Post
//
//	NOTE: The Callback function specified by pfnCallback MUST free the
//	returned response structure by calling HTTPResultFree. This parameter can
//	be NULL to indicate that a callback is not desired.
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
EstiResult CstiHTTPTask::HTTPFormPost (
	void *pRequestor,		// Requestor of action - can be anything,
							// but should be unique to process calling
							// this function. TaskId is a good choice.
	unsigned int unRequestNum,		// Request number - indicates which request
									// this represents.
	const char * szURL,				// URL of post request.
	const SHTTPVariable * astVariables, // Pointer to form variable structure.
	PstiHTTPCallback pfnCallback,	// Callback function that will process the
									// result (or NULL)
	void * pParam1,					// Optional pointer param - will be returned
									// in Callback function.
	EstiBool bUseSSL)
{
	EstiResult eResult = estiOK;

	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPFormPost);

	// Allocate memory for the HTTPPost structure
	// NOTE: These will be freed in the HTTPPost function.
	SHTTPFormPost * pstBuffer =
		(SHTTPFormPost *)stiHEAP_ALLOC (sizeof (SHTTPFormPost));

	// Count the number of variables that were sent in.
	int nCount = 0;
	while (astVariables[nCount++].szName[0] != 0)
	{
	}

	// Allocate enough memory to copy all of the variables, plus the blank.
	// NOTE: These will be freed in the HTTPFormPost function.
	int nSize = sizeof (SHTTPVariable) * (nCount + 1);
	SHTTPVariable * pstVariables = (SHTTPVariable *)stiHEAP_ALLOC (nSize);

	// Copy the variables to the new memory
	memcpy (pstVariables, astVariables, nSize);

	// Copy in the URL and variables
	pstBuffer->pRequestor = pRequestor;
	pstBuffer->unRequestNum = unRequestNum;
	strcpy (pstBuffer->szURL, szURL);
	pstBuffer->astVariables = pstVariables;

	// Copy the callback function and params
	pstBuffer->pfnCallback = pfnCallback;
	pstBuffer->pParam1 = pParam1;
	pstBuffer->bUseSSL = bUseSSL;   

	// Create the event
	CstiEvent oEvent (
		CstiHTTPTask::estiHTTP_TASK_FORM_POST, (stiEVENTPARAM)pstBuffer);

	// Send the message
	stiHResult hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	if (stiIS_ERROR (hResult))
	{
		eResult = estiERROR;
		stiASSERT (estiFALSE);
	}

	return (eResult);
} // end CstiHTTPTask::HTTPFormPostHTTPFormPost
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::HTTPPost
//
// Description: Tells the task to post HTTP message
//
// Abstract:
//	Post a message to the HTTP task requesting it begin HTTP Post
//
//	NOTE: The Callback function specified by pfnCallback MUST free the
//	returned response structure by calling HTTPResultFree. This parameter can
//	be NULL to indicate that a callback is not desired.
//
// Returns:
//	estiOK - success
//	estiERROR - failure
//
stiHResult CstiHTTPTask::HTTPPost (
	void *pRequestor,		// Requestor of action - can be anything,
							// but should be unique to process calling
							// this function. TaskId is a good choice.
	unsigned int unRequestNum,		// Request number - indicates which request
									// this represents.
	const char * pszURL,			// URL of post request.
	const char * pszUrlAlt,			// Alternate URL of post request.
	const char * pszData, 			// Pointer to body data.
	PstiHTTPCallback pfnCallback,	// Callback function that will process the
									// result (or NULL)
	void * pParam1,					// Optional pointer param - will be returned
									// in Callback function.
	EstiBool bUseSSL)
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPPost);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	// Allocate memory for the HTTPPost structure
	// NOTE: These will be freed in the HTTPPost function.
	auto post = new SHTTPPost;

	// Copy in the URL and variables
	post->pRequestor = pRequestor;
	post->unRequestNum = unRequestNum;
	post->url = pszURL;
	if (pszUrlAlt && pszUrlAlt[0])
	{
		post->urlAlt = pszUrlAlt;
	}

	post->bodyData = pszData;

	// Copy the callback function and params
	post->pfnCallback = pfnCallback;
	post->pParam1 = pParam1;
	post->bUseSSL = bUseSSL;

	// Create the event
	CstiEvent oEvent (CstiHTTPTask::estiHTTP_TASK_POST, (stiEVENTPARAM)post);

	// Send the message
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
} // end CstiHTTPTask::HTTPPost


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPTask::SecurityExpirationDateGet
//
// Description: Gets the security expiration date string
//
// Abstract:
//
// Returns:
//
std::string CstiHTTPTask::SecurityExpirationDateGet (
	CertificateType certificateType //!< The certificate to get the expiration date of.
) const
{
	stiLOG_ENTRY_NAME (CstiHTTPTask::HTTPResultFree);

	return stiSSLSecurityExpirationDateGet (certificateType);
} // end CstiHTTPTask::SecurityExpirationDateGet


stiHResult CstiHTTPTask::UrlResolveCallback(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	auto pThis = (CstiHTTPTask *)CallbackParam;
	
	return pThis->Callback (n32Message, MessageParam);
}


//:-----------------------------------------------------------------------------
//:	Debugging utilities
//:-----------------------------------------------------------------------------
#ifdef stiDEBUGGING_TOOLS
EstiResult stiHTTPCallbackFunction (
	IN EstiHTTPState eState,
	IN SHTTPResult * pstResult,
	void * pParam1)
{
	stiLOG_ENTRY_NAME (stiHTTPCallbackFunction);

	stiTrace ("In stiHTTPCallbackFunction: \n"
		"\tpParam1: %p\n",
		pParam1);

	switch (eState)
	{
//		case eHTTP_INVALID:
//			stiTrace ("\tBad state sent to stiHTTPCallbackFunction!\n");
//			break;

		case eHTTP_DNS_ERROR:
			stiTrace ("\teHTTP_DNS_ERROR sent to stiHTTPCallbackFunction!\n");
			break;      

		case eHTTP_ERROR:
			stiTrace ("\teHTTP_ERROR sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_CANCELLED:
			stiTrace ("\teHTTP_CANCELLED sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_ABANDONED:
			stiTrace ("\teHTTP_ABANDONED sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_IDLE:
			stiTrace ("\teHTTP_IDLE sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_DNS_RESOLVING:
			stiTrace ("\teHTTP_DNS_RESOLVING sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_SENDING:
			stiTrace ("\teHTTP_SENDING sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_WAITING_4_RESP:
			stiTrace ("\teHTTP_WAITING_4_RESP sent to stiHTTPCallbackFunction.\n");
			break;

		case eHTTP_COMPLETE:
			stiTrace ("\teHTTP_COMPLETE sent to stiHTTPCallbackFunction.\n");

			stiTrace ("In stiHTTPCallbackFunction: "
				"HTTP Return code: %d\n",
				pstResult->unReturnCode);

//SLB	stiTrace ("In stiHTTPCallbackFunction:\n"
//SLB		"HTTP Result:\n"
//SLB		"Return code: %d\n"
//SLB		"Requestor: %d\n"
//SLB		"Request #: %d\n"
//SLB		"Content Len: %d\n"
//SLB		"********************\n"
//SLB		"%s\n"
//SLB		"*******************\n",
//SLB		pstResult->unReturnCode,
//SLB		pstResult->pRequestor,
//SLB		pstResult->unRequestNum,
//SLB		pstResult->unContentLen,
//SLB		pstResult->szBody);
			break;

		case eHTTP_TERMINATED:
			stiTrace ("\teHTTP_TERMINATED sent to stiHTTPCallbackFunction.\n");
			break;
	} // end switch

	delete pstResult;
	pstResult = nullptr;

	return (estiOK);
} // end stiHTTPCallbackFunction

#if 0
extern "C" void stiHTTPGet (char * szString);
void stiHTTPGet (char * szString)
{
	static unsigned int unRequest = 0;

//SLB	taskPrioritySet (taskIdSelf (), 10);

	printf ("Request number: %d\n", unRequest);
	g_oHTTPTask.HTTPGet (
		0,
		unRequest,
		szString,
		stiHTTPCallbackFunction,
		(void *)0x33,
		44);
//SLB	g_oHTTPTask.HTTPCancel (0, unRequest);

	unRequest++;
}
#endif

#if 0
extern "C" void stiHTTPFormPost (char * szString);
void stiHTTPFormPost (char * szString)
{
	static unsigned int unRequest = 0;

	SHTTPVariable astVariables[3];
	strcpy (astVariables[0].szName, "account");
	strcpy (astVariables[0].szValue, "WSXPXZQWFQJ");
	strcpy (astVariables[1].szName, "toEmail1");
	strcpy (astVariables[1].szValue, "sbrooksby@sorenson.com");
	strcpy (astVariables[2].szName, "");
	strcpy (astVariables[2].szValue, "");

	printf ("Request number: %d\n", unRequest);
	g_oHTTPTask.HTTPFormPost (
		0,
		unRequest++,
		szString,
		astVariables,
		stiHTTPCallbackFunction,
		(void *)0x33,
		44);
}
#endif

#if 0
extern "C" void stiHTTPPost (char * szString);
void stiHTTPPost (char * szString)
{
	static unsigned int unRequest = 0;

	printf ("Request number: %d\n", unRequest);
	g_oHTTPTask.HTTPPost (
		0,
		unRequest++,
		szString,
		"<RequestStockPrice Symbol='csco'/>",
		stiHTTPCallbackFunction,
		(void *)0x33,
		44);
}
#endif
#endif // ifdef stiDEBUGGING_TOOLS

// end file CstiHTTPTask.cpp
