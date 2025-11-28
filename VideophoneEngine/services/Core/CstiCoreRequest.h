///
/// \file CstiCoreRequest.h
/// \brief Declaration of the class containing APIs for sending requests to core.
///
/// This class has the wrapper functions for creating core requests.  The public function names are
///  generally the same name as the core request itself (except the functions often start with a
///  lower-case letter).
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///


#ifndef CSTICOREREQUEST_H
#define CSTICOREREQUEST_H

//
// Includes
//

#include "CstiCallList.h"
#include "CstiContactList.h"
#include "CstiPhoneNumberList.h"
#include "CstiUserAccountInfo.h"
#include "CstiVPServiceRequest.h"
#include "CstiCoreResponse.h"

//
// Constants
//

extern const char g_szCRQUserTypeSorenson[];
extern const char g_szCRQUserTypeUser[];

//
// Typedefs
//

//
// Forward Declarations
//
#ifdef stiFAVORITES
class CstiFavorite;
struct SstiFavoriteList;
#endif

/**
 * \brief Declaration of APIs to make core requests
*/
class CstiCoreRequest : public CstiVPServiceRequest
{
public:
	///
	/// \brief Enumerates the value type of a user or phone setting item
	///
	enum ESettingType
	{
		eSETTING_TYPE_STRING,        /*!< Value is of type string */
		eSETTING_TYPE_INT,           /*!< Value is of type int */
		eSETTING_TYPE_BOOL,          /*!< Value is of type bool */
	}; // end ESettingType

	///
	/// \brief Enumerates the Real Phone Number type
	///
	enum ERealNumberType
	{
		eTOLL_FREE,       /*!< for 800-type phone number */
		eLOCAL,           /*!< for local phone number */
	}; // end ERealNumberType

	///
	/// \brief Enumerates the string ID type used to retrieve an ImageURL
	///
	enum EIdType
	{
		ePHONE_NUMBER,    /*!< Use the phone number to retrieve image */
		ePUBLIC_ID,       /*!< Use the GUID to retrieve image */
	}; // end EIdType

	///
	/// \brief Enumerates the Agreement accepted status
	///
	enum EAgreementStatus
	{
		eAGREEMENT_ACCEPTED,        /*!< Agreement was accepted */
		eAGREEMENT_REJECTED,        /*!< Agreement was rejected */
	}; // end EAgreementStatus

	enum EstiCoreRequestPriorities
	{
		estiCRP_SERVICE_CONTACTS_GET = 0,
		estiCRP_FIRMWARE_UPDATE_CHECK = 25,
		estiCRP_INITIAL_REQUESTS = 50,
		estiCRP_PHONE_ACCOUNT_CREATE = 100,
		estiCRP_USER_LOGIN_CHECK = 250,
		estiCRP_AUTHENTICATE = 300,
		estiCRP_AGREEMENT_STATUS_GET = 350,
		estiCRP_USER_ACCOUNT_INFO_GET = 400,
		estiCRP_REGISTRATION_INFO_GET = 600,
		estiCRP_PHONE_ONLINE = 700,
		estiCRP_DIRECTORY_RESOLVE = 800,
		estiCRP_NORMAL = 1000
	};

	///
	/// \brief static members holding core major and minor version
	///
	static int m_nAPIMajorVersion;
	static int m_nAPIMinorVersion;

    CstiCoreRequest() = default;
	~CstiCoreRequest() override = default;

	CstiCoreRequest (const CstiCoreRequest &) = delete;
	CstiCoreRequest (CstiCoreRequest &&) = delete;
	CstiCoreRequest &operator= (const CstiCoreRequest &) = delete;
	CstiCoreRequest &operator= (CstiCoreRequest &&) = delete;

////////////////////////////////////////////////////////////////////////////////
// The following APIs are called when submitting the request to the core server.
////////////////////////////////////////////////////////////////////////////////

	EstiBool authenticationRequired();

	// Indicates whether a request must be handled online.  If set to false,
	// OfflineCore will not handle this request.
	bool onlineOnlyGet ();
	void onlineOnlySet (bool value);

	void PrioritySet (
		int nPriority);

	int PriorityGet () const;

////////////////////////////////////////////////////////////////////////////////
// The following APIs create each of the supported core requests.
////////////////////////////////////////////////////////////////////////////////

	// NOTE: no need to take std::shared_ptr variants as arguments
	// they are just read only objects

	int agreementGet(const char* pszPublicId);

	int agreementStatusGet();

