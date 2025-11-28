/*!
 * \file CstiRemoteLoggerService.cpp
 * \brief Implementation of the CstiRemoteLoggerService class.
 *
 * Processes RemoteLoggerService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2013 – 2013 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//
#ifdef stiLINUX
#include <ctime>
#endif

#include <cctype>	// standard type definitions.
#include <cstring>
#include "ixml.h"
#include "stiOS.h"			// OS Abstraction
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiHTTPTask.h"
#include "stiCoreRequestDefines.h"
#include "CstiRemoteLoggerService.h"
#include <sstream>
#include <iomanip>

#include "stiTaskInfo.h"

//
// Constants
//
static const unsigned int unMAX_TRYS_ON_SERVER = 3;		// Number of times to try the first server
static const unsigned int unBASE_RETRY_TIMEOUT = 10;	// Number of seconds for the first retry attempt
static const unsigned int unMAX_RETRY_TIMEOUT =  7200;	// Max retry timeout is two hours
static const int nMAX_SERVERS = 2;						// The number of servers to try connecting to.
static const int DUPLICATE_ERROR_LOG_TIMEOUT = 5000; // 5 second timer to log duplicate errors.

//
// Constant strings representing the Request tag names
//
const std::string RLRQ_RemoteLogAssert = "RemoteLogAssert";
const std::string RLRQ_RemoteLogTrace = "RemoteLogTrace";
const std::string RLRQ_RemoteLogCallStats = "RemoteLogCallStats";
const std::string RLRQ_RemoteLogEventSend = "RemoteLogEvent";
const std::string RLRQ_SipStackCallStats = "SipStackStats";
const std::string RLRQ_EndPointDiagnostics = "EndPointDiagnostics";


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
// Locals
//

//
// Function Declarations
//


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::CstiRemoteLoggerService
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiRemoteLoggerService::CstiRemoteLoggerService (
	CstiHTTPTask *pHTTPTask,
	PstiObjectCallback pAppCallback,
	size_t CallbackParam)
:
	CstiVPService (
		pHTTPTask,
		pAppCallback,
		CallbackParam,
		(size_t)this,
		stiRLS_MAX_MSG_SIZE,
		RLRQ_RemoteLoggerResponse,
		stiRLS_TASK_NAME,
		unMAX_TRYS_ON_SERVER,
		unBASE_RETRY_TIMEOUT,
		unMAX_RETRY_TIMEOUT,
		nMAX_SERVERS),
	m_wdRemoteLogSend(nullptr),
	m_wdRemoteLogging(nullptr),
	m_bRemoteLogSendTimer(estiFALSE),
	m_bRemoteLogTimer(estiFALSE),
	m_bRemoteTraceLogging(estiFALSE),
	m_nRemoteLoggingTraceValue(0),
	m_bRemoteAssertLogging(estiFALSE),
	m_nRemoteLoggingAssertValue(0),
	m_bRemoteEventLogging(estiFALSE),
	m_nRemoteLoggingEventValue(0),
	m_unRequestId(0),
	m_logDuplicateErrorsTimer (DUPLICATE_ERROR_LOG_TIMEOUT, this)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::CstiRemoteLoggerService);
	m_signalConnections.push_back( m_logDuplicateErrorsTimer.timeoutSignal.Connect ( [this] {logDuplicateErrors ();}));
} // end CstiRemoteLoggerService::CstiRemoteLoggerService


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::~CstiRemoteLoggerService
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	none
//
CstiRemoteLoggerService::~CstiRemoteLoggerService ()
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::~CstiRemoteLoggerService);

	// remove the watchdog timers
	if (nullptr != m_wdRemoteLogSend)
	{
		stiOSWdDelete (m_wdRemoteLogSend);
		m_wdRemoteLogSend = nullptr;
	}
	// remove the watchdog timers
	if (nullptr !=m_wdRemoteLogging)
	{
		stiOSWdDelete (m_wdRemoteLogging);
		m_wdRemoteLogging = nullptr;
	}

	Cleanup ();
	
} // end CstiRemoteLoggerService::~CstiRemoteLoggerService

//:-----------------------------------------------------------------------------
//:	Task functions
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//	This method will initializes the RemoteLogger Service task members
//	and calls RemoteLoggerChanged to check for the start of logging.
//
// Returns:
//
stiHResult CstiRemoteLoggerService::Initialize (
	const std::string *pServiceContacts,
	const std::string &uniqueID,
	const std::string &version,
	ProductType productType,
	int nRemoteTraceLogging,
	int nRemoteAssertLogging,
	int nRemoteEventLogging)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::stringstream strUniqueID;
	for (auto letter : uniqueID)
	{
		if (letter != ':')
		{
			strUniqueID << letter;
		}
	}

	CstiRemoteLoggerRequest::macAddressSet (strUniqueID.str ());
	CstiRemoteLoggerRequest::productNameSet (productNameConvert (productType));
	CstiRemoteLoggerRequest::versionSet (version);

	m_nRemoteLoggingTraceValue = nRemoteTraceLogging;
	m_nRemoteLoggingAssertValue = nRemoteAssertLogging;
	m_nRemoteLoggingEventValue = nRemoteEventLogging;

	// NOTE: we don't need to call Cleanup to free up the allocated memory 
	// in the Initialize function

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

	CstiVPService::Initialize (pServiceContacts, uniqueID);

	if (nullptr == (m_wdRemoteLogSend = stiOSWdCreate ()))
	{
		stiDEBUG_TOOL (g_stiRemoteLoggerServiceDebug,
					stiTrace ("<CstiRemoteLoggerService::Initialize>: failure creating LogSend timer\n");
		); // end stiDEBUG_TOOL
	} // end if

	if (nullptr == (m_wdRemoteLogging = stiOSWdCreate ()))
	{
		stiDEBUG_TOOL (g_stiRemoteLoggerServiceDebug,
					stiTrace ("<CstiRemoteLoggerService::Initialize>: failure creating Logging timer\n");
		); // end stiDEBUG_TOOL
	} // end if

	//clear the buffers
	Lock ();
	memset(m_szTraceBuf, '\0', RLS_MAX_BUF_LEN);
	CstiRemoteLoggerRequest::loggedInPhoneNumberSet ("Unknown");
	Unlock();

	//NOTE: what should be done if not set to 0 or -1?
	if (m_nRemoteLoggingTraceValue == -1)
	{
		RemoteLoggerTraceEnable(m_nRemoteLoggingTraceValue);
	}

	//NOTE: if non 0 turn it on
	if (m_nRemoteLoggingAssertValue != 0)
	{
		RemoteLoggerAssertEnable(m_nRemoteLoggingAssertValue);
	}

	//NOTE: if non 0 turn it on
	if (m_nRemoteLoggingEventValue != 0)
	{
		RemoteLoggerEventEnable(m_nRemoteLoggingEventValue);
	}

	return (hResult);

} // end CstiRemoteLoggerService::RemoteLoggerPhoneNumberSet

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::LoggedInPhoneNumberSet
//
// Description: Phone number set method for the class
//
// Abstract:
//
//
// Returns:
//
void CstiRemoteLoggerService::LoggedInPhoneNumberSet(
	 const char *pszLoggedInPhoneNumber)
{
	stiLOG_ENTRY_NAME(CstiRemoteLoggerService::LoggedInPhoneNumberSet);

	CstiRemoteLoggerRequest::loggedInPhoneNumberSet (pszLoggedInPhoneNumber);

} // end ::LoggedInPhoneNumberSet

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::MacAddressSet
//
// Description: Mac address set method for the class
//
// Abstract:
//
//
// Returns:
//
void CstiRemoteLoggerService::MacAddressSet(
		const char *pszMacAddressSet)
{
	stiLOG_ENTRY_NAME(CstiRemoteLoggerService::MacAddressSet);

	CstiRemoteLoggerRequest::macAddressSet (pszMacAddressSet);

} // end ::MacAddressSet

//; CstiRemoteLoggerService::NetworkTypeSet
//
// Description: Network type set method for the class
//
// Abstract:
//
//
// Returns:
//
void CstiRemoteLoggerService::NetworkTypeSet(const std::string &networkType)
{
	stiLOG_ENTRY_NAME(CstiRemoteLoggerService::NetworkTypeSet);

	CstiRemoteLoggerRequest::networkTypeSet(networkType);
} // end ::NetworkTypeSet
////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::DeviceTokenSet
//
// Description: Device token set method for the class
//
// Abstract:
//
//
// Returns:
//
////////////////////////////////////////////////////////////////////////////////
void CstiRemoteLoggerService::DeviceTokenSet(const std::string &deviceToken)
{
	CstiRemoteLoggerRequest::deviceTokenSet (deviceToken);

} // end ::DeviceTokenSet

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::SessionIdSet
//
// Description: Session ID set method for the class
//
// Abstract:
//
//
// Returns:
//
void CstiRemoteLoggerService::SessionIdSet(const std::string &sessionId)
{
	CstiRemoteLoggerRequest::sessionIdSet (sessionId);

} // end ::SessionIdSet

//:-----------------------------------------------------------------------------
//:	Private methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::StateSet
//
// Description: Shell for set current state
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS
//
stiHResult CstiRemoteLoggerService::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = CstiVPService::StateSet(eState);

	stiTESTRESULT();

STI_BAIL:

	return hResult;
	
} // end CstiRemoteLoggerService::StateSet


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RequestRemovedOnError
//
// Description: Notifies the application layer that a request was removed.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiRemoteLoggerService::RequestRemovedOnError(
	CstiVPServiceRequest *request)
{
	stiUNUSED_ARG(request);
	stiHResult hResult = stiRESULT_SUCCESS;

	//Increment Failed attempts error counter
	CstiRemoteLoggerRequest::errorCountIncrease ();

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiRemoteLoggerService::RequestRemovedOnError - "
			"Removing request id: %d\n",
			request->requestIdGet());
	); // stiDEBUG_TOOL

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::ProcessRemainingResponse
//
// Description: Shell method as this is a virtual
//
// Abstract:
//
// Returns:
//	stiHResult - success
//
stiHResult CstiRemoteLoggerService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::ResponseNodeProcess
//
// Description: Parses the indicated node
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiRemoteLoggerService::ResponseNodeProcess (
	IXML_Node * pNode,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiBool * pbDatabaseAvailable,
	EstiBool * pbLoggedOutNotified,
	int *pnSystemicError,
	EstiRequestRepeat *peRepeatRequest)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::ResponseNodeProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Remove this request, unless otherwise instructed.
	*peRepeatRequest = estiRR_NO;

	stiDEBUG_TOOL (g_stiRemoteLoggerServiceDebug,
		stiTrace ("CstiRemoteLoggerService::ResponseNodeProcess - Node Name: %s\n",
			ixmlNode_getNodeName (pNode));
	);

	// Did we get an OK result?
	if (ResultSuccess (pNode))
	{
		//Note: Failed attempts clear error counter
		CstiRemoteLoggerRequest::errorCountSet (0);
	} // end if
	else
	{
		//Increment Failed attempts error counter
		CstiRemoteLoggerRequest::errorCountIncrease ();
	} // end else

//STI_BAIL:

	return (hResult);
} // end CstiRemoteLoggerService::ResponseNodeProcess


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::Restart
//
// Description:
//	Restart the RemoteLogger Services by initializing the member variables
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - if there is no error. Otherwise, some errors occur.
//
stiHResult CstiRemoteLoggerService::Restart ()
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::Restart);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiVPService::Restart();

	stiTESTRESULT();

STI_BAIL:

	return (hResult);

}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::VRSCallIdSet
//
// Description: Sets the VRs Call ID the Remote Logging Service
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS
//
stiHResult CstiRemoteLoggerService::VRSCallIdSet(const char *pszVRSCallId)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::VRSCallIdSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	CstiRemoteLoggerRequest::vrsCallIdSet (pszVRSCallId);

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RequestSend
//
// Description: Sends a request to the Remote Logging Service
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued.
//
stiHResult CstiRemoteLoggerService::RequestSend (
	CstiRemoteLoggerRequest *poRemoteLoggerRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RequestSend);

	unsigned int unRequestId;
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Were we given a valid request?
	stiTESTCOND (poRemoteLoggerRequest != nullptr, stiRESULT_ERROR);

	{
		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poRemoteLoggerRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		hResult = RequestQueue (poRemoteLoggerRequest, 500);
		stiTESTRESULT ();

		poRemoteLoggerRequest = nullptr;
	}

STI_BAIL:

	if (poRemoteLoggerRequest)
	{
		delete poRemoteLoggerRequest;
		poRemoteLoggerRequest = nullptr;
	}

	return (hResult);
} // end CstiRemoteLoggerService::RequestSend

stiHResult CstiRemoteLoggerService::RemoteDebugLogSend (
                                                        const std::string &message) {
    return RemoteLogSend(message, "DEBUG");
}

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RequestSendEx
//
// Description: Sends a request to the Remote Logging Service
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued.
//
stiHResult CstiRemoteLoggerService::RequestSendEx (
	CstiRemoteLoggerRequest *poRemoteLoggerRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RequestSendEx);

	unsigned int unRequestId;
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Were we given a valid request?
	stiTESTCOND (poRemoteLoggerRequest != nullptr, stiRESULT_ERROR);

	{
		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poRemoteLoggerRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		hResult = RequestQueue (poRemoteLoggerRequest, 500);
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiRemoteLoggerService::RequestSendEx


/*! \brief Sends message with additional logging headers
 *
 *  \param message Message to be sent.
 *  \param type Type of message being sent.
 */
