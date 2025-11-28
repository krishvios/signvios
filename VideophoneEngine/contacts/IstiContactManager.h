/*!
*  \file IstiContactManager.h
*  \brief The Contact List Manager Interface Class
*
*  Class declaration for the Contact Manager.  Provides storage
*  and access for persistent (both local and remote) and temporary data.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef ISTICONTACTMANAGER_H
#define ISTICONTACTMANAGER_H


#include "IstiContact.h"
#include "CstiFavorite.h"
#include "ISignal.h"
#include <vector>


/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Contact Manager Interface Class
 *
 */
class IstiContactManager
{
public:

	IstiContactManager () = default;
	IstiContactManager (const IstiContactManager &other) = delete;
	IstiContactManager (IstiContactManager &&other) = delete;
	IstiContactManager &operator= (const IstiContactManager &other) = delete;
	IstiContactManager &operator= (IstiContactManager &&other) = delete;

	/*!
	 * \brief Virtual destructor
	 */
	virtual ~IstiContactManager () = default;

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
		int nIndex) = 0;
	
	/*!
	 * \brief Retrieves the number of contacts in the list
	 *
	 * \return The number of contacts in the list
	 */
	virtual int getNumContacts() = 0;


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
		IstiContactSharedPtr *contact) = 0;

	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param id The ItemId to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	virtual std::shared_ptr<IstiContact> contactByIDGet(
		const CstiItemId &id) = 0;

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
		IstiContactSharedPtr *pContact) = 0;
	
	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param phoneNumber The phone number to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	virtual std::shared_ptr<IstiContact> contactByPhoneNumberGet(
		const std::string &phoneNumber) = 0;

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
		const std::string &id) = 0;

	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \param Id The ItemId to search for
	 *
	 * \return The index in the list
	 */
	virtual int ContactIndexGet(
		const CstiItemId &Id) = 0;

	/*!
	 * \brief Adds a new contact to the list
	 *
	 * \param pItem The contact being added
	 */
	virtual void addContact(
		const IstiContactSharedPtr &item) = 0;
	
	/*!
	 * \brief Edits an existing contact by looking up its ItemId
	 *
	 * \param pItem The contact that is being updated
	 */
	virtual void updateContact(
		const IstiContactSharedPtr &item) = 0;
	
	/*!
	 * \brief Removes (and deletes) a contact from the list
	 *
	 * \param pItem The contact being removed
	 */
	virtual void removeContact(
		const IstiContactSharedPtr &item) = 0;
	
	/*!
	 * \brief Removes (and deletes) ALL contacts
	 */
	virtual void clearContacts() = 0;

	/*!
	 * \brief Removes ALL contacts
	 */
	virtual void purgeContacts() = 0;

	/*!
	 * \brief Determines if the contact list has reached its capacity
	 *
	 * \return True if full, False otherwise
	 */
	virtual bool isFull() = 0;

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
	virtual size_t getMaxCount() = 0;

	/*!
	 * \brief Enables or disables the ContactManager
	 *
	 * \param bEnabled True to enable, False to disable
	 */
	virtual void EnabledSet(
		bool bEnabled)  = 0;

	/*!
	 * \brief Retrieves the entire contact list from Core
	 */
	virtual void ReloadContactsFromCore() = 0;

#ifdef stiFAVORITES
	/*!
	* \brief Retrieves the entire favorite list from Core
	*/
	virtual void ReloadFavoritesFromCore () = 0;
#endif

	/*!
	 * \brief Imports the user's contact list from Core
	 */
	virtual void ImportContacts(const std::string &phoneNumber, const std::string &ppassword) = 0;

#ifdef stiIMAGE_MANAGER

	/*!
	 * \brief Updates all contacts with the latest photos from core.
	 */
	virtual void UpdateAllPhotos() = 0;

	/*!
	 * \brief Removes contact photos from all contacts.
	 */
	virtual void PurgeAllContactPhotos() = 0;

	/*!
	 * \brief Converts any contacts currently using the supplied image GUID
	 * which core has reported as not found to an appropriate photo that exists.
	 *
	 * \param guid GUID of the photo that core can not find
	 */
	virtual void FixContactsForPhotoNotFound(const std::string& guid) = 0;

#endif

	/*!
	 * \brief Enables the Contact Photo feature
	 * \param bEnabled Whether or not to enable photos
	 */
	virtual void ContactPhotosEnable(bool bEnabled) = 0;

#ifdef stiFAVORITES

	virtual void FavoritesMaxSet (
		int favoriteMax) = 0;

	virtual stiHResult FavoriteAdd (
		const CstiItemId &PhoneNumberId,
		bool bIsContact = true) = 0;

	virtual stiHResult FavoriteRemove (
		int nIndex) = 0;

	virtual stiHResult FavoriteRemove (
		const CstiItemId &PhoneNumberId) = 0;

	virtual stiHResult FavoriteMove (
		int nOldIndex,
		int nNewIndex) = 0;

	virtual stiHResult FavoriteListSet (
		const std::vector<CstiFavorite> &newList) = 0;

	virtual bool IsFavorite (
		const CstiItemId &PhoneNumberId) const = 0;

	virtual bool IsFavorite (
		const std::string &dialString) const = 0;

	virtual int FavoritesCountGet() const = 0;

	virtual int FavoritesMaxCountGet() const = 0;

	virtual bool FavoritesFull() const = 0;

	virtual CstiFavoriteSharedPtr FavoriteByIndexGet(
		int nIndex) const = 0;

	virtual CstiFavoriteSharedPtr FavoriteByPhoneNumberIdGet(
		const CstiItemId &PhoneNumberId) const = 0;

	virtual IstiContactSharedPtr ContactByFavoriteGet(
		const CstiFavoriteSharedPtr &favorite) const = 0;

#endif

	virtual bool CompanyNameEnabled() const = 0;
	
	virtual void Refresh () = 0;

	/*!
	 * \brief Locks the ContactManager
	 */
	virtual void Lock() const = 0;

	/*!
	 * \brief Unlocks the ContactManager
	 */
	virtual void Unlock() const = 0;

	enum Response
	{
		Unknown,		//! Undefined response
		AddError,		//! Adding a new contact failed
		EditError,		//! Editing a contact failed
		DeleteError,	//! Deleting a contact failed
		GetListError,	//! Getting the contact list failed
		SetListError,	//! Clearing the contact list failed
		ImportError,	//! Importing the contact list failed
	};

	virtual ISignal<Response, stiHResult>& contactErrorSignalGet() = 0;

	virtual ISignal<Response, stiHResult>& favoriteErrorSignalGet() = 0;
	
}; // end ContactManager class definition

#endif // ISTICONTACTMANAGER_H
