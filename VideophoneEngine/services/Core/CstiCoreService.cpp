///
/// \file CstiCoreService.cpp
/// \brief Definition of the CstiCoreService class.
///
/// This task class coordinates sending core requests and processes the received core
///  responses and sends the responses back to the application.  See also the
///  CstiCoreRequest and CstiCoreResponse classes.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///


//
// Includes
//

// uncomment this if you are debugging and want to check for mutex deadlocks
//#define CORE_SERVICE_DEADLOCK_DETECTOR 1

#include <cctype>					// standard type definitions.
#include <cstring>

#include "ixml.h"
#include "stiOS.h"					// OS Abstraction
#include "CstiCoreService.h"
#include "stiDefs.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiHTTPTask.h"
#include "CstiCoreService.h"
#include "CstiCallList.h"
#include "CstiCoreRequest.h"
#include "CstiStateNotifyService.h"
#include "CstiMessageService.h"
#include "CstiConferenceService.h"
#include "CstiRemoteLoggerService.h"
#include "stiCoreRequestDefines.h"
#include "CstiUserAccountInfo.h"
#include "CstiItemId.h"
#include "cmPropertyNameDef.h"

	#include "stiTaskInfo.h"

#ifdef stiFAVORITES
#include "SstiFavoriteList.h"
#endif

//
// Constants
//

static const unsigned int unMAX_TRYS_ON_SERVER = 3;		// Number of times to try the first server
static const unsigned int unBASE_RETRY_TIMEOUT = 10;	// Number of seconds for the first retry attempt
static const unsigned int unMAX_RETRY_TIMEOUT = 1800;	// Max retry timeout is 30 minutes
static const int nMAX_SERVERS = 2;						// The number of servers to try connecting to.
static const int32_t n32CS_REFRESH_TIME = 24*60*60;	// 24 hour Refresh interval for
														// ServiceContactsGet

static const int MAX_SERVICE_CONTACT_PRIORITY = 0;

static const NodeTypeMapping g_aNodeTypeMappings[] =
{
	{"AgreementGetResult", CstiCoreResponse::eAGREEMENT_GET_RESULT},
	{"AgreementStatusGetResult", CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT},
	{"AgreementStatusSetResult", CstiCoreResponse::eAGREEMENT_STATUS_SET_RESULT},
	{"CallListCountGetResult", CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT},
	{"CallListGetResult", CstiCoreResponse::eCALL_LIST_GET_RESULT},
	{"CallListItemAddResult", CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT},
	{"CallListAddResult", CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT},
	{"CallListItemEditResult", CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT},
	{"CallListItemGetResult", CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT},
	{"CallListItemRemoveResult", CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT},
	{"CallListSetResult", CstiCoreResponse::eCALL_LIST_SET_RESULT},
	{"ClientAuthenticateResult", CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT},
	{"DefaultUserSettingsGetResult", CstiCoreResponse::eUSER_DEFAULTS_RESULT},
	{"ClientLogoutResult", CstiCoreResponse::eCLIENT_LOGOUT_RESULT},
	{"DirectoryResolveResult", CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT},
	{"EmergencyAddressDeprovisionResult", CstiCoreResponse::eEMERGENCY_ADDRESS_DEPROVISION_RESULT},
	{"EmergencyAddressGetResult", CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT},
	{"EmergencyAddressProvisionResult", CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_RESULT},
	{"EmergencyAddressProvisionStatusGetResult", CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT},
	{"EmergencyAddressUpdateAcceptResult", CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT},
	{"EmergencyAddressUpdateRejectResult", CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT},
	{"FavoriteAddResult", CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT},
	{"FavoriteEditResult", CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT},
	{"FavoriteRemoveResult", CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT},
	{"FavoriteListGetResult", CstiCoreResponse::eFAVORITE_LIST_GET_RESULT},
	{"FavoriteListSetResult", CstiCoreResponse::eFAVORITE_LIST_SET_RESULT},
	{"ImageDeleteResult", CstiCoreResponse::eIMAGE_DELETE_RESULT},
	{"ImageDownloadURLGetResult", CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT},
	{"ImageUploadURLGetResult", CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT},
	{"PhoneAccountCreateResult", CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT},
	{"PhoneOnlineResult", CstiCoreResponse::ePHONE_ONLINE_RESULT},
	{"PhoneSettingsGetResult", CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT},
	{"PhoneSettingsSetResult", CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT},
	{"PrimaryUserExistsResult", CstiCoreResponse::ePRIMARY_USER_EXISTS_RESULT},
	{"PublicIPGetResult", CstiCoreResponse::ePUBLIC_IP_GET_RESULT},
	{"RegistrationInfoGetResult", CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT},
	{"RingGroupCreateResult", CstiCoreResponse::eRING_GROUP_CREATE_RESULT},
	{"RingGroupCredentialsUpdateResult", CstiCoreResponse::eRING_GROUP_CREDENTIALS_UPDATE_RESULT},
	{"RingGroupCredentialsValidateResult", CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT},
	{"RingGroupInfoGetResult", CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT},
	{"RingGroupInfoSetResult", CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT},
	{"RingGroupInviteAcceptResult", CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT},
	{"RingGroupInviteInfoGetResult", CstiCoreResponse::eRING_GROUP_INVITE_INFO_GET_RESULT},
	{"RingGroupInviteRejectResult", CstiCoreResponse::eRING_GROUP_INVITE_REJECT_RESULT},
	{"RingGroupPasswordSetResult", CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT},
	{"RingGroupUserInviteResult", CstiCoreResponse::eRING_GROUP_USER_INVITE_RESULT},
	{"RingGroupUserRemoveResult", CstiCoreResponse::eRING_GROUP_USER_REMOVE_RESULT},
	{"MissedCallAddResult", CstiCoreResponse::eMISSED_CALL_ADD_RESULT},
	{"ServiceContactsGetResult", CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT},
	{"ServiceContactResult", CstiCoreResponse::eSERVICE_CONTACT_RESULT},
	{"SignMailRegisterResult", CstiCoreResponse::eSIGNMAIL_REGISTER_RESULT},
	{"TimeGetResult", CstiCoreResponse::eTIME_GET_RESULT},
	{"TrainerValidateResult", CstiCoreResponse::eTRAINER_VALIDATE_RESULT},
	{"URIListSetResult", CstiCoreResponse::eURI_LIST_SET_RESULT},
	{"UserAccountAssociateResult", CstiCoreResponse::eUSER_ACCOUNT_ASSOCIATE_RESULT},
	{"UserAccountInfoGetResult", CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT},
	{"UserAccountInfoSetResult", CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT},
	{"UserInterfaceGroupGetResult", CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT},
	{"UserLoginCheckResult", CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT},
	{"UserSettingsGetResult", CstiCoreResponse::eUSER_SETTINGS_GET_RESULT},
	{"UserSettingsSetResult", CstiCoreResponse::eUSER_SETTINGS_SET_RESULT},
	{"UpdateVersionCheckResult", CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT},
	{"VersionGetResult", CstiCoreResponse::eVERSION_GET_RESULT},
	{"LogCallTransferResult", CstiCoreResponse::eLOG_CALL_TRANSFER_RESULT},
	{"ContactsImportResult", CstiCoreResponse::eCONTACTS_IMPORT_RESULT},
	{"Error", CstiCoreResponse::eRESPONSE_ERROR},
	{"ScreenSaverListGet", CstiCoreResponse::eSCREEN_SAVER_LIST_GET_RESULT},
	{"VrsAgentHistoryAnsweredAdd", CstiCoreResponse::eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT},
	{"VrsAgentHistoryDialedAdd", CstiCoreResponse::eVRS_AGENT_HISTORY_DIALED_ADD_RESULT},
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

/**
 * \brief Constructor
 * \param pHTTPTask Pointer to HTTP Task object
 * \param pAppCallback Callback function pointer
 * \param CallbackParam Callback parameter
 */
CstiCoreService::CstiCoreService (
	CstiHTTPTask *pHTTPTask,
	PstiObjectCallback pAppCallback,
	size_t CallbackParam)
	:
	CstiVPService (
		pHTTPTask,
		pAppCallback,
		CallbackParam,
		(size_t)this,
		stiCS_MAX_MSG_SIZE,
		CRQ_CoreResponse,
		stiCS_TASK_NAME,
		unMAX_TRYS_ON_SERVER,
		unBASE_RETRY_TIMEOUT,
		unMAX_RETRY_TIMEOUT,
		nMAX_SERVERS),
	m_bAuthenticationInProcess (estiFALSE),
	m_bFirstAuthentication (estiTRUE),
	m_refreshTimer(n32CS_REFRESH_TIME * 1000, this),
	m_unConnectionId (0),
	m_nAPIMajorVersion (0),
	m_nAPIMinorVersion (0),
	m_pMessageService (nullptr),
	m_pStateNotifyService (nullptr),
	m_pConferenceService (nullptr),
	m_remoteLoggerService (nullptr)
{
	stiLOG_ENTRY_NAME (CstiCoreService::CstiCoreService);

	m_signalConnections.push_back (offlineCore.sendCoreResponse.Connect (
		[this](CstiCoreResponse* response) {
			PostEvent (std::bind (&CstiCoreService::offlineCoreResponseSend, this, response));
		}
	));
	m_signalConnections.push_back (offlineCore.credentialsUpdate.Connect (
		[this](const std::string& userName, const std::string& password) {
			PostEvent (std::bind (&CstiCoreService::offlineCoreCredentialsUpdate, this, userName, password));
		}
	));
	m_signalConnections.push_back (offlineCore.clientAuthenticateSignal.Connect (
		[this](const ServiceResponseSP<ClientAuthenticateResult>& response) {
			PostEvent (std::bind (&CstiCoreService::offlineCoreClientAuthenticate, this, response));
		}
	));
	m_signalConnections.push_back (offlineCore.userAccountInfoReceivedSignal.Connect (
		[this](const ServiceResponseSP<CstiUserAccountInfo>& response) {
			PostEvent ([this, response]() {
				userAccountInfoReceivedSignal.Emit (response);
			});
		}
	));
	m_signalConnections.push_back (offlineCore.emergencyAddressReceivedSignal.Connect (
		[this](const ServiceResponseSP<vpe::Address>& response) {
			PostEvent ([this, response]() {
				emergencyAddressReceivedSignal.Emit (response);
			});
		}
	));
	m_signalConnections.push_back (offlineCore.emergencyAddressStatusReceivedSignal.Connect (
		[this](const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>& response) {
			PostEvent ([this, response]() {
				emergencyAddressStatusReceivedSignal.Emit (response);
			});
		}
	));
	offlineCore.nextRequestIdGet = [this]() { return this->NextRequestIdGet (); };
} // end CstiCoreService::CstiCoreService


/**
 * \brief Destructor
 */
CstiCoreService::~CstiCoreService ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::~CstiCoreService);

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();
} // end CstiCoreService::~CstiCoreService


/**
 * \brief Initialization method for the class
 * \param pServiceContacts The service contacts for Core services
 * \param pszUniqueID The unique ID for this phone (MAC address)
 * \param pszPhoneType The type of phone
 */
stiHResult CstiCoreService::Initialize (
	const std::string *serviceContacts,
	const std::string &uniqueID,
	ProductType productType)
{
	stiLOG_ENTRY_NAME (CstiCoreService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	// NOTE: we don't need to call Cleanup to free up the allocated memory
	// if the Initialize function is called only once.

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

	m_productType = productType;

	CstiCoreRequest::SetUniqueID (uniqueID);
	CstiVPService::Initialize(serviceContacts, uniqueID);

	return (hResult);
} // end CstiCoreService::Initialize


/**
 * \brief Sets a pointer to the MessageService object
 * \param pMessageService The pointer to the MessageService object
 */
stiHResult CstiCoreService::MessageServiceSet (
	CstiMessageService *pMessageService)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_pMessageService = pMessageService;

	return hResult;
}


/**
 * \brief Sets a pointer to the ConferenceService object
 * \param pGroupChatService The pointer to the Conference Service object
 */
stiHResult CstiCoreService::ConferenceServiceSet (
	CstiConferenceService *pConferenceService)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_pConferenceService = pConferenceService;

	return hResult;
}


/**
 * \brief Sets a pointer to the StateNotifyService object
 * \param pStateNotifyService The pointer to the StateNotifyService object
 */
stiHResult CstiCoreService::StateNotifyServiceSet (
	CstiStateNotifyService *pStateNotifyService)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_pStateNotifyService = pStateNotifyService;

	return hResult;
}


stiHResult CstiCoreService::RemoteLoggerServiceSet (
	CstiRemoteLoggerService *remoteLoggerService)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_remoteLoggerService = remoteLoggerService;

	return hResult;
}


/**
 * \brief Shuts down the Core Service task.
 */
stiHResult CstiCoreService::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_refreshTimer.stop ();

	//
	// Call the base class ShutdownHandle
	//
	hResult = CstiVPService::ShutdownHandle ();

	return (hResult);
} // end CstiCoreService::ShutdownHandle


/**
 * \brief Queues up the ServiceContactsGet core request.
 */
stiHResult CstiCoreService::QueueServiceContactsGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;
	CstiCoreRequest *poCoreRequest = nullptr;

	if (m_unConnectionId == 0)
	{
		// We do NOT have a connection request in the queue.  Let's queue it up.
		stiDEBUG_TOOL (g_stiCSDebug,
			stiTrace ("CstiCoreService::HTTPError - "
				"Queuing a \"Connection\" request.\n");
		); // stiDEBUG_TOOL

		//
		// Send the service contacts get request by itself
		// without SSL encryption.  Also make sure that it stays
		// in the queue until it is successful.
		//
		poCoreRequest = new CstiCoreRequest ();
		stiTESTCOND(poCoreRequest != nullptr, stiRESULT_ERROR);

		poCoreRequest->requestIdSet (NextRequestIdGet ());

		// Set the SSL mode.
		// Force to not use ssl for this core request
		poCoreRequest->SSLEnabledSet(estiFALSE);

		nResult = poCoreRequest->serviceContactsGet ();
		stiTESTCOND(nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

		nResult = poCoreRequest->timeGet ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_CORE_REQUEST_ERROR);

#ifndef USE_PROXY_FOR_PUBLIC_IP
		nResult = poCoreRequest->publicIPGet ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);
#endif // USE_PROXY_FOR_PUBLIC_IP

		nResult = poCoreRequest->versionGet ();
		stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

		poCoreRequest->RepeatOnErrorSet (true);

		m_unConnectionId = poCoreRequest->requestIdGet ();

		hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());
		stiTESTRESULT ();

		poCoreRequest = nullptr;
	}
	else
	{
		// We have a connection request in the queue.  Don't requeue it.
		stiDEBUG_TOOL (g_stiCSDebug,
			stiTrace ("CstiCoreService::HTTPError - "
				"Found a \"Connection\" request already in the array; not requeuing.\n");
		); // stiDEBUG_TOOL
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poCoreRequest)
		{
			delete poCoreRequest;
			poCoreRequest = nullptr;
		}
	}

	return hResult;
}


/**
 * \brief Sends the request at the top of the queue.
 * \param poEvent Pointer to the event message
 * \return estiOK (Success), estiERROR (Failure)
 */
stiHResult CstiCoreService::RefreshHandle ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::RefreshHandle);

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiCoreService::RefreshHandle\n");
	); // stiDEBUG_TOOL

	stiHResult hResult = stiRESULT_SUCCESS;

	// Are we connected or authenticated?
	if (eCONNECTED == StateGet() ||
		eAUTHENTICATED == StateGet())
	{
		hResult = QueueServiceContactsGet ();
		stiTESTRESULT ();

		m_refreshTimer.start ();
	} // end if

STI_BAIL:

	return (hResult);
} // end CstiCoreService::RefreshHandle


/**
 * \brief Handle the logged out message
 * \param poEvent Pointer to the event message
 */
void CstiCoreService::LoggedOutNotifyHandle (const std::string& token)
{
	stiLOG_ENTRY_NAME (CstiCoreService::LoggedOutNotify);

	if (clientTokenGet().empty () || (token == clientTokenGet()))
	{
		clientTokenSet({});

		Reauthenticate ();
	}
} // end CstiCoreService::LoggedOutNotifyHandle


/**
 * \brief Sends the request at the top of the queue.
 * \param poEvent Pointer to the event message
 * \return stiHResult - success or failure
 */
stiHResult CstiCoreService::RequestSendHandle (SstiAsyncRequest* pAsyncRequest)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RequestSendHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiCoreService::RequestSendHandle\n");
	); // stiDEBUG_TOOL

	if (pAsyncRequest)
	{
		if (pAsyncRequest->pCoreRequest)
		{
			unsigned int unRequestId;
			auto hResult = RequestSend (pAsyncRequest->pCoreRequest, &unRequestId, pAsyncRequest->nPriority);

			if (pAsyncRequest->Callback)
			{
				if (!stiIS_ERROR (hResult))
				{
					pAsyncRequest->Callback(estiMSG_CORE_ASYNC_REQUEST_ID, unRequestId,
											pAsyncRequest->CallbackParam, (size_t)this);
				}
				else
				{
					pAsyncRequest->Callback(estiMSG_CORE_ASYNC_REQUEST_FAILED, 0,
											pAsyncRequest->CallbackParam, (size_t)this);
				}
			}
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}
	else
	{
		hResult = requestSendHandle ();
		stiTESTRESULT();
	}

STI_BAIL:

	if (pAsyncRequest)
	{
		delete pAsyncRequest;
		pAsyncRequest = nullptr;
	}

	return (hResult);
} // end CstiCoreService::RequestSendHandle


/**
 * \brief Sets the current state
 * \param eState The state to set
 * \return stiHResult - success or failure
 */
