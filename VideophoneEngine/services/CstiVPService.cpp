/*!
 * \file CstiVPService.cpp
 * \brief Base class for all enterpise services classes
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiVPService.h"
#include "ixml.h"
#include "stiTools.h"
#include "CstiVPServiceRequest.h"
#include "stiCoreRequestDefines.h"
#include "stiTrace.h"
#include "stiRemoteLogEvent.h"
#include "CstiHTTPTask.h"
#include "CstiExtendedEvent.h"
#include "CstiUniversalEvent.h"
#include <cctype>					// standard type definitions.
#include <cstring>

#ifdef stiDEBUGGING_TOOLS
	#define stiSVC_DEBUG_TOOL(ClassPtr, Stuff) {\
		const char *pszService; \
		if ((ClassPtr)->IsDebugTraceSet (&pszService)) \
		{Stuff}}
#else
	#define stiSVC_DEBUG_TOOL(ClassPtr, Stuff)
#endif // stiDEBUGGING_TOOLS

//#undef stiLOG_ENTRY_NAME
//#define stiLOG_ENTRY_NAME(name) stiTrace ("ENTER %s\n", #name);

//#define VP_SERVICE_DEADLOCK_DETECTOR

CstiVPService::CstiVPService(
	CstiHTTPTask *pHTTPTask,
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam,
	size_t CallbackFromId,
	uint32_t un32MsgMaxLen,
	const char *pszResponseType,
	const char *pszName,
	unsigned int unMaxTrysOnServer,
	unsigned int unBaseRetryTimeOut,
	unsigned int unMaxRetryTimeOut,
	int nMaxServers)
:
	CstiEventQueue (
		pszName),
	m_bAuthenticated (estiFALSE),
	m_pHTTPTask (pHTTPTask),
	m_nCurrentPrimaryServer (0),
	m_un32MaxMessageLength (un32MsgMaxLen),
	m_eState (eUNITIALIZED),
	m_unPauseCount (0),
	m_unResponsePending (0),
	m_bCancelPending (false),
	m_unErrorCount (0),
	m_unRetryCount (0),
	m_unErroredRequest (0),
	m_retryTimer(1000, this),
	m_unMaxTrysOnServer (unMaxTrysOnServer),
	m_unBaseRetryTimeOut (unBaseRetryTimeOut),
	m_unMaxRetryTimeOut (unMaxRetryTimeOut),
	m_unRequestId (0),
	m_eSSLFlag (stiSSL_DEFAULT_MODE),
	m_bPassError(estiFALSE),
	m_pszResponseType (pszResponseType),
	m_bShutdownInitiated (false),
	m_pfnObjectCallback(pfnAppCallback),
	m_ObjectCallbackParam(CallbackParam),
	m_CallbackFromId(CallbackFromId)
{
	m_signalConnections.push_back (m_retryTimer.timeoutSignal.Connect (std::bind (&CstiVPService::requestSendHandle, this)));
}


///
///\brief Strips all non-alphanumeric characters from the input string
///
static void StripNonAlphaNumeric (
	const std::string &UniqueID, ///< The input string to strip
	std::string *pUrlID)		///< The output string
{
	size_t nLength = UniqueID.length ();

	pUrlID->clear ();

	for (size_t i = 0; i < nLength; i++)
	{
		if (isalpha (UniqueID[i]) || isdigit (UniqueID[i]))
		{
			pUrlID->append (1, UniqueID[i]);
		}
	}
}


stiHResult CstiVPService::Initialize(
	const std::string *serviceContacts,
	const std::string &uniqueID)
{
	stiLOG_ENTRY_NAME (CstiVPService::Initialize);
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Set the unique id
	//
	m_uniqueID = uniqueID;

	//
	// Set the URL Identifier
	//
	StripNonAlphaNumeric (m_uniqueID, &m_URLID);

	m_messageBuffer.resize (sizeof(uint32_t) * m_un32MaxMessageLength + 1);

	ServiceContactsSet (serviceContacts);

	return hResult;
}


void CstiVPService::EmptyQueue()
{
	stiLOG_ENTRY_NAME (CstiVPService::EmptyQueue);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	std::list<CstiRequestQueueItem>::iterator i;

	for (i = m_Requests.begin (); i != m_Requests.end (); )
	{
		if (m_unResponsePending == (*i).m_pRequest->requestIdGet ())
		{
			//
			// Don't delete the request that is being processed.
			//
			++i;
		}
		else
		{
			RequestRemoved ((*i).m_pRequest->requestIdGet ());

			// Yes! Get the request
			delete (*i).m_pRequest;

			i = m_Requests.erase (i);
		}
	} // end for
}


void CstiVPService::Cleanup()
{
	stiLOG_ENTRY_NAME (CstiVPService::Cleanup);

	// Free any pending requests
	EmptyQueue();

	clientTokenSet ({});
}


/*!\brief Shutdown the VP Service
 *
 * Checks to see if there are pending responses before shutting down.
 * If there are responses pending this method will block until the responses
 * are complete.
 *
 */
stiHResult CstiVPService::Shutdown ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (eventLoopIsRunning())
	{
		m_bShutdownInitiated = true;

		//
		// Cancel any active timers
		//
		m_retryTimer.stop ();

		//
		// If we have an outstanding response then we need to wait for it.
		// Otherwise, when the response is complete and the callback is called
		// this object may have already been destroyed.
		//
		std::unique_lock<std::mutex> lock (m_pendingRequestMutex);
		m_WaitForPendingCond.wait (lock, [this]() { return m_unResponsePending == 0; });

		StopEventLoop ();
		PostEvent (std::bind(&CstiVPService::ShutdownHandle, this));
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::ShutdownHandle
//
// Description: Shuts down the Core Service task.
//
// Abstract:
//
// Returns:
//	none
//
stiHResult CstiVPService::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiVPService::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// We should not have a pending response at this point.
	//
	stiASSERT (m_unResponsePending == 0);

	m_unErroredRequest = 0;

	// Free any pending requests
	EmptyQueue();

	//
	// Set the state back to uninitialized
	//
	m_eState = eUNITIALIZED;

	// Shutdown has completed, so set the ShutdownInitiated flag back to false
	m_bShutdownInitiated = false;

	return (hResult);

} // end CstiVPService::ShutdownHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::PauseSet
//
// Description: Pauses or unpauses the service
//
// Abstract:
//
// Returns:
//	none
//
void CstiVPService::PauseSet (
	bool bPause)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (bPause)
	{
		//
		// This code currently limits the counter to only increase by one.  This was
		// added to overcome issues with multiple calls to pause and unpause the system.
		// Ideally the counter would be allowed to increase and it would only unpause
		// once the counter reached zero again.
		//
		if (m_unPauseCount == 0)
		{
			++m_unPauseCount;
		}
	}
	else
	{
		if (m_unPauseCount > 0)
		{
			--m_unPauseCount;
			if (m_unPauseCount == 0)
			{
				PostEvent(std::bind (&CstiVPService::requestSendHandle, this));
			}
		}
	}

