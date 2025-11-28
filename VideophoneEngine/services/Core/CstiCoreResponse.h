	///
/// \file CstiCoreResponse.h
/// \brief Declaration of the core response class.
///
/// This class has the enumerations, defines, and functions that help in processing core responses.
///
/// \note The "VP Services API" documentation on the Engineering website contains the official
///  documentation for core requests and responses, including error codes.  That documentation
///  should be used for the most up-to-date API information.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///

#ifndef CSTICORERESPONSE_H
#define CSTICORERESPONSE_H

//
// Includes
//
#include "stiSVX.h"
#include "CstiCallList.h"
#include "CstiContactList.h"
#include "CstiPhoneNumberList.h"
#include "CstiUserAccountInfo.h"
#include "CstiDirectoryResolveResult.h"
#include "SstiSettingItem.h"
#include <vector>
#include "CstiVPServiceResponse.h"

//
// Constants
//
#define MAX_SERVICE_CONTACTS	2

//
// Typedefs
//

//
// Forward Declarations
//
#ifdef stiFAVORITES
struct SstiFavoriteList;
#endif

//
// Globals
//

//
// Class Declaration
//

class CstiCoreResponse : public CstiVPServiceResponse
{
public:
	enum EResponse
	{
		eRESPONSE_UNKNOWN,
		eRESPONSE_ERROR,
		eAGREEMENT_GET_RESULT,
		eAGREEMENT_STATUS_GET_RESULT,
		eAGREEMENT_STATUS_SET_RESULT,
		eCALL_LIST_COUNT_GET_RESULT,
		eCALL_LIST_GET_RESULT,
		eCALL_LIST_ITEM_ADD_RESULT,
		eCALL_LIST_ITEM_EDIT_RESULT,
		eCALL_LIST_ITEM_GET_RESULT,
		eCALL_LIST_ITEM_REMOVE_RESULT,
		eCALL_LIST_SET_RESULT,
		eCLIENT_AUTHENTICATE_RESULT,
		eCLIENT_LOGOUT_RESULT,
		eDIRECTORY_RESOLVE_RESULT,
		eEMERGENCY_ADDRESS_DEPROVISION_RESULT,
		eEMERGENCY_ADDRESS_GET_RESULT,
		eEMERGENCY_ADDRESS_PROVISION_RESULT,
		eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT,
		eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT,
		eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT,
		eFAVORITE_ITEM_ADD_RESULT,
		eFAVORITE_ITEM_EDIT_RESULT,
		eFAVORITE_ITEM_REMOVE_RESULT,
		eFAVORITE_LIST_GET_RESULT,
		eFAVORITE_LIST_SET_RESULT,
		eIMAGE_DELETE_RESULT,
		eIMAGE_DOWNLOAD_URL_GET_RESULT,
		eIMAGE_UPLOAD_URL_GET_RESULT,
		eMISSED_CALL_ADD_RESULT,
		ePHONE_ACCOUNT_CREATE_RESULT,
		ePHONE_ONLINE_RESULT,
		ePHONE_SETTINGS_GET_RESULT,
		ePHONE_SETTINGS_SET_RESULT,
		ePRIMARY_USER_EXISTS_RESULT,
		ePUBLIC_IP_GET_RESULT,
		eREGISTRATION_INFO_GET_RESULT,
		eRING_GROUP_CREATE_RESULT,
		eRING_GROUP_CREDENTIALS_UPDATE_RESULT,
		eRING_GROUP_CREDENTIALS_VALIDATE_RESULT,
		eRING_GROUP_INFO_GET_RESULT,
		eRING_GROUP_INFO_SET_RESULT,
		eRING_GROUP_INVITE_ACCEPT_RESULT,
		eRING_GROUP_INVITE_INFO_GET_RESULT,
		eRING_GROUP_INVITE_REJECT_RESULT,
		eRING_GROUP_PASSWORD_SET_RESULT,
		eRING_GROUP_USER_INVITE_RESULT,
		eRING_GROUP_USER_REMOVE_RESULT,
		eSERVICE_CONTACTS_GET_RESULT,
		eSERVICE_CONTACT_RESULT,
		eSIGNMAIL_REGISTER_RESULT,
		eTIME_GET_RESULT,
		eTRAINER_VALIDATE_RESULT,
		eURI_LIST_SET_RESULT,
		eUSER_ACCOUNT_ASSOCIATE_RESULT,
		eUSER_ACCOUNT_INFO_GET_RESULT,
		eUSER_ACCOUNT_INFO_SET_RESULT,
		eUSER_INTERFACE_GROUP_GET_RESULT,
		eUSER_LOGIN_CHECK_RESULT,
		eUSER_SETTINGS_GET_RESULT,
		eUSER_SETTINGS_SET_RESULT,
		eUPDATE_VERSION_CHECK_RESULT,
		eVERSION_GET_RESULT,
		eLOG_CALL_TRANSFER_RESULT,
		eCONTACTS_IMPORT_RESULT,
		eUSER_DEFAULTS_RESULT,
		eSCREEN_SAVER_LIST_GET_RESULT,
		eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT,
		eVRS_AGENT_HISTORY_DIALED_ADD_RESULT,
	}; // end EResponse

