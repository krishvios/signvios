/*!
 *  \file CstiRingGroupManager.cpp
 *  \brief The purpose of the RingGroupManager task is to manage the members of a ring group
 *         used for automatically rejecting certain user-specified callers.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiRingGroupManager.h"
#include "XMLList.h"
#include "stiTrace.h"
#include "CstiCoreResponse.h"
#include "CstiRingGroupMgrResponse.h"
#include "stiTools.h"
#include "CstiEventQueue.h"


//
// Constants
//
#define DEFAULT_MAX_RING_GROUP_LENGTH 5
#define ABSOLUTE_MAX_RING_GROUP_ITEMS 5

// DBG_ERR_MSG is a tool to add file/line infromation to an stiTrace
// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiRingGroupDebug, stiTrace ("RingGroup: " fmt "\n", ##__VA_ARGS__); )
#define DBG_ERR_MSG(fmt,...) stiDEBUG_TOOL (g_stiRingGroupDebug, stiTrace (HERE " ERROR: " fmt "\n", ##__VA_ARGS__); )


//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

/*! \brief Constructor
 *
 * This is the default constructor for the CstiRingGroupManager class.
 *
 * \return None
 */
CstiRingGroupManager::CstiRingGroupManager (
	ERingGroupDisplayMode eMode,
	CstiCoreService *pCoreService,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam
)
:
	m_bInitialized (false),
	m_eMode (eMode),
	m_pCoreService (pCoreService),
	m_bAuthenticated (estiFALSE),
	m_nMaxLength (DEFAULT_MAX_RING_GROUP_LENGTH),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_CallbackFromId (0),
	m_unItemAddRequestId (0),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet ())
{
	std::string dynamicDataFolder;
	stiOSDynamicDataFolderGet (&dynamicDataFolder);
	std::stringstream filePath;
	filePath << dynamicDataFolder << m_fileName;
	SetFilename (filePath.str ());

	m_signalConnections.push_back (m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		m_eventQueue.PostEvent(std::bind(&CstiRingGroupManager::clientAuthenticateHandle, this, result));
	}));
}

CstiRingGroupManager::~CstiRingGroupManager ()
{
	m_pdoc->ListFree ();
}


/*!
 * This is called to Initialize the function.
 */
void CstiRingGroupManager::Initialize ()
{
	m_bInitialized = !stiIS_ERROR (init ());
}


/*!\brief Signal callback that is raised when a ClientAuthenticateResult core response is received.
 *
 */
void CstiRingGroupManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	DBG_MSG ("CoreResponse eCLIENT_AUTHENTICATE_RESULT (id %d) = %s", clientAuthenticateResult->requestId, clientAuthenticateResult->responseSuccessful ? "OK" : "Err");
	AuthenticatedSet (clientAuthenticateResult->responseSuccessful ? estiTRUE : estiFALSE);
}


/*!
 * Since the ring group can only be retreived when the user has been authenticated by core,
* this method informs the ring group manager when that state has been reached.
*
* \param[in] bAuthenticated Whether or not the user is authenticated.
*/
stiHResult CstiRingGroupManager::AuthenticatedSet (
	EstiBool bAuthenticated)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_bAuthenticated = bAuthenticated;

	Unlock ();

	return hResult;
}


/*!
 * Sets the Ring group settings
 *
 * \param eMode The display mode.
 */
void CstiRingGroupManager::ModeSet(ERingGroupDisplayMode eMode)
{
	Lock ();

	if (m_eMode != eMode)
	{
		m_eMode = eMode;

		if (m_eMode != eRING_GROUP_DISABLED)
		{
			if (m_bAuthenticated)
			{
				// Get RingGroup Information (if it exists)
				auto  poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);
			}
		}
		CallbackMessageSend (estiMSG_RING_GROUP_MODE_CHANGED, 0);
	}

	Unlock ();
}

/*!
 * Utility function to determine if the request id of the given core
 * response was one that we were waiting for.  If so, the request
 * id will be removed from the list.
 *
 * \param pCoreResponse The core request which will be checked.
 */
EstiBool CstiRingGroupManager::RemoveRequestId (
	unsigned int unRequestId)
{
	EstiBool bHasRingGroupCookie = estiFALSE;

	Lock ();
	for (auto it = m_PendingCookies.begin (); it != m_PendingCookies.end (); ++it)
	{
		if (*it == unRequestId)
		{
			bHasRingGroupCookie = estiTRUE;
			m_PendingCookies.erase (it);
			break;
		}
	}
	Unlock ();

	return bHasRingGroupCookie;
}


EstiBool CstiRingGroupManager::RemovedUponCommunicationErrorHandle (
	CstiVPServiceResponse *response)
{
	EstiBool bHandled = estiFALSE;

#if 0
	if (RemoveRequestId (pRemovedRequest->unRequestId))
	{
		// Create an item, not to be added, but for the application
		CstiCallListItem *pCallListItem = (CstiCallListItem *)pRemovedRequest->pContext;

		const char *pszItemId = "";
		if (pCallListItem)
		{
			if (pCallListItem->ItemIdGet())
			{
				pszItemId = pCallListItem->ItemIdGet();
			}
		}

		CstiRingGroupMgrResponse *pResponse = new CstiRingGroupMgrResponse (
			pszItemId, pRemovedRequest->unRequestId);

		CallbackMessageSend (estiMSG_RING_GROUP_ITEM_ERROR, (size_t)pResponse);

		delete pRemovedRequest;
		delete pCallListItem;

		bHandled = estiTRUE;
	}
#endif

	return bHandled;
}


