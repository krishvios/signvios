//
//  CstiLDAPDirectoryContactManager.h
//
//  Class declaration for the LDAP Directory Contact List Manager.  Provides storage
//  and access for persitent (both local and remote) and temporary data.
//
//  Created by J.R. Blackham on 2/4/15.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef CSTILDAPDIRECTORYCONTACTMANAGER_H
#define CSTILDAPDIRECTORYCONTACTMANAGER_H

#include "ixml.h"
#include "XMLManager.h"
#include "CstiCoreResponse.h"
#include "CstiCoreRequest.h"
#include "CstiStateNotifyResponse.h"
#include "ContactListItem.h"
#include "IstiLDAPDirectoryContactManager.h"
#include "CstiOsTaskMQ.h"
#include "stiEventMap.h"
#include "CstiEvent.h"
#include "CstiExtendedEvent.h"


#include <map>
#include <set>

class CstiCoreService;
class CstiContactListItem;

/*!
 *  \brief LDAP Directory Contact Manager Class
 *
 *  Outlines the public (and private) APIs and member variables
 */

class CstiLDAPDirectoryContactManager : public WillowPM::XMLManager, public IstiLDAPDirectoryContactManager, public CstiOsTaskMQ
{
public:
	
	/*!
	 * \brief CstiLDAPDirectoryContactManager constructor
	 *
	 * \param pCoreService Pointer to Core Service module
	 * \param pfnVPCallback Callback function pointer
	 * \param CallbackParam Parameter passed to callback function
	 */
	CstiLDAPDirectoryContactManager(bool bEnabled,CstiCoreService *pCoreService, PstiObjectCallback pfnVPCallback, size_t CallbackParam);
	
	CstiLDAPDirectoryContactManager (const CstiLDAPDirectoryContactManager &other) = delete;
	CstiLDAPDirectoryContactManager (CstiLDAPDirectoryContactManager &&other) = delete;
	CstiLDAPDirectoryContactManager &operator= (const CstiLDAPDirectoryContactManager &other) = delete;
	CstiLDAPDirectoryContactManager &operator= (CstiLDAPDirectoryContactManager &&other) = delete;

	/*!
	 * \brief CstiLDAPDirectoryContactManager destructor
	 */
	~CstiLDAPDirectoryContactManager() override;
	
	/*!
	 * \brief Initializes CstiLDAPDirectoryContactManager
	 *
	 * \return stiHResult
	 */
	virtual stiHResult Initialize();
	
	/*!
	 * \brief Creates a new instance of IstiContact
	 *
	 * \return Pointer to new contact
	 */
	IstiContactSharedPtr ContactCreate () override;
	
	/*!
	 * \brief Retrieves a contact from its index in the list
	 *
	 * \param nIndex The index in the list
	 *
	 * \return Pointer to the contact if found, NULL otherwise
	 */
	IstiContactSharedPtr getContact(int nIndex) const override;
	
	/*!
	 * \brief Retrieves the number of contacts in the list
	 *
	 * \return The number of contacts in the list
	 */
	int getNumContacts() const override;
	
	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param Id The ItemId to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	EstiBool ContactByIDGet(const CstiItemId &Id, IstiContactSharedPtr *contact) const override;
	
	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param id The ItemId to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	IstiContactSharedPtr contactByIDGet(const CstiItemId &Id) const override;

	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param pPhoneNumber The phone number to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	EstiBool getContactByPhoneNumber(const std::string &phoneNumber, IstiContactSharedPtr *contact) const override;
	
	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param phoneNumber The phone number to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	IstiContactSharedPtr contactByPhoneNumberGet(const std::string &phoneNumber) const override;

	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \deprecated
	 *
	 * \param pID The ItemId to search for
	 *
	 * \return The index in the list
	 */
	int getContactIndex(const std::string &id) const override;
	
	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \param Id The ItemId to search for
	 *
	 * \return The index in the list
	 */
	int ContactIndexGet(const CstiItemId &Id) const override;
		
	/*!
	 * \brief Removes (and deletes) ALL contacts
	 */
	void clearContacts() override;
	