	enum ECoreError
	{
		eCONNECTION_ERROR = -1,

		// Errors occurring during message processing
		eNO_ERROR = 0,							// No Error
		eNOT_LOGGED_IN = 1,						// Not Logged In
		eUSERNAME_OR_PASSWORD_INVALID = 2,		// Username and Password not valid
		ePHONE_NUMBER_NOT_FOUND = 3,			// Phone Number does not exist
		eDUPLICATE_PHONE_NUMBER = 4,			// Phone Number already exists
		eERROR_CREATING_USER = 5,				// Could not create user
		eERROR_ASSOCIATING_PHONE_NUMBER = 6,	// Phone Number could not be associated
		eCANNOT_ASSOCIATE_PHONE_NUMBER = 7,		// Phone Number could not be associated - not controlled by user or available
		eERROR_DISASSOCIATING_PHONE_NUMBER = 8,	// Phone Number could not be disassociated
		eCANNOT_DISASSOCIATE_PHONE_NUMBER = 9,	// Phone Number could not be disassociated - not controlled by user
		eERROR_UPDATING_USER = 10,				// Unable to update user or phone
		eUNIQUE_ID_UNRECOGNIZED = 11,			// UniqueId not recognized
		ePHONE_NUMBER_NOT_REGISTERED = 12,		// Phone Number is not registered with Directory
		eDUPLICATE_UNIQUE_ID = 13,				// UniqueId Already Exists
		eCONNECTION_TO_SIGNMAIL_DOWN = 14,		// Unable to connect to SignMail server
		eNO_INFORMATION_SENT = 15,				// No information sent
		eUNIQUE_ID_EMPTY = 17,					// Unique ID cannot be empty
		eREAL_NUMBER_LIST_RELEASE_FAILED = 22,	// Real Number service failed to release the list of numbers
		eRELEASE_NUMBERS_NOT_SPECIFIED = 23,	// Request did not specify phone numbers to release
		eCOUNT_ATTRIBUTE_INVALID = 25,			// Count attribute has an invalid value
		eREAL_NUMBER_LIST_GET_FAILED = 26,		// Real Number service failed to get a list of numbers
		eREAL_NUMBER_SERVICE_UNAVAILABLE = 27,	// Unable to connect to NANP (real number) server
		eNOT_PRIMARY_USER_ON_PHONE = 28,		// User is not the Primary User of this phone
		ePRIMARY_USER_ON_ANOTHER_PHONE = 29,	// User is a Primary User on another Phone
		ePROVISION_REQUEST_FAILED = 30,
		eNO_CURRENT_EMERGENCY_STATUS_FOUND = 31,
		eNO_CURRENT_EMERGENCY_ADDRESS_FOUND = 32,
		eEMERGENCY_ADDRESS_DATA_BLANK = 33,
		eDEPROVISION_REQUEST_FAILED = 34,
		eUNABLE_TO_CONTACT_DATABASE = 35,
		eNO_REAL_NUMBER_AVAILABLE = 36,
		eUNABLE_TO_ACQUIRE_NUMBER = 37,
		eNO_PENDING_NUMBER_AVAILABLE = 38,
		eROUTER_INFO_NOT_CLEARED = 39,
		eNO_PHONE_INFO_FOUND = 40,
		eQIC_DOES_NOT_EXIST = 41,
		eQIC_CANNOT_BE_REACHED = 42,
		eQIC_NOT_DECLARED = 43,
		eIMAGE_PROBLEM = 44,
		eIMAGE_PROBLEM_SAVING_DATA = 45,
		eIMAGE_PROBLEM_DELETING = 46,
		eIMAGE_LIST_GET_PROBLEM = 47,
		eENDPOINT_MATCH_ERROR = 48,
		eBLOCK_LIST_ITEM_DENIED = 49,
		eCALL_LIST_PLAN_NOT_FOUND = 50,
		eENDPOINT_TYPE_NOT_FOUND = 51,
		ePHONE_TYPE_NOT_RECOGNIZED = 52,
		eCANNOT_GET_SERVICE_CONTACT_OVERRIDES = 53,
		eCANNOT_CONTACT_ITRS = 54,
		eURI_LIST_SET_MISMATCH = 55,
		eURI_LIST_SET_DELETE_ERROR = 56,
		eURI_LIST_SET_ADD_ERROR = 57,
		eSIGNMAIL_REGISTER_ERROR = 58,
		eADDED_MORE_CONTACTS_THAN_ALLOWED = 59,
		eCANNOT_DETERMINE_WHICH_LOGIN = 60,
		eLOGIN_VALUE_INVALID = 61,
		eERROR_UPDATING_ALIAS = 62,
		eERROR_CREATING_GROUP = 63,
		eINVALID_PIN = 64,
		eUSER_CANNOT_CREATE_RINGGROUP = 65,
		eENDPOINT_NOT_RINGGROUP_CAPABLE = 66,
		eCALL_LIST_ITEM_NOT_OWNED = 67,
		eVIDEO_SERVER_UNREACHABLE = 68,
		eCANNOT_UPDATE_ENUM_SERVER = 69,
		eCALL_LIST_ITEM_NOT_FOUND = 70,
		eERROR_ROLLING_BACK_RING_GROUP = 71,
		eNO_INVITE_FOUND = 72,
		eNOT_MEMBER_OF_RING_GROUP = 73,
		eCANNOT_INVITE_PHONE_NUMBER_TO_GROUP = 74,
		eCANNOT_INVITE_USER = 75,
		eRING_GROUP_DOESNT_EXIST = 76,
		eRING_GROUP_NUMBER_NOT_FOUND = 77,
		eRING_GROUP_INVITE_ACCEPT_NOT_COMPLETED = 78,
		eRING_GROUP_INVITE_REJECT_NOT_COMPLETED = 79,
		eRING_GROUP_NUMBER_CANNOT_BE_EMPTY = 80,
		eINVALID_DESCRIPTION_LENGTH = 81,
		eUSER_NOT_FOUND = 82,
		eINVALID_USER = 83,
		eGROUP_USER_NOT_FOUND = 84,
		eUSER_BEING_REMOVED_NOT_IN_GROUP = 85,
		eUSER_CANNOT_BE_REMOVED_FROM_GROUP = 86,
		eUSER_NOT_MEMBER_OF_GROUP = 87,
		eELEMENT_HAS_INVALID_PHONE_NUMBER = 88,
		ePASSWORD_AND_CONFIRM_DO_NOT_MATCH = 89,
		eCANNOT_BLOCK_RING_GROUP_MEMBER = 90,
		eCANNOT_REMOVE_LAST_USER_FROM_GROUP = 91,
		eURI_LIST_MUST_CONTAIN_ITRS_URIS = 92,
		eINVALID_URI = 93,
		eREDIRECT_TO_AND_FROM_URIS_CANNOT_BE_SAME = 94,
		eREMOVE_OTHER_MEMBERS_BEFORE_REMOVING_RING_GROUP = 95,
		eNEED_ONE_MEMBER_IN_RING_GROUP = 96,
		eCOULD_NOT_LOCATE_ACTIVE_LOCAL_NUMBER = 97,
		eUNKNOWN_ERROR_2 = 98,
		eINVALID_GROUP_CREDENTIALS = 99,
		eNOT_MEMBER_OF_THIS_GROUP = 100,
		eERROR_SETTING_GROUP_PASSWORD = 101,
		ePHONE_NUMBER_CANNOT_BE_EMPTY = 102,
		eERROR_WHILE_SETTING_RING_GROUP_INFO = 103,
		eERROR_DIAL_STRING_REQUIRED_ELEMENT = 104,
		eERROR_CANNOT_REMOVE_RING_GROUP_CREATOR = 105,
		eERROR_IMAGE_NOT_FOUND = 106,
		eERROR_NO_AGREEMENT_FOUND = 107,
		//eERROR 108,
		eERROR_UPDATING_AGREEMENT = 109,
		// eERROR 110,
		eERROR_GUID_INCORRECT_FORMAT = 111,
		eERROR_CORE_CONFIGURATION = 112,
		eERROR_REMOVING_IMAGE = 113,
		eERROR_UNABLE_SET_SETTING = 114,
		eERROR_OFFLINE_ACTION_NOT_ALLOWED = 115,
		

