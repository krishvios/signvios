/*!
 *  \file IstiLDAPDirectoryContactManager.h
 *  \brief The LDAP Directory Contact List Manager Interface Class
 *
 *  Class declaration for the LDAP Directory Contact Manager.  Provides storage
 *  and access for persistent (both local and remote) and temporary data.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef ISTILDAPDIRECTORYCONTACTMANAGER_H
#define ISTILDAPDIRECTORYCONTACTMANAGER_H


#include "IstiContact.h"
#include "CstiFavorite.h"

#define LDAP_DIRECTORY_DISPLAY_NAME_DEFAULT "Directory"
#define LDAP_TELEPHONE_NUMBER_FIELD_DEFAULT "telephoneNumber"
#define LDAP_HOME_NUMBER_FIELD_DEFAULT "homePhone"
#define LDAP_MOBILE_NUMBER_FIELD_DEFAULT "mobile"

/*!
 * \ingroup VideophoneEngineLayer
 * \brief LDAP Directory Contact List Manager Interface Class
 *
 */
class IstiLDAPDirectoryContactManager
{
public:
	
	enum ELDAPAuthenticateMethod
	{
		eLDAPAuthenticateMethod_NONE = 0,
		eLDAPAuthenticateMethod_SIMPLE = 1,
		eLDAPAuthenticateMethod_SASL = 2
	};
	
	/*!
	 * \brief Virtual destructor
	 */
	virtual ~IstiLDAPDirectoryContactManager () = default;
	
	/*!
	 * \brief Creates a new instance of IstiContact
	 *
	 * \return Pointer to new contact
	 */
	virtual IstiContactSharedPtr ContactCreate () = 0;
	
	/*!
	 * \brief Retrieves a contact from its index in the list
	 *
	 * \param nIndex The index in the list
	 *
	 * \return Pointer to the contact if found, NULL otherwise
	 */
	virtual IstiContactSharedPtr getContact(
		int nIndex) const = 0;
	
	/*!
	 * \brief Retrieves the number of contacts in the list
	 *
	 * \return The number of contacts in the list
	 */
	virtual int getNumContacts() const = 0;
	
	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param Id The ItemId to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	virtual EstiBool ContactByIDGet(
		const CstiItemId &Id,
		IstiContactSharedPtr *contact) const = 0;
	
	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param id The ItemId to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	virtual std::shared_ptr<IstiContact> contactByIDGet(
		const CstiItemId &id) const = 0;

	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param pPhoneNumber The phone number to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	virtual EstiBool getContactByPhoneNumber(
		const std::string &phoneNumber,
		IstiContactSharedPtr *contact) const = 0;
	
	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param phoneNumber The phone number to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	virtual std::shared_ptr<IstiContact> contactByPhoneNumberGet(
		const std::string &phoneNumber) const = 0;

	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \deprecated
	 *
	 * \param pID The ItemId to search for
	 *
	 * \return The index in the list
	 */
	virtual int getContactIndex(
		const std::string &id) const = 0;
	
	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \param Id The ItemId to search for
	 *
	 * \return The index in the list
	 */
	virtual int ContactIndexGet(
		const CstiItemId &Id) const = 0;
		
	/*!
	 * \brief Removes (and deletes) ALL contacts
	 */
	virtual void clearContacts() = 0;
	
	/*!
	 * \brief Determines if the contact list has reached its capacity
	 *
	 * \return True if full, False otherwise
	 */
	virtual bool isFull() const = 0;
	
	/*!
	 * \brief Sets the maximum number of contacts for the list
	 *
	 * \param Count The maximum number of contacts
	 */
	virtual void setMaxCount(
							 size_t Count) = 0;
	
	/*!
	 * \brief Retrieves the maximum number of contacts for the list
	 *
	 * \return The maximum number of contacts
	 */
	virtual size_t getMaxCount() const = 0;
	
	/*!
	 * \brief Returns LDAP Directory ContactManager enabled status.
	 *
	 * \return The enabled status.
	 */
	virtual bool EnabledGet() const = 0;

	/*!
	 * \brief Enables or disables the ContactManager
	 *
	 * \param bEnabled True to enable, False to disable
	 */
	virtual void EnabledSet(
		bool bEnabled)  = 0;
	
	/*!
	 * \brief Returns the LDAP Directory name.
	 *
	 * \return A pointer to the direcory name.
	 */
	virtual std::string LDAPDirectoryNameGet() const = 0;

	/*!
	 * \brief Sets the host of the LDAP Directory ContactManager
	 *
	 * \param LDAPHost The LDAP host to connect to
	 */
	virtual void LDAPHostSet(const std::string &LDAPHost) = 0;
	