#if 0
	if (m_unPauseCount > 0)
	{
		stiTrace ("Subsystem paused: %p, %d\n", this, m_unPauseCount);
	}
	else
	{
		stiTrace ("Subsystem unpaused: %p\n", this);
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::PauseGet
//
// Description: Returns the current pause state.
//
// Abstract:
//
// Returns:
//	none
//
bool CstiVPService::PauseGet () const
{
	bool bPaused;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	bPaused = (m_unPauseCount > 0);

	return bPaused;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::Startup
//
// Description: This is called to initialize the class.
//
// Abstract:
//	This method is called soon after the object is created to initialize the
//	 object.
//
// Returns:
//
stiHResult CstiVPService::Startup (
	bool bPause)
{
	stiLOG_ENTRY_NAME (CstiVPService::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	PauseSet (bPause);

	//
	// Startup task
	//
	StartEventLoop ();

	stiTESTRESULT ();

	m_unErrorCount = 0;
	m_unRetryCount = 0;
	m_nCurrentPrimaryServer = 0;

	StateEvent(eSE_INITIALIZED);

	if (!m_Requests.empty ())
	{
		PostEvent(std::bind (&CstiVPService::requestSendHandle, this));
	}

STI_BAIL:

	return hResult;
} // end CstiVPService::Startup


stiHResult CstiVPService::StateEvent(
	EStateEvent eEvent)
{
	stiLOG_ENTRY_NAME (CstiVPService::StateEvent);

	stiHResult hResult = stiRESULT_SUCCESS;

/*#ifdef stiDEBUG_OUTPUT //...
	static char msg[256];
	const char* veState[] =
	{
		"UNITIALIZED",
		"DISCONNECTED",
		"CONNECTING",
		"CONNECTED",
		"AUTHENTICATED",
		"DNSERROR"
	}; // EState
	const char* veStateEvent[] =
	{
		"SE_INITIALIZED",
		"SE_REQUEST_SENT",
		"SE_HTTP_RESPONSE",
		"SE_AUTHENTICATED",
		"SE_AUTHENTICATION_FAILED",
		"SE_LOGOUT",
		"SE_HTTP_ERROR",
		"SE_DNS_ERROR",
		"SE_RESTART"
	}; // EStateEvent
	sprintf(msg, "vpEngine::CstiVPService::StateEvent() - m_pCoreService->Login() EState: %s  EStateEvent: %s", veState[m_eState], veStateEvent[eEvent]);
	DebugTools::instanceGet()->DebugOutput(msg);
#endif*/
	switch (m_eState)
	{
		case eUNITIALIZED:

			if (eEvent == eSE_INITIALIZED)
			{
				hResult = StateSet(eDISCONNECTED);

				stiTESTRESULT();
			}
			else
			{
				stiTHROW(stiRESULT_ERROR);
			}

			break;

		case eDISCONNECTED:

			if (eEvent == eSE_REQUEST_SENT)
			{
				hResult = StateSet(eCONNECTING);

				stiTESTRESULT();
			}
			else
			{
				stiTHROW(stiRESULT_ERROR);
			}

			break;

		case eCONNECTING:

			switch (eEvent)
			{
				case eSE_REQUEST_SENT:

					break;

				case eSE_HTTP_RESPONSE:

					if (m_bAuthenticated)
					{
						hResult = StateSet(eAUTHENTICATED);

						stiTESTRESULT();
					}
					else
					{
						hResult = StateSet(eCONNECTED);

						stiTESTRESULT();
					}

					break;

				case eSE_HTTP_ERROR:

					if (m_unErrorCount >= (m_unMaxTrysOnServer + 1))
					{
						hResult = StateSet(eDISCONNECTED);

						stiTESTRESULT();
					}

					break;

				case eSE_RESTART:

					hResult = StateSet(eDISCONNECTED);

					stiTESTRESULT();

					break;

				default:

					//stiTHROW(stiRESULT_ERROR);
					stiTESTCOND_NOLOG(estiFALSE, stiRESULT_ERROR);
			}
			break;

		case eCONNECTED:

			switch (eEvent)
			{
				case eSE_AUTHENTICATED:

					hResult = StateSet(eAUTHENTICATED);

					stiTESTRESULT();

					break;

				case eSE_HTTP_RESPONSE:
				case eSE_REQUEST_SENT:
				case eSE_AUTHENTICATION_FAILED:

					//
					// Do nothing
					//
					break;

				case eSE_HTTP_ERROR:

					hResult = StateSet(eCONNECTING);

					stiTESTRESULT();

					break;

				case eSE_RESTART:

					hResult = StateSet(eDISCONNECTED);

					stiTESTRESULT();

					break;

				default:

					stiTHROW(stiRESULT_ERROR);
			}

			break;

		case eAUTHENTICATED:

			switch (eEvent)
			{
				case eSE_AUTHENTICATED:

					break;

				case eSE_HTTP_RESPONSE:
				case eSE_REQUEST_SENT:

					//
					// Do nothing
					//
					break;

				case eSE_HTTP_ERROR:

					hResult = StateSet(eCONNECTING);

					stiTESTRESULT();

					break;

				case eSE_LOGOUT:
				case eSE_AUTHENTICATION_FAILED:

					m_bAuthenticated = estiFALSE;

					hResult = Callback (estiMSG_CORE_SERVICE_NOT_AUTHENTICATED, nullptr);

					stiTESTRESULT ();

					hResult = StateSet(eCONNECTED);

					stiTESTRESULT();

					break;

				case eSE_RESTART:

					hResult = StateSet(eDISCONNECTED);

					stiTESTRESULT();

					break;

				default:

					stiTHROW(stiRESULT_ERROR);

			}

			break;

		case eDNSERROR:

			switch (eEvent)
			{
				case eSE_HTTP_RESPONSE:

					if (m_bAuthenticated)
					{
						hResult = StateSet(eAUTHENTICATED);

						stiTESTRESULT();
					}
					else
					{
						hResult = StateSet(eCONNECTED);

						stiTESTRESULT();
					}

					break;

				case eSE_HTTP_ERROR:

					break;

				case eSE_RESTART:

					hResult = StateSet(eDISCONNECTED);

					stiTESTRESULT();

					break;

				default:

					stiTHROW(stiRESULT_ERROR);
			}
			break;
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiSVC_DEBUG_TOOL (this,
			stiTrace("CstiVPService::StateEvent (%s) Event: %d, State: %d\n", pszService, eEvent, m_eState);
		);
	}

	return hResult;
} // CstiVPService::StateEvent


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::HTTPResponseParse
//
// Description: Parses the response from the StateNotify Service
//
// Abstract:
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiVPService::HTTPResponseParse (
	unsigned int unRequestId,
	const char *pszResponse)
{
	stiLOG_ENTRY_NAME (CstiVPService::HTTPResponseParse);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiRequestRepeat eRepeatRequest = estiRR_NO; // Keep track of whether we should remove this request
	IXML_Document * pDocument = nullptr;
	int nRet;
	int nPriority = 0;
	
	CstiVPServiceRequest *pRequest = RequestGet(unRequestId, &nPriority);
	int nSystemicError = 0;

	hResult = StateEvent(eSE_HTTP_RESPONSE);

	//
	// Handle the error results carefully so that we behave correctly
	// when receiving bad XML or other errors in the response handling.
	//
	if (!stiIS_ERROR (hResult))
	{
		// Create an XML document from the data that was supplied
		nRet = ixmlParseBufferEx (pszResponse, &pDocument);

		if (IXML_SUCCESS == nRet && pDocument != nullptr)
		{
			hResult = ResponseParse (pDocument, pRequest, unRequestId, nPriority, &eRepeatRequest,
									 &nSystemicError);

			if (stiIS_ERROR (hResult))
			{
				if (pRequest && pRequest->RepeatOnErrorGet ())
				{
					eRepeatRequest = estiRR_DELAYED;
				}
			}
		}
		else
		{
			if (pRequest && pRequest->RepeatOnErrorGet ())
			{
				eRepeatRequest = estiRR_DELAYED;
			}
		}
	}
	else
	{
		if (pRequest && pRequest->RepeatOnErrorGet ())
		{
			eRepeatRequest = estiRR_DELAYED;
		}
	}

	//
	// Should we remove this request?
	// If yes, then remove it and do request
	// removal cleanup.
	//

	if (eRepeatRequest == estiRR_DELAYED)
	{
		if (m_bCancelPending)
		{
			RequestRemove (unRequestId);
		}
		else if (pRequest)
		{
			pRequest->ExpectedResultsReset();
		}

		RequestError ();
	}
	else
	{
		//
		// Clear the error count but only if the successful request
		// was the request that errored before.
		//
		if (m_unResponsePending == m_unErroredRequest)
		{
			m_unErrorCount = 0;
			m_unRetryCount = 0;
			m_unErroredRequest = 0;
		}

		//
		// Process any remaining expected responses...
		//
		if (pRequest)
		{
			pRequest->ProcessRemainingResponses (ProcessRemainingResponse, nSystemicError,
												 (size_t)this);
		}

		// We are no longer waiting for a response
		pendingResponseClear ();

		if (eRepeatRequest == estiRR_NO || m_bCancelPending)
		{
			RequestRemove (unRequestId);
		}
		else if (pRequest)
		{
			pRequest->ExpectedResultsReset();
		}

		//
		// Cause the next request to send
		//
		PostEvent(std::bind (&CstiVPService::requestSendHandle, this));
	}

//STI_BAIL:

	if (pDocument)
	{
		// Free the document.
		ixmlDocument_free (pDocument);
		pDocument = nullptr;
	}

	m_bCancelPending = false;

	return (hResult);
} // end CstiVPService::HTTPResponseParse


stiHResult CstiVPService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError,
	size_t CallbackParam)
{
	return ((CstiVPService *)CallbackParam)->ProcessRemainingResponse(pRequest, nResponse,
																	  nSystemicError);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::HTTPError
//
// Description: Handles HTTP errors
//
// Abstract:
//	This function handles HTTP errors, retrying and changing servers as
//	necessary. The algorithm works as follows:
//
//	The current server is attempted again, up to unMAX_TRYS_ON_SERVER times,
//	with an increasing timeout on each try. After unMAX_TRYS_ON_SERVER tries,
//	the next two servers are attempted with a Zero timeout. If these servers
//	also fail, we will wait the max timeout, then try all servers again with
//	no timeout.
//
//	The three servers tried will be:
//	1. The Primary Core Server
//	2. The Secondary Core Server
//	3. The default (hard-coded) value for the Primary Server.
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiVPService::HTTPError (bool bSslError)
{
	stiLOG_ENTRY_NAME (CstiVPService::HTTPError);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::HTTPError (%s)- "
			"#### HTTP Error\n", pszService);
	); // stiSVC_DEBUG_TOOL

	if (m_bCancelPending)
	{
		RequestRemove (m_unResponsePending);

		m_bCancelPending = false;
	}

	RequestError ();

	if (bSslError)
	{
		SSLFailOver();
	}

//STI_BAIL:

	return (hResult);
} // end CstiVPService::HTTPError


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::MaxRetryTimeoutSet
//
// Description: Sets the maximum retry timeout.
//
// Abstract:
//
// Returns:
//
void CstiVPService::MaxRetryTimeoutSet (
	unsigned int unMaxRetryTimeOut)
{
	m_unMaxRetryTimeOut = unMaxRetryTimeOut;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::NextRequestGet
//
// Description: Returns the next request from the queue.
//
// Abstract:
//
// Returns:
//
CstiVPServiceRequest *CstiVPService::NextRequestGet()
{
	CstiVPServiceRequest *pRequest = nullptr;
	if (!m_Requests.empty())
	{
		pRequest =  m_Requests.front().m_pRequest;
	}

	return pRequest;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestError
//
// Description: Performs error handling
//
// Abstract:
//
// Returns:
//
stiHResult CstiVPService::RequestError ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t unTimeoutValue = 0;

	//
	// Note which request errored.
	//
	m_unErroredRequest = m_unResponsePending;

	// We are no longer waiting for a response
	pendingResponseClear ();

	m_unErrorCount++;
	m_unRetryCount++;

	hResult = StateEvent(eSE_HTTP_ERROR);
	stiTESTRESULT();

	CommunicationErrorMarkedRequestRemove();

	// Are we still in the initial retry period?
	if (m_unRetryCount <= m_unMaxTrysOnServer)
	{
		// Yes! Calculate a timeout
		unTimeoutValue = m_unRetryCount * m_unBaseRetryTimeOut;
	} // end if
	else
	{
		// No! Attempt to contact the next server in the list
		// and start increasing the amount of time between retry attempts
		//
		m_nCurrentPrimaryServer++;
//		if (m_nCurrentPrimaryServer >= m_nMaxServers)
//		{
//			m_nCurrentPrimaryServer = 0;
//		} // end if

		stiSVC_DEBUG_TOOL (this,
			stiTrace ("CstiVPService::HTTPError (%s) - "
				"#### Changing PRIMARY to server # %d\n",
				pszService, m_nCurrentPrimaryServer);
		); // stiSVC_DEBUG_TOOL

		// Notify Splunk.
		stiRemoteLogEventSend ("EventType=VPService Reason=PrimaryChanged ServiceID=%s ServerID=%d",
									 ServiceIDGet(), m_nCurrentPrimaryServer);

		
		uint32_t un32Power = (m_unRetryCount - m_unMaxTrysOnServer);

		if (un32Power > 15U)
		{
			un32Power = 15U;
		}

		uint32_t unFactor = m_unMaxTrysOnServer * m_unBaseRetryTimeOut;

		if (unFactor > (1U << 15U))
		{
			unFactor = (1U << 15U);
		}

		unTimeoutValue = (1U << un32Power) * unFactor;
	} // end else

	if (unTimeoutValue > m_unMaxRetryTimeOut)
	{
		unTimeoutValue = m_unMaxRetryTimeOut;
	}

	//
	// Give the retry timeout some variability in
	// case multiple videophones are also retrying
	//
	unTimeoutValue += ((unTimeoutValue / 2) - (rand () % (unTimeoutValue + 1)));

	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::HTTPError (%s)- "
			"Timeout = %d\n",
			pszService, unTimeoutValue);
	); // stiSVC_DEBUG_TOOL

	m_retryTimer.restart ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::StateGet
//
// Description: Returns the state of the service
//
// Abstract:
//
// Precondition:
//
// Returns:
//	CstiVPService::EState
//
CstiVPService::EState CstiVPService::StateGet ()
{
	stiLOG_ENTRY_NAME (CstiVPService::StateGet);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_eState;
} // end CstiVPService::StateGet


bool CstiVPService::isOfflineGet ()
{
	auto state = StateGet ();
	return !(state == eCONNECTED || state == eAUTHENTICATED);
}


stiHResult CstiVPService::StateSet(
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiVPService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_eState = eState;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestCancelHandle
//
// Description: Cancels the specified request
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiVPService::RequestCancelHandle (unsigned int unRequestId)
{
	stiLOG_ENTRY_NAME (CstiVPService::RequestCancelHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::RequestCancelHandle (%s)\n", pszService);
	); // stiSVC_DEBUG_TOOL

	//
	// If we are waiting for a response then we have to wait until we receive the response and
	// the request has been fully handled.
	//
	if (unRequestId == m_unResponsePending)
	{
		m_bCancelPending = true;
	}
	else
	{
		// Could we Remove this request?
		if (RequestRemove (unRequestId))
		{
			// Yes! We removed it. Cancel it in the HTTP client, as well.
		m_pHTTPTask->HTTPCancel (
				this,
				unRequestId);

			//
			// If we are cancelling the request that errored last
			// then reset the error information.
			//
			if (m_unErroredRequest == unRequestId)
			{
				m_unErroredRequest = 0;
				m_unErrorCount = 0;
				m_unRetryCount = 0;
			}
		} // end if
	}

//STI_BAIL:

	return (hResult);
} // end CstiVPService::RequestCancelHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestCancel
//
// Description: Cancels an unsent request
//
// Abstract:
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	None
//
stiHResult CstiVPService::RequestCancel (
	unsigned int unRequestId)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::RequestCancel);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	//
	// If we are waiting for a response then we have to wait until we receive the response and
	// the request has been fully handled.
	//
	if (unRequestId == m_unResponsePending)
	{
		m_bCancelPending = true;
	}
	else
	{
		// Try to remove the request. Can we remove it?
		if (!RequestRemove (unRequestId))
		{
			// No! It might still be in the message queue, so post a message
			// to do the remove.
			PostEvent (std::bind (&CstiVPService::RequestCancelHandle, this, unRequestId));
		} // end if
		else
		{
			RequestRemoved(unRequestId);

			// Yes! We removed it. Cancel it in the HTTP client, as well.
			m_pHTTPTask->HTTPCancel (
					this,
					unRequestId);

			//
			// If we are cancelling the request that errored last
			// then reset the error information.
			//
			if (m_unErroredRequest == unRequestId)
			{
				m_unErroredRequest = 0;
				m_unErrorCount = 0;
				m_unRetryCount = 0;
			}
		} // end else
	}

	return hResult;

} // end CstiVPService::RequestCancel