stiHResult CstiCoreService::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiCoreService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto previousState = StateGet ();

	hResult = CstiVPService::StateSet(eState);

	stiTESTRESULT();

	// Switch on the new state...
	switch (eState)
	{
		default:
		case eUNITIALIZED:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eUNITIALIZED\n");
			); // stiDEBUG_TOOL

			break;
		} // end case eUNITIALIZED

		case eDISCONNECTED:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eDISCONNECTED\n");
			); // stiDEBUG_TOOL

			if (previousState != eUNITIALIZED)
			{
				offlineCore.enable ();
			}

			break;
		} // end case eDISCONNECTED

		case eCONNECTING:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eCONNECTING\n");
			); // stiDEBUG_TOOL

			hResult = Callback ((int32_t)estiMSG_CORE_SERVICE_CONNECTING, nullptr);

			stiTESTRESULT ();

			if (ErrorCountGet () == 1)
			{
				hResult = QueueServiceContactsGet ();
				stiTESTRESULT ();
			} // end if

			break;
		} // end case eCONNECTING

		case eCONNECTED:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eCONNECTED\n");
			); // stiDEBUG_TOOL

			offlineCore.disable ();

			// Start the refresh timer and send the "Connected" message to the application.
			hResult = Callback ((int32_t)estiMSG_CORE_SERVICE_CONNECTED, nullptr);

			stiTESTRESULT();

			// Yes! Start it,
			m_refreshTimer.start ();

			break;

		} // end case eCONNECTED

		case eAUTHENTICATED:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eAUTHENTICATED\n");
			); // stiDEBUG_TOOL

			if (!m_bAuthenticated)
			{
				if (m_bFirstAuthentication)
				{
					// Request a "post-authentication" heartbeat (only after first authentication)
					if (m_pStateNotifyService)
					{
						m_pStateNotifyService->HeartbeatRequestAsync(0);
					}
					// Any subsequent authentication is not the first, so set flag to false
					m_bFirstAuthentication = estiFALSE;
				}

				hResult = Callback (estiMSG_CORE_SERVICE_AUTHENTICATED, nullptr);

				stiTESTRESULT();

				// Keep track that we have been authenticated
				m_bAuthenticated = estiTRUE;
			} // end if
			else
			{
				// Since we already have been connected, simply notify the application that we are now
				// reconnected.
				hResult = Callback (estiMSG_CORE_SERVICE_RECONNECTED, nullptr);

				stiTESTRESULT();
			} // end else

			offlineCore.disable ();
			break;
		} // end case eAUTHENTICATED

		case eDNSERROR:
		{
			stiDEBUG_TOOL (g_stiCSDebug,
				stiTrace ("CstiCoreService::StateSet - Changing state to eDNSERROR\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_DNS_ERROR, nullptr);

			stiTESTRESULT();

			break;
		} // end case eDNSERROR
	} // end switch

STI_BAIL:

	return hResult;

} // end CstiCoreService::StateSet


/**
 * \brief Method for handling cleanup after a request has been removed.
 * \param unRequestId The ID of the request to remove
 */
void CstiCoreService::RequestRemoved(
	unsigned int unRequestId)
{
	if (unRequestId == m_unConnectionId)
	{
		m_unConnectionId = 0;
	}
}


/**
 * \brief Notifies the application layer that a request was removed.
 * \param poRemovedRequest
 * \return stiHResult - success or failure
 */
stiHResult CstiCoreService::RequestRemovedOnError(
	CstiVPServiceRequest *request)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RequestRemoved (request->requestIdGet());

	// Ensure offline Core gets enabled
	offlineCore.enable ();

	auto response = new CstiCoreResponse (request,
										  CstiCoreResponse::eRESPONSE_ERROR,
										  estiERROR,
										  CstiCoreResponse::eCONNECTION_ERROR,
										  nullptr);

	// Send the response to the application
	hResult = Callback (estiMSG_CORE_REQUEST_REMOVED, response);

	stiTESTRESULT();

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiCoreService::RequestRemovedOnError - "
			"Removing request id: %d\n",
			request->requestIdGet());
	); // stiDEBUG_TOOL

STI_BAIL:

	return hResult;
}


/**
 * \brief Processes the remaining response.
 * \param pRequest The associated Core request
 * \param nResponse The response value
 * \param nSystemicError The error value
 * \return stiHResult - success or failure
 */
stiHResult CstiCoreService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto coreError = CstiCoreResponse::eUNKNOWN_ERROR;

	if (nSystemicError != 0)
	{
		coreError = (CstiCoreResponse::ECoreError)nSystemicError;
	}

	// Create a new Core Response
	auto response = new CstiCoreResponse (
		pRequest,
		(CstiCoreResponse::EResponse)nResponse,
		estiERROR,
		coreError,
		nullptr);
	stiTESTCOND (response != nullptr, stiRESULT_ERROR);

	Callback (estiMSG_CORE_RESPONSE, response);

STI_BAIL:

	return hResult;
}


void CstiCoreService::LoggedOutErrorProcess (
	CstiVPServiceRequest *pRequest,
	EstiBool *pbLoggedOutNotified)
{
	stiHResult hResult = stiRESULT_SUCCESS;

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
					clientTokenSet({});

					hResult = Reauthenticate ();

					if (!stiIS_ERROR (hResult))
					{
						*pbLoggedOutNotified = estiTRUE;
					}
				}
			}
		}
		else
		{
			if (pRequest)
			{
				pRequest->AuthenticationAttemptsSet (pRequest->AuthenticationAttemptsGet () + 1);
			}

			hResult = Reauthenticate ();

			if (!stiIS_ERROR (hResult))
			{
				*pbLoggedOutNotified = estiTRUE;
			}
		}
	}
}


