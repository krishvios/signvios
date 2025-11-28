/*!
 *  \file CstiBlockListManager.cpp
 *  \brief The purpose of the BlockListManager task is to manage the block list numbers
 *         used for automatically rejecting certain user-specified callers.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiBlockListManager.h"
#include "XMLList.h"
#include "CstiBlockListItem.h"
#include "stiTrace.h"
#include "CstiBlockListMgrResponse.h"
#include "CstiEventQueue.h"
#include <sstream>

using namespace WillowPM;

//
// Constants
//
#define DEFAULT_MAX_BLOCK_LIST_LENGTH 10
#define ABSOLUTE_MAX_BLOCK_LIST_ITEMS 500

// DBG_ERR_MSG is a tool to add file/line infromation to an stiTrace
// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiBlockListDebug, stiTrace ("BlockList: " fmt "\n", ##__VA_ARGS__); )
#define DBG_ERR_MSG(fmt,...) stiDEBUG_TOOL (g_stiBlockListDebug, stiTrace (HERE " ERROR: " fmt "\n", ##__VA_ARGS__); )


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
 * This is the default constructor for the CstiBlockListManager class.
 *
 * \return None
 */
CstiBlockListManager::CstiBlockListManager (
	bool bEnabled,
	CstiCoreService *pCoreService,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam
)
:
	
	m_bEnabled (bEnabled),
	m_bInitialized (false),
	m_pCoreService (pCoreService),
	m_bAuthenticated (estiFALSE),
	m_nMaxLength (DEFAULT_MAX_BLOCK_LIST_LENGTH),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_CallbackFromId (0),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet())
{
	m_signalConnections.push_back (m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		m_eventQueue.PostEvent (std::bind (&CstiBlockListManager::clientAuthenticateHandle, this, result));
	}));

	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream BlockListFile;
	BlockListFile << DynamicDataFolder << "BlockList.xml";

	SetFilename (BlockListFile.str ());
}


/*!
 * This is called to Initialize the function.
 *
 * \param[in] bAuthenticated Whether or not the user is authenticated.
 */
stiHResult CstiBlockListManager::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (stiRESULT_SUCCESS == init ())
	{
		m_bInitialized = true;
		CallbackMessageSend (estiMSG_BLOCK_LIST_CHANGED);
	}

	Unlock ();

	return hResult;
}


/*!\brief Signal callback that is raised when a ClientAuthenticateResult core response is received.
 *
 */
void CstiBlockListManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	DBG_MSG ("CoreResponse eCLIENT_AUTHENTICATE_RESULT (id %d) = %s", clientAuthenticateResult->requestId, clientAuthenticateResult->responseSuccessful ? "OK" : "Err");
	AuthenticatedSet (clientAuthenticateResult->responseSuccessful ? estiTRUE : estiFALSE);
}


/*!
 * Since the block list can only be retreived when the user has been authenticated by core,
* this method informs the block list manager when that state has been reached.
*
* \param[in] bAuthenticated Whether or not the user is authenticated.
*/
stiHResult CstiBlockListManager::AuthenticatedSet (
	EstiBool bAuthenticated)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_bAuthenticated = bAuthenticated;

	if (m_bAuthenticated)
	{
		Refresh();
	}

	Unlock ();

	return hResult;
}

void CstiBlockListManager::Refresh()
{
	Lock();
	
	//
	// Go get the list but only if we have been enabled
	// and we have authentication
	//
	if (m_bEnabled && m_bAuthenticated)
	{
		// Send the core request to get the new list
		auto pRequest = new CstiCoreRequest ();
		pRequest->callListGet(CstiCallList::eBLOCKED, nullptr, 0, ABSOLUTE_MAX_BLOCK_LIST_ITEMS,
			CstiCallList::eNAME, CstiList::SortDirection::DESCENDING, -1, false, false);
		CoreRequestSend (pRequest, nullptr);
	}
	
	Unlock();
}

