/*!
 * \file CstiConferenceRequest.cpp
 * \brief Implementation of the CstiConferenceRequest class.
 *
 * Constructs XML Conference Request files to send to the Conference Service.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "ixml.h"
#include "CstiConferenceRequest.h"
#include "stiCoreRequestDefines.h"     // for the TAG_... defines, etc.
#include "CstiConferenceResponse.h"
#include <cstdlib>
#include <ctime>

#ifdef stiLINUX
using namespace std;
#endif

//
// Constant strings representing the Request tag names
//
const char CNFRQ_ConferenceRoomCreate[] = "ConferenceRoomCreate";
const char CNFRQ_ConferenceRoomStatusGet[] = "ConferenceRoomStatusGet";

//
// Constant strings representing the sub elements and attributes of the request
//
const char CNF_TAG_ENCRYPTION_ENABLED[] = "EncryptionEnabled";
const char CNF_TAG_CONFERENCE_PUBLIC_ID[] = "ConferencePublicId";
const char CNF_TAG_CONFERENCE_TYPE[] = "ConferenceType";


////////////////////////////////////////////////////////////////////////////////
//
// MessageRequest functions
//  NOTE: see doxygen comments in the CstiConferenceRequest.h file
//

////////////////////////////////////////////////////////////////////////////////
// Example XML for ConferenceRoomCreate
// <ConferenceRequest Id="0001" ClientToken="">
//   <ConferenceRoomCreate>
//     <EncryptionEnabled>false</EncryptionEnabled>
//   <ConferenceRoomCreate>
// </ConferenceRequest>
//
int CstiConferenceRequest::ConferenceRoomCreate (bool bEncrypted, ConferenceRoomType type)
{
	std::string roomType;
	auto hResult = AddRequest (CNFRQ_ConferenceRoomCreate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiConferenceResponse::eCONFERENCE_ROOM_CREATE_RESULT);

	hResult = AddSubElementToRequest(CNF_TAG_ENCRYPTION_ENABLED, bEncrypted ? "true" : "false");
	stiTESTRESULT ();
	
	switch (type)
	{
		case ConferenceRoomType::Generic:
		{
			break;
		}
		case ConferenceRoomType::Dhvi:
		{
			roomType = "DeafHearingVI";
			break;
		}
	}
	
	if (!roomType.empty ())
	{
		hResult =  AddSubElementToRequest(CNF_TAG_CONFERENCE_TYPE, roomType.c_str ());
		stiTESTRESULT ();
	}

STI_BAIL:
	
	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for ConferenceRoomStatusGet
// <ConferenceRequest Id="0001" ClientToken="">
//   <ConferenceRoomStatusGet>
//     <ConferencePublicId>xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx</ConferencePublicId>
//   <ConferenceRoomStatusGet>
// </ConferenceRequest>
//
int CstiConferenceRequest::ConferenceRoomStatusGet (const char *pszConferencePublicId)
{
	auto hResult = AddRequest (CNFRQ_ConferenceRoomStatusGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiConferenceResponse::eCONFERENCE_ROOM_STATUS_GET_RESULT);

	hResult = AddSubElementToRequest(CNF_TAG_CONFERENCE_PUBLIC_ID, pszConferencePublicId);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


const char *CstiConferenceRequest::RequestTypeGet()
{
	return CNFRQ_ConferenceRequest;
}