/*!
 * Core responses when retreived by CCI shall be passed to this method for handling.  If
 * the return value is true then the response was one that the ring group manager was expecting
 * to receive.  In this case, the response should not be passed on to other objects (such as the UI).
 * The response object shall be deleted by the ring group manager.
 *
 * If the return value is false, the response was not handled and the response shall be passed on
 * to other objects for handling.
 *
 * \param[in] pCoreResponse The core response to be handled.
 */
EstiBool CstiRingGroupManager::CoreResponseHandle (
	CstiCoreResponse *pCoreResponse)
{
	EstiBool bHandled = estiFALSE;

	switch (pCoreResponse->ResponseGet ())
	{
		case CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT:
			
			// Check to see if we are attempting to add/invite a new group member
			if (pCoreResponse->RequestIDGet () == m_unItemAddRequestId)
			{
				if (pCoreResponse->ResponseResultGet() == estiOK
					&& RingGroupInfoListCountGet(pCoreResponse) < (int)m_nMaxLength)
				{
					// Member count is less than max members, go ahead
					//  and send the request to invite new member
					ItemAddCoreRequestSend();
				}
				else
				{
					// Member count exceeds max size; send back an error
					auto pResponse = new CstiRingGroupMgrResponse(
						m_ItemAddNumber.c_str(),
						m_unItemAddRequestId,
						CstiCoreResponse::eCANNOT_INVITE_USER);
				
					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_ADD_ERROR, (size_t)pResponse);
					
					//NOTE: TODO: Possibly force a heartbeat at this point... to get ringGroupInfoChanged event
				}
				pCoreResponse->destroy ();
				pCoreResponse = nullptr;

				bHandled = estiTRUE;
			}
			else // The RingGroup Manager may or may not have sent the request, but process the result anyways.
			{
				if (pCoreResponse->ResponseResultGet() == estiOK)
				{
					// Get the account list from the core response
					RingGroupInfoListGetFromResponse(pCoreResponse);
					
					m_bInitialized = true;
				}
				else
				{
					// Clear the members as an error indicates
					// the user is not part of a group
					m_pdoc->ListFree ();
					
					m_bInitialized = false;
				}

				CallbackMessageSend (estiMSG_RING_GROUP_INFO_CHANGED, 0);
				
				if (RemoveRequestId (pCoreResponse->RequestIDGet ())) {
					// If the RingGroup Manager sent the request, then consume it.
					pCoreResponse->destroy ();
					pCoreResponse = nullptr;
					
					bHandled = estiTRUE;
				}
			}

			startSaveDataTimer ();
			
			break;
			
		case CstiCoreResponse::eRING_GROUP_USER_REMOVE_RESULT:
			
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				auto memberNumber = pCoreResponse->contextItemGet<std::string> ();
				
				auto pResponse = new CstiRingGroupMgrResponse(
					memberNumber.c_str(),
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				
				if (pCoreResponse->ResponseResultGet() == estiOK)
				{
					//
					// Find and remove the item from the list.
					//

					for (size_t i = 0; i < m_pdoc->CountGet (); i++)
					{
						auto item = std::static_pointer_cast<CstiRingGroupMemberInfo>(m_pdoc->ItemGet (i));
						if (item->LocalPhoneNumber == memberNumber || item->TollFreePhoneNumber == memberNumber)
						{
							m_pdoc->ItemDestroy (i);
							break;
						}
					}
	
					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_DELETED, (size_t)pResponse);
				}
				else
				{
					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_DELETE_ERROR, (size_t)pResponse);
				}

				startSaveDataTimer ();
				
				pCoreResponse->destroy ();
				pCoreResponse = nullptr;

				bHandled = estiTRUE;
			}
			
			break;
			
		case CstiCoreResponse::eRING_GROUP_USER_INVITE_RESULT:
			
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				auto ringGroupMember = pCoreResponse->contextItemGet<CstiRingGroupMemberInfo> ();
				
				auto pResponse = new CstiRingGroupMgrResponse(
					ringGroupMember.LocalPhoneNumber.c_str(),
					m_unItemAddRequestId,
					pCoreResponse->ErrorGet());
				
				if (pCoreResponse->ResponseResultGet() != estiOK)
				{
					//
					// An error occured. Find and remove the item from the list.
					//
// 					std::vector <CstiRingGroupMemberInfo>::iterator i;
// 					
// 					for (i = m_RingGroupMembers.begin (); i != m_RingGroupMembers.end (); i++)
// 					{
// 						if ((*i).LocalPhoneNumber == pRingGroupMember->LocalPhoneNumber)
// 						{
// 							m_RingGroupMembers.erase (i);
// 							
// 							break;
// 						}
// 					}
	
					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_ADD_ERROR, (size_t)pResponse);
				}
				else
				{
					//
					// Insert the new item into the list, in pending state
					//
					//!\todo: make sure the number is not already in the list (?)
					m_pdoc->ItemAdd (std::make_shared<CstiRingGroupMemberInfo> (ringGroupMember));

					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_ADDED, (size_t)pResponse);
				}

				startSaveDataTimer ();
				
				pCoreResponse->destroy ();
				pCoreResponse = nullptr;

				bHandled = estiTRUE;
			}
			
			break;
			
		case CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT:
			
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				auto ringGroupMember = pCoreResponse->contextItemGet<CstiRingGroupMemberInfo> ();
				
				auto pResponse = new CstiRingGroupMgrResponse(
					ringGroupMember.LocalPhoneNumber.c_str(),
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				
				if (pCoreResponse->ResponseResultGet() != estiOK)
				{
					//
					// An error occured... report it to the user.
					//

					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_EDIT_ERROR, (size_t)pResponse);
				}
				else
				{
					//
					// SUCCESS! Find and modify the item in the list.
					//
					for (size_t i = 0; i < m_pdoc->CountGet(); i++)
					{
						auto item = std::static_pointer_cast<CstiRingGroupMemberInfo>(m_pdoc->ItemGet (i));
						if (item->LocalPhoneNumber == ringGroupMember.LocalPhoneNumber)
						{
							item->Description = ringGroupMember.Description;
							break;
						}
					}

					CallbackMessageSend (estiMSG_RING_GROUP_ITEM_EDITED, (size_t)pResponse);
				}

				startSaveDataTimer ();
				
				pCoreResponse->destroy ();
				pCoreResponse = nullptr;

				bHandled = estiTRUE;
			}
			
			break;