	/*!
	 * \brief Determines if the contact list has reached its capacity
	 *
	 * \return True if full, False otherwise
	 */
	bool isFull() const override;
	
	/*!
	 * \brief Sets the maximum number of contacts for the list
	 *
	 * \param Count The maximum number of contacts
	 */
	void setMaxCount(size_t Count) override;
	
	/*!
	 * \brief Retrieves the maximum number of contacts for the list
	 *
	 * \return The maximum number of contacts
	 */
	size_t getMaxCount() const override;
	
	/*!
	 * \brief Enables or disables the LDAP Directory ContactManager
	 *
	 * \param bEnabled True to enable, False to disable
	 */
	void EnabledSet(bool bEnabled) override
	{
		m_bEnabled = bEnabled;
	}
	
	/*!
	 * \brief Returns LDAP Directory ContactManager enabled status.
	 *
	 * \return The enabled status.
	 */
	bool EnabledGet() const override
	{ 
		return m_bEnabled;
	}
		
	/*!
	 * \brief Stores the LDAP Directory name 
	 *
	 * \param pointer to the directory name
	 */
	void LDAPDirectoryNameSet(const std::string &directoryName);
	
	/*!
	 * \brief Returns the LDAP Directory name.
	 *
	 * \return A pointer to the direcory name.
	 */
	std::string LDAPDirectoryNameGet() const override
	{ 
		return m_LDAPDirectoryName;
	}

	/*!
	 * \brief Sets the host of the LDAP Directory ContactManager
	 *
	 * \param LDAPHost The LDAP host to connect to
	 */
	void LDAPHostSet(const std::string &LDAPHost) override
	{
		m_LDAPhost = LDAPHost;
	}
	
	/*!
	 * \brief Sets the Distinguished Name of the LDAP Directory ContactManager
	 *
	 * \param LDAPDistinguishedName The LDAP Distinguished Name to use
	 */
	void LDAPDistinguishedNameSet(const std::string &LDAPDistinguishedName) override
	{
		m_LDAPdn = LDAPDistinguishedName;
	}
	
	/*!
	 * \brief Sets the password of the LDAP Directory ContactManager
	 *
	 * \param LDAPPassword The LDAP password to use
	 */
	void LDAPPasswordSet(const std::string &LDAPPassword) override
	{
		m_LDAPpw = LDAPPassword;
	}

	
	/*!
	 * \brief Sets the Base Entry of the LDAP Directory ContactManager
	 *
	 * \param LDAPBaseEntry The LDAP Base Entry to use
	 */
	void LDAPBaseEntrySet(const std::string &LDAPBaseEntry) override
	{
		m_LDAPbase = LDAPBaseEntry;
	}

	
	/*!
	 * \brief Sets the search filter of the LDAP Directory ContactManager
	 *
	 * \param LDAPSearchFilter The LDAP search filter to use
	 */
	void LDAPSearchFilterSet(const std::string &LDAPSearchFilter) override
	{
		m_LDAPfilter = LDAPSearchFilter;
	}

	/*!
	 * \brief Sets the TelephoneNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPTelephoneNumberField The LDAP TelephoneNumberField to use
	 */
	void LDAPTelephoneNumberFieldSet(const std::string &LDAPTelephoneNumberField) override;
	
	/*!
	 * \brief Sets the HomeNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPHomeNumberField The LDAP HomeNumberField to use
	 */
	void LDAPHomeNumberFieldSet(const std::string &LDAPHomeNumberField) override;
	
	/*!
	 * \brief Sets the MobileNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPMobileNumberField The LDAP MobileNumberField to use
	 */
	void LDAPMobileNumberFieldSet(const std::string &LDAPMobileNumberField) override;

	/*!
	 * \brief Sets the LDAPServerPort of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerPort The LDAP LDAPServerPort to use
	 */
	void LDAPServerPortSet(int nLDAPServerPort) override
	{
		m_nLDAPServerPort = nLDAPServerPort;
	}
	
	/*!
	 * \brief Sets the LDAPServerUseSSL of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerUseSSL The LDAP LDAPServerUseSSL to use
	 */
	void LDAPServerUseSSLSet(int nLDAPServerUseSSL) override
	{
		m_nLDAPServerUseSSL = nLDAPServerUseSSL;
	}
	
