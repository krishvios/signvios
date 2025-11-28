////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiStateNotifyResponse
//
//	File Name:	CstiStateNotifyResponse.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		ACTION - enter description
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISTATENOTIFYRESPONSE_H
#define CSTISTATENOTIFYRESPONSE_H

//
// Includes
//
#include <cstring>
#include "stiSVX.h"
#include "CstiVPServiceResponse.h"
#include "VPServiceRequestContext.h"
#include "CstiVPServiceRequest.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiStateNotifyResponse : public CstiVPServiceResponse
{
public:
	enum EResponse
	{
		eRESPONSE_UNKNOWN,
		eRESPONSE_ERROR,
		eALLOW_NUMBER_CHANGE,
		//eBLOCKED_LIST_CHANGED,
		eCALL_LIST_CHANGED,
		eCLIENT_REAUTHENTICATE,
		eEMERGENCY_PROVISION_STATUS_CHANGE,
		eEMERGENCY_ADDRESS_UPDATE_VERIFY,
		eGROUP_CHANGED,
		eHEARTBEAT_RESULT,
		eLOGGED_OUT,
		eLOGIN,
		eNEW_MESSAGE_RECEIVED,
		eMESSAGE_LIST_CHANGED,
		ePASSWORD_CHANGED,
		ePUBLIC_IP_ADDRESS_CHANGED,
		ePUBLIC_IP_SETTINGS_CHANGED,
		ePUBLIC_IP_GET_RESULT,
		ePHONE_SETTING_CHANGED,
		eREBOOT_PHONE,
		eROUTER_REVALIDATE,
		eSERVICE_CONTACT_RESULT,
#if 0
		eSIGNMAIL_LIST_CHANGED,
#endif
		eTIME_CHANGED,
		eTIME_GET_RESULT,
		eUPDATE_VERSION_CHECK_FORCE,
		eUSER_ACCOUNT_INFO_SETTING_CHANGED,
		eUSER_DISASSOCIATED,
		eUSER_SETTING_CHANGED,
#if 0
		eSERVICE_MASK_CHANGED,
#endif
		eSET_CURRENT_USER_PRIMARY,
		eDISPLAY_PROVIDER_AGREEMENT,
		eDISPLAY_REGISTRATION_WIZARD,
		eUSER_INTERFACE_GROUP_CHANGED,
		eCALL_LIST_ITEM_ADD,
		eCALL_LIST_ITEM_EDIT,
		eCALL_LIST_ITEM_REMOVE,
		eRING_GROUP_CREATED,
		eRING_GROUP_INVITE,
		eRING_GROUP_REMOVED,
		eRING_GROUP_INFO_CHANGED,
		eREGISTRATION_INFO_CHANGED,
		eDISPLAY_RING_GROUP_WIZARD,
		eAGREEMENT_CHANGED,
		eUSER_NOTIFICATION,
		ePROFILE_IMAGE_CHANGED,
		eDIAGNOSTICS_GET,
		eFAVORITE_LIST_CHANGED,
		eFAVORITE_ADDED,
		eFAVORITE_REMOVED,
		eCLIENT_REREGISTER
	}; // end EResponse

	enum EStateNotifyError
	{
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

		// High-Level Server Errors (message not processed)
		eDATABASE_CONNECTION_ERROR = 0xFFFFFFFB,// Database connection error
		eXML_VALIDATION_ERROR = 0xFFFFFFFC,		// XML Validation Error
		eXML_FORMAT_ERROR = 0xFFFFFFFD,			// XML Format Error
		eGENERIC_SERVER_ERROR = 0xFFFFFFFE,		// General Server Error (exception, etc.)
		eUNKNOWN_ERROR = 0xFFFFFFFF
	}; // end EStateNotifyError

	enum ELogoutReason
	{
		eREASON_UNKNOWN = 0,
		eLOGGED_IN_SAME_PHONE = 1,				// A New User has logged in to the same phone
		eLOGGED_IN_NEW_PHONE = 2,				// User has logged in to another phone
	}; // end ELogoutReason

	CstiStateNotifyResponse (
		CstiVPServiceRequest *request,
		EResponse eResponse,
		EstiResult eResponseResult,
		EStateNotifyError eError,
		const char *pszError);

	CstiStateNotifyResponse (const CstiStateNotifyResponse &) = delete;
	CstiStateNotifyResponse (CstiStateNotifyResponse &&) = delete;
	CstiStateNotifyResponse &operator= (const CstiStateNotifyResponse &) = delete;
	CstiStateNotifyResponse &operator= (CstiStateNotifyResponse &&) = delete;

	inline EResponse ResponseGet () const;
	inline EStateNotifyError ErrorGet () const;

	time_t timeGet()
	{
		return m_time;
	}

	void timeSet(time_t time)
	{
		m_time = time;
	}

protected:

	~CstiStateNotifyResponse () override = default;

private:

	EResponse m_eResponse;
	EStateNotifyError m_eError;

	time_t m_time {};
}; // end class CstiStateNotifyResponse


////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyResponse::ResponseGet
//
// Description:
//
// Abstract:
//
// Returns:
//
CstiStateNotifyResponse::EResponse CstiStateNotifyResponse::ResponseGet () const
{
	stiLOG_ENTRY_NAME (CstiStateNotifyResponse::ResponseGet);

	return (m_eResponse);
} // end CstiStateNotifyResponse::ResponseGet

////////////////////////////////////////////////////////////////////////////////
//; CstiStateNotifyResponse::ErrorGet
//
// Description:
//
// Abstract:
//
// Returns:
//
CstiStateNotifyResponse::EStateNotifyError CstiStateNotifyResponse::ErrorGet () const
{
	stiLOG_ENTRY_NAME (CstiStateNotifyResponse::ErrorGet);

	return (m_eError);
} // end CstiStateNotifyResponse::ErrorGet


#endif // CSTISTATENOTIFYRESPONSE_H
// end file CstiStateNotifyResponse.h