CstiCoreResponse::ECoreError CstiCoreService::NodeErrorGet (
	IXML_Node *pNode,
	EstiBool *pbDatabaseAvailable)
{
	CstiCoreResponse::ECoreError eError = CstiCoreResponse::eNO_ERROR;

	// Determine the error
	const char *pszErrNum = AttributeValueGet (pNode, "ErrorNum");

	if ((nullptr != pszErrNum))
	{
		eError = (CstiCoreResponse::ECoreError)atoi (pszErrNum);

		if (eError == CstiCoreResponse::eNOT_LOGGED_IN)
		{
			// Did we receive a database connection error?
			if (estiFALSE == *pbDatabaseAvailable)
			{
				// Yes, change the error code to be that database error code
				eError = CstiCoreResponse::eDATABASE_CONNECTION_ERROR;
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
				eError = CstiCoreResponse::eXML_VALIDATION_ERROR;
			} // end if
			else if (strcmp (pszErrorType, "XML Format") == 0)
			{
				eError = CstiCoreResponse::eXML_FORMAT_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Generic") == 0)
			{
				eError = CstiCoreResponse::eGENERIC_SERVER_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Database Connection") == 0)
			{
				eError = CstiCoreResponse::eDATABASE_CONNECTION_ERROR;
				// Set flag so any "not logged in" errors in this response will be ignored
				*pbDatabaseAvailable = estiFALSE;
			} // end else if
			else
			{
				eError = CstiCoreResponse::eUNKNOWN_ERROR;
			}
		}
	}

	return eError;
}

stiHResult CstiCoreService::Callback (
	int32_t message,
	CstiVPServiceResponse* response)
{
	if (response != nullptr && response->hasCallbackType(typeid(CstiCoreResponse)))
	{
		auto coreResponsePtr = std::shared_ptr<CstiCoreResponse> (static_cast<CstiCoreResponse*>(response), [](CstiCoreResponse* cr) { cr->destroy (); });
		response->m_shared_ptr = coreResponsePtr;
		response->callbackRaise (coreResponsePtr, response->ResponseResultGet() != estiOK);
	}
	return CstiVPService::Callback (message, response);
}



/**
 * \brief Parses the indicated node
 * \param pNode The node to process
 * \param pRequest Pointer to the associated request object
 * \param unRequestId The request ID
 * \param nPriority Priority level
 * \param pbDatabaseAvailable Pointer to database available boolean
 * \param pbLoggedOutNotified Pointer to logged out notified boolean
 * \param pnSystemicError Pointer to systemic error number
 * \param peRepeat Pointer to repeat request enum
 * \return stiHResult - success or failure result
 */
//
stiHResult CstiCoreService::ResponseNodeProcess (
	IXML_Node * pNode,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiBool * pbDatabaseAvailable,
	EstiBool * pbLoggedOutNotified,
	int *pnSystemicError,
	EstiRequestRepeat *peRepeat)
{
	stiLOG_ENTRY_NAME (CstiCoreService::ResponseNodeProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Remove this request, unless otherwise instructed.
	*peRepeat = estiRR_NO;

	CstiCoreResponse * poResponse = nullptr;

	IXML_NodeList * pChildNodes = nullptr;
	IXML_NamedNodeMap * pAttributes = nullptr;
	unsigned long ulAttributeCount = 0; stiUNUSED_ARG (ulAttributeCount);

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiCoreService::ResponseNodeProcess - Node Name: %s\n",
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
	auto  eResponse = (CstiCoreResponse::EResponse)NodeTypeDetermine (
		ixmlNode_getNodeName (pNode), g_aNodeTypeMappings, g_nNumNodeTypeMappings);

#if 1
	if (pRequest)
	{
		pRequest->ExpectedResultHandled (eResponse);
	}
#endif

	// Did we get an OK result?
	if (ResultSuccess (pNode))
	{
		// Yes!
		// Create a new Core Response
		poResponse = new CstiCoreResponse (
			pRequest,
			eResponse,
			estiOK,
			CstiCoreResponse::eNO_ERROR,
			nullptr);

		stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

		// Yes! Handle the response
		switch (eResponse)
		{
#if 0
			// eBLOCKED_LIST_COUNT_GET_RESULT
			case CstiCoreResponse::eBLOCKED_LIST_COUNT_GET_RESULT:
			{
				// Set the count into the response value.
				const char * szCount = AttributeValueGet (
					pNode, ATT_Count);
				unsigned int unCount = atoi (szCount);
				poResponse->ResponseValueSet (unCount);

				hResult = Callback (estiMSG_CORE_RESPONSE, (size_t)poResponse);

				stiTESTRESULT();

				// We no longer own this response, so clear it.
				poResponse = NULL;
				break;
			} // end case eBLOCKED_LIST_COUNT_GET_RESULT
#endif

#if 0
			//: eBLOCKED_LIST_GET_RESULT
			case CstiCoreResponse::eBLOCKED_LIST_GET_RESULT:
			{
				hResult = Callback (estiMSG_CORE_RESPONSE, (size_t)poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = NULL;

				break;
			} // end case eBLOCKED_LIST_GET_RESULT
#endif

			// NOTE: Process eCALL_LIST_COUNT_GET_RESULT, eCALL_LIST_GET_RESULT and eCALL_LIST_ITEM_GET_RESULT
			//  in the same case statement, since they are processed the same
			//: eCALL_LIST_COUNT_GET_RESULT
			//: eCALL_LIST_GET_RESULT
			//: eCALL_LIST_ITEM_GET_RESULT
			//: eCALL_LIST_ITEM_ADD_RESULT
			case CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT:
			case CstiCoreResponse::eCALL_LIST_GET_RESULT:
			case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
			case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
			case CstiCoreResponse::eCALL_LIST_SET_RESULT:
			{
				// Yes! Are there any call lists in the result?
				if (pChildNodes != nullptr)
				{
					// Yes!
					int y = 0;
					IXML_Node * pChildNode = nullptr;

					pChildNode = ixmlNodeList_item (pChildNodes, y++);
					if (!pChildNode)
					{
						// Notify the application of the result
						hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

						stiTESTRESULT ();

						// We no longer own this response, so clear it.
						poResponse = nullptr;
					}
					else
					{
						// Loop through the Call Lists, and process them.
						do
						{
							// Process this node
							hResult = CallListProcess (poResponse, pChildNode);
							stiTESTRESULT ();

							// Send the response to the application
							hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
							stiTESTRESULT ();

							// We no longer own this response, so clear it.
							poResponse = nullptr;

							// Create a new response object for the next
							// list
							poResponse = new CstiCoreResponse (
								pRequest,
								eResponse,
								estiOK,
								CstiCoreResponse::eNO_ERROR,
								nullptr);

							stiTESTCOND(poResponse != nullptr, stiRESULT_ERROR);
						} while ((pChildNode = ixmlNodeList_item (pChildNodes, y++)));
					} // end else if
				} // end if
				break;
			} // end case eCALL_LIST_GET_RESULT and eCALL_LIST_ITEM_GET_RESULT
#ifdef stiFAVORITES
			case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
			case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
			case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
			case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
			case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
				// Process this node
				hResult = FavoriteListProcess (poResponse, pNode);
				stiTESTRESULT ();

				// Send the response to the application
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
#endif
			//: eCLIENT_AUTHENTICATE_RESULT
			case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT:
			{
				auto response = successfulResponseCreate<ClientAuthenticateResult> (unRequestId);

				// Yes! Can we get the returned client token?
				const char *pszToken = SubElementValueGet (pNode, "ClientToken");
				if (pszToken != nullptr)
				{
					clientTokenSet(pszToken);

					// We are authenticated now.
					StateEvent(eSE_AUTHENTICATED);

					m_bAuthenticationInProcess = estiFALSE;

					// Set client tokens for the other services
					if (m_pStateNotifyService)
					{
						m_pStateNotifyService->clientTokenSet(pszToken);
					}

					if (m_pMessageService)
					{
						m_pMessageService->clientTokenSet(pszToken);
					}

					if (m_pConferenceService)
					{
						m_pConferenceService->clientTokenSet (pszToken);
					}

					response->content.authToken = pszToken;
				} // end if

				// Is there a phone user id?
				const char * pszUserId = SubElementValueGet (
					pNode, TAG_PhoneUserID);
				if (pszUserId != nullptr)
				{
					// Yes!  Save it for use when making VRS calls
					response->content.userId = pszUserId;
				} // end if

				// Is there a ring group user id?
				const char * pszGroupUserId = SubElementValueGet (
					pNode, TAG_RingGroupUserID);
				if (pszGroupUserId != nullptr)
				{
					// Yes!  Save it for use when making VRS calls
					response->content.groupUserId = pszGroupUserId;
				} // end if

				// Notify the application of the result
				clientAuthenticateSignal.Emit (response);
				pRequest->callbackRaise(response, false);

				break;
			} // end case eCLIENT_AUTHENTICATE_RESULT

			case CstiCoreResponse::eUSER_DEFAULTS_RESULT:
			{
				// Process the retrieved user default settings
				hResult = SettingsGetProcess (
					poResponse, pChildNodes, nullptr);

				stiTESTRESULT ();

				// Yes! Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eDEFAULT_SETTINGS_RESULT

			//: eCLIENT_LOGOUT_RESULT
			case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
			{
				StateEvent(eSE_LOGOUT);

				clientTokenSet({});

				// Set client tokens for the other services
				if (m_pStateNotifyService)
				{
					m_pStateNotifyService->clientTokenSet({});
				}

				if (m_pMessageService)
				{
					m_pMessageService->clientTokenSet({});
				}

				if (m_pConferenceService)
				{
					m_pConferenceService->clientTokenSet({});
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eCLIENT_LOGOUT_RESULT

			//: eDIRECTORY_RESOLVE_RESULT
			case CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT:
			{

				// Process the retrieved phone settings
				hResult = DirectoryResolveProcess (poResponse, pNode);
				
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eDIRECTORY_RESOLVE_RESULT

#if 0
			//: ePHONE_NUMBER_REQUEST_RESULT
			case CstiCoreResponse::ePHONE_NUMBER_REQUEST_RESULT:
			{
				const char * szPhone = SubElementValueGet (
					pNode, "PhoneNumber");

				if (szPhone != NULL)
				{
					poResponse->ResponseStringSet (0, szPhone);
				} // end if

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, (size_t)poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = NULL;
				break;
			} // end case ePHONE_NUMBER_REQUEST_RESULT
#endif

			//: ePHONE_SETTINGS_GET_RESULT
			case CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT:
			{
				// Process the retrieved phone settings
				//
				// We can get handed an empty Request which the processing routine will
				// incorrectly process and throw a memory access violation exception.  So, check it here.
				//
				if (pRequest != nullptr)
				{
					hResult = SettingsGetProcess (
						poResponse, pChildNodes, &((CstiCoreRequest *)pRequest)->m_PhoneSettingList);

					stiTESTRESULT ();

					// Yes! Notify the application of the result
					hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

					stiTESTRESULT ();
				}

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case ePHONE_SETTINGS_GET_RESULT

			//: ePRIMARY_USER_EXISTS_RESULT
			case CstiCoreResponse::ePRIMARY_USER_EXISTS_RESULT:
			{
				// Set the "account exists" into the response value.
				const char * pszAccountExists = AttributeValueGet (
					pNode, "AccountExists");
				if (!stiStrICmp(pszAccountExists, VAL_True))
				{
					poResponse->ResponseValueSet (1);
				}
				else
				{
					poResponse->ResponseValueSet (0);
				}

				// Set the "ported" into the response string.
				auto ported = AttributeValueGet (pNode, "Ported");

				if (ported && !stiStrICmp(ported, VAL_True))
				{
					poResponse->ResponseStringSet (0, "Ported");
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case ePRIMARY_USER_EXISTS_RESULT

			//: ePUBLIC_IP_GET_RESULT
			case CstiCoreResponse::ePUBLIC_IP_GET_RESULT:
			{
				// Retrieve the PublicIP string
				const char *pszPublicIP = SubElementValueGet (
					pNode, TAG_PublicIP);

				if (pszPublicIP != nullptr)
				{
					// Add the response
					poResponse->ResponseStringSet (0, pszPublicIP);

					// Create a duplicate string for estiMSG_CB_PUBLIC_IP_RESOLVED
					poResponse->publicIPSet(pszPublicIP);

					hResult = Callback (estiMSG_CB_PUBLIC_IP_RESOLVED, poResponse);

					stiTESTRESULT ();

					// We no longer own this response, so clear it.
					poResponse = nullptr;

				} // end if

				break;
			} // end case ePUBLIC_IP_GET_RESULT

			//: eSERVICE_CONTACTS_GET_RESULT
			case CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT:
			{
				std::string ServiceContacts[MAX_SERVICE_CONTACTS];

				ServiceContactsProcess (pNode, TAG_CoreService, ServiceContacts);

				// Only if we have Service Contacts do we want to update our copies
				if (!ServiceContacts[0].empty () || !ServiceContacts[1].empty ())
				{
					ServiceContactsSet (ServiceContacts);
					poResponse->CoreServiceContactsSet (ServiceContacts);
				}

				if (m_pStateNotifyService)
				{
					ServiceContactsProcess (pNode, TAG_NotificationService, ServiceContacts);

					// Only if we have Service Contacts do we want to update our copies
					if (!ServiceContacts[0].empty () || !ServiceContacts[1].empty ())
					{
						m_pStateNotifyService->ServiceContactsSet (ServiceContacts);
						poResponse->NotifyServiceContactsSet (ServiceContacts);
					}
				}

				if (m_pMessageService)
				{
					ServiceContactsProcess (pNode, TAG_MessageService, ServiceContacts);

					// Only if we have Service Contacts do we want to update our copies
					if (!ServiceContacts[0].empty () || !ServiceContacts[1].empty ())
					{
						m_pMessageService->ServiceContactsSet (ServiceContacts);
						poResponse->MessageServiceContactsSet (ServiceContacts);
					}
				}

				if (m_pConferenceService)
				{
					ServiceContactsProcess (pNode, TAG_ConferenceService, ServiceContacts);

					// Only if we have Service Contacts do we want to update our copies
					if (!ServiceContacts[0].empty () || !ServiceContacts[1].empty ())
					{
						m_pConferenceService->ServiceContactsSet (ServiceContacts);
						poResponse->ConferenceServiceContactsSet (ServiceContacts);
					}
				}

				if (m_remoteLoggerService)
				{
					ServiceContactsProcess (pNode, TAG_RemoteLoggerService, ServiceContacts);

					// Only if we have Service Contacts do we want to update our copies
					if (!ServiceContacts[0].empty () || !ServiceContacts[1].empty ())
					{
						m_remoteLoggerService->ServiceContactsSet (ServiceContacts);
						poResponse->RemoteLoggerServiceContactsSet (ServiceContacts);
					}
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eSERVICE_CONTACTS_GET_RESULT

			//: eSERVICE_CONTACT_RESULT
			case CstiCoreResponse::eSERVICE_CONTACT_RESULT:
			{
				// This message is not passed on to the application.
				break;
			} // end case eSERVICE_CONTACT_RESULT

			//: eTIME_GET_RESULT
			case CstiCoreResponse::eTIME_GET_RESULT:
			{
				// Can we get the returned time?
				const char * szTime = SubElementValueGet (
					pNode, "Time");
				if (szTime != nullptr)
				{
					// Yes! Convert it to local time
					time_t ttTime;

					stiHResult hResult = TimeConvert (szTime, &ttTime);

					if (!stiIS_ERROR (hResult))
					{
						poResponse->timeSet(ttTime);

						// Set the time into the CCI.
						Callback (estiMSG_CB_CURRENT_TIME_SET, poResponse);

						// We no longer own this response, so clear it.
						poResponse = nullptr;
					}
				} // end if

				break;
			} // end case eTIME_GET_RESULT

			//: eUSER_ACCOUNT_INFO_GET_RESULT
			case CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT:
			{
				auto accountInfoResponse = successfulResponseCreate<CstiUserAccountInfo> (unRequestId);

				// Process the user account info
				hResult = UserAccountInfoProcess (&accountInfoResponse->content, pChildNodes);
				stiTESTRESULT ();

				userAccountInfoReceivedSignal.Emit (accountInfoResponse);
				pRequest->callbackRaise(accountInfoResponse, false);

				break;
			} // end case eUSER_ACCOUNT_INFO_GET_RESULT

			//: eUSER_SETTINGS_GET_RESULT
			case CstiCoreResponse::eUSER_SETTINGS_GET_RESULT:
			{
				// Process the retrieved user settings
				hResult = SettingsGetProcess (
					poResponse, pChildNodes, &((CstiCoreRequest *)pRequest)->m_UserSettingList);

				stiTESTRESULT ();

				// Yes! Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eUSER_SETTINGS_GET_RESULT

			//: eUPDATE_VERSION_CHECK_RESULT
			case CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT:
			{
				IXML_Node * pAppVersionOKSubNode = SubElementGet (
					pNode, "AppVersionOK");
				 IXML_Node * pAppLoaderVersionOKSubNode = SubElementGet (
					pNode, "AppLoaderVersionOK");

				if (pAppVersionOKSubNode)
				{
					poResponse->ResponseStringSet (0, "AppVersionOK");
					poResponse->ResponseStringSet (1, "0.0");
				}
				else
				{
					IXML_Node * pSubNode = SubElementGet (pNode, "AppUpdate");
					const char * szURL = SubElementValueGet (pSubNode, "DownloadPath");
					const char * szVersion = SubElementValueGet (pSubNode, "Version");

					if (szURL != nullptr)
					{
						poResponse->ResponseStringSet (0, szURL);
					} // end if

					if (szVersion != nullptr)
					{
						poResponse->ResponseStringSet (1, szVersion);
					} // end if
				}

				if (pAppLoaderVersionOKSubNode)
				{
					poResponse->ResponseStringSet (2, "AppLoaderVersionOK");
					poResponse->ResponseStringSet (3, "0.0");
				}
				else
				{
					IXML_Node * pSubNode = SubElementGet (pNode, "AppLoaderUpdate");
					const char * szURL = SubElementValueGet (pSubNode, "DownloadPath");
					const char * szVersion = SubElementValueGet (pSubNode, "Version");

					if (szURL != nullptr)
					{
						poResponse->ResponseStringSet (2, szURL);
					} // end if

					if (szVersion != nullptr)
					{
						poResponse->ResponseStringSet (3, szVersion);
					} // end if
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eUPDATE_VERSION_CHECK_RESULT
				
			//: eVERSION_GET_RESULT
			case CstiCoreResponse::eVERSION_GET_RESULT:
			{
				const char *pszVersionNumber = AttributeValueGet (pNode, ATT_VersionNumber);
				
				if (pszVersionNumber)
				{
					poResponse->ResponseStringSet (0, pszVersionNumber);
				}
				
				const char *pszAPIVersion = AttributeValueGet (pNode, ATT_APIVersion);
				
				if (pszAPIVersion)
				{
					poResponse->ResponseStringSet (1, pszAPIVersion);
					
					sscanf (pszVersionNumber,
							"%d.%d",
							&m_nAPIMajorVersion,
							&m_nAPIMinorVersion);

                    CstiCoreRequest::m_nAPIMajorVersion = m_nAPIMajorVersion;
                    CstiCoreRequest::m_nAPIMinorVersion = m_nAPIMinorVersion;
				}
				
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
				
				stiTESTRESULT ();
				
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				
				break;
			} // end case eVERSION_GET_RESULT

			//: eEMERGENCY_ADDRESS_GET_RESULT
			case CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT:
			{
				auto addressResponse = successfulResponseCreate<vpe::Address> (unRequestId);
				addressResponse->content.street = subElementStringGet (pNode, TAG_Street);
				addressResponse->content.apartmentNumber = subElementStringGet (pNode, TAG_ApartmentNumber);
				addressResponse->content.city = subElementStringGet (pNode, TAG_City);
				addressResponse->content.state = subElementStringGet (pNode, TAG_State);
				addressResponse->content.zipCode = subElementStringGet (pNode, TAG_Zip);

				emergencyAddressReceivedSignal.Emit (addressResponse);
				pRequest->callbackRaise(addressResponse, false);
				break;
			} // end case eEMERGENCY_ADDRESS_GET_RESULT

			//: eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT
			case CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT:
			{
				const char *pszStatus = SubElementValueGet (
					pNode, "EmergencyProvisionStatus");
				auto addressStatusResponse = successfulResponseCreate<EstiEmergencyAddressProvisionStatus> (unRequestId);
				addressStatusResponse->content = estiPROVISION_STATUS_NOT_SUBMITTED;

				if (pszStatus)
				{
					if (!stiStrICmp("SUBMITTED", pszStatus))
					{
						addressStatusResponse->content = estiPROVISION_STATUS_SUBMITTED;
					}
					else if (!stiStrICmp("VPUSERVERIFY", pszStatus))
					{
						addressStatusResponse->content = estiPROVISION_STATUS_VPUSER_VERIFY;
					}
					else if (!stiStrICmp("STATUSUNKNOWN", pszStatus))
					{
						addressStatusResponse->content = estiPROVISION_STATUS_UNKNOWN;
					}
				}

				emergencyAddressStatusReceivedSignal.Emit (addressStatusResponse);
				pRequest->callbackRaise(addressStatusResponse, false);
				break;
			} // end case eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT

			case CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT: //rgh
			{
				const char *pszInterfaceMode = SubElementValueGet 	(pNode, TAG_InterfaceMode);
				const char *pszFromName = SubElementValueGet 		(pNode, TAG_FromName);
				const char *pszFromDialString = SubElementValueGet 	(pNode, TAG_FromDialString);
				const char *pszFromDialType = SubElementValueGet 	(pNode, TAG_FromDialType);
				if (pszInterfaceMode)
				{
					poResponse->ResponseStringSet (0, pszInterfaceMode);
				}
				if (pszFromName)
				{
					poResponse->ResponseStringSet (1, pszFromName);
				}
				if (pszFromDialString)
				{
					poResponse->ResponseStringSet (2, pszFromDialString);
				}
				if (pszFromDialType)
				{
					poResponse->ResponseStringSet (3, pszFromDialType);
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eEMERGENCY_ADDRESS_GET_RESULT

			case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
			{
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			case CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_RESULT:
			{
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			case CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT:
			{
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			case CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT:
			{
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}
			case CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT:
			{
				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			case CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT:

				//
				// Resend the serviceContactsGet request now that the
				// core server has placed the device into a group
				// to see if we get sent somewhere else.
				//
				QueueServiceContactsGet ();

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;

			case CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT:
			{
				const char *pszValue = ixmlElement_getAttribute ((IXML_Element *)pNode, (DOMString)"ResultNumber");

				// Did we get a value?
				if (pszValue != nullptr)
				{
					auto userLoginCheckResult = static_cast<UserLoginCheckResult>(atoi (pszValue));

					if (m_reauthenticateUserLoginCheckRequestId == unRequestId)
					{
						m_reauthenticateUserLoginCheckRequestId = 0;

						switch (userLoginCheckResult)
						{
							case UserLoginCheckResult::LoggedInOnCurrentDevice:
							{
								if (!clientTokenGet ().empty ())
								{
									StateSet (eAUTHENTICATED);
									break;
								}
							}
							// Fall through
							case UserLoginCheckResult::NotLoggedIn:
							{
								auto request = new CstiCoreRequest ();
								request->clientAuthenticate ("", m_userPhone.c_str(), m_userPin.c_str());
								m_bAuthenticationInProcess = estiTRUE;
								RequestQueue (request, request->PriorityGet ());
								break;
							}
							case UserLoginCheckResult::UserNameOrPasswordInvalid:
							{
								m_userPhone.clear ();
								m_userPin.clear ();
							}
							// Fall Through
							case UserLoginCheckResult::LoggedInOnDifferentDevice:
							{
								auto request = new CstiCoreRequest ();
								request->clientLogout ();
								RequestQueue (request, request->PriorityGet ());
								break;
							}
						}
					}

					poResponse->ResponseValueSet (static_cast<unsigned int>(userLoginCheckResult));
				
					hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
				
					stiTESTRESULT ();
				}
				
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				
				break;
			}

			case CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT:
			{
				// Process the retrieved ring group info list
				hResult = RegistrationInfoGetProcess (poResponse, pChildNodes);

				stiTESTRESULT ();

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end eREGISTRATION_INFO_GET_RESULT

			case CstiCoreResponse::eRING_GROUP_INVITE_INFO_GET_RESULT:
			{
				const char *pszName = SubElementValueGet 	(pNode, TAG_RingGroupName);
				const char *pszLocalNumber = SubElementValueGet 		(pNode, TAG_RingGroupLocalNumber);
				const char *pszTollFreeNumber = SubElementValueGet 	(pNode, TAG_RingGroupTollFreeNumber);
				if (pszName)
				{
					poResponse->ResponseStringSet (0, pszName);
				}
				if (pszLocalNumber)
				{
					poResponse->ResponseStringSet (1, pszLocalNumber);
				}
				if (pszTollFreeNumber)
				{
					poResponse->ResponseStringSet (2, pszTollFreeNumber);
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eRING_GROUP_INVITE_INFO_GET_RESULT

			case CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT:
			{
				// Process the retrieved ring group info list
				hResult = RingGroupInfoGetProcess (poResponse, pChildNodes);

				stiTESTRESULT ();

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eRING_GROUP_INFO_GET_RESULT

			case CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT:
			{
				const char *pszPhoneNumber = SubElementValueGet (pNode, TAG_PhoneNumber);
				const char *pszImageId = SubElementValueGet (pNode, TAG_ImageId);
				const char *pszDate = SubElementValueGet (pNode, TAG_ImageDate);
				const char *pszURL1 = SubElementValueGet (pNode, "ImageDownloadURL1");
				if (pszPhoneNumber)
				{
					poResponse->ResponseStringSet (0, pszPhoneNumber);
				}
				if (pszImageId)
				{
					poResponse->ResponseStringSet (1, pszImageId);
				}
				if (pszDate)
				{
					poResponse->ResponseStringSet (2, pszDate);
				}
				if (pszURL1)
				{
					poResponse->ResponseStringSet (3, pszURL1);
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eIMAGE_DOWNLOAD_URL_GET_RESULT

			case CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT:
			{
				const char *pszImageId = SubElementValueGet (pNode, TAG_ImageId);
				const char *pszURL1 = SubElementValueGet (pNode, "ImageUploadURL1");
				if (pszImageId)
				{
					poResponse->ResponseStringSet (0, pszImageId);
				}
				if (pszURL1)
				{
					poResponse->ResponseStringSet (1, pszURL1);
				}

				// Notify the application of the result
				hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eIMAGE_UPLOAD_URL_GET_RESULT

			case CstiCoreResponse::eAGREEMENT_GET_RESULT:
			{
				const char *pszPublicId = SubElementValueGet (pNode, TAG_AgreementPublicId);
				const char *pszAgreement = SubElementValueGet (pNode, "AgreementText");
				const char *pszAgreementType = SubElementValueGet (pNode, "AgreementType");
				if (pszPublicId)
				{
					poResponse->ResponseStringSet (0, pszPublicId);
				}
				if (pszAgreement)
				{
					poResponse->ResponseStringSet (1, pszAgreement);
				}
				if (pszAgreementType)
				{
					poResponse->ResponseStringSet (2, pszAgreementType);
				}

				// Notify the application of the result
				Callback (estiMSG_CORE_RESPONSE, poResponse);
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}
			case CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT:
			{
				// Process the retrieved agreement status list
				hResult = AgreementStatusGetProcess (poResponse, pChildNodes);

				//TODO: Remove following code once System C is updated
				if (hResult != stiRESULT_SUCCESS)
				{
					// Use the old method until System C is updated
					CstiCoreResponse::SAgreement sAgreement;
					const char *pszPublicId = SubElementValueGet (pNode, TAG_AgreementPublicId);
					const char *pszStatus = SubElementValueGet (pNode, "Status");
					if (pszPublicId)
					{
						poResponse->ResponseStringSet (0, pszPublicId);
						sAgreement.AgreementPublicId = pszPublicId;
					}
					if (pszStatus)
					{
						poResponse->ResponseStringSet (1, pszStatus);
						sAgreement.AgreementStatus = pszStatus;
					}
					sAgreement.AgreementType = "Provider Agreement";
					std::vector <CstiCoreResponse::SAgreement> AgreementList;
					AgreementList.push_back(sAgreement);
					poResponse->AgreementListSet(&AgreementList);
				}

				// Notify the application of the result
				Callback (estiMSG_CORE_RESPONSE, poResponse);
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}
			case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
			{
				const char *pszAddedCount = SubElementValueGet (pNode, TAG_AddedCount);
				const char *pszDuplicateNotAddedCount = SubElementValueGet (pNode, TAG_DuplicateNotAddedCount);
				const char *pszCountExceedingQuota = SubElementValueGet (pNode, TAG_ContactCountExceedingQuota);
				if (pszAddedCount)
				{
					poResponse->ResponseStringSet (0, pszAddedCount);
				}
				if (pszDuplicateNotAddedCount)
				{
					poResponse->ResponseStringSet (1, pszDuplicateNotAddedCount);
				}
				if (pszCountExceedingQuota)
				{
					poResponse->ResponseStringSet (2, pszCountExceedingQuota);
				}
				// Notify the application of the result
				Callback (estiMSG_CORE_RESPONSE, poResponse);
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

            case CstiCoreResponse::eSCREEN_SAVER_LIST_GET_RESULT:
            {
				// Is there a message list in the result?
				if (pChildNodes != nullptr)
				{
					// Yes!
					IXML_Node *pChildNode = nullptr;

					// Loop through the screensaver Lists, and process them.
					pChildNode = ixmlNodeList_item (pChildNodes, 0);
						
					hResult = ScreenSaverListProcess (poResponse, pChildNode);
					stiTESTRESULT();

					// Notify the app
					hResult = Callback (estiMSG_CORE_RESPONSE, poResponse);
					stiTESTRESULT();

					// We no longer own this response, so clear it.
					poResponse = nullptr;
				}

				break;
			}

			case CstiCoreResponse::eAGREEMENT_STATUS_SET_RESULT:
			case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
			case CstiCoreResponse::eEMERGENCY_ADDRESS_DEPROVISION_RESULT:
			case CstiCoreResponse::eIMAGE_DELETE_RESULT:
			case CstiCoreResponse::ePHONE_ONLINE_RESULT:
			case CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT:
			case CstiCoreResponse::eMISSED_CALL_ADD_RESULT:
			case CstiCoreResponse::eRESPONSE_UNKNOWN:
			case CstiCoreResponse::eSIGNMAIL_REGISTER_RESULT:
			case CstiCoreResponse::eTRAINER_VALIDATE_RESULT:
			case CstiCoreResponse::eURI_LIST_SET_RESULT:
			case CstiCoreResponse::eUSER_ACCOUNT_ASSOCIATE_RESULT:
			case CstiCoreResponse::eUSER_SETTINGS_SET_RESULT:
			case CstiCoreResponse::eRING_GROUP_CREATE_RESULT:
			case CstiCoreResponse::eRING_GROUP_CREDENTIALS_UPDATE_RESULT:
			case CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT:
			case CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT:
			case CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT:
			case CstiCoreResponse::eRING_GROUP_INVITE_REJECT_RESULT:
			case CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT:
			case CstiCoreResponse::eRING_GROUP_USER_INVITE_RESULT:
			case CstiCoreResponse::eRING_GROUP_USER_REMOVE_RESULT:
			case CstiCoreResponse::eLOG_CALL_TRANSFER_RESULT:
			case CstiCoreResponse::eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT:
			case CstiCoreResponse::eVRS_AGENT_HISTORY_DIALED_ADD_RESULT:
			default:
			{
				// Notify the application of the result
				Callback (estiMSG_CORE_RESPONSE, poResponse);

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}
		} // end switch
	} // end if
	else
	{
		CstiCoreResponse::ECoreError eErrorNum = NodeErrorGet (pNode, pbDatabaseAvailable);

		if (eErrorNum == CstiCoreResponse::eUNIQUE_ID_UNRECOGNIZED)
		{
			// We don't have a valid phone account, so create one
			// now.
			// Make sure the priority is higher (smaller) than the priority of the
			// request that failed.
			//
			hResult = PhoneAccountCreate (nPriority - 1);
			stiTESTRESULT ();

			*peRepeat = estiRR_IMMEDIATE;
		}
		else if (pRequest && pRequest->RepeatOnErrorGet ())
		{
			*peRepeat = estiRR_DELAYED;


			if (eErrorNum == CstiCoreResponse::eNOT_LOGGED_IN)
			{
				LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);
			}
		}
		else
		{
			auto coreErrorNum = NodeErrorGet (pNode, pbDatabaseAvailable);
			auto coreErrorMessage = attributeStringGet (pNode, "ErrorMsg");

			switch (eResponse)
			{
				case CstiCoreResponse::eRESPONSE_ERROR: // Did we got a systemic error?
				{
					if (coreErrorNum == CstiCoreResponse::eNOT_LOGGED_IN)
					{
						LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);
					}
					// Enable UI to cancel on validation error from Core
					else if (coreErrorNum == CstiCoreResponse::eXML_VALIDATION_ERROR)
					{
						// Create a new Core Response
						poResponse = new CstiCoreResponse (
							pRequest,
							eResponse,
							estiERROR,
							coreErrorNum,
							nullptr);
						stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

						// Notify the application of the result
						Callback (estiMSG_CORE_RESPONSE, poResponse);

						// We no longer own this response, so clear it.
						poResponse = nullptr;
					}

					*pnSystemicError = coreErrorNum;
					break;
				}
				case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT:
				{
					auto response = failureResponseCreate<ClientAuthenticateResult> (unRequestId, coreErrorNum, coreErrorMessage);

					if (coreErrorNum == CstiCoreResponse::eCANNOT_DETERMINE_WHICH_LOGIN)
					{
						hResult = ProcessClientAuthenticateWhichLogin (response->content.ringGroupList, pChildNodes);
						stiTESTRESULT ();
					}
					else
					{
						m_bAuthenticationInProcess = estiFALSE;
						StateEvent (eSE_AUTHENTICATION_FAILED);
					}

					clientAuthenticateSignal.Emit (response);
					pRequest->callbackRaise(response, true);
					break;
				}

				case CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT:
				{
					auto response = failureResponseCreate<CstiUserAccountInfo> (unRequestId, coreErrorNum, coreErrorMessage);
					userAccountInfoReceivedSignal.Emit (response);
					pRequest->callbackRaise(response, true);
					break;
				}
				case CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT:
				{
					auto response = failureResponseCreate<vpe::Address> (unRequestId, coreErrorNum, coreErrorMessage);
					emergencyAddressReceivedSignal.Emit (response);
					pRequest->callbackRaise(response, true);
					break;
				}
				case CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_RESULT:
				{
					auto response = failureResponseCreate<EstiEmergencyAddressProvisionStatus> (unRequestId, coreErrorNum, coreErrorMessage);
					emergencyAddressStatusReceivedSignal.Emit (response);
					pRequest->callbackRaise(response, true);
					break;
				}
				case CstiCoreResponse::eRESPONSE_UNKNOWN:
				case CstiCoreResponse::eAGREEMENT_GET_RESULT:
				case CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT:
				case CstiCoreResponse::eAGREEMENT_STATUS_SET_RESULT:
				case CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT:
				case CstiCoreResponse::eCALL_LIST_GET_RESULT:
				case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
				case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
				case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
				case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
				case CstiCoreResponse::eCALL_LIST_SET_RESULT:
				case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
				case CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT:
				case CstiCoreResponse::eEMERGENCY_ADDRESS_DEPROVISION_RESULT:
				case CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT:
				case CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT:
				case CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT:
				case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
				case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
				case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
				case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
				case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
				case CstiCoreResponse::eIMAGE_DELETE_RESULT:
				case CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT:
				case CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT:
				case CstiCoreResponse::eMISSED_CALL_ADD_RESULT:
				case CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT:
				case CstiCoreResponse::ePHONE_ONLINE_RESULT:
				case CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT:
				case CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT:
				case CstiCoreResponse::ePRIMARY_USER_EXISTS_RESULT:
				case CstiCoreResponse::ePUBLIC_IP_GET_RESULT:
				case CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT:
				case CstiCoreResponse::eRING_GROUP_CREATE_RESULT:
				case CstiCoreResponse::eRING_GROUP_CREDENTIALS_UPDATE_RESULT:
				case CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT:
				case CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT:
				case CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT:
				case CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT:
				case CstiCoreResponse::eRING_GROUP_INVITE_INFO_GET_RESULT:
				case CstiCoreResponse::eRING_GROUP_INVITE_REJECT_RESULT:
				case CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT:
				case CstiCoreResponse::eRING_GROUP_USER_INVITE_RESULT:
				case CstiCoreResponse::eRING_GROUP_USER_REMOVE_RESULT:
				case CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT:
				case CstiCoreResponse::eSERVICE_CONTACT_RESULT:
				case CstiCoreResponse::eSIGNMAIL_REGISTER_RESULT:
				case CstiCoreResponse::eTIME_GET_RESULT:
				case CstiCoreResponse::eTRAINER_VALIDATE_RESULT:
				case CstiCoreResponse::eURI_LIST_SET_RESULT:
				case CstiCoreResponse::eUSER_ACCOUNT_ASSOCIATE_RESULT:
				case CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT:
				case CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT:
				case CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT:
				case CstiCoreResponse::eUSER_SETTINGS_GET_RESULT:
				case CstiCoreResponse::eUSER_SETTINGS_SET_RESULT:
				case CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT:
				case CstiCoreResponse::eVERSION_GET_RESULT:
				case CstiCoreResponse::eLOG_CALL_TRANSFER_RESULT:
				case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
				case CstiCoreResponse::eUSER_DEFAULTS_RESULT:
				case CstiCoreResponse::eSCREEN_SAVER_LIST_GET_RESULT:
				case CstiCoreResponse::eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT:
				case CstiCoreResponse::eVRS_AGENT_HISTORY_DIALED_ADD_RESULT:
				{
					if (coreErrorNum != CstiCoreResponse::eNO_ERROR)
					{
						// Create a new Core Response
						poResponse = new CstiCoreResponse (
							pRequest,
							eResponse,
							estiERROR,
							coreErrorNum,
							coreErrorMessage.c_str());
						stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

						// Yes! Handle the errors
						switch (coreErrorNum)
						{
							case CstiCoreResponse::eNOT_LOGGED_IN: //case 1: // Not Logged in
							{
								LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);

								if (*pbLoggedOutNotified)
								{
									if (pRequest)
									{
										if (pRequest->AuthenticationAttemptsGet () <= 1)
										{
											//
											// Don't remove this request, since we want to
											// try it again after logging in.
											//
											*peRepeat = estiRR_IMMEDIATE;
										}
										else if (!pRequest->RemoveUponCommunicationError ())
										{
											*peRepeat = estiRR_DELAYED;
										}
									}
								}

								//
								// If we are going to remove the request then
								// tell the application otherwise don't because we
								// are going to send it again.
								//
								if (*peRepeat == estiRR_NO)
								{
									// Notify the application of the result
									Callback (estiMSG_CORE_RESPONSE, poResponse);

									// We no longer own this response, so clear it.
									poResponse = nullptr;
								}

								break;
							} // end case 1

							case CstiCoreResponse::eERROR_IMAGE_NOT_FOUND: //case 106: // Image not found
							{
								// In this case, the response may contain the
								// requested phone number, image id, and/or timestamp
								// which may be of use to the application.
								const char* pszPhoneNumber = SubElementValueGet (pNode, TAG_PhoneNumber);
								const char* pszImageId = SubElementValueGet (pNode, TAG_ImageId);
								const char* pszDate = SubElementValueGet (pNode, TAG_ImageDate);

								if (pszPhoneNumber)
								{
									poResponse->ResponseStringSet (0, pszPhoneNumber);
								}
								if (pszImageId)
								{
									poResponse->ResponseStringSet (1, pszImageId);
								}
								if (pszDate)
								{
									poResponse->ResponseStringSet (2, pszDate);
								}

								// Notify the application of the result
								Callback (estiMSG_CORE_RESPONSE, poResponse);

								// We no longer own this response, so clear it.
								poResponse = nullptr;
								break;
							} // end case 106

							default:
							{
								// Notify the application of the result
								Callback (estiMSG_CORE_RESPONSE, poResponse);

								// We no longer own this response, so clear it.
								poResponse = nullptr;
								break;

							} // end default
						} // end switch
					} // end if
					else
					{
						// Create a new Core Response
						poResponse = new CstiCoreResponse (
							pRequest,
							eResponse,
							estiERROR,
							coreErrorNum,
							ElementValueGet (pNode));
						stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);

						// Notify the application of the result
						Callback (estiMSG_CORE_RESPONSE, poResponse);

						// We no longer own this response, so clear it.
						poResponse = nullptr;
					} // end else
					break;
				}
			}
		}
	} // end else

STI_BAIL:

	// Do we still have a CoreResponse object hanging around?
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
} // end CstiCoreService::ResponseNodeProcess


/**
 * \brief Processes a CallList from the response
 * \param poResponse Pointer to the core response object
 * \param pNode Pointer to the Call list IXML node
 * \return stiHResult - success or failure result
 */
stiHResult CstiCoreService::CallListProcess (
	CstiCoreResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiCoreService::CallListProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	IXML_NodeList * pNodeList = nullptr;
	IXML_Node * pTempNode = nullptr;
	CstiCallList *poCallList = nullptr;
	CstiContactList *poContactList = nullptr;

	// Get the count of items in the list
	const char * szCount = AttributeValueGet (pNode, ATT_Count);
	unsigned int unCount = 0;
	if (szCount != nullptr)
	{
		sscanf (szCount, "%ud", &unCount);
	} // end if

	// We need to figure out what type of list this is FIRST because
	// if it's a "Contact" list we store the data differently.
	const char * szListType = AttributeValueGet (pNode, ATT_Type);
	stiTESTCOND (szListType != nullptr && *szListType != 0, stiRESULT_ERROR);

	if (0 == strcmp(szListType, VAL_Contact))
	{
		poContactList = new CstiContactList();

		// Did we get one?
		if (poContactList != nullptr)
		{
			//
			// Set count.
			// This is for a list count get request
			// There are actually no items to add to this list
			// but the count is stored in a list structure
			// this way to pass to the application.
			//
			poContactList->CountSet (unCount);

			// Set the base index and count
			const char * szBaseIndex = AttributeValueGet (pNode, ATT_BaseIndex);

			unsigned int unBaseIndex = 0;

			if (szBaseIndex != nullptr)
			{
				sscanf (szBaseIndex, "%ud", &unBaseIndex);
			} // end if

			poContactList->BaseIndexSet (unBaseIndex);

			// Get all elements named "CallListItem"
			pNodeList = ixmlElement_getElementsByTagName (
				(IXML_Element *)pNode, (DOMString)TAG_CallListItem);

			// Did we get the list of items?
			if (pNodeList != nullptr)
			{
				// Get the first item in the list
				int nListItem = 0;
				pTempNode = ixmlNodeList_item (pNodeList, nListItem);

				// While there are still nodes...
				while (pTempNode)
				{
					// Get the name
					const char * pszName = SubElementValueGet (pTempNode, VAL_Name);
					if (nullptr == pszName)
					{
						pszName = "";
					} // end if

					// Get the company name
					const char * pszCompany = SubElementValueGet (pTempNode, VAL_Company);
					if (nullptr == pszCompany)
					{
						pszCompany = "";
					} // end if

					// get the ItemId
					const char * pszItemId = SubElementValueGet (pTempNode, TAG_ItemId);
					const char * pszItemPublicId = SubElementValueGet (pTempNode, TAG_PublicId);
					const char *pszValue = nullptr;

					// get the RingTone
					int nRingTone = 0;
					pszValue = SubElementValueGet (
						pTempNode, TAG_RingTone);
					if (nullptr != pszValue)
					{
						nRingTone = atoi(pszValue);
					}

                    // get the RingColor
                    int nRingColor = -1;
                    pszValue = SubElementValueGet (
                        pTempNode, TAG_RingColor);
                    if (nullptr != pszValue)
                    {
                        nRingColor = atoi(pszValue);
                    }

                    // get the relay language
					const char *pszLang = SubElementValueGet (pTempNode, TAG_RelayLanguage);

#ifdef stiCONTACT_PHOTOS
					const char * pszImageId = SubElementValueGet (
						pTempNode, TAG_ImageId);

					const char * pszImageTimestamp = SubElementValueGet (
						pTempNode, TAG_ImageDate);

					if (nullptr == pszImageId)
					{
						pszImageId = "";
					} // end if
					
					if (nullptr == pszImageTimestamp)
					{
						pszImageTimestamp = "";
					} // end if
#endif

					auto pContactListItem = std::make_shared<CstiContactListItem> ();

					pContactListItem->NameSet (pszName);
					pContactListItem->CompanyNameSet (pszCompany);
					CstiItemId Id(pszItemId);
					pContactListItem->ItemIdSet (Id);
					Id.ItemIdSet(pszItemPublicId);
					pContactListItem->ItemPublicIdSet(Id);
                    pContactListItem->RingToneSet (nRingTone);
                    pContactListItem->RingColorSet (nRingColor);
                    pContactListItem->RelayLanguageSet (pszLang);
#ifdef stiCONTACT_PHOTOS
					pContactListItem->ImageIdSet(pszImageId);
					pContactListItem->ImageTimestampSet(pszImageTimestamp);
#endif

					// There should be one element named "ContactPhoneNumberList"
					IXML_NodeList *pNumberListList = nullptr;
					pNumberListList = ixmlElement_getElementsByTagName (
						(IXML_Element *)pTempNode, (DOMString)TAG_ContactPhoneNumberList);

					// Did we get the list of items?
					if (pNumberListList != nullptr)
					{
						// Get the first item in the list
						IXML_Node *pNumberListNode = ixmlNodeList_item (pNumberListList, 0);

						int nNumberItem = 0;
						IXML_NodeList *pNumberList = ixmlElement_getElementsByTagName (
							(IXML_Element *)pNumberListNode, (DOMString)TAG_ContactPhoneNumber);

						if (pNumberList != nullptr)
						{
							IXML_Node *pNumberNode = ixmlNodeList_item(pNumberList, nNumberItem);

							// While there are still nodes...
							while (pNumberNode)
							{
								pContactListItem->PhoneNumberAdd();

								const char * szId = SubElementValueGet (
									pNumberNode, SNRQ_Id);
								if (nullptr == szId)
								{
									szId = "";
								} // end if

								Id.ItemIdSet(szId);
								pContactListItem->PhoneNumberIdSet(nNumberItem, Id);

								const char * szPublicId = SubElementValueGet (
									pNumberNode, TAG_PublicId);
								if (nullptr == szPublicId)
								{
									szPublicId = "";
								} // end if

								Id.ItemIdSet(szPublicId);
								pContactListItem->PhoneNumberPublicIdSet(nNumberItem, Id);

								int nDialType = 0;
								const char * szDialType = SubElementValueGet (
									pNumberNode, VAL_DialType);
								if (szDialType != nullptr)
								{
									nDialType = atoi (szDialType);
								} // end if

								pContactListItem->DialMethodSet(nNumberItem, (EstiDialMethod)nDialType);

								const char * szDialString = SubElementValueGet (
									pNumberNode, VAL_DialString);
								if (nullptr == szDialString)
								{
									szDialString = "";
								} // end if

								pContactListItem->DialStringSet(nNumberItem, szDialString);

								const char * szPhoneNumberType = SubElementValueGet (
									pNumberNode, TAG_PhoneNumberType);
								if (nullptr == szPhoneNumberType)
								{
									szPhoneNumberType = VAL_Home;
								}

								CstiContactListItem::EPhoneNumberType type = !strcmp(szPhoneNumberType, VAL_Work)?CstiContactListItem::ePHONE_WORK:
									!strcmp(szPhoneNumberType, VAL_Cell)?CstiContactListItem::ePHONE_CELL:CstiContactListItem::ePHONE_HOME;

								pContactListItem->PhoneNumberTypeSet(nNumberItem, type);

								// Get the next CoreResponse node
								nNumberItem++;
								pNumberNode = ixmlNodeList_item (pNumberList, nNumberItem);
							}

							// Free the node list
							ixmlNodeList_free (pNumberList);
						}

						// Free the node list
						ixmlNodeList_free (pNumberListList);
					}

					poContactList->ItemAdd (pContactListItem);

					// Get the next CoreResponse node
					nListItem++;
					pTempNode = ixmlNodeList_item (pNodeList, nListItem);
				} // end while

				// Free the node list
				ixmlNodeList_free (pNodeList);
			} // end if

			// Add the call list to the response
			poResponse->ContactListSet (poContactList);
		} // end if
		else
		{
			// No! Log an error
			stiTHROW (stiRESULT_ERROR);
		} // end else
	}
	else
	{
		// Create a Call List Object
		poCallList = new CstiCallList ();

		// Did we get one?
		if (poCallList != nullptr)
		{
			//
			// Set count.
			// This is for a list count get request
			// There are actually no items to add to this list
			// but the count is stored in a list structure
			// this way to pass to the application.
			//
			poCallList->CountSet (unCount);

			// Yes! Figure out what type of list this is...
			const char * szListType = AttributeValueGet (pNode, ATT_Type);
			stiTESTCOND (szListType != nullptr && *szListType != 0, stiRESULT_ERROR);

			if (0 == strcmp (szListType, VAL_Dialed))
			{
				poCallList->TypeSet (CstiCallList::eDIALED);
			} // end else if
			else if (0 == strcmp (szListType, VAL_Answered))
			{
				poCallList->TypeSet (CstiCallList::eANSWERED);
			} // end else if
			else if (0 == strcmp (szListType, VAL_Missed))
			{
				poCallList->TypeSet (CstiCallList::eMISSED);
			} // end else if
			else if (0 == strcmp (szListType, VAL_Blocked))
			{
				poCallList->TypeSet (CstiCallList::eBLOCKED);
			} // end else if
			else
			{
				poCallList->TypeSet (CstiCallList::eTYPE_UNKNOWN);
			} // end else

			// Set the base index and count
			const char * szBaseIndex = AttributeValueGet (pNode, ATT_BaseIndex);

			unsigned int unBaseIndex = 0;

			if (szBaseIndex != nullptr)
			{
				sscanf (szBaseIndex, "%ud", &unBaseIndex);
			} // end if

			poCallList->BaseIndexSet (unBaseIndex);

			// Get all elements named "CallListItem"
			pNodeList = ixmlElement_getElementsByTagName (
				(IXML_Element *)pNode, (DOMString)TAG_CallListItem);

			// Did we get the list of items?
			if (pNodeList != nullptr)
			{
				// Get the first item in the list
				int nListItem = 0;
				pTempNode = ixmlNodeList_item (pNodeList, nListItem);

				// While there are still nodes...
				while (pTempNode)
				{
					// Yes! Retrieve the dial type value from this CallListItem
					int nDialType = 0;
					const char * szDialType = SubElementValueGet (
						pTempNode, VAL_DialType);
					if (szDialType != nullptr)
					{
						nDialType = atoi (szDialType);
					} // end if

					// Convert the the time
					time_t ttTime = 0;
					const char * szTime = SubElementValueGet (pTempNode, VAL_Time);
					if (nullptr != szTime)
					{
						hResult = TimeConvert (szTime, &ttTime);
						stiTESTRESULT ();
					} // end if

					// Get the name and dial string
					const char * szName = SubElementValueGet (pTempNode, VAL_Name);
					if (nullptr == szName)
					{
						szName = "";
					} // end if

					const char * szDialString = SubElementValueGet (
						pTempNode, VAL_DialString);
					if (nullptr == szDialString)
					{
						szDialString = "";
					} // end if

					// get the ItemId
					const char * szItemId = SubElementValueGet (
						pTempNode, TAG_ItemId);

					// Get and convert the InAddressBook flag
					EstiBool bInAddressBook = estiFALSE;
					const char * szInAddressBook =
						SubElementValueGet (pTempNode, VAL_InAddressBook);
					if (nullptr != szInAddressBook)
					{
						if (0 == stiStrICmp (szInAddressBook, "True"))
						{
							bInAddressBook = estiTRUE;
						} // end if
					} // end if

                    // get the RingTone
                    int nRingTone = 0;
                    const char * szRingTone = SubElementValueGet (
                        pTempNode, TAG_RingTone);
                    if (nullptr != szRingTone)
                    {
                        nRingTone = atoi(szRingTone);
                    }

					std::string vrsCallId;
					std::deque<AgentHistoryItem> agentHistory;

					auto vrsAgentHistoryElement = SubElementGet (pTempNode, TAG_VrsAgentHistory);
					if (vrsAgentHistoryElement != nullptr)
					{
						vrsCallId = AttributeValueGet (vrsAgentHistoryElement, ATT_VrsCallId);
						auto agentElement = SubElementGet (vrsAgentHistoryElement, TAG_Agent);
						while (agentElement != nullptr)
						{
							auto id = AttributeValueGet (agentElement, TAG_AgentId);
							auto pszTime = AttributeValueGet (agentElement, ATT_CallJoinUtc);
							time_t callJoinUtc;
							hResult = TimeConvert (pszTime, &callJoinUtc);
							stiTESTRESULT ();

							agentHistory.emplace_back (id, callJoinUtc);
							agentElement = ixmlNode_getNextSibling (agentElement);
						}
					}

                    CstiCallListItemSharedPtr pCallListItem = std::make_shared<CstiCallListItem> ();

					pCallListItem->DialMethodSet ((EstiDialMethod)nDialType);
					pCallListItem->NameSet (szName);
					pCallListItem->DialStringSet (szDialString);
					pCallListItem->CallTimeSet (ttTime);
					pCallListItem->ItemIdSet (szItemId);
					pCallListItem->InSpeedDialSet (bInAddressBook);
                    pCallListItem->RingToneSet (nRingTone);
					pCallListItem->vrsCallIdSet (vrsCallId);
					pCallListItem->m_agentHistory = agentHistory;

					poCallList->ItemAdd (pCallListItem);

					// Get the next CoreResponse node
					nListItem++;
					pTempNode = ixmlNodeList_item (pNodeList, nListItem);
				} // end while

				// Free the node list
				ixmlNodeList_free (pNodeList);
			} // end if

			// Add the call list to the response
			poResponse->CallListSet (poCallList);
		} // end if
		else
		{
			// No! Log an error
			stiTHROW (stiRESULT_ERROR);
		} // end else
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (poContactList)
		{
			delete poContactList;
			poContactList = nullptr;
		}

		if (poCallList)
		{
			delete poCallList;
			poCallList = nullptr;
		}
	}

	return (hResult);
} // end CstiCoreService::CallListProcess

#ifdef stiFAVORITES
/**
 * \brief Processes a Favorite List from the response
 * \param poResponse Pointer to the core response object
 * \param pNode Pointer to the Favorite list IXML node
 * \return stiHResult - success or failure result
 */
stiHResult CstiCoreService::FavoriteListProcess (
	CstiCoreResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiCoreService::FavoriteListProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	IXML_NodeList * pNodeList = nullptr;
	IXML_Node * pTempNode = nullptr;
	SstiFavoriteList *poFavoriteList = nullptr;

	// Create a Favorite List Object
	poFavoriteList = new SstiFavoriteList ();

	// Did we get one?
	if (poFavoriteList != nullptr)
	{
		// Get the Favorite Limit and set it on the list
		auto favoriteLimitString = AttributeValueGet (pNode, ATT_FavoriteLimit);
		int favoriteLimit = 0;
		if (favoriteLimitString != nullptr)
		{
			sscanf (favoriteLimitString, "%d", &favoriteLimit);
		} // end if
		poFavoriteList->maxCount = favoriteLimit;

		// Get all elements named "FavoriteItem"
		pNodeList = ixmlElement_getElementsByTagName (
			(IXML_Element *)pNode, (DOMString)TAG_FavoriteItem);

		// Did we get the list of items?
		if (pNodeList != nullptr)
		{
			// Get the first item in the list
			int nListItem = 0;
			pTempNode = ixmlNodeList_item (pNodeList, nListItem);

			// While there are still nodes...
			while (pTempNode)
			{
				// Yes! Retrieve the phone number ID from this Favorite Item
				const char * szId = SubElementValueGet (
					pTempNode, TAG_PublicId);
				CstiItemId Id(szId);

				// Get the favorite type
				bool bIsContact = false;
				const char * szType = SubElementValueGet (pTempNode, TAG_FavoriteType);
				if (nullptr != szType)
				{
					if (0 == stiStrICmp (szType, "Contact"))
					{
						bIsContact = true;
					} // end if
				} // end if

				// Get the position
				int nPos = 0;
				const char * szPos = SubElementValueGet (
					pTempNode, TAG_Position);
				if (nullptr != szPos)
				{
					nPos = atoi(szPos);
				}

				CstiFavorite favorite;

				favorite.PhoneNumberIdSet (Id);
				favorite.IsContactSet (bIsContact);
				favorite.PositionSet (nPos);

				poFavoriteList->favorites.push_back (favorite);

				// Get the next CoreResponse node
				nListItem++;
				pTempNode = ixmlNodeList_item (pNodeList, nListItem);
			} // end while

			// Free the node list
			ixmlNodeList_free (pNodeList);
		} // end if

		// Add the favorite list to the response
		poResponse->FavoriteListSet (poFavoriteList);
	} // end if
	else
	{
		// No! Log an error
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (poFavoriteList)
		{
			delete poFavoriteList;
			poFavoriteList = nullptr;
		}
	}

	return (hResult);
} // end CstiCoreService::FavoriteListProcess
#endif

/**
 * \brief Processes a RealNumberList from the response
 * 
 * Creates a CstiPhoneNumberList to hold the returned phone number lists.
 * \param poResponse Pointer to the core response
 * \param pNode The IXML node to process.
 * \return stiHResult - success or failure result
 */
stiHResult CstiCoreService::PhoneNumberListProcess (
	CstiCoreResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiCoreService::PhoneNumberListProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	IXML_NodeList * pNodeList = nullptr;
	IXML_Node * pTempNode = nullptr;

	// Get the count of items in the list
	const char * szCount = AttributeValueGet (pNode, ATT_Count);
	unsigned int unCount = 0;

	if (szCount != nullptr)
	{
		sscanf (szCount, "%ud", &unCount);
	} // end if

	// Create a Call List Object
	auto pPhoneNumberList = new CstiPhoneNumberList ();

	// Did we get one?
	if (pPhoneNumberList != nullptr)
	{
		//
		// Set count.
		// This is for a list count get request
		// There are actually no items to add to this list
		// but the count is stored in a list structure
		// this way to pass to the application.
		//
		pPhoneNumberList->CountSet (unCount);

		// Yes! Figure out what type of list this is...
		const char * szListType = AttributeValueGet (pNode, ATT_Type);

		if (szListType == nullptr || *szListType == 0)
		{
			szListType = VAL_TollFree;
		}

		if (0 == strcmp (szListType, VAL_Local))
		{
			pPhoneNumberList->TypeSet (CstiPhoneNumberList::eLOCAL);
		} // end else if

		else // if (0 == strcmp (szListType, VAL_TollFree))
		{
			pPhoneNumberList->TypeSet (CstiPhoneNumberList::eTOLL_FREE);

		} // end if

		// Get all elements named "RealNumberItem"
		pNodeList = ixmlElement_getElementsByTagName (
			(IXML_Element *)pNode, (DOMString)TAG_RealNumberItem);

		// Did we get the list of items?
		if (pNodeList != nullptr)
		{
			// Get the first item in the list
			int nListItem = 0;
			pTempNode = ixmlNodeList_item (pNodeList, nListItem);

			// While there are still nodes...
			while (pTempNode)
			{
				// Yes! Retrieve the DialString
				const char * szDialString = SubElementValueGet (
					pTempNode, VAL_DialString);

				if (nullptr == szDialString)
				{
					szDialString = "";
				} // end if

				auto phoneNumber = std::make_shared<CstiPhoneNumberListItem> (szDialString);

				pPhoneNumberList->ItemAdd (phoneNumber);

					// Get the next CoreResponse node
					nListItem++;
					pTempNode = ixmlNodeList_item (pNodeList, nListItem);

			} // end while

			// Free the node list
			ixmlNodeList_free (pNodeList);
		} // end if

		// Add the call list to the response
		poResponse->PhoneNumberListSet (pPhoneNumberList);

	} // end if
	else
	{
		// No! Log an error
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	return (hResult);
}  // end CstiCoreService::PhoneNumberListProcess


/**
 * \brief Processes a directory resolve response
 * \param poResponse Pointer to the core response
 * \param pNode The IXML node to process.
 */
stiHResult CstiCoreService::DirectoryResolveProcess (
	CstiCoreResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiCoreService::DirectoryResolveProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Get all elements named "DirectoryItem" (should be only 1)
	IXML_NodeList * pDirectoryItemNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pNode, (DOMString)"DirectoryItem");

	// Did we get the list of items?
	if (pDirectoryItemNodeList != nullptr)
	{
		// Yes! Get the first item in the list
		IXML_Node * pDirectoryItemNode = ixmlNodeList_item (
			pDirectoryItemNodeList, 0);

		// Did we get it?
		if (pDirectoryItemNode != nullptr)
		{
			// Yes! Create a Directory Resolve Result
			auto  poDirResult =
				new CstiDirectoryResolveResult;
			const char* szTemp = nullptr;

			if (nullptr != poDirResult)
			{
				// Store the Directory Resolve Result information
				poDirResult->NameSet (
					SubElementValueGet (pDirectoryItemNode, "Name"));

				poDirResult->DialStringSet (
					SubElementValueGet (pDirectoryItemNode, "DialString"));

#if 0
				szTemp = SubElementValueGet (pDirectoryItemNode, "UserOnlineFlag");
				if (NULL != szTemp
				 && 0 == stiStrICmp (szTemp, "True"))
				{
					poDirResult->UserOnlineSet (estiTRUE);
				} // end if
#endif
#if 0
				szTemp = SubElementValueGet (pDirectoryItemNode, "PhoneOnlineFlag");
				if (NULL != szTemp
				 && 0 == stiStrICmp (szTemp, "True"))
				{
					poDirResult->PhoneOnlineSet (estiTRUE);
				} // end if
#endif
				szTemp =  SubElementValueGet (pDirectoryItemNode, "CallerBlocked");
				if (nullptr != szTemp
				 && 0 == stiStrICmp (szTemp, "True"))
				{
					poDirResult->BlockedSet (estiTRUE);
				} // end if

				szTemp =  SubElementValueGet (pDirectoryItemNode, "AnonymousCallerBlocked");
				if (nullptr != szTemp
				 && 0 == stiStrICmp (szTemp, "True"))
				{
					poDirResult->AnonymousBlockedSet (estiTRUE);
				} // end if

				poDirResult->NewNumberSet(
					SubElementValueGet(pDirectoryItemNode, "NewNumber"));

#if 0
				poDirResult->BlockedMsgSet (
					SubElementValueGet (pDirectoryItemNode, "BlockedMsg"));
#endif
				// Get the "FromName" value
				poDirResult->FromNameSet(
					SubElementValueGet(pDirectoryItemNode, "FromName"));

				IXML_Node *pUriList = SubElementGet(pDirectoryItemNode, "URIList");

				if (pUriList)
				{
					IXML_Node *pUri = SubElementGet (pUriList, "URI");
					
					std::list<SstiURIInfo> UriList;

					while (pUri)
					{
						SstiURIInfo URIInfo;
						
						const char *pszNetwork = ixmlElement_getAttribute (
							(IXML_Element *)pUri,
							(DOMString)"Network");
						
						if (strcmp (pszNetwork, "Sorenson") == 0)
						{
							URIInfo.eNetwork = SstiURIInfo::estiSorenson;
						}
						else
						{
							URIInfo.eNetwork = SstiURIInfo::estiITRS;
						}
						
						URIInfo.URI = ElementValueGet (pUri);

						UriList.push_back (URIInfo);

						pUri = NextElementGet (pUri);
					}

					if (!UriList.empty())
					{
						poDirResult->UriListSet (UriList);
					}
				}

				// Get the "Source" value
				poDirResult->SourceSet(
					SubElementValueGet(pDirectoryItemNode, "Source"));

#ifdef stiSIGNMAIL_ENABLED
				// Get the Video Msg Data (Point to Point video messaging)
				IXML_Node *pVideoMsgDataNode = SubElementGet (pDirectoryItemNode,
															  "VideoMsgData");
				if (pVideoMsgDataNode)
				{
					szTemp =  SubElementValueGet (pVideoMsgDataNode, "GreetingPreference");
					if (nullptr != szTemp)
					{
						auto  eGreetingPreference = (eSignMailGreetingType)atoi (szTemp);
						poDirResult->GreetingPreferenceSet (eGreetingPreference);
					}

					poDirResult->GreetingURLSet (
						SubElementValueGet (pVideoMsgDataNode, "GreetingURL"));
					poDirResult->GreetingTextSet (
						SubElementValueGet (pVideoMsgDataNode, "GreetingText"));
					szTemp =  SubElementValueGet (pVideoMsgDataNode, "NumberRings");
					if (nullptr != szTemp)
					{
						poDirResult->MaxNumberOfRingsSet (atoi (szTemp));
					}

					szTemp =  SubElementValueGet (pVideoMsgDataNode, "MaxRecordSeconds");
					if (nullptr != szTemp)
					{
						poDirResult->MaxRecordSecondsSet (atoi (szTemp));
					}

					szTemp =  SubElementValueGet (pVideoMsgDataNode, "CountdownEmbedded");
					if (nullptr != szTemp)
					{
						poDirResult->CountdownEmbeddedSet (0 == stiStrICmp (szTemp, "True") ? estiTRUE : estiFALSE);
					}

					szTemp =  SubElementValueGet (pVideoMsgDataNode, "VideoMessageSupported");
					if (nullptr != szTemp)
					{
						if (0 == stiStrICmp (szTemp, "True"))
						{
							poDirResult->P2PMessageSupportedSet (estiTRUE);
						}
					}

					szTemp =  SubElementValueGet (pVideoMsgDataNode, "DownloadSpeed");
					if (nullptr != szTemp)
					{
						poDirResult->RemoteDownloadSpeedSet (atoi (szTemp));
					}
				}
#endif
				poResponse->DirectoryResolveResultSet (poDirResult);
			} // end if
		} // end if

		// Free the node list
		ixmlNodeList_free (pDirectoryItemNodeList);

	} // end if
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		poResponse->ResponseResultSet (estiERROR);
	}

	return (hResult);
} // end CstiCoreService::DirectoryResolveProcess


/**
 * \brief Processes a service contacts get response
 * \param pNode The IXML node to process.
 * \param pszServiceName The service name
 * \param ServiceContacts Array holding the retrieved service contacts
 */
void CstiCoreService::ServiceContactsProcess (
	IXML_Node *pNode,
	const char *pszServiceName,
	std::string ServiceContacts[])
{
	stiLOG_ENTRY_NAME (CstiCoreService::ServiceContactsProcess);

	// First initialize the ServiceContacts array to empty in case we don't get any new info
	ServiceContacts[0].erase ();
	ServiceContacts[1].erase ();
	
	// Get all elements named "CoreService" (should be only 1)
	IXML_NodeList *pServiceNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pNode, (DOMString)pszServiceName);

	// Did we get the list of items?
	if (pServiceNodeList != nullptr)
	{
		// We know the ServiceNodeList is not empty, so get the first/only item in the list
		IXML_Node * pServiceNode = ixmlNodeList_item (
			pServiceNodeList, 0);

		// Now get all core service nodes
		IXML_NodeList * pServiceContactNodeList =
			ixmlElement_getElementsByTagName (
			(IXML_Element *)pServiceNode,
			(DOMString)TAG_ServiceContact);

		// Did we get the list of items?
		if (pServiceContactNodeList != nullptr)
		{
			// Yes! Loop through to get both items in the list.
			for (int i = 0; i < MAX_SERVICE_CONTACTS; i++)
			{
				ServiceContacts[i].clear ();

				IXML_Node * pTempNode = ixmlNodeList_item (
					pServiceContactNodeList, i);
				if (pTempNode != nullptr)
				{
					// Get the Priority
					const char *pszPriority = SubElementValueGet (
						pTempNode, TAG_ServicePriority);

					if (pszPriority != nullptr)
					{
						int nPriority = atoi (pszPriority);

						if (nPriority >= 0 && nPriority <= MAX_SERVICE_CONTACT_PRIORITY)
						{
							// Get the URL
							const char *pszUrl = SubElementValueGet (
								pTempNode, TAG_ServiceUrl);

							if (pszUrl != nullptr)
							{
								ServiceContacts[nPriority] = pszUrl;
							}
						} // end if
					} // end if
				} // end if
			}

			// Free the node list
			ixmlNodeList_free (pServiceContactNodeList);
		} // end if

		// Free the node list
		ixmlNodeList_free (pServiceNodeList);
	}
} // end CstiCoreService::ServiceContactsProcess


/**
 * \brief Helper function to parse newNumberAttributes.
 * \param xmlNode XML Node that contains the attributes.
 * \param startDate a time_t that will return the startDate of 
 *  	  the new number
 * \param validDays a CstiString that will return the number of 
 *  	  days the new badge should be displayed.
 */
void newNumberAttributesParse(
	IXML_Node *xmlNode,
	time_t *startDate,
	std::string *validDays)
{
	const char *date = AttributeValueGet (xmlNode, ATT_StateChangeUTC); 
	if (date != nullptr && *date)
	{
		TimeConvert(date, startDate);
	}

	*validDays = attributeStringGet (xmlNode, ATT_NewTagValidDays);
}

/**
 * \brief Processes a UserAccountInfoGetResult response
 * \param poResponse Pointer to the core response
 * \param pChildNodes The IXML nodes to process.
 */
stiHResult CstiCoreService::UserAccountInfoProcess (
	CstiUserAccountInfo* poAccountInfo,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::UserAccountInfoProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Yes! Are there any user info items in the result?
	if (pChildNodes != nullptr)
	{
		// Yes!
		int y = 0;
		IXML_Node * pChildNode = nullptr;

		// Could we allocate it?
		if (poAccountInfo != nullptr)
		{
			// Yes!
			// Loop through the items, and process them.
			pChildNode = ixmlNodeList_item (pChildNodes, y++);
			while (pChildNode)
			{
				// Process this node
				const char * szNodeName =
					ixmlNode_getNodeName (pChildNode);

				auto elementStringValue = elementStringValueGet (pChildNode);

				if (0 == strcmp (szNodeName, TAG_Name))
				{
					poAccountInfo->m_Name = elementStringValue;
				} // end if
				else if (0 == strcmp (szNodeName, TAG_PhoneNumber))
				{
					poAccountInfo->m_PhoneNumber = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_TollFreeNumber))
				{
					poAccountInfo->m_TollFreeNumber = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_LocalNumber))
				{
					poAccountInfo->m_LocalNumber = elementStringValue;

					// If we have attributes load them.
					if (ixmlNode_hasAttributes (pChildNode))
					{
						newNumberAttributesParse(pChildNode,
												 &poAccountInfo->m_localNumberNewDate,
												 &poAccountInfo->m_newTagValidDays);
					}
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_AssociatedPhone))
				{
					poAccountInfo->m_AssociatedPhone = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CurrentPhone))
				{
					poAccountInfo->m_CurrentPhone = elementStringValue;
				} // end else if
                else if (0 == strcmp (szNodeName, TAG_Address1))
                {
                    poAccountInfo->m_Address1 = elementStringValue;
                } // end else if
                else if (0 == strcmp (szNodeName, TAG_Address2))
                {
                    poAccountInfo->m_Address2 = elementStringValue;
                } // end else if
                else if (0 == strcmp (szNodeName, TAG_City))
                {
                    poAccountInfo->m_City = elementStringValue;
                } // end else if
                else if (0 == strcmp (szNodeName, TAG_State))
                {
                    poAccountInfo->m_State = elementStringValue;
                } // end else if
                else if (0 == strcmp (szNodeName, TAG_ZipCode))
                {
                    poAccountInfo->m_ZipCode = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_Email))
				{
					poAccountInfo->m_Email = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_EmailMain))
				{
					poAccountInfo->m_EmailMain = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_EmailPager))
				{
					poAccountInfo->m_EmailPager = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_SignMailEnabled))
				{
					poAccountInfo->m_SignMailEnabled = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_MustCallCir))
				{
					poAccountInfo->m_MustCallCIR = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLAnsweredQuota))
				{
					poAccountInfo->m_CLAnsweredQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLDialedQuota))
				{
					poAccountInfo->m_CLDialedQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLMissedQuota))
				{
					poAccountInfo->m_CLMissedQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLRedialQuota))
				{
					poAccountInfo->m_CLRedialQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLBlockedQuota))
				{
					poAccountInfo->m_CLBlockedQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLContactQuota))
				{
					poAccountInfo->m_CLContactQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_CLFavoritesQuota))
				{
					poAccountInfo->m_CLFavoritesQuota = elementStringValue;
				} // end else if
				else if (0 == strcmp (szNodeName, TAG_RingGroupLocalNumber))
				{
					poAccountInfo->m_RingGroupLocalNumber = elementStringValue;
					if (ixmlNode_hasAttributes (pChildNode))
					{
						newNumberAttributesParse(pChildNode,
												 &poAccountInfo->m_ringGroupLocalNumberNewDate,
												 &poAccountInfo->m_ringGroupNewTagValidDays);
					}
				} // end else if

				else if (0 == strcmp (szNodeName, TAG_RingGroupTollFreeNumber))
				{
					poAccountInfo->m_RingGroupTollFreeNumber = elementStringValue;
				} // end else if

				else if (0 == strcmp (szNodeName, TAG_RingGroupName))
				{
					poAccountInfo->m_RingGroupName = elementStringValue;
				} // end else if

				else if (0 == strcmp (szNodeName, TAG_ImageId))
				{
					poAccountInfo->m_ImageId = elementStringValue;
				} // end else if

				else if (0 == strcmp (szNodeName, TAG_ImageDate))
				{
					poAccountInfo->m_ImageDate = elementStringValue;
				} // end else if
				
				else if (0 == strcmp (szNodeName, TAG_UserRegistrationDataRequired))
				{
					poAccountInfo->m_UserRegistrationDataRequired = elementStringValue;
				} // end else if

				else if (0 == strcmp (szNodeName, TAG_PhoneUserID))
				{
					poAccountInfo->m_userId = elementStringValue;
				}

				else if (0 == strcmp (szNodeName, TAG_RingGroupUserID))
				{
					poAccountInfo->m_groupUserId = elementStringValue;
				}

				// Move to the next child node
				pChildNode = ixmlNodeList_item (pChildNodes, y++);
			} // end while
		} // end if
		else
		{
			// No! Log an error
			stiTHROW (stiRESULT_ERROR);
		} // end else
	} // end if
	else
	{
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	return (hResult);
} // end CstiCoreService::UserAccountInfoProcess


/**
 * \brief Creates a PhoneAccountCreate request
 * \param nRequestPriority Priority of the request
 */
stiHResult CstiCoreService::PhoneAccountCreate (
	int nRequestPriority)
{
	stiLOG_ENTRY_NAME (CstiCoreService::PhoneAccountCreate);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Create a core request object
	auto  poCoreRequest = new CstiCoreRequest ();

	stiTESTCOND(poCoreRequest != nullptr, stiRESULT_ERROR);

	// Yes! Set the ID
	poCoreRequest->requestIdSet (NextRequestIdGet ());

	nResult = poCoreRequest->phoneAccountCreate(phoneTypeConvert (m_productType));

	stiTESTCOND(nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

	poCoreRequest->PrioritySet (nRequestPriority);

	// Yes! Send the request.
	hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());

	stiTESTRESULT();

	poCoreRequest = nullptr;

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poCoreRequest)
		{
			delete poCoreRequest;

			poCoreRequest = nullptr;
		}
	}

	return (hResult);
} // end CstiCoreService::PhoneAccountCreate


void CstiCoreService::retryOffline (CstiVPServiceRequest *request, const std::function<void ()>& callback)
{
	auto coreRequest = (CstiCoreRequest*)request;
	offlineCore.enable ();
	offlineCore.handleRequest (coreRequest, callback);
}


void CstiCoreService::offlineCoreResponseSend (CstiCoreResponse *response)
{
	// Send the response
	Callback (estiMSG_CORE_RESPONSE, response);
}

void CstiCoreService::offlineCoreCredentialsUpdate (const std::string& userName, const std::string& password)
{
	m_userPhone = userName;
	m_userPin = password;
}

void CstiCoreService::offlineCoreClientAuthenticate (const ServiceResponseSP<ClientAuthenticateResult>& response)
{
	if (response->responseSuccessful)
	{
		clientTokenSet ({});

		m_bAuthenticationInProcess = estiFALSE;

		// Set client tokens for the other services
		if (m_pStateNotifyService)
		{
			m_pStateNotifyService->clientTokenSet ({});
		}

		if (m_pMessageService)
		{
			m_pMessageService->clientTokenSet ({});
		}

		if (m_pConferenceService)
		{
			m_pConferenceService->clientTokenSet ({});
		}
	}

	clientAuthenticateSignal.Emit (response);
}


/**
 * \brief Restarts the Core service
 * 
 * Restart the Core Services by initializing the member variables and tring to
 *  connect to the core server again
 * \return stiRESULT_SUCCESS - if there is no eror. Otherwize, some errors occur.
 */
stiHResult CstiCoreService::Restart ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::Restart);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_refreshTimer.stop ();

	CstiVPService::Restart();

	hResult = QueueServiceContactsGet ();
	stiTESTRESULT();

STI_BAIL:

	return (hResult);

}


/**
 * \brief Initializes the class.
 * 
 * This method is called soon after the object is created to initialize the object.
 * \param bPause Pause the service
 */
stiHResult CstiCoreService::Startup (
	bool bPause)
{
	stiLOG_ENTRY_NAME (CstiCoreService::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	hResult = CstiVPService::Startup(bPause);

	stiTESTRESULT ();

	hResult = QueueServiceContactsGet ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end CstiCoreService::Startup


/**
 * \brief Logs the current user off the videophone
 * \param pContext The context to set in the request
 * \param punRequestId The request ID to set (if not null
 * \return EstiResult - estiOK on success, otherwise estiERROR
 */
stiHResult CstiCoreService::Logout (
	const VPServiceRequestContextSharedPtr &context,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCoreService::Logout);
	int nResult = 0;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Create a core request object
	auto  poCoreRequest = new CstiCoreRequest ();
	stiTESTCOND (poCoreRequest, stiRESULT_ERROR);

	poCoreRequest->contextItemSet (context);

	// Create the ClientLogout request.
	nResult = poCoreRequest->clientLogout ();
	stiTESTCOND (REQUEST_SUCCESS == nResult, stiRESULT_ERROR);

	hResult = (stiHResult)RequestSend (poCoreRequest, punRequestId, poCoreRequest->PriorityGet ());
	stiTESTRESULT ();

	poCoreRequest = nullptr;

	// Forget our saved credentials, since they will no longer be valid.
	m_userPhone.clear ();
	m_userPin.clear ();

STI_BAIL:

	if (poCoreRequest)
	{
		delete poCoreRequest;
		poCoreRequest = nullptr;
	}

	return (hResult);
} // end CstiCoreService::Logout


/**
 * \brief Causes the system to reauthenticate silently
 * 
 * This generates new client credentials, without prompting the user to
 *  reenter their password.
 * \param bLogOut Send a logout core request
 * \return stiHResult - success or failure result
 */
stiHResult CstiCoreService::Reauthenticate ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::Reauthenticate);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *poCoreRequest = nullptr;

	// If not already in Process, send authenticate request
	if (!m_bAuthenticationInProcess && m_reauthenticateUserLoginCheckRequestId == 0)
	{
		stiTESTCOND (!m_userPhone.empty() && !m_userPin.empty(), stiRESULT_ERROR);

		// Create a core request object
		poCoreRequest = new CstiCoreRequest ();
		stiTESTCOND (poCoreRequest != nullptr, stiRESULT_ERROR);

		m_reauthenticateUserLoginCheckRequestId = NextRequestIdGet ();

		// Yes! Set the request number
		poCoreRequest->requestIdSet (m_reauthenticateUserLoginCheckRequestId);

		int nResult = REQUEST_SUCCESS;

		// Format the request using our stored credentials
		nResult = poCoreRequest->userLoginCheck(
			m_userPhone.c_str(),
			m_userPin.c_str());

		if (REQUEST_SUCCESS != nResult)
		{
			m_reauthenticateUserLoginCheckRequestId = 0;
			stiTHROW (stiRESULT_ERROR);
		}

		hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());
		stiTESTRESULT ();

		poCoreRequest = nullptr;
	}

STI_BAIL:

	delete poCoreRequest;

	return (hResult);
} // end CstiCoreService::Reauthenticate


/**
 * \brief Sends a request to the Core Service
 * \param poCoreRequest The core request object that is to be sent
 * \param punRequestId The request ID assigned to this request
 * \param nPriority The priority of this request
 * \return EstiResult - estiOK if the request has been queued, estiERROR if the request cannot be queued.
 *
 * \deprecated This method has been deprecated.  Use RequestSendEx instead.
 */
stiHResult CstiCoreService::RequestSend (
	CstiCoreRequest *poCoreRequest,
	unsigned int *punRequestId,
	int nPriority)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RequestSend);

	unsigned int unRequestId;
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Were we given a valid request?
	stiTESTCOND (poCoreRequest != nullptr, stiRESULT_ERROR);

	// Yes; Check if we need to be logged in for this request.
	// If we do but we don't have credentials then
	// fail.
	//
	if (poCoreRequest->authenticationRequired()
	 && StateGet() != CstiCoreService::eAUTHENTICATED
	 && (m_userPhone.empty() || m_userPin.empty()))
	{
		delete poCoreRequest;
		stiTHROW (stiRESULT_ERROR);
	}
	else
	{
		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poCoreRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		poCoreRequest->PrioritySet (nPriority);

		if (isOfflineGet() && !poCoreRequest->onlineOnlyGet())
		{
			offlineCore.handleRequest (poCoreRequest, nullptr);

			hResult = QueueServiceContactsGet ();
			stiTESTRESULT ();

			PostEvent([this]{ requestSendHandle (); });
		}
		else
		{
			hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());

			if (stiIS_ERROR (hResult))
			{
				delete poCoreRequest;
				stiTHROW (stiRESULT_ERROR);
			}
			else
			{
				// Check if we need to be logged in for this request
				// If not already in process of authenticating, send authenticate request
				if (poCoreRequest->authenticationRequired()
				 && StateGet() != CstiCoreService::eAUTHENTICATED)
				{
					hResult = Reauthenticate();
					stiTESTRESULT ();
				}
			}
		}
	}

STI_BAIL:

	return (hResult);
} // end CstiCoreService::RequestSend


