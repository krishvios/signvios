/*!
 *  \file CstiCallHistoryManager.cpp
 *  \brief The purpose of the CallHistoryManager task is to manage all of the
 *         call lists and update them as needed
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiCallHistoryManager.h"
#include "CstiCallHistoryMgrResponse.h"
#include "CstiCallHistoryItem.h"
#include "CstiCallList.h"
#include "stiTrace.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "CstiStateNotifyResponse.h"
#include "CstiCoreService.h"
#include "IstiContactManager.h"
#include "CstiEventQueue.h"

//
// Constants
//


// DBG_ERR_MSG is a tool to add file/line infromation to an stiTrace
// 	This next bit is a neat trick to get it to print the file reference (Do not "optimize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (0, stiTrace ("CallHistory: " fmt "\n", ##__VA_ARGS__); )
#define DBG_ERR_MSG(fmt,...) stiDEBUG_TOOL (1, stiTrace (HERE " ERROR: " fmt "\n", ##__VA_ARGS__); )

using namespace WillowPM;


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
 * This is the default constructor for the CstiCallHistoryManager class.
 *
 * \return None
 */
CstiCallHistoryManager::CstiCallHistoryManager (
	bool bEnabled,
	CstiCoreService *pCoreService,
	IstiContactManager *pContactMgr,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam
)
:
	
	m_bEnabled (bEnabled),
	m_bInitialized (false),
	m_pCoreService (pCoreService),
	m_bAuthenticated (estiFALSE),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_CallbackFromId (0),
	m_LastMissedTime(0),
	m_unOnlyNewItemsCookie(0),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet()),
	m_pContactMgr(pContactMgr)

{
	m_pCallLists.resize(CstiCallList::eTYPE_LAST);

	m_pCallLists[CstiCallList::eDIALED] = std::make_shared<CstiCallHistoryList>(CstiCallList::eDIALED);
	m_pCallLists[CstiCallList::eANSWERED] = std::make_shared<CstiCallHistoryList>(CstiCallList::eANSWERED);
	m_pCallLists[CstiCallList::eMISSED] = std::make_shared<CstiCallHistoryList>(CstiCallList::eMISSED);
	m_pCallLists[CstiCallList::eRECENT] = std::make_shared<CstiCallHistoryList>(CstiCallList::eRECENT);

	PropertyManager::getInstance()->propertyGet(LAST_MISSED_TIME,
		(int*)&m_LastMissedTime, PropertyManager::Temporary);

	m_signalConnections.push_back (m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		m_eventQueue.PostEvent (std::bind (&CstiCallHistoryManager::clientAuthenticateHandle, this, result));
	}));

	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream CallHistoryListFile;
	CallHistoryListFile << DynamicDataFolder << "CallHistory.xml";
	SetFilename (CallHistoryListFile.str ());
}


/*! \brief Destructor
*
* This is the default destructor for the CstiCallHistoryManager class.
*
* \return None
*/
CstiCallHistoryManager::~CstiCallHistoryManager ()
{
	m_pdoc->ListFree ();
	m_pCallLists[CstiCallList::eRECENT].reset ();
}


/*!
 * This is called to Initialize the function.
 *
 * \param[in] bAuthenticated Whether or not the user is authenticated.
 */
stiHResult CstiCallHistoryManager::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	hResult = XMLManager::init ();

	if (stiRESULT_CODE(hResult) == stiRESULT_UNABLE_TO_OPEN_FILE)
	{
		m_pdoc->ItemAdd (m_pCallLists[CstiCallList::eANSWERED]);
		m_pdoc->ItemAdd (m_pCallLists[CstiCallList::eMISSED]);
		m_pdoc->ItemAdd (m_pCallLists[CstiCallList::eDIALED]);
		m_pCallLists[CstiCallList::eANSWERED]->NeedsUpdateSet (true);
		m_pCallLists[CstiCallList::eMISSED]->NeedsUpdateSet (true);
		m_pCallLists[CstiCallList::eDIALED]->NeedsUpdateSet (true);
	}

	// Initialize unviewed count
	m_pCallLists[CstiCallList::eMISSED]->UnviewedItemCountCalculate (m_LastMissedTime);

	m_bInitialized = true;

	Unlock ();

	return hResult;
}



stiHResult CstiCallHistoryManager::Lock () const
{
	lock ();
	return stiRESULT_SUCCESS;
}


void CstiCallHistoryManager::Unlock () const
{
	unlock ();
}

CstiCallHistoryItemSharedPtr CstiCallHistoryManager::ItemCreate()
{
	CstiCallHistoryItemSharedPtr spItem = std::make_shared<CstiCallHistoryItem>();
	return spItem;
}


/*!\brief Signal callback that is raised when a ClientAuthenticateResult core response is received.
 *
 */
void CstiCallHistoryManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
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
stiHResult CstiCallHistoryManager::AuthenticatedSet (
	EstiBool bAuthenticated)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_bAuthenticated = bAuthenticated;
	if (m_bAuthenticated == estiTRUE)
	{
		// Notify the UI that we have loaded the call lists.
		CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)CstiCallList::eANSWERED);
		CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)CstiCallList::eDIALED);
		CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)CstiCallList::eMISSED);
		RecentListRebuild();

		Refresh();
	}
	Unlock ();

	return hResult;
}


/*!
 * Sets whether or not the call lists are enabled.
 *
 * \param bEnabled Whether or not the call lists shall be enabled.
 */
void CstiCallHistoryManager::EnabledSet(bool bEnabled)
{
	Lock ();

	if (m_bEnabled != bEnabled)
	{
		m_bEnabled = bEnabled;

		if (bEnabled)
		{
			if (m_bAuthenticated)
			{
				// Update the call lists
				ListUpdateAll();
			}

			CallbackMessageSend (estiMSG_CALL_LIST_ENABLED);
		}
		else
		{
			CallbackMessageSend (estiMSG_CALL_LIST_DISABLED);
		}
	}

	Unlock ();
}


