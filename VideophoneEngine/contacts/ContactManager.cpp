// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved


#include "ContactManager.h"
#include "XMLList.h"
#include "ContactListItem.h"
#include "CstiCoreService.h"
#include "IstiImageManager.h"
#include "FavoriteList.h"
#include "SstiFavoriteList.h"
#include "CstiEventQueue.h"
#include <sstream>

using namespace WillowPM;

#define ABSOLUTE_MAX_CONTACT_LIST_ITEMS 500


CstiContactManager::CstiContactManager(
	CstiCoreService *pCoreService,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam,
	IstiImageManager* pImageManager
)
	: 
	m_pCoreService (pCoreService),
	m_pImageManager (pImageManager),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet()),
	m_bEnabled(true),
	m_bAuthenticated (estiFALSE),
	m_unLastCookie(0),
	m_bContactPhotosEnabled(true)
	
{
	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream ContactListFile;

	ContactListFile << DynamicDataFolder << "ContactList.xml";

	SetFilename(ContactListFile.str ());

	if (m_pImageManager)
	{
		m_pImageManager->ContactManagerSet(this);
	}

	m_signalConnections.push_back (m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		m_eventQueue.PostEvent(std::bind(&CstiContactManager::clientAuthenticateHandle, this, result));
	}));
}

stiHResult CstiContactManager::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	Lock ();
	hResult = XMLManager::init();
	Unlock();
	return hResult;
}

IstiContactSharedPtr CstiContactManager::ContactCreate()
{
	return std::make_shared<ContactListItem> ();
}




IstiContactSharedPtr CstiContactManager::getContact(int nIndex)
{
	Lock();
	auto contact = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(nIndex));
	Unlock();
	return contact;
}


int CstiContactManager::getNumContacts()
{
	Lock();
	int nCount = m_pdoc->CountGet();
	Unlock();

	return nCount;
}


#if 0
EstiBool CstiContactManager::getContactByName(const char *pName, IstiContact **pContact)
{
	Lock();

	EstiBool bFound = estiFALSE;
	int nResult;

	ContactListItem *pItem = NULL;
	nResult = itemGet(pName, (XMLListItem **)&pItem);
	*pContact = pItem;

	if (nResult == XM_RESULT_SUCCESS)
	{
		bFound = estiTRUE;
	}

	Unlock();

	return bFound;
}
#endif


#if 0
EstiBool CstiContactManager::getContactByID(const char *pID, IstiContact **pContact)
{
	Lock();

	EstiBool bFound = estiFALSE;

	if (pID)
	{
		int Count = m_pdoc->CountGet();
		ContactListItem *pItem = NULL;

		for (int i = 0; i < Count; i++)
		{
			pItem = (ContactListItem *)m_pdoc->ItemGet(i);

			if (pItem->IDGet())
			{
				if (strcmp(pItem->IDGet(), pID) == 0)
				{
					*pContact = pItem;
					bFound = estiTRUE;
					break;
				}
			}
		}
	}

	Unlock();

	return bFound;
}
#endif


EstiBool CstiContactManager::ContactByIDGet(const CstiItemId &Id, IstiContactSharedPtr *contact)
{
	Lock();

	EstiBool bFound = estiFALSE;

	int Count = m_pdoc->CountGet();
	ContactListItemSharedPtr pItem;

	for (int i = 0; i < Count; i++)
	{
		pItem = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(i));
		if (Id == pItem->ItemIdGet())
		{
			*contact = pItem;
			bFound = estiTRUE;
			break;
		}
	}

	Unlock();

	return bFound;
}

IstiContactSharedPtr CstiContactManager::contactByIDGet(
	const CstiItemId &id)
{
	IstiContactSharedPtr contact;
	IstiContactSharedPtr contactCopy;

	Lock();

	if (estiTRUE == ContactByIDGet(id, &contact))
	{
		contactCopy = std::make_shared<ContactListItem>(contact);
	}

	Unlock();

	return contactCopy;
}

EstiBool CstiContactManager::getContactByPhoneNumber(
	const std::string &phoneNumber,
	IstiContactSharedPtr *contact)
{
	Lock();

	EstiBool bFound = estiFALSE;

	if (!phoneNumber.empty ())
	{
		bool bMatch = false;
		int nCount = m_pdoc->CountGet();
		ContactListItemSharedPtr pItem;
		IstiContactSharedPtr pCandidate;
		std::string candidateName;

		for (int i = 0; i < nCount; i++)
		{
			pItem = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(i));

			auto itemName = pItem->NameGet ();
			
			//
			// If NameGet returns an empty string then this is an issue.
			// However, we will just continue with the next item.
			//
			if (itemName.empty ())
			{
				// Contacts are allowed to have an empty name if companyName is populated
				itemName = pItem->CompanyNameGet ();

				if (itemName.empty ())
				{
					continue;
				}
			}
			
			if (pCandidate)
			{
				//
				// Only consider the next item if the item is
				// alphanumerically earlier in the list
				//
				if (itemName > candidateName)
				{
					continue;
				}
			}
			
#ifdef stiCONTACT_LIST_MULTI_PHONE
			bMatch = pItem->DialStringMatch(phoneNumber);
#else
			pItem->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_HOME, &bMatch);
			
			if (!bMatch)
			{
				pItem->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_WORK, &bMatch);
				
				if (!bMatch)
				{
					pItem->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_CELL, &bMatch);
				}
			}
#endif

			if (bMatch)
			{
				pCandidate = pItem;
				
				candidateName = itemName;
			}
		}
		
		if (pCandidate)
		{
			*contact = pCandidate;
			bFound = estiTRUE;
		}
	}

	Unlock();

	return bFound;
}

IstiContactSharedPtr CstiContactManager::contactByPhoneNumberGet(
	const std::string &phoneNumber)
{
	IstiContactSharedPtr contact;
	IstiContactSharedPtr contactCopy;

	Lock();

	if (estiTRUE == getContactByPhoneNumber(phoneNumber, &contact))
	{
		contactCopy = std::make_shared<ContactListItem>(contact);
	}

	Unlock();

	return contactCopy;
}

int CstiContactManager::getContactIndex(const std::string &id)
{
	Lock();

	int Index = -1;

	if (!id.empty ())
	{
		int Count = m_pdoc->CountGet();
		ContactListItemSharedPtr pItem;

		for (int i = 0; i < Count; i++)
		{
			pItem = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(i));

			if (pItem->IDGet() == id)
			{
				Index = i;
				break;
			}
		}
	}

	Unlock();

	return Index;
}


int CstiContactManager::ContactIndexGet(const CstiItemId &Id)
{
	Lock();

	int Index = -1;

	int Count = m_pdoc->CountGet();
	ContactListItemSharedPtr pItem;

	for (int i = 0; i < Count; i++)
	{
		pItem = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(i));
		if (Id == pItem->ItemIdGet())
		{
			Index = i;
			break;
		}
	}

	Unlock();

	return Index;
}


