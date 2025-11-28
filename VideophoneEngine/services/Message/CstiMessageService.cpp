/*!
 * \file CstiMessgeService.cpp
 * \brief Implementation of the CstiMessageService class.
 *
 * Processes MessageService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//

#include <cctype>	// standard type definitions.
#include <cstring>
#include "ixml.h"
#include "stiOS.h"			// OS Abstraction
#include "CstiMessageService.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiHTTPTask.h"
#include "CstiMessageRequest.h"
#include "CstiCoreService.h"
#include "CstiMessageList.h"
#include "stiCoreRequestDefines.h"

#include "stiTaskInfo.h"

//
// Constants
//
//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-MessageService: %s\n", #name);

static const unsigned int unMAX_TRYS_ON_SERVER = 3;		// Number of times to try the first server
static const unsigned int unBASE_RETRY_TIMEOUT = 10;	// Number of seconds for the first retry attempt
static const unsigned int unMAX_RETRY_TIMEOUT = 7200;	// Max retry timeout is two hours
static const int nMAX_SERVERS = 2;						// The number of servers to try connecting to.

static const NodeTypeMapping g_aNodeTypeMappings[] =
{
	{"MessageInfoGetResult", CstiMessageResponse::eMESSAGE_INFO_GET_RESULT},
	{"MessageViewedResult", CstiMessageResponse::eMESSAGE_VIEWED_RESULT},
	{"MessageListItemEdit", CstiMessageResponse::eMESSAGE_LIST_ITEM_EDIT_RESULT},
	{"MessageListGetResult", CstiMessageResponse::eMESSAGE_LIST_GET_RESULT},
	{"MessageCreateResult", CstiMessageResponse::eMESSAGE_CREATE_RESULT},
	{"MessageDeleteResult", CstiMessageResponse::eMESSAGE_DELETE_RESULT},
	{"MessageDeleteAllResult", CstiMessageResponse::eMESSAGE_DELETE_ALL_RESULT},
	{"SignMailSendResult", CstiMessageResponse::eMESSAGE_SEND_RESULT},
	{"MessageListCountGetResult", CstiMessageResponse::eMESSAGE_LIST_COUNT_GET_RESULT},
	{"MessageDownloadURLGetResult", CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_GET_RESULT},
	{"MessageDownloadURLListGetResult", CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT},
	{"SignMailUploadURLGetResult", CstiMessageResponse::eMESSAGE_UPLOAD_URL_GET_RESULT},
	{"NewMessageCountGetResult", CstiMessageResponse::eNEW_MESSAGE_COUNT_GET_RESULT},
	{"RetrieveMessageKeyResult", CstiMessageResponse::eRETRIEVE_MESSAGE_KEY_RESULT},
	{"GreetingInfoGetResult", CstiMessageResponse::eGREETING_INFO_GET_RESULT},
	{"GreetingUploadURLGetResult", CstiMessageResponse::eGREETING_UPLOAD_URL_GET_RESULT},
	{"GreetingDeleteResult", CstiMessageResponse::eGREETING_DELETE_RESULT},
	{"GreetingEnabledStateSetResult", CstiMessageResponse::eGREETING_ENABLED_STATE_SET_RESULT},
	{"Error", CstiMessageResponse::eRESPONSE_ERROR}
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
//; CstiMessageService::CstiMessageService
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiMessageService::CstiMessageService (
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
		stiMS_MAX_MSG_SIZE,
		MSGRQ_MessageResponse,
		stiMS_TASK_NAME,
		unMAX_TRYS_ON_SERVER,
		unBASE_RETRY_TIMEOUT,
		unMAX_RETRY_TIMEOUT,
		nMAX_SERVERS),
	m_bConnected (estiFALSE),
	m_pCoreService (pCoreService)
{
	stiLOG_ENTRY_NAME (CstiMessageService::CstiMessageService);
	
} // end CstiMessageService::CstiMessageService


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::~CstiMessageService
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	none
//
CstiMessageService::~CstiMessageService ()
{
	stiLOG_ENTRY_NAME (CstiMessageService::~CstiMessageService);

	Cleanup ();
	
} // end CstiMessageService::~CstiMessageService

//:-----------------------------------------------------------------------------
//:	Task functions
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//	This method will call the method that spawns the Message Service task
//	and also will create the message queue that is used to receive messages
//	sent to this task by other tasks.
//
// Returns:
//
stiHResult CstiMessageService::Initialize (
	const std::string *serviceContacts,
	const std::string &uniqueID)
{
	stiLOG_ENTRY_NAME (CstiMessageService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	// NOTE: we don't need to call Cleanup to free up the allocated memory 
	// if the Initialize function

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

	CstiVPService::Initialize (serviceContacts, uniqueID);

	return (hResult);
	
} // end CstiMessageService::Initialize

//:-----------------------------------------------------------------------------
//:	Private methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::StateSet
//
// Description: Sets the current state
//
// Abstract:
//
// Returns:
//	None
//
stiHResult CstiMessageService::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiMessageService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = CstiVPService::StateSet(eState);

	stiTESTRESULT();
	
	// Switch on the new state...
	switch (eState)
	{
		default:
		case eUNITIALIZED:
		{
			stiDEBUG_TOOL (g_stiMSDebug,
				stiTrace ("CstiMessageService::StateSet - Changing state to eUNITIALIZED\n");
			); // stiDEBUG_TOOL

			break;
		} // end case eUNITIALIZED

		case eDISCONNECTED:
		{
			stiDEBUG_TOOL (g_stiMSDebug,
				stiTrace ("CstiMessageService::StateSet - Changing state to eDISCONNECTED\n");
			); // stiDEBUG_TOOL

			m_bConnected = estiFALSE;
			break;
		} // end case eDISCONNECTED

		case eCONNECTING:
		{
			stiDEBUG_TOOL (g_stiMSDebug,
				stiTrace ("CstiMessageService::StateSet - Changing state to eCONNECTING\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_MESSAGE_SERVICE_CONNECTING, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING

		case eCONNECTED:
		{
			stiDEBUG_TOOL (g_stiMSDebug,
				stiTrace ("CstiMessageService::StateSet - Changing state to eCONNECTED\n");
			); // stiDEBUG_TOOL

			if (!m_bConnected)
			{
				hResult = Callback (estiMSG_MESSAGE_SERVICE_CONNECTED, nullptr);

				stiTESTRESULT ();

				// Keep track that we have been connected.
				m_bConnected = estiTRUE;
			} // end if
			else
			{
				// Since we already have been connected, simply notify the application that we are now
				// reconnected.
				hResult = Callback (estiMSG_MESSAGE_SERVICE_RECONNECTED, nullptr);

				stiTESTRESULT ();
			} // end else
			break;
		} // end case eCONNECTED

		case eDNSERROR:
		{
			stiDEBUG_TOOL (g_stiMSDebug,
				stiTrace ("CstiMessageService::StateSet - Changing state to eDNSERROR\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_DNS_ERROR, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING
	} // end switch

STI_BAIL:

	return hResult;
	
} // end CstiMessageService::StateSet


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::RequestRemovedOnError
//
// Description: Notifies the application layer that a request was removed.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiMessageService::RequestRemovedOnError(
	CstiVPServiceRequest *request)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto response = new CstiMessageResponse (request,
											 CstiMessageResponse::eRESPONSE_ERROR,
											 estiERROR,
											 CstiMessageResponse::eMESSAGE_SERVER_NOT_REACHABLE,
											 nullptr);

	stiTESTCOND(response != nullptr, stiRESULT_ERROR);

	// Send the response to the application
	hResult = Callback (estiMSG_MESSAGE_REQUEST_REMOVED, response);

	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiCSDebug,
		stiTrace ("CstiMessageService::RequestRemovedOnError - "
			"Removing request id: %d\n",
			request->requestIdGet());
	); // stiDEBUG_TOOL

STI_BAIL:

	return hResult;
}


stiHResult CstiMessageService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (nSystemicError == 0)
	{
		nSystemicError = CstiMessageResponse::eUNKNOWN_ERROR;
	}
	
	// Create a new Core Response
	auto response = new CstiMessageResponse (
		pRequest,
		(CstiMessageResponse::EResponse)nResponse,
		estiERROR,
		(CstiMessageResponse::EResponseError)nSystemicError,
		nullptr);
	stiTESTCOND (response != nullptr, stiRESULT_ERROR);
	
	Callback (estiMSG_MESSAGE_RESPONSE, response);

STI_BAIL:

	return hResult;
}


CstiMessageResponse::EResponseError CstiMessageService::NodeErrorGet (
	IXML_Node *pNode,
	EstiBool *pbDatabaseAvailable)
{
	CstiMessageResponse::EResponseError eError = CstiMessageResponse::eNO_ERROR;
	
	// Determine the error
	const char *pszErrNum = AttributeValueGet (pNode, "ErrorNum");

	if ((nullptr != pszErrNum))
	{
		eError = (CstiMessageResponse::EResponseError)atoi (pszErrNum);

		if (eError == CstiMessageResponse::eNOT_LOGGED_IN)
		{
			// Did we receive a database connection error?
			if (estiFALSE == *pbDatabaseAvailable)
			{
				// Yes, change the error code to be that database error code
				eError = CstiMessageResponse::eDATABASE_CONNECTION_ERROR;
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
				eError = CstiMessageResponse::eXML_VALIDATION_ERROR;
			} // end if
			else if (strcmp (pszErrorType, "XML Format") == 0)
			{
				eError = CstiMessageResponse::eXML_FORMAT_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Generic") == 0)
			{
				eError = CstiMessageResponse::eGENERIC_SERVER_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Database Connection") == 0)
			{
				eError = CstiMessageResponse::eDATABASE_CONNECTION_ERROR;
				// Set flag so any "not logged in" errors in this response will be ignored
				*pbDatabaseAvailable = estiFALSE;
			} // end else if
			else
			{
				eError = CstiMessageResponse::eUNKNOWN_ERROR;
			} // end else
		}
	}
	
	return eError;
}


stiHResult CstiMessageService::LoggedOutErrorProcess (
	CstiVPServiceRequest *pRequest,
	EstiBool * pbLoggedOutNotified)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!*pbLoggedOutNotified)
	{
		// Log back in to core services
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
					m_pCoreService->LoggedOutNotifyAsync(currentToken);

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
//; CstiMessageService::ResponseNodeProcess
//
// Description: Parses the indicated node
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiMessageService::ResponseNodeProcess (
	IXML_Node * pNode,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiBool * pbDatabaseAvailable,
	EstiBool * pbLoggedOutNotified,
	int *pnSystemicError,
	EstiRequestRepeat *peRepeatRequest)
{
	stiLOG_ENTRY_NAME (CstiMessageService::ResponseNodeProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Remove this request, unless otherwise instructed.
	*peRepeatRequest = estiRR_NO;

	// Response Object pointer
	CstiMessageResponse * poResponse = nullptr;

	IXML_NodeList * pChildNodes = nullptr;
	IXML_NamedNodeMap * pAttributes = nullptr;
	unsigned long ulAttributeCount = 0; stiUNUSED_ARG (ulAttributeCount);

	stiDEBUG_TOOL (g_stiMSDebug,
		stiTrace ("CstiMessageService::ResponseNodeProcess - Node Name: %s\n",
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
	auto  eResponse = (CstiMessageResponse::EResponse)NodeTypeDetermine (
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
		// Yes! Create a new response
		poResponse = new CstiMessageResponse (
				pRequest,
				eResponse,
				estiOK,
				CstiMessageResponse::eNO_ERROR,
				nullptr);

		stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);
		
		// Yes! Handle the response
		switch (eResponse)
		{
			case CstiMessageResponse::eMESSAGE_INFO_GET_RESULT:
			{
				IXML_Node *pSubNode = SubElementGet(pNode, "MessageInfo");
			
				const char * pszGUID = SubElementValueGet (pSubNode, "Id");

				const char * pszServer = SubElementValueGet (pSubNode, "Server");

				const char * pszRetrievalMethod = SubElementValueGet (pSubNode, "Method");

				// Save the attribute values to associated member strings
				//
				poResponse->ResponseStringSet (
					CstiMessageResponse::eRESPONSE_STRING_GUID,
					pszGUID);
				poResponse->ResponseStringSet (
					CstiMessageResponse::eRESPONSE_STRING_SERVER,
					pszServer);
				poResponse->ResponseStringSet (
					CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD,
					pszRetrievalMethod);
				poResponse->clientTokenSet(clientTokenGet());

				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eMESSAGE_INFO_GET_RESULT

			case CstiMessageResponse::eMESSAGE_CREATE_RESULT:
			{
				const char * pszGUID = SubElementValueGet (pNode, "Id");

				const char * pszServer = SubElementValueGet (pNode, "Server");

				// Save the attribute values to associated member strings
				//
				poResponse->ResponseStringSet (
					CstiMessageResponse::eRESPONSE_STRING_GUID,
					pszGUID);
				poResponse->ResponseStringSet (
					CstiMessageResponse::eRESPONSE_STRING_SERVER,
					pszServer);

				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eMESSAGE_CREATE_RESULT

			case CstiMessageResponse::eMESSAGE_LIST_GET_RESULT:
			{
				// Is there a message list in the result?
				if (pChildNodes != nullptr)
				{
					// Yes!
					IXML_Node * pChildNode = nullptr;

					// Loop through the Call Lists, and process them.
					pChildNode = ixmlNodeList_item (pChildNodes, 0);
						
					// Process this node
					hResult = MessageListProcess (poResponse, pChildNode);
					stiTESTRESULT ();

					// Send the response to the application
					hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

					stiTESTRESULT ();

					// We no longer own this response, so clear it.
					poResponse = nullptr;

				} // end if

				break;
			} // end case eMESSAGE_LIST_GET

			case CstiMessageResponse::eMESSAGE_LIST_COUNT_GET_RESULT:
			{
				const char * pszCount = AttributeValueGet (pNode, "Count");
				unsigned int unCount = atoi (pszCount);
				poResponse->ResponseValueSet (unCount);
					
				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eMESSAGE_LIST_COUNT_GET_RESULT


			case CstiMessageResponse::eNEW_MESSAGE_COUNT_GET_RESULT:
			{
				const char * pszCount = AttributeValueGet (pNode, "Count");
				unsigned int unCount = atoi (pszCount);
				poResponse->ResponseValueSet (unCount);
					
				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eNEW_MESSAGE_COUNT_GET_RESULT
			case CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_GET_RESULT:
			{
				const char *pszDownloadURL = SubElementValueGet (pNode, "DownloadURL");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_GUID, pszDownloadURL);
				
				const char *pszViewId = SubElementValueGet (pNode, "ViewId");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD, pszViewId);

				const char *pszFileSize = SubElementValueGet (pNode, "FileSize");
				if (pszFileSize)
				{
					unsigned int unFileSize = atoi(pszFileSize);
					poResponse->ResponseValueSet(unFileSize);
				}
				poResponse->clientTokenSet(clientTokenGet());

				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eMESSAGE_DOWNLOAD_URL_GET_RESULT
			case CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT:
			{
				// Process this node
				hResult = MessageDownloadUrlListProcess (poResponse, pNode);
				stiTESTRESULT ();

				// Send the response to the application
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			} // end case eMESSAGE_DOWNLOAD_URL_GET_RESULT
			case CstiMessageResponse::eRETRIEVE_MESSAGE_KEY_RESULT:
			{
				const char * pszMessageKey = AttributeValueGet (pNode, "MessageURL");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_GUID, pszMessageKey);
					
				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eRETRIEVE_MESSAGE_KEY_RESULT
			case CstiMessageResponse::eMESSAGE_UPLOAD_URL_GET_RESULT:
			{
				const char *pszUploadURL = SubElementValueGet (pNode, "UploadURL");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_GUID, pszUploadURL);
				
				const char *pszMailBoxFull = SubElementValueGet (pNode, "MailboxFull");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_SERVER, pszMailBoxFull);

				poResponse->clientTokenSet(clientTokenGet());

				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eNEW_MESSAGE_COUNT_GET_RESULT

			case CstiMessageResponse::eMESSAGE_VIEWED_RESULT:
			case CstiMessageResponse::eMESSAGE_LIST_ITEM_EDIT_RESULT:
			{
				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eMESSAGE_LIST_COUNT_GET_RESULT
			case CstiMessageResponse::eGREETING_INFO_GET_RESULT:
			{
				//   <GreetingInfoGetResult MaxRecordSeconds="30" GreetingPreference="Personal" Result="OK">
				//     <Greeting Type="Default">
				//        <GreetingViewURL>http://sorensongreetings.akamai.com/greetings/?greetingID=00000000--000000000100</GreetingViewURL>
				//        <GreetingImageURL>http://sorensongreetings.akamai2.com/greetings/?greetingID=00000000--000000000100</GreetingImageURL>
				//     </Greeting>
				// 
				//     <Greeting Type="Personal">
				//       <GreetingViewURL>http://sorenson.com/greetings/?greetingID=C4E63C88-538D-412B-AC1F-090C2FC253BC</GreetingViewURL>
				//       <GreetingImageURL>http://sorenson2.com/greetings/?greetingID=C4E63C88-538D-412B-AC1F-090C2FC253BC</GreetingImageURL>
				//     </Greeting>
				//   </GreetingInfoGetResult>

				const char * pszMaxSeconds = AttributeValueGet (pNode, "MaxRecordSeconds");
				
				int nMaxSeconds = 0;
				if (pszMaxSeconds != nullptr)
				{
					sscanf (pszMaxSeconds, "%d", &nMaxSeconds);
				} // end if

				hResult = GreetingInfoGetProcess(poResponse, pChildNodes, nMaxSeconds);

				stiTESTRESULT ();

				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();
				
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				
				break;
			}
			case CstiMessageResponse::eGREETING_UPLOAD_URL_GET_RESULT:
			{
//				<GreetingUploadURLGetResult>
//				  <GreetingUploadURL>http://10.10.10.10/SignMailGreetingUpload/Upload.aspx?id=00000000-0000-0000-0000-000000000000</GreetingUploadURL>
//				</ GreetingUploadURLGetResult>

				auto greetingUploadURLRaw = SubElementValueGet (pNode, "GreetingUploadURL");
				if (nullptr == greetingUploadURLRaw)
				{
					greetingUploadURLRaw = "";
				}

				//Switching to use HTTPS instead of HTTP
				std::string greetingUploadURL{ greetingUploadURLRaw };
				protocolSchemeChange (greetingUploadURL, "https");
				poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_GUID, greetingUploadURL.c_str());
				poResponse->clientTokenSet(clientTokenGet());
				
				// Notify the app
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();
				
				// We no longer own this response, so clear it.
				poResponse = nullptr;

				break;
			}
			case CstiMessageResponse::eGREETING_ENABLED_STATE_SET_RESULT:
			{
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);
				stiTESTRESULT ();
				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}
			default:
			case CstiMessageResponse::eMESSAGE_DELETE_RESULT:
			case CstiMessageResponse::eMESSAGE_DELETE_ALL_RESULT:
			case CstiMessageResponse::eMESSAGE_SEND_RESULT:
			case CstiMessageResponse::eRESPONSE_UNKNOWN:
			{
				hResult = Callback (estiMSG_MESSAGE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eRESPONSE_UNKNOWN
		} // end switch
	} // end if
	else
	{
		CstiMessageResponse::EResponseError eErrorNum = CstiMessageResponse::eUNKNOWN_ERROR;

		// No! Did we got a systemic error?
		if (eResponse == CstiMessageResponse::eRESPONSE_ERROR)
		{
			*pnSystemicError = NodeErrorGet (pNode, pbDatabaseAvailable);

			if (eErrorNum == CstiMessageResponse::eNOT_LOGGED_IN)
			{
				LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);
			}
			else
			{
				hResult = errorResponseSend(pRequest,
											eResponse,
											(CstiMessageResponse::EResponseError)*pnSystemicError,
											ElementValueGet(pNode));
				stiTESTRESULT();
			}
		}
		else
		{
			eErrorNum = NodeErrorGet (pNode, pbDatabaseAvailable);
			
			if (eErrorNum != CstiMessageResponse::eNO_ERROR)
			{
				// Handle errors that can affect any of the messages first
				switch (eErrorNum)
				{
					case CstiMessageResponse::eNOT_LOGGED_IN: //case 1: // Not Logged in
					{
						LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);

						if (*pbLoggedOutNotified)
						{
							if (pRequest)
							{
								if (pRequest->AuthenticationAttemptsGet() <= 1)
								{
									//
									// Don't remove this request, since we want to
									// try it again after logging in.
									//
									*peRepeatRequest = estiRR_DELAYED;
								}
								else if (!pRequest->RemoveUponCommunicationError ())
								{
									*peRepeatRequest = estiRR_DELAYED;
								}
							}
						}

						//
						// If we are going to remove the request then
						// tell the application otherwise don't because we
						// are going to send it again.
						//
						if (*peRepeatRequest == estiRR_NO)
						{
							hResult = errorResponseSend(pRequest,
														eResponse,
														eErrorNum,
														ElementValueGet(pNode));
							stiTESTRESULT();
						}

						break;
					} // end case 1
					case CstiMessageResponse::eSQL_SERVER_NOT_REACHABLE: // case 39
					case CstiMessageResponse::eMESSAGE_SERVER_NOT_REACHABLE: // case 14
					default:
					{
						hResult = errorResponseSend(pRequest,
													eResponse,
													eErrorNum,
													ElementValueGet(pNode));
						stiTESTRESULT();
						break;
					}
					
				} // end switch
			} // end if
			else
			{
				hResult = errorResponseSend(pRequest,
											eResponse,
											(CstiMessageResponse::EResponseError)*pnSystemicError,
											ElementValueGet(pNode));
				stiTESTRESULT();
			}
		}
	} // end else

STI_BAIL:

	// Do we still have a MessageResponse object hanging around?
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
} // end CstiMessageService::ResponseNodeProcess


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::MessageListProcess
//
// Description: Processes a MessageList from the response
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiMessageService::MessageListProcess (
	CstiMessageResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiMessageService::MessageListProcess);

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
	auto  poMessageList = new CstiMessageList ();

	// Did we get one?
	if (poMessageList != nullptr)
	{
		//
		// Set count.
		// This is for a list count get request
		// There are actually no items to add to this list
		// but the count is stored in a list structure
		// this way to pass to the application.
		//
		poMessageList->CountSet (unCount);
		
		// Set the base index and count
		const char * szBaseIndex = AttributeValueGet (pNode, ATT_BaseIndex);
		unsigned int unBaseIndex = 0;
		
		if (szBaseIndex != nullptr)
		{
			sscanf (szBaseIndex, "%ud", &unBaseIndex);
		} // end if
		poMessageList->BaseIndexSet (unBaseIndex);

		// Get the count of SignMail Messages and set it on the list
		const char * szSignMailCount = AttributeValueGet (pNode, ATT_SignMailCount);
		unsigned int unSignMailCount = 0;
		if (szSignMailCount != nullptr)
		{
			sscanf (szSignMailCount, "%ud", &unSignMailCount);
		} // end if
		poMessageList->SignMailMsgCountSet(unSignMailCount);

		// Get the SignMail Message Limit and set it on the list
		const char * szSignMailLimit = AttributeValueGet (pNode, ATT_SignMailLimit);
		unsigned int unSignMailLimit = 0;
		if (szSignMailLimit != nullptr)
		{
			sscanf (szSignMailLimit, "%ud", &unSignMailLimit);
		} // end if
		poMessageList->SignMailMsgLimitSet(unSignMailLimit);
		
		// Get all elements named "MessageListItem"
		pNodeList = ixmlElement_getElementsByTagName (
			(IXML_Element *)pNode, (DOMString)"MessageListItem");

		// Did we get the list of items?
		if (pNodeList != nullptr)
		{

			// Get the first item in the list
			int nListItem = 0;
			pTempNode = ixmlNodeList_item (pNodeList, nListItem);

			// While there are still nodes...
			while (pTempNode)
			{
				// get the ItemId value from this CallListItem
				const char * pszItemId = SubElementValueGet (pTempNode, TAG_ItemId);

				// Get the dial string
				const char * pszDialString = SubElementValueGet (pTempNode, VAL_DialString);
				if (nullptr == pszDialString)
				{
					pszDialString = "";
				} // end if

				// Retrieve the dial type
				int nDialType = 0;
				const char * pszDialType = SubElementValueGet (pTempNode, VAL_DialType);
				if (pszDialType != nullptr)
				{
					nDialType = atoi (pszDialType);
				} // end if

				// Get and Convert the the Viewed field
				EstiBool bViewed = estiFALSE;
				const char * pszViewed = SubElementValueGet (pTempNode, "Viewed");
				if (nullptr != pszViewed && !stiStrICmp(pszViewed, "True"))
				{
					bViewed = estiTRUE;
				} // end if

				// Get and Convert the the Time
				time_t ttTime = 0;
				const char *pszTime = SubElementValueGet (pTempNode, VAL_Time);
				if (nullptr != pszTime)
				{
					hResult = TimeConvert (pszTime, &ttTime);
					stiTESTRESULT ();
				} // end if

				// Get the name
				const char * pszName = SubElementValueGet (pTempNode, VAL_Name);
				if (nullptr == pszName)
				{
					pszName = "";
				} // end if

				int messageTypeId = 0;
				const char * pMessageTypeId = SubElementValueGet (
					pTempNode, "MessageTypeId");
				if (pMessageTypeId != nullptr)
				{
					messageTypeId = atoi (pMessageTypeId);
				} // end if

				int nMessageLength = 0;
				const char *pszMessageLength = SubElementValueGet (
					pTempNode, "MessageLength");
				if (pszMessageLength)
				{
					nMessageLength = atoi (pszMessageLength);
				}

				int nFileSize = 0;
				const char *pszFileSize = SubElementValueGet (
					pTempNode, "FileSize");
				if (pszFileSize)
				{
					nFileSize = atoi (pszFileSize);
				}

				uint32_t un32PausePoint = 0;
				const char *pszPausePoint = SubElementValueGet (
					pTempNode, "PlaybackTime");
				if (pszPausePoint)
				{
					un32PausePoint = atoi (pszPausePoint);
				}

				// Get the InterpreterId string
				const char * pszInterpreterId = SubElementValueGet (pTempNode, "InterpreterId");
				if (nullptr == pszInterpreterId)
				{
					pszInterpreterId = "";
				} // end if

				// Get the Preivew Image URL
				const char *pszPreviewImageURL = SubElementValueGet(pTempNode, "PreviewImageDownloadURL");
				if (nullptr == pszPreviewImageURL)
				{
					pszPreviewImageURL = "";
				}

				// Get and Convert the the is featured field
				EstiBool bIsFeatured = estiFALSE;
				const char * pszIsFeatured = SubElementValueGet(pTempNode, "IsFeaturedVideo");
				if (nullptr != pszIsFeatured && !stiStrICmp(pszIsFeatured, "True"))
				{
					bIsFeatured = estiTRUE;
				} // end if

				// Get the featured video position
				uint32_t un32FeaturedPos = 0;
				const char *pszFeaturedPos = SubElementValueGet(pTempNode, "FeaturedVideoPosition");
				if (pszFeaturedPos)
				{
					un32FeaturedPos = atoi (pszFeaturedPos);
				}

				// Get and Convert the airdate 
				time_t ttAirDate = 0;
				const char *pszAirDate = SubElementValueGet (pTempNode, "AirDate");
				if (nullptr != pszAirDate)
				{
					hResult = TimeConvert (pszAirDate, &ttAirDate);
					stiTESTRESULT ();
				} // end if

				// Get and Convert the ExpirationDate
				time_t ttExpirationDate = 0;
				const char *pszExpirationDate = SubElementValueGet (pTempNode, "ExpireDate");
				if (nullptr != pszExpirationDate)
				{
					hResult = TimeConvert (pszExpirationDate, &ttExpirationDate);
					stiTESTRESULT ();
				} // end if

				// Get the dial string
				const char * pszDescString = SubElementValueGet (pTempNode, "Description");
				if (nullptr == pszDescString)
				{
					pszDescString = "";
				} // end if

				auto pMessageListItem = std::make_shared<CstiMessageListItem> ();
				
				pMessageListItem->ItemIdSet (pszItemId);
				pMessageListItem->DialStringSet (pszDialString);
				pMessageListItem->DialMethodSet ((EstiDialMethod)nDialType);
				pMessageListItem->NameSet (pszName);
				pMessageListItem->CallTimeSet (ttTime);
				pMessageListItem->ViewedSet (bViewed);
				pMessageListItem->MessageTypeIdSet ((EstiMessageType)messageTypeId);
				pMessageListItem->DurationSet (nMessageLength);
				pMessageListItem->FileSizeSet(nFileSize);
				pMessageListItem->PausePointSet(un32PausePoint);
				pMessageListItem->InterpreterIdSet(pszInterpreterId);
				pMessageListItem->PreviewImageURLSet(pszPreviewImageURL);
				pMessageListItem->FeaturedVideoSet(bIsFeatured);
				pMessageListItem->FeaturedVideoPosSet(un32FeaturedPos);
				pMessageListItem->AirDateSet(ttAirDate);
				pMessageListItem->ExpirationDateSet(ttExpirationDate);
				pMessageListItem->DescriptionSet(pszDescString);

				poMessageList->ItemAdd (pMessageListItem);
				
				// Get the next MessageResponse node
				nListItem++;
				pTempNode = ixmlNodeList_item (pNodeList, nListItem);
			} // end while

			// Free the node list
			ixmlNodeList_free (pNodeList);
		} // end if

		// Add the call list to the response
		poResponse->MessageListSet (poMessageList);
	} // end if
	else
	{
		// No! Log an error
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (poMessageList)
		{
			delete poMessageList;
			poMessageList = nullptr;
		}
	}
	
	return (hResult);
} // end CstiMessageService::MessageListProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::MessageDownloadUrlListProcess
//
// Description: Processes a MessageDownloadURLList from the response
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure result
//<MessageDownloadURLListGetResult Result="OK">
//  <ViewId>12977874</ViewId>
//  <MessageList>
//    <MessageItem>
//      <DownloadURL>The URL for this file</DownloadURL>
//      <FileSize>115918</FileSize>
//      <MaxBitRate>256000</MaxBitRate >
//      <Width>352</Width>
//      <Height>288</Height>
//      <Codec>H264</Codec>
//      <Profile>Baseline</Profile>
//      <Level>1.2</Level>
//	  </MessageItem>
//    <MessageItem>
//...
//    </MessageItem>
//  </MessageList>
//</MessageDownloadURLListGetResult>
//
stiHResult CstiMessageService::MessageDownloadUrlListProcess (
	CstiMessageResponse * poResponse,
	IXML_Node * pNode)
{
	stiLOG_ENTRY_NAME (CstiMessageService::MessageDownloadUrlListProcess);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::vector <CstiMessageResponse::CstiMessageDownloadURLItem> messageDownloadURLItems;
	
	IXML_NodeList * pNodeList = nullptr;
	IXML_Node * pTempNode = nullptr;
	
	if (pNode != nullptr)
	{
		const char * pszViewId = SubElementValueGet (pNode, "ViewId");
		if (pszViewId != nullptr)
		{
			poResponse->ResponseStringSet (CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD, pszViewId);
			//TODO:	poMessageDownloadUrlList->ViewIdSet (unViewId);
		}
		
		// Get all elements named "MessageItem"
		pNodeList = ixmlElement_getElementsByTagName ((IXML_Element *)pNode, (DOMString)"MessageItem");

		// Did we get the list of items?
		if (pNodeList != nullptr)
		{
			CstiMessageResponse::CstiMessageDownloadURLItem urlItem;
			// Get the first item in the list
			int nListItem = 0;
			pTempNode = ixmlNodeList_item (pNodeList, nListItem);
			
			// While there are still nodes...
			while (pTempNode)
			{
				const char * pszDownloadURL = SubElementValueGet (pTempNode, "DownloadURL");
				urlItem.m_DownloadURL = pszDownloadURL;
				
				const char *pszFileSize = SubElementValueGet (pTempNode, "FileSize");
				if (pszFileSize)
				{
					urlItem.m_nFileSize = atoi (pszFileSize);
				}
				
				const char *pszMaxBitRate = SubElementValueGet (pTempNode, "MaxBitRate");
				if (pszMaxBitRate)
				{
					urlItem.m_nMaxBitRate = atoi (pszMaxBitRate);
				}
				
				const char *pszWidth = SubElementValueGet (pTempNode, "Width");
				if (pszWidth)
				{
					urlItem.m_nWidth = atoi (pszWidth);
				}

				const char *pszHeight = SubElementValueGet (pTempNode, "Height");
				if (pszHeight)
				{
					urlItem.m_nHeight = atoi (pszHeight);
				}
				
				const char * pszCodec = SubElementValueGet (pTempNode, "Codec");
				if (pszCodec)
				{
					urlItem.m_Codec = pszCodec;
				}
				
				const char * pszProfile = SubElementValueGet (pTempNode, "Profile");
				if (pszProfile)
				{
					urlItem.m_Profile = pszProfile;
				}
				
				const char * pszLevel = SubElementValueGet (pTempNode, "Level");
				if (pszLevel)
				{
					urlItem.m_Level = pszLevel;
				}
				messageDownloadURLItems.push_back (urlItem);
				// Get the next MessageItem node
				nListItem++;
				pTempNode = ixmlNodeList_item (pNodeList, nListItem);
			} // end while
			
			// Free the node list
			ixmlNodeList_free (pNodeList);
		}
		
		poResponse->MessageDownloadUrlListSet (messageDownloadURLItems);
	}
	else
	{
		// No! Log an error
		stiTHROW (stiRESULT_ERROR);
	}
	
STI_BAIL:
	
//	if (stiIS_ERROR (hResult))
//	{
//		if (poMessageDownloadUrlList)
//		{
//			delete poMessageDownloadUrlList;
//			poMessageDownloadUrlList = NULL;
//		}
//	}
	
	return (hResult);
} // end CstiMessageService::MessageDownloadUrlListProcess

////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::Restart
//
// Description:
//	Restart the Message  Services by initializing the member variables
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - if there is no eror. Otherwize, some errors occur.
//
stiHResult CstiMessageService::Restart ()
{
	stiLOG_ENTRY_NAME (CstiMessageService::Restart);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiVPService::Restart();

	stiTESTRESULT();
	
	m_bConnected = estiFALSE;

STI_BAIL:

	return (hResult);

}

CstiCoreService* CstiMessageService::coreServiceGet ()
{
	return m_pCoreService;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::RequestSend
//
// Description: Sends a request to the Core Service
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued.
//
// Deprecated: This method should not be used.  Use RequestSendEx instead.  The difference is that
// RequestSendEx returns the caller back to  a known state because it does not delete the poMessageRequest.
// This matches the coding standard defined in the "C++ Coding Standards" book, chapter 71.
//
EstiResult CstiMessageService::RequestSend (
	CstiMessageRequest * poMessageRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiMessageService::RequestSend);

	unsigned int unRequestId;
	EstiResult eResult = estiERROR;

	// Were we given a valid request?
	if (poMessageRequest != nullptr)
	{
		// Yes; First, check if we are logged in
		if (m_pCoreService->StateGet () != eAUTHENTICATED)
		{
			// We are not logged in!
			// If we aren't already trying to log in, we need to log in!
			if (m_pCoreService->StateGet () != eCONNECTING)
			{
//				m_pCoreService->ReauthenticateNotifyAsync();
			}
			
			//  Delete the request and then skip the rest of the function
			delete poMessageRequest;
		}
		else
		{
			std::lock_guard<std::recursive_mutex> lock (m_execMutex);

			// We have a valid request, and we are properly authenticated... send the request
			// Set the request number
			unRequestId = NextRequestIdGet ();
			poMessageRequest->requestIdSet (unRequestId);
	
			//
			// If the calling function passed in a non-null pointer
			// for request id then return the request id through
			// that parameter.
			//
			if (nullptr != punRequestId)
			{
				*punRequestId = unRequestId;
			}

			stiHResult hResult = RequestQueue (poMessageRequest, 500);
			
			if (stiIS_ERROR (hResult))
			{
				eResult = estiERROR;
				delete poMessageRequest;
			}
			else
			{
				eResult = estiOK;
			}
		}
	} // end if
	else
	{
		stiASSERT (estiFALSE);
	} // end else

	return (eResult);
} // end CstiMessageService::RequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::RequestSendEx
//
// Description: Sends a request to the Core Service
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued.
//
stiHResult CstiMessageService::RequestSendEx (
	CstiMessageRequest * poMessageRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiMessageService::RequestSend);
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unRequestId;

	stiTESTCOND (poMessageRequest, stiRESULT_ERROR);

	// Yes; First, check if we are logged in
	if (m_pCoreService->StateGet () != eAUTHENTICATED)
	{
		// We are not logged in!
		// If we aren't already trying to log in, we need to log in!
		if (m_pCoreService->StateGet () != eCONNECTING)
		{
//			m_pCoreService->ReauthenticateNotifyAsync();
		}

		stiTHROW (stiRESULT_ERROR);
	}
	else
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poMessageRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		hResult = RequestQueue (poMessageRequest, 500);
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiMessageService::RequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageService::SSLFailedOver
//
// Description: Called by the base class to inform the derived class
// that SSL has failed over.
//
// Abstract:
//
// Returns:
//	None
//
void CstiMessageService::SSLFailedOver()
{
	Callback (estiMSG_MESSAGE_SERVICE_SSL_FAILOVER, nullptr);
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
EstiBool CstiMessageService::IsDebugTraceSet (const char **ppszService) const
{
	EstiBool bRet = estiFALSE;
	
	if (g_stiMSDebug)
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
 * \param ppszService Returns the service type string ("MS" in this case).
 */