	/*!
	 * \brief Sets the LDAPScope of the LDAP Directory ContactManager
	 *
	 * \param nLDAPScope The LDAP LDAPScope to use
	 */
	void LDAPScopeSet(int nLDAPScope) override
	{
		m_nLDAPScope = nLDAPScope;
	}
	
	/*!
	 * \brief Sets the LDAPServerRequiresAuthentication of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerRequiresAuthentication The LDAP Authentication to use
	 */
	void LDAPServerRequiresAuthenticationSet(IstiLDAPDirectoryContactManager::ELDAPAuthenticateMethod eLDAPServerRequiresAuthentication) override
	{
		m_eLDAPServerRequiresAuthentication = eLDAPServerRequiresAuthentication;
	}

	/*!
	 * \brief Gets the LDAPServerRequiresAuthentication level from the LDAP Directory ContactManager
	 *
	 * return The LDAP Authentication in use
	 */
	IstiLDAPDirectoryContactManager::ELDAPAuthenticateMethod LDAPServerRequiresAuthenticationGet() const override
	{
		return m_eLDAPServerRequiresAuthentication;
	}

	/*!
	 * \brief Responds to relevant CoreResponses
	 *
	 * \param poResponse The CoreResponse object
	 *
	 * \return True if the response was handled, False otherwise
	 */
	EstiBool CoreResponseHandle(CstiCoreResponse *poResponse);
	
//	/*!
//	 * \brief Responds to relevant StateNotifyResponses
//	 *
//	 * \param poResponse The StateNotifyResponses object
//	 *
//	 * \return True if the response was handled, False otherwise
//	 */
//	EstiBool StateNotifyEventHandle(CstiStateNotifyResponse *poResponse);
//	
//	/*!
//	 * \brief Stops waiting for a response if there was a communication error
//	 *
//	 * \param poRequest The request that never got to be sent
//	 *
//	 * \return True if the response was handled, False otherwise
//	 */
//	EstiBool RemovedUponCommunicationErrorHandle(SstiRemovedRequest *poRequest);
//	
	/*!
	 * \brief Converts a piece of an XML document into a ContactListItem
	 *
	 * \param pNode The piece of XML
	 *
	 * \return A pointer to a ContactListItem
	 */
	WillowPM::XMLListItemSharedPtr NodeToItemConvert(IXML_Node *pNode) override;
	
	/*!
	 * \brief Retrieves the entire contact list from the LDAP Directory
	 */
	void ReloadContactsFromLDAPDirectory() override;
	
	/*!
     * \brief Requests the entire contact list from the LDAP Directory to be downloaded. 
     *  
     * \return stiRESULT_SUCCESS if the request was succefully put on the queue.
	 */
	stiHResult RequestContactsFromLDAPDirectory() override;
	
	/*!
     * \brief Requests that the login credentials be validated. 
     *  
     * \param pszLoginName the user name to verify. 
     * \param pszPassword the password to verify. 
     *  
     * \return stiRESULT_SUCCESS if the request was succefully put on the queue.
	 */
	stiHResult LDAPCredentialsValidate(
		const std::string &loginName,
		const std::string &password) override;
	
#ifdef stiFAVORITES
	
//	void FavoritesMaxSet (int favoriteMax);
//	
//	stiHResult FavoriteAdd (
//							const CstiItemId &PhoneNumberId,
//							bool bIsContact = true);
//	
//	stiHResult FavoriteAddWithPosition(
//									   const CstiItemId &PhoneNumberId,
//									   bool bIsContact = true,
//									   int position = 0);
//	
//	stiHResult FavoriteRemove (
//							   int nIndex);
//	
//	stiHResult FavoriteRemove (
//							   const CstiItemId &PhoneNumberId);
//	
//	stiHResult FavoriteMove (
//							 int nOldIndex,
//							 int nNewIndex);
//	
//	stiHResult FavoriteListSet (
//								CstiFavoriteList *pList);
//	
//	bool IsFavorite (
//					 const CstiItemId &PhoneNumberId) const;
//	
//	bool IsFavorite (
//					 const std::string &dialString) const;
//	
//	int FavoritesCountGet() const;
//	
//	int FavoritesMaxCountGet() const;
//	
//	bool FavoritesFull() const;
//	
//	CstiFavorite *FavoriteByIndexGet(
//									 int nIndex) const;
//	
//	int FavoriteIndexByPhoneNumberIdGet(
//										const CstiItemId &PhoneNumberId) const;
//	
//	virtual CstiFavorite *FavoriteByPhoneNumberIdGet(
//													 const CstiItemId &PhoneNumberId) const;
	