void CstiContactManager::removeContact(const IstiContactSharedPtr &item)
{
	Lock();

	//const CstiContactListItemSharedPtr cstiItem;
	CstiCoreRequest *pRequest = nullptr;

	// Create a CstiContactListItem.
	const CstiContactListItemSharedPtr &cstiItem = std::static_pointer_cast<ContactListItem> (item)->GetCstiContactListItem();

	// Send the core request
	pRequest = new CstiCoreRequest();
	pRequest->contactListItemRemove(*cstiItem);
	pRequest->contextItemSet(cstiItem);
	pRequest->retryOfflineSet (true);
    CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT);

	Unlock();
}


void CstiContactManager::clearContacts()
{
	Lock();

	CstiList *pList = nullptr;
	CstiCoreRequest *pRequest = nullptr;

	// Create an empty list
	pList = new CstiContactList;

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->contactListSet((CstiContactList *)pList);
    CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_SET_RESULT);
	
	delete pList;
	pList = nullptr;

	Unlock();
}

void CstiContactManager::purgeContacts()
{
	Lock();

	m_pdoc->ListFree();
	m_Favorites.purge();

	startSaveDataTimer();

	CallbackMessageSend(estiMSG_CONTACT_LIST_CHANGED, (size_t)0);

	Unlock();
}


void CstiContactManager::addContact(const IstiContactSharedPtr &pItem)
{
	Lock();

	CstiContactListItemSharedPtr pCstiItem;
	CstiCoreRequest *pRequest = nullptr;

	// Create a CstiContactListItem.
	const ContactListItemSharedPtr &contactListItem = std::static_pointer_cast<ContactListItem> (pItem);
	pCstiItem = contactListItem->GetCstiContactListItem();

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->contactListItemAdd (*pCstiItem);
	pRequest->contextItemSet (contactListItem);
	pRequest->retryOfflineSet (true);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT);

	Unlock();
}


void CstiContactManager::updateContact(const IstiContactSharedPtr &pItem)
{
	Lock();

	CstiContactListItemSharedPtr pCstiItem;
	CstiCoreRequest *pRequest = nullptr;

	// Create a CstiContactListItem.
	const ContactListItemSharedPtr &contactListItem = std::static_pointer_cast<ContactListItem> (pItem);
	pCstiItem = contactListItem->GetCstiContactListItem();

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->contactListItemEdit(*pCstiItem);
	pRequest->contextItemSet (contactListItem);
	pRequest->retryOfflineSet (true);
    CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT);

	Unlock();
}


bool CstiContactManager::isFull()
{
	Lock();
	bool bFull = m_pdoc->CountGet() >= m_pdoc->MaxCountGet();
	Unlock();

	return bFull;
}


void CstiContactManager::setMaxCount(size_t Count)
{
	Lock();
	m_pdoc->MaxCountSet(Count);
	Unlock();
}


size_t CstiContactManager::getMaxCount()
{
	Lock();
	size_t unCount = m_pdoc->MaxCountGet();
	Unlock();

	return unCount;
}


EstiBool CstiContactManager::CoreResponseHandle(CstiCoreResponse *poResponse)
{
	Lock();

	EstiBool bHandled = estiFALSE;

	switch (poResponse->ResponseGet())
	{
	case CstiCoreResponse::eCALL_LIST_GET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			if (poResponse->ResponseResultGet() == estiOK)
			{
				CstiContactList *pList = poResponse->ContactListGet();

				if (pList)
				{
					m_pdoc->ListFree();
					for (unsigned int i = 0; i < pList->CountGet(); i++)
					{
						CstiContactListItemSharedPtr pItem = pList->ItemGet(i);
						if (pItem)
						{
							addCstiContact(pItem);
						}
					}
					CallbackMessageSend(estiMSG_CONTACT_LIST_CHANGED, (size_t)0);
				}
			}
			else
			{
				// An error occurred.
				// Don't do anything
			}

			bHandled = estiTRUE;
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			auto contactListItem = poResponse->contextItemGet<ContactListItemSharedPtr> ();

			CstiContactListItemSharedPtr pItem = contactListItem->GetCstiContactListItem();

			if (poResponse->ResponseResultGet() == estiOK)
			{
				CstiContactList *pList = poResponse->ContactListGet();

				if (pList)
				{
					for (unsigned int i = 0; i < pList->CountGet(); i++) // may have more than one contact if importing
					{
						CstiContactListItemSharedPtr pContact = pList->ItemGet(i);
						if (pContact)
						{
							addCstiContact(pContact);
							auto pID = new CstiItemId;
							pContact->ItemIdGet(pID);
							if (i == 0)
							{
								CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_ADDED, (size_t)pID);
							}
						}
					}
					if (pList->CountGet()>1)
					{
						CallbackMessageSend(estiMSG_CONTACT_LIST_CHANGED, (size_t)0);
					}
#ifdef stiFAVORITES
					//We need to see if the item we just added requires us to update favorites.
					CreateFavoriteFromNewNumbers(contactListItem);
#endif
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, false);
			}

			bHandled = estiTRUE;

			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			if (poResponse->ResponseResultGet() == estiOK)
			{
				auto contactListItem = poResponse->contextItemGet<ContactListItemSharedPtr> ();
				stiASSERT (contactListItem);

				if (contactListItem)
				{
					CstiContactListItemSharedPtr pItem = contactListItem->GetCstiContactListItem();
					stiASSERT (pItem);

					if (pItem)
					{
						// Ick...we need to now request the item we just edited
						CstiCoreRequest *pRequest = nullptr;
						pRequest = new CstiCoreRequest ();
						pRequest->contactListItemGet(*pItem);
						pRequest->contextItemSet(contactListItem);
						CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT);
					}
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, false);
			}

			bHandled = estiTRUE;

			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			// Remove the PendingContactListItemChangedRequest from the m_PendingContactListItemChangedRequests list.  There was no delete event after the add/edit event.
			for (auto it = m_PendingContactListItemChangedRequests.begin(); it != m_PendingContactListItemChangedRequests.end(); ++it)
			{
				if ((*it).m_unCoreRequestId == poResponse->RequestIDGet())
				{
					m_PendingContactListItemChangedRequests.erase(it);
					break;
				}
			}

			if (poResponse->ResponseResultGet() == estiOK)
			{
				// We should only be requesting an individual item
				// if we were editing an existing contact.  (Core
				// unfortunately does not give us the edited item in
				// its response, so we need to go and request it.)
				CstiContactList *pList = poResponse->ContactListGet();

				if (pList)
				{
					CstiContactListItemSharedPtr pContact = pList->ItemGet(0);
					if (pContact)
					{
						auto contactListItem = poResponse->contextItemGet<ContactListItemSharedPtr> ();
#ifdef stiFAVORITES
						//We need to see if the item we just added requires us to update favorites.
						if (contactListItem)
						{
							CreateFavoriteFromNewNumbers(contactListItem);
						}
#endif
						updateCstiContact(pContact);
						auto pID = new CstiItemId;
						pContact->ItemIdGet(pID);
						CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_EDITED, (size_t)pID);

#ifdef stiIMAGE_MANAGER
						// update the cached photo information
						auto item =
							m_pendingPhotoCheck.find(pContact->ItemIdGet());

						if (item != m_pendingPhotoCheck.end())
						{
							if (CoreHasImage(pContact->ImageIdGet())
								|| !strcmp(pContact->ImageIdGet(), SilhouetteAvatarGUID) )
							{
								// Only update if core has the image.
								UpdatePhoto(*item);
							}
							m_pendingPhotoCheck.erase(item);
						}
#endif
					}
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, false);
			}

			bHandled = estiTRUE;
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			auto pItem = poResponse->contextItemGet<CstiContactListItemSharedPtr> ();

			if (poResponse->ResponseResultGet() == estiOK)
			{
#ifdef stiIMAGE_MANAGER
				const char *pszImageId = pItem->ImageIdGet();
				if (m_pImageManager && CoreHasImage(pszImageId))
				{
					m_pImageManager->PurgeImageFromCache(pszImageId);
				}
#endif

				removeCstiContact(pItem);
				auto pID = new CstiItemId;
				pItem->ItemIdGet(pID);
				CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_DELETED, (size_t)pID);