		// High-Level Server Errors (message not processed)
		eDATABASE_CONNECTION_ERROR = 0xFFFFFFFB,// Database connection error
		eXML_VALIDATION_ERROR = 0xFFFFFFFC,		// XML Validation Error
		eXML_FORMAT_ERROR = 0xFFFFFFFD,			// XML Format Error
		eGENERIC_SERVER_ERROR = 0xFFFFFFFE,		// General Server Error (exception, etc.)
		eUNKNOWN_ERROR = 0xFFFFFFFF
	}; // end ECoreError

	// Enum for Ring Group member state
	enum ERingGroupMemberState
	{
		eRING_GROUP_STATE_UNKNOWN,
		eRING_GROUP_STATE_ACTIVE,
		eRING_GROUP_STATE_PENDING,
	}; // EResponse


	///
	/// \brief Struct defining the Relay language valuesstRelayLanguage
	///
	struct SRelayLanguage
	{
		SRelayLanguage () = default;

		SRelayLanguage (
			int id,
			const std::string &name)
		:
			nId(id),
			Name(name)
		{
		}

		int nId{0};			/*!< Id of the language (e.g. 0 = English) */
		std::string Name;	/*!< Name of the language (e.g. English) */
	};

	///
	/// \brief Struct defining the Ring group information
	///
	struct SRingGroupInfo
	{
		std::string Description;		/*!< Descriptive name (or alias) of device */
		std::string LocalPhoneNumber;		/*!< Private local phone number of device */
		std::string TollFreePhoneNumber;	/*!< Private toll-free phone number of device */
		ERingGroupMemberState eState {eRING_GROUP_STATE_UNKNOWN	};	/*!< Member state (e.g. Active, Pending) */
		bool bIsGroupCreator{false};			/*!< True if group creator */
	};

