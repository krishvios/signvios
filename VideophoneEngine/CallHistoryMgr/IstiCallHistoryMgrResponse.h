/*!
 *  \file IstiCallHistoryMgrResponse.h
 *  \brief The purpose of the CallHistoryMgrResponse object is to present
		information to the UI about changes to the Call Lists.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef ISTICALLHISTORYMGRRESPONSE_H
#define ISTICALLHISTORYMGRRESPONSE_H

//
// Includes
//
#include "CstiCallList.h"
#include "CstiCallHistoryItem.h"

/*!
 * \brief Call History Manager Response
 *
 */
class IstiCallHistoryMgrResponse
{
public:

	IstiCallHistoryMgrResponse (const IstiCallHistoryMgrResponse &other) = delete;
	IstiCallHistoryMgrResponse (IstiCallHistoryMgrResponse &&other) = delete;
	IstiCallHistoryMgrResponse &operator= (const IstiCallHistoryMgrResponse &other) = delete;
	IstiCallHistoryMgrResponse &operator= (IstiCallHistoryMgrResponse &&other) = delete;

	/*!
	 * \brief Returns the type of call list this response refers to.
	 *
	 * \return The Call List type
	 */
	virtual CstiCallList::EType CallListTypeGet () = 0;

	/*!
	 * \brief Returns a smart pointer to the item referred to in this response
	 *
	 * \return A smart pointer to the item.
	 */
	virtual CstiCallHistoryItemSharedPtr ItemGet () = 0;

	/*!
	 * \brief Returns the request id
	 *
	 * \return The request id associated with this item response.
	 */
	virtual unsigned int RequestIdGet () = 0;

protected:

	IstiCallHistoryMgrResponse() = default;
	virtual ~IstiCallHistoryMgrResponse() = default;

};

#endif // ISTICALLHISTORYMGRRESPONSE_H