#ifdef stiFAVORITES
				// Core deletes the contact, we need to go update our favorites list.
				auto pRequest = new CstiCoreRequest ();
				pRequest->favoriteListGet();
				CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_LIST_GET_RESULT);
#endif
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, false);
			}

			bHandled = estiTRUE;

			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;
		
	case CstiCoreResponse::eCALL_LIST_SET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			if (poResponse->ResponseResultGet() == estiOK)
			{
				CstiContactList *pList = poResponse->ContactListGet();
				
				if (pList->CountGet() > 0)
				{
					m_pdoc->ListFree();
					for (unsigned int i = 0; i < pList->CountGet(); i++)
					{
						CstiContactListItemSharedPtr pItem = pList->ItemGet(i);
						if (pItem)
						{
							addCstiContact(pItem);
						}
					}
					CallbackMessageSend(estiMSG_CONTACT_LIST_CHANGED, (size_t)0);
				}
				else
				{
					m_pdoc->ListFree();
					CallbackMessageSend(estiMSG_CONTACT_LIST_DELETED, (size_t)0);
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, false);
			}

			bHandled = estiTRUE;
			
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;
#ifdef stiIMAGE_MANAGER
	case CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			DownloadURLGetResultHandle(poResponse);
		}
		break;
	#endif
	case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			if (poResponse->ResponseResultGet() == estiOK)
			{
				int nImportCount = atoi(poResponse->ResponseStringGet(0).c_str ());
				CallbackMessageSend(estiMSG_CONTACT_LIST_IMPORT_COMPLETE, nImportCount);
			}
			else
			{
				errorSend(poResponse, false);
			}
		}
		break;

#ifdef stiFAVORITES
	case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			auto pItem = poResponse->contextItemGet<CstiFavoriteSharedPtr> ();

			if (poResponse->ResponseResultGet() == estiOK)
			{
				if (pItem)
				{
					m_Favorites.ItemAdd(pItem);
					auto pID = new CstiItemId;
					*pID = pItem->PhoneNumberIdGet();
					CallbackMessageSend(estiMSG_FAVORITE_ITEM_ADDED, (size_t)pID);
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, true);
			}

			bHandled = estiTRUE;
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			auto change = poResponse->contextItemGet<FavoritePosChange> ();

			if (poResponse->ResponseResultGet() == estiOK)
			{
				m_Favorites.ItemMove(change.nOldIndex, change.nNewIndex);
				CallbackMessageSend(estiMSG_FAVORITE_ITEM_EDITED, (size_t)0);
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, true);
			}

			bHandled = estiTRUE;

			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			auto item = poResponse->contextItemGet<CstiFavoriteSharedPtr> ();

			if (poResponse->ResponseResultGet() == estiOK)
			{
				auto pID = new CstiItemId;
				*pID = item->PhoneNumberIdGet();
				m_Favorites.ItemRemove(m_Favorites.ItemIndexGet(item));
				CallbackMessageSend(estiMSG_FAVORITE_ITEM_REMOVED, (size_t)pID);
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, true);
			}

			bHandled = estiTRUE;

			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			if (poResponse->ResponseResultGet() == estiOK)
			{
				auto pList = poResponse->FavoriteListGet();

				if (pList)
				{
					m_Favorites.purge ();

					for (auto &item : pList->favorites)
					{
						m_Favorites.ItemAdd (std::make_shared<CstiFavorite> (item));
					}

					CallbackMessageSend(estiMSG_FAVORITE_LIST_CHANGED, (size_t)0);
				}
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, true);
			}

			bHandled = estiTRUE;
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;

	case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
		if (RemoveRequestId(poResponse->RequestIDGet()))
		{
			// TODO: should this be a SharedPtr instead?  Shouldn't need to be...
			auto list = poResponse->contextItemGet<SstiFavoriteList> ();

			if (poResponse->ResponseResultGet() == estiOK)
			{
				m_Favorites.purge ();

				for (auto item : list.favorites)
					{
					m_Favorites.ItemAdd (std::make_shared<CstiFavorite> (item));
				}

				CallbackMessageSend(estiMSG_FAVORITE_LIST_SET, (size_t)0);
			}
			else
			{
				// An error occurred.
				errorSend(poResponse, true);
			}

			bHandled = estiTRUE;
			poResponse->destroy ();
			poResponse = nullptr;
		}
		break;
#endif

	default:
		break;
	}

	Unlock();

	return bHandled;
}


EstiBool CstiContactManager::StateNotifyEventHandle(CstiStateNotifyResponse *pStateNotifyResponse)
{
	Lock();

	EstiBool bHandled = estiFALSE;

	if (pStateNotifyResponse
	 && pStateNotifyResponse->ResponseValueGet () == CstiCallList::eCONTACT)
	{
		switch (pStateNotifyResponse->ResponseGet ())
		{
			case CstiStateNotifyResponse::eCALL_LIST_CHANGED:
			{
				// Send the core request to get the new list
				ReloadContactsFromCore();
				
#ifdef stiFAVORITES
				ReloadFavoritesFromCore();
#endif
				
				pStateNotifyResponse->destroy ();
				pStateNotifyResponse = nullptr;
				bHandled = estiTRUE;
				break;
			}
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD:
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_EDIT:
			{
				ContactListItemChangedHandle(pStateNotifyResponse);

				pStateNotifyResponse->destroy ();
				pStateNotifyResponse = nullptr;
				bHandled = estiTRUE;
				break;
			}
			case CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE:
			{
				ContactListItemRemovedHandle(pStateNotifyResponse);

				pStateNotifyResponse->destroy ();
				pStateNotifyResponse = nullptr;
				bHandled = estiTRUE;
				break;
			}
			default:
				break;
		}
	}
#ifdef stiFAVORITES
	else if (pStateNotifyResponse)
	{
		//For right now the only other messages falling through will be favorites, however we should
		//still check to make sure they are the messages we want.
		if (pStateNotifyResponse->ResponseGet () == CstiStateNotifyResponse::eFAVORITE_ADDED
			|| pStateNotifyResponse->ResponseGet () == CstiStateNotifyResponse::eFAVORITE_LIST_CHANGED
			|| pStateNotifyResponse->ResponseGet () == CstiStateNotifyResponse::eFAVORITE_REMOVED)
		{
			ReloadFavoritesFromCore();
		}
	}
#endif

	Unlock();

	return bHandled;
}