stiHResult CstiCoreService::logInUsingCache (const std::string& phoneNumber, const std::string& pin, const std::string& token)
{
	stiHResult hResult{ stiRESULT_SUCCESS };
	CstiUserAccountInfo userAccountInfo{};
	vpe::Address address{};
	EstiEmergencyAddressProvisionStatus addressStatus{};
	bool result{};
	auto requestId = NextRequestIdGet ();

	// Create Response object
	auto clientAuthenticateResult = std::make_shared<vpe::ServiceResponse<ClientAuthenticateResult>> ();
	auto userAccountInfoGetResult = std::make_shared<vpe::ServiceResponse<CstiUserAccountInfo>> ();
	auto addressResult = std::make_shared<vpe::ServiceResponse<vpe::Address>> ();
	auto addressStatusResult = std::make_shared<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>> ();

	clientAuthenticateResult->origin = vpe::ResponseOrigin::LocalCache;
	userAccountInfoGetResult->origin = vpe::ResponseOrigin::LocalCache;
	addressResult->origin = vpe::ResponseOrigin::LocalCache;
	addressStatusResult->origin = vpe::ResponseOrigin::LocalCache;

	clientAuthenticateResult->requestId = requestId;
	clientAuthenticateResult->requestId = requestId;
	addressResult->requestId = requestId;
	addressStatusResult->requestId = requestId;

	// Get and validate data for responses
	stiTESTCOND (!token.empty (), stiRESULT_ERROR);

	result = offlineCore.authenticate (phoneNumber, pin);
	stiTESTCOND (result, stiRESULT_ERROR);

	hResult = StateSet (eAUTHENTICATED);
	stiTESTRESULT ();

	result = offlineCore.userAccountInfoGet (userAccountInfo);
	stiTESTCOND (result, stiRESULT_ERROR);

	stiTESTCOND (!userAccountInfo.m_LocalNumber.empty (), stiRESULT_ERROR);

	result = offlineCore.emergencyAddressGet (address);
	stiTESTCOND (result, stiRESULT_ERROR);

	result = offlineCore.emergencyAddressStatusGet (addressStatus);
	stiTESTCOND (result, stiRESULT_ERROR);

	// Fill data on results
	clientAuthenticateResult->content.userId = offlineCore.propertyStringGet (PHONE_USER_ID);
	clientAuthenticateResult->content.groupUserId = offlineCore.propertyStringGet (GROUP_USER_ID);
	userAccountInfoGetResult->content = userAccountInfo;
	addressResult->content = address;
	addressStatusResult->content = addressStatus;

	// Update service data
	m_userPhone = phoneNumber;
	m_userPin = pin;

	clientTokenSet (token);
	m_pStateNotifyService->clientTokenSet (token);
	m_pMessageService->clientTokenSet (token);
	m_pConferenceService->clientTokenSet (token);

STI_BAIL:
	auto success = !stiIS_ERROR (hResult);
	clientAuthenticateResult->responseSuccessful = success;
	userAccountInfoGetResult->responseSuccessful = success;
	addressResult->responseSuccessful = success;
	addressStatusResult->responseSuccessful = success;

	clientAuthenticateSignal.Emit (clientAuthenticateResult);
	if (success)
	{
		userAccountInfoReceivedSignal.Emit (userAccountInfoGetResult);
		emergencyAddressReceivedSignal.Emit (addressResult);
		emergencyAddressStatusReceivedSignal.Emit (addressStatusResult);
	}

	return hResult;
}

