////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiCallList
//
//	File Name:	CstiCallList.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Encapsulates the Call List - a collection of CstiCallListItem objects
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CstiCallList.h"

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
// Locals
//

//
// Function Declarations
//

//:-----------------------------------------------------------------------------
//:	Construction / Destruction
//:-----------------------------------------------------------------------------


//:-----------------------------------------------------------------------------
//:	Public Methods
//:-----------------------------------------------------------------------------

/*!
 *  \brief Returns the indicated item, or empty pointer
 */
CstiCallListItemSharedPtr CstiCallList::ItemGet (
	unsigned int unItem) const
{
	stiLOG_ENTRY_NAME (CstiCallList::ItemGet);

	CstiCallListItemSharedPtr item;

	// Was a valid index requested?
	if (m_listItems && unItem < m_listItems->size ())
	{
		item = std::static_pointer_cast<CstiCallListItem> ((*m_listItems)[unItem]);
	}

	return item;
}

/*!
 *  \brief Adds an item to the array of CallListItems
 *
 *  \return stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
 */
stiHResult CstiCallList::ItemAdd (
	const CstiCallListItemSharedPtr &callListItem)
{
	stiLOG_ENTRY_NAME (CstiCallList::ItemAdd);
	stiHResult hResult = stiRESULT_ERROR;

	hResult = CstiList::ItemAdd (callListItem);

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiList::TypeGet
//
// Description: Returns the list type
//
// Abstract:
//
// Returns:
//	CstiList::EType
//
CstiCallList::EType CstiCallList::TypeGet () const
{
	stiLOG_ENTRY_NAME (CstiCallList::TypeGet);

	return (m_eType);
	
} // end CstiCallList::TypeGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::TypeSet
//
// Description: Sets the list type
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallList::TypeSet (
	CstiCallList::EType eType)
{
	stiLOG_ENTRY_NAME (CstiCallList::TypeSet);

	m_eType = eType;

	return (stiRESULT_SUCCESS);
	
} // end CstiCallList::TypeSet

////////////////////////////////////////////////////////////////////////////////
//; CstiList::SortFieldGet
//
// Description: Returns the field by which the list is sorted
//
// Abstract:
//
// Returns:
//	CstiList::ESortField
//
CstiCallList::ESortField CstiCallList::SortFieldGet () const
{
	stiLOG_ENTRY_NAME (CstiCallList::SortFieldGet);

	return (m_eSortField);
} // end CstiCallList::SortFieldGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::SortFieldSet
//
// Description: Sets the field on which the data is sorted.
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallList::SortFieldSet (
	CstiCallList::ESortField eSortField)
{
	stiLOG_ENTRY_NAME (CstiCallList::SortFieldSet);

	m_eSortField = eSortField;

	return (stiRESULT_SUCCESS);
	
} // end CstiCallList::SortFieldSet


////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::BaseIndexGet
//
// Description: Returns the base index of the list items
//
// Abstract:
//	When the call list is requested from the server, a base index can be
//	specified, along with a count. For example, there may be 100 call list
//	items on the server, but only items 10-19 are wanted. This method
//	specifies the server index of item 0.
//
// Returns:
//	unsigned int - The base index of the list
//
unsigned int CstiCallList::BaseIndexGet () const
{
	stiLOG_ENTRY_NAME (CstiCallList::BaseIndexGet);

	return (m_unBaseIndex);
	
} // end CstiCallList::BaseIndexGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::BaseIndexSet
//
// Description: Sets the base index of the list items.
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallList::BaseIndexSet (
	unsigned int unBaseIndex)
{
	stiLOG_ENTRY_NAME (CstiCallList::BaseIndexSet);

	m_unBaseIndex = unBaseIndex;

	return (stiRESULT_SUCCESS);
	
} // end CstiCallList::BaseIndexSet

////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::ThresholdGet
//
// Description: Returns the list Threshold size (-1 is default)
//
// Abstract:
//
// Returns:
//	int
//
int CstiCallList::ThresholdGet () const
{
	stiLOG_ENTRY_NAME (CstiCallList::ThresholdGet);

	return (m_nThreshold);
	
} // end CstiCallList::ThresholdGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::ThresholdSet
//
// Description: Sets the list threshold size
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallList::ThresholdSet (
	int nThreshold)
{
	stiLOG_ENTRY_NAME (CstiCallList::ThresholdSet);

	m_nThreshold = (nThreshold < -1) ? -1 : nThreshold;

	return (stiRESULT_SUCCESS);
	
} // end CstiCallList::ThresholdSet

// end file CstiCallList.cpp