EstiBool CstiContactManager::RemovedUponCommunicationErrorHandle(
	CstiVPServiceResponse *response)
{
	Lock();

	EstiBool bHandled = estiFALSE;
	CstiCoreResponse::EResponse unExpectedResult = CstiCoreResponse::eRESPONSE_UNKNOWN;
	bool favoriteResponse = false;

	if (RemoveRequestId (response->RequestIDGet(), (unsigned int *)&unExpectedResult))
	{
		switch (unExpectedResult)
		{
			case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
			case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
			case CstiCoreResponse::eCALL_LIST_SET_RESULT:
			case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
			case CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT:
			case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
			{
				break;
			}

			case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
			case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
			case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
			case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
			case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
			{
				favoriteResponse = true;
				break;
			}

			default:
			{
				stiASSERTMSG(false, "Unexpected Core response: %u", unExpectedResult);
				break;
			}
		}

		Response message = coreResponseConvert(unExpectedResult);

		if (favoriteResponse)
		{
			favoriteErrorSignal.Emit(message, stiRESULT_OFFLINE_ACTION_NOT_ALLOWED);
		}
		else
		{
			contactErrorSignal.Emit(message, stiRESULT_OFFLINE_ACTION_NOT_ALLOWED);
		}

		response->destroy ();
		response = nullptr;

		bHandled = estiTRUE;
	}

	Unlock();

	return bHandled;
}

/*!
 * Since the contact list can only be retreived when the user has been authenticated by core,
 * this method informs the contact list manager when that state has been reached.
 *
 * \param[in] bAuthenticated Whether or not the user is authenticated.
 */
stiHResult CstiContactManager::AuthenticatedSet (EstiBool bAuthenticated)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	m_bAuthenticated = bAuthenticated;
	
	// Notify the UI that we have loaded the contact lists.
	CallbackMessageSend (estiMSG_CONTACT_LIST_CHANGED, (size_t)0);
	CallbackMessageSend (estiMSG_FAVORITE_LIST_CHANGED, (size_t)0);

	if (m_bAuthenticated)
	{
		Refresh();
	}

	Unlock ();
	
	return hResult;
}


void CstiContactManager::Refresh()
{
	Lock();
	
	if (m_bEnabled && m_bAuthenticated)
	{
		ReloadContactsFromCore();
	}
	
#ifdef stiFAVORITES
	if (m_bEnabled && m_bAuthenticated)
	{
		ReloadFavoritesFromCore();
	}
#endif
	
	Unlock();
}


void CstiContactManager::ReloadContactsFromCore()
{
	Lock();
	
	auto pRequest = new CstiCoreRequest ();
	pRequest->contactListGet(nullptr, 0, ABSOLUTE_MAX_CONTACT_LIST_ITEMS, -1);
	CoreRequestSend(pRequest, estiFALSE, CstiCoreResponse::eCALL_LIST_GET_RESULT);

	Unlock();
}


#ifdef stiFAVORITES
void CstiContactManager::ReloadFavoritesFromCore()
{
	auto pRequest = new CstiCoreRequest ();
	pRequest->favoriteListGet ();
	CoreRequestSend (pRequest, estiFALSE, CstiCoreResponse::eFAVORITE_LIST_GET_RESULT);
}
#endif
#ifdef stiIMAGE_MANAGER

void CstiContactManager::UpdateAllPhotos()
{
	if (m_bContactPhotosEnabled)
	{
		m_photoUpdateState.clear();

		// purge all photos from the image cache so that they all get reloaded fresh
		if (m_pImageManager)
		{
			m_pImageManager->PurgeAllContactPhotos();
		}

		m_expectedPhotoDownloadURLs.clear();

		// request the image download URL for every phone number for every contact
		Lock();

		auto pRequest = new CstiCoreRequest();

		int Count = m_pdoc->CountGet();

		for (int i = 0; i < Count; i++)
		{
			auto item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));

			std::string number;

			if (   (   item->DialStringGet(CstiContactListItem::ePHONE_HOME, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				m_expectedPhotoDownloadURLs.insert(number);
			}

			if (   (   item->DialStringGet(CstiContactListItem::ePHONE_WORK, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				m_expectedPhotoDownloadURLs.insert(number);
			}

			if (   (   item->DialStringGet(CstiContactListItem::ePHONE_CELL, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				m_expectedPhotoDownloadURLs.insert(number);
			}
		}

		Unlock();

		if (!m_expectedPhotoDownloadURLs.empty())
		{
			CoreRequestSend(pRequest, estiTRUE, CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT);
		}
		else
		{
			delete pRequest;
			pRequest = nullptr;
		}
	}
}


void CstiContactManager::PurgeAllContactPhotos()
{
	Lock();

	int Count = m_pdoc->CountGet();

	for (int i = 0; i < Count; i++)
	{
		auto pItem = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));

		std::string photoId;
		pItem->PhotoGet(&photoId);

		if (!photoId.empty () && CoreHasImage(photoId))
		{
			pItem->PhotoSet(NoGUID);
			favoritesPreserve(pItem);	// Don't lose favorites info

			updateContact(pItem);
		}
	}

	Unlock();
}

void CstiContactManager::FixContactsForPhotoNotFound(const std::string& guid)
{
	if (m_bContactPhotosEnabled)
	{
		if (!CoreHasImage(guid))
		{
			return;
		}

		ExpectedPhotoDownloadURLs expectedDownloadURLs;
		auto pRequest = new CstiCoreRequest();

		Lock();

		int Count = m_pdoc->CountGet();

		for (int i = 0; i < Count; i++)
		{
			auto item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));

			std::string photoId;
			item->PhotoGet(&photoId);

			if (!photoId.empty ()
				&& (guid == photoId))
			{
				std::string number;

				if ((item->DialStringGet(CstiContactListItem::ePHONE_HOME, &number)
						== XM_RESULT_SUCCESS)
					&& !number.empty())
				{
					pRequest->imageDownloadURLGet(
						number.c_str(),
						CstiCoreRequest::ePHONE_NUMBER);

					expectedDownloadURLs.insert(number);
				}

				if ((item->DialStringGet(CstiContactListItem::ePHONE_WORK, &number)
						== XM_RESULT_SUCCESS)
					&& !number.empty())
				{
					pRequest->imageDownloadURLGet(
						number.c_str(),
						CstiCoreRequest::ePHONE_NUMBER);

					expectedDownloadURLs.insert(number);
				}

				if ((item->DialStringGet(CstiContactListItem::ePHONE_CELL, &number)
						== XM_RESULT_SUCCESS)
					&& !number.empty())
				{
					pRequest->imageDownloadURLGet(
						number.c_str(),
						CstiCoreRequest::ePHONE_NUMBER);

					expectedDownloadURLs.insert(number);
				}
			}
		}

		Unlock();

		if (!expectedDownloadURLs.empty())
		{
			m_expectedPhotoDownloadURLs.insert(
				expectedDownloadURLs.begin(),
				expectedDownloadURLs.end());

			CoreRequestSend(pRequest, estiTRUE, CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT);
		}
		else
		{
			delete pRequest;
			pRequest = nullptr;
		}
	}
}

