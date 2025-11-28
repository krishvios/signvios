//
//  CstiLDAPDirectoryContactManager.cpp
//  ntouchMac
//
//  Created by J.R. Blackham on 2/4/15.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifdef stiLDAP_CONTACT_LIST

#include "CstiLDAPDirectoryContactManager.h"
#include "XMLList.h"
#include "ContactListItem.h"
#include "CstiCoreService.h"
#include "LDAPDirectoryContactListItem.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "stiRemoteLogEvent.h"
#include "CstiPhoneNumberValidator.h"
#include "stiTaskInfo.h"

#include <ldap.h>

#include <sstream>

#define stiLDAP_DIRECTORY_MANAGER_TASK_NAME				 "stiLDAP_DIRECTORY_MANAGER"
#define stiLDAP_DIRECTORY_MANAGER_TASK_PRIORITY			 251
#define stiLDAP_DIRECTORY_MANAGER_MAX_MESSAGES_IN_QUEUE	 10
#define stiLDAP_DIRECTORY_MANAGER_MAX_MSG_SIZE			 512
#define stiLDAP_DIRECTORY_MANAGER_STACK_SIZE			 2048
#define LDAP_DEFAULT_NAME 	"directory32123"
#define LDAP_DEFAULT_PW		  "1nc0rp0r@t3d"

using namespace WillowPM;

stiEVENT_MAP_BEGIN (CstiLDAPDirectoryContactManager)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiLDAPDirectoryContactManager::EventTaskShutdown)
stiEVENT_DEFINE (estiLDAP_RELOAD_CONTACTS, CstiLDAPDirectoryContactManager::EventReloadContacts)
stiEVENT_DEFINE (estiLDAP_CREDENTIALS_VALIDATE, CstiLDAPDirectoryContactManager::EventCredentialsValidate)
stiEVENT_MAP_END (CstiLDAPDirectoryContactManager)
stiEVENT_DO_NOW (CstiLDAPDirectoryContactManager)

CstiLDAPDirectoryContactManager::CstiLDAPDirectoryContactManager(
	bool bEnabled,
	CstiCoreService *pCoreService,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam)
	: 
	CstiOsTaskMQ (
		nullptr,
		0,
		(size_t)this,
		stiLDAP_DIRECTORY_MANAGER_MAX_MESSAGES_IN_QUEUE,
		stiLDAP_DIRECTORY_MANAGER_MAX_MSG_SIZE,
		stiLDAP_DIRECTORY_MANAGER_TASK_NAME,
		stiLDAP_DIRECTORY_MANAGER_TASK_PRIORITY,
		stiLDAP_DIRECTORY_MANAGER_STACK_SIZE),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_bEnabled(bEnabled)
{
	std::string DynamicDataFolder;
	
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	
	std::stringstream ContactListFile;
	
	ContactListFile << DynamicDataFolder << "LDAPDirectoryContactList.xml";
	
	SetFilename(ContactListFile.str ());
	
	LDAPSearchFilterSet("(objectClass=organizationalPerson)");
}

CstiLDAPDirectoryContactManager::~CstiLDAPDirectoryContactManager()
{
	Shutdown();
	WaitForShutdown();
}

stiHResult CstiLDAPDirectoryContactManager::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	XMLManager::init();

	hResult = Startup();

	m_LDAPdn = LDAP_DEFAULT_NAME;
	m_LDAPpw = LDAP_DEFAULT_PW;

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	// Try to login with the default credentials.
	LDAPCredentialsValidate (m_LDAPdn, 
							 m_LDAPpw);
#endif

	return hResult;
}

IstiContactSharedPtr CstiLDAPDirectoryContactManager::ContactCreate()
{
	return std::make_shared<LDAPDirectoryContactListItem> ();
}


IstiContactSharedPtr CstiLDAPDirectoryContactManager::getContact(int nIndex) const
{
	Lock();
	auto contact = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet(nIndex));
	Unlock();
	return contact;
}


int CstiLDAPDirectoryContactManager::getNumContacts() const
{
	Lock();
	int nCount = m_pdoc->CountGet();
	Unlock();
	
	return nCount;
}


EstiBool CstiLDAPDirectoryContactManager::ContactByIDGet(const CstiItemId &Id, IstiContactSharedPtr *contact) const
{
	Lock();
	
	EstiBool bFound = estiFALSE;
	
	int Count = m_pdoc->CountGet();
	ContactListItemSharedPtr item;
	
	for (int i = 0; i < Count; i++)
	{
		item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));
		if (Id == item->ItemIdGet())
		{
			*contact = item;
			bFound = estiTRUE;
			break;
		}
	}
	
	Unlock();
	
	return bFound;
}

IstiContactSharedPtr CstiLDAPDirectoryContactManager::contactByIDGet(
	const CstiItemId &id) const
{
	IstiContactSharedPtr contact;
	IstiContactSharedPtr contactCopy;

	Lock();

	if (estiTRUE == ContactByIDGet(id, &contact))
	{
		contactCopy = std::make_shared<LDAPDirectoryContactListItem>(contact);
	}

	Unlock();

	return contactCopy;
}

EstiBool CstiLDAPDirectoryContactManager::getContactByPhoneNumber(
	const std::string &phoneNumber,
	IstiContactSharedPtr *contact) const
{
	Lock();
	
	EstiBool bFound = estiFALSE;
	
	if (!phoneNumber.empty ())
	{
		bool bMatch = false;
		int nCount = m_pdoc->CountGet();
		ContactListItemSharedPtr item;
		IstiContactSharedPtr candidate;
		std::string candidateName;
		
		for (int i = 0; i < nCount; i++)
		{
			item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));
			
			auto itemName = item->NameGet ();
			
			//
			// If NameGet returns an empty string then this is an issue.
			// However, we will just continue with the next item.
			//
			if (itemName.empty ())
			{
				continue;
			}
			
			if (candidate)
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
			bMatch = item->DialStringMatch(phoneNumber);
#else
			item->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_HOME, &bMatch);
			
			if (!bMatch)
			{
				item->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_WORK, &bMatch);
				
				if (!bMatch)
				{
					item->ValueCompare(phoneNumber, ContactListItem::eCONTACT_NUMBER_CELL, &bMatch);
				}
			}