void CstiCallHistoryManager::Refresh ()
{
	if (m_bEnabled && m_bAuthenticated)
	{
		int i = CstiCallList::eANSWERED;
		for (; i <= CstiCallList::eDIALED; i++)
		{
			auto  eCallListType = (CstiCallList::EType)i;
			if (m_pCallLists[eCallListType]->NeedsUpdateGet ())
			{
				ListUpdate (eCallListType);
			}
			else if (m_pCallLists[eCallListType]->NeedsNewItemsGet ())
			{
				auto  pRequest = new CstiCoreRequest ();
				pRequest->callListCountGet (eCallListType, false, m_pCallLists[eCallListType]->LastRetrievalTimeGet());
				CoreRequestSend (pRequest, nullptr, eCallListType);
			}
		}
	}
}


/*!
 * Utility function to determine if the request id of the given core
 * response was one that we were waiting for.  If so, the request
 * id will be removed from the list.
 *
 * \param pCoreResponse The core request which will be checked.
 *
 * \return The call list type that made the request, eTYPE_UNKNOWN if not found
 */
CstiCallList::EType CstiCallHistoryManager::RemoveRequestId (
	unsigned int unRequestId)
{
	CstiCallList::EType eCallListType = CstiCallList::eTYPE_UNKNOWN;

	Lock ();

	for (int i = 0; i < CstiCallList::eTYPE_LAST; i++)
	{
		for (auto it = m_PendingCookies[i].begin (); it != m_PendingCookies[i].end (); ++it)
		{
			if (*it == unRequestId)
			{
				eCallListType = (CstiCallList::EType) i;
				m_PendingCookies[i].erase (it);
				break;
			}
		}
	}
	Unlock ();

	return eCallListType;
}

/*!
 * Utility function to convert a CstiCallListItem to a CstiCallHistoryItem.
 * We need to look the phone number up in the contact list to add extra info.
 *
 * \param eCallListType The call list that the item belongs to
 * \param pCoreItem The item to be converted
 *
 * \return A smart pointer to the newly created CstiCallHistoryItem
 */
CstiCallHistoryItemSharedPtr CstiCallHistoryManager::CoreListItemConvert(
	CstiCallList::EType eCallListType,
	const CstiCallListItemSharedPtr &coreItem)
{
	CstiCallHistoryItemSharedPtr spHistoryItem = std::make_shared<CstiCallHistoryItem> ();
	*spHistoryItem = *coreItem;

	// We add a few fields that Core doesn't store in Call History
	spHistoryItem->CallListTypeSet(eCallListType);
	if (estiTRUE == spHistoryItem->InSpeedDialGet())
	{
		IstiContactSharedPtr pContact;
		m_pContactMgr->getContactByPhoneNumber(spHistoryItem->DialStringGet(), &pContact);

		if (pContact)
		{
			CstiContactListItem::EPhoneNumberType eNumberType;
			pContact->PhoneNumberTypeGet(spHistoryItem->DialStringGet(), &eNumberType);
			spHistoryItem->PhoneNumberTypeSet((int)eNumberType);

			std::string imageId;
			pContact->PhotoGet(&imageId);
			spHistoryItem->ImageIdSet(imageId);
		}
	}

	return spHistoryItem;
}

/*!
 * Utility function to handle an Add response.  This is for CoreResponseHandle internal use only!
 *
 *  \param pCoreResponse the core response to extract items from
 *  \param eCallListType The Call List type that this message is for.
 *
 *  \return stiHResult
 */
stiHResult CstiCallHistoryManager::CoreListItemsAdd (
	CstiCoreResponse *pCoreResponse,
	CstiCallList::EType eCallListType)
{
	auto pCoreList = (CstiCallList *)pCoreResponse->CallListGet();
	CstiCallHistoryItemSharedPtr spNewHistoryItem;
	CstiCallHistoryItemSharedPtr spHistoryItem;

	spHistoryItem = pCoreResponse->contextItemGet<CstiCallHistoryItemSharedPtr> ();

	if (pCoreResponse->ResponseResultGet() == estiOK)
	{
		Lock ();

		if (pCoreList)
		{
			CstiCallListItemSharedPtr pCoreItem = pCoreList->ItemGet (0);
			if (pCoreItem)
			{
				spNewHistoryItem = CoreListItemConvert(eCallListType, pCoreItem);

				// If the caller ID was blocked (stored in the original call history object)
				// then make sure the number is removed from the new history object.
				// Note: these lines can be removed once core stops sending us the
				// dial string back in the core response to the add request.
				if (spHistoryItem->BlockedCallerIDGet ())
				{
					spNewHistoryItem->DialStringSet("");
				}

				spNewHistoryItem->vrsCallIdSet (spHistoryItem->vrsCallIdGet ());
				spNewHistoryItem->m_agentHistory = spHistoryItem->m_agentHistory;
				spNewHistoryItem->callIndexSet (spHistoryItem->callIndexGet ());

				// List is ordered newest to oldest; insert new items at the front of the list
				m_pCallLists[eCallListType]->ItemInsert(spNewHistoryItem, 0);
			}

			// Update the Recent list as well
			if (CstiCallList::eDIALED == eCallListType ||
				CstiCallList::eANSWERED == eCallListType ||
				CstiCallList::eMISSED == eCallListType)
			{
				if (m_pCallLists[eCallListType]->CountGet() < m_pCallLists[eCallListType]->MaxCountGet())
				{
					CstiCallHistoryItemSharedPtr spRecentItem = std::make_shared<CstiCallHistoryItem> ();
					*spRecentItem = *spNewHistoryItem;
					ItemInsert(CstiCallList::eRECENT, spRecentItem);
					
#ifdef stiCALL_HISTORY_COMPRESSED
					RecentListCompress();
#endif
				}
				else
				{
					RecentListRebuild();
				}
			}

//            CstiCallHistoryMgrResponse *pResponse = new CstiCallHistoryMgrResponse (eCallListType, spHistoryItem, pCoreResponse->RequestIDGet ());
//            CallbackMessageSend (estiMSG_CALL_LIST_ITEM_ADDED, (size_t)pResponse);
			CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)eCallListType);
			if (spNewHistoryItem != nullptr)
			{
				callHistoryAdded.Emit (spNewHistoryItem);
			}
			startSaveDataTimer ();
		}

		Unlock ();
	}
	else
	{
		auto pResponse = new CstiCallHistoryMgrResponse (
				eCallListType, spHistoryItem, pCoreResponse->RequestIDGet ());

		CallbackMessageSend (estiMSG_CALL_LIST_ITEM_ADD_ERROR, (size_t)pResponse);
	}

	return stiRESULT_SUCCESS;
}


