/*!
 * \file validator.cpp
 * \brief Helper Class Collection of methods for validating input fields.
 *
 * Validates various data types using a variety of techniques.
 * \todo The default regex library only handles 8-bit characters.  This means all text strings
 * that use regular expressions must be converted to ASCII.  We will want to either to manual
 * validation or switch to one of the regular expression libraries "out there" that supports
 * wide characters.  These will need to be modified to use PEGCHAR instead of wchar_t since
 * on linux wchar_t is 4 bytes but PEGCHAR is 2.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <regex.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "validator.h"
#include "IstiVideophoneEngine.h"

#if (APPLICATION == APP_NTOUCH_VP2)
#include "enginemanager.h"
#endif

namespace Vp200Ui {

extern IstiVideophoneEngine *g_pVE;

char canonicalPhoneRegex[] = "\\+[\\d]{1,4} \\([\\d]{0,5}\\) ([\\d]+[-. ])*[\\d]{2,}";      //!< Canonical phone number regular expression.
char domesticPhoneRegex[] = "^(1|)[^0-9]*[2-9][0-9]{2,2}[^0-9]*[0-9]{3,3}[^0-9]*[0-9]{4,4}$";       //!< Domestic 10 or 11-digit phone number.
char domesticPhoneRegexExt[] = "^(1|)[^0-9]*[2-9][0-9]{2,2}[^0-9]*[0-9]{3,3}[^0-9]*[0-9]{4,4}(x|)[0-9]{0,5}$";       //!< Domestic 10 or 11-digit phone number + up to 5 digits of extension.
char domesticPhoneRegexFull[] = "^(1)[2-9][0-9]{2,2}[0-9]{3,3}[0-9]{4,4}$";      //!< Domestic 11-digit phone number.
char domesticPhoneRegexFullExt[] = "^(1)[2-9][0-9]{2,2}[0-9]{3,3}[0-9]{4,4}(x|)[0-9]{0,5}$";      //!< Domestic 11-digit phone number + up to 5 digits of extension.
//char shortPhoneRegex[] = "^(1|)[^0-9]*[2-9][0-9]{2,2}[^0-9]*[0-9]{4,4}$";       //!< 7 or 8 digits, fist digit must be 1 (8 digits) or > 1 for 7 digits.
char shortPhoneRegex[] = "^(1|)[^0-9]*[0-9]{3,3}[^0-9]*[0-9]{4,4}$";       //!< 7 or 8 digits, fist digit must be 1 for 8 digits.
char ipregex[] = "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"; //!< Regular expression for IP address validation.
char emailregex[] = "^[A-Za-z0-9_.-]+@([A-Za-z0-9_-]+\\.)+[A-Za-z]{2,63}$";   //!< Regular exprssion for e-mail address validation.
char aimregex[] = "^[A-Za-z0-9 ]{2,}$";   //!< Regular exprssion for AIM screen name validation.
char addressregex[] = "^[A-Za-z0-9 #@&*()_+,.:;\"'/-]{0,50}$";   //!< Regular exprssion for mailing address validation.
char cityregex[] = "^[A-Za-z #@&*()_+,.:;\"'/-]{0,32}$";   //!< Regular exprssion for city name validation.
char zipregex[] = "(^[0-9]{5,5}$)|(^[0-9]{5,5}-[0-9]{4,4}$)";   //!< Regular expression for US zip code validation.
char hostNameRegex[] = "^[a-zA-Z][a-zA-Z0-9-]*[a-zA-Z0-9]$";        //!< Regular expression for host name.
char urlParseRegex[] = "^(ftp|http|file)://([^/]+)(/.*)?(/.*)";   //!< Regular expression to parse URL's
char Emergency911[] = "^911$";  //!< Emergency phone number
char Directory411[] = "^411$";  //!< Directory information phone number
char InternationalPhoneRegex[] = "^(011)[0-9]*"; //!< Regular expression for international phone number (calling from USA to another country).

// The following scripts disallow area codes or prefixes of type 1NN or n11 (N== any number 0-9, n== any number 2-9)
char restrictiveDomesticPhoneRegex[] = "^(1|)[^0-9]*[2-9](1[0,2-9]|[0,2-9][0-9])[^0-9]*[2-9](1[0,2-9]|[0,2-9][0-9])[^0-9]*[0-9]{4}$";    //!< Domestic 10 or 11-digit phone number.
char restrictiveDomesticPhoneRegexFull[] = "^(1)[2-9](1[0,2-9]|[0,2-9][0-9])[2-9](1[0,2-9]|[0,2-9][0-9])[0-9]{4}$";      //!< Domestic 11-digit phone number.

char PwdRepeatChar[] = "(.)\\1{3,}";  //!< Regex for repeating chars: chars cannot be repeated more than three times in a row
//char SsnRegex[] = "(?!0000)[0-9]{4}$";  //!< Regex to validate last 4 digits of SSN
char SsnRegex[] = "[0-9]{4}$";  //!< Regex to validate last 4 digits of SSN

/*!
 * \brief Array of US states and territories 2-letter abbreviations.
 */