////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestCancelAsync
//
// Description: Cancels a pending request
//
// Abstract:
//	Posts an event to cancel a pending (unsent) request. If the request has
//	already been sent, it might not get cancelled.
//
// Returns:
//	None
//
void CstiVPService::RequestCancelAsync (
	unsigned int unRequestId)
{
	stiLOG_ENTRY_NAME (CstiVPService::RequestCancelAsync);
	PostEvent (std::bind (&CstiVPService::RequestCancelHandle, this, unRequestId));
} // end CstiVPService::RequestCancelAsync


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestGet
//
// Description: Gets the request in the queue based on request number
//
// Abstract:
//	This function will traverse the queue looking for
//	the request identified by the request number.  Once
//	found, the request in the queue is returned.  If not found the
//	value of NULL is returned.
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	A pointer to the request in the queue or NULL if not found
//
CstiVPServiceRequest *CstiVPService::RequestGet(
	unsigned int unRequestId,
	int *pnPriority)
{
	stiLOG_ENTRY_NAME (CstiVPService::RequestGet);

	CstiVPServiceRequest *pRequest = nullptr;
	std::list<CstiRequestQueueItem>::iterator i;

	for (i = m_Requests.begin (); i != m_Requests.end (); i++)
	{
		// Is there a valid request pointer in this slot?
		// Does this request number match the ID of this request?
		if (unRequestId == (*i).m_pRequest->requestIdGet ())
		{
			// Yes! Get the request
			pRequest = (*i).m_pRequest;
			
			if (pnPriority)
			{
				*pnPriority = (*i).m_nPriority;
			}
			
			break;
		} // end if
	} // end for

	return pRequest;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestRemove
//
// Description: Removes the requested request from the queue
//
// Abstract:
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	EstiBool - estiTRUE if request was removed, otherwise estiFALSE
//
EstiBool CstiVPService::RequestRemove (
	unsigned int unRequestId)
{
	stiLOG_ENTRY_NAME (CstiVPService::RequestRemove);

	std::list<CstiRequestQueueItem>::iterator i;
	bool bFound = false;

	for (i = m_Requests.begin (); i != m_Requests.end (); i++)
	{
		// Is there a valid request pointer in this slot?
		// Does this request number match the ID of this request?
		if (unRequestId == (*i).m_pRequest->requestIdGet ())
		{
			RequestRemoved ((*i).m_pRequest->requestIdGet ());

			delete (*i).m_pRequest;

			i = m_Requests.erase (i);

			bFound = true;
			break;
		} // end if
	} // end for

	stiSVC_DEBUG_TOOL (this,
		if (!bFound)
		{
			stiTrace ("CstiVPService::RequestRemove (%s) - "
				"Did not remove request %u from the array, since it wasn't found\n",
				pszService, unRequestId);
		} // end if
	); // stiSVC_DEBUG_TOOL

	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::RequestRemove (%s) - "
			"There are %u requests pending:\n",
			pszService, m_Requests.size ());
		for (i = m_Requests.begin (); i != m_Requests.end (); i++)
		{
			stiTrace ("  - Request %p (%u)\n",
				(*i).m_pRequest,
				(*i).m_pRequest->requestIdGet ());
		} // end for
	); // stiSVC_DEBUG_TOOL

	return (bFound ? estiTRUE : estiFALSE);
} // end CstiVPService::RequestRemove