	int agreementStatusSet(
		const char* pszAgreementPublicId,
		EAgreementStatus eStatus);

	int callListCountGet(
		CstiCallList::EType eCallListType,
		bool bUnique = false,
		time_t ttAfterTime = 0);

	int callListGet(
		CstiCallList::EType eCallListType,
		const char* pszUserType = nullptr,
		unsigned int baseIndex = 0,
		unsigned int count = 0,
		CstiCallList::ESortField eSortField = CstiCallList::eNAME,
		CstiList::SortDirection eSortDir = CstiList::SortDirection::ASCENDING,
		int nThreshold = -1,
		bool bUnique = false,
		bool bFlagAddressBook = false);

	int callListItemAdd (
			const CstiCallListItem &newCallListItem,
			CstiCallList::EType eCallListType);

	int callListItemEdit (
			const CstiCallListItem &editCallListItem,
			CstiCallList::EType eCallListType);

	int callListItemGet (
			const CstiCallListItem &getCallListItem,
			CstiCallList::EType eCallListType);

	int callListItemRemove (
			const CstiCallListItem &callListItem,
			CstiCallList::EType eCallListType);

	stiHResult callListSet (const CstiCallList *callList);

	int clientAuthenticate(
		const char *szName,
		const char *szPhoneNumber,
		const char *szPIN,
		const char *szUniqueID = nullptr,
		const char *pszLoginAs = nullptr);

	int userLoginCheck(
		const char *szPhoneNumber,
							const char *szPIN,
							const char *szUniqueID = nullptr);
	
	int clientLogout();

	int contactListCountGet();

	int contactListGet(
		const char* pszUserType = nullptr,
		unsigned int baseIndex = 0,
		unsigned int count = 0,
		int nThreshold = -1);

	int contactListItemAdd (
		const CstiContactListItem &newContactListItem);

	int contactListItemEdit (
		const CstiContactListItem &editContactListItem);

	int contactListItemGet (
		const CstiContactListItem &getContactListItem);

	int contactListItemRemove (
		const CstiContactListItem &contactListItem);

	stiHResult contactListSet (
		const CstiContactList *contactList);

#ifdef stiFAVORITES
	int favoriteListGet();

	int favoriteListSet(
		const SstiFavoriteList &list);

	int favoriteItemAdd (
		const CstiFavorite &favorite);

	int favoriteItemEdit (
		const CstiFavorite &favorite);

	int favoriteItemRemove (
		const CstiFavorite &favorite);
#endif

	int directoryResolve(
		const char *pszDialString,
		EstiDialMethod eDialMethod = estiDM_BY_DS_PHONE_NUMBER,
		const char *pszFromDialString = nullptr,
		EstiBool bBLockCallerId = estiFALSE);

	int emergencyAddressDeprovision();

	int emergencyAddressGet();

	int emergencyAddressProvision(
		const char *pszAddress1,
		const char *pszAddress2,
		const char *pszCity,
		const char *pszState,
		const char *pszZip);

	int emergencyAddressProvisionStatusGet();

	int emergencyAddressUpdateAccept();

	int emergencyAddressUpdateReject();

	int imageDelete();

	int imageDownloadURLGet(const char *pszId, EIdType eIdType);

	int imageUploadURLGet();

	int missedCallAdd(
		const char *calleeDialString,
		time_t callTime,
        EstiBool bBLockCallerId);

	int missedCallAdd(
		const char *calleeDialString,
		EstiDialMethod eDialMethod = estiDM_BY_DIAL_STRING,
		const char *dialString = "",
		const char *name = "",
		time_t ttCallTime = 0,
		const char *pszCallPublicId = "",
        EstiBool bBLockCallerId = estiFALSE);

	int phoneAccountCreate(
		const std::string &phoneType);

	int phoneOnline(
		const char *dialString);

	int phoneSettingsGet(
		const char *phoneSettingName = nullptr);

	int phoneSettingsSet(
		const char *phoneSettingName,
							const char *phoneSettingValue,
							ESettingType eSettingType);

	int phoneSettingsSet(
		const char *phoneSettingName,
							const char *phoneSettingStringValue);

	int phoneSettingsSet(
		const char *phoneSettingName,
							int phoneSettingIntValue);

	int phoneSettingsSet(
		const char *phoneSettingName,
							EstiBool phoneSettingBoolValue);

	int primaryUserExists();

	int publicIPGet();

	int registrationInfoGet ();

	int ringGroupCreate (
		const char *pszAlias,
		const char *pszPin);