/*!
 * Utility function to handle a Remove response.  This is for CoreResponseHandle internal use only!
 *
 *  \param pCoreResponse the core response to extract items from
 *  \param eCallListType The Call List type that this message is for.
 *
 *  \return stiHResult
 */
stiHResult CstiCallHistoryManager::CoreListItemsRemove (
	CstiCoreResponse *coreResponse,
	CstiCallList::EType callListType)
{
	CstiCallHistoryItemSharedPtr historyItem;

	if (coreResponse->ResponseResultGet() == estiOK)
	{
		historyItem = coreResponse->contextItemGet<CstiCallHistoryItemSharedPtr> ();

		if (historyItem)
		{
			Lock ();
#ifdef stiCALL_HISTORY_COMPRESSED
			if (CstiCallList::eRECENT == callListType && historyItem->IsGroup ())
			{
				for (unsigned int i = 0; i < historyItem->GroupedItemsCountGet (); i++)
				{
					// Get the group item.
					auto subCallItem = historyItem->GroupedItemGet (i);
					CstiCallHistoryListSharedPtr callList = m_pCallLists[subCallItem->CallListTypeGet()];
					callList->ItemRemoveByID (subCallItem->ItemIdGet ());
				}
				RecentListRebuild ();
			}
			else
#endif
			{
				CstiCallHistoryListSharedPtr callList = m_pCallLists[historyItem->CallListTypeGet()];
				callList->ItemRemoveByID (historyItem->ItemIdGet ());
			}


			// Update the Recent list as well
			if (CstiCallList::eDIALED == callListType ||
				CstiCallList::eANSWERED == callListType ||
				CstiCallList::eMISSED == callListType)
			{
#ifdef stiCALL_HISTORY_COMPRESSED
				// We might be deleting a single item or a child of a group.
				// After deleting it, we may need to combine remaining items into new groups.
				// While not the most efficient method, the best way to guarantee we
				// don't mess up the list is to just rebuild the Recent list from scratch.

				RecentListRebuild();
#else
				CstiCallHistoryListSharedPtr callList = m_pCallLists[CstiCallList::eRECENT];
				callList->ItemRemoveByID (historyItem->ItemIdGet ());
#endif
			}

//            CstiCallHistoryMgrResponse *pResponse = new CstiCallHistoryMgrResponse (eCallListType, spHistoryItem, pCoreResponse->RequestIDGet ());
//            CallbackMessageSend (estiMSG_CALL_LIST_ITEM_DELETED, (size_t)pResponse);
			CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)callListType);
			startSaveDataTimer ();

			Unlock ();
		}
		else
		{
			CallbackMessageSend (estiMSG_CALL_LIST_ITEM_DELETE_ERROR);
		}
	}
	else
	{
		// Try and get any item associated with the failed request.
		historyItem = coreResponse->contextItemGet<CstiCallHistoryItemSharedPtr> ();

		auto response = new CstiCallHistoryMgrResponse (
				callListType, historyItem, coreResponse->RequestIDGet ());

		CallbackMessageSend (estiMSG_CALL_LIST_ITEM_DELETE_ERROR, (size_t)response);
	}

	return stiRESULT_SUCCESS;
}

/*!
 * Utility function to handle a Get response.  This is for CoreResponseHandle internal use only!
 *
 *  \param pCoreResponse the core response to extract items from
 *  \param eCallListType The Call List type that this message is for.
 *
 *  \return stiHResult
 */
stiHResult CstiCallHistoryManager::CoreListItemsGet (
	CstiCoreResponse *pCoreResponse,
	CstiCallList::EType eCallListType)
{
	auto pCoreList = (CstiCallList *)pCoreResponse->CallListGet();
	CstiCallHistoryItemSharedPtr spHistoryItem;

	if (pCoreResponse->ResponseResultGet() == estiOK)
	{
		Lock ();

		// Clear the list, unless we're only retrieving the new items
		if (m_unOnlyNewItemsCookie != pCoreResponse->RequestIDGet())
		{
			m_pCallLists[eCallListType]->ListFree();
		}

		if (pCoreList)
		{
			if (m_unOnlyNewItemsCookie == pCoreResponse->RequestIDGet())
			{
				for (auto i = pCoreList->CountGet() - 1; i < pCoreList->CountGet(); --i)
				{
					CstiCallListItemSharedPtr pCoreItem = pCoreList->ItemGet (i);
					spHistoryItem = CoreListItemConvert(eCallListType, pCoreItem);

					// New items get inserted at the head of the list.
					m_pCallLists[eCallListType]->ItemInsert(spHistoryItem, 0);
				}
			}
			else
			{
				for (unsigned int i = 0; i < pCoreList->CountGet(); ++i)
				{
					CstiCallListItemSharedPtr pCoreItem = pCoreList->ItemGet (i);
					spHistoryItem = CoreListItemConvert(eCallListType, pCoreItem);
					m_pCallLists[eCallListType]->ItemAdd(spHistoryItem);
				}
			}

			// It's easier to completely rebuild the recent list, rather than
			// try to determine which ones were deleted and which ones were added.
			if (CstiCallList::eDIALED == eCallListType ||
				CstiCallList::eANSWERED == eCallListType ||
				CstiCallList::eMISSED == eCallListType)
			{
				RecentListRebuild();
			}
		}

		CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)eCallListType);
		startSaveDataTimer ();

		Unlock ();
	}
	else
	{
		auto pResponse = new CstiCallHistoryMgrResponse (
				eCallListType, spHistoryItem, pCoreResponse->RequestIDGet ());

		CallbackMessageSend (estiMSG_CALL_LIST_ERROR, (size_t)pResponse);
	}

	// NOTE: moved clearing these flags outside of (ResponseResultGet() == estiOK) (bug 24144)
	m_pCallLists[eCallListType]->NeedsUpdateSet(false);
	m_pCallLists[eCallListType]->NeedsNewItemsSet(false);

	if (m_unOnlyNewItemsCookie == pCoreResponse->RequestIDGet ())
	{
		m_unOnlyNewItemsCookie = 0;
	}

	return stiRESULT_SUCCESS;
}


