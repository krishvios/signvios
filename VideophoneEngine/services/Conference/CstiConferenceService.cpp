/*!
 * \file CstiConferenceService.cpp
 * \brief Implementation of the CstiConferenceService class.
 *
 * Processes ConferenceService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2014 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//

#include <cctype>	// standard type definitions.
#include <cstring>
#include "ixml.h"
#include "stiOS.h"			// OS Abstraction
#include "CstiConferenceService.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiHTTPTask.h"
#include "CstiConferenceRequest.h"
#include "CstiCoreService.h"
#include "stiCoreRequestDefines.h"

#include "stiTaskInfo.h"

//
// Constants
//
//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-ConferenceService: %s\n", #name);

static const unsigned int unMAX_TRYS_ON_SERVER = 3;		// Number of times to try the first server
static const unsigned int unBASE_RETRY_TIMEOUT = 10;	// Number of seconds for the first retry attempt
static const unsigned int unMAX_RETRY_TIMEOUT = 7200;	// Max retry timeout is two hours
static const int nMAX_SERVERS = 2;						// The number of servers to try connecting to.

static const NodeTypeMapping g_aNodeTypeMappings[] =
{
	{"ConferenceRoomCreateResult", CstiConferenceResponse::eCONFERENCE_ROOM_CREATE_RESULT},
	{"ConferenceRoomStatusGetResult", CstiConferenceResponse::eCONFERENCE_ROOM_STATUS_GET_RESULT},
	{"Error", CstiConferenceResponse::eRESPONSE_ERROR}
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
//; CstiConferenceService::CstiConferenceService
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiConferenceService::CstiConferenceService (
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
		stiGENERIC_MAX_MSG_SIZE,
		CNFRQ_ConferenceResponse,
		stiCNF_TASK_NAME,
		unMAX_TRYS_ON_SERVER,
		unBASE_RETRY_TIMEOUT,
		unMAX_RETRY_TIMEOUT,
		nMAX_SERVERS),
	m_bConnected (estiFALSE),
	m_pCoreService (pCoreService)
{
	stiLOG_ENTRY_NAME (CstiConferenceService::CstiConferenceService);
	
} // end CstiConferenceService::CstiConferenceService


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::~CstiConferenceService
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	none
//
CstiConferenceService::~CstiConferenceService ()
{
	stiLOG_ENTRY_NAME (CstiConferenceService::~CstiConferenceService);

	Cleanup ();
	
} // end CstiConferenceService::~CstiConferenceService

//:-----------------------------------------------------------------------------
//:	Task functions
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//	This method will call the method that spawns the Conference Service task
//	and also will create the message queue that is used to receive messages
//	sent to this task by other tasks.
//
// Returns:
//
stiHResult CstiConferenceService::Initialize (
	const std::string *serviceContacts,
	const std::string &uniqueID)
{
	stiLOG_ENTRY_NAME (CstiConferenceService::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	// NOTE: we don't need to call Cleanup to free up the allocated memory 
	// if the Initialize function

	// Free the memory allocated for mutex, watchdog timer and request queue
	Cleanup ();

	CstiVPService::Initialize (serviceContacts, uniqueID);

	return (hResult);
	
} // end CstiConferenceService::Initialize

//:-----------------------------------------------------------------------------
//:	Private methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::StateSet
//
// Description: Sets the current state
//
// Abstract:
//
// Returns:
//	None
//
stiHResult CstiConferenceService::StateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiConferenceService::StateSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = CstiVPService::StateSet(eState);

	stiTESTRESULT();
	
	// Switch on the new state...
	switch (eState)
	{
		default:
		case eUNITIALIZED:
		{
			stiDEBUG_TOOL (g_stiCnfDebug,
				stiTrace ("CstiConferenceService::StateSet - Changing state to eUNITIALIZED\n");
			); // stiDEBUG_TOOL
			break;
		} // end case eUNITIALIZED

		case eDISCONNECTED:
		{
			stiDEBUG_TOOL (g_stiCnfDebug,
				stiTrace ("CstiConferenceService::StateSet - Changing state to eDISCONNECTED\n");
			); // stiDEBUG_TOOL
			m_bConnected = estiFALSE;
			break;
		} // end case eDISCONNECTED

		case eCONNECTING:
		{
			stiDEBUG_TOOL (g_stiCnfDebug,
				stiTrace ("CstiConferenceService::StateSet - Changing state to eCONNECTING\n");
			); // stiDEBUG_TOOL
			hResult = Callback (estiMSG_CONFERENCE_SERVICE_CONNECTING, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING

		case eCONNECTED:
		{
			stiDEBUG_TOOL (g_stiCnfDebug,
				stiTrace ("CstiConferenceService::StateSet - Changing state to eCONNECTED\n");
			); // stiDEBUG_TOOL
			if (!m_bConnected)
			{
				hResult = Callback (estiMSG_CONFERENCE_SERVICE_CONNECTED, nullptr);

				stiTESTRESULT ();

				// Keep track that we have been connected.
				m_bConnected = estiTRUE;
			} // end if
			else
			{
				// Since we already have been connected, simply notify the application that we are now
				// reconnected.
				hResult = Callback (estiMSG_CONFERENCE_SERVICE_RECONNECTED, nullptr);

				stiTESTRESULT ();
			} // end else
			break;
		} // end case eCONNECTED

		case eDNSERROR:
		{
			stiDEBUG_TOOL (g_stiCnfDebug,
				stiTrace ("CstiConferenceService::StateSet - Changing state to eDNSERROR\n");
			); // stiDEBUG_TOOL

			hResult = Callback (estiMSG_DNS_ERROR, nullptr);

			stiTESTRESULT ();

			break;
		} // end case eCONNECTING
	} // end switch

STI_BAIL:

	return hResult;
	
} // end CstiConferenceService::StateSet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::RequestRemovedOnError
//
// Description: Notifies the application layer that a request was removed.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiConferenceService::RequestRemovedOnError(
	CstiVPServiceRequest *request)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto response = new CstiCoreResponse (request,
										  CstiCoreResponse::eRESPONSE_ERROR,
										  estiERROR,
										  CstiCoreResponse::eUNKNOWN_ERROR,
										  nullptr);

	// Send the response to the application
	hResult = Callback (estiMSG_CONFERENCE_REQUEST_REMOVED, response);

	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiCnfDebug,
		stiTrace ("CstiConferenceService::RequestRemovedOnError - "
			"Removing request id: %d\n",
			request->requestIdGet());
	); // stiDEBUG_TOOL

STI_BAIL:

	return hResult;
}


stiHResult CstiConferenceService::ProcessRemainingResponse (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (nSystemicError == 0)
	{
		nSystemicError = CstiConferenceResponse::eUNKNOWN_ERROR;
	}
	
	// Create a new Core Response
	auto response = new CstiConferenceResponse (
		pRequest,
		(CstiConferenceResponse::EResponse)nResponse,
		estiERROR,
		(CstiConferenceResponse::EResponseError)nSystemicError,
		nullptr);
	stiTESTCOND (response != nullptr, stiRESULT_ERROR);
	
	Callback (estiMSG_CONFERENCE_RESPONSE, response);

STI_BAIL:

	return hResult;
}


CstiConferenceResponse::EResponseError CstiConferenceService::NodeErrorGet (
	IXML_Node *pNode,
	EstiBool *pbDatabaseAvailable)
{
	CstiConferenceResponse::EResponseError eError = CstiConferenceResponse::eNO_ERROR;
	
	// Determine the error
	const char *pszErrNum = AttributeValueGet (pNode, ATT_ErrorNumber);

	if ((nullptr != pszErrNum))
	{
		eError = (CstiConferenceResponse::EResponseError)atoi (pszErrNum);

		if (eError == CstiConferenceResponse::eNOT_LOGGED_IN)
		{
			// Did we receive a database connection error?
			if (estiFALSE == *pbDatabaseAvailable)
			{
				// Yes, change the error code to be that database error code
				eError = CstiConferenceResponse::eDATABASE_CONNECTION_ERROR;
			}
		}
	}
	else
	{
		// Yes! Find out what type
		const char *pszErrorType = AttributeValueGet (pNode, ATT_Type);

		if (nullptr != pszErrorType)
		{
			if (strcmp (pszErrorType, "Validation") == 0)
			{
				eError = CstiConferenceResponse::eXML_VALIDATION_ERROR;
			} // end if
			else if (strcmp (pszErrorType, "XML Format") == 0)
			{
				eError = CstiConferenceResponse::eXML_FORMAT_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Generic") == 0)
			{
				eError = CstiConferenceResponse::eGENERIC_SERVER_ERROR;
			} // end else if
			else if (strcmp (pszErrorType, "Database Connection") == 0)
			{
				eError = CstiConferenceResponse::eDATABASE_CONNECTION_ERROR;
				// Set flag so any "not logged in" errors in this response will be ignored
				*pbDatabaseAvailable = estiFALSE;
			} // end else if
			else
			{
				eError = CstiConferenceResponse::eUNKNOWN_ERROR;
			} // end else
		}
	}
	
	return eError;
}


stiHResult CstiConferenceService::LoggedOutErrorProcess (
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
//; CstiConferenceService::ResponseNodeProcess
//
// Description: Parses the indicated node
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure result
//
stiHResult CstiConferenceService::ResponseNodeProcess (
	IXML_Node * pNode,
	CstiVPServiceRequest *pRequest,
	unsigned int unRequestId,
	int nPriority,
	EstiBool * pbDatabaseAvailable,
	EstiBool * pbLoggedOutNotified,
	int *pnSystemicError,
	EstiRequestRepeat *peRepeatRequest)
{
	stiLOG_ENTRY_NAME (CstiConferenceService::ResponseNodeProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Remove this request, unless otherwise instructed.
	*peRepeatRequest = estiRR_NO;

	// Response Object pointer
	CstiConferenceResponse * poResponse = nullptr;

	IXML_NodeList * pChildNodes = nullptr;
	IXML_NamedNodeMap * pAttributes = nullptr;
	unsigned long ulAttributeCount = 0; stiUNUSED_ARG (ulAttributeCount);

	stiDEBUG_TOOL (g_stiCnfDebug,
		stiTrace ("CstiConferenceService::ResponseNodeProcess - Node Name: %s\n",
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
	auto  eResponse = (CstiConferenceResponse::EResponse)NodeTypeDetermine (
		ixmlNode_getNodeName (pNode), g_aNodeTypeMappings, g_nNumNodeTypeMappings);

	if (pRequest)
	{
		pRequest->ExpectedResultHandled (eResponse);
	}

	// Did we get an OK result?
	if (ResultSuccess (pNode))
	{
		// Yes! Create a new response
		poResponse = new CstiConferenceResponse (
				pRequest,
				eResponse,
				estiOK,
				CstiConferenceResponse::eNO_ERROR,
				nullptr);

		stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);
		
		// Yes! Handle the response
		switch (eResponse)
		{
			case CstiConferenceResponse::eCONFERENCE_ROOM_CREATE_RESULT:
			{
				// Get the conference room URI

				// Add the Conference ID to the response
				const char *szConferenceId = SubElementValueGet (
					pNode, TAG_ConferencePublicId);

				if (szConferenceId != nullptr)
				{
					poResponse->ResponseStringSet (0, szConferenceId);
				} // end if

				// Add the URI to the response
				const char *szConferenceURI = SubElementValueGet (
					pNode, TAG_ConferenceRoomUri);

				if (szConferenceURI != nullptr)
				{
					poResponse->ResponseStringSet (1, szConferenceURI);
				} // end if

				// Notify the application of the result
				hResult = Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			case CstiConferenceResponse::eCONFERENCE_ROOM_STATUS_GET_RESULT:
			{
				hResult = ConferenceRoomStatusProcess (poResponse, pNode);
				stiTESTRESULT ();

				// Notify the application of the result
				hResult = Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);
				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			}

			default:
			case CstiConferenceResponse::eRESPONSE_UNKNOWN:
			{
				hResult = Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
				break;
			} // end case eRESPONSE_UNKNOWN
		} // end switch
	} // end if
	else
	{
		CstiConferenceResponse::EResponseError eErrorNum = CstiConferenceResponse::eUNKNOWN_ERROR;

		// No! Did we get a systemic error?
		if (eResponse == CstiConferenceResponse::eRESPONSE_ERROR)
		{
			*pnSystemicError = NodeErrorGet (pNode, pbDatabaseAvailable);

			if (eErrorNum == CstiConferenceResponse::eNOT_LOGGED_IN)
			{
				LoggedOutErrorProcess (pRequest, pbLoggedOutNotified);
			}
		}
		else
		{
			eErrorNum = NodeErrorGet (pNode, pbDatabaseAvailable);
			
			if (eErrorNum != CstiConferenceResponse::eNO_ERROR)
			{
				// Handle errors that can affect any of the messages first
				switch (eErrorNum)
				{
					case CstiConferenceResponse::eNOT_LOGGED_IN: //case 1: // Not Logged in
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
							// Notify the application of the result
							Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);

							// We no longer own this response, so clear it.
							poResponse = nullptr;
						}

						break;
					} // end case 1
					case CstiConferenceResponse::eSQL_SERVER_NOT_REACHABLE: // case 39
					case CstiConferenceResponse::eCONFERENCE_SERVER_NOT_REACHABLE: // case 14
					default:
					{
						// Create a new Response
						poResponse = new CstiConferenceResponse (
							pRequest,
							eResponse,
							estiERROR,
							(CstiConferenceResponse::EResponseError)eErrorNum,
							ElementValueGet (pNode));
			
						stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);
								
						// Notify the application of the result
						hResult = Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);
						stiTESTRESULT ();
			
						// We no longer own this response, so clear it.
						poResponse = nullptr;
						break;
					}
					
				} // end switch
			} // end if
			else
			{
				// Create a new Response
				poResponse = new CstiConferenceResponse (
					pRequest,
					eResponse,
					estiERROR,
					(CstiConferenceResponse::EResponseError)*pnSystemicError,
					ElementValueGet (pNode));
				stiTESTCOND (poResponse != nullptr, stiRESULT_ERROR);
						
				// Notify the application of the result
				hResult = Callback (estiMSG_CONFERENCE_RESPONSE, poResponse);

				stiTESTRESULT ();

				// We no longer own this response, so clear it.
				poResponse = nullptr;
			} // end else
		}
	} // end else

STI_BAIL:

	// Do we still have a ConferenceResponse object hanging around?
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
} // end CstiConferenceService::ResponseNodeProcess


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------


/**
 * \brief Processes a conference room status response
 * \param poResponse Pointer to the core response
 * \param pNode The IXML node to process.
 */
