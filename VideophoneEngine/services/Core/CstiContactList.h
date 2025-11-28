////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiContactList
//
//	File Name:	CstiContactList.h
//
//	Abstract:
//		See CstiContactList.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTI_CONTACT_LIST_H
#define CSTI_CONTACT_LIST_H

//
// Includes
//
#include "stiDefs.h"
#include "CstiList.h"
#include "CstiContactListItem.h"

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
class CstiContactList : public CstiList
{
public:
	CstiContactList() = default;
	~CstiContactList() override = default;

	stiHResult ItemAdd(const CstiContactListItemSharedPtr &callListItem);
	CstiContactListItemSharedPtr ItemGet(unsigned int nItem) const;

	unsigned int BaseIndexGet() const;
	stiHResult BaseIndexSet(unsigned int unBaseIndex);
	int ThresholdGet() const;
	stiHResult ThresholdSet(int nThreshold);

private:

	using CstiList::ItemAdd;

	unsigned int m_unBaseIndex{0};
	int m_nThreshold{-1};

}; // end class CstiContactList


#endif // CSTI_CONTACT_LIST_H

// end file CstiContactList.h
