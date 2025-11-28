/*!
 * \file CstiList.cpp
 * \brief Encapsulates the Call List - a collection of objects
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 – 2018 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//

#include "stiSVX.h"
#include "CstiList.h"
#include "stiTools.h"

//:-----------------------------------------------------------------------------
//:	Construction / Destruction
//:-----------------------------------------------------------------------------


void CstiList::ListFree ()
{
	if (m_listItems)
	{
		m_listItems->clear ();
		m_listItems.reset ();
	}
	m_count = 0;
}

//:-----------------------------------------------------------------------------
//:	Public Methods
//:-----------------------------------------------------------------------------

/*!
 *  \brief Returns the indicated item, or unset pointer
 */
CstiListItemSharedPtr CstiList::ItemGet (
	unsigned int unItem) const
{
	stiLOG_ENTRY_NAME (CstiList::ItemGet);

	CstiListItemSharedPtr returnListItem;

	if (m_listItems && unItem < m_listItems->size ())
	{
		returnListItem = (*m_listItems)[unItem];
	}

	return returnListItem;
}

/*!
 *  \brief Removes an item from the list
 *
 * \return stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
 */
stiHResult CstiList::ItemRemove (
	unsigned int nItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiList::ItemRemove);

	if (m_listItems && nItem < m_listItems->size ())
	{
		m_listItems->erase (m_listItems->begin () + nItem);
		
		--m_count;
	}
	
	return hResult;
}

/*!
 *  \brief Combination of ItemGet and ItemRemove, without the ItemFree.
 */
CstiListItemSharedPtr CstiList::ItemDetach (
	unsigned int nItem)
{
	stiLOG_ENTRY_NAME (CstiList::ItemRemove);

	// Can we get this item?
	CstiListItemSharedPtr item = ItemGet (nItem);
	
	if (item)
	{
		ItemRemove (nItem);
		--m_count;
	}
	
	return item;
}


//:-----------------------------------------------------------------------------
//:	Protected methods
//:-----------------------------------------------------------------------------

/*!
 *  \brief Adds an item to the array of CallListItems
 *
 *  \return stiRESULT_SUCCESS
 */
stiHResult CstiList::ItemAdd (
	const CstiListItemSharedPtr &listItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiList::ItemAdd);

	if (!m_listItems)
	{
		m_listItems = sci::make_unique<std::vector<CstiListItemSharedPtr>> ();

		// This is the strange part of resetting the count when reallocating the array
		m_count = 0;
	}

	m_listItems->push_back (listItem);
	++m_count;

	return hResult;
}

/*!
 *  \brief get size of the list
 *
 * NOTE: m_count is separate than the list size
 *
 *  \return size
 */
unsigned int CstiList::CountGet () const
{
	stiLOG_ENTRY_NAME (CstiList::CountGet);

	return m_count;
}

/*!
 *  \brief set size of the list
 *
 * NOTE: setting this sets a count that may be different from the
 * underlying size, with the intent of retrieving that count later
 *
 *  \return stiRESULT_SUCCESS
 */
stiHResult CstiList::CountSet (
	unsigned int unCount)
{
	stiLOG_ENTRY_NAME (CstiList::CountSet);

	m_count = unCount;

	return stiRESULT_SUCCESS;
}

/*!
 *  \brief SortDirection getter
 */
CstiList::SortDirection CstiList::SortDirectionGet () const
{
	stiLOG_ENTRY_NAME (CstiList::SortDirectionGet);

	return m_sortDirection;

}

/*!
 *  \brief SortDirection setter
 */
stiHResult CstiList::SortDirectionSet (
	SortDirection sortDirection)
{
	stiLOG_ENTRY_NAME (CstiList::SortDirectionSet);

	m_sortDirection = sortDirection;

	return (stiRESULT_SUCCESS);
	
}

/*!
 *  \brief unique getter
 *
 *  Returns whether the list retrieves unique items only (true) or
 *  retrieves all items (false)
 */
bool CstiList::UniqueGet () const
{
	stiLOG_ENTRY_NAME (CstiList::UniqueGet);

	return (m_unique);

}

/*!
 *  \brief Sets the m_unique flag (see description above)
 *
 *  \return stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
 */
stiHResult CstiList::UniqueSet (
	bool unique)
{
	stiLOG_ENTRY_NAME (CstiList::UniqueSet);

	m_unique = unique;

	return (stiRESULT_SUCCESS);

}