stiHResult CstiRemoteLoggerService::RemoteLogSend (const std::string &message, const std::string &type)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!message.empty ())
	{
		char timeBuffer[64];
		auto now = time(nullptr);
		struct tm utcTime{};
		gmtime_r(&now, &utcTime);
		std::strftime(timeBuffer, sizeof (timeBuffer), "%F %T", &utcTime);

		auto *pRemoteLoggerRequest = new CstiRemoteLoggerRequest();
		
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		pRemoteLoggerRequest->RemoveUponCommunicationError() = estiTRUE;
		
		pRemoteLoggerRequest->RemoteLog (type,
										 timeBuffer,
										 message);
		
		hResult = RequestSend(pRemoteLoggerRequest, &m_unRequestId);
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteAssertLogger
//
// Description: Called by stiAssert with debug trace information.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::RemoteAssertSend (size_t *pParam, const std::string &error, const std::string &fileAndLine)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteAssertSend);

	auto pThis = (CstiRemoteLoggerService*)pParam;
	
	if (pThis->m_bRemoteAssertLogging == estiTRUE && !error.empty ())//Are we logging
	{
		// Save the error and check for logging before calling RemoteLogSend this prevents a deadlock with the event queues mutex.
		if (pThis->saveErrorAndSendLogNow (fileAndLine, error))
		{
			pThis->RemoteLogSend (error, RLRQ_RemoteLogAssert);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggerAssertEnable
//
// Description: Called by the CCI when RemoteLogging property assert value has changed
// if (RemoteLoggerAssertValue = 0) send event to stop assert logging
// if (RemoteLoggerAssertValue = 1) send event to start assert logging
//
// Abstract:
//
// Returns:
//	estiOK if success, or some error
//
stiHResult CstiRemoteLoggerService::RemoteLoggerAssertEnable(int nRemoteLoggerAssertValue)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteLoggerAssertEnable);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (nRemoteLoggerAssertValue == 0)
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		m_bRemoteAssertLogging = estiFALSE;
	}
	else
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		m_bRemoteAssertLogging = estiTRUE;
	}
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteCallStatsLoggingEnable
//
// Description: Called by the CCI when RemoteLogginCallStats property assert value has changed
// if (RemoteLoggerAssertValue = 0) send event to stop assert logging
// if (RemoteLoggerAssertValue = 1) send event to start assert logging
//
// Abstract:
//
// Returns:
//    estiOK if success, or some error
//
stiHResult CstiRemoteLoggerService::RemoteCallStatsLoggingEnable(int nRemoteCallStatsLoggingValue)
{
    stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteCallStatsLoggingEnable);

    stiHResult hResult = stiRESULT_SUCCESS;
    

    if (nRemoteCallStatsLoggingValue == 0)
    {
        std::lock_guard<std::recursive_mutex> lock (m_execMutex);
        m_bRemoteCallStatsLogging = estiFALSE;
    }
    else
    {
        std::lock_guard<std::recursive_mutex> lock (m_execMutex);
        m_bRemoteCallStatsLogging = estiTRUE;
    }
    return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteCallStatsSend
//
// Description: Called from CstiCCI to send the Call Stats to the logging server.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::RemoteCallStatsSend (const char *pszBuff)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteCallStatsSend);

	if (pszBuff && pszBuff[0])
	{
		RemoteLogSend (pszBuff, RLRQ_RemoteLogCallStats);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::SipStackStatsSend
//
// Description: Called from CstiCCI to send the Sip Stack Stats to the logging server.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::SipStackStatsSend (const char *pszBuff)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::SipStackStatsSend);

	if (pszBuff && pszBuff[0])
	{
		RemoteLogSend (pszBuff, RLRQ_SipStackCallStats);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::EndPointDiagnosticsSend
//
// Description: Used to send the End Point Diagnostics to the remote logging server.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::EndPointDiagnosticsSend (const char *pszBuff)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::EndPointDiagnosticsSend);

	if (pszBuff && pszBuff[0])
	{
		RemoteLogSend (pszBuff, RLRQ_EndPointDiagnostics);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLogEventSend
//
// Description: Called from CstiCCI to send the event data to the logging server.
//
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued or the buffer is empty.
//
stiHResult CstiRemoteLoggerService::RemoteLogEventSend (size_t *pParam, const char *pszBuff)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteLogEventSend);

	auto pThis = (CstiRemoteLoggerService*)pParam;

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiRemoteLoggerRequest *pRemoteLoggerRequest = nullptr;

	//Are we logging
	if (pThis->m_bRemoteEventLogging)
	{
		hResult = pThis->RemoteLogSend (pszBuff, RLRQ_RemoteLogEventSend);
		stiTESTRESULT_FORCELOG ();
	}

STI_BAIL:

	if (pRemoteLoggerRequest)
	{
		delete pRemoteLoggerRequest;
		pRemoteLoggerRequest = nullptr;
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggerEventEnable
//
// Description: Called by the CCI when RemoteLogging property LogEventSend value has changed
// if (RemoteLoggerEventValue = 0) send event to stop Event logging
// if (RemoteLoggerEventValue = 1) send event to start Event logging
//
// Abstract:
//
// Returns:
//	estiOK if success, or some error
//
stiHResult CstiRemoteLoggerService::RemoteLoggerEventEnable(int nRemoteLoggingEventValue)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteLoggingEventEnable);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (nRemoteLoggingEventValue == 0)
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		m_bRemoteEventLogging = estiFALSE;
	}
	else
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		m_bRemoteEventLogging = estiTRUE;
	}
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteTraceSend
//
// Description: Called by stiTrace with debug trace information.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::RemoteTraceSend (size_t *pParam, const char *pszBuff)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteTraceSend);

	auto pThis = (CstiRemoteLoggerService*)pParam;

	if (pThis->m_bRemoteTraceLogging == estiTRUE)//Are we logging
	{
		pThis->Lock();
		if (nullptr != pThis->m_wdRemoteLogSend &&  pThis->m_bRemoteLogSendTimer == estiFALSE)
		{
			stiHResult hResult = stiOSWdStart ( pThis->m_wdRemoteLogSend,
							RLS_BUFFER_SEND_INTERVAL,
							(stiFUNC_PTR) &(CstiRemoteLoggerService::RemoteLoggingSendTimerCallback), (size_t)pThis);

			if (stiIS_ERROR (hResult))
			{
				stiDEBUG_TOOL (g_stiRemoteLoggerServiceDebug,
					stiTrace ("<CstiRemoteLoggerService::RemoteLogger> WatchDog Remote Logging Send Timer _NOT_ started: %d\n", estiERROR);
				);
			}
			else
			{
				pThis->m_bRemoteLogSendTimer = estiTRUE;//Will be cleared in timer callback

			}
		}

		if (pszBuff && pszBuff[0])
		{	//Make sure that we have enough buffer space
			if ((RLS_MAX_BUF_LEN - strlen(pThis->m_szTraceBuf)) > (strlen(pszBuff) + 1))
			{
				strcat(pThis->m_szTraceBuf, pszBuff);
			}
			else
			{   //The global trace buffer is full send it chunks until there is room.
				for (int i = 0; i < (int)(strlen(pszBuff) + 1); i += (RLS_MAX_BUF_LEN - 1))
				{
					pThis->RemoteLoggingTraceSend ();
					strncat(pThis->m_szTraceBuf, pszBuff+i, (RLS_MAX_BUF_LEN - 1));

				}
			}
		}
		pThis->Unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggingTraceSend
//
// Description: Called from RemoteLoggingSendTimerCallback/RemoteLogger to send to logging server.
//
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::RemoteLoggingTraceSend ()
{
	if (m_szTraceBuf[0])
	{
		RemoteLogSend (m_szTraceBuf, RLRQ_RemoteLogTrace);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggingSendTimerCallback
//
// Description: Called from WD timer when its time to send to logging server.
//
//
// Abstract:
//
// Returns:
// 0
//
int CstiRemoteLoggerService::RemoteLoggingSendTimerCallback (
	size_t param)
{	
	auto pThis = (CstiRemoteLoggerService*)param;

	pThis->RemoteLoggingTraceSend ();

	return (0);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggingTimerCallback
//
// Description: Called from WD timer when its time to stop logging to the remote server.
//
//
// Abstract:
//
// Returns:
//	None
//
int CstiRemoteLoggerService::RemoteLoggingTimerCallback (
	size_t param)
{
	auto pThis = (CstiRemoteLoggerService*)param;
	pThis->Lock();
	pThis->m_bRemoteTraceLogging = estiFALSE;//Turn Remote Logging off
	pThis->m_bRemoteLogTimer = estiFALSE;
	pThis->Unlock();
	
	return (0);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::RemoteLoggerTraceEnable
//
// Description: Called by the CCI when RemoteLogging property value has changed
// if (RemoteLoggerValue = 0) send event to stop logging
// if (RemoteLoggerValue = -1 or ) send event to start logging
// if (RemoteLoggerValue = 1 to x min) send event to start logging and start event timer (
//
// Abstract:
//
// Returns:
//	estiOK if success, or some error
//
stiHResult CstiRemoteLoggerService::RemoteLoggerTraceEnable(int nRemoteLoggerTraceValue)
{
	stiLOG_ENTRY_NAME (CstiRemoteLoggerService::RemoteLoggerEnable);

	stiHResult hResult = stiRESULT_SUCCESS;
	m_nRemoteLoggingTraceValue = nRemoteLoggerTraceValue;

	//Cancel any timed logging
	if (m_bRemoteLogTimer != estiFALSE)
	{
		if (nullptr != m_wdRemoteLogging)
		{
			stiOSWdCancel (m_wdRemoteLogging);
		}
		m_bRemoteLogTimer = estiFALSE;
	}

	if (m_nRemoteLoggingTraceValue == 0)
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		m_bRemoteTraceLogging = estiFALSE;
	}
	else
	{
		if (nullptr != m_wdRemoteLogging && m_nRemoteLoggingTraceValue > 0)
		{
			//Start the remote logging timer
			stiHResult hResult = stiOSWdStart (m_wdRemoteLogging,
								m_nRemoteLoggingTraceValue * RLS_REMOTE_LOGGING_TIME_INTERVAL,
							(stiFUNC_PTR) &RemoteLoggingTimerCallback, (size_t)this);

			if (stiIS_ERROR (hResult))
			{
				stiDEBUG_TOOL (g_stiRemoteLoggerServiceDebug,
					stiTrace ("<CstiRemoteLoggerService::RemoteLoggerTraceChanged> WatchDog Remote Logging Timer _NOT_ started: %d\n", estiERROR);
				);
			}
			else
			{
				m_nRemoteLoggingTraceValue = 0;
				m_bRemoteLogTimer = estiTRUE;
			}
		}
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);
		memset(m_szTraceBuf, '\0', RLS_MAX_BUF_LEN);//Clear the Trace buffer
		m_bRemoteTraceLogging = estiTRUE;
	}
	return hResult;
}


void CstiRemoteLoggerService::InterfaceModeSet (EstiInterfaceMode mode)
{
	CstiRemoteLoggerRequest::interfaceModeSet (InterfaceModeToString (mode));
}


void CstiRemoteLoggerService::NetworkTypeAndDataSet (
	NetworkType type,
	const std::string& data)
{
	CstiRemoteLoggerRequest::networkTypeSet (NetworkTypeToString (type));
	CstiRemoteLoggerRequest::networkDataSet (data);
}


void CstiRemoteLoggerService::CoreUserIdsSet (
	const std::string& userId,
	const std::string& groupId)
{
	CstiRemoteLoggerRequest::coreUserIdSet (userId);
	CstiRemoteLoggerRequest::coreGroupIdSet (groupId);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::SSLFailedOver
//
// Description: Called by the base class to inform the derived class
// that SSL has failed over.
//
// Abstract:
//
// Returns:
//	None
//
void CstiRemoteLoggerService::SSLFailedOver()
{
	Callback (estiMSG_REMOTELOGGER_SERVICE_SSL_FAILOVER, nullptr);//Updates property
}


#ifdef stiDEBUGGING_TOOLS
////////////////////////////////////////////////////////////////////////////////
//; CstiRemoteLoggerService::IsDebugTraceSet
//
// Description: Called by the base class to determine the derived type and to
// know if the debug tool for the derived class is enabled.
//
// Abstract:
//
// Returns:
//	estiTRUE if the debug tool for the derived class is enabled
//	estiFALSE otherwise.
//
EstiBool CstiRemoteLoggerService::IsDebugTraceSet (const char **ppszService) const
{
	EstiBool bRet = estiFALSE;
	
	if (g_stiRemoteLoggerServiceDebug)
	{
		bRet = estiTRUE;
		*ppszService = ServiceIDGet();
	}
	
	return bRet;
}
#endif

/**
 * \brief Determine the ID of the derived type
 *
 * Called by the base class to determine the derived type.
 *
 * \param ppszService Returns the service type string ("RLS" in this case).
 */
const char * CstiRemoteLoggerService::ServiceIDGet () const
{
	return "RLS";
}


/**
* \brief Saves error and starts duplicate error log timer.
*
* Called by the base class to determine the derived type.
*
* \param fileAndLine File and Line of error being logged.
 *\param error Error being logged.
 *
 *\Return true if we need to send the log now. False if this is a duplicate error to be logged with other duplicates later.
*/
bool CstiRemoteLoggerService::saveErrorAndSendLogNow (const std::string &fileAndLine, const std::string &error)
{
	bool sendLogNow = false;
	std::lock_guard<std::mutex> lock (m_mapMutex);
	
	if (m_errorMap.find (fileAndLine) == m_errorMap.end ())
	{
		sendLogNow = true;
		m_errorMap[fileAndLine].second = 0;
	}
	else
	{
		m_errorMap[fileAndLine].second++;
	}
	
	m_errorMap[fileAndLine].first = error;
	m_logDuplicateErrorsTimer.start ();
	
	return sendLogNow;
}


void CstiRemoteLoggerService::logDuplicateErrors ()
{
	std::unique_lock<std::mutex> lock (m_mapMutex);
	auto errorMap = m_errorMap;
	m_errorMap.clear ();
	lock.unlock();
	
	for (auto error : errorMap)
	{
		if (error.second.second)
		{
			std::stringstream message;
			message <<error.second.first << "DuplicateCount = " << error.second.second << "\n";
			RemoteLogSend (message.str (), RLRQ_RemoteLogAssert);
		}
	}
}

// end file CstiRemoteLoggerService.cpp
