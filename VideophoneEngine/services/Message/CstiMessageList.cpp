////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiMessageList
//
//	File Name:	CstiMessageList.cpp
//
//	Abstract:
//		Encapsulates the message List - a collection of CstiMessageListItem objects
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CstiMessageList.h"

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

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::ItemGet
//
// Description: Returns the indicated item, or NULL
//
// Abstract:
//
// Returns:
//	CstiMessageListItem * - Pointer to requested item, or NULL on error
//
CstiMessageListItemSharedPtr CstiMessageList::ItemGet (
	unsigned int unItem) const
{
	stiLOG_ENTRY_NAME (CstiMessageList::ItemGet);

	CstiMessageListItemSharedPtr item;

	// Was a valid index requested?
	if (m_listItems && unItem < m_listItems->size())
	{
		item = std::static_pointer_cast<CstiMessageListItem> ((*m_listItems)[unItem]);
	}
	
	return item;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::ItemAdd
//
// Description: Adds an item to the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise estiERROR
//
stiHResult CstiMessageList::ItemAdd (
	const CstiMessageListItemSharedPtr &messageListItem)
{
	stiLOG_ENTRY_NAME (CstiMessageList::ItemAdd);

	stiHResult hResult = stiRESULT_ERROR;

	hResult = CstiList::ItemAdd (messageListItem);

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::SortFieldGet
//
// Description: Returns the field by which the list is sorted
//
// Abstract:
//
// Returns:
//	CstiMessageList::ESortField
//
CstiMessageList::ESortField CstiMessageList::SortFieldGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageList::SortFieldGet);

	return (m_eSortField);
	
} // end CstiMessageList::SortFieldGet


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::SortFieldSet
//
// Description: Sets the field on which the data is sorted.
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise estiERROR
//
stiHResult CstiMessageList::SortFieldSet (
	CstiMessageList::ESortField eSortField)
{
	stiLOG_ENTRY_NAME (CstiMessageList::SortFieldSet);

	m_eSortField = eSortField;

	return stiRESULT_SUCCESS;
	
} // end CstiMessageList::SortFieldSet


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::BaseIndexGet
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
unsigned int CstiMessageList::BaseIndexGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageList::BaseIndexGet);

	return (m_unBaseIndex);
	
} // end CstiMessageList::BaseIndexGet


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::BaseIndexSet
//
// Description: Sets the base index of the list items.
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise estiERROR
//
stiHResult CstiMessageList::BaseIndexSet (
	unsigned int unBaseIndex)
{
	stiLOG_ENTRY_NAME (CstiMessageList::BaseIndexSet);

	m_unBaseIndex = unBaseIndex;

	return (stiRESULT_SUCCESS);
	
} // end CstiMessageList::BaseIndexSet

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::ThresholdGet
//
// Description: Returns the list Threshold size (-1 is default)
//
// Abstract:
//
// Returns:
//	int
//
int CstiMessageList::ThresholdGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageList::ThresholdGet);

	return (m_nThreshold);
	
} // end CstiMessageList::ThresholdGet


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageList::ThresholdSet
//
// Description: Sets the list threshold size
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise estiERROR
//
stiHResult CstiMessageList::ThresholdSet (
	int nThreshold)
{
	stiLOG_ENTRY_NAME (CstiMessageList::ThresholdSet);

	m_nThreshold = (nThreshold < -1) ? -1 : nThreshold;

	return (stiRESULT_SUCCESS);
	
} // end CstiMessageList::ThresholdSet


// end file CstiMessageList.cpp
