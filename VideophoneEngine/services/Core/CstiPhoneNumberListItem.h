/*!
 * \file CstiPhoneNumberListItem.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 – 2018 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

//
// Includes
// 
#include "CstiListItem.h"
#include <string>
#include <memory>

//
// Class Declaration
//

class CstiPhoneNumberListItem : public CstiListItem
{
public:
	
	CstiPhoneNumberListItem (const std::string &);
	~CstiPhoneNumberListItem () override = default;
	
	std::string phoneNumberGet ();
	void phoneNumberSet (const std::string &phoneNumber);
	
private:
	
	std::string m_phoneNumber;
};

using CstiPhoneNumberListItemSharedPtr = std::shared_ptr<CstiPhoneNumberListItem>;
