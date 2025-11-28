////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiPhoneNumberList
//
//	File Name:	CstiPhoneNumberList.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Encapsulates the Call List - a collection of CstiPhoneNumberListItem objects
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CstiPhoneNumberList.h"

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
//; CstiPhoneNumberList::ItemGet
//
// Description: Returns the indicated item, or NULL
//
// Abstract:
//
// Returns:
//	CstiPhoneNumberListItem * - Pointer to requested item, or NULL on error
//
CstiPhoneNumberListItemSharedPtr CstiPhoneNumberList::ItemGet (
	unsigned int unItem) const
{
	stiLOG_ENTRY_NAME (CstiPhoneNumberList::ItemGet);

	CstiPhoneNumberListItemSharedPtr item;

	// Was a valid index requested?
	if (m_listItems && unItem < m_listItems->size ())
	{
		item = std::static_pointer_cast<CstiPhoneNumberListItem> ((*m_listItems)[unItem]);
	}

	return item;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberList::ItemAdd
//
// Description: Adds an item to the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiHResult - estiOK on success, otherwise stiRESULT_ERROR
//
stiHResult CstiPhoneNumberList::ItemAdd (
	const CstiPhoneNumberListItemSharedPtr &item)
{
	stiLOG_ENTRY_NAME (CstiPhoneNumberList::ItemAdd);

	stiHResult hResult = stiRESULT_ERROR;

	hResult = CstiList::ItemAdd (item);

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberList::TypeGet
//
// Description: Returns the list type
//
// Abstract:
//
// Returns:
//	CstiList::EType
//
CstiPhoneNumberList::EType CstiPhoneNumberList::TypeGet () const
{
	stiLOG_ENTRY_NAME (CstiPhoneNumberList::TypeGet);

	return (m_eType);
	
} // end CstiPhoneNumberList::TypeGet


////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberList::TypeSet
//
// Description: Sets the list type
//
// Abstract:
//
// Returns:
//	stiHResult - estiOK on success, otherwise stiRESULT_ERROR
//
stiHResult CstiPhoneNumberList::TypeSet (
	CstiPhoneNumberList::EType eType)
{
	stiLOG_ENTRY_NAME (CstiPhoneNumberList::TypeSet);

	m_eType = eType;

	return stiRESULT_SUCCESS;

}