const char * CstiMessageService::ServiceIDGet () const
{
	return "MS";
}


//   <GreetingInfoGetResult MaxRecordSeconds="30" GreetingPreference="Personal" Result="OK">
//     <Greeting Type="Default">
//        <GreetingViewURL>http://sorensongreetings.akamai.com/greetings/?greetingID=00000000--000000000100</GreetingViewURL>
//        <GreetingImageURL>http://sorensongreetings.akamai2.com/greetings/?greetingID=00000000--000000000100</GreetingImageURL>
//     </Greeting>
// 
//     <Greeting Type="Personal">
//       <GreetingViewURL>http://sorenson.com/greetings/?greetingID=C4E63C88-538D-412B-AC1F-090C2FC253BC</GreetingViewURL>
//       <GreetingImageURL>http://sorenson2.com/greetings/?greetingID=C4E63C88-538D-412B-AC1F-090C2FC253BC</GreetingImageURL>
//     </Greeting>
//   </GreetingInfoGetResult>

stiHResult CstiMessageService::GreetingInfoGetProcess (
	CstiMessageResponse * poResponse,
	IXML_NodeList * pGreetingNodes,
	int nMaxSeconds)
{

	stiLOG_ENTRY_NAME (CstiCoreService::GreetingInfoGetProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::vector <CstiMessageResponse::SGreetingInfo> GreetingInfoList;

	int y = 0;
	IXML_Node * pChildNode = nullptr;

	// pGreetingNodes holds all elements named "Greeting"
	// Get the count of items in the list
	unsigned long ulNodeCount = ixmlNodeList_length (pGreetingNodes);

	// Were there any items in the list?
	if (ulNodeCount > 0)
	{
		CstiMessageResponse::SGreetingInfo stGreetingInfo;

		// Loop through the items, and process them.
		pChildNode = ixmlNodeList_item (pGreetingNodes, y);
		while (pChildNode)
		{
			// Is this node really a Greeting item?
			if ((0 == stiStrICmp (ixmlNode_getNodeName (pChildNode), "Greeting")))
			{
				// Get the Type attribute value from Greeting tag
				const char *pszType = AttributeValueGet (pChildNode, "Type");
				
				CstiMessageResponse::EGreetingType eType =
					(0 == stiStrICmp (pszType, "Default")) ? CstiMessageResponse::eGREETING_TYPE_DEFAULT :
					((0 == stiStrICmp (pszType, "Personal")) ? CstiMessageResponse::eGREETING_TYPE_PERSONAL :
					CstiMessageResponse::eGREETING_TYPE_UNKNOWN);
				stGreetingInfo.eGreetingType = eType;

				// Set the max greeting length (maxSeconds)
				stGreetingInfo.nMaxSeconds = nMaxSeconds;

				// Get the greeting URL nodes
                const char *pszViewUrl = SubElementValueGet (pChildNode, "GreetingViewURL");
                const char *pszImageUrl = SubElementValueGet (pChildNode, "GreetingImageURL");

				if (nullptr == pszViewUrl)
				{
					pszViewUrl = "";
				}

				//Switching to use HTTPS instead of HTTP
				std::string greetingViewURL (pszViewUrl);
				protocolSchemeChange (greetingViewURL, "https");

				// Add the Url1
				stGreetingInfo.Url1 = greetingViewURL;

				if (nullptr == pszImageUrl)
				{
					pszImageUrl = "";
				}
				// Add the Url1
				stGreetingInfo.GreetingImageUrl = pszImageUrl;

				// Add this structure to the vector
				GreetingInfoList.push_back (stGreetingInfo);
			} // end if

			// Move to the next child node
			y++;
			pChildNode = ixmlNodeList_item (pGreetingNodes, y);
		} // end while

		if (!GreetingInfoList.empty ())
		{
			poResponse->GreetingInfoListSet (GreetingInfoList);
		}
	} // end if

	return hResult;

}				

stiHResult CstiMessageService::errorResponseSend (
	CstiVPServiceRequest *request,
	CstiMessageResponse::EResponse response,
	CstiMessageResponse::EResponseError errResponse,
	const char *xmlError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Create the error response and send it.
	auto msgResponse = new CstiMessageResponse (request,
												response,
												estiERROR,
												errResponse,
												xmlError);
	stiTESTCOND (msgResponse != nullptr, stiRESULT_ERROR);

	// Notify the application of the error.
	hResult = Callback(estiMSG_MESSAGE_RESPONSE, msgResponse);
	stiTESTRESULT();

	// We no longer own this response, so clear it.
	msgResponse = nullptr;

STI_BAIL:

	if (msgResponse != nullptr)
	{
		msgResponse->destroy ();
		msgResponse = nullptr;
	}

	return hResult;
}

// end file CstiMessageService.cpp