void CstiVPService::RequestRemoved(
	unsigned int unRequestId)
{
}


stiHResult CstiVPService::RequestRemovedOnError(
	CstiVPServiceRequest *poRemovedRequest)
{
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::CommunicationErrorMarkedRequestRemove()
//
// Description: Removes Requests marked to be removed upon communication failure
//
// Abstract:
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	stiHResult - Success or failure result
//
stiHResult CstiVPService::CommunicationErrorMarkedRequestRemove ()
{
	stiLOG_ENTRY_NAME (CstiVPService::CommunicationErrorMarkedRequestRemove);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::list<CstiRequestQueueItem>::iterator i;

	for (i = m_Requests.begin (); i != m_Requests.end ();)
	{
		CstiVPServiceRequest *request = (*i).m_pRequest;

		// Yes! Check to see if we need to remove this request
		if (request->RemoveUponCommunicationError ())
		{
			if (request->retryOfflineGet ())
			{
				retryOffline (request, [this, request]() {
					RequestRemovedOnError (request);
					delete request;
				});
			}
			else
			{
				hResult = RequestRemovedOnError (request);
				delete request;
			}

			i = m_Requests.erase (i);

			stiTESTRESULT ();
		}
		else
		{
			i++;
		}
	} // end for

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestQueue
//
// Description: Adds a request to the queue.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiVPService::RequestQueue(
	CstiVPServiceRequest *poRequest,
	int nPriority)
{
	stiLOG_ENTRY_NAME (CstiVPService::RequestQueue);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::RequestQueue (%s) - "
			"There are %u requests pending\n"
			"Adding %p to the array\n",
			pszService,
			m_Requests.size (),
			poRequest);
	); // stiSVC_DEBUG_TOOL

	std::list<CstiRequestQueueItem>::reverse_iterator i;

	//
	// Start from the back of the queue and search
	// towards the front for a priority that is greater than
	// or equal to the priority for this request.  When found, insert
	// the request into the queue.
	//
	for (i = m_Requests.rbegin ();; i++)
	{
		if (i == m_Requests.rend ())
		{
			CstiRequestQueueItem Item{};

			Item.m_pRequest = poRequest;
			Item.m_nPriority = nPriority;

			m_Requests.push_front (Item);

			break;
		}
		else if ((*i).m_nPriority <= nPriority)
		{
			CstiRequestQueueItem Item{};

			Item.m_pRequest = poRequest;
			Item.m_nPriority = nPriority;

			m_Requests.insert (i.base (), Item);

			break;
		}
	}

	//
	// If the retry count is greater than zero then we must be in
	// a state of retrying.  When a request is queued short cut the
	// retry period by killing the timer and sending the "request send"
	// event.
	//
	if (m_unRetryCount > 0)
	{
		m_unRetryCount = 0;
		
		m_retryTimer.stop ();

		PostEvent (std::bind (&CstiVPService::requestSendHandle, this));
	}
	else
	{
		//
		// If the queue has only one element in it then we must have just inserted it
		// so send a message to get the request sent.
		//
		if (m_Requests.size () == 1)
		{
			PostEvent (std::bind (&CstiVPService::requestSendHandle, this));
		}
	}
	
	stiSVC_DEBUG_TOOL (this,
		stiTrace ("CstiVPService::RequestQueue (%s) - "
			"There are %u requests pending:\n",
			pszService, m_Requests.size ());
		std::list<CstiRequestQueueItem>::iterator j;
		for (j = m_Requests.begin (); j != m_Requests.end (); j++)
		{
			stiTrace ("  - Request %p (%u)\n",
				(*j).m_pRequest,
				(*j).m_pRequest->requestIdGet ());
		} // end for
	); // stiSVC_DEBUG_TOOL

//STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::stiHTTPResultCallback
//
// Description: Callback function for all HTTP results
//
// Abstract:
//	Handles the result of the HTTP post.
//
// Returns:
//	estiOK on success, estiERROR otherwise.
//
stiHResult CstiVPService::HTTPResultCallback (
	uint32_t un32RequestNum,
	EstiHTTPState eState,
	SHTTPResult *pstResult,
	void *pParam1)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto pThis = (CstiVPService *)pParam1;

	pThis->PostEvent ([pThis, un32RequestNum, eState, pstResult]() {
		pThis->HTTPResultCallbackHandle (un32RequestNum, eState, pstResult);
	});

	return hResult;
}


/*!\brief Handle the HTTP Result callback.
 *
 */
stiHResult CstiVPService::HTTPResultCallbackHandle (uint32_t un32RequestNum, EstiHTTPState eState, SHTTPResult* pstResult)
{
	stiLOG_ENTRY_NAME (CstiVPService::stiHTTPResultCallback);

	stiHResult hResult = stiRESULT_SUCCESS;
	bool bSslError = true;

	if (m_bShutdownInitiated)
	{
		//
		// If we have started the shutdown process then just throw away the state changes
		// until we reach the IDLE state.  When we see the IDLE state then signal all
		// waiters that there are no pending responses.
		//
		stiSVC_DEBUG_TOOL (this,
			stiTrace("Shut down initiated for %s.  Response state %d\n", pszService, eState);
		);

		if (eState == eHTTP_TERMINATED)
		{
			pendingResponseClear ();
		}
	}
	else
	{
		switch (eState)
		{
			case eHTTP_DNS_ERROR:
				bSslError = false;
				// Fall through
			case eHTTP_ABANDONED:
			case eHTTP_ERROR:
				// Should we just pass this error back?
				if (m_bPassError)
				{
					// Yes! Just pass it back and let HTTP report it.
					m_bPassError = estiFALSE;
					stiTHROW (stiRESULT_ERROR);
				} // end if
				else
				{
					// No! Send an error to the state machine
					HTTPError (bSslError);
				} // end else
				break;

			case eHTTP_COMPLETE:
			{
				// Did the HTTP operation return a success code?
				if (pstResult->unReturnCode >= 200 &&
					pstResult->unReturnCode < 300)
				{
					stiSVC_DEBUG_TOOL (this,
						stiTrace ("CstiVPService::stiHTTPResultCallback (%s) - Received:\n%s\n",
							pszService, pstResult->body.get ());
					);

					// Yes! Parse the response. Did it succeed?
					stiHResult hResult = HTTPResponseParse (
						pstResult->unRequestNum,
						pstResult->body.get ());

					if (stiIS_ERROR(hResult))
					{
						// No! Pass the error on to HTTP.
						m_bPassError = estiTRUE;
						stiTHROW (stiRESULT_ERROR);
					} // end if
				} // end if
				else
				{
					// No! Return an error condition. This will cause us
					// to receive the eHTTP_ERROR notification, and we will
					// log an error there if necessary.
					HTTPError (true);
				} // end else
				break;
			} // end case

			case eHTTP_TERMINATED:
			{
				if (m_unResponsePending == un32RequestNum)
				{
					pendingResponseClear ();

					//
					// Cause the next request to send
					//
					PostEvent(std::bind (&CstiVPService::requestSendHandle, this));
				}

				break;
			}

			// Nothing needs to be done in these cases.
			//case eHTTP_INVALID:
			case eHTTP_CANCELLED:
			case eHTTP_IDLE:
			case eHTTP_DNS_RESOLVING:
			case eHTTP_SENDING:
			case eHTTP_WAITING_4_RESP:
				break;

		} // end switch
	}

STI_BAIL:

	if (pstResult)
	{
		// Free the memory that was allocated for this result.
		delete pstResult;
		pstResult = nullptr;
	}
	
	return (hResult);
} // end HTTPResultCallback


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::RequestSend
//
// Description: Sends the oldest request
//
// Abstract:
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiVPService::requestSendHandle ()
{
	stiLOG_ENTRY_NAME (CstiVPService::requestSendHandle);

	stiASSERT (CurrentThreadIsMine ());

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiVPServiceRequest * poRequest = nullptr;
	EstiBool bUseSSL = estiFALSE;

	// Are we waiting for any responses or has a shutdown been initiated?
	if (!PauseGet () && m_unResponsePending == 0 && !m_bShutdownInitiated)
	{
		// Are there already pending requests?
		if (!m_Requests.empty())
		{
			// Yes! Get the oldest one.
			poRequest = m_Requests.front ().m_pRequest;

			//
			// Set the request id only if it has not already
			// been set.
			//
			if (poRequest->requestIdGet() == 0)
			{
				poRequest->requestIdSet(NextRequestIdGet());
			}

			// Yes! Add the request number and token
			poRequest->PrepareRequestForSubmission (
				m_clientToken,
				m_uniqueID);

			// Get the text of the request
			char *pszData = nullptr;
			poRequest->getXMLString (&pszData);

			stiSVC_DEBUG_TOOL (this,
				stiTrace ("CstiVPService::RequestSend (%s) - "
					"Sending request: %u\n%s\n", pszService, poRequest->requestIdGet (), pszData);
			); // stiSVC_DEBUG_TOOL

#define NUM_SERVERS 2
			char arrayRequestUrl[NUM_SERVERS][un8stiMAX_URL_LENGTH * 2];

			// Create the request to be sent.
			for (int i = 0; i < NUM_SERVERS; i++)
			{
				//
				// PING-PONG between primary and alternate server, if there was a failure.
				// This code switches the alternate server to be the primary
				//
				int nServer = i + m_nCurrentPrimaryServer;
				if (nServer >= NUM_SERVERS)
				{
					// We are in a failure scenario.  Only put URL into primary position,
					//  Don't put in alternate, just put in empty string.
					arrayRequestUrl[1][0] = 0;
				}
				// Create the URLs (most of the time we create both the primary and alternate)
				if (i == 0 || nServer == 1)
				{
					sprintf (
						arrayRequestUrl[i],
						"%s?u=%s&i=%u",
						m_ServiceContacts[(nServer % NUM_SERVERS)].c_str (), // modulus result either 0 or 1
						m_URLID.c_str (),
						stiOSTickGet ());
				}

				stiSVC_DEBUG_TOOL (this,
					stiTrace ("URL (%s): %s\n", pszService, arrayRequestUrl[i]);
				); // stiSVC_DEBUG_TOOL
			}

			switch (m_eSSLFlag)
			{
				case eSSL_OFF:

					bUseSSL = estiFALSE;

					break;

				case eSSL_ON_WITH_FAILOVER:
				case eSSL_ON_WITHOUT_FAILOVER:

					bUseSSL = poRequest->SSLEnabledGet ();

					break;
				default:
					stiSVC_DEBUG_TOOL (this,
						stiTrace ("CstiVPService::RequestSend (%s) enumeration value UNACCEPTABLE!",
							pszService);
					);
					break;
			}

			// Send the request.
			hResult = m_pHTTPTask->HTTPPost (
				this,
				poRequest->requestIdGet (),
				arrayRequestUrl[0],
				arrayRequestUrl[1],
				pszData,
				HTTPResultCallback,
				this,
				bUseSSL);
			stiTESTRESULT();

			// We are now waiting for a response
			m_unResponsePending = poRequest->requestIdGet ();

			hResult = StateEvent(eSE_REQUEST_SENT);
			stiTESTRESULT();

			// Free the text of the request
			poRequest->freeXMLString (pszData);
		} // end if
		else
		{
			stiSVC_DEBUG_TOOL (this,
				stiTrace ("CstiVPService::RequestSend (%s) - "
					"Nothing to send.\n", pszService);
			); // stiSVC_DEBUG_TOOL
		} // end else
	} // end if
	else
	{
		stiSVC_DEBUG_TOOL (this,
			stiTrace ("CstiVPService::RequestSend (%s) - "
				"Skipping send, response is pending or shutdown has been initiated.\n", pszService);
		); // stiSVC_DEBUG_TOOL
	} // end else

STI_BAIL:
	return (hResult);
} // end CstiVPService::RequestSend