#ifndef stiRING_GROUP_MANAGER_v2
		case CstiCoreResponse::eRING_GROUP_CREATE_RESULT:
		case CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT:
			{
				// This endpoint has created or been added to a ring group, which means the ring group info has changed, therefore we need to request the ring group info
				if (pCoreResponse->ResponseResultGet() == estiOK)
				{
					CstiCoreRequest* poCoreRequest = new CstiCoreRequest ();
					poCoreRequest->ringGroupInfoGet ();
					CoreRequestSend (poCoreRequest, NULL);
				}

				bHandled = estiFALSE;
				break;
			}
#else

		case CstiCoreResponse::eRING_GROUP_CREATE_RESULT:
			// This endpoint has created a ring group, which means the ring
			// group info has changed, therefore we need to request the ring
			// group info
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				auto  poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);

				CallbackMessageSend(estiMSG_RING_GROUP_CREATED, 0);
			}
			else
			{
				auto pResponse = new CstiRingGroupMgrResponse(
					"",
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				CallbackMessageSend(estiMSG_RING_GROUP_CREATE_ERROR, (size_t)pResponse);
			}

			bHandled = estiTRUE;
			break;

		case CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT:
			// This endpoint has been added to a ring group, which means the
			// ring group info has changed, therefore we need to request the
			// ring group info
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				auto  poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);

				CallbackMessageSend(estiMSG_RING_GROUP_INVITE_ACCEPTED, 0);
			}
			else
			{
				auto pResponse = new CstiRingGroupMgrResponse(
					"",
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				CallbackMessageSend(estiMSG_RING_GROUP_INVITE_ACCEPT_ERROR, (size_t)pResponse);
			}

			m_InviteName.clear();
			m_InviteLocalNumber.clear();
			m_InviteTollFreeNumber.clear();
			bHandled = estiTRUE;
			break;

		case CstiCoreResponse::eRING_GROUP_INVITE_REJECT_RESULT:
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				CallbackMessageSend(estiMSG_RING_GROUP_INVITE_REJECTED, 0);
			}
			else
			{
				auto pResponse = new CstiRingGroupMgrResponse(
					"",
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				CallbackMessageSend(estiMSG_RING_GROUP_INVITE_REJECT_ERROR, (size_t)pResponse);
			}

			m_InviteName.clear();
			m_InviteLocalNumber.clear();
			m_InviteTollFreeNumber.clear();
			bHandled = estiTRUE;
			break;

		case CstiCoreResponse::eRING_GROUP_INVITE_INFO_GET_RESULT:
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				// Get the user name of the inviting account
				if (!pCoreResponse->ResponseStringGet (0).empty ())
				{
					m_InviteName = pCoreResponse->ResponseStringGet (0);
				}
				// Get the local phone of the inviting account
				if (!pCoreResponse->ResponseStringGet (1).empty ())
				{
					m_InviteLocalNumber = pCoreResponse->ResponseStringGet (1);
				}
				// Get the tollfree phone of the inviting account
				if (!pCoreResponse->ResponseStringGet (2).empty ())
				{
					m_InviteTollFreeNumber = pCoreResponse->ResponseStringGet (2);
				}

				CallbackMessageSend(estiMSG_RING_GROUP_INVITATION, 0);
			}
			break;

		case CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT:
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				CallbackMessageSend(estiMSG_RING_GROUP_VALIDATED, 0);
			}
			else
			{
				auto pResponse = new CstiRingGroupMgrResponse(
					"",
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				CallbackMessageSend(estiMSG_RING_GROUP_VALIDATE_ERROR, (size_t)pResponse);
			}
			bHandled = estiTRUE;
			break;

		case CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT:
			if (pCoreResponse->ResponseResultGet() == estiOK)
			{
				CallbackMessageSend(estiMSG_RING_GROUP_PASSWORD_CHANGED, 0);
			}
			else
			{
				auto pResponse = new CstiRingGroupMgrResponse(
					"",
					pCoreResponse->RequestIDGet(),
					pCoreResponse->ErrorGet());
				CallbackMessageSend(estiMSG_RING_GROUP_PASSWORD_CHANGE_ERROR, (size_t)pResponse);
			}
			bHandled = estiTRUE;
			break;

#endif
		case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
		{
			AuthenticatedSet (estiFALSE);
			clear ();
			break;
		}
		default:
			break;
	}

	return bHandled;
}