char states[][3] = {
    "AK",    //!<    ALASKA
    "AL",    //!<    ALABAMA
    "AR",    //!<    ARKANSAS
    "AS",    //!<    AMERICAN SAMOA
    "AZ",    //!<    ARIZONA
    "CA",    //!<    CALIFORNIA
    "CO",    //!<    COLORADO
    "CT",    //!<    CONNECTICUT
    "DC",    //!<    DISTRICT OF COLUMBIA
    "DE",    //!<    DELAWARE
    "FL",    //!<    FLORIDA
    "FM",    //!<    FEDERATED STATES OF MICRONESIA
    "GA",    //!<    GEORGIA
    "GU",    //!<    GUAM
    "HI",    //!<    HAWAII
    "IA",    //!<    IOWA
    "ID",    //!<    IDAHO
    "IL",    //!<    ILLINOIS
    "IN",    //!<    INDIANA
    "KS",    //!<    KANSAS
    "KY",    //!<    KENTUCKY
    "LA",    //!<    LOUISIANA
    "MA",    //!<    MASSACHUSETTS
    "MD",    //!<    MARYLAND
    "ME",    //!<    MAINE
    "MH",    //!<    MARSHALL ISLANDS
    "MI",    //!<    MICHIGAN
    "MN",    //!<    MINNESOTA
    "MO",    //!<    MISSOURI
    "MP",    //!<    NORTHERN MARIANA ISLANDS
    "MS",    //!<    MISSISSIPPI
    "MT",    //!<    MONTANA
    "NC",    //!<    NORTH CAROLINA
    "ND",    //!<    NORTH DAKOTA
    "NE",    //!<    NEBRASKA
    "NH",    //!<    NEW HAMPSHIRE
    "NJ",    //!<    NEW JERSEY
    "NM",    //!<    NEW MEXICO
    "NV",    //!<    NEVADA
    "NY",    //!<    NEW YORK
    "OH",    //!<    OHIO
    "OK",    //!<    OKLAHOMA
    "OR",    //!<    OREGON
    "PA",    //!<    PENNSYLVANIA
    "PR",    //!<    PUERTO RICO
    "PW",    //!<    PALAU
    "RI",    //!<    RHODE ISLAND
    "SC",    //!<    SOUTH CAROLINA
    "SD",    //!<    SOUTH DAKOTA
    "TN",    //!<    TENNESSEE
    "TX",    //!<    TEXAS
    "UT",    //!<    UTAH
    "VA",    //!<    VIRGINIA
    "VI",    //!<    VIRGIN ISLANDS
    "VT",    //!<    VERMONT
    "WA",    //!<    WASHINGTON
    "WI",    //!<    WISCONSIN
    "WV",    //!<    WEST VIRGINIA
    "WY"    //!<    WYOMING
};