bool CstiCoreService::isOfflineGet ()
{
	return offlineCore.enabledGet();
}


/**
 * \brief Sends a request to the Core Service
 * \param poCoreRequest The core request object that is to be sent
 * \param punRequestId The request ID assigned to this request
 * \param nPriority The priority of this request
 * \returns Success or failure in an stiHResult
 */
stiHResult CstiCoreService::RequestSendEx (
	CstiCoreRequest *poCoreRequest,
	unsigned int *punRequestId,
	int nPriority)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RequestSend);

	unsigned int unRequestId;
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Were we given a valid request?
	stiTESTCOND (poCoreRequest, stiRESULT_ERROR);

	// Yes; Check if we need to be logged in for this request.
	// If we do but we don't have credentials then
	// fail.
	//
	if (poCoreRequest->authenticationRequired()
	 && StateGet() != CstiCoreService::eAUTHENTICATED
	 && (m_userPhone.empty() || m_userPin.empty()))
	{
		stiTHROW (stiRESULT_ERROR);
	}
	else if (isOfflineGet() && !poCoreRequest->onlineOnlyGet())
	{
		unRequestId = NextRequestIdGet ();
		poCoreRequest->requestIdSet (unRequestId);
		if (punRequestId) 
		{
			*punRequestId = unRequestId;
		}
		offlineCore.handleRequest (poCoreRequest, nullptr);

		hResult = QueueServiceContactsGet ();
		stiTESTRESULT ();

		PostEvent([this] { requestSendHandle (); });
	}
	else
	{
		// Check if we need to be logged in for this request
		// If not already in process of authenticating, send authenticate request
		if (poCoreRequest->authenticationRequired()
		 && StateGet() != CstiCoreService::eAUTHENTICATED)
		{
			hResult = Reauthenticate();
			stiTESTRESULT ();
		}

		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poCoreRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		poCoreRequest->PrioritySet(nPriority);

		hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiCoreService::RequestSendEx


/**
 * \brief Sends a request to the Core Service (asynchronously)
 * \param poCoreRequest The core request object that is to be sent
 * \param Callback The callback function pointer
 * \param CallbackParam Callback function parameter
 * \param nPriority The priority of this request
 * \return EstiResult - estiOK if the request has been queued, estiERROR if the	request cannot be queued.
 */
stiHResult CstiCoreService::RequestSendAsync (
	CstiCoreRequest * poCoreRequest,
	PstiObjectCallback Callback,
	size_t CallbackParam,
	int nPriority)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiAsyncRequest *pAsyncRequest = nullptr;

	// Were we given a valid request?
	stiTESTCOND (poCoreRequest != nullptr, stiRESULT_ERROR);

	{
		pAsyncRequest = new SstiAsyncRequest;

		pAsyncRequest->Callback = Callback;
		pAsyncRequest->pCoreRequest = poCoreRequest;
		pAsyncRequest->CallbackParam = CallbackParam;
		pAsyncRequest->nPriority = nPriority;

		PostEvent ([this, pAsyncRequest]() { RequestSendHandle (pAsyncRequest); });

		pAsyncRequest = nullptr;

	} // end if

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (pAsyncRequest)
		{
			delete pAsyncRequest;
			pAsyncRequest = nullptr;
		}
	}

	return (hResult);
} // end CstiCoreService::RequestSendAsync