/*!
 * The ring group manager shall retreive a new ring group if an event indicates that the
 * ring group has changed in core.  This may occur if a white list entry has been added
 * or removed.
 *
 * \param[in] pStateNotifyResponse The state notify response to be handled.
 */
EstiBool CstiRingGroupManager::StateNotifyEventHandle (
	CstiStateNotifyResponse *pStateNotifyResponse)
{
	EstiBool bHandled = estiFALSE;

	if (pStateNotifyResponse)
	{
		switch (pStateNotifyResponse->ResponseGet ())
		{
			case CstiStateNotifyResponse::eRING_GROUP_INFO_CHANGED:
			{
				// The ring group info has changed, therefore we need to request the ring group info
				auto  poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);

				pStateNotifyResponse->destroy ();
				bHandled = estiTRUE;
				
				break;
			}
			case CstiStateNotifyResponse::eRING_GROUP_REMOVED:
			{
				// Clear the list
				clear ();
				CallbackMessageSend (estiMSG_RING_GROUP_REMOVED, 0);

				pStateNotifyResponse->destroy ();
				bHandled = estiTRUE;

				break;
			}
			case CstiStateNotifyResponse::eLOGGED_OUT:
			{
				AuthenticatedSet (estiFALSE);
				clear ();
				break;
			}
#ifdef stiRING_GROUP_MANAGER_v2
			case CstiStateNotifyResponse::eRING_GROUP_INVITE:
			{
				auto poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInviteInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);

				pStateNotifyResponse->destroy ();
				bHandled = estiTRUE;

				break;
			}

			case CstiStateNotifyResponse::eRING_GROUP_CREATED:
			{
				// We're now in a ring group!  Let's get our ring group info.
				auto  poCoreRequest = new CstiCoreRequest ();
				poCoreRequest->ringGroupInfoGet ();
				CoreRequestSend (poCoreRequest, nullptr);

				CallbackMessageSend(estiMSG_RING_GROUP_CIR_CREATED, 0);

				pStateNotifyResponse->destroy ();
				bHandled = estiTRUE;

				break;
			}

			case CstiStateNotifyResponse::eDISPLAY_RING_GROUP_WIZARD:
				CallbackMessageSend(estiMSG_RING_GROUP_WIZARD, 0);

				pStateNotifyResponse->destroy ();
				bHandled = estiTRUE;

				break;
#endif

			default:
				break;
		}
	}

	return bHandled;
}


/*!
 * Utility function to send a callback message (if any callback is registered)
 *
 * \param[in] eMessage The message to send.
 * \param[in] Param The parameter to send (if required).
 */
void CstiRingGroupManager::CallbackMessageSend (
	EstiCmMessage eMessage,
	size_t Param)
{
	Lock ();
	if (m_pfnVPCallback)
	{
		m_pfnVPCallback (eMessage, Param, m_CallbackParam, m_CallbackFromId);
	}
	Unlock ();
}


/*!
 * Determines if the number identifies the group creator.
 *
 * \param[in] pszMemberNumber The number to check if it is the ring group creator.
 */
bool CstiRingGroupManager::IsGroupCreator (
	const std::string& memberNumber)
{
	bool bReturn = false;
	CstiRingGroupMemberInfo RingGroupItem;

	Lock ();

	if (ItemGet(memberNumber, &RingGroupItem))
	{
		bReturn = RingGroupItem.bIsGroupCreator;
	}

	Unlock ();

	return bReturn;
}