void CstiBlockListManager::PurgeItems()
{
	Lock();
	
	m_pdoc->ListFree();
	
	startSaveDataTimer();
	
	CallbackMessageSend(estiMSG_BLOCK_LIST_CHANGED, (size_t)0);
	
	Unlock();
}


/*!
 * Sets whether or not the block list is enabled.
 *
 * \param bEnabled Whether or not the block list shall be enabled.
 */
void CstiBlockListManager::EnabledSet(bool bEnabled)
{
	Lock ();

	if (m_bEnabled != bEnabled)
	{
		m_bEnabled = bEnabled;

		if (bEnabled)
		{
			if (m_bAuthenticated)
			{
				// Send the core request to get the new list
				auto pRequest = new CstiCoreRequest ();
				pRequest->callListGet(CstiCallList::eBLOCKED, nullptr, 0, ABSOLUTE_MAX_BLOCK_LIST_ITEMS,
					CstiCallList::eNAME, CstiList::SortDirection::DESCENDING, -1, false, false);
				CoreRequestSend (pRequest, nullptr);
			}

			CallbackMessageSend (estiMSG_BLOCK_LIST_ENABLED);
		}
		else
		{
			CallbackMessageSend (estiMSG_BLOCK_LIST_DISABLED);
		}
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
EstiBool CstiBlockListManager::RemoveRequestId (
	unsigned int unRequestId)
{
	EstiBool bHasBlockListCookie = estiFALSE;

	Lock ();
	for (auto it = m_PendingCookies.begin (); it != m_PendingCookies.end (); ++it)
	{
		if (*it == unRequestId)
		{
			bHasBlockListCookie = estiTRUE;
			m_PendingCookies.erase (it);
			break;
		}
	}
	Unlock ();

	return bHasBlockListCookie;
}


/*!
 * Utility function to perform an item data change.  This is for CoreResponseHandle internal use only!  All
 * other code should go through Add, Item, and Delete functions.
 *
 * For each item in the core response:
 * 	1) If the item exists, it will be modified.
 * 	2) If the item does not exist, it will be added.
 *
 *  \param pCoreResponse the core response to extract items from
 *  \param eMessage the callback message to be sent upon success.
 *
 *  \return the number of items modified
 */
int CstiBlockListManager::CoreListItemsModify (
	CstiCoreResponse *pCoreResponse,
	EstiCmMessage eMessage)
{
	int nNumItemsModified = 0;

	auto pCallList = (CstiCallList *)pCoreResponse->CallListGet();
	CstiCallListItemSharedPtr pCallListItem;

	if (pCoreResponse->ResponseResultGet() == estiOK)
	{
		Lock ();

		if (pCallList)
		{
			// Change the values for the items.
			for (int i = 0; ; ++i)
			{
				pCallListItem = pCallList->ItemGet (i);
				if (!pCallListItem)
				{
					break;
				}
				CstiBlockListItemSharedPtr pBlockListItem = ItemGetById (pCallListItem->ItemIdGet ());
				if (!pBlockListItem)
				{
					pBlockListItem = std::make_shared<CstiBlockListItem> ();
					pBlockListItem->IDSet (pCallListItem->ItemIdGet ());
					m_pdoc->ItemAdd (pBlockListItem);
				}
				pBlockListItem->DescriptionSet (pCallListItem->NameGet ());
				pBlockListItem->PhoneNumberSet (pCallListItem->DialStringGet ());

				if (estiMSG_BLOCK_LIST_CHANGED != eMessage) // This message is only sent once
				{
					auto pResponse = new CstiBlockListMgrResponse (pBlockListItem->IDGet (), pCoreResponse->RequestIDGet ());
					CallbackMessageSend (eMessage, (size_t)pResponse);
				}

				++nNumItemsModified;
			}
		}

		if (0 == nNumItemsModified)
		{
			pCallListItem = pCoreResponse->contextItemGet <CstiCallListItemSharedPtr>();

			if (pCallListItem)
			{
				CstiBlockListItemSharedPtr pBlockListItem = ItemGetById (pCallListItem->ItemIdGet ());

				if (pBlockListItem)
				{
					pBlockListItem->DescriptionSet (pCallListItem->NameGet ());
					pBlockListItem->PhoneNumberSet (pCallListItem->DialStringGet ());

					auto pResponse = new CstiBlockListMgrResponse (pBlockListItem->IDGet (), pCoreResponse->RequestIDGet ());
					CallbackMessageSend (eMessage, (size_t)pResponse);

					++nNumItemsModified;
				}
			}
		}

		if ((estiMSG_BLOCK_LIST_CHANGED == eMessage))
		{
			CallbackMessageSend (eMessage);
		}

		saveToPersistentStorage ();

		Unlock ();
	}
	else
	{
		errorSend(pCoreResponse, eMessage);
	}

	return nNumItemsModified;
}


EstiBool CstiBlockListManager::RemovedUponCommunicationErrorHandle (
	CstiVPServiceResponse *response)
{
	EstiBool bHandled = estiFALSE;

	if (RemoveRequestId (response->RequestIDGet()))
	{
		// Create an item, not to be added, but for the application
		CstiCallListItemSharedPtr pCallListItem = contextItemGet<CstiCallListItemSharedPtr> (response->contextGet());
		std::string itemId;

		if (pCallListItem)
		{
			if (pCallListItem->ItemIdGet())
			{
				itemId = pCallListItem->ItemIdGet();
			}
		}

		errorSignal.Emit(Response::Unknown, stiRESULT_ERROR, response->RequestIDGet(), itemId);

		response->destroy ();

		bHandled = estiTRUE;
	}

	return bHandled;
}


/*!
 * Core responses when retreived by CCI shall be passed to this method for handling.  If
 * the return value is true then the response was one that the block list manager was expecting
 * to receive.  In this case, the response should not be passed on to other objects (such as the UI).
 * The response object shall be deleted by the block list manager.
 *
 * If the return value is false, the response was not handled and the response shall be passed on
 * to other objects for handling.
 *
 * \param[in] pCoreResponse The core response to be handled.
 */
EstiBool CstiBlockListManager::CoreResponseHandle (
	CstiCoreResponse *pCoreResponse)
{
	EstiBool bHandled = estiFALSE;

	switch (pCoreResponse->ResponseGet ())
	{
		case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_ADD_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsModify (pCoreResponse, estiMSG_BLOCK_LIST_ITEM_ADDED);

				pCoreResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_EDIT_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsModify (pCoreResponse, estiMSG_BLOCK_LIST_ITEM_EDITED);

				pCoreResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_GET_RESULT:
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				m_bInitialized = true;
				
				DBG_MSG ("CoreResponse eCALL_LIST_GET_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				if (m_pdoc)
				{
					// Empty existing list entirely first.
					m_pdoc->ListFree ();

					// Add all the new items
					CoreListItemsModify (pCoreResponse, estiMSG_BLOCK_LIST_CHANGED);
				}

				pCoreResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_GET_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsModify (pCoreResponse, estiMSG_BLOCK_LIST_CHANGED);

				pCoreResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
			if (RemoveRequestId (pCoreResponse->RequestIDGet ()))
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_REMOVE_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				auto pCallListItem = pCoreResponse->contextItemGet<CstiCallListItemSharedPtr> ();

				if (pCallListItem && pCoreResponse->ResponseResultGet () == estiOK)
				{
					// IMPORTANT: Create the response before removing the list item, so it adds a reference before the remove.
					//		(if you did this after ItemDestroy, the item would be deleted)
					auto pResponse = new CstiBlockListMgrResponse (pCallListItem->ItemIdGet (), pCoreResponse->RequestIDGet ());
					
					if (pResponse)
					{
						CstiBlockListItemSharedPtr pBlockListItem = ItemGetById (pCallListItem->ItemIdGet ());

						if (pBlockListItem)
						{
							// Find and delete the item from the list.
							for (size_t i = 0; i < m_pdoc->CountGet (); ++i)
							{
								if (pCallListItem->ItemIdGet () == (std::static_pointer_cast<CstiBlockListItem>(m_pdoc->ItemGet (i)))->IDGet ())
								{
									m_pdoc->ItemDestroy (i);
									break;
								}
							}
							
							saveToPersistentStorage ();
						}

						//
						// Event if we didn't find the item we will send up this deleted message because
						// it must have been removed in some other way (list refreshed between deletion request and response?).
						//
						CallbackMessageSend (estiMSG_BLOCK_LIST_ITEM_DELETED, (size_t)pResponse);
					}
					else
					{
						errorSend(pCoreResponse, estiMSG_BLOCK_LIST_ITEM_DELETED);
					}
				}
				else
				{
					errorSend(pCoreResponse, estiMSG_BLOCK_LIST_ITEM_DELETED);
				}

				pCoreResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;
		default:
			break;
	}

	return bHandled;
}


/*!
 * The block list manager shall retreive a new block list if an event indicates that the
 * block list has changed in core.  This may occur if a white list entry has been added
 * or removed.
 *
 * \param[in] pStateNotifyResponse The state notify response to be handled.
 */
EstiBool CstiBlockListManager::StateNotifyEventHandle (
	CstiStateNotifyResponse *pStateNotifyResponse)
{
	EstiBool bHandled = estiFALSE;

	if (pStateNotifyResponse
	 && pStateNotifyResponse->ResponseValueGet () == CstiCallList::eBLOCKED)
	{
		switch (pStateNotifyResponse->ResponseGet ())
		{
			case CstiStateNotifyResponse::eCALL_LIST_CHANGED:
			//NOTE: any of these events causes a complete reload of the list...
			// We could revisit this, however it seemed the best way to go for now.
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD:
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_EDIT:
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE:
			{
		DBG_MSG ("StateNotifyResponse eBLOCKED_LIST_CHANGED");

		// Send the core request to get the new list
		auto pRequest = new CstiCoreRequest ();
		pRequest->callListGet(CstiCallList::eBLOCKED, nullptr, 0, ABSOLUTE_MAX_BLOCK_LIST_ITEMS,
			CstiCallList::eNAME, CstiList::SortDirection::DESCENDING, -1, false, false);
		CoreRequestSend (pRequest, nullptr);

		pStateNotifyResponse->destroy ();
		bHandled = estiTRUE;
				break;
			}
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
void CstiBlockListManager::CallbackMessageSend (
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
* Adds an item to the block list.
*
* The manager will search its list for the number.  If the number is not found then the
* manager will add the number to the list and send a core request to add the number to core.
*
* If the number is found then an error is returned.
*
* If the list already contains the maximum number of entries, an error is returned.
*
* A core request shall be issued to add the blocked to number to core.  The request id for
* this request is returned to the caller.  Once the request has been completed the block list
* manager shall call the callback with a message and the request id indicating the request is
* complete.  The caller of this method may use this to monitor the completion of
* the asynchronous process.
*
* \param[in] pszBlockedNumber The number to add to the list.
* \param[in] pszDescription A description to associate to the number.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiBlockListManager::ItemAdd (
	const char *pszBlockedNumber,
	const char *pszDescription,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiBlockListItemSharedPtr pBlockListItem;
	CstiCallListItemSharedPtr pCallListItem;
	CstiCoreRequest *pRequest = nullptr;

	Lock ();

	stiTESTCOND (!m_pCoreService->isOfflineGet (), stiRESULT_OFFLINE_ACTION_NOT_ALLOWED);

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && m_bEnabled && m_pdoc, stiRESULT_ERROR);

	// Verify that the number is not already in the blocked list.
	pBlockListItem = ItemGet (pszBlockedNumber);

	stiTESTCOND (!pBlockListItem, stiRESULT_BLOCKED_NUMBER_EXISTS);

	// Verify that the blocked list is not already full.
	stiTESTCOND (m_pdoc->CountGet () < m_nMaxLength, stiRESULT_BLOCK_LIST_FULL);

	// Create a call list item.
	pCallListItem = std::make_shared<CstiCallListItem> ();
	pCallListItem->NameSet (pszDescription);
	pCallListItem->DialStringSet (pszBlockedNumber);
	pCallListItem->DialMethodSet (estiDM_BY_DS_PHONE_NUMBER);

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->callListItemAdd (*pCallListItem, CstiCallList::eBLOCKED);
	pRequest->contextItemSet (pCallListItem);
	hResult = CoreRequestSend (pRequest, punRequestId);

STI_BAIL:

	Unlock ();

	return hResult;
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
stiHResult CstiBlockListManager::CoreRequestSend (
	CstiCoreRequest *poCoreRequest,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unInternalCoreRequestId = 0;

	stiTESTCOND(m_pCoreService, stiRESULT_ERROR);

	poCoreRequest->RemoveUponCommunicationError () = estiTRUE;
	hResult = m_pCoreService->RequestSend (
		poCoreRequest,
		&unInternalCoreRequestId);

	stiTESTRESULT();

	m_PendingCookies.push_back (unInternalCoreRequestId);

	DBG_MSG ("Core request %d sent.", unInternalCoreRequestId);

	if (punRequestId)
	{
		*punRequestId = unInternalCoreRequestId;
	}

STI_BAIL:

	return hResult;
} // end CstiBlockListManager::CoreRequestSend


/*!
* Deletes an item from the block list.
*
* The manager will search its list for the number.  If the number is found then the manager
* will remove the number from the list and send a core request to remove the number from
* core.
*
* A core request shall be issued to delete the blocked number from core.  The request id
* for this request is returned to the caller.  Once the request has been completed the
* block list manager shall call the callback with a message and the request id indicating
* the request is complete.  The caller of this method may use this to monitor the completion
* of the asynchronous process.
*
* \param[in] pszBlockedNumber The number to remove from the block list.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiBlockListManager::ItemDelete (
	const char *pszBlockedNumber,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiBlockListItemSharedPtr pItem;
	int nCount = 0;
	bool bMatch = false;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && m_bEnabled && m_pdoc, stiRESULT_ERROR);

	nCount = m_pdoc->CountGet ();

	for (int i = 0; i < nCount; i++)
	{
		pItem = std::static_pointer_cast<CstiBlockListItem> (m_pdoc->ItemGet (i));

		pItem->ValueCompare (pszBlockedNumber, CstiBlockListItem::eDIAL_STRING, bMatch);

		if (bMatch)
		{
			// Create a call list item.
			CstiCallListItemSharedPtr pCallListItem = std::make_shared<CstiCallListItem> ();
			pCallListItem->ItemIdSet (pItem->IDGet ().c_str ());
			auto description = pItem->DescriptionGet ();
			pCallListItem->NameSet (description.c_str ());
			auto phoneNumber = pItem->PhoneNumberGet ();
			pCallListItem->DialStringSet (phoneNumber.c_str ());
			pCallListItem->DialMethodSet (estiDM_BY_DS_PHONE_NUMBER);

			// Send the core request
			auto pRequest = new CstiCoreRequest ();
			pRequest->callListItemRemove (*pCallListItem, CstiCallList::eBLOCKED);
			pRequest->contextItemSet (pCallListItem);
			hResult = CoreRequestSend (pRequest, punRequestId);

			break;
		}
	}

	stiTESTCOND (bMatch, stiRESULT_ERROR);

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Changes the block list entry to the new values specified.
*
* The manager will search its list for the old number.  If the old number is found then the
* manager will replace the old number with the new number and send a core request to core to
* update the number.
*
* A core request shall be issued to edit the blocked number in core.  The request id for this
* request is returned to the caller.  Once the request has been completed the block list
* manager shall call the callback with a message and the request id indicating the request
* is complete.  The caller of this method may use this to monitor the completion of the
* asynchronous process.
*
* \param[in] pszItemId The Database ID of the blocklist item you wish to edit.
* \param[in] pszNewBlockedNumber The new number.  This may be NULL if the number is not to be
* changed.
* \param[in] pszNewDescription The new description for the number.  This may be NULL if the
* description is not be changed.
* \param[out] punRequestId A request id for this operation.
*/
stiHResult CstiBlockListManager::ItemEdit (
	const char *pszItemId,
	const char *pszNewBlockedNumber,
	const char *pszNewDescription,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiBlockListItemSharedPtr pBlockListItem;
	CstiCallListItemSharedPtr pCallListItem;
	CstiCoreRequest *pRequest = nullptr;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && m_bEnabled && m_pdoc, stiRESULT_ERROR);

	//
	// Make sure, if we are changing the number, that the new number doesn't already exist.
	// If we find an item with the new number and it isn't the item we are looking for then
	// throw an error.
	//
	pBlockListItem = ItemGet (pszNewBlockedNumber);

	if (pBlockListItem
	 && pBlockListItem->IDGet () != pszItemId)
	{
		stiTHROW (stiRESULT_ERROR);
	}

	//
	// If we get here and we have an item it must be the item we are looking for.  If we
	// don't have an item then get by Item Id.
	//
	if (!pBlockListItem)
	{
		pBlockListItem = ItemGetById (pszItemId);
		stiTESTCOND (pBlockListItem, stiRESULT_ERROR);
	}

	// Create a call list item.
	pCallListItem = std::make_shared<CstiCallListItem> ();

	{
		pCallListItem->ItemIdSet (pszItemId);
		auto oldDescription = pBlockListItem->DescriptionGet();
		pCallListItem->NameSet (pszNewDescription ? pszNewDescription : oldDescription.c_str ());
		auto oldPhoneNumber = pBlockListItem->PhoneNumberGet();
		pCallListItem->DialStringSet (pszNewBlockedNumber ? pszNewBlockedNumber : oldPhoneNumber.c_str ());
		pCallListItem->DialMethodSet (estiDM_BY_DS_PHONE_NUMBER);
	}

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->callListItemEdit (*pCallListItem, CstiCallList::eBLOCKED);
	pRequest->contextItemSet (pCallListItem);
	hResult = CoreRequestSend (pRequest, punRequestId);

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Finds a blocked list entry by phone number.
*
* The manager will search its list for the number.  If the number is found then eestiTRUE
* is returned.  Otherwise, eestiFALSE is returned.
*
* \param pszBlockedNumber The phone number used to identify the entry
* \param[out] pDescription The description associated to the entry.
*/
bool CstiBlockListManager::ItemGetByNumber (
	const char *pszBlockedNumber,
	std::string *pItemId,
	std::string *pDescription) const
{
	CstiBlockListItemSharedPtr pBlockListItem;

	Lock ();
	// Can we even do this?
	if (m_bEnabled)
	{
		pBlockListItem = ItemGet (pszBlockedNumber);

		// Fill out the item id string if the caller gave us one.
		if (pItemId)
		{
			if (pBlockListItem)
			{
				*pItemId = pBlockListItem->IDGet ();
			}
			else
			{
				pItemId->clear ();
			}
		}

		// Fill out the description string if the caller gave us one.
		if (pDescription)
		{
			if (pBlockListItem)
			{
				*pDescription = pBlockListItem->DescriptionGet ();
			}
			else
			{
				pDescription->clear ();
			}
		}
	}

	Unlock ();

	return pBlockListItem ? estiTRUE : estiFALSE;
}


/*!
 * Finds a blocked list entry by item id.
 *
 * The manager will search its list for the id.
 *
 * \param[in] nId The item id to be returned.
 * \return the item found or NULL
 */
CstiBlockListItemSharedPtr CstiBlockListManager::ItemGetById (
	const char *pszItemId) const
{
	CstiBlockListItemSharedPtr pBlockListItem;

	Lock ();

	// Find the item with the matching id.
	if (m_bEnabled && m_pdoc)
	{
		int nCount = m_pdoc->CountGet ();
		CstiBlockListItemSharedPtr pItem;

		for (int i = 0; i < nCount; i++)
		{
			pItem = std::static_pointer_cast<CstiBlockListItem> (m_pdoc->ItemGet (i));
			if (pszItemId == pItem->IDGet ())
			{
				pBlockListItem = pItem;
				break;
			}
		}
	}

	Unlock ();

	return pBlockListItem;
}


/*!
 * Finds a blocked list entry by item id.
 *
 * The manager will search its list for the id.  If the number is found then estiTRUE
 * is returned.  Otherwise, estiFALSE is returned.
 *
 * \param[in] nId The item id to be returned.
 * \param[out] pBlockedNumber The phone number associated to the entry.
 * \param[out] pszDescription The description associated to the entry.
 */
bool CstiBlockListManager::ItemGetById (
	const char *pszItemId,
	std::string *pBlockedNumber,
	std::string *pDescription) const
{
	CstiBlockListItemSharedPtr pBlockListItem = ItemGetById (pszItemId);

	if (pDescription)
	{
		pDescription->clear ();
	}

	if (pBlockedNumber)
	{
		pBlockedNumber->clear ();
	}

	// Set return strings based on the item found.
	if (pBlockListItem)
	{
		if (pDescription)
		{
			*pDescription = pBlockListItem->DescriptionGet ();
		}

		if (pBlockedNumber)
		{
			*pBlockedNumber = pBlockListItem->PhoneNumberGet ();
		}
	}

	return pBlockListItem ? estiTRUE : estiFALSE;
}


/*!
 * Finds a blocked list entry by phone number.
 *
 * Returns the item found or NULL.
 *
 * \param pszBlockedNumber The phone number used to identify the entry
 */
CstiBlockListItemSharedPtr CstiBlockListManager::ItemGet (
	const char *pszBlockedNumber) const
{
	CstiBlockListItemSharedPtr pBlockListItem;

	if (pszBlockedNumber && pszBlockedNumber[0] != '\0')
	{
		Lock ();
		int nCount = m_pdoc->CountGet ();
		CstiBlockListItemSharedPtr pItem;

		for (int i = 0; i < nCount; i++)
		{
			pItem = std::static_pointer_cast<CstiBlockListItem> (m_pdoc->ItemGet (i));

			bool bMatch = false;
			pItem->ValueCompare (pszBlockedNumber, CstiBlockListItem::eDIAL_STRING, bMatch);
			if (bMatch)
			{
				pBlockListItem = pItem;
				break;
			}
		}
		Unlock ();
	}

	return pBlockListItem;
}


/*!
* Finds a blocked list entry by index.
*
* This method is used to iterate through the list.  An object should lock the list before
* iterating through the list to protect the list from changing during the iteration process.
*
* \param nIndex The index used to identify the entry
* \param[out] pBlockedNumber The phone number associated for the entry at the provided index
* \param[out] pDescription The description associated to the blocked number.
*/
stiHResult CstiBlockListManager::ItemGetByIndex (
	int nIndex,
	std::string *pItemId,
	std::string *pBlockedNumber,
	std::string *pDescription) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = false;

	pItemId->clear ();
	pBlockedNumber->clear ();
	pDescription->clear ();

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bEnabled && m_pdoc, stiRESULT_ERROR);

	// Is the index within range?
	if ((unsigned int)nIndex < m_pdoc->CountGet ())
	{
		auto pBlockListItem = std::static_pointer_cast<CstiBlockListItem> (m_pdoc->ItemGet (nIndex));
		if (pBlockListItem)
		{
			*pBlockedNumber = pBlockListItem->PhoneNumberGet();

			*pDescription = pBlockListItem->DescriptionGet();

			*pItemId = pBlockListItem->IDGet ();

			bFound = true;
		}
	}

	stiTESTCOND_NOLOG (bFound, stiRESULT_ERROR);

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Sets the maximum number of entries allowed in the list.
*
* The block list manager is informed of the maxium length through this method. Setting a
* maximum length shorter than the current list length shall succeed.  Entries beyond the
* maximum shall be maintained.  New entries shall be disallowed.
*
* \param nMaxLength The maximum number of entries allowed.
*/
stiHResult CstiBlockListManager::MaximumLengthSet (
	int nMaxLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_nMaxLength = std::min (nMaxLength, ABSOLUTE_MAX_BLOCK_LIST_ITEMS);
	Unlock ();

	return hResult;
}


/*!
 * XMLManager XML->Object conversion routine.
 *
 * \param pNode The XML node we are being called upon to convert.
 */
WillowPM::XMLListItemSharedPtr CstiBlockListManager::NodeToItemConvert (
	IXML_Node *pNode)
{
	CstiBlockListItemSharedPtr pItem;

	// pNode should point to "<contact>" initially

	const char *pValue = nullptr;

	pItem = std::make_shared<CstiBlockListItem> ();

	pNode = ixmlNode_getFirstChild (pNode);   // the first subtag

	while  (pNode)
	{
		const char *pTagName = ixmlNode_getNodeName (pNode);
		pValue =  (const char *)ixmlNode_getNodeValue (ixmlNode_getFirstChild (pNode));

		if (pValue)
		{
			if  (strcmp (pTagName, g_szIdTag) == 0)
			{
				pItem->IDSet (pValue);
			}
			else if  (strcmp (pTagName, g_szDescriptionTag) == 0)
			{
				pItem->StringSet (pValue, CstiBlockListItem::eDESCRIPTION);
			}
			else if  (strcmp (pTagName, g_szDialStringTag) == 0)
			{
				pItem->StringSet (pValue, CstiBlockListItem::eDIAL_STRING);
			}
			else
			{
				// unknown field
			}
		}

		pNode = ixmlNode_getNextSibling (pNode);
	}

	return pItem;
}


///
/// \brief Determines whether the calling number is blocked
///
bool CstiBlockListManager::CallBlocked (const char *pszNumber)
{
	return m_bInitialized && m_bEnabled && ItemGet (pszNumber);
}

IstiBlockListManager::Response CstiBlockListManager::coreResponseConvert(CstiCoreResponse::EResponse responseType)
{
	Response response = Response::Unknown;

	switch (responseType)
	{
	case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
		response = Response::AddError;
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
		response = Response::EditError;
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
		response = Response::DeleteError;
		break;

	case CstiCoreResponse::eCALL_LIST_GET_RESULT:
	case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
		response = Response::GetListError;
		break;

	default:
		DBG_ERR_MSG ("Unknown BlockList error (%d) from core.", responseType);
	}

	return response;
}


void CstiBlockListManager::errorSend(
		CstiCoreResponse *coreResponse,
		EstiCmMessage message)
{
	Response response = Response::Unknown;
	stiHResult errorCode = stiRESULT_ERROR;
	unsigned int requestID = coreResponse->RequestIDGet ();
	std::string itemID;

	auto pCallList = (CstiCallList *)coreResponse->CallListGet();
	CstiCallListItemSharedPtr pCallListItem;

	// Let's look for an itemID to send with the error.
	// If the response contains a list, take the first entry
	if (pCallList)
	{
		pCallListItem = pCallList->ItemGet (0);
	}

	// If we didn't find a list, look in the context.
	if (!pCallListItem)
	{
		pCallListItem = coreResponse->contextItemGet<CstiCallListItemSharedPtr> ();
	}

	// If we found a block list item, extract the itemID.
	if (pCallListItem)
	{
		if (pCallListItem->ItemIdGet())
		{
			itemID = pCallListItem->ItemIdGet();
		}
	}

	// Determine error type
	if (coreResponse->ErrorGet () == CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED)
	{
		errorCode = stiRESULT_OFFLINE_ACTION_NOT_ALLOWED;
	}

	if (coreResponse->ErrorGet () == CstiCoreResponse::eBLOCK_LIST_ITEM_DENIED)
	{
		response = Response::Denied;
	}
	else
	{
		response = coreResponseConvert(coreResponse->ResponseGet());
	}

	errorSignal.Emit(response, errorCode, requestID, itemID);
}

ISignal<IstiBlockListManager::Response, stiHResult, unsigned int, const std::string&>& WillowPM::CstiBlockListManager::errorSignalGet ()
{
	return errorSignal;
}

// end CstiBlockListManager.cpp
