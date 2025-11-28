/*!
 *  \file CstiCallHistoryMgrResponse.cpp
 *  \brief The purpose of the CallHistoryMgrResponse object is to present
 * 		information to the UI about changes to the Call Lists.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiCallHistoryMgrResponse.h"


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
 * This is the default constructor for the CstiCallHistoryMgrResponse class.
 *
 * \return None
 */
CstiCallHistoryMgrResponse::CstiCallHistoryMgrResponse (
	CstiCallList::EType eType,
	const CstiCallHistoryItemSharedPtr &spItem,
	unsigned int requestId)
:
	m_eCallListType(eType),
	m_spItem (spItem),
	m_requestId (requestId)
{
}


// end CstiCallHistoryMgrResponse.cpp
