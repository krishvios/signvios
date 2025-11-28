//
//  LDAPDirectoryContactListItem.cpp
//  ntouchMac
//
//  Created by J.R. Blackham on 2/5/15.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#include "LDAPDirectoryContactListItem.h"

#include <cstring>
#include "ContactListItem.h"


LDAPDirectoryContactListItem::LDAPDirectoryContactListItem(const IstiContactSharedPtr &other)
	: ContactListItem(other)
{
}

#ifdef stiCONTACT_LIST_MULTI_PHONE

void LDAPDirectoryContactListItem::PhoneNumberAdd(
	CstiContactListItem::EPhoneNumberType type,
	const std::string &dialString)
{
	int locBase = eCONTACT_MAX + type * eNUMBERFIELD_MAX;
	std::string publicID;
	
	switch (type)
	{
	case CstiContactListItem::ePHONE_HOME:
		publicID = "H";
		break;
	case CstiContactListItem::ePHONE_WORK:
		publicID = "W";
		break;
	case CstiContactListItem::ePHONE_CELL:
		publicID = "M";
		break;
	default:
		publicID = "W";
		break;
	}
	
	publicID = publicID + m_contactIDStr;
	
	IntSet(0, locBase + eNUMBERFIELD_ID);
	StringSet(publicID, locBase + eNUMBERFIELD_PUBLIC_ID);
	StringSet(dialString, locBase + eNUMBERFIELD_DIALSTRING);
}

#endif


