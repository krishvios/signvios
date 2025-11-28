/*!
 *  \file CstiBlockListMgrResponse.cpp
 *  \brief The purpose of the BlockListManager object is to present a
 * 		block list manager response to the UI.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

//
// Includes
//
#include "CstiBlockListMgrResponse.h"


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
 * This is the default constructor for the CstiBlockListMgrResponse class.
 *
 * \return None
 */
CstiBlockListMgrResponse::CstiBlockListMgrResponse (
	const std::string &itemId,
	unsigned int requestId)
:
	m_ItemId (itemId),
	m_requestId (requestId)
{
}


// end CstiBlockListMgrResponse.cpp