void CstiContactManager::UpdatePhoto(const std::string& itemId)
{
	if (m_bContactPhotosEnabled)
	{
		Lock();
		ExpectedPhotoDownloadURLs expectedDownloadURLs;
		auto pRequest = new CstiCoreRequest();
		IstiContactSharedPtr contact;

		if (   (ContactByIDGet(CstiItemId(itemId.c_str()), &contact) == estiTRUE)
			&& contact)
		{
			std::string number;

			if (   (   contact->DialStringGet(CstiContactListItem::ePHONE_HOME, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				expectedDownloadURLs.insert(number);
			}

			if (   (   contact->DialStringGet(CstiContactListItem::ePHONE_WORK, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				expectedDownloadURLs.insert(number);
			}

			if (   (   contact->DialStringGet(CstiContactListItem::ePHONE_CELL, &number)
					== XM_RESULT_SUCCESS)
				&& !number.empty())
			{
				pRequest->imageDownloadURLGet(
					number.c_str(),
					CstiCoreRequest::ePHONE_NUMBER);

				expectedDownloadURLs.insert(number);
			}
		}

		Unlock();

		if (!expectedDownloadURLs.empty())
		{
			m_expectedPhotoDownloadURLs.insert(
				expectedDownloadURLs.begin(),
				expectedDownloadURLs.end());

			CoreRequestSend(pRequest, estiTRUE, CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT);
		}
		else
		{
			delete pRequest;
			pRequest = nullptr;
		}
	}
}
#endif //End stiIMAGE_MANAGER

void CstiContactManager::addCstiContact(const CstiContactListItemSharedPtr &cstiItem)
{
	ContactListItemSharedPtr contact = std::make_shared<ContactListItem> (cstiItem);
	m_pdoc->ItemAdd(contact);

	startSaveDataTimer();
}


void CstiContactManager::updateCstiContact(const CstiContactListItemSharedPtr &cstiItem)
{
	CstiItemId Id;
	cstiItem->ItemIdGet(&Id);

	if (Id.IsValid())
	{
		IstiContactSharedPtr contact;
		ContactByIDGet(Id, &contact);

		if (contact)
		{
#ifdef stiIMAGE_MANAGER
			// see if the photo changed
			std::string currentPhotoId;
			std::string currentPhotoTimestamp;
			const char *new_photo_id = nullptr;
			const char *new_photo_timestamp = nullptr;

			contact->PhotoGet(&currentPhotoId);
			contact->PhotoTimestampGet(&currentPhotoTimestamp);

			new_photo_id = cstiItem->ImageIdGet();
			new_photo_timestamp = cstiItem->ImageTimestampGet();

			bool photo_changed = true;

			if ((currentPhotoId.empty () && !new_photo_id)
				|| (!currentPhotoId.empty ()
					&& new_photo_id
					&& !currentPhotoTimestamp.empty ()
					&& new_photo_timestamp
					&& currentPhotoId != new_photo_id
					&& currentPhotoTimestamp != new_photo_timestamp))
			{
				photo_changed = false;
			}

			if (photo_changed)
			{
				// tell the image manager to purge cached versions of
				// the old photo so that it will fetch the updated
				// version the next time the image is requested
				if (!currentPhotoId.empty ()
					&& m_pImageManager
					&& CoreHasImage(currentPhotoId))
				{
					m_pImageManager->PurgeImageFromCache(currentPhotoId.c_str ());
				}
			}
#endif //End stiIMAGE_MANAGER
			// update the contact
			std::static_pointer_cast<ContactListItem> (contact)->CopyCstiContactListItem(cstiItem.get());
			startSaveDataTimer();
		}
		else
		{
			// Couldn't find the ID...just add a new item
			addCstiContact(cstiItem);

			// Send event to update the UI
			auto pID = new CstiItemId;
			cstiItem->ItemIdGet(pID);
			CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_ADDED, (size_t)pID);
		}
	}
	else
	{
		// No ID...just add a new item
		addCstiContact(cstiItem);
	}
}


void CstiContactManager::removeCstiContact(const CstiContactListItemSharedPtr &cstiItem)
{
	CstiItemId Id;
	cstiItem->ItemIdGet(&Id);
	int index = ContactIndexGet(Id);

	if (index != -1)
	{
		m_pdoc->ItemDestroy(index);
		startSaveDataTimer();
	}
}


XMLListItemSharedPtr CstiContactManager::NodeToItemConvert(IXML_Node *pNode)
{
	Lock();
	XMLListItemSharedPtr item = std::make_shared<ContactListItem> (pNode);
	Unlock();

	return item;
}

/*!
 * Utility function to determine if the request id of the given core
 * response was one that we were waiting for.  If so, the request
 * id will be removed from the list.
 */
EstiBool CstiContactManager::RemoveRequestId(unsigned int unRequestId, unsigned int *punExpectedResult)
{
	EstiBool bHasBlockListCookie = estiFALSE;

	for (auto it = m_PendingCookies.begin(); it != m_PendingCookies.end(); ++it)
	{
		if ((*it).m_unCookie == unRequestId)
		{
			bHasBlockListCookie = estiTRUE;
			if (punExpectedResult)
			{
				*punExpectedResult = (*it).m_unExpectedResult;
			}
			m_PendingCookies.erase(it);
			m_unLastCookie = unRequestId;
			break;
		}
	}

	// Note: There are instances where our response gets broken up
	//       into multiple parts by the engine.  This is a simple
	//       hack that allows us to receive those messages too.
	if (unRequestId == m_unLastCookie)
	{
		bHasBlockListCookie = estiTRUE;
	}

	return bHasBlockListCookie;
}

unsigned int CstiContactManager::CoreRequestSend(CstiCoreRequest *poCoreRequest, EstiBool bRemoveUponError, unsigned int unExpectedResult)
{
	unsigned int unInternalCoreRequestId = 0;

	if (m_pCoreService)
	{
		poCoreRequest->RemoveUponCommunicationError() = bRemoveUponError;
		auto hResult = m_pCoreService->RequestSend(poCoreRequest, &unInternalCoreRequestId);
		if (!stiIS_ERROR (hResult))
		{
			PendingRequest req = {unInternalCoreRequestId, unExpectedResult};
			m_PendingCookies.push_back(req);
		}
	}

	return unInternalCoreRequestId;
}

void CstiContactManager::CallbackMessageSend(EstiCmMessage eMessage, size_t Param)
{
	if (m_pfnVPCallback)
	{
		m_pfnVPCallback(eMessage, Param, m_CallbackParam, 0);
	}
}

/**
 * \brief Sends a core request to get the changed contact list item
 */
void CstiContactManager::ContactListItemChangedHandle(CstiStateNotifyResponse *pStateNotifyResponse)
{
	//Get the Item Id from the response
	auto itemId = pStateNotifyResponse->ResponseStringGet(0);

#ifdef stiIMAGE_MANAGER
	// this event may have been generated to inform us of a profile photo update
	// but it may not actually tell us that the photo updated when it actually
	// did so we need to do applicable ImageDownloadURLGets after getting the
	// updated contact information from core.
	Lock();
	m_pendingPhotoCheck.insert(itemId);
	Unlock();
#endif  //End stiIMAGE_MANAGER

	CstiContactListItemSharedPtr contactItem = std::make_shared<CstiContactListItem> ();
	contactItem->ItemIdSet(itemId.c_str ());

	auto pRequest = new CstiCoreRequest ();
	pRequest->contactListItemGet(*contactItem);
	unsigned int unCoreRequestId = CoreRequestSend(pRequest, estiTRUE, CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT);
	
	// In a myPhone group it is possible to get a an add or edit event and then
	// delete event if another user in the group adds/edits a contact and then
	// deletes it before the other endpoints heartbeat.  This causes an error to
	// be displayed on the other endpoints that there was an error adding or editing
	// the contact(which is incorrect).  So we add the item ID and Core request ID
	// to a list that we check when we receive the delete event and if the item ID
	// of the delete event matches the add/edit event item ID, then we cancel the
	// contact list item get request and remove it form the pending request list.
	Lock();
	PendingContactListItemChangedRequest req = {itemId, unCoreRequestId};
	m_PendingContactListItemChangedRequests.push_back(req);
	Unlock();
}

/**
 * \brief Sends an event to remove the contact list item
 */
void CstiContactManager::ContactListItemRemovedHandle(CstiStateNotifyResponse *pStateNotifyResponse)
{
	//Get the Item Id from the response
	auto itemId = pStateNotifyResponse->ResponseStringGet(0);
	
	// In a myPhone group it is possible to get a an add or edit event and then
	// delete event if another user in the group adds/edits a contact and then
	// deletes it before the other endpoints heartbeat.  This causes an error to
	// be displayed on the other endpoints that there was an error adding or editing
	// the contact(which is incorrect).  So we add the item ID and Core request ID
	// to a list that we check when we receive the delete event and if the item ID
	// of the delete event matches the add/edit event item ID, then we cancel the
	// contact list item get request and remove it form the pending request list.
	for (auto it = m_PendingContactListItemChangedRequests.begin(); it != m_PendingContactListItemChangedRequests.end(); ++it)
	{
		if ((*it).m_ItemId == itemId)
		{
			m_pCoreService->RequestCancel ((*it).m_unCoreRequestId);

			for (auto it2 = m_PendingCookies.begin(); it2 != m_PendingCookies.end(); ++it2)
			{
				if ((*it2).m_unCookie == (*it).m_unCoreRequestId)
				{
					m_PendingCookies.erase(it2);
					break;
				}
			}

			m_PendingContactListItemChangedRequests.erase(it);
			break;
		}
	}

	CstiContactListItemSharedPtr contactItem = std::make_shared<CstiContactListItem> ();
	contactItem->ItemIdSet(itemId.c_str ());

	removeCstiContact(contactItem);

	// Notify UI to remove item
	auto pID = new CstiItemId;
	contactItem->ItemIdGet(pID);
	CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_DELETED, (size_t)pID);
}

#ifdef stiIMAGE_MANAGER

void CstiContactManager::DownloadURLGetResultHandle(CstiCoreResponse *poResponse)
{
	if (m_expectedPhotoDownloadURLs.empty())
	{
		return;
	}

	if (poResponse->ResponseResultGet() == estiOK)
	{
		Lock();

		IstiContactSharedPtr contact;

		if (getContactByPhoneNumber(poResponse->ResponseStringGet(0), &contact)
			== estiTRUE)
		{
			PhotoState& photo_state = m_photoUpdateState[contact];

			CstiContactListItem::EPhoneNumberType new_photo_num_type(
				CstiContactListItem::ePHONE_MAX);

			contact->PhoneNumberTypeGet(
				poResponse->ResponseStringGet(0),
				&new_photo_num_type);

			if (new_photo_num_type <= photo_state.number_type)
			{
				photo_state.number_type = new_photo_num_type;
				photo_state.guid = poResponse->ResponseStringGet(1);
				photo_state.timestamp = poResponse->ResponseStringGet(2);
			}
		}

		Unlock();
	}
	else if (!poResponse->ResponseStringGet(0).empty ())
	{
		Lock();

		IstiContactSharedPtr contact;

		if (getContactByPhoneNumber(poResponse->ResponseStringGet(0), &contact)
			== estiTRUE)
		{
			// Ensure there is a photo update state for this contact even though
			// this request failed... this can make the contact wind up with a
			// silhouette if there are no photos
			m_photoUpdateState[contact];
		}

		Unlock();
	}

	if (!poResponse->ResponseStringGet(0).empty ())
	{
		m_expectedPhotoDownloadURLs.erase(poResponse->ResponseStringGet(0));
	}

	if (m_expectedPhotoDownloadURLs.empty())
	{
		Lock();

		for (auto &contactPhotoState: m_photoUpdateState)
		{
			PhotoState& photo_state = contactPhotoState.second;
			bool photo_changed = true;
			std::string currentPhotoId;
			std::string currentPhotoTimestamp;
			IstiContactSharedPtr contact = contactPhotoState.first;

			contact->PhotoGet(&currentPhotoId);
			contact->PhotoTimestampGet(&currentPhotoTimestamp);

			if (!currentPhotoId.empty ()
				&& (photo_state.guid == currentPhotoId)
				&& !currentPhotoTimestamp.empty ()
				&& (photo_state.timestamp == currentPhotoTimestamp))
			{
				photo_changed = false;
			}

			// If the photo changed, tell core about it but be careful to
			// preserve avatars that have been selected for contacts that
			// don't have actual contact photos.
			if (photo_changed
				&& (!photo_state.guid.empty()
					|| CoreHasImage(currentPhotoId)))
			{
				// tell the image manager to purge cached versions of
				// the old photo so that it will fetch the updated
				// version the next time the image is requested
				if (   m_pImageManager
					&& CoreHasImage(currentPhotoId))
				{
					m_pImageManager->PurgeImageFromCache(currentPhotoId.c_str ());
				}

				if (photo_state.guid.empty())
				{
					photo_state.guid = SilhouetteAvatarGUID;
				}

				// update the cached photo information
				contact->PhotoSet(photo_state.guid);
				contact->PhotoTimestampSet(photo_state.timestamp);

				if (CoreHasImage(currentPhotoId))
				{
					// Tell core about the new photo for this contact
					favoritesPreserve(contact);		// Don't lose favorites info
					updateContact(contact);
				}

				// Tell interested parties that the contact was updated
				CallbackMessageSend(
					estiMSG_CONTACT_LIST_ITEM_EDITED,
					(size_t)new CstiItemId(contact->ItemIdGet()));
			}
		}

		m_photoUpdateState.clear();

		Unlock();
	}

}

#endif //End stiIMAGE_MANAGER


void CstiContactManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	if (m_bEnabled)
	{
		AuthenticatedSet (clientAuthenticateResult->responseSuccessful ? estiTRUE : estiFALSE);
	}
}


void CstiContactManager::ImportContacts(
	const std::string &phoneNumber,
	const std::string &password)
{
	auto pRequest = new CstiCoreRequest ();
	pRequest->contactsImport (phoneNumber.c_str (), password.c_str ());
	pRequest->retryOfflineSet (true);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eCONTACTS_IMPORT_RESULT);
}