	///
	/// \brief Struct defining the registration information for SIP proxy, etc.
	///
	struct SRegistrationCredentials
	{
		std::string UserName;
		std::string Password;
	};
	
	struct SRegistrationInfo
	{
		EstiProtocol eType {estiPROT_UNKNOWN};	/*!< RegistrationInfo type (e.g. SIP) */
		std::string PublicDomain;		/*!< Public domain url */
		std::string PrivateDomain;		/*!< Private domain url */
		std::vector<SRegistrationCredentials> Credentials;
	};
	
	///
	/// \brief Struct defining the Agreement information
	///

	struct SAgreement
	{
		std::string AgreementType;
		std::string AgreementPublicId;
		std::string AgreementStatus;
	};

	///
	/// \brief Struct defining the ScreenSaverInfo
	///
	struct SScreenSaverInfo
	{
		std::string screenSaverId;		        /*!< The screensaver ID */
		std::string name;		        		/*!< The display name of the screensaver */
		std::string description;				/*!< The description of the screensaver */
		std::string schedule;			   	 	/*!< The names and start dates for seasonal screensavers */
		std::string previewVideoUrl;	    	/*!< Url to a preview video of the screensaver */
		std::string screenSaverVideoUrl;	    /*!< Url of the screen saver video */
	};

	CstiCoreResponse (
		CstiVPServiceRequest *request,
		EResponse eResponse,
		EstiResult eResponseResult,
		ECoreError eError,
		const char *pszError);

	CstiCoreResponse (const CstiCoreResponse &) = delete;
	CstiCoreResponse (CstiCoreResponse &&) = delete;
	CstiCoreResponse &operator= (const CstiCoreResponse &) = delete;
	CstiCoreResponse &operator= (CstiCoreResponse &&) = delete;
	
	inline EResponse ResponseGet () const;

	inline ECoreError ErrorGet () const;

	inline EstiResult ErrorSet (ECoreError eError);

	inline CstiCallList * CallListGet () const;

	inline void CallListSet (CstiCallList *poCallList);

	inline CstiContactList * ContactListGet () const;

	inline void ContactListSet (CstiContactList *poCallList);

#ifdef stiFAVORITES
	inline SstiFavoriteList *FavoriteListGet () const;

	inline void FavoriteListSet (SstiFavoriteList *poFavoriteList);
#endif