#endif
			
			if (bMatch)
			{
				candidate = item;
				
				candidateName = itemName;
			}
		}
		
		if (candidate)
		{
			*contact = candidate;
			bFound = estiTRUE;
		}
	}
	
	Unlock();
	
	return bFound;
}

IstiContactSharedPtr CstiLDAPDirectoryContactManager::contactByPhoneNumberGet(
	const std::string &phoneNumber) const
{
	IstiContactSharedPtr contact;
	IstiContactSharedPtr contactCopy;

	Lock();

	if (estiTRUE == getContactByPhoneNumber(phoneNumber, &contact))
	{
		contactCopy = std::make_shared<LDAPDirectoryContactListItem>(contact);
	}

	Unlock();

	return contactCopy;
}

int CstiLDAPDirectoryContactManager::getContactIndex(const std::string &id) const
{
	Lock();
	
	int Index = -1;
	
	if (!id.empty ())
	{
		int Count = m_pdoc->CountGet();
		ContactListItemSharedPtr item;
		
		for (int i = 0; i < Count; i++)
		{
			item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));
			
			if (item->IDGet() == id)
			{
				Index = i;
				break;
			}
		}
	}
	
	Unlock();
	
	return Index;
}


int CstiLDAPDirectoryContactManager::ContactIndexGet(const CstiItemId &Id) const
{
	Lock();
	
	int Index = -1;
	
	int Count = m_pdoc->CountGet();
	ContactListItemSharedPtr item;
	
	for (int i = 0; i < Count; i++)
	{
		item = std::static_pointer_cast<ContactListItem> (m_pdoc->ItemGet (i));
		if (Id == item->ItemIdGet())
		{
			Index = i;
			break;
		}
	}
	
	Unlock();
	
	return Index;
}

void CstiLDAPDirectoryContactManager::clearContacts()
{
	Lock();
	
	m_pdoc->ListFree();
	CallbackMessageSend(estiMSG_LDAP_DIRECTORY_CONTACT_LIST_CHANGED, (size_t)0);

	
	Unlock();
}

bool CstiLDAPDirectoryContactManager::isFull() const
{
	Lock();
	bool bFull = m_pdoc->CountGet() >= m_pdoc->MaxCountGet();
	Unlock();
	
	return bFull;
}


void CstiLDAPDirectoryContactManager::setMaxCount(size_t Count)
{
	Lock();
	m_pdoc->MaxCountSet(Count);
	Unlock();
}


size_t CstiLDAPDirectoryContactManager::getMaxCount() const
{
	Lock();
	size_t unCount = m_pdoc->MaxCountGet();
	Unlock();
	
	return unCount;
}


EstiBool CstiLDAPDirectoryContactManager::CoreResponseHandle(CstiCoreResponse *poResponse)
{
	Lock();
	
	EstiBool bHandled = estiFALSE;
	
	switch (poResponse->ResponseGet())
	{
			
		case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT:
		{
			if (m_bEnabled)
			{
				if (poResponse->ResponseResultGet() == estiOK)
				{
//					ReloadContactsFromLDAPDirectory();
				}
			}
		}
			break;
			
			
		default:
			break;
	}
	
	Unlock();
	
	return bHandled;
}


//EstiBool CstiLDAPDirectoryContactManager::StateNotifyEventHandle(CstiStateNotifyResponse *pStateNotifyResponse)
//{
//	Lock();
//	
//	EstiBool bHandled = estiFALSE;
//	
//	if (pStateNotifyResponse
//	 && pStateNotifyResponse->ResponseValueGet () == CstiCallList::eCONTACT)
//	{
//		switch (pStateNotifyResponse->ResponseGet ())
//		{
//			case CstiStateNotifyResponse::eCALL_LIST_CHANGED:
//			{
//				// Send the core request to get the new list
//				ReloadContactsFromLDAPDirectory();
//				
//				delete pStateNotifyResponse;
//				pStateNotifyResponse = NULL;
//				bHandled = estiTRUE;
//				break;
//			}
//			case CstiStateNotifyResponse::eCALL_LIST_ITEM_ADD:
//			case CstiStateNotifyResponse::eCALL_LIST_ITEM_EDIT:
//			{
//				ContactListItemChangedHandle(pStateNotifyResponse);
//				
//				delete pStateNotifyResponse;
//				pStateNotifyResponse = NULL;
//				bHandled = estiTRUE;
//				break;
//			}
//			case CstiStateNotifyResponse::eCALL_LIST_ITEM_REMOVE:
//			{
//				ContactListItemRemovedHandle(pStateNotifyResponse);
//				
//				delete pStateNotifyResponse;
//				pStateNotifyResponse = NULL;
//				bHandled = estiTRUE;
//				break;
//			}
//			default:
//				break;
//		}
//	}
//	
//	Unlock();
//	
//	return bHandled;
//}
//