/*!
* Adds an item to the ring group.
*
* The manager will search its list for the number.  If the number is not found then the
* manager will add the number to the list and send a core request to add the number to core.
*
* If the number is found then an error is returned.
*
* If the list already contains the maximum number of entries, an error is returned.
*
* A core request shall be issued to add the blocked to number to core.  The request id for
* this request is returned to the caller.  Once the request has been completed the ring group
* manager shall call the callback with a message and the request id indicating the request is
* complete.  The caller of this method may use this to monitor the completion of
* the asynchronous process.
*
* \param[in] pszBlockedNumber The number to add to the list.
* \param[in] pszDescription A description to associate to the number.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiRingGroupManager::ItemAdd (
	const char *pszMemberNumber,
	const char *pszDescription,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pRequest = nullptr;
	
	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && (m_eMode == eRING_GROUP_DISPLAY_MODE_READ_WRITE) && m_pdoc->CountGet() < m_nMaxLength, stiRESULT_ERROR);

	// Before we send the ItemAdd core request, we need to see if ring group already has maximum number
	//  of members.  We need to send the RingGroupInfoGet request to determine this.

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->ringGroupInfoGet ();
	
	m_ItemAddNumber = pszMemberNumber;
	m_ItemAddDescription = pszDescription;

	hResult = CoreRequestSend (pRequest, &m_unItemAddRequestId);

	if (punRequestId)
	{
		*punRequestId = m_unItemAddRequestId;
	}

STI_BAIL:

	Unlock ();

	return hResult;
}


//
// Private function to send along the RingGroupUserInvite core request
//
stiHResult CstiRingGroupManager::ItemAddCoreRequestSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pRequest = nullptr;
	CstiRingGroupMemberInfo ringGroupMember;
	
	Lock ();

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->ringGroupUserInvite (m_ItemAddNumber.c_str(), m_ItemAddDescription.c_str());
	
	ringGroupMember.LocalPhoneNumber = m_ItemAddNumber;
	ringGroupMember.Description = m_ItemAddDescription;
	ringGroupMember.eState = eRING_GROUP_STATE_PENDING;

	pRequest->contextItemSet (ringGroupMember);
	
	hResult = CoreRequestSend (pRequest, nullptr);
	
	// Clear these member vars... no longer needed.
	m_ItemAddNumber.clear();
	m_ItemAddDescription.clear();

	Unlock ();

	return hResult;
}


/*!
	Creates a CstiRingGroupMemberInfo instance from XML
 */
WillowPM::XMLListItemSharedPtr CstiRingGroupManager::NodeToItemConvert (IXML_Node* pNode)
{
	std::string nodeName = pNode->nodeName;
	if (nodeName == CstiRingGroupMemberInfo::NodeName)
	{
		return std::make_shared<CstiRingGroupMemberInfo> (pNode);
	}
	return nullptr;
}


/*!
	Clears myPhone group data when user is logged out or removed from myPhone group.
 */
void CstiRingGroupManager::clear ()
{
	m_ItemAddNumber.clear();
	m_ItemAddDescription.clear();
	m_unItemAddRequestId = 0;
	m_pdoc->ListFree ();
	startSaveDataTimer ();
}


/*!
 * Send a Core Request
 *
 * This function should be called when Core Services has been logged in.  If
 * not logged in, the CoreService.RequestSend will initiate a re-login
 * sequence.
 *
 * NOTE: The poCoreRequest parameter MUST have been created with the 'new'
 * operator. This object WILL NO LONGER belong to the calling code, and will
 * be freed by this function.
 *
 * \param[in] poCoreRequest The core request to send (this will be deleted when sent).
 * \param[out] punRequestId The sent core request id/cookie.
 *
 * Returns:
 * estiOK if in an initialized state and the Core Service logout message is
 * successfully sent, estiERROR otherwise.
*/
stiHResult CstiRingGroupManager::CoreRequestSend (
	CstiCoreRequest *poCoreRequest,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unInternalCoreRequestId = 0;

	stiTESTCOND (m_pCoreService, stiRESULT_ERROR);

	poCoreRequest->RemoveUponCommunicationError () = estiTRUE;
	hResult = m_pCoreService->RequestSend (
		poCoreRequest,
		&unInternalCoreRequestId);
	stiTESTRESULT ();

	m_PendingCookies.push_back (unInternalCoreRequestId);

	DBG_MSG ("Core request %d sent.", unInternalCoreRequestId);

	if (punRequestId)
	{
		*punRequestId = unInternalCoreRequestId;
	}

STI_BAIL:

	return hResult;
} // end CstiRingGroupManager::CoreRequestSend