	inline CstiPhoneNumberList * PhoneNumberListGet ();

	inline void PhoneNumberListSet (CstiPhoneNumberList *poPhoneNumberList);

	inline CstiUserAccountInfo * UserAccountInfoGet () const;
	inline void UserAccountInfoSet (CstiUserAccountInfo * poUserAccountInfo);

	inline CstiDirectoryResolveResult * DirectoryResolveResultGet () const;
	inline void DirectoryResolveResultSet (CstiDirectoryResolveResult * poDirectoryResolveResult);

	void RelayLanguageListGet (std::vector <SRelayLanguage> *pRelayLanguage) const;
	void RelayLanguageListSet (const std::vector <SRelayLanguage> *pRelayLanguage);

	void RingGroupInfoListGet (std::vector <SRingGroupInfo> *pRingGroupInfoList) const;
	void RingGroupInfoListSet (const std::vector <SRingGroupInfo> *pRingGroupInfoList);

	void RegistrationInfoListGet (std::vector <SRegistrationInfo> *pRegistrationInfoList) const;
	void RegistrationInfoListSet (const std::vector <SRegistrationInfo> *pRegistrationInfoList);

	void AgreementListGet (std::vector <SAgreement> *pAgreementList) const;
	void AgreementListSet (const std::vector <SAgreement> *pAgreementList);

	std::vector<SstiSettingItem> SettingListGet ();

	void SettingListSet (
		const std::vector<SstiSettingItem> &list);
	
	void MissingSettingsListGet (
		std::list <std::string> *list) const;

	void MissingSettingsListSet (
		const std::list <std::string> &list);
	
	void CoreServiceContactsGet (
		std::string ServiceContacts[]);

	void MessageServiceContactsGet (
		std::string ServiceContacts[]);

	void ConferenceServiceContactsGet (
		std::string ServiceContacts[]);

	void RemoteLoggerServiceContactsGet (
		std::string ServiceContacts[]);

	void NotifyServiceContactsGet (
		std::string ServiceContacts[]);

	std::string publicIPGet()
	{
		return m_publicIP;
	}

	time_t timeGet()
	{
		return m_time;
	}

	void CoreServiceContactsSet (
		std::string ServiceContacts[]);

	void MessageServiceContactsSet (
		std::string ServiceContacts[]);

	void ConferenceServiceContactsSet (
		std::string ServiceContacts[]);

	void RemoteLoggerServiceContactsSet (
		std::string ServiceContacts[]);

	void NotifyServiceContactsSet (
		std::string ServiceContacts[]);
	
	void publicIPSet(const char *publicIP)
	{
		m_publicIP = publicIP;
	}

	void timeSet(time_t time)
	{
		m_time = time;
	}

	std::vector <SScreenSaverInfo> *ScreenSaverInfoListGet();
	void ScreenSaverInfoListSet (const std::vector <SScreenSaverInfo> &screenSaverInfoList);

protected:

	~CstiCoreResponse () override;

private:
	
	EResponse m_eResponse;
	ECoreError m_eError;

	std::string m_CoreServiceContacts[MAX_SERVICE_CONTACTS];
	std::string m_NotifyServiceContacts[MAX_SERVICE_CONTACTS];
	std::string m_MessageServiceContacts[MAX_SERVICE_CONTACTS];
	std::string m_ConferenceServiceContacts[MAX_SERVICE_CONTACTS];
	std::string m_RemoteLoggerServiceContacts[MAX_SERVICE_CONTACTS];
	std::string m_publicIP;
	time_t m_time {};

	CstiCallList *m_poCallList;
	CstiContactList *m_poContactList;
#ifdef stiFAVORITES
	// TODO: use shared_ptr to manage this since there's code to clean up in multiple places
	SstiFavoriteList *m_poFavoriteList;
#endif
	CstiPhoneNumberList * m_poPhoneNumberList{nullptr};
	CstiUserAccountInfo * m_poUserAccountInfo;
	CstiDirectoryResolveResult * m_poDirectoryResolveResult;

	std::vector<SstiSettingItem> m_settingList;

	std::list <std::string> m_MissingSettingList;

	std::vector <SRelayLanguage> m_RelayLanguageList;

	std::vector <SRingGroupInfo> m_RingGroupInfoList;

	std::vector <SRegistrationInfo> m_RegistrationInfoList;

	std::vector <SAgreement> m_AgreementList;

