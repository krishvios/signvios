/*!
 *  \file CstiRingGroupMgrResponse.cpp
 *  \brief The purpose of the RingGroupManager object is to present a
 * 		Ring Group Manager response to the UI.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiRingGroupMgrResponse.h"


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
/*! \brief Constructor
 *
 * This is the default constructor for the CstiRingGroupMgrResponse class.
 *
 * \return None
 */
CstiRingGroupMgrResponse::CstiRingGroupMgrResponse (
	const char *pszMemberNumber,
	unsigned int unRequestId,
	unsigned int unCoreErrorCode)
:
	m_MemberNumber (pszMemberNumber),
	m_unRequestId (unRequestId),
	m_unCoreErrorCode (unCoreErrorCode)
{
}


// end CstiRingGroupMgrResponse.cpp
