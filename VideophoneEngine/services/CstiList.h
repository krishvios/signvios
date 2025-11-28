/*!
 * \file CstiList.h
 * \brief See CstiList.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 – 2018 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

//
// Includes
//
#include "stiDefs.h"
#include "CstiListItem.h"

#include <vector>

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

//
// Class Declaration
//
class CstiList
{
public:

	//
	// Public Enumerations
	//
	/*!
	 * \brief Enumerates the sort direction
	 */
	enum class SortDirection
	{
		DESCENDING,		//!< Descending order
		ASCENDING,			//!< Ascending order
	};

	//
	// Public Methods
	//
	CstiList () = default;
	virtual ~CstiList () = default;
	
	CstiList (const CstiList &) = delete;
	CstiList (CstiList &&) = delete;
	CstiList &operator= (const CstiList &) = delete;
	CstiList &operator= (CstiList &&) = delete;

	virtual stiHResult ItemAdd (
		const CstiListItemSharedPtr &listItem);

	virtual void ListFree ();
	
	CstiListItemSharedPtr ItemGet (
		unsigned int nItem) const;
	
	virtual stiHResult ItemRemove (
		unsigned int nItem);
    
	virtual CstiListItemSharedPtr ItemDetach (
		unsigned int nItem);

	virtual unsigned int CountGet () const;
	
	virtual stiHResult CountSet (
		unsigned int unCount);
	
	virtual SortDirection SortDirectionGet () const;
	
	virtual stiHResult SortDirectionSet (SortDirection eSortDirection);
	
	virtual bool UniqueGet () const;
	
	virtual stiHResult UniqueSet (bool bUnique);

protected:

	//
	// Protected Members
	//
	// NOTE: it's possible for this to not be allocated, even if count is set
	std::unique_ptr<std::vector<CstiListItemSharedPtr>> m_listItems;

	unsigned int m_count = 0;

	SortDirection m_sortDirection = SortDirection::DESCENDING;
	bool m_unique = false;

}; // end class CstiList

// end file CstiList.h