	std::vector <SScreenSaverInfo> m_ScreenSaverInfoList;
}; // end class CstiCoreResponse


/**
 * \brief Get the response value from the response object
 */
CstiCoreResponse::EResponse CstiCoreResponse::ResponseGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::ResponseGet);

	return (m_eResponse);
} // end CstiCoreResponse::ResponseGet

/**
 * \brief Get the error value from the response object
 */
CstiCoreResponse::ECoreError CstiCoreResponse::ErrorGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::ErrorGet);

	return (m_eError);
} // end CstiCoreResponse::ErrorGet

/**
 * \brief Set the error value in the response object
 */
EstiResult CstiCoreResponse::ErrorSet (const CstiCoreResponse::ECoreError eError)
{
	EstiResult eResult = estiOK;
	stiLOG_ENTRY_NAME (CstiCoreResponse::ErrorSet);

	m_eError = eError;

	return (eResult);
} // end CstiCoreResponse::ErrorSet

/**
 * \brief Get the call list from the response object
 */
CstiCallList * CstiCoreResponse::CallListGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::CallListGet);

	return (m_poCallList);

} // end CstiCoreResponse::CallListGet

/**
 * \brief Set the call list in the response object
 */
void CstiCoreResponse::CallListSet (
	CstiCallList * poCallList)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::CallListSet);

	m_poCallList = poCallList;

} // end CstiCoreResponse::CallListSet


/**
 * \brief Get the contact list from the response object
 */
CstiContactList * CstiCoreResponse::ContactListGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::ContactListGet);

	return (m_poContactList);

} // end CstiCoreResponse::ContactListGet


/**
 * \brief Set the contact list in the response object
 */
void CstiCoreResponse::ContactListSet (
	CstiContactList * poContactList)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::ContactListSet);

	m_poContactList = poContactList;

} // end CstiCoreResponse::ContactListSet


#ifdef stiFAVORITES
/**
 * \brief Get the favorite list from the response object
 */
SstiFavoriteList * CstiCoreResponse::FavoriteListGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::FavoriteListGet);

	return (m_poFavoriteList);

} // end CstiCoreResponse::FavoriteListGet


/**
 * \brief Set the favorite list in the response object
 */
void CstiCoreResponse::FavoriteListSet (
	SstiFavoriteList * poFavoriteList)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::FavoriteListSet);

	m_poFavoriteList = poFavoriteList;

} // end CstiCoreResponse::FavoriteListSet
#endif

/**
 * \brief UNUSED: Get the phone number list from the response object
 */
CstiPhoneNumberList * CstiCoreResponse::PhoneNumberListGet ()
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::PhoneNumberListGet);

	return (m_poPhoneNumberList);

} // end CstiCoreResponse::PhoneNumberListGet

/**
 * \brief Set the Phone number list
 */
void CstiCoreResponse::PhoneNumberListSet (
	CstiPhoneNumberList * poPhoneNumberList)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::PhoneNumberListSet);

	m_poPhoneNumberList = poPhoneNumberList;

} // end CstiCoreResponse::CallListSet

/**
 * \brief Gets the user account info structure from the response
 */
CstiUserAccountInfo * CstiCoreResponse::UserAccountInfoGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::UserAccountInfoGet);

	return (m_poUserAccountInfo);
} // end CstiCoreResponse::UserAccountInfoGet

/**
 * \brief Sets the user account info structure in the response
 */
void CstiCoreResponse::UserAccountInfoSet (
	CstiUserAccountInfo * poUserAccountInfo)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::UserAccountInfoSet);

	m_poUserAccountInfo = poUserAccountInfo;
} // end CstiCoreResponse::UserAccountInfoSet

/**
 * \brief Gets the directory resolve result from the response
 */
CstiDirectoryResolveResult * CstiCoreResponse::DirectoryResolveResultGet () const
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::DirectoryResolveResultGet);

	return (m_poDirectoryResolveResult);
} // end CstiCoreResponse::DirectoryResolveResultGet

/**
 * \brief Sets the directory resolve result in the response
 */
void CstiCoreResponse::DirectoryResolveResultSet (
	CstiDirectoryResolveResult * poDirectoryResolveResult)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::DirectoryResolveResultSet);

	m_poDirectoryResolveResult = poDirectoryResolveResult;
} // end CstiCoreResponse::DirectoryResolveResultSet

#endif // CSTICORERESPONSE_H
// end file CstiCoreResponse.h