//EstiBool CstiLDAPDirectoryContactManager::RemovedUponCommunicationErrorHandle(SstiRemovedRequest *pRemovedRequest)
//{
//	Lock();
//	
//	EstiBool bHandled = estiFALSE;
//	unsigned int unExpectedResult = 0;
//	
//	if (RemoveRequestId (pRemovedRequest->unRequestId, &unExpectedResult))
//	{
//		switch (unExpectedResult)
//		{
//			case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
//			{
//				ContactListItem *contactListItem = (ContactListItem *)pRemovedRequest->pContext;
//				//CstiContactListItem *pCstiItem = ((ContactListItem *)contactListItem)->GetCstiContactListItem();
//				CstiItemId *pID = new CstiItemId;
//				
//				if (contactListItem)
//				{
//					*pID = contactListItem->ItemIdGet();
//				}
//				
//				CallbackMessageSend (estiMSG_CONTACT_LIST_ITEM_ERROR, (size_t)pID);
//				
//				delete contactListItem;
//				contactListItem = NULL;
//				
//				break;
//			}
//				
//			case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
//			{
//				ContactListItem *contactListItem = (ContactListItem *)pRemovedRequest->pContext;
//				//CstiContactListItem *pCstiItem = ((ContactListItem *)contactListItem)->GetCstiContactListItem();
//				CstiItemId *pID = new CstiItemId;
//				
//				if (contactListItem)
//				{
//					*pID = contactListItem->ItemIdGet();
//				}
//				
//				CallbackMessageSend (estiMSG_CONTACT_LIST_ITEM_ERROR, (size_t)pID);
//				
//				delete contactListItem;
//				contactListItem = NULL;
//				
//				break;
//			}
//				
//			case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
//			case CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT:
//			case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
//			case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
//			{
//				CstiFavorite *pCstiItem = (CstiFavorite *)pRemovedRequest->pContext;
//				CstiItemId *pID = new CstiItemId;
//				
//				if (pCstiItem)
//				{
//					*pID = pCstiItem->PhoneNumberIdGet();
//				}
//				
//				CallbackMessageSend (estiMSG_FAVORITE_ITEM_ERROR, (size_t)pID);
//				
//				delete pCstiItem;
//				pCstiItem = NULL;
//				
//				break;
//			}
//				
//			case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
//			{
//				CstiFavoriteList *pList = (CstiFavoriteList *)pRemovedRequest->pContext;
//				delete pList;
//				pList = NULL;
//				
//				CallbackMessageSend (estiMSG_FAVORITE_LIST_SET_ERROR, 0);
//				
//				break;
//			}
//				
//			default:
//			{
//				CstiContactListItem *pCstiItem = (CstiContactListItem*) pRemovedRequest->pContext;
//				CstiItemId *pID = new CstiItemId;
//				
//				if (pCstiItem)
//				{
//					pCstiItem->ItemIdGet(pID);
//				}
//				
//				CallbackMessageSend (estiMSG_CONTACT_LIST_ITEM_ERROR, (size_t)pID);
//				
//				delete pCstiItem;
//				pCstiItem = NULL;
//				
//				break;
//			}
//				
//		}
//		
//		delete pRemovedRequest;
//		pRemovedRequest = NULL;
//		
//		bHandled = estiTRUE;
//	}
//	
//	Unlock();
//	
//	return bHandled;
//}