EstiBool CstiCallHistoryManager::RemovedUponCommunicationErrorHandle (
	CstiVPServiceResponse *response)
{
	EstiBool bHandled = estiFALSE;

	CstiCallList::EType eCallListType = RemoveRequestId (response->RequestIDGet());
	if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
	{

		CstiCallHistoryItemSharedPtr spCallListItem = contextItemGet<CstiCallHistoryItemSharedPtr>(
			response->contextGet()
			);

		// Create an item, not to be added, but for the application
		if (spCallListItem)
		{
			auto pResponse = new CstiCallHistoryMgrResponse (
				eCallListType, spCallListItem, response->RequestIDGet());

			CallbackMessageSend (estiMSG_CALL_LIST_ERROR, (size_t)pResponse);

			// NOTE: Clear these flags upon communication error (bug 24144)
			m_pCallLists[eCallListType]->NeedsUpdateSet(false);
			m_pCallLists[eCallListType]->NeedsNewItemsSet(false);
		}
		else
		{
			CallbackMessageSend (estiMSG_CALL_LIST_ERROR, 0);
		}

		response->destroy ();

		bHandled = estiTRUE;
	}

	return bHandled;
}


/*!
 * Core responses when retreived by CCI shall be passed to this method for handling.  If
 * the return value is true then the response was one that the call history manager was expecting
 * to receive.  In this case, the response should not be passed on to other objects (such as the UI).
 * The response object shall be deleted by the call history manager
 *
 * If the return value is false, the response was not handled and the response shall be passed on
 * to other objects for handling.
 *
 * \param pCoreResponse The core response to be handled.
 */
EstiBool CstiCallHistoryManager::CoreResponseHandle (
	CstiCoreResponse *pCoreResponse)
{
	EstiBool bHandled = estiFALSE;
	CstiCallList::EType eCallListType;

	switch (pCoreResponse->ResponseGet ())
	{
		case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_ADD_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsAdd (pCoreResponse, eCallListType);

				bHandled = estiTRUE;
			}
			break;

		case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_EDIT_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				// Because we are editing an item and all modes are already done on the 
				// call history item we just need notify the UI if there was an error and
				// mark this response as handled.
				if (pCoreResponse->ResponseResultGet() != estiOK)
				{
					CstiCallHistoryItemSharedPtr spHistoryItem = pCoreResponse->contextItemGet<CstiCallHistoryItemSharedPtr> ();

					auto response = new CstiCallHistoryMgrResponse (eCallListType, 
																	spHistoryItem, 
																	pCoreResponse->RequestIDGet ());

					CallbackMessageSend (estiMSG_CALL_LIST_ITEM_EDIT_ERROR, (size_t)response);
				}

				bHandled = estiTRUE;
			}
			break;

		case CstiCoreResponse::eCALL_LIST_GET_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				m_bInitialized = true;

				DBG_MSG ("CoreResponse eCALL_LIST_GET_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsGet(pCoreResponse, eCallListType);

				bHandled = estiTRUE;
			}
			break;

		case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				DBG_MSG ("CoreResponse eCALL_LIST_ITEM_REMOVE_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				CoreListItemsRemove(pCoreResponse, eCallListType);

				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				DBG_MSG ("CoreResponse eCALL_LIST_COUNT_GET_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				//
				// Get the number of new items that have been added to the core services list
				//
				CstiCallList *pCstiCallList = pCoreResponse->CallListGet();
				unsigned int newCount = 0;

				if (pCstiCallList)
				{
					newCount = pCstiCallList->CountGet();
					if (newCount)
					{
						m_pCallLists[eCallListType]->NeedsNewItemsSet(true);
					}
					else
					{
						// Make sure these get cleared if there's nothing to do.
						m_pCallLists[eCallListType]->NeedsUpdateSet(false);
						m_pCallLists[eCallListType]->NeedsNewItemsSet(false);
					}
				}

				m_pCallLists[eCallListType]->NewItemCountSet(newCount);
				m_pCallLists[eCallListType]->UnviewedItemCountSet(m_pCallLists[eCallListType]->UnviewedItemCountGet() + newCount);
				CallbackMessageSend (estiMSG_MISSED_CALL_COUNT, (size_t)(m_pCallLists[eCallListType]->UnviewedItemCountGet()));

				ListUpdate(eCallListType);

				bHandled = estiTRUE;
			}
			break;
		case CstiCoreResponse::eCALL_LIST_SET_RESULT:
			eCallListType = RemoveRequestId (pCoreResponse->RequestIDGet ());
			if (CstiCallList::eTYPE_UNKNOWN != eCallListType)
			{
				DBG_MSG ("CoreResponse eCALL_LIST_COUNT_SET_RESULT (id %d) = %s", pCoreResponse->RequestIDGet (), pCoreResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

				// We should only be here if we were setting the list to an empty list,
				// which is our way of clearing it.

				m_pCallLists[eCallListType]->ListFree();
				RecentListRebuild();

				CallbackMessageSend(estiMSG_CALL_LIST_CHANGED, (size_t)eCallListType);
				startSaveDataTimer ();

				bHandled = estiTRUE;
			}
			break;

		case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
			if (pCoreResponse->ResponseResultGet () == estiOK)
			{
				AuthenticatedSet (estiFALSE);
			}
			break;

		case CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT:
			if (pCoreResponse->ResponseResultGet () == estiOK)
			{
				if (!m_bAuthenticated)
				{
					unsigned int value = pCoreResponse->ResponseValueGet ();

					//	0 Username and Password not valid
					//	1 User not logged in
					//	2 User logged in on current phone
					//	3 User logged in on another phone
					switch (value)
					{
						case 2:  // Already logged in on this phone.  Only need to get any missed calls
							m_pCallLists[CstiCallList::eMISSED]->NeedsNewItemsSet (true);
							break;

						default:
							m_pCallLists[CstiCallList::eANSWERED]->NeedsUpdateSet (true);
							m_pCallLists[CstiCallList::eDIALED]->NeedsUpdateSet (true);
							m_pCallLists[CstiCallList::eMISSED]->NeedsUpdateSet (true);
							break;
					}
				}
			}
			break;

		case CstiCoreResponse::eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT:
			// Update the recent list before we notify anyone that the answered list has changed.
			RecentListRebuild();
			CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)CstiCallList::eANSWERED);
			startSaveDataTimer ();
			break;

		case CstiCoreResponse::eVRS_AGENT_HISTORY_DIALED_ADD_RESULT:
			// Update the recent list before we notify anyone that the dialed list has changed.
			RecentListRebuild();
			CallbackMessageSend (estiMSG_CALL_LIST_CHANGED, (size_t)CstiCallList::eDIALED);
			startSaveDataTimer ();
			break;

		default:
			break;
	}

	if (bHandled)
	{
		pCoreResponse->destroy ();
	}

	return bHandled;
}


