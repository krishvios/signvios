////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiListItem
//
//	File Name:	CstiListItem.h
//
//	Owner:
//
//	Abstract:
//		See CstiListItem.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTI_LIST_ITEM_H
#define CSTI_LIST_ITEM_H

//
// Includes
//
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiTrace.h"
#include <memory>

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

class CstiListItem
{
	
public:
	
	CstiListItem () = default;
	virtual ~CstiListItem () = default;
	
}; // end class CstiListItem

using CstiListItemSharedPtr = std::shared_ptr<CstiListItem>;

#endif // CSTI_LIST_ITEM_H

// end file CstiListItem.h

