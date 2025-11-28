/*!
*  \file ContactManager.h
*  \brief The Contact List Manager
*
*  Class declaration for the Contact List Manager.  Provides storage
*  and access for persitent (both local and remote) and temporary data.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef CONTACTMANAGER_H
#define CONTACTMANAGER_H


#include "ixml.h"
#include "XMLManager.h"
#include "CstiCoreResponse.h"
#include "ServiceResponse.h"
#include "CstiCoreRequest.h"
#include "CstiStateNotifyResponse.h"
#include "IstiContactManager.h"
#include "ContactListItem.h"
#include "FavoriteList.h"
#include "CstiSignal.h"

#include <map>
#include <set>

class CstiCoreService;
class CstiContactListItem;
class IstiImageManager;
struct ClientAuthenticateResult;
class CstiEventQueue;

/*!
*  \brief Contact Manager Class
*
*  Outlines the public (and private) APIs and member variables
*/

class CstiContactManager : public WillowPM::XMLManager, public IstiContactManager
{
public:

	CstiContactManager () = delete;

	/*!
	 * \brief CstiContactManager constructor
	 *
	 * \param pCoreService Pointer to Core Service module
	 * \param pfnVPCallback Callback function pointer
	 * \param CallbackParam Parameter passed to callback function
	 * \param pImageManager point to the image manager
	 */
	CstiContactManager(CstiCoreService *pCoreService, PstiObjectCallback pfnVPCallback, size_t CallbackParam, IstiImageManager *pImageManager);

	CstiContactManager (const CstiContactManager &other) = delete;
	CstiContactManager (CstiContactManager &&other) = delete;
	CstiContactManager &operator= (const CstiContactManager &other) = delete;
	CstiContactManager &operator= (CstiContactManager &&other) = delete;

	/*!
	 * \brief CstiContactManager destructor
	 */
	~CstiContactManager() override = default;

	/*!
	 * \brief Initializes CstiContactManager
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
	IstiContactSharedPtr getContact(int nIndex) override;

	/*!
	 * \brief Retrieves the number of contacts in the list
	 *
	 * \return The number of contacts in the list
	 */
	int getNumContacts() override;

#if 0
	/*!
	 * \brief Retrieves the first contact found with a specified name
	 *
	 * \param pName The name to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	virtual EstiBool getContactByName(const std::string &name, IstiContact **pContact);
#endif

#if 0
	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \deprecated
	 *
	 * \param pID The ItemId to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	virtual EstiBool getContactByID(const std::string &id, IstiContact **pContact);
#endif

	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param Id The ItemId to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	EstiBool ContactByIDGet(const CstiItemId &Id, IstiContactSharedPtr *contact) override;

	/*!
	 * \brief Retrieves a contact by looking up its ItemId
	 *
	 * \param id The ItemId to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return A unique pointer to the contact, if found
	 */
	IstiContactSharedPtr contactByIDGet(const CstiItemId &id) override;

	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param pPhoneNumber The phone number to search for
	 * \param pContact Outputs the contact that is found, or NULL if not found
	 *
	 * \return True if found, False otherwise
	 */
	EstiBool getContactByPhoneNumber(const std::string &phoneNumber, IstiContactSharedPtr *contact) override;

	/*!
	 * \brief Retrieves the first contact alphabetically with the specified phone number
	 *
	 * \param phoneNumber The phone number to search for
	 *
	 * \return A unique pointer to the contact, if found
	 */
	IstiContactSharedPtr contactByPhoneNumberGet(const std::string &phoneNumber) override;

	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \deprecated
	 *
	 * \param pID The ItemId to search for
	 *
	 * \return The index in the list
	 */
	int getContactIndex(const std::string &id) override;

	/*!
	 * \brief Retrieves the index of a contact from its ItemId
	 *
	 * \param Id The ItemId to search for
	 *
	 * \return The index in the list
	 */
	int ContactIndexGet(const CstiItemId &Id) override;

	/*!
	 * \brief Adds a new contact to the list
	 *
	 * \param pItem The contact being added
	 */
	void addContact(const IstiContactSharedPtr &item) override;

	/*!
	 * \brief Edits an existing contact by looking up its ItemId
	 *
	 * \param pItem The contact that is being updated
	 */
	void updateContact(const IstiContactSharedPtr &item) override;

