/*!
 *  \file IstiRingGroupMgrResponse.h
 *  \brief The purpose of the RingGroupManager object is to present a
 * 		ring group manager response to the UI.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef ISTIRINGGROUPMGRRESPONSE_H
#define ISTIRINGGROUPMGRRESPONSE_H

//
// Includes
//

/*! 
 * \brief Ring Group Manager Response 
 *  
 */
class IstiRingGroupMgrResponse
{
public:
	
	virtual ~IstiRingGroupMgrResponse() = default;

	/*!
	 * \brief Returns the item id
	 * 
	 * \return A pointer to the item identifier.
	 */
	virtual const char *MemberNumberGet () = 0;

	/*!
	 * \brief Returns the request id
	 * 
	 * \return The request id associated with this item response.
	 */
	virtual unsigned int RequestIdGet () = 0;

	/*!
	 * \brief Returns the core error code
	 * 
	 * \return The core error code associated with this item response.
	 */
	virtual unsigned int CoreErrorCodeGet () = 0;
};

#endif // ISTIRINGGROUPMGRRESPONSE_H