void CstiLDAPDirectoryContactManager::ReloadContactsFromLDAPDirectory()
{
    stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bEnabled)
	{
		Lock();
		m_pdoc->ListFree();
		
		LDAP *ld = nullptr;
		int result;
		int auth_method = LDAP_AUTH_UNKNOWN;
		int desired_version = LDAP_VERSION3;
		int scope = LDAP_SCOPE_DEFAULT;
		
		std::string loginName;
		std::string password;
		
		std::string phoneUserId;
		
		// Athenticate the user name and password
		switch (m_eLDAPServerRequiresAuthentication)
		{
			case eLDAPAuthenticateMethod_NONE:
			{
				auth_method = LDAP_AUTH_SIMPLE;
				break;
			}
			case eLDAPAuthenticateMethod_SIMPLE:
			{
				auth_method = LDAP_AUTH_SIMPLE;
				
				loginName = m_LDAPdn;
				password = m_LDAPpw;
				break;
			}
			case eLDAPAuthenticateMethod_SASL:
			{
				auth_method = LDAP_AUTH_SASL;
				break;
			}
			default:
			{
				auth_method = LDAP_AUTH_SIMPLE;
				
				loginName = m_LDAPdn;
				password = m_LDAPpw;
				break;
			}
		}
		
		switch (m_nLDAPScope)
		{
			case 0:
				scope = LDAP_SCOPE_BASE;
				break;
			case 1:
				scope = LDAP_SCOPE_ONELEVEL;
				break;
			case 2:
				scope = LDAP_SCOPE_SUBTREE;
				break;
			default:
				scope = LDAP_SCOPE_SUBTREE;
				break;
		}

		int l_rc;
		int l_entries;
		int l_entry_count = 0;
		int morePages;
		int l_errcode = 0;
		int page_nbr = 1;
		unsigned long pageSize = 500;
		struct berval *cookie = nullptr;
		char pagingCriticality = 'T';
		char *l_dn = nullptr;
		ber_int_t totalCount;
		LDAPControl *pageControl = nullptr;
		LDAPControl *M_controls[2] = { nullptr, nullptr };
		LDAPControl **returnedControls = nullptr;
		LDAPMessage *l_result = nullptr;
		LDAPMessage *l_entry = nullptr;
		BerElement* ber;
		
		char* attr;
		char** vals;
		int i;
		
		struct timeval     timeOut = {10,0};   /* 10 second connection timeout */

#ifndef LDAP_DEPRECATED
		struct berval cred;
		struct berval *servcred;
		cred.bv_val = m_LDAPpw;
		cred.bv_len = strlen(m_LDAPpw);
#endif
		
        bool hasTelephoneNumberField = !m_LDAPtelephoneNumberField.empty ();
        bool hasHomeNumberField = !m_LDAPhomeNumberField.empty ();
        bool hasMobileNumberField = !m_LDAPmobileNumberField.empty ();
        
        char *attrs[] = {(char *)"cn",
            hasTelephoneNumberField ? (char *)m_LDAPtelephoneNumberField.c_str () : (hasHomeNumberField ? (char *)m_LDAPhomeNumberField.c_str () : (hasMobileNumberField ? (char *)m_LDAPmobileNumberField.c_str () : nullptr)),
            (hasHomeNumberField && hasTelephoneNumberField) ? (char *)m_LDAPhomeNumberField.c_str () : ((hasMobileNumberField && (hasHomeNumberField || hasTelephoneNumberField)) ? (char *)m_LDAPmobileNumberField.c_str () : nullptr),
            (hasMobileNumberField && hasHomeNumberField && hasTelephoneNumberField) ? (char *)m_LDAPmobileNumberField.c_str () : nullptr,
            nullptr};
        
        std::string filter = std::string("(|")
        + (!m_LDAPtelephoneNumberField.empty () ? std::string("(") + m_LDAPtelephoneNumberField + std::string("=*)") : std::string(""))
        + (!m_LDAPhomeNumberField.empty () ? std::string("(") + m_LDAPhomeNumberField + std::string("=*)") : std::string(""))
        + (!m_LDAPmobileNumberField.empty () ? std::string("(") + m_LDAPmobileNumberField + std::string("=*)") : std::string(""))
        + std::string(")");

		PropertyManager *pPM = PropertyManager::getInstance ();
		
		pPM->propertyGet(PHONE_USER_ID, &phoneUserId);
		
		std::string ldapURI;
		if (m_nLDAPServerUseSSL == 0)
		{
			ldapURI = "ldap://";
		}
		else if (m_nLDAPServerUseSSL == 1)
		{
			ldapURI ="ldaps://";
		}
		ldapURI = ldapURI + m_LDAPhost;
		
		char szPort[7];
		
		snprintf (szPort, sizeof(szPort), ":%d", m_nLDAPServerPort);
		
		ldapURI += szPort;
		
		// If we do not need to update the authentication, but the username or password have changed
		// continue.  NULL usernames and passwords are valid.
		if ((m_eLDAPServerRequiresAuthentication == eLDAPAuthenticateMethod_NONE) ||
			!m_LDAPdn.empty () || !m_LDAPpw.empty ())
		{
			if (m_nLDAPServerUseSSL == 1)
			{
				/* set the LDAP_OPT_X_TLS_REQUIRE_CERT option */
				int reqcert = LDAP_OPT_X_TLS_ALLOW;

				if ((result = ldap_set_option(nullptr, LDAP_OPT_X_TLS_REQUIRE_CERT, &reqcert)) != LDAP_OPT_SUCCESS)
				{
					stiTrace("ldap_set_option 'LDAP_OPT_X_TLS_REQUIRE_CERT' %d: %s\n", reqcert, ldap_err2string(result));
					CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
					stiTHROW_NOLOG(stiRESULT_ERROR);
				}
			}
			
			if ((result = ldap_initialize(&ld, ldapURI.c_str())) != LDAP_SUCCESS)
			{
				stiTrace("Cannot initialise LDAP at '%s': %s\n",ldapURI.c_str(), ldap_err2string(result));
				CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
				stiTHROW_NOLOG(stiRESULT_ERROR);
			}
			
			/* set the LDAP version to be 3 */
			if ((result = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version)) != LDAP_OPT_SUCCESS)
			{
				stiTrace("ldap_set_option 'LDAP_OPT_PROTOCOL_VERSION' %d: %s\n", desired_version, ldap_err2string(result));
				CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
				stiTHROW_NOLOG(stiRESULT_ERROR);
			}
			
			if ((result = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeOut)) != LDAP_OPT_SUCCESS)
			{
				stiTrace("ldap_set_option 'LDAP_OPT_NETWORK_TIMEOUT' %ld.%.6ld: %s\n", timeOut.tv_sec, timeOut.tv_usec, ldap_err2string(result));
				CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
				stiTHROW_NOLOG(stiRESULT_ERROR);
			}
			
			/* authenticate */
			
			result = ldap_bind_s(ld, loginName.c_str (), password.c_str (), auth_method);
			if (result != LDAP_SUCCESS)
			{
				stiTrace("ldap_bind : %s\n", ldap_err2string(result));

				switch (result)
				{
					case LDAP_SERVER_DOWN:
						CallbackMessageSend(estiMSG_LDAP_DIRECTORY_SERVER_UNAVAILABLE, (size_t)0);
						break;
					case LDAP_INVALID_CREDENTIALS:
						CallbackMessageSend(estiMSG_LDAP_DIRECTORY_INVALID_CREDENTIALS, (size_t)0);
						break;
					default:
						CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
						break;
				}
				// Log to Splunk that we could not bind to the LDAP server
				stiRemoteLogEventSend ("EventType=LDAP Reason=\"AuthenticationFailed\" CoreId=%s", phoneUserId.c_str ());
				stiTHROW_NOLOG(stiRESULT_ERROR);
			}

			/* search */
			
			/******************************************************************/
			/* Get one page of the returned results each time                 */
			/* through the loop                                               */
			do
			{
				l_rc = ldap_create_page_control(ld, pageSize, cookie, pagingCriticality, &pageControl);

				/* Insert the control into a list to be passed to the search.     */
				M_controls[0] = pageControl;

				/* Search for entries in the directory using the parmeters.       */
				l_rc = ldap_search_ext_s(ld, m_LDAPbase.c_str (), scope, filter.c_str(), attrs, 0, M_controls, nullptr, nullptr, 0, &l_result);
				if ((l_rc != LDAP_SUCCESS) & (l_rc != LDAP_PARTIAL_RESULTS))
				{
					stiTrace("ldap_search_s : %s\n", ldap_err2string(l_rc));

					CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
					stiTHROW_NOLOG(stiRESULT_ERROR);
				}

				/* Parse the results to retrieve the contols being returned.      */
				l_rc = ldap_parse_result(ld, l_result, &l_errcode, nullptr, nullptr, nullptr, &returnedControls, false);

				if (cookie != nullptr)
				{
					ber_bvfree(cookie);
					cookie = nullptr;
				}

				/* Parse the page control returned to get the cookie and          */
				/* determine whether there are more pages.                        */
				l_rc = ldap_parse_page_control(ld, returnedControls, &totalCount, &cookie);

				/* Determine if the cookie is not empty, indicating there are more pages */
				/* for these search parameters. */
				if (cookie && cookie->bv_val != nullptr && (strlen(cookie->bv_val) > 0))
				{
					morePages = true;
				}
				else
				{
					morePages = false;
				}

				/* Cleanup the controls used. */
				if (returnedControls != nullptr)
				{
					ldap_controls_free(returnedControls);
					returnedControls = nullptr;
				}
				M_controls[0] = nullptr;
				ldap_control_free(pageControl);
				pageControl = nullptr;

				/******************************************************************/
				/* Disply the returned result                                     */
				/*                                                                */
				/* Determine how many entries have been found.                    */
	//			if (morePages == true)
	//				printf("===== Page : %d =====\n", page_nbr);

				l_entries = ldap_count_entries(ld, l_result);
				
				if (l_entries > 0)
				{
					l_entry_count = l_entry_count + l_entries;
				}
				
				/* Iterate through the returned entries */
				for (l_entry = ldap_first_entry(ld, l_result); l_entry != nullptr; l_entry = ldap_next_entry(ld, l_entry))
				{
					IstiContactSharedPtr istiContact = ContactCreate();
					bool hasTelephoneNumber = false;
					bool hasHomeNumber = false;
					bool hasMobileNumber = false;

					if((l_dn = ldap_get_dn(ld, l_entry)) != nullptr)
					{
						CstiItemId id(l_dn);
						istiContact->ItemIdSet(id);

	//					printf("dn: %s\n", l_dn);
						ldap_memfree(l_dn);
					}

					for (attr = ldap_first_attribute(ld, l_entry, &ber);
						attr != nullptr;
						attr = ldap_next_attribute(ld, l_entry, ber))
					{
						if ((vals = ldap_get_values(ld, l_entry, attr)) != nullptr)
						{
							for (i = 0; vals[i] != nullptr; i++)
							{
								if (strcmp(attr, "cn") == 0)
								{
									istiContact->NameSet(vals[i]);
	//								printf("%s: %s\n", attr, vals[i]);
								}

								if (attr == m_LDAPtelephoneNumberField)
								{
									std::string newPhoneNumber;
									stiHResult result = CstiPhoneNumberValidator::InstanceGet()->PhoneNumberReformat(vals[i], newPhoneNumber);

									if (result == stiRESULT_SUCCESS)
									{
										istiContact->PhoneNumberSet(CstiContactListItem::ePHONE_WORK, newPhoneNumber);
										hasTelephoneNumber = true;
									}
	//								printf("%s: %s %s\n", attr, vals[i], szNewPhoneNumber);
								}

								if (attr == m_LDAPhomeNumberField)
								{
									std::string newPhoneNumber;
									stiHResult result = CstiPhoneNumberValidator::InstanceGet()->PhoneNumberReformat(vals[i], newPhoneNumber);

									if (result == stiRESULT_SUCCESS)
									{
										istiContact->PhoneNumberSet(CstiContactListItem::ePHONE_HOME, newPhoneNumber);
										hasHomeNumber = true;
									}
	//								printf("%s: %s %s\n", attr, vals[i], szNewPhoneNumber);
								}

								if (attr == m_LDAPmobileNumberField)
								{
									std::string newPhoneNumber;
									stiHResult result = CstiPhoneNumberValidator::InstanceGet()->PhoneNumberReformat(vals[i], newPhoneNumber);
									if (result == stiRESULT_SUCCESS)
									{
										istiContact->PhoneNumberSet(CstiContactListItem::ePHONE_CELL, newPhoneNumber);
										hasMobileNumber = true;
									}
	//								printf("%s: %s %s\n", attr, vals[i], szNewPhoneNumber);
								}
							}

							ldap_value_free(vals);
						}
						
						ldap_memfree(attr);
					}

					if (ber != nullptr)
					{
						ber_free(ber,0);
					}

					if (hasTelephoneNumber || hasHomeNumber || hasMobileNumber)
					{
						m_pdoc->ItemAdd(std::static_pointer_cast<ContactListItem> (istiContact));
					}
					
	//				printf("\n");
				}
				
				/* Free the search results.                                       */
				ldap_msgfree(l_result);
				l_result = nullptr;
				page_nbr = page_nbr + 1;
				
			} while (static_cast<bool>(morePages));
			
			printf("The number of entries returned was %d\n\n", l_entry_count);
			
			// Log to Splunk the number of contacts downloaded from the LDAP server
			stiRemoteLogEventSend ("EventType=LDAP Reason=\"ContactsDownloaded\" NumContacts=%d CoreId=%s",l_entry_count, phoneUserId.c_str ());
		}
		