	/*!
	 * \brief Removes (and deletes) a contact from the list
	 *
	 * \param pItem The contact being removed
	 */
	void removeContact(const IstiContactSharedPtr &item) override;

	/*!
	 * \brief Removes (and deletes) ALL contacts
	 */
	void clearContacts() override;

	/*!
	 * \brief Removes ALL contacts
	 */
	void purgeContacts() override;

	/*!
	 * \brief Determines if the contact list has reached its capacity
	 *
	 * \return True if full, False otherwise
	 */
	bool isFull() override;

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
	size_t getMaxCount() override;

	/*!
	 * \brief Enables or disables the ContactManager
	 *
	 * \param bEnabled True to enable, False to disable
	 */
	void EnabledSet(bool bEnabled) override { m_bEnabled = bEnabled; }

	/*!
	 * \brief Responds to relevant CoreResponses
	 *
	 * \param poResponse The CoreResponse object
	 *
	 * \return True if the response was handled, False otherwise
	 */
	EstiBool CoreResponseHandle(CstiCoreResponse *poResponse);

	/*!
	 * \brief Responds to relevant StateNotifyResponses
	 *
	 * \param poResponse The StateNotifyResponses object
	 *
	 * \return True if the response was handled, False otherwise
	 */
	EstiBool StateNotifyEventHandle(CstiStateNotifyResponse *poResponse);

	/*!
	 * \brief Stops waiting for a response if there was a communication error
	 *
	 * \param poRequest The request that never got to be sent
	 *
	 * \return True if the response was handled, False otherwise
	 */
	EstiBool RemovedUponCommunicationErrorHandle(CstiVPServiceResponse *response);

	/*!
	 * \brief Converts a piece of an XML document into a ContactListItem
	 *
	 * \param pNode The piece of XML
	 *
	 * \return A pointer to a ContactListItem
	 */
	WillowPM::XMLListItemSharedPtr NodeToItemConvert(IXML_Node *pNode) override;

	/*!
	 * \brief Retrieves the entire contact list from Core
	 */
	void ReloadContactsFromCore() override;
#ifdef stiFAVORITES
	/*!
	 * \brief Retrieves the entire favorites list from Core
	 */
	void ReloadFavoritesFromCore() override;
#endif

#ifdef stiIMAGE_MANAGER
	/*!
	 * \brief Updates all contacts with the latest photos from core.
	 */
	void UpdateAllPhotos() override;

	/*!
	 * \brief Removes contact photos from all contacts.
	 */
	void PurgeAllContactPhotos() override;

	/*!
	 * \brief Converts any contacts currently using the supplied image GUID
	 * which core has reported as not found to an appropriate photo that exists.
	 *
	 * \param guid GUID of the photo that core can not find
	 */
	void FixContactsForPhotoNotFound(const std::string& guid) override;

	/*!
	 * \brief Updates the specified contact with the most appropriate photo.
	 * \param itemId unique item identifier for the contact to update
	 */
	void UpdatePhoto(const std::string& itemId);
#endif

	/*!
	 * \brief Enables the Contact Photo feature
	 * \param bEnabled Whether or not to enable photos
	 */
	void ContactPhotosEnable(bool bEnabled) override { m_bContactPhotosEnabled = bEnabled; }

#ifdef stiFAVORITES

	void FavoritesMaxSet (int favoriteMax) override;

	stiHResult FavoriteAdd (
		const CstiItemId &PhoneNumberId,
		bool bIsContact = true) override;

	stiHResult FavoriteAddWithPosition(
			const CstiItemId &PhoneNumberId,
			bool bIsContact = true,
			int position = 0);

	stiHResult FavoriteRemove (
		int nIndex) override;

	stiHResult FavoriteRemove (
		const CstiItemId &PhoneNumberId) override;

	stiHResult FavoriteMove (
		int nOldIndex,
		int nNewIndex) override;

	stiHResult FavoriteListSet (
		const std::vector<CstiFavorite> &list) override;

	bool IsFavorite (
		const CstiItemId &PhoneNumberId) const override;

	bool IsFavorite (
		const std::string &dialString) const override;

	int FavoritesCountGet() const override;

	int FavoritesMaxCountGet() const override;

	bool FavoritesFull() const override;

	CstiFavoriteSharedPtr FavoriteByIndexGet(
		int nIndex) const override;

	int FavoriteIndexByPhoneNumberIdGet(
		const CstiItemId &PhoneNumberId) const;

