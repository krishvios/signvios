/*!
 *  \file IstiBlockListMgrResponse.h
 *  \brief The purpose of the BlockListManager object is to present a
 * 		block list manager response to the UI.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef ISTIBLOCKLISTMGRRESPONSE_H
#define ISTIBLOCKLISTMGRRESPONSE_H

//
// Includes
//
#include <string>

/*! 
 * \brief Block List Manager Response 
 *  
 */
class IstiBlockListMgrResponse
{
public:
	
	IstiBlockListMgrResponse (const IstiBlockListMgrResponse &other) = delete;
	IstiBlockListMgrResponse (IstiBlockListMgrResponse &&other) = delete;
	IstiBlockListMgrResponse &operator= (const IstiBlockListMgrResponse &other) = delete;
	IstiBlockListMgrResponse &operator= (IstiBlockListMgrResponse &&other) = delete;

	/*!
	 * \brief Returns the item id
	 * 
	 * \return A pointer to the item identifier.
	 */
	virtual std::string ItemIdGet () = 0;

	/*!
	 * \brief Returns the request id
	 * 
	 * \return The request id associated with this item response.
	 */
	virtual unsigned int RequestIdGet () = 0;

protected:

	IstiBlockListMgrResponse () = default;
	virtual ~IstiBlockListMgrResponse() = default;

};

#endif // ISTIBLOCKLISTMGRRESPONSE_H