	IstiContactSharedPtr ContactByFavoriteGet(const CstiFavoriteSharedPtr &favorite) const override;
	
#endif

	
	/*!
	 * \brief Locks the LDAPDirectoryContactManager
	 */
	stiHResult Lock() const override 
    { 
        return CstiOsTask::Lock(); 
    }
	
	/*!
	 * \brief Unlocks the LDAPDirectoryContactManager
	 */
	void Unlock() const override 
    { 
        CstiOsTask::Unlock(); 
    }
	
private:
	
//	void addCstiContact(CstiContactListItem *pCstiItem);
//	void updateCstiContact(CstiContactListItem *pCstiItem);
//	void removeCstiContact(CstiContactListItem *pCstiItem);
//	EstiBool ListUpdate(CstiCoreResponse *poResponse);
//	
//	unsigned int CoreRequestSend(CstiCoreRequest *poCoreRequest, EstiBool bRemoveUponError = estiTRUE, unsigned int unExpectedResult = CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT);
	void CallbackMessageSend(EstiCmMessage eMessage, size_t Param);
//	void ImportContacts(const std::string &phoneNumber, const std::string &password, const std::string &macAddress);
//	EstiBool RemoveRequestId(unsigned int unRequestId, unsigned int *punExpectedResult = NULL);
//	void ContactListItemChangedHandle(CstiStateNotifyResponse *pStateNotifyResponse);
//	void ContactListItemRemovedHandle(CstiStateNotifyResponse *pStateNotifyResponse);
	
//	void DownloadURLGetResultHandle(CstiCoreResponse *poResponse);
	
	//
	// Supporting Functions
	//
	int Task () override;
	stiHResult Shutdown () override;
	stiHResult WaitForShutdown () override;
	stiHResult Startup() override;
	
	stiDECLARE_EVENT_MAP(CstiLDAPDirectoryContactManager);
	stiDECLARE_EVENT_DO_NOW(CstiLDAPDirectoryContactManager);

	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	
	bool m_bEnabled;
//
//	struct PendingRequest
//	{
//		unsigned int m_unCookie;
//		unsigned int m_unExpectedResult;
//	};
//	
//	std::list<PendingRequest> m_PendingCookies;
//	
//	unsigned int m_unLastCookie;
//	
//	typedef std::set<std::string> ItemIdSet;
	
	std::string m_LDAPhost;
	
	std::string m_LDAPdn;
	std::string m_LDAPpw;
	
	std::string m_LDAPbase;
	
	std::string m_LDAPtelephoneNumberField;
	std::string m_LDAPhomeNumberField;
	std::string m_LDAPmobileNumberField;

	std::string m_LDAPfilter;

	std::string m_LDAPDirectoryName;
	
	int m_nLDAPServerPort{0};
	int m_nLDAPServerUseSSL{0};
	int m_nLDAPScope{0};
	IstiLDAPDirectoryContactManager::ELDAPAuthenticateMethod m_eLDAPServerRequiresAuthentication{eLDAPAuthenticateMethod_NONE};

	enum EEvent
	{
		estiLDAP_RELOAD_CONTACTS = estiEVENT_NEXT,
		estiLDAP_CREDENTIALS_VALIDATE
	};

	//
	// Event Handlers
	//
	stiHResult EventTaskShutdown (CstiEvent *poEvent);
	stiHResult EventReloadContacts (CstiEvent *poEvent);
	stiHResult EventCredentialsValidate (CstiEvent *poEvent);
	
}; // end ContactManager class definition


#endif //CSTILDAPDIRECTORYCONTACTMANAGER_H
