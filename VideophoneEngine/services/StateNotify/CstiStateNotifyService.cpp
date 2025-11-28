////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiStateNotifyService
//
//  File Name:	CstiStateNotifyService.cpp
//
//	Abstract:
//		The StateNotifyService task implements the functions needed to communicate with
//		the videophone StateNotify Service.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include <cctype>	// standard type definitions.
#include <cstring>

#include "stiOS.h"			// OS Abstraction
#include "CstiStateNotifyService.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiHTTPTask.h"
#include "CstiStateNotifyRequest.h"
#include "CstiCoreRequest.h"
#include "CstiCoreService.h"
#include "stiCoreRequestDefines.h"

#include "stiTaskInfo.h"

//
// Constants
//
//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-StateNotifyService: %s\n", #name);

static const unsigned int unMAX_TRYS_ON_SERVER = 3;		// Number of times to try the first server
static const unsigned int unBASE_RETRY_TIMEOUT = 10;	// Number of seconds for the first retry attempt
static const int nMAX_SERVERS = 2;						// The number of servers to try connecting to.

static const NodeTypeMapping g_aNodeTypeMappings[] =
{
	{"AllowNumberChange", CstiStateNotifyResponse::eALLOW_NUMBER_CHANGE},
#if 0
	{"BlockedListCountChanged", CstiStateNotifyResponse::eBLOCKED_LIST_CHANGED},
#endif
	{"CallListChanged", CstiStateNotifyResponse::eCALL_LIST_CHANGED},
	{"CallListItemAdd", CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD},
	{"CallListItemEdit", CstiStateNotifyResponse::eCALL_LIST_ITEM_EDIT},
	{"CallListItemRemove", CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE},
	{"ClientReauthenticate", CstiStateNotifyResponse::eCLIENT_REAUTHENTICATE},
	{"EmergencyProvisionStatusChange", CstiStateNotifyResponse::eEMERGENCY_PROVISION_STATUS_CHANGE},
	{"EmergencyAddressUpdateVerify", CstiStateNotifyResponse::eEMERGENCY_ADDRESS_UPDATE_VERIFY},
	{"GroupChanged", CstiStateNotifyResponse::eGROUP_CHANGED},
	{"HeartbeatResult", CstiStateNotifyResponse::eHEARTBEAT_RESULT},
	{"LoggedOut", CstiStateNotifyResponse::eLOGGED_OUT},
	{"Login", CstiStateNotifyResponse::eLOGIN},
	{"NewMessageReceived", CstiStateNotifyResponse::eNEW_MESSAGE_RECEIVED},
	{"MessageListChanged", CstiStateNotifyResponse::eMESSAGE_LIST_CHANGED},
	{"PasswordChanged", CstiStateNotifyResponse::ePASSWORD_CHANGED},
	{"PublicIpAddressChanged", CstiStateNotifyResponse::ePUBLIC_IP_ADDRESS_CHANGED},
	{"PublicIpSettingsChanged", CstiStateNotifyResponse::ePUBLIC_IP_SETTINGS_CHANGED},
	{"PublicIPGetResult", CstiStateNotifyResponse::ePUBLIC_IP_GET_RESULT},
	{"PhoneSettingChanged", CstiStateNotifyResponse::ePHONE_SETTING_CHANGED},
	{"RebootPhone", CstiStateNotifyResponse::eREBOOT_PHONE},
	{"RouterRevalidate", CstiStateNotifyResponse::eROUTER_REVALIDATE},
	{"ServiceContactResult", CstiStateNotifyResponse::eSERVICE_CONTACT_RESULT},
#if 0
	{"SignMailListChanged", CstiStateNotifyResponse::eSIGNMAIL_LIST_CHANGED},
#endif
	{"TimeChanged", CstiStateNotifyResponse::eTIME_CHANGED},
	{"TimeGetResult", CstiStateNotifyResponse::eTIME_GET_RESULT},
	{"UpdateVersionCheckForce", CstiStateNotifyResponse::eUPDATE_VERSION_CHECK_FORCE},
	{"UserAccountInfoSettingChanged", CstiStateNotifyResponse::eUSER_ACCOUNT_INFO_SETTING_CHANGED},
	{"UserDisassociated", CstiStateNotifyResponse::eUSER_DISASSOCIATED},
	{"UserSettingChanged", CstiStateNotifyResponse::eUSER_SETTING_CHANGED},
	{"Error", CstiStateNotifyResponse::eRESPONSE_ERROR},
#if 0
	{"ServiceMaskChange", CstiStateNotifyResponse::eSERVICE_MASK_CHANGED},
#endif
	{"SetCurrentUserPrimary", CstiStateNotifyResponse::eSET_CURRENT_USER_PRIMARY},
	{"DisplayProviderAgreement", CstiStateNotifyResponse::eDISPLAY_PROVIDER_AGREEMENT},
	{"DisplayRegistrationWizard", CstiStateNotifyResponse::eDISPLAY_REGISTRATION_WIZARD},
	{"UserInterfaceGroupChange", CstiStateNotifyResponse::eUSER_INTERFACE_GROUP_CHANGED},
	{"RingGroupCreated", CstiStateNotifyResponse::eRING_GROUP_CREATED},
	{"RingGroupInvite", CstiStateNotifyResponse::eRING_GROUP_INVITE},
	{"RingGroupRemoved", CstiStateNotifyResponse::eRING_GROUP_REMOVED},
	{"RingGroupInfoChanged", CstiStateNotifyResponse::eRING_GROUP_INFO_CHANGED},
	{"RegistrationInfoChanged", CstiStateNotifyResponse::eREGISTRATION_INFO_CHANGED},
	{"DisplayRingGroupWizard", CstiStateNotifyResponse::eDISPLAY_RING_GROUP_WIZARD},
	{"AgreementChanged", CstiStateNotifyResponse::eAGREEMENT_CHANGED},
	{"UserNotification", CstiStateNotifyResponse::eUSER_NOTIFICATION},
	{"ProfileImageChanged", CstiStateNotifyResponse::ePROFILE_IMAGE_CHANGED},
	{"DiagnosticsGet", CstiStateNotifyResponse::eDIAGNOSTICS_GET},
	{"FavoriteListChanged", CstiStateNotifyResponse::eFAVORITE_LIST_CHANGED},
	{"FavoriteAdded", CstiStateNotifyResponse::eFAVORITE_ADDED},
	{"FavoriteRemoved", CstiStateNotifyResponse::eFAVORITE_REMOVED},
	{"ClientReregister", CstiStateNotifyResponse::eCLIENT_REREGISTER}
};