/*!
 * The call history manager shall retreive an updated list if an event indicates that the
 * list has changed in core.
 *
 * \param pStateNotifyResponse The state notify response to be handled.
 */
EstiBool CstiCallHistoryManager::StateNotifyEventHandle (
	CstiStateNotifyResponse *pStateNotifyResponse)
{
	EstiBool bHandled = estiFALSE;

	if (pStateNotifyResponse)
	{
		CstiStateNotifyResponse::EResponse eResponse = pStateNotifyResponse->ResponseGet ();
		auto  eCallListType = (CstiCallList::EType)pStateNotifyResponse->ResponseValueGet ();

		if (eResponse == CstiStateNotifyResponse::eCALL_LIST_CHANGED ||
			eResponse == CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD ||
			eResponse == CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE)
		{
			switch (eCallListType)
			{
				case CstiCallList::eDIALED:
				case CstiCallList::eANSWERED:
					DBG_MSG ("StateNotifyResponse eCALL_LIST_CHANGED");
					m_pCallLists[eCallListType]->NeedsUpdateSet(true);
					ListUpdate(eCallListType);
					bHandled = estiTRUE;
					break;

				case CstiCallList::eMISSED:
				{
					//
					// Request the number of new items from core services
					//
					DBG_MSG ("StateNotifyResponse eCALL_LIST_CHANGED");

					// If we're already retrieving the list, there's no need to do this again.
					if (!m_pCallLists[eCallListType]->NeedsUpdateGet() && !m_pCallLists[eCallListType]->NeedsNewItemsGet())
					{
						auto  pRequest = new CstiCoreRequest();
						pRequest->callListCountGet(eCallListType, false, m_pCallLists[eCallListType]->LastRetrievalTimeGet());
						CoreRequestSend(pRequest, nullptr, eCallListType);
					}

					bHandled = estiTRUE;
					break;
				}

				case CstiCallList::eTYPE_UNKNOWN:
				case CstiCallList::eCONTACT:
				case CstiCallList::eBLOCKED:
				case CstiCallList::eRECENT:
				case CstiCallList::eTYPE_LAST:

					break;
			}
		}
	}

	if (bHandled)
	{
		pStateNotifyResponse->destroy ();
	}

	return bHandled;
}


/*!
 * Utility function to send a callback message (if any callback is registered)
 *
 * \param[in] eMessage The message to send.
 * \param[in] Param The parameter to send (if required).
 */
