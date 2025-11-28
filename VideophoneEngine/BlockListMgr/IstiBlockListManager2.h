/*!
 *
 *\file IstiBlockListManager2.h
 *\brief Declaration of the BlockListManager interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTIBLOCKLISTMANAGER2_H
#define ISTIBLOCKLISTMANAGER2_H

/*!
 * \brief This is a simpler interface that is passed to objects that only need to check to see if a call is blocked.
 *  
 */
class IstiBlockListManager2
{
public:

	IstiBlockListManager2 (const IstiBlockListManager2 &other) = delete;
	IstiBlockListManager2 (IstiBlockListManager2 &&other) = delete;
	IstiBlockListManager2 &operator= (const IstiBlockListManager2 &other) = delete;
	IstiBlockListManager2 &operator= (IstiBlockListManager2 &&other) = delete;

	/*!
	 * \brief Test against phone number to see if it's in the block list.
	 * 
	 * \param pszNumber The number of the caller to check
	 * 
	 * \return True if the caller identified by pszNumber is blocked. 
	 */
	virtual bool CallBlocked (
		const char *pszNumber) = 0;

protected:

	IstiBlockListManager2 () = default;
	virtual ~IstiBlockListManager2() = default;
};

#endif // ISTIBLOCKLISTMANAGER2_H