static const int g_nNumNodeTypeMappings = sizeof(g_aNodeTypeMappings) / sizeof(g_aNodeTypeMappings[0]);


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
//; CstiStateNotifyService::CstiStateNotifyService
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiStateNotifyService::CstiStateNotifyService (
	CstiCoreService *pCoreService,
	CstiHTTPTask *pHTTPTask,
	PstiObjectCallback pAppCallback,
	size_t CallbackParam)
	:
	CstiVPService (
		pHTTPTask,
		pAppCallback,
		CallbackParam,
		(size_t)this,
		stiNS_MAX_MSG_SIZE,
		SNRQ_StateNotifyResponse,
		stiNS_TASK_NAME,
		unMAX_TRYS_ON_SERVER,
		unBASE_RETRY_TIMEOUT,
		nDEFAULT_HEARTBEAT_DELAY,
		nMAX_SERVERS),
	m_bConnected (estiFALSE),
	m_unHeartbeatRequestId (0),
	m_heartbeatTimer (nDEFAULT_HEARTBEAT_DELAY * 1000, this),
	m_nHeartbeatDelay (nDEFAULT_HEARTBEAT_DELAY),
	m_pCoreService (pCoreService)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::CstiStateNotifyService);

	m_signalConnections.push_back(m_heartbeatTimer.timeoutSignal.Connect ([this]() {
		HeartbeatSend ();
	}));

	m_LastIP.clear();

} // end CstiStateNotifyService::CstiStateNotifyService


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::~CstiStateNotifyService
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	none
//
CstiStateNotifyService::~CstiStateNotifyService ()
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::~CstiStateNotifyService);

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

} // end CstiStateNotifyService::~CstiStateNotifyService