CValidator* CValidator::m_pInstance = NULL;
stiMUTEX_ID CValidator::m_Lock = stiOSMutexCreate();

/*!
 * \brief Retrieves the singleton instance of the CValidator.
 *
 * The CValidator class is a singleton helper class.  This method retrieves a
 * reference to the singleton object creating it by calling the constructor
 * if necessary.
 * \return Reference to the singleton CValidator object.
 */
CValidator& CValidator::GetInstance()
{
    Lock();
    if (!m_pInstance)
    {
        m_pInstance = new CValidator();
    }
    Unlock();
    return *m_pInstance;
}

/*!
 * \brief Destroys the CValidator singleton if it has been created.
 */
void
CValidator::Destroy()
{
	if (m_Lock)
	{
		stiOSMutexDestroy(m_Lock);
		m_Lock = NULL;
	}

    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

/*!
 * \brief Constructs the CValidator.
 *
 * Instantiates the regular expression objects for use by validators that
 * use regular expressions.  These are \b NOT unicode safe.
 */
CValidator::CValidator()
{
    int result = regcomp(&m_RegexEmail, emailregex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexAim, aimregex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexIp, ipregex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexAddress, addressregex, REG_EXTENDED);
    assert(result == 0);
	result = regcomp(&m_RegexCity, cityregex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexZip, zipregex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexDomesticPhone, domesticPhoneRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexDomesticPhoneExt, domesticPhoneRegexExt, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexDomesticPhoneFull, domesticPhoneRegexFull, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexDomesticPhoneFullExt, domesticPhoneRegexFullExt, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexCanonicalPhone, canonicalPhoneRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexShortPhone, shortPhoneRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexHostName, hostNameRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexUrlParse, urlParseRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexEmergency911, Emergency911, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexDirectory411, Directory411, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexInternationalPhone, InternationalPhoneRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexRestrictiveDomesticPhone, restrictiveDomesticPhoneRegex, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexRestrictiveDomesticPhoneFull, restrictiveDomesticPhoneRegexFull, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexPwdRepeatChar, PwdRepeatChar, REG_EXTENDED);
    assert(result == 0);
    result = regcomp(&m_RegexSsn, SsnRegex, REG_EXTENDED);
    assert(result == 0);
}

/*!
 * \brief Destroys the CValidator singleton freeing the compiled regular expressions.
 */
CValidator::~CValidator()
{
    regfree(&m_RegexEmail);
    regfree(&m_RegexAim);
    regfree(&m_RegexIp);
    regfree(&m_RegexAddress);
    regfree(&m_RegexCity);
    regfree(&m_RegexZip);
    regfree(&m_RegexDomesticPhone);
    regfree(&m_RegexDomesticPhoneExt);
    regfree(&m_RegexDomesticPhoneFull);
    regfree(&m_RegexDomesticPhoneFullExt);
    regfree(&m_RegexCanonicalPhone);
    regfree(&m_RegexShortPhone);
    regfree(&m_RegexHostName);
    regfree(&m_RegexUrlParse);
    regfree(&m_RegexEmergency911);
    regfree(&m_RegexDirectory411);
    regfree(&m_RegexInternationalPhone);
    regfree(&m_RegexRestrictiveDomesticPhone);
    regfree(&m_RegexRestrictiveDomesticPhoneFull);
    regfree(&m_RegexPwdRepeatChar);
    regfree(&m_RegexSsn);
}

/*!
 * \brief Validates a standard SMTP e-mail address.
 *
 * Uses a pre-compiled regular expression to validate an e-mail address.
 * \param str Pointer to the string to validate.
 * \param bRequired \a true If the e-mail address is required.  Validation will fail if this flag.
 * is true but the string is empty or NULL.
 * \retval true If the e-mail is valid.
 * \retval false If the e-mail is invalid or not present but required.
 */
bool
CValidator::ValidateEmail(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        bResult = (regexec(&m_RegexEmail, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validates an AIM screen name.
 *
 * Uses a pre-compiled regular expression to validate AIM screen name.
 * \param str Pointer to the string to validate.
 * \param bRequired \a true If the field is required.  Validation will fail if this flag.
 * is true but the string is empty or NULL.
 * \retval true If the screen name is valid.
 * \retval false If the screen name is invalid or not present but required.
 */
bool
CValidator::ValidateAim(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexAim, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validates a US international phone number (with the country exit code '011' required).
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidateInternationalPhone(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexInternationalPhone, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a valid point to point phone number.
 *
 * A valid point to point phone number is a valid Sorenson Number
 * or toll-free number which could be 7, 10 or 11 digits of number.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidatePoint2PointPhone (const char* str, bool bRequired)
{
    Lock();
    bool bResult = false;

    if (str && *str)
    {
        //
        // We have an input value, validate it
        //

        //
        // This is a list of valid point to point phone number
        //
        if (false == bResult)
        {
            bResult = ValidateShortPhone (str, bRequired);
        }

        if (false == bResult)
        {
            bResult = ValidatePhone (str, bRequired);
        }
    }
    else if (!bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If not required, then validate.
        //
        bResult = true;
    }
    Unlock();

    return bResult;
}


/*!
 * \brief Validates a US emergency phone number.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::Validate911(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexEmergency911, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US directory info phone number.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool CValidator::Validate411(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexDirectory411, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US phone number.
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidateDomesticPhoneAllowable (const char* str, bool bRequired)
{
    Lock();
    bool bResult = false;

    if (str && *str)
    {
        //
        // We have an input value, validate it
        //

        //
        // This is a list of valid allowable phone numbers
        // as different types of phone numbers become allowable
        // add them to this list
        //
        if (false == bResult)
        {
            bResult = ValidatePhoneExt (str, bRequired);
        }

        if (false == bResult)
        {
            bResult = ValidatePhoneCanonical (str, bRequired);
        }

        if (false == bResult)
        {
            bResult = ValidateShortPhone (str, bRequired);
        }

        if (false == bResult)
        {
            bResult = Validate911 (str, bRequired);
        }

        if (false == bResult)
        {
            bResult = Validate411 (str, bRequired);
        }
    }
    else if (!bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If not required, then validate.
        //
        bResult = true;
    }
    Unlock();

    return bResult;
}

/*!
 * \brief Validates a US phone number.
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \param bRestrictive \a true if we want the more restrictive validation.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidatePhone(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
#if (APPLICATION == APP_NTOUCH_VP2)
        bResult = EngineManager::VEGet()->PhoneNumberIsValid(str);
#else
        bResult = g_pVE->PhoneNumberIsValid(str);
#endif
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US phone number with extension.
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidatePhoneExt(const char* str, bool bRequired)
{
    Lock();

    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexDomesticPhoneExt, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US phone number (with the country code '1' required).
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \param bRestrictive \a true if we want the more restrictive validation.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidatePhoneFull(const char* str, bool bRequired, bool bRestrictive)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        if (bRestrictive)
        {
            bResult = (regexec(&m_RegexRestrictiveDomesticPhoneFull, str, 0, NULL, 0) == 0);
        }
        else
        {
            bResult = (regexec(&m_RegexDomesticPhoneFull, str, 0, NULL, 0) == 0);
        }
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US phone number (with the country code '1' required) + extension.
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidatePhoneFullExt(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexDomesticPhoneFullExt, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a US phone number.
 *
 * Note:  Since we do not support international dialing, the 1 country code
 * is not used.  If it is present, ignore it.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidateShortPhone(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexShortPhone, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


bool
CValidator::ValidatePhoneCanonical(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexCanonicalPhone, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validates a string as a valid phone number.
 *
 * \param str String containing the phone number to test.
 * \param bRequired \a true if the value is required.
 * \retval true String is valid.
 * \retval false String is not valid or not supplied but is required.
 */
bool
CValidator::ValidateDialString(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
#if (APPLICATION == APP_NTOUCH_VP2)
        bResult = EngineManager::VEGet()->DialStringIsValid(str);
#else
        bResult = g_pVE->DialStringIsValid(str);
#endif
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}


/*!
 * \brief Validate a US address line.
 *
 * Allowed characters are numbers, digits, and specials [#@&*()-_+,.:;”’/].
 *
 * \param str The string to check.
 * \param bRequired \a true If this is a required field.
 * \retval true The input is valid.
 * \retval false The input is not valid or required and not provided.
 */
bool
CValidator::ValidateUSAddress(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        bResult = (regexec(&m_RegexAddress, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validate a US city.
 *
 * Allowed characters are numbers, specials [#@&*()-_+,.:;”’/], but not digits.
 *
 * \param str The string to check.
 * \param bRequired \a true If this is a required field.
 * \retval true The input is valid.
 * \retval false The input is not valid or required and not provided.
 */
bool
CValidator::ValidateUSCity(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        bResult = (regexec(&m_RegexCity, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validates a US State using standard 2-character abbreviations.
 *
 * Validates the US state by doing a lookup in the US states table.
 *
 * \param str The 2-character US state.
 * \param bRequired \a true if required, \b false if the state is optional.
 * \retval true If the US state is valid.
 * \retval false If invalid or not present but required.
 */
bool
CValidator::ValidateUSState(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        // O(N) Consider binary search of sorted list
        //
        int i = 0;
        bResult = false;
        while (states[i++])
        {
            if (strcmp(states[i], str) == 0)
            {
                bResult = true;
                break;
            }
        }
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validate a US zip code.
 *
 * All we do here is make sure we have 5 or 9 digits with (or without a - after the 5th
 * character.
 *
 * \param str The string to check.
 * \param bRequired \a true If this is a required field.
 * \retval true The input is valid.
 * \retval false The input is not valid or required and not provided.
 */
bool
CValidator::ValidateUSZip(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        bResult = (regexec(&m_RegexZip, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validates a standard IPv4 IP address in dotted notation.
 *
 * Uses a regular expression to ensure the IP address is in the valid format of a.b.c.d
 * The IP is converted from Unicode to ASCII for the regular expression.  Input must
 * therefore be PEG unicde format.
 * \param str The IP address string in PEGCHAR format.
 * \param bRequired \b true if the IP is required \b false otherwise.
 * \retval true If the IP address is valid.
 * \retval false If the IP is invalid or required but not supplied.
 * \todo Add IPv6 support when IPv6 is supported by the videophone.
 */
bool
CValidator::ValidateIP(const char* str, bool bRequired)
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input valie, validate it
        //
        bResult = (regexec(&m_RegexIp, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

/*!
 * \brief Validates a required field.
 *
 * This simple validator simply checks if a required value is provided.  It makes no presumptions about
 * the validity or content of the value.
 * \param str The string to validate.
 * \retval true If the string was supplied and is non-empty.
 * \retval false If the string was not supplied or is empty.
 */
bool
CValidator::ValidateRequired(const char* str)
{
    return (str && *str);
}

/*!
 * \brief Validates a required field, ensuring beginning char is not white space.
 *
 * This validator simply checks if a required value is provided, and ensures first character
 * is not white space.
 * \param str The string to validate.
 * \retval true If the string was supplied and is non-empty.
 * \retval false If the string was not supplied or is empty.
 */
bool
CValidator::ValidateNoBeginningWhiteSpace(const char* str)
{
    bool bResult = false;
    if (str)
    {
        if ( (*str != ' ') && (*str != '\t') )
        {
            bResult = true;
        }
    }
    return bResult;
}

/*!
 * \brief Simple password validator.
 *
 * Makes no assumptions about the content of the password other than a supplied minimum length
 * If the minimum length is 0, then empty passwords are allowed.  If it is greater than 0, then
 * the passwords must be as long or longer than the minimum length.  The passwords must match for
 * use with password and confirm password fields.
 * \param str1 The first password field.
 * \param str2 The confirm password field.
 * \param nMinLength The minimum length of the password.  If \a 0, then empty passwords are allowed.
 * \retval true If the passwords are supplied and match and >= nMinLength
 * or both empty and nMinLength is \a 0.
 * \retval false If passwords do not match or are shorter than nMinLength.
 */
bool
CValidator::ValidatePassword(const char* str1, const char* str2, int nMinLength)
{
    bool bValid = false;
    bool bEmpty1 = (!str1 || (str1 && !*str1));
    bool bEmpty2 = (!str2 || (str2 && !*str2));
    if (!bEmpty1 && !bEmpty2 && nMinLength >= 0)
    {
        bValid = (strcmp(str1, str2) == 0);
    }
    else if (bEmpty1 && bEmpty2 && nMinLength == 0)
    {
        bValid = true;
    }
    return bValid;
}

/*!
 * \brief Validates a host name
 *
 * \retval true If the host name is valid.
 * \retval false If the host name is invalid or required but not supplied.
 */
bool CValidator::ValidateHost(
    const char* str, //!< The host name string
    bool bRequired) //!<  \b true if the host name is required \b false otherwise.
{
    Lock();
    bool bResult = true;
    if (str && *str)
    {
        //
        // We have an input value, validate it
        //
        bResult = (regexec(&m_RegexHostName, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        //
        // We were passed NULL or a NULL string.  If required, then we failed.
        //
        bResult = false;
    }
    Unlock();
    return bResult;
}

bool
CValidator::ParseUrl(const char* url, size_t max, regmatch_t match[])
{
    bool bResult = false;
    if (url)
    {
        bResult = (regexec(&m_RegexUrlParse, url, max, match, 0) == 0);
    }
    return bResult;
}

///
/// \brief Validates a that a password (or other) string contains no more three repeating chars.
///
/// \param str String to test.
/// \param bRequired \a true if the value is required.
/// \retval true String is valid.
/// \retval false String is not valid or not supplied but is required.
///
bool CValidator::ValidatePwdRepeatChar(const char* str, bool bRequired)
{
    bool bValid = true;

    if (str && *str)
    {
        // The regex returns true if it finds the pattern;
        //  since we _don't_ want this pattern, invert the result for bValid
        bValid = !(regexec(&m_RegexPwdRepeatChar, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        bValid = false;
    }
    return bValid;
}


///
/// \brief Validates a that a password string doesn't 4 or more sequential chars (e.g. 1234).
///
/// \param str String to test.
/// \param bRequired \a true if the value is required.
/// \retval true String is valid.
/// \retval false String is not valid or not supplied but is required.
/// \note This routine is limited...
///
bool CValidator::ValidatePwdNoSequentialChar(const char* str, bool bRequired)
{
    bool bValid = true;

    if (str && *str)
    {
        if (!strcmp(str, "1234"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "2345"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "3456"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "4567"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "5678"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "6789"))
        {
            bValid = false;
        }
        else if (!strcmp(str, "7890"))
        {
            bValid = false;
        }
        else if (!stricmp(str, "abcd"))
        {
            bValid = false;
        }
        else if (!stricmp(str, "wxyz"))
        {
            bValid = false;
        }
    }
    else if (bRequired)
    {
        bValid = false;
    }
    return bValid;
}

///
/// \brief Validates last 4 digits of SSN.
///
/// \param str String to test.
/// \param bRequired \a true if the value is required.
/// \retval true String is valid.
/// \retval false String is not valid or not supplied but is required.
///
bool CValidator::ValidateSsn(const char* str, bool bRequired)
{
    bool bValid = true;

    if (str && *str)
    {
        // The regex returns true if it finds the pattern;
        //  since we _don't_ want this pattern, invert the result for bValid
        bValid = (regexec(&m_RegexSsn, str, 0, NULL, 0) == 0);
    }
    else if (bRequired)
    {
        bValid = false;
    }
    return bValid;
}

};