/*!
* Deletes an item from the ring group.
*
* The manager will search its list for the number.  If the number is found then the manager
* will remove the number from the list and send a core request to remove the number from
* core.
*
* A core request shall be issued to delete the ring group number from core.  The request id
* for this request is returned to the caller.  Once the request has been completed the
* ring group manager shall call the callback with a message and the request id indicating
* the request is complete.  The caller of this method may use this to monitor the completion
* of the asynchronous process.
*
* \param[in] pszMemberNumber The number to remove from the ring group.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiRingGroupManager::ItemDelete (
	const char *pszMemberNumber,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && (m_eMode == eRING_GROUP_DISPLAY_MODE_READ_WRITE), stiRESULT_ERROR);

	{
	// Send the core request
	auto pRequest = new CstiCoreRequest ();
	pRequest->ringGroupUserRemove (pszMemberNumber);
	
	pRequest->contextItemSet (std::string (pszMemberNumber));
	
	hResult = CoreRequestSend (pRequest, punRequestId);
	}

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Changes the ring group entry to the new values specified.
*
* The manager will search its list for the old number.  If the old number is found then the
* manager will replace the old number with the new number and send a core request to core to
* update the number.
*
* A core request shall be issued to edit the ring group member in core.  The request id for this
* request is returned to the caller.  Once the request has been completed the ring group
* manager shall call the callback with a message and the request id indicating the request
* is complete.  The caller of this method may use this to monitor the completion of the
* asynchronous process.
*
* \param[in] pszItemId The Database ID of the RingGroup item you wish to edit.
* \param[in] pszNewDescription The new description for the number.  This may be NULL if the
* description is not be changed.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiRingGroupManager::ItemEdit (
	const char *pszMemberNumber,
	const char *pszNewDescription,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiRingGroupMemberInfo RingGroupItem;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && (m_eMode == eRING_GROUP_DISPLAY_MODE_READ_WRITE), stiRESULT_ERROR);

#if 1

	if (!ItemGet(pszMemberNumber, &RingGroupItem))
	{
		stiTHROW (stiRESULT_ERROR);
	}
#endif

	{
	auto pRequest = new CstiCoreRequest ();
	CstiRingGroupMemberInfo ringGroupMember;

	// Send the core request
	pRequest->ringGroupInfoSet(pszMemberNumber, pszNewDescription);

	ringGroupMember.LocalPhoneNumber = pszMemberNumber;
	ringGroupMember.Description = pszNewDescription;
	ringGroupMember.eState = RingGroupItem.eState;

	pRequest->contextItemSet (ringGroupMember);

	hResult = CoreRequestSend (pRequest, punRequestId);
	}

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Finds a Ring Group member entry by phone number.
*
* The manager will search its list for the number.  If the number is found then estiTRUE
* is returned.  Otherwise, estiFALSE is returned.
*
* \param pszMemberNumber The phone number used to identify the entry
* \param[out] pszDescription The description associated to the entry.
*/
EstiBool CstiRingGroupManager::ItemGetByNumber (
	const char *pszMemberNumber,
	std::string *pDescription,
	ERingGroupMemberState *pMemberState) const
{
	CstiRingGroupMemberInfo RingGroupItem;
	EstiBool bFound = estiFALSE;

	Lock ();
	
	// Can we even do this?
	if ((m_eMode != eRING_GROUP_DISABLED))
	{
#if 1
		bFound = ItemGet (pszMemberNumber, &RingGroupItem);

		// Fill out the description and Member state
		if (bFound)
		{
			pDescription->assign (RingGroupItem.Description);
			*pMemberState = RingGroupItem.eState;
		}
		else
		{
			pDescription->clear ();
			*pMemberState = eRING_GROUP_STATE_UNKNOWN;
		}
#else
		// Search list for the requested member description
		std::vector <CstiRingGroupMemberInfo>::const_iterator i;
		std::string PhoneNumber = pszMemberNumber;
		
		for (i = m_RingGroupMembers.begin (); i != m_RingGroupMembers.end (); i++)
		{
			if ((*i).LocalPhoneNumber == PhoneNumber)
			{
				pDescription->assign ((*i).Description);
				*pMemberState = (*i).eState;
				
				break;
			}
			if ((*i).TollFreePhoneNumber == PhoneNumber)
			{
				pDescription->assign ((*i).Description);
				*pMemberState = (*i).eState;
				
				break;
			}
		}
#endif
	}

	Unlock ();

	return bFound;
}


EstiBool CstiRingGroupManager::ItemGetByDescription (
	const char *pszDescription,
	std::string *pMemberNumber,
	ERingGroupMemberState *pMemberState) const
{
	Lock ();
	
	pMemberNumber->clear();
	*pMemberState = eRING_GROUP_STATE_UNKNOWN;

	// Can we even do this?
	if (m_eMode != eRING_GROUP_DISABLED)
	{
		// Search list for the requested member description
		for (size_t i = 0; i < m_pdoc->CountGet(); i++)
		{
			auto item = std::static_pointer_cast<CstiRingGroupMemberInfo>(m_pdoc->ItemGet (i));
			if (stiStrICmp (item->Description.c_str (), pszDescription) == 0)
			{
				*pMemberNumber = item->LocalPhoneNumber;
				*pMemberState = item->eState;
				break;
			}
		}
	}

	Unlock ();
	
	return ((!pMemberNumber->empty() && *pMemberState == eRING_GROUP_STATE_ACTIVE) ? estiTRUE : estiFALSE);
}


/*!
 * Finds a ring group member by phone number.
 *
 * Returns the item found or NULL.
 *
 * \param pszMemberNumber The phone number used to identify the entry
 */
