/*!
 * \file CstiConferenceResponse.h
 * \brief Definition of the CstiConferenceResponse class.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef CSTICONFERENCERESPONSE_H
#define CSTICONFERENCERESPONSE_H

//
// Includes
//
#include "stiSVX.h"
#include "CstiVPServiceResponse.h"
#include <list>

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

class CstiConferenceResponse : public CstiVPServiceResponse
{
public:
	enum EResponse
	{
		eRESPONSE_UNKNOWN,
		eRESPONSE_ERROR,
		eCONFERENCE_ROOM_CREATE_RESULT,
		eCONFERENCE_ROOM_STATUS_GET_RESULT,
	}; // end EResponse


	enum EResponseError
	{
		// Errors occurring during message processing
		eNO_ERROR = 0,							// No Error
		eNOT_LOGGED_IN = 1,						// Not Logged In
		eRESPONSE_INSUFFICIENT_RESOURCES = 2,
		eRESPONSE_CONFERENCE_ROOM_CREATION_FAILED = 3,
		eRESPONSE_CONFERENCE_ROOM_NOT_FOUND = 4,
		eRESPONSE_CONFERENCE_ROOM_STATUS_UNAVAILABLE = 5,
		eCONFERENCE_SERVER_NOT_REACHABLE = 14,
		eSQL_SERVER_NOT_REACHABLE = 39,

		// High-Level Server Errors (message not processed)
		eDATABASE_CONNECTION_ERROR = 0xFFFFFFFB,// Database connection error
		eXML_VALIDATION_ERROR = 0xFFFFFFFC,		// XML Validation Error
		eXML_FORMAT_ERROR = 0xFFFFFFFD,			// XML Format Error
		eGENERIC_SERVER_ERROR = 0xFFFFFFFE,		// General Server Error (exception, etc.)
		eUNKNOWN_ERROR = 0xFFFFFFFF
	}; // end EResponseError

	CstiConferenceResponse (
		CstiVPServiceRequest *request,
		EResponse eResponse,
		EstiResult eResponseResult,
		EResponseError eError,
		const char *pszError);
	
	CstiConferenceResponse (const CstiConferenceResponse &) = delete;
	CstiConferenceResponse (CstiConferenceResponse &&) = delete;
	CstiConferenceResponse &operator= (const CstiConferenceResponse &) = delete;
	CstiConferenceResponse &operator= (CstiConferenceResponse &&) = delete;

	inline EResponse ResponseGet () const;
	inline EResponseError ErrorGet () const;
	bool AddAllowedGet () const;
	void AddAllowedSet (bool bSet);
	int ActiveParticipantsGet () const;
	void ActiveParticipantsSet (int nCount);
	int AllowedParticipantsGet () const;
	void AllowedParticipantsSet (int nCount);
	const std::list<SstiGvcParticipant> * ParticipantListGet () const;
	stiHResult ParticipantListAddItem (SstiGvcParticipant *pstParticipant);

protected:

	~CstiConferenceResponse () override = default;

private:
	
	EResponse m_eResponse;
	EResponseError m_eError;
	bool m_bAddAllowed;
	int m_nActiveParticipants;
	int m_nAllowedParticipants;
	std::list<SstiGvcParticipant> m_ParticipantList;

}; // end class CstiConferenceResponse


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceResponse::ResponseGet
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiConferenceResponse::EResponse CstiConferenceResponse::ResponseGet () const
{
	stiLOG_ENTRY_NAME (CstiConferenceResponse::ResponseGet);

	return (m_eResponse);
} // end CstiConferenceResponse::ResponseGet

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceResponse::ErrorGet
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiConferenceResponse::EResponseError CstiConferenceResponse::ErrorGet () const
{
	stiLOG_ENTRY_NAME (CstiConferenceResponse::ErrorGet);

	return (m_eError);
} // end CstiConferenceResponse::ErrorGet


#endif // CSTICONFERENCERESPONSE_H
// end file CstiConferenceResponse.h
