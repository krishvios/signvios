////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiContactList
//
//	File Name:	CstiContactList.cpp
//
//	Abstract:
//		Encapsulates the Contact List - a collection of CstiContactListItem objects
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CstiContactList.h"

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
//; CstiContactList::ItemGet
//
// Description: Returns the indicated item, or NULL
//
// Abstract:
//
// Returns:
//	CstiContactListItem * - Pointer to requested item, or NULL on error
//
CstiContactListItemSharedPtr CstiContactList::ItemGet(unsigned int unItem) const
{
	stiLOG_ENTRY_NAME(CstiContactList::ItemGet);

	CstiContactListItemSharedPtr item;

	// Was a valid index requested?
	if (m_listItems && unItem < m_listItems->size ())
	{
		item = std::static_pointer_cast<CstiContactListItem> ((*m_listItems)[unItem]);
	}

	return item;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactList::ItemAdd
//
// Description: Adds an item to the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiContactList::ItemAdd(const CstiContactListItemSharedPtr &callListItem)
{
	stiLOG_ENTRY_NAME(CstiContactList::ItemAdd);
	stiHResult hResult = stiRESULT_ERROR;

	hResult = CstiList::ItemAdd(callListItem);

	return (hResult);

}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactList::BaseIndexGet
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
unsigned int CstiContactList::BaseIndexGet() const
{
	stiLOG_ENTRY_NAME(CstiContactList::BaseIndexGet);

	return (m_unBaseIndex);

} // end CstiContactList::BaseIndexGet


////////////////////////////////////////////////////////////////////////////////
//; CstiContactList::BaseIndexSet
//
// Description: Sets the base index of the list items.
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiContactList::BaseIndexSet(unsigned int unBaseIndex)
{
	stiLOG_ENTRY_NAME(CstiContactList::BaseIndexSet);

	m_unBaseIndex = unBaseIndex;

	return (stiRESULT_SUCCESS);

} // end CstiContactList::BaseIndexSet

////////////////////////////////////////////////////////////////////////////////
//; CstiContactList::ThresholdGet
//
// Description: Returns the list Threshold size (-1 is default)
//
// Abstract:
//
// Returns:
//	int
//
int CstiContactList::ThresholdGet() const
{
	stiLOG_ENTRY_NAME(CstiContactList::ThresholdGet);

	return (m_nThreshold);

} // end CstiContactList::ThresholdGet


////////////////////////////////////////////////////////////////////////////////
//; CstiContactList::ThresholdSet
//
// Description: Sets the list threshold size
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiContactList::ThresholdSet(int nThreshold)
{
	stiLOG_ENTRY_NAME(CstiContactList::ThresholdSet);

	m_nThreshold = (nThreshold < -1) ? -1 : nThreshold;

	return (stiRESULT_SUCCESS);

} // end CstiContactList::ThresholdSet

// end file CstiContactList.cpp
