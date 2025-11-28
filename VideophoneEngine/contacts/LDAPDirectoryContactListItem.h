//
//  LDAPDirectoryContactListItem.h
//  ntouchMac
//
//  Created by J.R. Blackham on 2/5/15.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef LDAPDIRECTORYCONTACTLISTITEM_H
#define LDAPDIRECTORYCONTACTLISTITEM_H

#include "ContactListItem.h"

class LDAPDirectoryContactListItem : public ContactListItem
{
public:
	LDAPDirectoryContactListItem() = default;
	LDAPDirectoryContactListItem(const IstiContactSharedPtr &other);
	~LDAPDirectoryContactListItem() override = default;

#ifdef stiCONTACT_LIST_MULTI_PHONE
	void PhoneNumberAdd(CstiContactListItem::EPhoneNumberType type, const std::string &dialString) override;
#endif

private:
};

#endif