	int ringGroupCredentialsUpdate (
		const char *pszPhoneNumber,
		const char *pszPin);

	int ringGroupCredentialsValidate (
		const char *pszPhoneNumber,
		const char *pszPin);

	int ringGroupInfoGet ();

	int ringGroupInfoSet (
		const char *pszPhoneNumber,
		const char *pszDescription);

	int ringGroupInviteAccept (
		const char *pszPhoneNumber);

	int ringGroupInviteInfoGet ();

	int ringGroupInviteReject (
		const char *pszPhoneNumber);

	int ringGroupPasswordSet (
		const char *pszPin);

	int ringGroupUserInvite (
		const char *pszPhoneNumber,
		const char *pszDescription);

	int ringGroupUserRemove (
		const char *pszPhoneNumber);

	int screenSaverListGet();

	int serviceContact(
		const char *svcPriority,
		const char *svcUrl);

	int serviceContactsGet();

	int signMailRegister();

	int timeGet();

	int trainerValidate(
		const char *pszName,
		const char *pszPassword);

	int getUserDefaults();

	int updateVersionCheck(
		const char *appVersion,
		const char *appLoaderVersion = nullptr);
	
	int versionGet();

	int uriListSet (
		const char *pszPublicPriority1Uri,
		const char *pszPrivatePriority1Uri,
		const char *pszPublicPriority2Uri,
		const char *pszPrivatePriority2Uri,
		const char *pszPublicPriority3Uri,
		const char *pszPrivatePriority3Uri);

	int userAccountAssociate(
		const char *szName,
		const char *szPhoneNumber,
		const char *szPin,
		EstiBool bIsPrimaryUser = estiTRUE,
		EstiBool bMatchingPrimaryUserRequired = estiFALSE,
		EstiBool bMultipleEndpointEnabled = estiFALSE);

	int userAccountInfoGet();

	int userPinSet (const std::string& pin);

	int profileImageIdSet (const std::string& imageId);

	int signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary);

	int userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification);

	int userInterfaceGroupGet();

	int userSettingsGet(
		const char *userSettingName);

	int userSettingsSet(
		const char *userSettingName,
						const char *userSettingValue);

	int userSettingsSet(
		const char *userSettingName,
						int userSettingIntValue);

	int userSettingsSet(
		const char *userSettingName,
						EstiBool userSettingBoolValue);

	int userSettingsSet(
		const char *userSettingName,
						const char *userSettingValue,
						ESettingType eUserSettingType);

	static void SetUniqueID (
		const std::string &uniqueID);

	int logCallTransfer(
		const char *pszTransferredFromDialString,
		const char *pszTransferredToDialString,
		const char *pszOriginalCallPublicId,
		const char *pszTransferredCallPublicId,
		bool bInitiatedTransfer);

	int contactsImport(
		const char *szPhoneNumber,
		const char *szPIN);

	stiHResult vrsAgentHistoryAdd (const CstiCallListItem& callHistoryItem, EstiDirection callDirection);

	/// The phone setting list
	std::list <std::string> m_PhoneSettingList;

	/// The user setting list
	std::list <std::string> m_UserSettingList;

private:

	const char *RequestTypeGet() override;

	stiHResult addCallListItemToElement(
		const CstiCallListItem &callListItem,
		IXML_Element *callListElement);

	stiHResult addCallListItemIdToElement(
		const CstiCallListItem &callListItem,
		IXML_Element *callListElement);

	IXML_Element* obtainExistingCallList(
		const char *listType);

	stiHResult constructCallListAddOrEditRequest(
		const char *requestType,
		const CstiCallListItem &newCallListItem,
		CstiCallList::EType eCallListType);

	stiHResult constructCallListRemoveOrGetRequest(
		const char *requestType,
		const CstiCallListItem &newCallListItem,
		CstiCallList::EType eCallListType);

	stiHResult addContactListItemToElement(
		const CstiContactListItem &contactListItem,
									IXML_Element *contactListElement);

	stiHResult constructContactListAddOrRemoveRequest(
		const char *requestType,
		const CstiContactListItem &newContactListItem);

	stiHResult addFavoriteItemToElement(
		IXML_Element *pElement,
		const CstiFavorite &favorite);

	/// This request requires user to be authenticated (logged in)
	EstiBool m_bAuthenticationRequired{estiFALSE};

	bool m_onlineOnly {false};

	int m_nPriority{estiCRP_NORMAL};

	/// The unique id for this core request
	static std::string m_uniqueID;
	
};

#endif //CSTICOREREQUEST_H