	CstiFavoriteSharedPtr FavoriteByPhoneNumberIdGet(
		const CstiItemId &PhoneNumberId) const override;

	IstiContactSharedPtr ContactByFavoriteGet(
		const CstiFavoriteSharedPtr &favorite) const override;

	void favoritesPreserve(
		const IstiContactSharedPtr &pItem);

#endif

	/*! 
	 * \brief Determines if the company name field is available
	 *
	 * \return true if enabled, false otherwise
	 */
	bool CompanyNameEnabled() const override;
	
	void Refresh () override;

	/*!
	 * \brief Locks the ContactManager
	 */
	void Lock() const override { XMLManager::lock(); }

	/*!
	 * \brief Unlocks the ContactManager
	 */
	void Unlock() const override { XMLManager::unlock(); }

	ISignal<IstiContactManager::Response, stiHResult>& contactErrorSignalGet () override;

	ISignal<IstiContactManager::Response, stiHResult>& favoriteErrorSignalGet () override;

private:

	void addCstiContact(const CstiContactListItemSharedPtr &pCstiItem);
	void updateCstiContact(const CstiContactListItemSharedPtr &pCstiItem);
	void removeCstiContact(const CstiContactListItemSharedPtr &pCstiItem);
	EstiBool ListUpdate(CstiCoreResponse *poResponse);
	
	stiHResult AuthenticatedSet (EstiBool bAuthenticated);

	unsigned int CoreRequestSend(CstiCoreRequest *poCoreRequest, EstiBool bRemoveUponError, unsigned int unExpectedResult);
	void CallbackMessageSend(EstiCmMessage eMessage, size_t Param);
	void errorSend(CstiCoreResponse *coreResponse, bool favorite);
	Response coreResponseConvert(CstiCoreResponse::EResponse responseType);
	void ImportContacts(const std::string &phoneNumber, const std::string &ppassword) override;
	EstiBool RemoveRequestId(unsigned int unRequestId, unsigned int *punExpectedResult = nullptr);
	void ContactListItemChangedHandle(CstiStateNotifyResponse *pStateNotifyResponse);
	void ContactListItemRemovedHandle(CstiStateNotifyResponse *pStateNotifyResponse);
#ifdef stiFAVORITES
	void CreateFavoriteFromNewNumbers(const ContactListItemSharedPtr &contactListItem);
#endif

	void DownloadURLGetResultHandle(CstiCoreResponse *poResponse);

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);

	CstiCoreService *m_pCoreService;
	IstiImageManager *m_pImageManager;
	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	CstiEventQueue& m_eventQueue;

	bool m_bEnabled;
	
	EstiBool m_bAuthenticated;

	struct PendingRequest
	{
		unsigned int m_unCookie;
		unsigned int m_unExpectedResult;
	};

	std::list<PendingRequest> m_PendingCookies;
	
	struct PendingContactListItemChangedRequest
	{
		std::string m_ItemId;
		unsigned int m_unCoreRequestId;
	};
	
	std::list<PendingContactListItemChangedRequest> m_PendingContactListItemChangedRequests;

	unsigned int m_unLastCookie;

	struct FavoritePosChange
	{
		int nOldIndex;
		int nNewIndex;
	};

	struct PhotoState
	{
		CstiContactListItem::EPhoneNumberType number_type{CstiContactListItem::ePHONE_MAX};
		std::string guid;
		std::string timestamp;

		PhotoState() = default;
	};

	typedef std::map<IstiContactSharedPtr, PhotoState> ContactPhotoStateMap;

	ContactPhotoStateMap m_photoUpdateState;

	typedef std::set<std::string> ExpectedPhotoDownloadURLs;

	ExpectedPhotoDownloadURLs m_expectedPhotoDownloadURLs;
	
	typedef std::set<std::string> ItemIdSet;
	
	ItemIdSet m_pendingPhotoCheck;

	bool m_bContactPhotosEnabled;
	
#ifdef stiFAVORITES
	FavoriteList m_Favorites;
#endif
	//Signals
	CstiSignalConnection::Collection m_signalConnections;
	CstiSignal<IstiContactManager::Response, stiHResult> contactErrorSignal;
	CstiSignal<IstiContactManager::Response, stiHResult> favoriteErrorSignal;

}; // end ContactManager class definition

#endif // CONTACTMANAGER_H
