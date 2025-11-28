/*!
 * \file CstiConferenceRequest.h
 * \brief Definition of the CstiConferenceRequest class.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
 
#ifndef CSTICONFERENCEREQUEST_H
#define CSTICONFERENCEREQUEST_H


#include "stiSVX.h"
#include "CstiVPServiceRequest.h"
#include "CstiItemId.h"

//
// Constants
//

//
// Forward Declarations
//


/**
*/
class CstiConferenceRequest : public CstiVPServiceRequest
{
public:

	CstiConferenceRequest() = default;
	~CstiConferenceRequest() override = default;

	CstiConferenceRequest (const CstiConferenceRequest &) = delete;
	CstiConferenceRequest (CstiConferenceRequest &&) = delete;
	CstiConferenceRequest &operator= (const CstiConferenceRequest &) = delete;
	CstiConferenceRequest &operator= (CstiConferenceRequest &&) = delete;
	
	enum class ConferenceRoomType
	{
		Generic, // This is a generic conference room
		Dhvi // This room is for DHVI calls
	};

	////////////////////////////////////////////////////////////////////////////////
	// The following APIs create each of the supported message requests.
	////////////////////////////////////////////////////////////////////////////////
	int ConferenceRoomCreate (bool bEncrypted, ConferenceRoomType roomType);

	int ConferenceRoomStatusGet (const char *pszConferencePublicId);


////////////////////////////////////////////////////////////////////////////////
// Protected APIs and variables internal to creating message requests
////////////////////////////////////////////////////////////////////////////////

private:

	const char *RequestTypeGet() override;
};


#endif // CSTICONFERENCEREQUEST_H
