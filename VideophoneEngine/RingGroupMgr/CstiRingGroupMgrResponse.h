/*!
 * \file CstiRinGroupMgrResponse.h
 * \brief See CstiRingGroupMgrResponse.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIRINGGROUPMGRRESPONSE_H
#define CSTIRINGGROUPMGRRESPONSE_H

//
// Includes
//
#include "IstiRingGroupMgrResponse.h"
#include <cstdlib>
#include <string>


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
class CstiRingGroupMgrResponse: public IstiRingGroupMgrResponse
{
public:
	CstiRingGroupMgrResponse (const char *pszMemberNumber, unsigned int unRequestId, unsigned int unCoreErrorCode);

	~CstiRingGroupMgrResponse () override = default;

	const char *MemberNumberGet () override
		{ return m_MemberNumber.c_str(); }


	unsigned int RequestIdGet () override
		{ return m_unRequestId; }

	unsigned int CoreErrorCodeGet () override
		{ return m_unCoreErrorCode; }

private:
	std::string m_MemberNumber;
	unsigned int m_unRequestId;
	unsigned int m_unCoreErrorCode;
};


#endif // CSTIRINGGROUPMGRRESPONSE_H