void CstiVPService::pendingResponseClear ()
{
	{
		std::unique_lock<std::mutex> lock (m_pendingRequestMutex);
		m_unResponsePending = 0;
	}
	m_WaitForPendingCond.notify_one ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::ResponseParse
//
// Description: Parses the CoreResponse message.
//
// Abstract:
//
// Returns:
//	stiHResult - sucess or failure result
//
stiHResult CstiVPService::ResponseParse (
	IXML_Document *pDocument,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiRequestRepeat *peRepeatRequest,
	int *pnSystemicError)
{
	stiLOG_ENTRY_NAME (CstiVPService::ResponseParse);

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_NodeList *pNodeList = nullptr;
	IXML_Node *pTempNode = nullptr;
	IXML_NodeList *pChildNodes = nullptr;
	int x = 0;

	pNodeList = ixmlDocument_getElementsByTagName (
		pDocument, (DOMString)m_pszResponseType);
	stiTESTCOND(pNodeList, stiRESULT_ERROR);

	// While there are still nodes...
	while ((pTempNode = ixmlNodeList_item (pNodeList, x++)))
	{
		// Get the child nodes - are there any?
		pChildNodes = ixmlNode_getChildNodes (pTempNode);
		if (pChildNodes != nullptr)
		{
			// Yes!
			int y = 0;
			IXML_Node * pChildNode = nullptr;

			// Assume the database is available
			EstiBool bDatabaseAvailable = estiTRUE;
			EstiBool bLoggedOutNotified = estiFALSE;

			// Loop through the children, and process them.
			while ((pChildNode = ixmlNodeList_item (pChildNodes, y++)))
			{
				// Process this node
				hResult = ResponseNodeProcess (
					pChildNode,
					pRequest,
					unRequestId,
					nPriority,
					&bDatabaseAvailable,
					&bLoggedOutNotified,
					pnSystemicError,
					peRepeatRequest);
				stiTESTRESULT();

				//
				// If this request is not to be removed then break out
				// of the loops.  The whole request must be sent again.
				//
				if (*peRepeatRequest != estiRR_NO)
				{
					break;
				}
			} // end while

			// Free the child nodes
			ixmlNodeList_free (pChildNodes);
			pChildNodes = nullptr;

			//
			// If this request is not be removed then break out
			// of the loops.  The whole request must be sent again.
			//
			if (*peRepeatRequest != estiRR_NO)
			{
				break;
			}
		} // end if
	} // end while

STI_BAIL:

	if (pChildNodes)
	{
		// Free the child nodes
		ixmlNodeList_free (pChildNodes);
		pChildNodes = nullptr;
	}

	if (pNodeList)
	{
		// Free the node list
		ixmlNodeList_free (pNodeList);
		pNodeList = nullptr;
	}

	return (hResult);
} // end CstiVPService::ResponseParse


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::NextRequestIdGet
//
// Description:
//
// Abstract:
//
// Returns:
//
unsigned int CstiVPService::NextRequestIdGet ()
{
	stiLOG_ENTRY_NAME (CstiVPService::NextRequestIdGet);

	unsigned int unReturn;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	++m_unRequestId;

	if (m_unRequestId == 0)
	{
		m_unRequestId = 1;
	}

	unReturn = m_unRequestId;

	return (unReturn);
} // end CstiVPService::NextRequestIdGet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::clientTokenSet
//
// Description: Sets the StateNotifyService's local client token
//
// Abstract:  Called from CoreService after it authenticates
//
// Precondition:
//	This function assumes that all appropriate locking has been
//	taken care of by the calling function.
//
// Returns:
//	None
//
void CstiVPService::clientTokenSet (
	const std::string &clientToken)
{
	stiLOG_ENTRY_NAME (CstiVPService::clientTokenSet);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_clientToken = clientToken;

	stiSVC_DEBUG_TOOL (this,
		stiTrace (
			"CstiVPService::clientTokenSet (%s) - "
			"Client Token = %s\n", pszService, m_clientToken.c_str ());
	); // stiSVC_DEBUG_TOOL
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::clientTokenGet
//
// Description: Returns the client token.
//
// Abstract:
//
// Returns:
//	The client token.
//
std::string CstiVPService::clientTokenGet() const
{
	return m_clientToken;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::Restart
//
// Description: Restarts the service.
//
// Abstract:
//
// Returns:
//	Success or failure.
//
stiHResult CstiVPService::Restart()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Do we have a pending response?
	//
	if (m_unResponsePending != 0)
	{
		hResult = RequestCancel (m_unResponsePending);
		stiTESTRESULT ();

		pendingResponseClear ();
	}

	// Free any pending requests
	EmptyQueue();

	//
	// Cancel any active timers
	//
	m_retryTimer.stop ();

	//
	// Initialize our member variables.
	//
	m_bAuthenticated = estiFALSE;
	m_nCurrentPrimaryServer = 0;
	m_unErrorCount = 0;
	m_unRetryCount = 0;

	hResult = StateEvent (eSE_RESTART);
	stiTESTRESULT();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::SSLFlagSet
//
// Description: Sets the SSL flag.
//
// Abstract:
//
// Returns:
//	None.
//
void CstiVPService::SSLFlagSet (
	ESSLFlag eSSLFlag)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_eSSLFlag = eSSLFlag;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::SSLFlagGet
//
// Description: Returns the SSL flag.
//
// Abstract:
//
// Returns:
//	The SSL Flag
//
ESSLFlag CstiVPService::SSLFlagGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_eSSLFlag;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPService::SSLFailOver
//
// Description: If SSL fail over is allowed then sets the data member
// to turn off SSL.
//
// Abstract:
//
// Returns:
//	None.
//
void CstiVPService::SSLFailOver ()
{
	if (m_eSSLFlag == eSSL_ON_WITH_FAILOVER)
	{
		m_eSSLFlag = eSSL_OFF;

		SSLFailedOver();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; AttributeValueGet
//
// Description: Returns text value of specified sub item
//
// Abstract:
//
// Returns:
//	const char * - value of specified sub item, or NULL if not found
//
const char *AttributeValueGet (
	IXML_Node * pNode,
	const char * szAttributeName)
{
	const char * szValue = ixmlElement_getAttribute (
		(IXML_Element *)pNode, (DOMString)szAttributeName);

	return szValue;
} // end AttributeValueGet


////////////////////////////////////////////////////////////////////////////////
//; attributeStringGet
//
// Description: Returns text value of specified sub item
//
// Abstract:
//
// Returns:
//	std::string - value of specified sub item, or empty string if not found
//
std::string attributeStringGet (IXML_Node* node, const std::string& attributeName)
{
	auto value = AttributeValueGet (node, attributeName.c_str ());
	if (value != nullptr)
	{
		return value;
	}
	return std::string ();
}


////////////////////////////////////////////////////////////////////////////////
//; ElementValueGet
//
// Description: Returns text value of specified element
//
// Abstract:
//
// Returns:
//	const char * - value of specified sub item, or NULL if not found
//
const char *ElementValueGet (
	IXML_Node * pNode)
{
	const char * szValue = nullptr;

	IXML_Node * pChildVal = nullptr;

	// Can we get the text value from the first node?
	pChildVal = ixmlNode_getFirstChild (pNode);
	if (pChildVal != nullptr)
	{
		// Yes! Return the value
		szValue = ixmlNode_getNodeValue (pChildVal);
	} // end if

	return szValue;
} // end ElementValueGet

std::string elementStringValueGet (IXML_Node* node)
{
	auto value = ElementValueGet (node);
	if (value != nullptr)
	{
		return value;
	}
	return std::string ();
}


////////////////////////////////////////////////////////////////////////////////
//; SubElementValueGet
//
// Description: Returns text value of specified sub item
//
// Abstract:
//
// Returns:
//	const char * - value of specified sub item, or NULL if not found
//
const char *SubElementValueGet (
	IXML_Node * pNode,
	const char * pszItem)
{
	const char *pszValue = nullptr;
	IXML_Node * pChild = nullptr;

	pChild = SubElementGet (pNode, pszItem);

	if (pChild != nullptr)
	{
		pszValue = ElementValueGet (pChild);
	} // end if

	return pszValue;
} // end SubElementValueGet


////////////////////////////////////////////////////////////////////////////////
//; subElementStringGet
//
// Description: Returns text value of specified sub item
//
// Abstract:
//
// Returns:
//	std::string - value of specified sub item, or empty string if not found
//
std::string subElementStringGet (IXML_Node* node, const std::string& item)
{
	auto value = SubElementValueGet (node, item.c_str ());
	if (value != nullptr)
	{
		return value;
	}
	return std::string ();
}


////////////////////////////////////////////////////////////////////////////////
//; SubElementGet
//
// Description: Returns node pointer to specified sub element
//
// Abstract:
//
// Returns:
//	IXML_Node * - pointer to specified sub item, or NULL if not found
//
IXML_Node * SubElementGet (
	IXML_Node * pNode,
	const char * szItem)
{
	IXML_Node * pChild = nullptr;

	// Request a list of nodes with the indicated name
	IXML_NodeList * pNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pNode, (DOMString)szItem);

	// Did we get a list?
	if (pNodeList != nullptr)
	{
		// Yes! Can we get the first one?
		pChild = ixmlNodeList_item (pNodeList, 0);

		ixmlNodeList_free (pNodeList);
	} // end if

	return pChild;
} // end SubElementValueGet


////////////////////////////////////////////////////////////////////////////////
//; NextElementGet
//
// Description: Returns the next element
//
// Abstract:
//
// Returns:
//	IXML_Node * - pointer to the next item, or NULL if not found
//
IXML_Node * NextElementGet (
	IXML_Node * pNode)
{
	// Get the next node
	IXML_Node * pNextNode = ixmlNode_getNextSibling (pNode);

	return pNextNode;
} // end NextElementGet


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::ResultSuccess
//
// Description: Returns whether the result node indicates Success
//
// Abstract:
//	NOTE: If the Result tag does not exist in the specified node, success is
//	assumed.
//
// Returns:
//	EstiBool - estiTRUE if Result means success, otherwise estiFALSE
//
EstiBool ResultSuccess (
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (ResultSuccess);

	EstiBool bResult = estiTRUE;

	const char *pszValue = ixmlElement_getAttribute (
		(IXML_Element *)pNode, (DOMString)"Result");

	// Did we get a value?
	if (pszValue != nullptr)
	{
		// Yes! Was it Not OK?
		if (0 != strcmp (pszValue, VAL_OK))
		{
			// Yes! Return failure
			bResult = estiFALSE;
		} // end if
	} // end if
	else
	{
		// No! Was the node name "Error"
		if (0 == stiStrICmp (ixmlNode_getNodeName (pNode), "Error"))
		{
			// Yes! Return failure
			bResult = estiFALSE;
		} // end if
	} // end else

	return bResult;
} // end ResultSuccess


////////////////////////////////////////////////////////////////////////////////
//; NodeTypeDetermine
//
// Description: Translates between a node name and its enumeration
//
// Abstract:
//
// Returns:
//	CstiStateNotifyResponse::EResponse - the type of node, or eUNKNOWN
//
int CstiVPService::NodeTypeDetermine (
	const char *pszNodeName,
	const NodeTypeMapping *pNodeTypeMapping,
	int nNumNodeTypeMappings)
{
	stiLOG_ENTRY_NAME (NodeTypeDetermine);

	int nReturn = 0;

	for (int i = 0; i < nNumNodeTypeMappings; i++)
	{
		if (0 == strcmp (pszNodeName, pNodeTypeMapping[i].pszNodeName))
		{
			stiSVC_DEBUG_TOOL (this,
				stiTrace ("NodeTypeDetermine (%s) - Found:%s\n", pszService, pszNodeName);
			);

			nReturn = pNodeTypeMapping[i].nNodeType;

			break;
		} // end if
	}

	stiSVC_DEBUG_TOOL (this,
		if (nReturn == 0)
		{
			stiTrace ("NodeTypeDetermine (%s) - Found: UNKNOWN MODE!\n", pszService);
		}
	);

	return (nReturn);
} // end NodeTypeDetermine


void CstiVPService::ServiceContactsSet (
	const std::string *serviceContacts)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!serviceContacts[0].empty () && serviceContacts[0] != m_ServiceContacts[0])
	{
		m_ServiceContacts[0] = serviceContacts[0];
	}

	if (!serviceContacts[1].empty ())
	{
		m_ServiceContacts[1] = serviceContacts[1];
	}
}

stiHResult CstiVPService::Callback (
	int32_t message,
	CstiVPServiceResponse *response)
{
	stiHResult hResult{ stiRESULT_SUCCESS };
	if (m_pfnObjectCallback)
	{
		hResult = m_pfnObjectCallback (message, (size_t)response,
			m_ObjectCallbackParam, m_CallbackFromId);
	}
	return hResult;
}

#ifdef stiDEBUGGING_TOOLS
////////////////////////////////////////////////////////////////////////////////
//; NodePrint
//
// Description: Prints the indicated node to the trace
//
// Abstract:
//
// Returns:
//	None
//
void NodePrint (
	IXML_Node *pNode)
{
	stiLOG_ENTRY_NAME (NodePrint);

	static char szIndent[1024] = "";
	static int nDepth = 0;

	// Is our depth zero?
	if (nDepth == 0)
	{
		// Yes! Print a separator
		stiTrace ("\n-----------------------------------------------------\n");
	} // end if

	nDepth++;

	IXML_NodeList* pChildNodes;

	// Print the basic info
	stiTrace (
		"%sName: %s, Value: %s\n",
		szIndent,
		ixmlNode_getNodeName (pNode),
		ixmlNode_getNodeValue (pNode));

	// Does the node have attributes?
	if (ixmlNode_hasAttributes (pNode))
	{
		IXML_NamedNodeMap* pAttr;
		unsigned long ulNum;
		unsigned long x;

		// Yes! Get them
		pAttr = ixmlNode_getAttributes (pNode);
		ulNum = ixmlNamedNodeMap_getLength (pAttr);

		// Loop through the attributes
		for (x = 0; x < ulNum; x++)
		{
			IXML_Node * pAttrNode = ixmlNamedNodeMap_item (pAttr, x);

			// Print the attribute
			stiTrace (
				"%sAttribute %d - Name: %s, Value: %s\n",
				szIndent,
				x + 1,
				ixmlNode_getNodeName (pAttrNode),
				ixmlNode_getNodeValue (pAttrNode));
		} /* end for */

		// Free the attributes
		ixmlNamedNodeMap_free (pAttr);
	} // end if

	// Does the node have children?
	pChildNodes = ixmlNode_getChildNodes (pNode);
	if (pChildNodes)
	{
		// Yes!
		int x = 0;
		IXML_Node * pTempNode = nullptr;

		// Add to the indent
		strcat (szIndent, "\t");

		// Loop through children
		pTempNode = ixmlNodeList_item (pChildNodes, x++);
		while (pTempNode != nullptr)
		{
			// Print this child
			NodePrint (pTempNode);

			// Get the next child
			pTempNode = ixmlNodeList_item (pChildNodes, x++);
		} // end while

		// Remove the last indent
		szIndent[strlen(szIndent) - 1] = 0;

		// Free the children
		ixmlNodeList_free (pChildNodes);
	} /* end if */
	else
	{
		stiTrace ("%sNo Children.\n", szIndent);
	} /* end else */

	nDepth--;

	// Is our depth zero?
	if (nDepth == 0)
	{
		// Yes! Print a separator
		stiTrace ("-----------------------------------------------------\n\n");
	} // end if

} // end NodePrint
#endif // ifdef stiDEBUGGING_TOOLS