stiHResult CstiConferenceService::ConferenceRoomStatusProcess (
	CstiConferenceResponse *poResponse,
	IXML_Node *pNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	const char *pszBuff;
	int nCount = 0;

	// Get all elements named "ConferenceRoom" (should be only 1)
	IXML_NodeList *pConferenceRoomNodeList = ixmlElement_getElementsByTagName (
		(IXML_Element *)pNode, (DOMString)TAG_ConferenceRoom);

	// Did we get the list of items?
	if (pConferenceRoomNodeList != nullptr)
	{
		// Yes! Get the first item in the list
		IXML_Node * pConferenceRoomItemNode = ixmlNodeList_item (
				pConferenceRoomNodeList, 0);

		// Did we get it?
		if (pConferenceRoomItemNode != nullptr)
		{
			// Add the Conference ID to the response
			pszBuff = AttributeValueGet (
					pConferenceRoomItemNode, TAG_ConferencePublicId);

			if (pszBuff)
			{
				poResponse->ResponseStringSet (0, pszBuff);
			} // end if

			// Add the Availability
			pszBuff = SubElementValueGet (
					pConferenceRoomItemNode, TAG_ConferenceRoomStatus);

			if (pszBuff)
			{
				if (0 == strcmp (TAG_ConferenceRoomAddAllowed, pszBuff))
				{
					poResponse->AddAllowedSet (true);
				}
			} // end if

			// Get the Active Participant counts
			pszBuff = SubElementValueGet (
					pConferenceRoomItemNode, TAG_ActiveParticipantsCount);

			if (pszBuff)
			{
				nCount = atoi (pszBuff);
				poResponse->ActiveParticipantsSet (nCount);
			}

			// Get the Allowed Participant counts
			pszBuff = SubElementValueGet (
					pConferenceRoomItemNode, TAG_AllowedParticipantsCount);

			if (pszBuff)
			{
				nCount = atoi (pszBuff);
				poResponse->AllowedParticipantsSet (nCount);
			}

			// Get all elements named "Participant"
			IXML_NodeList *pParticipantNodeList = ixmlElement_getElementsByTagName (
				(IXML_Element *)pConferenceRoomItemNode, (DOMString)TAG_Participant);

			// Did we get the list of items?
			if (pParticipantNodeList != nullptr)
			{
				// Yes! Get the first item in the list
				IXML_Node * pParticipantItemNode = ixmlNodeList_item (
						pParticipantNodeList, 0);

				int i;
				SstiGvcParticipant stParticipant;

				// Loop through each of the participants in the list obtaining the data
				for (i = 1; pParticipantItemNode; i++)
				{
					pszBuff = SubElementValueGet (pParticipantItemNode, TAG_ParticipantId);
					if (pszBuff)
					{
						stParticipant.Id = pszBuff;
					}
					pszBuff = SubElementValueGet (pParticipantItemNode, TAG_PhoneNumber);
					if (pszBuff)
					{
						stParticipant.PhoneNumber = pszBuff;
					}
					pszBuff = SubElementValueGet (pParticipantItemNode, TAG_ParticipantType);
					if (pszBuff)
					{
						stParticipant.Type = pszBuff;
					}

					// Add this participant to the list in the response
					poResponse->ParticipantListAddItem (&stParticipant);

					// Get the next item
					pParticipantItemNode = ixmlNodeList_item (
											pParticipantNodeList, i);
				}
			}
		}
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyService::Restart
//
// Description:
//	Restart the Conference Services by initializing the member variables
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS - if there is no eror. Otherwize, some errors occur.
//
stiHResult CstiConferenceService::Restart ()
{
	stiLOG_ENTRY_NAME (CstiConferenceService::Restart);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiVPService::Restart();

	stiTESTRESULT();
	
	m_bConnected = estiFALSE;

STI_BAIL:

	return (hResult);

}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::RequestSendEx
//
// Description: Sends a request to the Conference Service
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK if the request has been queued, estiERROR if the
//	request cannot be queued.
//
stiHResult CstiConferenceService::RequestSendEx (
	CstiConferenceRequest * poConferenceRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiConferenceService::RequestSend);
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unRequestId;

	stiTESTCOND (poConferenceRequest, stiRESULT_ERROR);

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
		Lock ();

		// We have a valid request, and we are properly authenticated... send the request
		// Set the request number
		unRequestId = NextRequestIdGet ();
		poConferenceRequest->requestIdSet (unRequestId);

		//
		// If the calling function passed in a non-null pointer
		// for request id then return the request id through
		// that parameter.
		//
		if (nullptr != punRequestId)
		{
			*punRequestId = unRequestId;
		}

		hResult = RequestQueue (poConferenceRequest, 500);

		//
		// Make sure we unlock before we check the result
		//
		Unlock ();

		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiConferenceService::RequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceService::SSLFailedOver
//
// Description: Called by the base class to inform the derived class
// that SSL has failed over.
//
// Abstract:
//
// Returns:
//	None
//
void CstiConferenceService::SSLFailedOver()
{
	Callback (estiMSG_CONFERENCE_SERVICE_SSL_FAILOVER, nullptr);
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
EstiBool CstiConferenceService::IsDebugTraceSet (const char **ppszService) const
{
	EstiBool bRet = estiFALSE;
	
	if (g_stiCnfDebug)
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
 * \param ppszService Returns the service type string ("CNF" in this case).
 */
const char * CstiConferenceService::ServiceIDGet () const
{
	return "CNF";
}


// end file CstiConferenceService.cpp