/**
 * \brief Creates and sends a ClientAuthenticate core request
 * \param szPhoneNumber The user phone number (or login name)
 * \param szPin The user password (or login password)
 * \param pContext The context to set in the request
 * \param punRequestId Pointer to request id associated with this request
 * \return EstiResult - estiOK on success, otherwise estiERROR
 */
stiHResult CstiCoreService::Login (
	const char * szPhoneNumber,
	const char * szPin,
	const VPServiceRequestContextSharedPtr &context,
	unsigned int *punRequestId,
	const char * pszLoginAs)
{
	stiLOG_ENTRY_NAME (CstiCoreService::Login);

	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unRequestId = 0;
	char szPhone[un8stiCANONICAL_PHONE_LENGTH] = "";
	int nIndex = 0;
	int nResult = 0;

	if (isOfflineGet())
	{
		offlineCore.login (szPhoneNumber, szPin, punRequestId);
		return stiRESULT_SUCCESS;
	}

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Create a core request object
	auto  poCoreRequest = new CstiCoreRequest ();
	stiTESTCOND (poCoreRequest, stiRESULT_ERROR);

	// Yes! Set the request number
	unRequestId = NextRequestIdGet ();

	poCoreRequest->requestIdSet (unRequestId);
	poCoreRequest->contextItemSet (context);

	poCoreRequest->retryOfflineSet (true);

	// Remove non-digits from the phone number
	for (size_t i = 0; i < strlen (szPhoneNumber); i++)
	{
		if (isdigit (szPhoneNumber[i]))
		{
			szPhone[nIndex] = szPhoneNumber[i];
			nIndex++;
		} // end if
	} // end for
	szPhone[nIndex] = '\0';

	// Keep a copy of the credentials for the reauthenticate function
	m_userPhone = szPhone;
	m_userPin = szPin;

	nResult = poCoreRequest->userLoginCheck (szPhone, szPin);

	// Did we succeed?
	stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

	// Format the request
	nResult = poCoreRequest->clientAuthenticate(
		"",
		szPhone,
		szPin,
		nullptr,
		pszLoginAs);

	// Did we succeed?
	stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

	hResult = RequestQueue (poCoreRequest, poCoreRequest->PriorityGet ());
	stiTESTRESULT ();

	poCoreRequest = nullptr;

	m_bAuthenticationInProcess = estiTRUE;

	if (punRequestId)
	{
		*punRequestId = unRequestId;
	}

STI_BAIL:

	if (poCoreRequest)
	{
		delete poCoreRequest;
		poCoreRequest = nullptr;
	}

	return (hResult);
} // end CstiCoreService::Login