void CstiCallHistoryManager::CallbackMessageSend (
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
* Adds an item to a call list.
*
* A core request shall be issued to add the item to core.  The request id for
* this request is returned to the caller.  Once the request has been completed the call list
* manager shall call the callback with a message and the request id indicating the request is
* complete.  The caller of this method may use this to monitor the completion of
* the asynchronous process.
*
* \param eListType The call list to add the item to
* \param spCallListItem The item being added
* \param bBlockedCallerID Caller ID should be blocked status.
* \param punRequestId A request id for this operation.
*/
stiHResult CstiCallHistoryManager::ItemAdd (
	CstiCallList::EType eCallListType,
	CstiCallHistoryItemSharedPtr spCallListItem,
	bool bBlockedCallerID,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pRequest = nullptr;
	CstiCallListItemSharedPtr coreItem;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && m_bEnabled, stiRESULT_ERROR);

	// Send the core request
	pRequest = new CstiCoreRequest ();

    if (m_pCoreService->APIMajorVersionGet () < 8)
    {
    	coreItem = spCallListItem->CoreItemCreate(false);
    }
    else
    {
    	coreItem = spCallListItem->CoreItemCreate(bBlockedCallerID);
    }

	pRequest->callListItemAdd (*coreItem, eCallListType);
	pRequest->contextItemSet (spCallListItem);

	if (bBlockedCallerID)
	{
		spCallListItem->DialStringSet("");

		// Set the blocked state on the call history object
		// Note: this can be removed once core stops sending us the number
		// back in the response to the add request.
		spCallListItem->BlockedCallerIDSet(estiTRUE);
	}

	hResult = CoreRequestSend (pRequest, punRequestId, eCallListType);
	stiTESTRESULT();

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
* Edits an item to in the call list.
*
* A core request shall be issued to edit the item in core.  The request id for
* this request is returned to the caller.  Once the request has been completed the call list
* manager shall call the callback with a message and the request id indicating the request is
* complete.  The caller of this method may use this to monitor the completion of
* the asynchronous process.
*
* \param eListType The call list to add the item to
* \param spCallListItem The item being added
* \param bBlockedCallerID Caller ID should be blocked status.
* \param punRequestId A request id for this operation.
*/
stiHResult CstiCallHistoryManager::ItemEdit (
	CstiCallList::EType eCallListType,
	CstiCallHistoryItemSharedPtr spCallListItem,
	bool bBlockedCallerID,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pRequest = nullptr;
	CstiCallListItemSharedPtr coreItem;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bAuthenticated && m_bEnabled, stiRESULT_ERROR);

	// Send the core request
	pRequest = new CstiCoreRequest ();

    if (m_pCoreService->APIMajorVersionGet () < 8)
    {
		coreItem = spCallListItem->CoreItemCreate(false);
    }
    else
    {
		coreItem = spCallListItem->CoreItemCreate(bBlockedCallerID);
    }

	pRequest->callListItemEdit (*coreItem, eCallListType);
	pRequest->contextItemSet (spCallListItem);

	if (bBlockedCallerID)
	{
		spCallListItem->DialStringSet("");

		// Set the blocked state on the call history object
		// Note: this can be removed once core stops sending us the number
		// back in the response to the add request.
		spCallListItem->BlockedCallerIDSet(estiTRUE);
	}

	hResult = CoreRequestSend (pRequest, punRequestId, eCallListType);
	stiTESTRESULT();

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
 * \param[in] eCallListType The Call List type this request is for.
 *
 * Returns:
 * stiRESULT_SUCCESS if in an initialized state and the Core Service logout message is
 * successfully sent, stiRESULT_ERROR otherwise.
*/
stiHResult CstiCallHistoryManager::CoreRequestSend (
	CstiCoreRequest *poCoreRequest,
	unsigned int *punRequestId,
	CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unInternalCoreRequestId = 0;

	stiTESTCOND (m_pCoreService, stiRESULT_ERROR);

	poCoreRequest->RemoveUponCommunicationError () = estiTRUE;
	hResult = m_pCoreService->RequestSend (
		poCoreRequest,
		&unInternalCoreRequestId);
	stiTESTRESULT ();

	m_PendingCookies[eCallListType].push_back (unInternalCoreRequestId);

	DBG_MSG ("Core request %d sent.", unInternalCoreRequestId);

	if (punRequestId)
	{
		*punRequestId = unInternalCoreRequestId;
	}

STI_BAIL:

	return hResult;
} // end CstiCallHistoryManager::CoreRequestSend


/*!
* Deletes an item from a call history list
*
* A core request shall be issued to delete the item from core.  The request id
* for this request is returned to the caller.  Once the request has been completed the
* call history manager shall call the callback with a message and the request id indicating
* the request is complete.  The caller of this method may use this to monitor the completion
* of the asynchronous process.
*
* \param[in] callListType The call list to add the item to
* \param[in] itemId Id of the item being added
* \param[out] requestId A request id for this operation.
*/
stiHResult CstiCallHistoryManager::ItemDelete (
	CstiCallList::EType callListType,
	const CstiItemId &itemId,
	unsigned int *requestId)
{
	auto hResult = stiRESULT_SUCCESS;
	auto coreRequest = new CstiCoreRequest ();
	CstiCallHistoryItemSharedPtr callItem;
	std::string stringItemID;

	Lock ();

	stiTESTCOND (m_bAuthenticated && m_bEnabled && itemId.IsValid(), stiRESULT_ERROR);

	stiTESTCOND (!m_pCoreService->isOfflineGet (), stiRESULT_OFFLINE_ACTION_NOT_ALLOWED);

	itemId.ItemIdGet(&stringItemID);

	callItem = m_pCallLists[callListType]->ItemFind (stringItemID.c_str ());
	stiTESTCOND (callItem, stiRESULT_ERROR);

#ifdef stiCALL_HISTORY_COMPRESSED
	if (CstiCallList::eRECENT == callListType && callItem->IsGroup ())
	{
		// If the item we're deleting is a group, we need to delete each entry in that group.
		for (unsigned int i = 0; i < callItem->GroupedItemsCountGet (); i++)
		{
			// Get the group item.
			auto subCallItem = callItem->GroupedItemGet (i);
			auto coreItem = subCallItem->CoreItemCreate (false);
			coreRequest->callListItemRemove (*coreItem, subCallItem->CallListTypeGet ());
		}
	}
	else
#endif
	{
		callListType = callItem->CallListTypeGet ();
		auto pCoreItem = callItem->CoreItemCreate (false);
		coreRequest->callListItemRemove (*pCoreItem, callListType);
	}
	coreRequest->contextItemSet (callItem);

	hResult = CoreRequestSend (coreRequest, requestId, callListType);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!
 * Finds a call list entry by item id.
 *
 * The manager will search its list for the id.
 *
 * \param[in] eCallListType The Call List to be searched
 * \param[in] pszItemId The item id to be returned.
 * \return the item found or NULL
 */
CstiCallHistoryItemSharedPtr CstiCallHistoryManager::ItemGetById (
	CstiCallList::EType eCallListType,
	const char *pszItemId) const
{
	CstiCallHistoryItemSharedPtr spCallListItem = nullptr;

	Lock ();

	// Find the item with the matching id.
	if (m_bEnabled)
	{
		spCallListItem = m_pCallLists[eCallListType]->ItemFind(pszItemId);
	}

	Unlock ();

	return spCallListItem;
}


/*!
* Finds a call list entry by index.
*
* This method is used to iterate through the list.  An object should lock the list before
* iterating through the list to protect the list from changing during the iteration process.
*
* \param[in] eCallListType The Call List to search
* \param[in] nIndex The index used to identify the entry
* \param[out] pCallListItem The found item
*/
stiHResult CstiCallHistoryManager::ItemGetByIndex (
	CstiCallList::EType eCallListType,
	int nIndex,
	CstiCallHistoryItemSharedPtr *pspCallListItem) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	// Can we even do this?
	stiTESTCOND (m_bEnabled, stiRESULT_ERROR);

	// Is the index within range?
	stiTESTCOND ((unsigned int)nIndex < m_pCallLists[eCallListType]->CountGet (), stiRESULT_ERROR);

	*pspCallListItem = m_pCallLists[eCallListType]->ItemGet (nIndex);
	stiTESTCOND (*pspCallListItem, stiRESULT_ERROR);

STI_BAIL:

	Unlock ();

	return hResult;
}

CstiCallHistoryItemSharedPtr CstiCallHistoryManager::itemGetByCallIndex (int callIndex, CstiCallList::EType callListType)
{
	auto listCount = ListCountGet (callListType);
	for (auto i = 0U; i < listCount; i++)
	{
		CstiCallHistoryItemSharedPtr callHistoryItem;
		ItemGetByIndex (callListType, i, &callHistoryItem);

		// found it
		if (callHistoryItem->callIndexGet () == callIndex)
		{
			return callHistoryItem;
		}
	}
	return nullptr;
}


/*!
* Sets the maximum number of entries allowed in the list.
*
* The call list manager is informed of the maxium length through this method. Setting a
* maximum length shorter than the current list length shall succeed.  Entries beyond the
* maximum shall be maintained.  New entries shall be disallowed.
*
* \param eCallListType The Call List to set
* \param nMaxLength The maximum number of entries allowed.
*/
stiHResult CstiCallHistoryManager::MaxCountSet (
	CstiCallList::EType eCallListType,
	int nMaxLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_pCallLists[eCallListType]->MaxCountSet(nMaxLength);

	// Update the size of the Recent list
	m_pCallLists[CstiCallList::eRECENT]->MaxCountSet(
		m_pCallLists[CstiCallList::eDIALED]->MaxCountGet() +
		m_pCallLists[CstiCallList::eANSWERED]->MaxCountGet() +
		m_pCallLists[CstiCallList::eMISSED]->MaxCountGet());

	Unlock ();

	return hResult;
}


stiHResult CstiCallHistoryManager::ListUpdate (CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int numItems = 0;

	if (m_pCallLists[eCallListType]->NeedsUpdateGet ())
	{
		numItems = m_pCallLists[eCallListType]->MaxCountGet ();
	}
	else if (m_pCallLists[eCallListType]->NeedsNewItemsGet ())
	{
		numItems = m_pCallLists[eCallListType]->NewItemCountGet ();
	}

	if (numItems != 0)
	{
		auto poCoreRequest = new CstiCoreRequest ();
		stiTESTCOND(poCoreRequest, stiRESULT_ERROR);

		unsigned int unCookie = 0;
		poCoreRequest->callListGet (
			eCallListType,
			nullptr,
			0,
			numItems,
			CstiCallList::eTIME,
			CstiList::SortDirection::DESCENDING,
			-1,
			false,
			true);

		CoreRequestSend (poCoreRequest, &unCookie, eCallListType);
		m_PendingCookies[eCallListType].push_back (unCookie);

		if (!m_pCallLists[eCallListType]->NeedsUpdateGet () && m_pCallLists[eCallListType]->NeedsNewItemsGet ())
		{
			m_unOnlyNewItemsCookie = unCookie;
		}
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiCallHistoryManager::ListUpdateAll ()
{
	// Mark all for a full update
	m_pCallLists[CstiCallList::eDIALED]->NeedsUpdateSet(true);
	m_pCallLists[CstiCallList::eANSWERED]->NeedsUpdateSet(true);
	m_pCallLists[CstiCallList::eMISSED]->NeedsUpdateSet(true);

	// Update the missed call count first
	auto  pRequest = new CstiCoreRequest();
	pRequest->callListCountGet(CstiCallList::eMISSED, false, m_pCallLists[CstiCallList::eMISSED]->LastRetrievalTimeGet());
	CoreRequestSend(pRequest, nullptr, CstiCallList::eMISSED);

	// Then get the rest of the lists
	ListUpdate(CstiCallList::eDIALED);
	ListUpdate(CstiCallList::eANSWERED);

	return stiRESULT_SUCCESS;
}

stiHResult CstiCallHistoryManager::ListClear (
	CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (!m_pCoreService->isOfflineGet (), stiRESULT_OFFLINE_ACTION_NOT_ALLOWED);

	if (CstiCallList::eRECENT == eCallListType)
	{
		ListClearAll();
	}
	else
	{
		auto  poCoreRequest = new CstiCoreRequest();
		stiTESTCOND (poCoreRequest, stiRESULT_ERROR);

		unsigned int unCookie = 0;
		auto  pNewCallList = new CstiCallList();
		pNewCallList->TypeSet(eCallListType);
		poCoreRequest->callListSet (pNewCallList);

		CoreRequestSend(poCoreRequest, &unCookie, eCallListType);
		m_PendingCookies[eCallListType].push_back (unCookie);
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiCallHistoryManager::ListClearAll()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = ListClear(CstiCallList::eDIALED);
	stiTESTRESULT ();

	hResult = ListClear(CstiCallList::eANSWERED);
	stiTESTRESULT ();

	hResult = ListClear(CstiCallList::eMISSED);
	stiTESTRESULT ();

	m_pCallLists[CstiCallList::eRECENT]->ListFree();

STI_BAIL:

	return hResult;
}


stiHResult CstiCallHistoryManager::MissedCallsClear ()
{
	// Set the most recent missed call time in the property table
	CstiCallHistoryListSharedPtr pList = m_pCallLists[CstiCallList::eMISSED];

	if (pList && pList->CountGet())
	{
		CstiCallHistoryItemSharedPtr spItem = pList->ItemGet(0);
		if (spItem)
		{
			PropertyManager::getInstance()->propertySet(
				LAST_MISSED_TIME, (int)spItem->CallTimeGet(),
				PropertyManager::Persistent);
			//
			// MultiRing: Send the LastMissedTime to core so it can propogate to other MR endpoints
			//
			PropertyManager::getInstance()->PropertySend (LAST_MISSED_TIME, estiPTypeUser);
		}
	}

	pList->UnviewedItemCountSet(0);

	return stiRESULT_SUCCESS;
}

unsigned int CstiCallHistoryManager::ListCountGet (CstiCallList::EType eType)
{
	unsigned int unCount = 0;

	if (eType < CstiCallList::eTYPE_LAST)
	{
		if (m_pCallLists[eType])
		{
			unCount = m_pCallLists[eType]->CountGet();
		}
	}

	return unCount;
}

unsigned int CstiCallHistoryManager::UnviewedItemCountGet(
	CstiCallList::EType eCallListType)
{
	return m_pCallLists[eCallListType]->UnviewedItemCountGet();
}

stiHResult CstiCallHistoryManager::RecentListCompress ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	CstiCallHistoryListSharedPtr pList = m_pCallLists[CstiCallList::eRECENT];
	unsigned int unCount = pList->CountGet();
	CstiCallHistoryItemSharedPtr spPrev = pList->ItemGet(0);

	for (unsigned int i = 1; i < unCount; i++)
	{
		CstiCallHistoryItemSharedPtr spCurr = pList->ItemGet(i);

		if (spCurr->DialStringGet() == spPrev->DialStringGet())
		{
			// They're the same, so group them.
			// (We assume they are already sorted by date, so spPrev should
			//  be the most recent.)
			spPrev->GroupedItemAdd(spCurr);

			pList->ItemRemove(i);
			i--;
			unCount--;
		}
		else
		{
			spPrev = spCurr;
		}
	}

	Unlock();
	return hResult;
}

stiHResult CstiCallHistoryManager::RecentListRebuild ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCallHistoryListSharedPtr pList;

	Lock();

	// Empty the list first
	m_pCallLists[CstiCallList::eRECENT]->ListFree();


	pList = m_pCallLists[CstiCallList::eDIALED];

	for (unsigned int i = 0; i < pList->CountGet(); i++)
	{
		CstiCallHistoryItemSharedPtr spItem = std::make_shared<CstiCallHistoryItem> ();
		*spItem = *pList->ItemGet(i);
		hResult = ItemInsert(CstiCallList::eRECENT, spItem);
		stiTESTRESULT();
	}

	pList = m_pCallLists[CstiCallList::eANSWERED];

	for (unsigned int i = 0; i < pList->CountGet(); i++)
	{
		CstiCallHistoryItemSharedPtr spItem = std::make_shared<CstiCallHistoryItem> ();
		*spItem = *pList->ItemGet(i);
		hResult = ItemInsert(CstiCallList::eRECENT, spItem);
		stiTESTRESULT();
	}

	pList = m_pCallLists[CstiCallList::eMISSED];

	for (unsigned int i = 0; i < pList->CountGet(); i++)
	{
		CstiCallHistoryItemSharedPtr spItem = std::make_shared<CstiCallHistoryItem> ();
		*spItem = *pList->ItemGet(i);
		hResult = ItemInsert(CstiCallList::eRECENT, spItem);
		stiTESTRESULT();
	}

#ifdef stiCALL_HISTORY_COMPRESSED
	RecentListCompress();
#endif

STI_BAIL:

	Unlock();

	return hResult;

}


stiHResult CstiCallHistoryManager::ItemInsert (
	CstiCallList::EType eCallListType,
	CstiCallHistoryItemSharedPtr spNewItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallHistoryListSharedPtr pList = m_pCallLists[eCallListType];

	unsigned int unCount = pList->CountGet();

	if (!unCount)
	{
		pList->ItemAdd(spNewItem);
	}
	else
	{
		unsigned int i = 0;

		for (; i < unCount; i++)
		{
			CstiCallHistoryItemSharedPtr spHistoryItem = pList->ItemGet(i);

			// Insert in order of the call time
			if (spHistoryItem->CallTimeGet() <= spNewItem->CallTimeGet())
			{
				pList->ItemInsert(spNewItem, i);
				break;
			}
		}

		if (i == unCount)
		{
			// Haven't found a spot, so it must be at the end of the list
			pList->ItemInsert(spNewItem, unCount);
		}
	}

	return hResult;
}


XMLListItemSharedPtr CstiCallHistoryManager::NodeToItemConvert (IXML_Node *pNode)
{
	Lock ();
	CstiCallHistoryListSharedPtr pNewItem = std::make_shared<CstiCallHistoryList> (pNode);

	m_pCallLists[pNewItem->TypeGet ()].reset ();
	m_pCallLists[pNewItem->TypeGet ()] = pNewItem;

	RecentListRebuild();

	Unlock ();

	return pNewItem;
}

void CstiCallHistoryManager::CachePurge ()
{
	Lock ();
	m_pCallLists[CstiCallList::eANSWERED]->ListFree ();
	m_pCallLists[CstiCallList::eDIALED]->ListFree ();
	m_pCallLists[CstiCallList::eMISSED]->ListFree ();
	m_pCallLists[CstiCallList::eRECENT]->ListFree ();
	m_pCallLists[CstiCallList::eANSWERED]->NeedsUpdateSet (true);
	m_pCallLists[CstiCallList::eDIALED]->NeedsUpdateSet (true);
	m_pCallLists[CstiCallList::eMISSED]->NeedsUpdateSet (true);
	m_pCallLists[CstiCallList::eRECENT]->NeedsUpdateSet (true);
	startSaveDataTimer ();
	Unlock ();
}

// end CstiCallHistoryManager.cpp