#ifdef stiFAVORITES

void CstiContactManager::FavoritesMaxSet (int favoriteMax)
{
	m_Favorites.MaxCountSet(favoriteMax);
}

stiHResult CstiContactManager::FavoriteAdd (
	const CstiItemId &PhoneNumberId,
	bool bIsContact)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	CstiFavoriteSharedPtr favorite;
	CstiCoreRequest *pRequest = nullptr;

	// Create a CstiFavorite.
	favorite = std::make_shared<CstiFavorite> ();
	favorite->PhoneNumberIdSet(PhoneNumberId);
	favorite->IsContactSet(bIsContact);
	favorite->PositionSet(m_Favorites.CountGet() + 1);

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->favoriteItemAdd (*favorite);
	pRequest->contextItemSet (favorite);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT);

	Unlock();

	return hResult;
}

stiHResult CstiContactManager::FavoriteAddWithPosition (
	const CstiItemId &PhoneNumberId,
	bool bIsContact,
	int position)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	CstiFavoriteSharedPtr favorite;
	CstiCoreRequest *pRequest = nullptr;

	// Create a CstiFavorite.
	favorite = std::make_shared<CstiFavorite> ();
	favorite->PhoneNumberIdSet(PhoneNumberId);
	favorite->IsContactSet(bIsContact);
	favorite->PositionSet(position);

	// Send the core request
	pRequest = new CstiCoreRequest ();
	pRequest->favoriteItemAdd (*favorite);
	pRequest->contextItemSet (favorite);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT);

	Unlock();

	return hResult;
}