STI_BAIL:
		/* clean up */
		if (l_result != nullptr)
		{
			ldap_msgfree(l_result);
		}
		
		/* Free the cookie since all the pages for these search parameters   */
		/* have been retrieved.                                              */
		ber_bvfree(cookie);
		cookie = nullptr;

		if (ld != nullptr)
		{
			result = ldap_unbind_s(ld);
			if (result != 0)
			{
				stiTrace("ldap_unbind_s: %s\n", ldap_err2string(result));
			}
		}
		
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
        if (hResult == stiRESULT_SUCCESS)
#endif        		
        {
            CallbackMessageSend(estiMSG_LDAP_DIRECTORY_CONTACT_LIST_CHANGED, (size_t)0);
        }
		
		Unlock();
	}
}

IstiContactSharedPtr CstiLDAPDirectoryContactManager::ContactByFavoriteGet(const CstiFavoriteSharedPtr &pFavorite) const
{
	ContactListItemSharedPtr item;
	
	Lock();
	
	if (pFavorite)
	{
		CstiItemId PhoneNumberId = pFavorite->PhoneNumberIdGet();
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


//void CstiLDAPDirectoryContactManager::addCstiContact(CstiContactListItem *pCstiItem)
//{
//	ContactListItem *pContact = new ContactListItem(pCstiItem);
//	m_pdoc->ItemAdd(pContact);
//	
//	startSaveDataTimer();
//}


//void CstiLDAPDirectoryContactManager::updateCstiContact(CstiContactListItem *pCstiItem)
//{
//	CstiItemId Id;
//	pCstiItem->ItemIdGet(&Id);
//	
//	if (Id.IsValid())
//	{
//		IstiContact *pContact = NULL;
//		ContactByIDGet(Id, &pContact);
//		
//		if (pContact)
//		{
//			// update the contact
//			((ContactListItem *)pContact)->CopyCstiContactListItem(pCstiItem);
//			startSaveDataTimer();
//		}
//		else
//		{
//			// Couldn't find the ID...just add a new item
//			addCstiContact(pCstiItem);
//			
//			// Send event to update the UI
//			CstiItemId *pID = new CstiItemId;
//			pCstiItem->ItemIdGet(pID);
//			CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_ADDED, (size_t)pID);
//		}
//	}
//	else
//	{
//		// No ID...just add a new item
//		addCstiContact(pCstiItem);
//	}
//}
//
//
//void CstiLDAPDirectoryContactManager::removeCstiContact(CstiContactListItem *pCstiItem)
//{
//	CstiItemId Id;
//	pCstiItem->ItemIdGet(&Id);
//	int index = ContactIndexGet(Id);
//	
//	if (index != -1)
//	{
//		m_pdoc->ItemDestroy(index);
//		startSaveDataTimer();
//	}
//}
//
//
XMLListItemSharedPtr CstiLDAPDirectoryContactManager::NodeToItemConvert(IXML_Node *pNode)
{
	Lock();
	XMLListItemSharedPtr item = std::make_shared<ContactListItem> (pNode);
	Unlock();
	
	return item;
}

///*!
// * Utility function to determine if the request id of the given core
// * response was one that we were waiting for.  If so, the request
// * id will be removed from the list.
// */
//EstiBool CstiLDAPDirectoryContactManager::RemoveRequestId(unsigned int unRequestId, unsigned int *punExpectedResult)
//{
//	EstiBool bHasBlockListCookie = estiFALSE;
//	
//	for (auto it = m_PendingCookies.begin(); it != m_PendingCookies.end(); ++it)
//	{
//		if ((*it).m_unCookie == unRequestId)
//		{
//			bHasBlockListCookie = estiTRUE;
//			if (punExpectedResult)
//			{
//				*punExpectedResult = (*it).m_unExpectedResult;
//			}
//			m_PendingCookies.erase(it);
//			m_unLastCookie = unRequestId;
//			break;
//		}
//	}
//	
//	// Note: There are instances where our response gets broken up
//	//       into multiple parts by the engine.  This is a simple
//	//       hack that allows us to receive those messages too.
//	if (unRequestId == m_unLastCookie)
//	{
//		bHasBlockListCookie = estiTRUE;
//	}
//	
//	return bHasBlockListCookie;
//}

//unsigned int CstiLDAPDirectoryContactManager::CoreRequestSend(CstiCoreRequest *poCoreRequest, EstiBool bRemoveUponError, unsigned int unExpectedResult)
//{
//	unsigned int unInternalCoreRequestId = 0;
//	
//	if (m_pCoreService)
//	{
//		poCoreRequest->RemoveUponCommunicationError() = bRemoveUponError;
//		auto hResult = m_pCoreService->RequestSend(poCoreRequest, &unInternalCoreRequestId);
//		if (!stiIS_ERROR(hResult))
//		{
//			PendingRequest req = {unInternalCoreRequestId, unExpectedResult};
//			m_PendingCookies.push_back(req);
//		}
//	}
//	
//	return unInternalCoreRequestId;
//}

void CstiLDAPDirectoryContactManager::CallbackMessageSend(EstiCmMessage eMessage, size_t Param)
{
	if (m_pfnVPCallback)
	{
		m_pfnVPCallback(eMessage, Param, m_CallbackParam, 0);
	}
}

///**
// * \brief Sends a core request to get the changed contact list item
// */
//void CstiLDAPDirectoryContactManager::ContactListItemChangedHandle(CstiStateNotifyResponse *pStateNotifyResponse)
//{
//	//Get the Item Id from the response
//	const char *pszItemId = pStateNotifyResponse->ResponseStringGet(0);
//	
//	CstiContactListItem *pContactItem = new CstiContactListItem();
//	pContactItem->ItemIdSet(pszItemId);
//	
//	CstiCoreRequest *pRequest = new CstiCoreRequest ();
//	pRequest->contactListItemGet(pContactItem);
//	CoreRequestSend(pRequest);
//	
//	delete pContactItem;
//	pContactItem = NULL;
//}

///**
// * \brief Sends an event to remove the contact list item
// */
//void CstiLDAPDirectoryContactManager::ContactListItemRemovedHandle(CstiStateNotifyResponse *pStateNotifyResponse)
//{
//	//Get the Item Id from the response
//	const char *pszItemId = pStateNotifyResponse->ResponseStringGet(0);
//	
//	CstiContactListItem *pContactItem = new CstiContactListItem();
//	pContactItem->ItemIdSet(pszItemId);
//	
//	removeCstiContact(pContactItem);
//	
//	// Notify UI to remove item
//	CstiItemId *pID = new CstiItemId;
//	pContactItem->ItemIdGet(pID);
//	CallbackMessageSend(estiMSG_CONTACT_LIST_ITEM_DELETED, (size_t)pID);
//	
//	delete pContactItem;
//	pContactItem = NULL;
//}


//void CstiLDAPDirectoryContactManager::ImportContacts(const char *phoneNumber, const char *password, const char *macAddress)
//{
//	int nCoreMajorVersion = atoi (m_pCoreService->VersionGet().c_str());
//	if (3 < nCoreMajorVersion)
//	{
//		CstiCoreRequest *pRequest = new CstiCoreRequest ();
//		pRequest->contactsImport (phoneNumber, password);
//		CoreRequestSend (pRequest, estiFALSE);
//	}
//}

void CstiLDAPDirectoryContactManager::LDAPDirectoryNameSet(const std::string &directoryName)
{
	m_LDAPDirectoryName = directoryName;
}

/*!
 * \brief This is the main looping task.
 * 
 * \return int  
 */
int CstiLDAPDirectoryContactManager::Task ()
{
	stiLOG_ENTRY_NAME (CstiLDAPDirectoryContactManager::Task);

	EstiBool bContinue = estiTRUE;
	uint32_t un32Buffer[(stiLDAP_DIRECTORY_MANAGER_MAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	while (bContinue)
	{
		if (stiRESULT_SUCCESS == MsgRecv ((char *)un32Buffer, stiFP_MAX_MSG_SIZE))
		{
			auto  poEvent = (CstiEvent*)(void *)un32Buffer;
			Lock ();
			EventDoNow (poEvent);
			Unlock ();

			if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
			{
				bContinue = estiFALSE;
			}
		}
	}

	return (0);
} 

/*!
* \brief Start up the task
* \retval None
*/
stiHResult CstiLDAPDirectoryContactManager::Startup ()
{
	//
	// Start the task
	//

	CstiOsTaskMQ::Startup ();
	return stiRESULT_SUCCESS;
}

stiHResult CstiLDAPDirectoryContactManager::Shutdown ()
{
	return CstiOsTaskMQ::Shutdown ();
}

stiHResult CstiLDAPDirectoryContactManager::WaitForShutdown ()
{
	return CstiOsTaskMQ::WaitForShutdown ();
}

/*!
* \brief Shuts down the task.
* \param poEvent is a pointer to the event being processed.  Not used in this function.
* \retval none
*/
stiHResult CstiLDAPDirectoryContactManager::EventTaskShutdown(CstiEvent *poEvent)  // The event to be handled
{
	stiLOG_ENTRY_NAME (CstiLDAPDirectoryContactManager::EventShutdown);

	//
	// Call the base class shutdown.
	//
	return CstiOsTaskMQ::ShutdownHandle ();

} 

stiHResult CstiLDAPDirectoryContactManager::RequestContactsFromLDAPDirectory()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiEvent oEvent (CstiLDAPDirectoryContactManager::estiLDAP_RELOAD_CONTACTS);

	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return hResult;
}

stiHResult CstiLDAPDirectoryContactManager::EventReloadContacts (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiLDAPDirectoryContactManager::EventReloadContacts);

    ReloadContactsFromLDAPDirectory();

	return stiRESULT_SUCCESS;
}

stiHResult CstiLDAPDirectoryContactManager::LDAPCredentialsValidate(
	const std::string &loginName,
	const std::string &password)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char *pLoginName = nullptr;
	char *pPassword = nullptr;

	pLoginName = new char[loginName.length () + 1];
	strcpy(pLoginName, loginName.c_str ());

	pPassword = new char[password.length () + 1];
	strcpy(pPassword, password.c_str ());

	CstiExtendedEvent oEvent (CstiLDAPDirectoryContactManager::estiLDAP_CREDENTIALS_VALIDATE, 
							  (stiEVENTPARAM)pLoginName, 
							  (stiEVENTPARAM)pPassword);
//	CstiEvent oEvent (CstiLDAPDirectoryContactManager::estiLDAP_CREDENTIALS_VALIDATE);

	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return hResult;
}

stiHResult CstiLDAPDirectoryContactManager::EventCredentialsValidate (CstiEvent *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiLDAPDirectoryContactManager::EventCredentialsValidate);

	char *pLoginName = nullptr;
	char *pPassword = nullptr;
	std::string phoneUserId;
	LDAP *pLdap = nullptr;
	int nResult;
	int nAuthenticateMethod = LDAP_AUTH_UNKNOWN;
	int nScope = LDAP_SCOPE_DEFAULT;
	int desiredVersion = LDAP_VERSION3;
	unsigned long pageSize = 1;
	LDAPControl *pPageControl = nullptr;
	LDAPControl *pM_controls[2] = { nullptr, nullptr };
	LDAPMessage *pLdapResult = nullptr;
	struct timeval timeOut = {1,0};   // 1 second connection timeout
	char szPort[7];
	std::string ldapURI;
	if (m_nLDAPServerUseSSL == 0)
	{
		ldapURI = "ldap://";
	}
	else if (m_nLDAPServerUseSSL == 1)
	{
		ldapURI ="ldaps://";
	}

	ldapURI = ldapURI + m_LDAPhost;
	snprintf (szPort, sizeof(szPort), ":%d", m_nLDAPServerPort);
	ldapURI += szPort;
	
	PropertyManager *pPM = PropertyManager::getInstance ();
	pPM->propertyGet(PHONE_USER_ID, &phoneUserId);

    bool hasTelephoneNumberField = !m_LDAPtelephoneNumberField.empty ();
    bool hasHomeNumberField = !m_LDAPhomeNumberField.empty ();
    bool hasMobileNumberField = !m_LDAPmobileNumberField.empty ();
    
    char *attrs[] = {(char *)"cn",
        hasTelephoneNumberField ? (char *)m_LDAPtelephoneNumberField.c_str () : (hasHomeNumberField ? (char *)m_LDAPhomeNumberField.c_str () : (hasMobileNumberField ? (char *)m_LDAPmobileNumberField.c_str () : nullptr)),
        (hasHomeNumberField && hasTelephoneNumberField) ? (char *)m_LDAPhomeNumberField.c_str () : ((hasMobileNumberField && (hasHomeNumberField || hasTelephoneNumberField)) ? (char *)m_LDAPmobileNumberField.c_str () : nullptr),
        (hasMobileNumberField && hasHomeNumberField && hasTelephoneNumberField) ? (char *)m_LDAPmobileNumberField.c_str () : nullptr,
        nullptr};
    
    std::string filter = std::string("(|")
    + (!m_LDAPtelephoneNumberField.empty () ? std::string("(") + m_LDAPtelephoneNumberField + std::string("=*)") : std::string(""))
    + (!m_LDAPhomeNumberField.empty () ? std::string("(") + m_LDAPhomeNumberField + std::string("=*)") : std::string(""))
    + (!m_LDAPmobileNumberField.empty () ? std::string("(") + m_LDAPmobileNumberField + std::string("=*)") : std::string(""))
    + std::string(")");
	
	if (m_nLDAPServerUseSSL == 1)
	{
		/* set the LDAP_OPT_X_TLS_REQUIRE_CERT option */
		int reqcert = LDAP_OPT_X_TLS_ALLOW;
		
		if ((nResult = ldap_set_option(nullptr, LDAP_OPT_X_TLS_REQUIRE_CERT, &reqcert)) != LDAP_OPT_SUCCESS)
		{
			stiTrace("ldap_set_option 'LDAP_OPT_X_TLS_REQUIRE_CERT' %d: %s\n", reqcert, ldap_err2string(nResult));
			CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
			stiTHROW_NOLOG(stiRESULT_ERROR);
		}
	}

	// Initialize the ldap server.
	if (ldap_initialize(&pLdap, ldapURI.c_str()) != LDAP_SUCCESS)
	{
		CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	// Athenticate the user name and password
	switch (m_eLDAPServerRequiresAuthentication)
	{
		case eLDAPAuthenticateMethod_NONE:
		{
			nAuthenticateMethod = LDAP_AUTH_SIMPLE;
		}
			break;
		case eLDAPAuthenticateMethod_SIMPLE:
		{
			nAuthenticateMethod = LDAP_AUTH_SIMPLE;
			
			pLoginName = (char *)((CstiExtendedEvent *)poEvent)->ParamGet(0);
			pPassword = (char *)((CstiExtendedEvent *)poEvent)->ParamGet(1);
		}
			break;
		case eLDAPAuthenticateMethod_SASL:
			nAuthenticateMethod = LDAP_AUTH_SASL;
			break;
		default:
		{
			nAuthenticateMethod = LDAP_AUTH_SIMPLE;
			
			pLoginName = (char *)((CstiExtendedEvent *)poEvent)->ParamGet(0);
			pPassword = (char *)((CstiExtendedEvent *)poEvent)->ParamGet(1);
		}
			break;
	}

	// Set the LDAP version to 3
	if ((nResult = ldap_set_option(pLdap, LDAP_OPT_PROTOCOL_VERSION, &desiredVersion)) != LDAP_OPT_SUCCESS)
	{
		CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	if ((nResult = ldap_set_option(pLdap, LDAP_OPT_NETWORK_TIMEOUT, &timeOut)) != LDAP_OPT_SUCCESS)
	{
		CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	nResult = ldap_bind_s(pLdap, pLoginName, pPassword, nAuthenticateMethod);
	if (nResult != LDAP_SUCCESS)
	{
		switch (nResult) 
		{
		case LDAP_SERVER_DOWN:
			CallbackMessageSend(estiMSG_LDAP_DIRECTORY_SERVER_UNAVAILABLE, (size_t)0);
			break;
		case LDAP_INVALID_CREDENTIALS:
			CallbackMessageSend(estiMSG_LDAP_DIRECTORY_INVALID_CREDENTIALS, (size_t)0);
			break;
		default:
			CallbackMessageSend(estiMSG_LDAP_DIRECTORY_ERROR, (size_t)0);
			break;
		}
		
		stiRemoteLogEventSend ("EventType=LDAP Reason=\"AuthenticationFailed\" CoreId=%s", phoneUserId.c_str ());
		
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	nResult = ldap_create_page_control(pLdap, 
									   pageSize, 
									   nullptr, 'T', 
									   &pPageControl);

	//Insert the control into a list to be passed to the search. 
	pM_controls[0] = pPageControl;

	switch (m_nLDAPScope) {
		case 0:
			nScope = LDAP_SCOPE_BASE;
			break;
		case 1:
			nScope = LDAP_SCOPE_ONELEVEL;
			break;
		case 2:
			nScope = LDAP_SCOPE_SUBTREE;
			break;
		default:
			nScope = LDAP_SCOPE_SUBTREE;
			break;
	}

	// Test for an entrie in the directory using the parmeters.
	nResult = ldap_search_ext_s(pLdap, 
								m_LDAPbase.c_str (),
								nScope, 
								filter.c_str(), 
								attrs, 0, 
								pM_controls, 
								nullptr, nullptr, 
								0, 
								&pLdapResult);
	if (nResult != LDAP_SUCCESS && 
		nResult != LDAP_PARTIAL_RESULTS)
	{
  		CallbackMessageSend(estiMSG_LDAP_DIRECTORY_INVALID_CREDENTIALS, (size_t)0);
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	CallbackMessageSend(estiMSG_LDAP_DIRECTORY_VALID_CREDENTIALS, (size_t)0);

STI_BAIL:

	// Cleanup
	if (pPageControl)
	{
		ldap_control_free(pPageControl);
		pPageControl = nullptr;
	}

	if (pLdapResult)
	{
		ldap_msgfree(pLdapResult);
		pLdapResult = nullptr;
	}

	if (pLdap)
	{
		ldap_unbind_s(pLdap);
		pLdap = nullptr;
	}

	if (pLoginName)
	{
		delete []pLoginName;
		pLoginName = nullptr;
	}

	if (pPassword)
	{
		delete []pPassword;
		pPassword = nullptr;
	}
	
	return hResult;
}

static void telephoneNumberFieldSet (
	std::string &telephoneNumberFieldDst,
	const std::string &ldapTelephoneNumberFieldSrc,
	const std::string &defaultValue)
{
	if (!ldapTelephoneNumberFieldSrc.empty ()
	 && ldapTelephoneNumberFieldSrc != "0")
	{
		telephoneNumberFieldDst = ldapTelephoneNumberFieldSrc;
	}
	else
	{
		// Value was zero length or invalid (0) so use default
		telephoneNumberFieldDst = defaultValue;
	}
}

/*!
 * \brief Sets the TelephoneNumberField of the LDAP Directory ContactManager
 *
 * \param pszLDAPTelephoneNumberField The LDAP TelephoneNumberField to use
 */
void CstiLDAPDirectoryContactManager::LDAPTelephoneNumberFieldSet(const std::string &LDAPTelephoneNumberField)
{
	telephoneNumberFieldSet (m_LDAPtelephoneNumberField, LDAPTelephoneNumberField, LDAP_TELEPHONE_NUMBER_FIELD_DEFAULT);
}

/*!
 * \brief Sets the HomeNumberField of the LDAP Directory ContactManager
 *
 * \param pszLDAPHomeNumberField The LDAP HomeNumberField to use
 */
void CstiLDAPDirectoryContactManager::LDAPHomeNumberFieldSet(const std::string &LDAPHomeNumberField)
{
	telephoneNumberFieldSet (m_LDAPhomeNumberField, LDAPHomeNumberField, LDAP_HOME_NUMBER_FIELD_DEFAULT);
}

/*!
 * \brief Sets the MobileNumberField of the LDAP Directory ContactManager
 *
 * \param pszLDAPMobileNumberField The LDAP MobileNumberField to use
 */
void CstiLDAPDirectoryContactManager::LDAPMobileNumberFieldSet(const std::string &LDAPMobileNumberField)
{
	telephoneNumberFieldSet (m_LDAPmobileNumberField, LDAPMobileNumberField, LDAP_MOBILE_NUMBER_FIELD_DEFAULT);
}


#endif // stiLDAP_CONTACT_LIST
