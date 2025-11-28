////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiPhoneNumberList
//
//	File Name:	CstiPhoneNumberList.h
//
//	Abstract:
//		See CstiPhoneNumberList.cpp
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

//
// Includes
//
#include "stiDefs.h"
#include "CstiList.h"
#include "CstiPhoneNumberListItem.h"

//
// Class Declaration
//
class CstiPhoneNumberList : public CstiList
{
public:
	
	//
	// Public Enumerations
	//
	enum EType
	{
		eTYPE_UNKNOWN,	//!< Unknown list type
		eLOCAL,			//!< NANP Local type
		eTOLL_FREE		//!< NANP Toll Free type
	}; // end EType
	
	//
	// Public Methods
	//
	CstiPhoneNumberList () = default;
	CstiPhoneNumberList (const CstiPhoneNumberList &other) = delete;
	CstiPhoneNumberList (CstiPhoneNumberList &&other) = delete;
	CstiPhoneNumberList &operator= (const CstiPhoneNumberList &other) = delete;
	CstiPhoneNumberList &operator= (CstiPhoneNumberList &&other) = delete;
	~CstiPhoneNumberList () override = default;
	
	stiHResult ItemAdd (const CstiPhoneNumberListItemSharedPtr &item);
	CstiPhoneNumberListItemSharedPtr ItemGet (unsigned int nItem) const;
	
	EType TypeGet () const;
	stiHResult TypeSet (EType eType);

private:
	
	using CstiList::ItemAdd;

	EType m_eType {eTYPE_UNKNOWN};

};