stiHResult CstiContactManager::FavoriteRemove (
	int nIndex)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	Lock();

	CstiFavoriteSharedPtr favorite, favoriteCopy;
	CstiCoreRequest *pRequest = nullptr;

	// Find the favorite
	favorite = m_Favorites.ItemGet(nIndex);
	stiTESTCOND(favorite, stiRESULT_ERROR);

	favoriteCopy = std::make_shared<CstiFavorite> (*favorite);

	// Send the core request
	pRequest = new CstiCoreRequest();
	pRequest->favoriteItemRemove(*favoriteCopy);
	pRequest->contextItemSet(favoriteCopy);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT);

STI_BAIL:
	Unlock();

	return hResult;
}

stiHResult CstiContactManager::FavoriteRemove (
	const CstiItemId &PhoneNumberId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	CstiFavoriteSharedPtr favorite, favoriteCopy;
	CstiCoreRequest *pRequest = nullptr;

	// Find the favorite
	favorite = FavoriteByPhoneNumberIdGet(PhoneNumberId);
	stiTESTCOND(favorite, stiRESULT_ERROR);

	favoriteCopy = std::make_shared<CstiFavorite> (*favorite);

	// Send the core request
	pRequest = new CstiCoreRequest();
	pRequest->favoriteItemRemove(*favoriteCopy);
	pRequest->contextItemSet(favoriteCopy);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT);

STI_BAIL:
	Unlock();

	return hResult;
}

stiHResult CstiContactManager::FavoriteMove (
	int nOldIndex,
	int nNewIndex)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	CstiFavoriteSharedPtr favorite, favoriteCopy;
	CstiCoreRequest *pRequest = nullptr;

	// Find the favorite
	favorite = m_Favorites.ItemGet(nOldIndex);
	stiTESTCOND(favorite, stiRESULT_ERROR);

	favoriteCopy = std::make_shared<CstiFavorite> (*favorite);

	favoriteCopy->PositionSet(nNewIndex);

	// Send the core request
	pRequest = new CstiCoreRequest();
	pRequest->favoriteItemEdit(*favoriteCopy);
	pRequest->contextItemSet(favoriteCopy);
	CoreRequestSend (pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT);

STI_BAIL:
	Unlock();

	return hResult;
}

stiHResult CstiContactManager::FavoriteListSet (
	const std::vector<CstiFavorite> &list)
{
	// Make a copy of the list
	SstiFavoriteList listCopy;

	listCopy.favorites = list;

	auto pRequest = new CstiCoreRequest();
	pRequest->favoriteListSet(listCopy);
	pRequest->contextItemSet(listCopy);
	CoreRequestSend(pRequest, estiTRUE, CstiCoreResponse::eFAVORITE_LIST_SET_RESULT);

	return stiRESULT_SUCCESS;
}

bool CstiContactManager::IsFavorite (
	const CstiItemId &PhoneNumberId) const
{
	return (FavoriteIndexByPhoneNumberIdGet(PhoneNumberId) != -1);
}

bool CstiContactManager::IsFavorite (
	const std::string &dialString) const
{
	bool bFound = false;

	Lock();

	if (!dialString.empty ())
	{
		int nCount = m_pdoc->CountGet();
		ContactListItemSharedPtr item;

		for (int i = 0; i < nCount; i++)
		{
			item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));

			CstiContactListItem::EPhoneNumberType eNumberType = item->DialStringFind(dialString);

			if (eNumberType != CstiContactListItem::ePHONE_MAX)
			{
				CstiItemId PhoneNumberId = item->PhoneNumberPublicIdGet(eNumberType);

				if (IsFavorite(PhoneNumberId))
				{
					bFound = true;
					break;
				}
			}
		}
	}

	Unlock();

	return bFound;
}

