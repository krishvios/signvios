/*!
 * \file CstiCallHistoryMgrResponse.h
 * \brief See CstiCallHistoryMgrResponse.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTICALLHISTORYMGRRESPONSE_H
#define CSTICALLHISTORYMGRRESPONSE_H

//
// Includes
//
#include "IstiCallHistoryMgrResponse.h"


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
class CstiCallHistoryMgrResponse: public IstiCallHistoryMgrResponse
{
public:
	CstiCallHistoryMgrResponse (CstiCallList::EType eType, const CstiCallHistoryItemSharedPtr &, unsigned int requestId);

	CstiCallHistoryMgrResponse (const CstiCallHistoryMgrResponse &other) = delete;
	CstiCallHistoryMgrResponse (CstiCallHistoryMgrResponse &&other) = delete;
	CstiCallHistoryMgrResponse &operator= (const CstiCallHistoryMgrResponse &other) = delete;
	CstiCallHistoryMgrResponse &operator= (CstiCallHistoryMgrResponse &&other) = delete;

	~CstiCallHistoryMgrResponse () override = default;

	CstiCallList::EType CallListTypeGet() override
		{ return m_eCallListType; }

	CstiCallHistoryItemSharedPtr ItemGet() override
		{ return m_spItem; }


	unsigned int RequestIdGet () override
		{ return m_requestId; }


private:
	CstiCallList::EType m_eCallListType;
	CstiCallHistoryItemSharedPtr m_spItem;
	unsigned int m_requestId;
};


#endif // CSTICALLHISTORYMGRRESPONSE_H