/**
 * \brief Notify system that Logged out message was recived
 * 
 * This checks if we really need a reauthentication, and if so, calls it.
 * 
 * \param pszClientToken The new client token to associate with the log out request
 */
void CstiCoreService::LoggedOutNotifyAsync (const std::string& token)
{
	stiLOG_ENTRY_NAME (CstiCoreService::LoggedOutNotifyAsync);
	PostEvent ([this, token]() {
		LoggedOutNotifyHandle (token);
	});
} // end CstiCoreService::LoggedOutNotifyAsync


/**
 * \brief Notify system that Reauthenticate message was recived
 * 
 * This checks if we really need a reauthentication, and if so, calls it.
 */
void CstiCoreService::ReauthenticateAsync ()
{
	stiLOG_ENTRY_NAME (CstiCoreService::ReauthenticateAsync);

	PostEvent (std::bind(&CstiCoreService::Reauthenticate, this));
} // end CstiCoreService::ReauthenticateAsync


/**
 * \brief Update internally stored user pin
 * \param szPin The new PIN (or Password)
 */
void CstiCoreService::UserPinSet (
	const char *szPin)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_userPin = szPin;
}


/**
 * \brief Called by the base class to inform the derived class that SSL has failed over.
 */
void CstiCoreService::SSLFailedOver()
{
	Callback (estiMSG_CORE_SERVICE_SSL_FAILOVER, nullptr);
}

#ifdef stiDEBUGGING_TOOLS
/**
 * \brief Determine if debug trace is set in derived class
 * 
 * Called by the base class to determine the derived type and to
 *  know if the debug tool for the derived class is enabled.
 * 
 * \param ppszService Returns the service type string ("CS" in this case)
 * \return estiTRUE if the debug tool for the derived class is enabled, estiFALSE otherwise.
 */