EstiBool CstiRingGroupManager::ItemGet (
	const std::string& memberNumber,
	CstiRingGroupMemberInfo *pMemberInfo) const
{
	EstiBool bFound = estiFALSE;

	if (!memberNumber.empty())
	{
		Lock ();

		// Search list for the requested member number
		for (size_t i = 0; i < m_pdoc->CountGet (); i++)
		{
			auto item = std::static_pointer_cast<CstiRingGroupMemberInfo>(m_pdoc->ItemGet (i));
			if (item->LocalPhoneNumber == memberNumber || item->TollFreePhoneNumber == memberNumber)
			{
				*pMemberInfo = *item;
				bFound = estiTRUE;
				break;
			}
		}

		Unlock ();
	}

	return bFound;
}


/*!
* Gets a ring group list entry by index.
*
* This method is used to iterate through the list.  An object should lock the list before
* iterating through the list to protect the list from changing during the iteration process.
*
* \param nIndex The index used to identify the entry
* \param[out] pMemberNumber The phone number associated for the entry at the provided index
* \param[out] pDescription The description associated to the member number.
* \param[out] pMemberState The RingGroup state of the member number.
*/
stiHResult CstiRingGroupManager::ItemGetByIndex (
	int nIndex,
	std::string *pMemberLocalNumber,
	std::string *pMemberTollFreeNumber,
	std::string *pDescription,
	ERingGroupMemberState *pMemberState) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = false;

	pMemberLocalNumber->clear ();
	pMemberTollFreeNumber->clear ();
	pDescription->clear ();
	*pMemberState = IstiRingGroupManager::eRING_GROUP_STATE_UNKNOWN;

	Lock ();

	// Can we even do this?
	stiTESTCOND ((m_eMode != eRING_GROUP_DISABLED) && m_bInitialized, stiRESULT_ERROR);

	if (nIndex >= 0 && nIndex < static_cast<int>(m_pdoc->CountGet ()))
	{
		auto item = std::static_pointer_cast<CstiRingGroupMemberInfo>(m_pdoc->ItemGet (nIndex));
		*pMemberLocalNumber = item->LocalPhoneNumber;
		*pMemberTollFreeNumber = item->TollFreePhoneNumber;
		*pDescription = item->Description;
		*pMemberState = item->eState;
		
		bFound = true;
	}
	
	stiTESTCOND_NOLOG (bFound, stiRESULT_ERROR);

STI_BAIL:

	Unlock ();

	return hResult;
}

/*!
 * Gets the number of ring group members in the group.
 */
int CstiRingGroupManager::ItemCountGet() const
{
	return m_pdoc->CountGet();
}


/*!
* Retrieves the ring group info.
*
*/
void CstiRingGroupManager::RingGroupInfoGet ()
{
	CstiCoreRequest *pRequest = nullptr;

	Lock ();

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->ringGroupInfoGet ();

	CoreRequestSend (pRequest, nullptr);

	Unlock ();

}


/*!
 * \brief Extracts the RingGroupMembersList from the core response and copies to member vector
 */
void CstiRingGroupManager::RingGroupInfoListGetFromResponse (
	CstiCoreResponse *poResponse)
{
	// Clear the existing list, in preparation to be loaded (or reloaded)
	m_pdoc->ListFree ();
	
	std::vector <CstiCoreResponse::SRingGroupInfo> ringGroupMembers;
	std::vector <CstiCoreResponse::SRingGroupInfo>::const_iterator i;
	
	poResponse->RingGroupInfoListGet (&ringGroupMembers);
	
	for (i = ringGroupMembers.begin (); i != ringGroupMembers.end (); i++)
	{
		auto ringGroupMemberInfo = std::make_shared<CstiRingGroupMemberInfo>();
		
		ringGroupMemberInfo->Description = i->Description;
		ringGroupMemberInfo->LocalPhoneNumber = i->LocalPhoneNumber;
		ringGroupMemberInfo->TollFreePhoneNumber = i->TollFreePhoneNumber;
		ringGroupMemberInfo->eState = (IstiRingGroupManager::ERingGroupMemberState)i->eState;
		ringGroupMemberInfo->bIsGroupCreator = i->bIsGroupCreator;
		
		if (i->bIsGroupCreator)
		{
			// This is the ring group creator... insert it at the begining of the list
			m_pdoc->itemInsert (ringGroupMemberInfo, 0);
		}
		else
		{
			m_pdoc->ItemAdd (ringGroupMemberInfo);
		}
	}
}


/*!
 * \brief Retrieves the list returned by RingGroupInfoGet and returns the number of members
 */
int CstiRingGroupManager::RingGroupInfoListCountGet (
	CstiCoreResponse *poResponse)
{
	std::vector <CstiCoreResponse::SRingGroupInfo> ringGroupMembers;

	poResponse->RingGroupInfoListGet (&ringGroupMembers);
	
	return (int)ringGroupMembers.size();
}

#ifdef stiRING_GROUP_MANAGER_v2
/*!
 * \brief Retrieves the data from an invitation that we've received to join a ring group.
 */
stiHResult CstiRingGroupManager::InvitationGet (std::string *pLocalNumber,
		std::string *pTollFreeNumber, std::string *pDescription) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND(m_InviteLocalNumber.length() != 0 || m_InviteTollFreeNumber.length() != 0, stiRESULT_ERROR);

	*pLocalNumber = m_InviteLocalNumber;
	*pTollFreeNumber = m_InviteTollFreeNumber;
	*pDescription = m_InviteName;