////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//
// Returns:
//
stiHResult CstiStateNotifyService::Initialize (
	const std::string *serviceContacts,
	const std::string &uniqueID)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	// NOTE: we don't need to call Cleanup to free up the allocated memory
	// if the Initialize function is called only once.

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

	CstiVPService::Initialize(serviceContacts, uniqueID);

	return (hResult);

} // end CstiStateNotifyService::Initialize


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::ShutdownHandle
//
// Description: Shuts down the StateNotify Service task.
//
// Abstract:
//
// Returns:
//	none
//
stiHResult CstiStateNotifyService::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_unHeartbeatRequestId = 0;

	m_heartbeatTimer.stop ();

	// Call the base class ShutdownHandle
	hResult = CstiVPService::ShutdownHandle ();

	return (hResult);

} // end CstiStateNotifyService::ShutdownHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::ConnectHandle
//
// Description: Handles initial connection to the server
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
EstiResult CstiStateNotifyService::ConnectHandle ()
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::ConnectHandle);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	const char *pszRequestURL = nullptr;
	int nResult;

	stiDEBUG_TOOL (g_stiNSDebug,
		stiTrace ("CstiStateNotifyService::ConnectHandle\n");
	); // stiDEBUG_TOOL

	// Create a StateNotify request object
	auto  poStateNotifyRequest = new CstiStateNotifyRequest ();
	stiTESTCOND (poStateNotifyRequest != nullptr, stiRESULT_ERROR);

	//NOTE: the server ignores any parameters passed in ServiceContact, just use a fake string
	pszRequestURL = "Fake";

	char szPriority[12];
	sprintf (szPriority, "%d", 0);

	nResult = poStateNotifyRequest->serviceContact (szPriority, pszRequestURL);
	stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

	// Yes! Send the request.
	hResult = RequestQueue (poStateNotifyRequest, 500);
	stiTESTRESULT ();

	poStateNotifyRequest = nullptr;

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poStateNotifyRequest)
		{
			delete poStateNotifyRequest;
			poStateNotifyRequest = nullptr;
		}

		eResult = estiERROR;
	}

	return (eResult);
} // end CstiStateNotifyService::ConnectHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::RequestRemoved
//
// Description: Method for handling cleanup after a request has been removed.
//
// Abstract:
//
// Returns:
//	None.
//
void CstiStateNotifyService::RequestRemoved(
	unsigned int unRequestId)
{
	if (unRequestId == m_unHeartbeatRequestId)
	{
		m_unHeartbeatRequestId = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::HeartbeatRequestHandle
//
// Description: Requests a heartbeat message after a specified delay
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
void CstiStateNotifyService::HeartbeatRequestHandle (uint32_t delaySeconds)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::HeartbeatRequestHandle);

	stiDEBUG_TOOL (g_stiNSDebug,
		stiTrace ("CstiStateNotifyService::HeartbeatRequestHandle\n");
	); // stiDEBUG_TOOL

	// Set the heartbeat timer to fire in the specified time
	m_heartbeatTimer.timeoutSet (static_cast<int>(delaySeconds) * 1000);
	m_heartbeatTimer.start ();

} // end CstiStateNotifyService::HeartbeatRequestHandle


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::HeartbeatSend
//
// Description: Sends a heartbeat request to the StateNotify server.
//
// Abstract:
//
// Returns:
//	None
//
stiHResult CstiStateNotifyService::HeartbeatSend ()
{

	stiLOG_ENTRY_NAME (CstiStateNotifyService::HeartbeatSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;

	if (m_unHeartbeatRequestId == 0)
	{
		// Create a StateNotify request object
		auto poStateNotifyRequest = new CstiStateNotifyRequest ();

		stiTESTCOND(poStateNotifyRequest != nullptr, stiRESULT_ERROR);

		poStateNotifyRequest->requestIdSet(NextRequestIdGet());

		// Yes! Create the Heartbeat request.
		nResult = poStateNotifyRequest->heartbeat ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

		nResult = poStateNotifyRequest->timeGet ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

		nResult = poStateNotifyRequest->publicIPGet ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

		poStateNotifyRequest->RepeatOnErrorSet(true);

		m_unHeartbeatRequestId = poStateNotifyRequest->requestIdGet();

		hResult = RequestQueue(poStateNotifyRequest, 500);

		stiTESTRESULT ();
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		m_unHeartbeatRequestId = 0;
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::StateSet
//
// Description: Sets the current state
//
// Abstract:
//
// Returns:
//	None
//
stiHResult CstiStateNotifyService::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiVPService::StateSet(eState);

	stiTESTRESULT();

	// Switch on the new state...
	switch (eState)
	{
		default:
		case eUNITIALIZED:
		{
			stiDEBUG_TOOL (g_stiNSDebug,
				stiTrace ("CstiStateNotifyService::StateSet - Changing state to eUNITIALIZED\n");
			); // stiDEBUG_TOOL

			break;
		} // end case eUNITIALIZED

		case eDISCONNECTED:
		{
			stiDEBUG_TOOL (g_stiNSDebug,
				stiTrace ("CstiStateNotifyService::StateSet - Changing state to eDISCONNECTED\n");
			); // stiDEBUG_TOOL

			m_bConnected = estiFALSE;

			break;
		} // end case eDISCONNECTED

		case eCONNECTING:
		{
			stiDEBUG_TOOL (g_stiNSDebug,
				stiTrace ("CstiStateNotifyService::StateSet - Changing state to eCONNECTING\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_STATE_NOTIFY_SERVICE_CONNECTING, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING

		case eCONNECTED:
		{
			stiDEBUG_TOOL (g_stiNSDebug,
				stiTrace ("CstiStateNotifyService::StateSet - Changing state to eCONNECTED\n");
			); // stiDEBUG_TOOL

			if (!m_bConnected)
			{
				// If we haven't been in a Connected state start the Heartbeat notification and
				// send the "Connected" message to the application.
				hResult = HeartbeatSend ();

				stiTESTRESULT();

				hResult = Callback (estiMSG_STATE_NOTIFY_SERVICE_CONNECTED, nullptr);

				stiTESTRESULT ();

				// Keep track that we have been connected.
				m_bConnected = estiTRUE;
			} // end if
			else
			{
				// Since we already have been connected, simply notify the application that we are now
				// reconnected.
				hResult = Callback (estiMSG_STATE_NOTIFY_SERVICE_RECONNECTED, nullptr);

				stiTESTRESULT ();

			} // end else
			break;
		} // end case eCONNECTED

		case eDNSERROR:
		{
			stiDEBUG_TOOL (g_stiNSDebug,
				stiTrace ("CstiStateNotifyService::StateSet - Changing state to eDNSERROR\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_DNS_ERROR, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING
	} // end switch

STI_BAIL:

	return hResult;

} // end CstiStateNotifyService::StateSet


stiHResult CstiStateNotifyService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (nSystemicError == 0)
	{
		nSystemicError = CstiStateNotifyResponse::eUNKNOWN_ERROR;
	}

	// Create a new Core Response
	auto response = new CstiStateNotifyResponse (
		pRequest,
		(CstiStateNotifyResponse::EResponse)nResponse,
		estiERROR,
		(CstiStateNotifyResponse::EStateNotifyError)nSystemicError,
		nullptr);
	stiTESTCOND (response != nullptr, stiRESULT_ERROR);

	Callback (estiMSG_STATE_NOTIFY_RESPONSE, response);

STI_BAIL:

	return hResult;
}


CstiStateNotifyResponse::EStateNotifyError CstiStateNotifyService::NodeErrorGet (
	IXML_Node *pNode,
	EstiBool *pbDatabaseAvailable)
{
	CstiStateNotifyResponse::EStateNotifyError eError = CstiStateNotifyResponse::eNO_ERROR;

	// Determine the error
	const char * szErrNum = AttributeValueGet (pNode, "ErrorNum");

	if ((nullptr != szErrNum))
	{
		eError = (CstiStateNotifyResponse::EStateNotifyError)atoi (szErrNum);

		if (eError == CstiStateNotifyResponse::eNOT_LOGGED_IN)
		{
			// Did we receive a database connection error?
			if (estiFALSE == *pbDatabaseAvailable)
			{
				// Yes, change the error code to be that database error code
				eError = CstiStateNotifyResponse::eDATABASE_CONNECTION_ERROR;
			}
		}
	}
	else
	{
		// Yes! Find out what type
		const char *pszErrorType = AttributeValueGet (pNode, "Type");

		if (nullptr != pszErrorType)
		{
			if (strcmp (pszErrorType, "Validation") == 0)
			{
				eError = CstiStateNotifyResponse::eXML_VALIDATION_ERROR;
			} // end if
			else if (strcmp (pszErrorType, "XML Format") == 0)
			{
				eError = CstiStateNotifyResponse::eXML_FORMAT_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Generic") == 0)
			{
				eError = CstiStateNotifyResponse::eGENERIC_SERVER_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Database Connection") == 0)
			{
				eError = CstiStateNotifyResponse::eDATABASE_CONNECTION_ERROR;
				// Set flag so any "not logged in" errors in this response will be ignored
				*pbDatabaseAvailable = estiFALSE;
			} // end else if
			else
			{
				eError = CstiStateNotifyResponse::eUNKNOWN_ERROR;
			} // end else
		}
	}

	return eError;
}


stiHResult CstiStateNotifyService::LoggedOutErrorProcess (
	CstiVPServiceRequest *pRequest,
	EstiBool * pbLoggedOutNotified)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Process this error only if the database is available
	if (!*pbLoggedOutNotified)
	{
		//
		// Check first to see if the client token for this request is
		// the same or different from the one assigned to this service.
		// If they are the same then the service's client token must be
		// bad.  If they are different then we can assume that the service's
		// client token is good.
		//
		auto currentToken = clientTokenGet ();

		if (!currentToken.empty ())
		{
			if (pRequest)
			{
				pRequest->AuthenticationAttemptsSet (pRequest->AuthenticationAttemptsGet () + 1);

				auto requestToken = pRequest->clientTokenGet();

				if (currentToken == requestToken)
				{
					m_pCoreService->LoggedOutNotifyAsync (currentToken);

					clientTokenSet({});

					*pbLoggedOutNotified = estiTRUE;
				}
			}
		}
		else
		{
			if (pRequest)
			{
				pRequest->AuthenticationAttemptsSet (pRequest->AuthenticationAttemptsGet () + 1);
			}

			m_pCoreService->LoggedOutNotifyAsync (currentToken);

			*pbLoggedOutNotified = estiTRUE;
		}
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::ResponseNodeProcess
//
// Description: Parses the indicated node
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise EstiERROR
//
stiHResult CstiStateNotifyService::ResponseNodeProcess (
	IXML_Node * pNode,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiBool * pbDatabaseAvailable,
	EstiBool * pbLoggedOutNotified,
	int *pnSystemicError,
	EstiRequestRepeat *peRepeatRequest)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::ResponseNodeProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Remove this request, unless otherwise instructed.
	*peRepeatRequest = estiRR_NO;

	// Response Object pointer
	CstiStateNotifyResponse * poResponse = nullptr;

	IXML_NodeList * pChildNodes = nullptr;
	IXML_NamedNodeMap * pAttributes = nullptr;
	unsigned long ulAttributeCount = 0; stiUNUSED_ARG (ulAttributeCount);

	stiDEBUG_TOOL (g_stiNSDebug,
		stiTrace ("CstiStateNotifyService::ResponseNodeProcess - Node Name: %s\n",
			ixmlNode_getNodeName (pNode));
	);

	// Find the original request and obtain the context parameter
	VPServiceRequestContextSharedPtr context;

	if (pRequest != nullptr)
	{
		context = pRequest->contextGet ();
	}

	// Does the node have attributes?
	if (ixmlNode_hasAttributes (pNode))
	{
		// Yes! Get them so we can pass them along.
		pAttributes = ixmlNode_getAttributes (pNode);

		// Get the count of attributes, too.
		ulAttributeCount = ixmlNamedNodeMap_getLength (pAttributes);
	} // end if

	// Retrieve any child nodes
	pChildNodes = ixmlNode_getChildNodes (pNode);

	// Determine the type of node...
	auto  eResponse = (CstiStateNotifyResponse::EResponse)NodeTypeDetermine (
		ixmlNode_getNodeName (pNode), g_aNodeTypeMappings, g_nNumNodeTypeMappings);

	if (pRequest)
	{
		pRequest->ExpectedResultHandled (eResponse);
	}

	// Did we get an OK result?
	if (ResultSuccess (pNode))
	{
		// Yes! Create a new response
		poResponse = new CstiStateNotifyResponse (
				pRequest,
				eResponse,
				estiOK,
				CstiStateNotifyResponse::eNO_ERROR,
				nullptr);

		stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

		// Yes! Handle the response
		switch (eResponse)
		{
			case CstiStateNotifyResponse::eALLOW_NUMBER_CHANGE:
			{
				const char * szType = AttributeValueGet (pNode, "Type");

				// Save the type of number that is allowed to be changed
				poResponse->ResponseStringSet (0, szType);

				// Notify the app
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eALLOW_NUMBER_CHANGE

			case CstiStateNotifyResponse::eCALL_LIST_CHANGED:
			{
				// Can we get the call list that has changed?
				const char * szResult = AttributeValueGet (pNode, ATT_Type);
				if (nullptr != szResult)
				{
					// Yes!

					// Is it the Missed list?
					if (0 == strcmp (szResult, VAL_Missed))
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eMISSED);
					} // end if

					// Is it the Contact list?
					else if (0 == strcmp (szResult, VAL_Contact))
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eCONTACT);
					} // end else if

					// Is it the Dialed list?
					else if (0 == strcmp (szResult, VAL_Dialed))
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eDIALED);
					} // end else if

					// Is it the Answered list?
					else if (0 == strcmp (szResult, VAL_Answered))
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eANSWERED);
					} // end else if

					else if (0 == strcmp (szResult, VAL_Blocked))
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eBLOCKED);
					} // end else if

					// Nope, none of those
					else
					{
						// Yes! Notify the app
						poResponse->ResponseValueSet (
							CstiCallList::eTYPE_UNKNOWN);

						// Pass the name of the list back to the app, as
						// well.
						poResponse->ResponseStringSet (0, szResult);
					} // end else
				} // end if

				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eCALL_LIST_CHANGED

/////////////////////>>>>>
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD:
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_EDIT:
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE:
			{
				// Get the type of list item that changed
				const char * szResult = AttributeValueGet (pNode, ATT_Type);
				if (nullptr != szResult)
				{
					// Is it the Contact list?
					if (0 == strcmp (szResult, VAL_Contact))
					{
						poResponse->ResponseValueSet (
							CstiCallList::eCONTACT);
					}
					// Blocked list?
					else if (0 == strcmp (szResult, VAL_Blocked))
					{
						poResponse->ResponseValueSet (
							CstiCallList::eBLOCKED);
					}

					// Get the ItemId from the ItemId attribute
					const char * szItemId = AttributeValueGet (pNode, TAG_ItemId);
					if (nullptr != szItemId)
					{
						// Save out the ItemId
						poResponse->ResponseStringSet (0, szItemId);
					}

					// Get the PublicId from the ItemId attribute (on contacts only!!)
// NOTE: Public Id not currently needed for this request
// 					const char * szPublicId = AttributeValueGet (pNode, (const char*)"PublicId");
// 					if (NULL != szPublicId)
// 					{
// 						// Save out the PublicId
// 						poResponse->ResponseStringSet (1, szPublicId);
// 					}
				} // end if

				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eCALL_LIST_ITEM_ADD/EDIT
/////////////////////>>>>>
			case CstiStateNotifyResponse::eFAVORITE_ADDED:
			case CstiStateNotifyResponse::eFAVORITE_REMOVED:
			case CstiStateNotifyResponse::eFAVORITE_LIST_CHANGED:
			{
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eFAVORITE_LIST_CHANGED, eFAVORITE_ADDED and eFAVORITE_REMOVED

			case CstiStateNotifyResponse::eCLIENT_REAUTHENTICATE:
			{
				//
				// Tell core that we need to reauthenticate.
				//
				clientTokenSet ({});
				m_pCoreService->ReauthenticateAsync ();


				break;
			} // end case eCLIENT_REAUTHENTICATE

			case CstiStateNotifyResponse::eHEARTBEAT_RESULT:
			{
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				std::uniform_int_distribution<int> engine (m_nHeartbeatDelay, m_nHeartbeatDelay + m_nHeartbeatDelay);
				auto timerInterval = engine (m_numberGenerator);
				// Yes! Start it again.
				m_heartbeatTimer.timeoutSet (timerInterval* stiOSSysClkRateGet ());
				m_heartbeatTimer.start ();

				break;
			} // end case eHEARTBEAT_RESULT

			case CstiStateNotifyResponse::eLOGGED_OUT:
			{
				// Notify the app
				// Get the User, ReasonNumber, ReasonMsg
				const char * szUser = AttributeValueGet (pNode, "User");
				const char * szReasonNumber = AttributeValueGet (pNode, "ReasonNumber");
				CstiStateNotifyResponse::ELogoutReason eReason =
					CstiStateNotifyResponse::eREASON_UNKNOWN;

				if (szReasonNumber != nullptr)
				{
					eReason = (CstiStateNotifyResponse::ELogoutReason) atoi (
						szReasonNumber);
				} // end if

				// Save the reason for logout
				poResponse->ResponseValueSet (eReason);

				// Save the person logged out
				poResponse->ResponseStringSet (0, szUser);

				// Yes! Notify the app.
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eLOGGED_OUT

			case CstiStateNotifyResponse::ePASSWORD_CHANGED:
			{
				// Notify the app
				// Get the ChangedTo value
				const char * pszChangedTo = AttributeValueGet (pNode, "ChangedTo");
				stiTESTCOND (pszChangedTo != nullptr, stiRESULT_ERROR);
				
				// Save the new password for the app to use
				poResponse->ResponseStringSet (0, pszChangedTo);

//RINGGROUP:
				// TODO: IS THIS NEEDED? if so, why?  We don't need to store this password, but do we need to notify the other phones in the group that it changed??  I can't think why we would.
				//
				// Get the User value
				const char * pszUser = AttributeValueGet (pNode, "User");
				// Save the User for the app to use so it knows which password to change
				if (pszUser)
				{
					poResponse->ResponseStringSet (1, pszUser);
				}
//end RINGGROUP

				// Notify the app.
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case ePASSWORD_CHANGED

			case CstiStateNotifyResponse::ePUBLIC_IP_ADDRESS_CHANGED:
			{
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				// Tell the Core Service to re-request the public IP
				auto  poCoreRequest = new CstiCoreRequest;
				if (poCoreRequest != nullptr)
				{
					poCoreRequest->publicIPGet ();

					hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

					if (stiIS_ERROR (hResult))
					{
						delete poCoreRequest;
						poCoreRequest = nullptr;
					}

				} // end if
				break;
			} // end case ePUBLIC_IP_ADDRESS_CHANGED

			case CstiStateNotifyResponse::ePUBLIC_IP_SETTINGS_CHANGED:
			{
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			}

			case CstiStateNotifyResponse::ePUBLIC_IP_GET_RESULT:
			{
				const char * szPublicIP = SubElementValueGet (pNode, "PublicIP");

				// Did we get an IP address?
				if (nullptr != szPublicIP)
				{
					// Yes! Has the IP address changed?
					if (m_LastIP != szPublicIP)
					{
						// Yes! Have we previously stored a public IP?
						// If we have not we are assuming that we just started up
						// and the core service module will take care of getting the
						// initial public IP address.
						if (!m_LastIP.empty ())
						{
							// Yes! Tell the Core Service to re-request the public IP
							auto  poCoreRequest = new CstiCoreRequest;
							if (poCoreRequest != nullptr)
							{
								poCoreRequest->publicIPGet ();

								hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

								if (stiIS_ERROR (hResult))
								{
									delete poCoreRequest;
									poCoreRequest = nullptr;
								}
							} // end if
						} // end if

						// Store the new address.
						m_LastIP.assign (szPublicIP);
					} // end if
				} // end if
				break;
			} // end case CstiStateNotifyResponse::ePUBLIC_IP_GET_RESULT

			case CstiStateNotifyResponse::ePHONE_SETTING_CHANGED:
			{
				const char * szSettingName = AttributeValueGet (
					pNode, "PhoneSettingName");

				// Tell the Core Service to request the setting
				auto  poCoreRequest = new CstiCoreRequest;
				if (poCoreRequest != nullptr)
				{
					// Yes! Get the specified phone setting
					poCoreRequest->phoneSettingsGet(szSettingName);

					hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

					if (stiIS_ERROR (hResult))
					{
						delete poCoreRequest;
						poCoreRequest = nullptr;
					}
				} // end if
				break;
			} // end case ePHONE_SETTING_CHANGED

			case CstiStateNotifyResponse::eTIME_CHANGED:
			{
				// Notify the app
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				// Tell the Core Service to request the time
				auto  poCoreRequest = new CstiCoreRequest;
				if (poCoreRequest != nullptr)
				{
					poCoreRequest->timeGet();

					hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

					if (stiIS_ERROR (hResult))
					{
						delete poCoreRequest;
						poCoreRequest = nullptr;
					}
				} // end if
				break;
			} // end case eTIME_CHANGED

			case CstiStateNotifyResponse::eTIME_GET_RESULT:
			{
				// Can we get the returned time?
				const char * szTime = SubElementValueGet (pNode, "Time");
				if (szTime != nullptr)
				{
					// Yes! Convert it to local time
					time_t ttTime;
					
					stiHResult hResult = TimeConvert (szTime, &ttTime);

					if (!stiIS_ERROR (hResult))
					{
						poResponse->timeSet(ttTime);

						// Send the time to CCI.
						hResult = Callback (estiMSG_CB_CURRENT_TIME_SET, poResponse);
						stiTESTRESULT ();

						// We no longer own this response, so clear it.
						poResponse = nullptr;
					}
				} // end if
				break;
			} // end case CstiStateNotifyResponse::eTIME_GET_RESULT

			case CstiStateNotifyResponse::eUSER_ACCOUNT_INFO_SETTING_CHANGED:
			{
				// Tell the Core Service to request the settings
				auto  poCoreRequest = new CstiCoreRequest;
				if (poCoreRequest != nullptr)
				{
					poCoreRequest->userAccountInfoGet();

					hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

					if (stiIS_ERROR (hResult))
					{
						delete poCoreRequest;
						poCoreRequest = nullptr;
					}
				} // end if

				break;
			} // end case eUSER_ACCOUNT_INFO_SETTING_CHANGED

			case CstiStateNotifyResponse::eUSER_DISASSOCIATED:
			{
				// Notify the app
				// Get the User
				const char * szUser = AttributeValueGet (pNode, "User");
				// Save the disassociated user
				poResponse->ResponseStringSet (0, szUser);

				// Notify the app.
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eUSER_DISASSOCIATED

			case CstiStateNotifyResponse::eUSER_SETTING_CHANGED:
			{
				const char * szSettingName = AttributeValueGet (
					pNode, "UserSettingName");

				// Tell the Core Service to request the setting
				auto  poCoreRequest = new CstiCoreRequest;
				if (poCoreRequest != nullptr)
				{
					poCoreRequest->userSettingsGet(szSettingName);

					hResult = m_pCoreService->RequestSendAsync (poCoreRequest, nullptr, 0);

					if (stiIS_ERROR (hResult))
					{
						delete poCoreRequest;
						poCoreRequest = nullptr;
					}
				} // end if

				break;
			} // end case eUSER_SETTING_CHANGED
			case CstiStateNotifyResponse::eUSER_NOTIFICATION:
			{
				const char * szNotificationType = AttributeValueGet (
					pNode, "Type");

				const char * szMessage = AttributeValueGet (
					pNode, "Message");

				//stiTrace("CstiStateNotifyResponse::eUSER_NOTIFICATION Type %s, Msg '%s'\n", szNotificationType, szMessage);

				// Save the notification type
				poResponse->ResponseStringSet (0, szNotificationType);

				// Save the notification type
				poResponse->ResponseStringSet (1, szMessage);

				// Notify the app.
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eUSER_NOTIFICATION

			case CstiStateNotifyResponse::eLOGIN:
			{
				const char *phoneNumber = AttributeValueGet(pNode, "PhoneNumber");

				const char *pin = AttributeValueGet (pNode, "Pin");

				// Save the phoneNumber
				poResponse->ResponseStringSet (0, phoneNumber);

				// Save the pin
				poResponse->ResponseStringSet (1, pin);

				// Notify the app.
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			}
#if 0
			case CstiStateNotifyResponse::eBLOCKED_LIST_CHANGED:
#endif
			case CstiStateNotifyResponse::eEMERGENCY_PROVISION_STATUS_CHANGE:
			case CstiStateNotifyResponse::eEMERGENCY_ADDRESS_UPDATE_VERIFY:
			case CstiStateNotifyResponse::eGROUP_CHANGED:
			case CstiStateNotifyResponse::eNEW_MESSAGE_RECEIVED:
			case CstiStateNotifyResponse::eMESSAGE_LIST_CHANGED:
			case CstiStateNotifyResponse::eREBOOT_PHONE:
			case CstiStateNotifyResponse::eROUTER_REVALIDATE:
#if 0
			case CstiStateNotifyResponse::eSIGNMAIL_LIST_CHANGED:
#endif
			case CstiStateNotifyResponse::eUPDATE_VERSION_CHECK_FORCE:
#if 0
			case CstiStateNotifyResponse::eSERVICE_MASK_CHANGED:
#endif
			case CstiStateNotifyResponse::eSET_CURRENT_USER_PRIMARY:
			case CstiStateNotifyResponse::eDISPLAY_PROVIDER_AGREEMENT:
			case CstiStateNotifyResponse::eDISPLAY_REGISTRATION_WIZARD:
			case CstiStateNotifyResponse::eRESPONSE_UNKNOWN:
			case CstiStateNotifyResponse::eSERVICE_CONTACT_RESULT:

			case CstiStateNotifyResponse::eRING_GROUP_CREATED:
			case CstiStateNotifyResponse::eRING_GROUP_REMOVED:
			case CstiStateNotifyResponse::eRING_GROUP_INVITE:
			case CstiStateNotifyResponse::eRING_GROUP_INFO_CHANGED:
			case CstiStateNotifyResponse::eREGISTRATION_INFO_CHANGED:
			case CstiStateNotifyResponse::eDISPLAY_RING_GROUP_WIZARD:
			case CstiStateNotifyResponse::eAGREEMENT_CHANGED:
			case CstiStateNotifyResponse::ePROFILE_IMAGE_CHANGED:
			case CstiStateNotifyResponse::eDIAGNOSTICS_GET:
			case CstiStateNotifyResponse::eCLIENT_REREGISTER:
			default:
			{
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eRESPONSE_UNKNOWN
		} // end switch
	} // end if
	else
	{
		CstiStateNotifyResponse::EStateNotifyError eErrorNum = CstiStateNotifyResponse::eUNKNOWN_ERROR;

		// No! Did we got a systemic error?
		if (eResponse == CstiStateNotifyResponse::eRESPONSE_ERROR)
		{
			*pnSystemicError = NodeErrorGet (pNode, pbDatabaseAvailable);

			if (eErrorNum == CstiStateNotifyResponse::eNOT_LOGGED_IN)
			{
				LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);
			}
		}
		else
		{
			eErrorNum = NodeErrorGet (pNode, pbDatabaseAvailable);

			if (eErrorNum != CstiStateNotifyResponse::eNO_ERROR)
			{
				// Handle errors that can affect any of the messages first
				switch (eErrorNum)
				{
					case CstiStateNotifyResponse::eNOT_LOGGED_IN: //case 1: // Not Logged in
					{
						LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);

						break;
					} // end case 1

					default:

						break;
				} // end switch
			} // end if
			else
			{

				// Create a new Response
				poResponse = new CstiStateNotifyResponse (
					pRequest,
					eResponse,
					estiERROR,
					(CstiStateNotifyResponse::EStateNotifyError)*pnSystemicError,
					ElementValueGet (pNode));

				stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

				// Notify the application of the result
				hResult = Callback (estiMSG_STATE_NOTIFY_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
			} // end if
		}

		// In any error case, restart the heartbeat timer.
		m_heartbeatTimer.timeoutSet (m_nHeartbeatDelay * 1000);
		m_heartbeatTimer.start ();
	} // end else

STI_BAIL:

	// Do we still have a StateNotifyResponse object hanging around?
	if (poResponse != nullptr)
	{
		// Yes! get rid of it now.
		poResponse->destroy ();
		poResponse = nullptr;
	} // end if

	// Were any attributes retrieved?
	if (pAttributes != nullptr)
	{
		// Yes! Free them now.
		ixmlNamedNodeMap_free (pAttributes);
	} // end if

	// Were there any child nodes?
	if (pChildNodes != nullptr)
	{
		// Yes! Free them now.
		ixmlNodeList_free (pChildNodes);
	} // end if

	return (hResult);
} // end CstiStateNotifyService::ResponseNodeProcess


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::Restart
//
// Description:
//	Restart the StateNotify services by initializing the member variables and trying to
//	connect to the core server again
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - if there is no eror. Otherwize, some errors occur.
//
stiHResult CstiStateNotifyService::Restart ()
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::Restart);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiVPService::Restart();

	// Cancel heartbeat
	m_heartbeatTimer.stop ();

	m_bConnected  = estiFALSE;

	PostEvent (std::bind (&CstiStateNotifyService::ConnectHandle, this));

	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

}


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::Startup
//
// Description: This is called to initialize the class.
//
// Abstract:
//	This method is called soon after the object is created
//
// Returns:
//
stiHResult CstiStateNotifyService::Startup (
	bool bPause)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyService::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiVPService::Startup (bPause);

	stiTESTRESULT();

	PostEvent (std::bind (&CstiStateNotifyService::ConnectHandle, this));

STI_BAIL:

	return hResult;
} // end CstiStateNotifyService::Startup


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::HeartbeatRequest
//
// Description: Requests a heartbeat message to be sent after a delay
//
// Abstract:
//	Requests that a heartbeat message be sent to the state notification server
//	after a specified delay. This allows the application to cause remote data
//	to be retrieved.
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
void CstiStateNotifyService::HeartbeatRequestAsync (
	uint32_t delaySeconds)	// Number of seconds to delay before sending message
{
	PostEvent(std::bind (&CstiStateNotifyService::HeartbeatRequestHandle, this, delaySeconds));
} // end CstiStateNotifyService::HeartbeatRequest


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::SSLFailedOver
//
// Description: Called by the base class to inform the derived class
// that SSL has failed over.
//
// Abstract:
//
// Returns:
//	None
//
void CstiStateNotifyService::SSLFailedOver()
{
	Callback (estiMSG_STATE_NOTIFY_SSL_FAILOVER, nullptr);
}


void CstiStateNotifyService::HeartbeatDelaySet (
	int nHeartbeatDelay)
{
	static const int minimumHeartbeatDelay {30};

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_nHeartbeatDelay = std::max(nHeartbeatDelay, minimumHeartbeatDelay);

	MaxRetryTimeoutSet (nHeartbeatDelay);
}


void CstiStateNotifyService::PublicIPClear ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	//
	// We cannot simply clear the last known public IP
	// because it won't force state notify to request
	// a public IP through the core services module so
	// we will just set the public IP to all zeros.
	//
	m_LastIP = "0.0.0.0";
}

void CstiStateNotifyService::clientTokenSet(const std::string& clientToken)
{
	CstiVPService::clientTokenSet(clientToken);
	m_unHeartbeatRequestId = 0;
}


#ifdef stiDEBUGGING_TOOLS
////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::IsDebugTraceSet
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
EstiBool CstiStateNotifyService::IsDebugTraceSet (const char **ppszService) const
{
	EstiBool bRet = estiFALSE;

	if (g_stiNSDebug)
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
 * \param ppszService Returns the service type string ("NS" in this case).
 */
const char * CstiStateNotifyService::ServiceIDGet () const
{
	return "NS";
}

// end file CstiStateNotifyService.cpp