int CstiContactManager::FavoritesCountGet() const
{
	return m_Favorites.CountGet();
}

int CstiContactManager::FavoritesMaxCountGet() const
{
	return m_Favorites.MaxCountGet();
}

bool CstiContactManager::FavoritesFull() const
{
	return (m_Favorites.CountGet() >= m_Favorites.MaxCountGet());
}

CstiFavoriteSharedPtr CstiContactManager::FavoriteByIndexGet(
	int nIndex) const
{
	return m_Favorites.ItemGet(nIndex);
}

int CstiContactManager::FavoriteIndexByPhoneNumberIdGet(
	const CstiItemId &PhoneNumberId) const
{
	Lock();

	unsigned int unCount = m_Favorites.CountGet();
	int nIndex = -1;

	for (int i = 0; i < static_cast<int>(unCount); i++)
	{
		CstiFavoriteSharedPtr item = m_Favorites.ItemGet(i);
		if (PhoneNumberId == item->PhoneNumberIdGet())
		{
			nIndex = i;
			break;
		}
	}

	Unlock();

	return nIndex;
}

CstiFavoriteSharedPtr CstiContactManager::FavoriteByPhoneNumberIdGet(
	const CstiItemId &PhoneNumberId) const
{
	CstiFavoriteSharedPtr favorite;

	Lock();

	int nIndex = FavoriteIndexByPhoneNumberIdGet(PhoneNumberId);

	if (nIndex != -1)
	{
		favorite = m_Favorites.ItemGet(nIndex);
	}

	Unlock();

	return favorite;
}

IstiContactSharedPtr CstiContactManager::ContactByFavoriteGet(
	const CstiFavoriteSharedPtr &favorite) const
{
	ContactListItemSharedPtr item;

	Lock();

	if (favorite)
	{
		CstiItemId PhoneNumberId = favorite->PhoneNumberIdGet();
		int nCount = m_pdoc->CountGet();

		for (int i = 0; i < nCount; i++)
		{
			item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));

			CstiContactListItem::EPhoneNumberType eNumberType = item->PhoneNumberPublicIdFind(PhoneNumberId);

			if (eNumberType != CstiContactListItem::ePHONE_MAX)
			{
				break;
			}

			item = nullptr;
		}
	}

	Unlock();

	return item;
}

void CstiContactManager::CreateFavoriteFromNewNumbers(
		const ContactListItemSharedPtr &contactListItem)
{
	CstiItemId homeId = contactListItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_HOME);
	CstiItemId workId = contactListItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_WORK);
	CstiItemId cellId = contactListItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_CELL);
	int position = m_Favorites.CountGet() + 1;

	// Remove favorites first in case we are removing and adding a favorite in one transaction.
	if (homeId.IsValid() && !contactListItem->HomeFavoriteOnSaveGet() && IsFavorite(homeId))
	{
		FavoriteRemove(homeId);
		position--;
	}
	if (workId.IsValid() && !contactListItem->WorkFavoriteOnSaveGet() && IsFavorite(workId))
	{
		FavoriteRemove(workId);
		position--;
	}
	if (cellId.IsValid() && !contactListItem->MobileFavoriteOnSaveGet() && IsFavorite(cellId))
	{
		FavoriteRemove(cellId);
		position--;
	}
	
	// Add favorites.
	if (homeId.IsValid() && contactListItem->HomeFavoriteOnSaveGet() && !IsFavorite(homeId))
	{
		FavoriteAddWithPosition(homeId, true, position);
		position++;
	}
	if (workId.IsValid() && contactListItem->WorkFavoriteOnSaveGet() && !IsFavorite(workId))
	{
		FavoriteAddWithPosition(workId, true, position);
		position++;
	}
	if (cellId.IsValid() && contactListItem->MobileFavoriteOnSaveGet() && !IsFavorite(cellId))
	{
		FavoriteAddWithPosition(cellId, true, position);
		position++;
	}
}

void CstiContactManager::favoritesPreserve(const IstiContactSharedPtr &pItem)
{
	// There are times when a user may favorite/unfavorite a number at the same
	// time that they are editing a contact. This function is for cases where
	// that is NOT the case. We want to make sure the existing favorite info
	// stays preserved. (We're seeing a lot of instances where favorites
	// accidentally get deleted after we're told that a contact no longer has a
	// photo, for example. See bug IS-27544.)

	CstiItemId homeId = pItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_HOME);
	CstiItemId workId = pItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_WORK);
	CstiItemId cellId = pItem->PhoneNumberPublicIdGet(CstiContactListItem::ePHONE_CELL);

	if (homeId.IsValid())
	{
		pItem->HomeFavoriteOnSaveSet(IsFavorite(homeId));
	}

	if (workId.IsValid())
	{
		pItem->WorkFavoriteOnSaveSet(IsFavorite(workId));
	}

	if (cellId.IsValid())
	{
		pItem->MobileFavoriteOnSaveSet(IsFavorite(cellId));
	}
}

#endif

bool CstiContactManager::CompanyNameEnabled() const
{
#ifdef stiDEBUG
	// Always enable for debug builds
	return true;
#else
	// API Version will not be returned by older versions of Core that do not support Company Name
	return m_pCoreService->APIMajorVersionGet() > 0;
#endif
}

IstiContactManager::Response CstiContactManager::coreResponseConvert(CstiCoreResponse::EResponse responseType)
{
	Response message = Response::Unknown;

	switch (responseType)
	{
	case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
	case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
		message = Response::AddError;
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
	case CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT:
	case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
		message = Response::EditError;
		break;

	case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
	case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
		message = Response::DeleteError;
		break;

	case CstiCoreResponse::eCALL_LIST_GET_RESULT:
	case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
		message = Response::GetListError;
		break;

	case CstiCoreResponse::eCALL_LIST_SET_RESULT:
	case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
		message = Response::SetListError;
		break;

	case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
		message = Response::ImportError;
		break;

	default:
		break;
	}

	return message;
}

void CstiContactManager::errorSend(CstiCoreResponse *coreResponse, bool favorite)
{
	Response message = coreResponseConvert(coreResponse->ResponseGet());
	stiHResult errCode = stiRESULT_ERROR;

	if (coreResponse->ErrorGet() == CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED)
	{
		errCode = stiRESULT_OFFLINE_ACTION_NOT_ALLOWED;
	}

	if (favorite)
	{
		favoriteErrorSignal.Emit(message, errCode);
	}
	else
	{
		contactErrorSignal.Emit(message, errCode);
	}
}

ISignal<IstiContactManager::Response, stiHResult>& CstiContactManager::contactErrorSignalGet ()
{
	return contactErrorSignal;
}

ISignal<IstiContactManager::Response, stiHResult>& CstiContactManager::favoriteErrorSignalGet ()
{
	return favoriteErrorSignal;
}