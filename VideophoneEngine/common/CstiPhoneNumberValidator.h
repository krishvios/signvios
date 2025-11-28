// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIPHONENUMBERVALIDATOR
#define CSTIPHONENUMBERVALIDATOR

#include "CstiRegEx.h"
#include <vector>
#include "stiSVX.h"


const int g_nAREA_CODE_LENGTH = 3;

class CstiPhoneNumberValidator
{
public:
	~CstiPhoneNumberValidator ();

    static CstiPhoneNumberValidator *InstanceGet ();

	bool DialStringIsValid (
		const char *pszPhoneNumber);

    void LocalAreaCodeSet(
        const char *pszAreaCode);

    bool PhoneNumberIsValid(
        const char *pszPhoneNumber);

	bool PhoneNumberIsSupport (
		std::string dialString);

    stiHResult PhoneNumberReformat(
        const std::string &oldPhoneNumber,
        std::string &newPhoneNumber,
        bool *pbIsVRS);

    stiHResult PhoneNumberReformat(
        const std::string &oldPhoneNumber,
        std::string &newPhoneNumber);

	stiHResult VRSDialStringCheck (
		const char *pszDialString,
		bool *pbIsVRS);

private:
    CstiPhoneNumberValidator ();

	bool DialStringContains (
		std::string dialString,
		const char* phoneNumber);

	CstiRegEx * m_PhoneNumbersWithRegionCode;
	CstiRegEx * m_PhoneNumbersWithoutRegionCode;
	CstiRegEx * m_PhoneNumbersWithoutRegionCodeAndExplicitExt;
	CstiRegEx * m_PhoneNumberWithoutAreaCode;
	CstiRegEx * m_PhoneNumberWithoutAreaCodeAndExplicitExt;
	CstiRegEx *  m_AllDigits;
	CstiRegEx *  m_AllAlphaNumeric;
	CstiRegEx *  m_AllDigitsAndDots;
	CstiRegEx *  m_IPAddress;

	std::vector <CstiRegEx *> m_VRSDialStrings;

    char m_szAreaCode [g_nAREA_CODE_LENGTH + 1]{};
};



#endif // CSTIPHONENUMBERVALIDATOR