EstiBool CstiCoreService::IsDebugTraceSet (const char **ppszService) const
{
	EstiBool bRet = estiFALSE;

	if (g_stiCSDebug)
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
 * \param ppszService Returns the service type string ("CS" in this case).
 */
const char * CstiCoreService::ServiceIDGet () const
{
	return "CS";
}


/**
 * \brief Retrieves the RelayLanguage list from the core response
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::RelayLanguageListGetProcess (
	CstiCoreResponse * poResponse,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RelayLanguageListGetProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::vector <CstiCoreResponse::SRelayLanguage> RelayLanguageList;

	// Yes! Are there any relay language items in the result?
	if (pChildNodes != nullptr)
	{
		// Yes!
		int y = 0;
		IXML_Node * pChildNode = nullptr;

		// Get to the relay language list
		// This line will look like:  <RelayLanguageList Count="2">
		pChildNode = ixmlNodeList_item (pChildNodes, 0);

		// Get all elements named "RelayLanguage"
		IXML_NodeList *pRelayNodeList = ixmlElement_getElementsByTagName (
			(IXML_Element *)pChildNode, (DOMString)TAG_RelayLanguage);

		// Get the count of items in the list
		unsigned long ulNodeCount = ixmlNodeList_length (pRelayNodeList);

		// Does iXML report that there is only one node?
		if (1 == ulNodeCount)
		{
			// Yes! Get the first child node.
			pChildNode = ixmlNodeList_item (pChildNodes, 0);

			// Is the single node really valid?
			if (eELEMENT_NODE != ixmlNode_getNodeType (pChildNode))
			{
				// No! There are really zero nodes.
				ulNodeCount = 0;
			} // end if
		} // end if

		// Were there any items in the list?
		if (ulNodeCount > 0)
		{
			CstiCoreResponse::SRelayLanguage stRelayLanguage;

			// Loop through the items, and process them.
			pChildNode = ixmlNodeList_item (pRelayNodeList, y);
			while (pChildNode)
			{
				// Is this node really a relay language item?
				if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
						TAG_RelayLanguage)))
				{
					IXML_NodeList *pNameValueNodes = ixmlNode_getChildNodes (pChildNode);

					if (pNameValueNodes)
					{
						// Yes! Retrieve id and name
						const char *pszId = nullptr;

						IXML_Node *pIdNode = ixmlNodeList_item(pNameValueNodes, 0);

						// Process this node
						IXML_Node *pTextNode;  // temporary node to hold the tag's text value
						pTextNode = ixmlNode_getFirstChild(pIdNode);
						pszId = ixmlNode_getNodeValue (pTextNode);

						// Was there really a name?
						if (nullptr != pszId)
						{
							// Yes! Process it
							stRelayLanguage.nId = atoi (pszId);

							const char *pszName2 = nullptr;

							//
							// Get the property value
							//
							IXML_Node *pNameNode = ixmlNodeList_item(pNameValueNodes, 1);
							pTextNode = ixmlNode_getFirstChild(pNameNode);
							pszName2 = ixmlNode_getNodeValue(pTextNode);
							if (nullptr == pszName2)
							{
								pszName2 = "";
							} // end if

							stRelayLanguage.Name = pszName2;

						} // end if

						ixmlNodeList_free(pNameValueNodes);

						// Add this structure to the vector
						RelayLanguageList.push_back (stRelayLanguage);
					}
				} // end if

				// Move to the next child node
				y++;
				pChildNode = ixmlNodeList_item (pRelayNodeList, y);
			} // end while

			if (!RelayLanguageList.empty ())
			{
				poResponse->RelayLanguageListSet (&RelayLanguageList);
			}

		} // end if
		ixmlNodeList_free (pRelayNodeList);

	} // end if

	return hResult;
}


/**
 * \brief Retrieves the Ring Group info from the core response
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::RingGroupInfoGetProcess (
	CstiCoreResponse * poResponse,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RingGroupInfoGetProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::vector <CstiCoreResponse::SRingGroupInfo> RingGroupInfoList;

	int y = 0;
	IXML_Node * pChildNode = nullptr;

	// NOTE: There are items in this core response we don't need to get, 
	//  so we skip several of the tags that are before <RingGroupMemberList>

#if 0 
//NOTE: THIS CODE caused inexplicable crash occasionally (down in IXML), so I
// replaced it with the code below to get the ring group member list element directly
	// Get to the RingGroup member list
	// This line will look like:  <RingGroupMemberList> 
	IXML_NodeList *pRingGroupMemberList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pChildNodes, (DOMString)TAG_RingGroupMemberList);

	// Get the first child node, which is the <RingGroupMemberList>
	IXML_Node * pRingGroupMemberListNode = ixmlNodeList_item (pRingGroupMemberList, 0);

	// Now that we have member list node, we don't need the containing list, so free it
	ixmlNodeList_free (pRingGroupMemberList);
#else
	// RingGroupMemberList is the 4th tag (or index 3)
	IXML_Node *pRingGroupMemberListNode = ixmlNodeList_item(pChildNodes, 3);
#endif

	// Get all elements named "RingGroupMember"
	IXML_NodeList *pRingGroupNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pRingGroupMemberListNode, (DOMString)TAG_RingGroupMember);

	// Get the count of items in the list
	unsigned long ulNodeCount = ixmlNodeList_length (pRingGroupNodeList);

	// Were there any items in the list?
	if (ulNodeCount > 0)
	{
		CstiCoreResponse::SRingGroupInfo stRingGroupInfo;

		// Loop through the items, and process them.
		pChildNode = ixmlNodeList_item (pRingGroupNodeList, y);
		while (pChildNode)
		{
			// Is this node really a RingGroupMember item?
			if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
					TAG_RingGroupMember)))
			{
				IXML_NodeList *pRGMemberNodes = ixmlNode_getChildNodes (pChildNode);

				// NOTE: the RingGroupMember tag has the following structure...
				// <RingGroupMember>
				//  <UserName>MyName</UserName
				//  <Description>Living Room</Description>
				//  <LocalNumber>12223334444</LocalNumber>
				//  <TollFreeNumber>18882223333</TollFreeNumber>
				//  <GroupState>Active</GroupState>
				//  <GroupCreator>True</GroupCreator>
				// </RingGroupMember>
				//
				// NOTE: THIS CODE USES INDEXES FOR THE TAGS, SO IF THE ORDER CHANGES,
				//  THE CODE MUST ALSO CHANGE!
				//
				if (pRGMemberNodes)
				{
					IXML_Node *pTextNode;  // temporary node to hold the tag's text value

					// Retrieve the description
					const char *pszDescription = nullptr;

					// Description is the 2nd tag (or index 1)
					IXML_Node *pDescNode = ixmlNodeList_item(pRGMemberNodes, 1);

					// Process this description node
					pTextNode = ixmlNode_getFirstChild(pDescNode);
					pszDescription = ixmlNode_getNodeValue (pTextNode);

					if (nullptr == pszDescription)
					{
						pszDescription = "";
					}
					// Add the description
					stRingGroupInfo.Description = pszDescription;

					// Retrieve the phone number
					const char *pszLocalPhoneNumber = nullptr;
					// Local is the 3rd tag (or index 2)
					IXML_Node *pLocalNumNode = ixmlNodeList_item(pRGMemberNodes, 2);

					// Process this node
					pTextNode = ixmlNode_getFirstChild(pLocalNumNode);
					pszLocalPhoneNumber = ixmlNode_getNodeValue (pTextNode);
					if (nullptr == pszLocalPhoneNumber)
					{
						pszLocalPhoneNumber = "";
					}
					// Add phone number
					stRingGroupInfo.LocalPhoneNumber = pszLocalPhoneNumber;

					// Retrieve the toll-free phone number
					const char *pszTollFreePhoneNumber = nullptr;
					// Toll-free is the 4th tag (or index 3)
					IXML_Node *pTollFreeNumNode = ixmlNodeList_item(pRGMemberNodes, 3);

					// Process this node
					pTextNode = ixmlNode_getFirstChild(pTollFreeNumNode);
					pszTollFreePhoneNumber = ixmlNode_getNodeValue (pTextNode);
					
					if (nullptr == pszTollFreePhoneNumber)
					{
						pszTollFreePhoneNumber = "";
					}
					// Add toll-free phone number
					stRingGroupInfo.TollFreePhoneNumber = pszTollFreePhoneNumber;

					// Now retrieve the group state
					const char *pszState = nullptr;

					// Get the GroupState value (tag 5, index 4)
					IXML_Node *pGroupStateNode = ixmlNodeList_item(pRGMemberNodes, 4);
					pTextNode = ixmlNode_getFirstChild(pGroupStateNode);
					pszState = ixmlNode_getNodeValue(pTextNode);
					if (nullptr == pszState)
					{
						pszState = "";
					} // end if

					// Set the group state
					CstiCoreResponse::ERingGroupMemberState eGrpState =
						(0 == stiStrICmp (pszState, "Active")) ? CstiCoreResponse::eRING_GROUP_STATE_ACTIVE :
						((0 == stiStrICmp (pszState, "Pending")) ? CstiCoreResponse::eRING_GROUP_STATE_PENDING :
						CstiCoreResponse::eRING_GROUP_STATE_UNKNOWN);
					stRingGroupInfo.eState = eGrpState;

					// Get the group creator flag (tag 6, index 5)
					const char *pszCreatorFlag = nullptr;
					IXML_Node *pCreatorNode = ixmlNodeList_item(pRGMemberNodes, 5);
					pTextNode = ixmlNode_getFirstChild(pCreatorNode);
					pszCreatorFlag = ixmlNode_getNodeValue(pTextNode);
					if (nullptr == pszCreatorFlag)
					{
						pszCreatorFlag = "false";
					} // end if

					// Set the group creator flag
					stRingGroupInfo.bIsGroupCreator =
						0 == stiStrICmp (pszCreatorFlag, "true");

					ixmlNodeList_free(pRGMemberNodes);

					// Add this structure to the vector
					RingGroupInfoList.push_back (stRingGroupInfo);
				}
			} // end if

			// Move to the next child node
			y++;
			pChildNode = ixmlNodeList_item (pRingGroupNodeList, y);
		} // end while

		if (!RingGroupInfoList.empty ())
		{
			poResponse->RingGroupInfoListSet (&RingGroupInfoList);
		}
	} // end if
	ixmlNodeList_free (pRingGroupNodeList);

	return hResult;
}


/**
 * \brief Retrieves the Registration info from the core response
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::RegistrationInfoGetProcess (
	CstiCoreResponse * poResponse,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::RegistrationInfoGetProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCoreResponse::SRegistrationInfo sRegInfo;
	std::vector <CstiCoreResponse::SRegistrationInfo> RegistrationInfoList;

	for (int i = 0;; ++i)
	{
		IXML_Node *pSIPRegNode = ixmlNodeList_item(pChildNodes, i);

		if (pSIPRegNode == nullptr)
		{
			break;
		}

		auto pszValue = ixmlElement_getAttribute (
			(IXML_Element *)pSIPRegNode,
			(DOMString)"Type");

		if (0 == stiStrICmp (pszValue, "sip"))
		{
			sRegInfo.eType = estiSIP;	// Should be "SIP"
	
			// Get the SIP Public and private domain values...
			IXML_Node *pSIPRegPublicDomain = ixmlNode_getFirstChild(pSIPRegNode);
			IXML_Node *pTextNode = ixmlNode_getFirstChild(pSIPRegPublicDomain);
			sRegInfo.PublicDomain = ixmlNode_getNodeValue(pTextNode);

			// Get the private domain
			IXML_Node *pSIPRegPrivateDomain = ixmlNode_getNextSibling(pSIPRegPublicDomain);
			if (pSIPRegPrivateDomain)
			{
				pTextNode = ixmlNode_getFirstChild(pSIPRegPrivateDomain);
				if (pTextNode)
				{
					sRegInfo.PrivateDomain = ixmlNode_getNodeValue(pTextNode);
				}
			}

			// Get the Credentials List
			IXML_Node *pSIPRegCredentials = ixmlNode_getNextSibling(pSIPRegPrivateDomain);

			IXML_NodeList *pCredentialsList = ixmlNode_getChildNodes(pSIPRegCredentials);

			// Get the number of credentials in the list
		// 	unsigned long ulNodeCount = ixmlNodeList_length (pCredentialsList);
			unsigned long y = 0;

			// Get first item in list
			IXML_Node * pCredentialsNode = ixmlNodeList_item (pCredentialsList, 0);

			while (pCredentialsNode)
			{
				CstiCoreResponse::SRegistrationCredentials Credentials;

				// Get the username/password from the first (local number) credentials list item...
				IXML_Node *pLocalUserName = ixmlNode_getFirstChild(pCredentialsNode);
				pTextNode = ixmlNode_getFirstChild(pLocalUserName);
				if (pTextNode)
				{
					Credentials.UserName = ixmlNode_getNodeValue(pTextNode);
				}

				IXML_Node *pLocalPassword = ixmlNode_getLastChild(pCredentialsNode);
				pTextNode = ixmlNode_getFirstChild(pLocalPassword);
				if (pTextNode)
				{
					Credentials.Password = ixmlNode_getNodeValue(pTextNode);
				}

				// Put this SIP credential into the list
				sRegInfo.Credentials.push_back (Credentials);

				// Move to the next child node
				y++;
				pCredentialsNode = ixmlNodeList_item (pCredentialsList, y);
			}

			ixmlNodeList_free (pCredentialsList);

			// Put this SIP registration info in the list
			RegistrationInfoList.push_back (sRegInfo);

			//
			// We found the SIP node. No need to look any further.
			//
			break;
		}
	}
	
	// Save to the registration object
	poResponse->RegistrationInfoListSet (&RegistrationInfoList);


	return hResult;
}


/**
 * \brief Retrieves the Agreements from the core response
 * <AgreementStatusGetResult Result="OK">
 *   <Agreement>
 *     <AgreementType>Provider Agreement</AgreementType>
 *     <AgreementPublicId>932959C0-CA37-4FF1-B911-A3F6D112A2C8</AgreementPublicId>
 *     <Status>NeedsAcceptance</Status>
 *   </Agreement>
 *   <Agreement>
 *     <AgreementType> Deaf Certification </AgreementType>
 *     <AgreementPublicId>942959C0-BA37-4FF1-D911-F3F6D112A241</AgreementPublicId>
 *     <Status>Accepted</Status>
 *   </Agreement>
 * </AgreementStatusGetResult>
 *
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::AgreementStatusGetProcess (
	CstiCoreResponse * poResponse,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::AgreementGetProcess);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int currentNode = 0;
	IXML_Node * pChildNode = nullptr;
	
	// Get the count of items in the list
	unsigned long ulNodeCount = ixmlNodeList_length (pChildNodes);
	
	stiTESTCOND(ulNodeCount != 0, stiRESULT_ERROR);
	
	// Were there any items in the list?
	if (ulNodeCount > 0)
	{
		std::vector <CstiCoreResponse::SAgreement> AgreementList;
		// Loop through the items, and process them.
		pChildNode = ixmlNodeList_item (pChildNodes, currentNode);
		while (pChildNode)
		{
			// Is this node really an Agreement item?
			stiTESTCOND(0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
									TAG_Agreement), stiRESULT_ERROR);
			
			CstiCoreResponse::SAgreement sAgreement;
			const char *pszAgreementType = SubElementValueGet (pChildNode, TAG_AgreementType);
			const char *pszPublicId = SubElementValueGet (pChildNode, TAG_AgreementPublicId);
			const char *pszStatus = SubElementValueGet (pChildNode, TAG_Status);
			if (pszAgreementType)
			{
				sAgreement.AgreementType = pszAgreementType;
			}
			if (pszPublicId)
			{
				sAgreement.AgreementPublicId = pszPublicId;
				poResponse->ResponseStringSet (0, pszPublicId);
			}
			if (pszStatus)
			{
				sAgreement.AgreementStatus = pszStatus;
				poResponse->ResponseStringSet (1, pszStatus);
			}
			// Put this agreement in the list
			AgreementList.push_back (sAgreement);
			
			currentNode++;
			pChildNode = ixmlNodeList_item (pChildNodes, currentNode);
		}
		// Save to the agreement object
		poResponse->AgreementListSet (&AgreementList);
	}
	
STI_BAIL:
	
	return hResult;
}


//////////////////
/**
 * \brief ProcessClientAuthenticateWhichLogin
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::ProcessClientAuthenticateWhichLogin (
	std::vector<CstiCoreResponse::SRingGroupInfo>& ringGroupInfoList,
	IXML_NodeList * pChildNodes)
{
	stiLOG_ENTRY_NAME (CstiCoreService::ProcessClientAuthenticateWhichLogin);

	stiHResult hResult = stiRESULT_SUCCESS;

	int y = 0;
	IXML_Node * pChildNode = nullptr;

	// Get the count of items in the list
	unsigned long ulNodeCount = ixmlNodeList_length (pChildNodes);

	// Were there any items in the list?
	if (ulNodeCount > 0)
	{
		CstiCoreResponse::SRingGroupInfo stRingGroupInfo;

		// Loop through the items, and process them.
		pChildNode = ixmlNodeList_item (pChildNodes, y);
		while (pChildNode)
		{
			// Is this node really a UserIdentity item?
			if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
					TAG_UserIdentity)))
			{
				IXML_NodeList *pUserIdNodes = ixmlNode_getChildNodes (pChildNode);

				// NOTE: the UserIdentity tag has the following structure...
				// <UserIdentity>
				//  <Description>Living Room</Description>
				//  <PhoneNumber>12223334444</PhoneNumber>
				// </UserIdentity>
				//
				// NOTE: THIS CODE USES INDEXES FOR THE TAGS, SO IF THE ORDER CHANGES,
				//  THE CODE MUST ALSO CHANGE!
				//
				if (pUserIdNodes)
				{
					IXML_Node *pTextNode;  // temporary node to hold the tag's text value

					// Retrieve the description
					const char *pszDescription = nullptr;

					// Description is the 2nd tag (or index 1)
					IXML_Node *pDescNode = ixmlNodeList_item(pUserIdNodes, 1);

					// Process this description node
					pTextNode = ixmlNode_getFirstChild(pDescNode);
					pszDescription = ixmlNode_getNodeValue (pTextNode);

					if (nullptr == pszDescription)
					{
						pszDescription = "";
					}
					// Add the description
					stRingGroupInfo.Description = pszDescription;

					// Retrieve the local phone number
					const char *pszLocalPhoneNumber = nullptr;
					// Local is the 1st tag (or index 0)
					IXML_Node *pLocalNumNode = ixmlNodeList_item(pUserIdNodes, 0);

					// Process this node
					pTextNode = ixmlNode_getFirstChild(pLocalNumNode);
					pszLocalPhoneNumber = ixmlNode_getNodeValue (pTextNode);

					if (nullptr == pszLocalPhoneNumber)
					{
						pszLocalPhoneNumber = "";
					}
					// Add phone number
					stRingGroupInfo.LocalPhoneNumber = pszLocalPhoneNumber;

					// Set the group state (not needed for this response)
					CstiCoreResponse::ERingGroupMemberState eGrpState = CstiCoreResponse::eRING_GROUP_STATE_UNKNOWN;
					stRingGroupInfo.eState = eGrpState;

					ixmlNodeList_free(pUserIdNodes);

					// Add this structure to the vector
					ringGroupInfoList.push_back (stRingGroupInfo);
				}
			} // end if

			// Move to the next child node
			y++;
			pChildNode = ixmlNodeList_item (pChildNodes, y);
		} // end while

	} // end if

	return hResult;
}
////////////////////

/**
 * \brief Retrieves the settings list from the core response
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \param pRequestedSettingsList The list in which to store the retrieved settings
 * \return success or failure
 */
stiHResult CstiCoreService::SettingsGetProcess (
	CstiCoreResponse * poResponse,
	IXML_NodeList * pChildNodes,
	std::list <std::string> *pRequestedSettingsList)
{
	stiLOG_ENTRY_NAME (CstiCoreService::SettingsGetProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Yes! Are there any user info items in the result?
	if (pChildNodes != nullptr)
	{
		// Yes!
		int y = 0;
		IXML_Node * pChildNode = nullptr;

		// Get the count of items in the list
		unsigned long ulNodeCount = ixmlNodeList_length (pChildNodes);

		// Does iXML report that there is only one node?
		if (1 == ulNodeCount)
		{
			// Yes! Get the first child node.
			pChildNode = ixmlNodeList_item (pChildNodes, 0);

			// Is the single node really valid?
			if (eELEMENT_NODE != ixmlNode_getNodeType (pChildNode))
			{
				// No! There are really zero nodes.
				ulNodeCount = 0;
			} // end if
		} // end if

		// Were there any items in the list?
		if (ulNodeCount > 0)
		{
			std::vector<SstiSettingItem> settingList;

			// Loop through the items, and process them.
			pChildNode = ixmlNodeList_item (pChildNodes, y);
			while (pChildNode)
			{
				// Is this node really a user or phone setting item?
				if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
						TAG_UserSettingItem)) ||
					(0 == stiStrICmp (ixmlNode_getNodeName (pChildNode),
						TAG_PhoneSettingItem)))
				{
					IXML_NodeList *pNameValueNodes = ixmlNode_getChildNodes (pChildNode);

					if (pNameValueNodes)
					{
						// Yes! Retrieve name and value
						const char *pszSettingName = nullptr;

						IXML_Node *pNameNode = ixmlNodeList_item(pNameValueNodes, 0);

						// Process this node
						IXML_Node *pTextNode;  // temporary node to hold the tag's text value
						pTextNode = ixmlNode_getFirstChild(pNameNode);
						pszSettingName = ixmlNode_getNodeValue (pTextNode);

						// Was there really a name?
						if (nullptr != pszSettingName)
						{
							// Yes! Process it
							SstiSettingItem settingItem;

							settingItem.Name = pszSettingName;

							const char *pszSettingValue = nullptr;
							const char *pszSettingType = nullptr;

							//
							// Get the property value
							//
							IXML_Node *pValueNode = ixmlNodeList_item(pNameValueNodes, 1);
							pTextNode = ixmlNode_getFirstChild(pValueNode);
							pszSettingValue = ixmlNode_getNodeValue(pTextNode);
							if (nullptr == pszSettingValue)
							{
								pszSettingValue = "";
							} // end if

							settingItem.Value = pszSettingValue;

							//
							// Get the property type
							//
							IXML_Node *pTypeNode = ixmlNodeList_item(pNameValueNodes, 2);
							pTextNode = ixmlNode_getFirstChild(pTypeNode);
							pszSettingType = ixmlNode_getNodeValue(pTextNode);
							if (nullptr == pszSettingType)
							{
								pszSettingType = "string";
							} // end if

							if (0 == stiStrICmp(pszSettingType, "int"))
							{
								settingItem.eType = SstiSettingItem::estiInt;
							} // end if
							else if (0 == stiStrICmp(pszSettingType, "bool"))
							{
								settingItem.eType = SstiSettingItem::estiBool;
							} // end else if
							else
							{
								settingItem.eType = SstiSettingItem::estiString;
							} // end else

							settingList.push_back (settingItem);
							
							if (pRequestedSettingsList)
							{
								if (!pRequestedSettingsList->empty())
								{
									for (auto i = pRequestedSettingsList->begin ();
										i != pRequestedSettingsList->end ();)
									{
										if (*i == settingItem.Name)
										{
											i = pRequestedSettingsList->erase (i);
										}
										else
										{
											++i;
										}
									}
								}
							}
						} // end if

						ixmlNodeList_free(pNameValueNodes);
					}
				} // end if

				// Move to the next child node
				y++;
				pChildNode = ixmlNodeList_item (pChildNodes, y);
			} // end while

			poResponse->SettingListSet (settingList);
		} // end if
		
		if (pRequestedSettingsList)
		{
			poResponse->MissingSettingsListSet (*pRequestedSettingsList);
		}
	} // end if

//STI_BAIL:

	return hResult;
} // end CstiCoreService::SettingsGetProcess

/**
 * \brief Retrieves the ScreenSaverList from the core response
 *  <ScreenSaverList>
 *      <ScreenSaverItem>
 *          <SortValue>1</SortValue>
 *          <PublicId>4DE21F65-3894-48C2-A2BA-8D75FB7014B2</PublicId>
 *          <Name>Lake and Dog</Name>
 *          <Description>A day at the lake with my dog</Description>
 *          <Schedule></Schedule>
 *          <ScreenSaverURL>http://akamai/Sorenson/guid1/video</ScreenSaverURL>
 *          <PreviewVideoURL>http://akamai/Sorenson/guid1/preview</PreviewVideoURL>
 *      </ScreenSaverItem>
 *      <ScreenSaverItem>
 *          <SortValue>2</SortValue>
 *          <PublicId>0C1FBCA4-F7C1-4879-8FD8-9FD4B8EAF9EA</PublicId>
 *          <Name>Puppy and Ball</Name>
 *          <Description> A puppy playing with a ball </Description>
 *          <Schedule>{[{"name":"test.mp4","date":"01-01"},{"name":"test2.mp4","date":"02-01"}]</Schedule>
 *          <ScreenSaverURL>http://akamai/Sorenson/guid2/video</ScreenSaverURL>
 *          <PreviewVideoURL>http://akamai/Sorenson/guid2/preview</PreviewVideoURL>
 *      </ScreenSaverItem>
 *  </ScreenSaverList>
 *
 * \param poResponse The response object to process
 * \param pChildNodes The list of IXML nodes to process
 * \return success or failure
 */
stiHResult CstiCoreService::ScreenSaverListProcess (
	CstiCoreResponse *poResponse,
	IXML_Node *pScreenSaverNode)
{
	stiLOG_ENTRY_NAME (CstiCoreService::ScreenSaverListProcess);
	stiHResult hResult = stiRESULT_SUCCESS;

	std::vector <CstiCoreResponse::SScreenSaverInfo> screenSaverInfoList;

	IXML_NodeList *pNodeList = nullptr;
	IXML_Node *pChildNode = nullptr;

	// Get all elements named "MessageListItem"
	pNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pScreenSaverNode, (DOMString)"ScreenSaverItem");

	// Were there any items in the list?
	if (pNodeList != nullptr)
	{
		CstiCoreResponse::SScreenSaverInfo stScreenSaverInfo;

		// Get the first item in the list
		int nListItem = 0;
		pChildNode = ixmlNodeList_item (pNodeList, nListItem);

		// While there are still nodes...
		while (pChildNode)
		{
			// Is this node really a ScreenSaver item?
			if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode), "ScreenSaverItem")))
			{
				// Get the greeting URL nodes
				const char *pszScreenSaverId = SubElementValueGet (pChildNode, "PublicId");
				if (nullptr == pszScreenSaverId)
				{
					pszScreenSaverId = "";
				}

				const char *pszName = SubElementValueGet (pChildNode, "Name");
				if (nullptr == pszName)
				{
					pszName = "";
				}

				const char *pszDescription = SubElementValueGet (pChildNode, "Description");
				if (nullptr == pszDescription)
				{
					pszDescription = "";
				}

				const char *pszSchedule = SubElementValueGet (pChildNode, "Schedule");
				if (nullptr == pszSchedule)
				{
					pszSchedule = "";
				}

				const char *pszPreviewURL = SubElementValueGet (pChildNode, "PreviewVideoURL");
				if (nullptr == pszPreviewURL)
				{
					pszPreviewURL = "";
				}

				const char *pszScreenSaverURL = SubElementValueGet (pChildNode, "ScreenSaverURL");
				if (nullptr == pszScreenSaverURL)
				{
					pszScreenSaverURL = "";
				}

				// Initialize the ScreenSaverInfo members.
				stScreenSaverInfo.screenSaverId = pszScreenSaverId;
				stScreenSaverInfo.name = pszName;
				stScreenSaverInfo.description = pszDescription;
				stScreenSaverInfo.schedule = pszSchedule;
				stScreenSaverInfo.previewVideoUrl = pszPreviewURL;
				stScreenSaverInfo.screenSaverVideoUrl = pszScreenSaverURL;

				// Add this structure to the vector
				screenSaverInfoList.push_back (stScreenSaverInfo);
			} // end if

			// Move to the next child node
			nListItem++;
			pChildNode = ixmlNodeList_item (pNodeList, nListItem);
		} // end while

		if (!screenSaverInfoList.empty ())
		{
			poResponse->ScreenSaverInfoListSet(screenSaverInfoList);
		}

		// Free the node list
		ixmlNodeList_free (pNodeList);
		pNodeList = nullptr;
	} // end if

	return hResult;
}

// end file CstiCoreService.cpp
