///
/// \file CstiUserAccountInfo.h
/// \brief Declaration of the class containing User Account Information.
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2003 – 2011 Sorenson Communications, Inc. -- All rights reserved
///

#ifndef CSTIUSERACCOUNTINFO_H
#define CSTIUSERACCOUNTINFO_H

//
// Includes
//
#include "CstiString.h"

//
// Constants
//


class CstiUserAccountInfo
{
public:
	std::string m_Name;
	std::string m_PhoneNumber;
	std::string m_TollFreeNumber;
	std::string m_LocalNumber;
	time_t m_localNumberNewDate {0};
	std::string m_newTagValidDays;
	std::string m_AssociatedPhone;
	std::string m_CurrentPhone;
	std::string m_Address1;
	std::string m_Address2;
	std::string m_City;
	std::string m_State;
	std::string m_ZipCode;
	std::string m_Email;
	std::string m_EmailMain;
	std::string m_EmailPager;
	std::string m_SignMailEnabled;
	std::string m_MustCallCIR;
	std::string m_CLAnsweredQuota;
	std::string m_CLDialedQuota;
	std::string m_CLMissedQuota;
	std::string m_CLRedialQuota;
	std::string m_CLBlockedQuota;
	std::string m_CLContactQuota;
	std::string m_CLFavoritesQuota;
	std::string m_RingGroupLocalNumber;
	time_t m_ringGroupLocalNumberNewDate {0};
	std::string m_ringGroupNewTagValidDays;
	std::string m_RingGroupTollFreeNumber;
	std::string m_RingGroupName;
	std::string m_ImageId;
	std::string m_ImageDate;
	std::string m_UserRegistrationDataRequired;
	std::string m_userId;
	std::string m_groupUserId;
};


#endif //CSTIUSERACCOUNTINFO_H
