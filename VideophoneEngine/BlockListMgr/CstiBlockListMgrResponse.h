/*!
 * \file CstiBlockListMgrResponse.h
 * \brief See CstiBlockListMgrResponse.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIBLOCKLISTMGRRESPONSE_H
#define CSTIBLOCKLISTMGRRESPONSE_H

//
// Includes
//
#include "IstiBlockListMgrResponse.h"
#include "CstiBlockListItem.h"
#include <cstdlib>


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
class CstiBlockListMgrResponse: public IstiBlockListMgrResponse
{
public:
	CstiBlockListMgrResponse (const std::string &itemId, unsigned int requestId);

	~CstiBlockListMgrResponse () override = default;

	CstiBlockListMgrResponse (const CstiBlockListMgrResponse &other) = delete;
	CstiBlockListMgrResponse (CstiBlockListMgrResponse &&other) = delete;
	CstiBlockListMgrResponse &operator= (const CstiBlockListMgrResponse &other) = delete;
	CstiBlockListMgrResponse &operator= (CstiBlockListMgrResponse &&other) = delete;

	std::string ItemIdGet () override { return m_ItemId; }

	unsigned int RequestIdGet () override
		{ return m_requestId; }

private:
	std::string m_ItemId;
	unsigned int m_requestId;
};


#endif // CSTIBLOCKLISTMGRRESPONSE_H
