////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiPhoneNumberListItem
//
//	File Name:	CstiPhoneNumberListItem.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Implement a single call list item
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stiDefs.h"
#include "CstiPhoneNumberListItem.h"

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

////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberListItem::CstiPhoneNumberListItem
//
// Description: Default Constructor
//
// Abstract:
//
// Returns:
//	None
//
CstiPhoneNumberListItem::CstiPhoneNumberListItem (const std::string &phoneNumber) :
	m_phoneNumber (phoneNumber)
{
	stiLOG_ENTRY_NAME (CstiPhoneNumberListItem::CstiPhoneNumberListItem);
} // end CstiPhoneNumberListItem::CstiPhoneNumberListItem


////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberListItem::DialStringGet
//
// Description: DialStringGet
//
// Abstract:
//	DialStringGet
//
// Returns:
//	CstiString
//
std::string CstiPhoneNumberListItem::phoneNumberGet ()
{
	return m_phoneNumber;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiPhoneNumberListItem::DialStringSet
//
// Description: DialStringSet
//
// Abstract:
//	DialStringSet
//
// Returns:
//	None
//
void CstiPhoneNumberListItem::phoneNumberSet (const std::string &phoneNumber)
{
	m_phoneNumber = phoneNumber;
}


// end file CstiPhoneNumberListItem.cpp