STI_BAIL:
	return hResult;
}


/*!
 * \brief Notifies Core that we are accepting the invitation.
 */
stiHResult CstiRingGroupManager::InvitationAccept ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *poCoreRequest = nullptr;

	stiTESTCOND(m_InviteLocalNumber.length() != 0 || m_InviteTollFreeNumber.length() != 0, stiRESULT_ERROR);

	poCoreRequest = new CstiCoreRequest();
	poCoreRequest->ringGroupInviteAccept (m_InviteLocalNumber.c_str());
	CoreRequestSend (poCoreRequest, nullptr);

STI_BAIL:
	return hResult;
}

/*!
 * \brief Notifies Core that we are accepting the invitation.
 */
stiHResult CstiRingGroupManager::InvitationReject ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *poCoreRequest = nullptr;

	stiTESTCOND(m_InviteLocalNumber.length() != 0 || m_InviteTollFreeNumber.length() != 0, stiRESULT_ERROR);

	poCoreRequest = new CstiCoreRequest();
	poCoreRequest->ringGroupInviteReject (m_InviteLocalNumber.c_str());
	CoreRequestSend (poCoreRequest, nullptr);

STI_BAIL:
	return hResult;
}

/*!
 * \brief Notifies Core that we are accepting the invitation.
 */
stiHResult CstiRingGroupManager::PasswordValidate(const char *pszNumber, const char *pszPassword)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto poCoreRequest = new CstiCoreRequest();
	poCoreRequest->RemoveUponCommunicationError () = estiTRUE;
	poCoreRequest->ringGroupCredentialsValidate(pszNumber, pszPassword);
	CoreRequestSend(poCoreRequest, nullptr);

//STI_BAIL:
	return hResult;
}

stiHResult CstiRingGroupManager::PasswordChange(const char *pszPassword)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto poCoreRequest = new CstiCoreRequest();
	poCoreRequest->RemoveUponCommunicationError () = estiTRUE;
	poCoreRequest->ringGroupPasswordSet(pszPassword);
	CoreRequestSend(poCoreRequest, nullptr);

	return hResult;
}

/*!
* \brief Creates a ring group using this endpoint's account for the ring group account
*/
stiHResult CstiRingGroupManager::RingGroupCreate (
	const char *pszDescription,
	const char *pszPassword)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pRequest = nullptr;

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && (m_eMode == eRING_GROUP_DISPLAY_MODE_READ_WRITE), stiRESULT_ERROR);

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->ringGroupCreate (pszDescription, pszPassword);
	hResult = CoreRequestSend (pRequest, nullptr);

STI_BAIL:

	return hResult;
}

#endif

static const std::string DescriptionNode = "Description";
static const std::string LocalPhoneNumberNode = "LocalPhoneNumber";
static const std::string TollFreePhoneNumberNode = "TollFreePhoneNumber";
static const std::string StateNode = "State";
static const std::string IsGroupCreatorNode = "IsGroupCreator";

CstiRingGroupManager::CstiRingGroupMemberInfo::CstiRingGroupMemberInfo (IXML_Node* pNode)
{
	for (auto node = pNode->firstChild; node != nullptr; node = node->nextSibling)
	{
		std::string value;
		if (node->firstChild != nullptr && node->firstChild->nodeValue != nullptr)
		{
			value = node->firstChild->nodeValue;
		}

		if (node->nodeName == DescriptionNode)
		{
			Description = value;
		}
		else if (node->nodeName == LocalPhoneNumberNode)
		{
			LocalPhoneNumber = value;
		}
		else if (node->nodeName == TollFreePhoneNumberNode)
		{
			TollFreePhoneNumber = value;
		}
		else if (node->nodeName == StateNode)
		{
			eState = static_cast<ERingGroupMemberState>(std::stoi (value));
		}
		else if (node->nodeName == IsGroupCreatorNode)
		{
			bIsGroupCreator = std::stoi (value);
		}
	}
}

void CstiRingGroupManager::CstiRingGroupMemberInfo::Write (FILE* pFile)
{
	fprintf (pFile, "<%s>\n", NameGet().c_str());
	WriteField (pFile, Description, DescriptionNode);
	WriteField (pFile, LocalPhoneNumber, LocalPhoneNumberNode);
	WriteField (pFile, TollFreePhoneNumber, TollFreePhoneNumberNode);
	WriteField (pFile, std::to_string (eState), StateNode);
	WriteField (pFile, std::to_string (bIsGroupCreator), IsGroupCreatorNode);
	fprintf (pFile, "</%s>\n", NameGet ().c_str ());
}

std::string CstiRingGroupManager::CstiRingGroupMemberInfo::NameGet () const
{
	return NodeName;
}

void CstiRingGroupManager::CstiRingGroupMemberInfo::NameSet (const std::string& name)
{
}

bool CstiRingGroupManager::CstiRingGroupMemberInfo::NameMatch (const std::string& name)
{
	return name == NameGet();
}


// end CstiRingGroupManager.cpp
