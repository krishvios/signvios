/*!
 * \file validator.h
 * \brief Collection of methods for validating input fields.
 *
 * \date Tue May 28 2004
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef WILLOWUIVALIDATOR_H
#define WILLOWUIVALIDATOR_H

#include <regex.h>
#include "stiOSMutex.h"

namespace Vp200Ui {

// states[][3] is defined in validator.cpp
extern char states[][3];

/*!
 * \brief Singleton helper class of data validators.
 *
 * Contains members that validate for format of various types of data including whether they
 * are provided at all (required fields).
 */
class CValidator{
public:

    static CValidator& GetInstance();

    ~CValidator();

    bool ValidateEmail(const char* str, bool bRequired = false);
    bool ValidateAim(const char* str, bool bRequired = false);
    bool Validate911(const char* str, bool bRequired = false);
    bool Validate411(const char* str, bool bRequired = false);
    bool ValidateDomesticPhoneAllowable(const char* str, bool bRequired = false);
    bool ValidatePhone(const char* str, bool bRequired = false);
    bool ValidatePhoneExt(const char* str, bool bRequired = false);
    bool ValidatePhoneFull(const char* str, bool bRequired = false, bool bRestrictive = false);
    bool ValidatePhoneFullExt(const char* str, bool bRequired = false);
    bool ValidateShortPhone(const char* str, bool bRequired = false);
    bool ValidatePhoneCanonical(const char* str, bool bRequired = false);
    bool ValidateUSAddress(const char* str, bool bRequired = false);
    bool ValidateUSCity(const char* str, bool bRequired = false);
    bool ValidateUSState(const char* str, bool bRequired = false);
    bool ValidateUSZip(const char* str, bool bRequired = false);
    bool ValidateIP(const char* str, bool bRequired = false);
    bool ValidateRequired(const char* str);
    bool ValidateNoBeginningWhiteSpace(const char* str);
    bool ValidatePassword(const char* str1, const char* str2, int nMinLength);
    bool ValidateHost(const char* str, bool bRequired = false);
    bool ParseUrl(const char* url, size_t max, regmatch_t match[]);
    bool ValidateInternationalPhone (const char* str, bool bRequired = false);
    bool ValidatePoint2PointPhone (const char* str, bool bRequired = false);
    bool ValidateDialString(const char* str, bool bRequired = false);
    bool ValidatePwdRepeatChar(const char* str, bool bRequired = false);
    bool ValidatePwdNoSequentialChar(const char* str, bool bRequired = false);
    bool ValidateSsn(const char* str, bool bRequired = false);

    static void Destroy();

protected:

    CValidator();


    static void Lock() { stiOSMutexLock(m_Lock); }
    static void Unlock() { stiOSMutexUnlock(m_Lock); }

    static CValidator* m_pInstance;         //!< Pointer to the static instance of the object.  There can be only one.

    regex_t    m_RegexEmail;         //!< E-mail validator precompiled regular expression.
    regex_t    m_RegexAim;         //!< AIM screen name validator precompiled regular expression.
    regex_t    m_RegexIp;            //!< IP address (IPv4) validator precompiled regular expression.
    regex_t    m_RegexAddress;           //!< US mailing address validator precompiled regular expression.
    regex_t    m_RegexCity;           //!< US City validator precompiled regular expression.
    regex_t    m_RegexZip;           //!< US Zip code validator precompiled regular expression.
    regex_t    m_RegexDomesticPhone;    //!< Domestic US phone number regular expression.
    regex_t    m_RegexDomesticPhoneExt;    //!< Domestic US phone number regular expression + extension.
    regex_t    m_RegexDomesticPhoneFull;    //!< Domestic US phone number (with beginning '1') regular expression.
    regex_t    m_RegexDomesticPhoneFullExt;    //!< Domestic US phone number (with beginning '1') regular expression + extension.
    regex_t    m_RegexCanonicalPhone;   //!< Canonical phone number regular expression.
    regex_t    m_RegexShortPhone;       //!< Short phone number (7-8 digots) regular expression.
    regex_t    m_RegexHostName;         //!< host name regular expression.
    regex_t    m_RegexUrlParse;         //!< Parse regular expression.
    regex_t    m_RegexEmergency911;	//!< Emergency number
    regex_t    m_RegexDirectory411;	//!< Directory information number
    regex_t    m_RegexInternationalPhone; //!< International phone number
    regex_t    m_RegexRestrictiveDomesticPhone;    //!< Domestic US phone number regular expression (restricted prefixes).
    regex_t    m_RegexRestrictiveDomesticPhoneFull;    //!< Domestic US phone number (with beginning '1') regular expression.
    regex_t    m_RegexPwdRepeatChar;	//!< Allow a max of 3 repeating chars
    regex_t    m_RegexSsn;          //!< Last 4 digits of Social Security number

    static stiMUTEX_ID m_Lock;
};

};

#endif