	/*!
	 * \brief Sets the Distinguished Name of the LDAP Directory ContactManager
	 *
	 * \param LDAPDistinguishedName The LDAP Distinguished Name to use
	 */
	virtual void LDAPDistinguishedNameSet(const std::string &LDAPDistinguishedName) = 0;
	
	/*!
	 * \brief Sets the password of the LDAP Directory ContactManager
	 *
	 * \param LDAPPassword The LDAP password to use
	 */
	virtual void LDAPPasswordSet(const std::string &LDAPPassword)  = 0;
	
	/*!
	 * \brief Sets the Base Entry of the LDAP Directory ContactManager
	 *
	 * \param LDAPBaseEntry The LDAP Base Entry to use
	 */
	virtual void LDAPBaseEntrySet(const std::string &LDAPBaseEntry) = 0;
	
	/*!
	 * \brief Sets the search filter of the LDAP Directory ContactManager
	 *
	 * \param LDAPSearchFilter The LDAP search filter to use
	 */
	virtual void LDAPSearchFilterSet(const std::string &LDAPSearchFilter) = 0;
	
	/*!
	 * \brief Sets the TelephoneNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPTelephoneNumberField The LDAP TelephoneNumberField to use
	 */
	virtual void LDAPTelephoneNumberFieldSet(const std::string &LDAPTelephoneNumberField) = 0;

	/*!
	 * \brief Sets the HomeNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPHomeNumberField The LDAP HomeNumberField to use
	 */
	virtual void LDAPHomeNumberFieldSet(const std::string &LDAPHomeNumberField) = 0;
	
	/*!
	 * \brief Sets the MobileNumberField of the LDAP Directory ContactManager
	 *
	 * \param pszLDAPMobileNumberField The LDAP MobileNumberField to use
	 */
	virtual void LDAPMobileNumberFieldSet(const std::string &LDAPMobileNumberField) = 0;
	
	/*!
	 * \brief Sets the LDAPServerPort of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerPort The LDAP LDAPServerPort to use
	 */
	virtual void LDAPServerPortSet(int nLDAPServerPort) = 0;

	/*!
	 * \brief Sets the LDAPServerUseSSL of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerUseSSL The LDAP LDAPServerUseSSL to use
	 */
	virtual void LDAPServerUseSSLSet(int nLDAPServerUseSSL) = 0;
	
	/*!
	 * \brief Sets the LDAPScope of the LDAP Directory ContactManager
	 *
	 * \param nLDAPScope The LDAP LDAPScope to use
	 */
	virtual void LDAPScopeSet(int nLDAPScope) = 0;
	
	/*!
	 * \brief Sets the LDAPServerRequiresAuthentication of the LDAP Directory ContactManager
	 *
	 * \param nLDAPServerRequiresAuthentication The LDAP Authentication to use
	 */
	virtual void LDAPServerRequiresAuthenticationSet(ELDAPAuthenticateMethod nLDAPServerRequiresAuthentication) = 0;
	
	/*!
	 * \brief Gets the LDAPServerRequiresAuthentication level from the LDAP Directory ContactManager
	 *
	 * \return The LDAP Authentication in use
	 */
	virtual ELDAPAuthenticateMethod LDAPServerRequiresAuthenticationGet() const = 0;

	/*!
	 * \brief Retrieves the entire contact list from the LDAP Directory
	 */
	virtual void ReloadContactsFromLDAPDirectory() = 0;

	/*!
     * \brief Requests the entire contact list from the LDAP Directory to be downloaded. 
     *  
     * \return stiRESULT_SUCCESS if the request was succefully put on the queue.
	 */
	virtual stiHResult RequestContactsFromLDAPDirectory() = 0;

	/*!
     * \brief Requests that the login credentials be validated. 
     *  
     * \param pszLoginName the user name to verify. 
     * \param pszPassword the password to verify. 
     *  
     * \return stiRESULT_SUCCESS if the request was succefully put on the queue.
	 */
	virtual stiHResult LDAPCredentialsValidate(
		const std::string &loginName,
		const std::string &password) = 0;

#ifdef stiFAVORITES
	virtual IstiContactSharedPtr ContactByFavoriteGet(
		const CstiFavoriteSharedPtr &favorite) const = 0;
	
#endif
	
	/*!
	 * \brief Locks the ContactManager
	 */
	virtual stiHResult Lock() const = 0;
	
	/*!
	 * \brief Unlocks the ContactManager
	 */
	virtual void Unlock() const = 0;
	
}; // end LDAPDirectoryContactManager class definition

#endif // ISTILDAPDIRECTORYCONTACTMANAGER_H
