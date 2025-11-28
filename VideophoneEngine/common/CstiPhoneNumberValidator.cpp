// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "stiSVX.h"
#include "CstiPhoneNumberValidator.h"
#include <algorithm>

#define stiSVRS_ESPANOL_PHONENUMBER_WITHOUT_COUNTRY_CODE "8669877528"
#define stiSVRS_PHONENUMBER_WITHOUT_COUNTRY_CODE "8663278877"

static const char *g_szVRSDialStrings[] =
{
	"^211$",			// Community information and referral
	"^311$",			// Non=-emergency police
	"^411$",			// Directory assistance
	"^511$",			// Traffic and transportation information
	"^711$",			// State TTY relay
	"^811$",			// Local utility call-before-you-dig service
	"^911$", 			// Emergency services
	"^(011)[0-9]{4,}$",		// International
	"^1?" stiSVRS_ESPANOL_PHONENUMBER_WITHOUT_COUNTRY_CODE "$",		// SVRS Espanol
	"^1?" stiSVRS_PHONENUMBER_WITHOUT_COUNTRY_CODE "$",	// SVRS
};
static const int g_nNumberOfVRSDialStrings = sizeof (g_szVRSDialStrings) / sizeof (g_szVRSDialStrings[0]);

static const char g_szPhoneNumbersWithRegionCode[] = "^(1|\\+1)[ -.]?\\(?[2-9][0-9]{2,2}\\)?[ -.]?[0,2-9][0-9]{2,2}[ -.]?[0-9]{4,4}(x?[0-9]{1," szEXTENSION_NUMBER_LENGTH "})?$";
static const char g_szPhoneNumbersWithoutRegionCode[] = "^\\(?[2-9][0-9]{2,2}\\)?[ -.]?[0,2-9][0-9]{2,2}[ -.]?[0-9]{4,4}(x?[0-9]{1," szEXTENSION_NUMBER_LENGTH "})?$";
static const char g_szPhoneNumbersWithoutAreaCode[] = "^[0,2-9][0-9]{2,2}[ -.]?[0-9]{4,4}(x?[0-9]{1," szEXTENSION_NUMBER_LENGTH "})?$";

static const char g_szPhoneNumbersWithoutRegionCodeAndExplicitExt[] = "^\\(?[2-9][0-9]{2,2}\\)?[ -.]?[0,2-9][0-9]{2,2}[ -.]?[0-9]{4,4}(x[0-9]{1," szEXTENSION_NUMBER_LENGTH "})?$";
static const char g_szPhoneNumbersWithoutAreaCodeAndExplicitExt[] = "^[0,2-9][0-9]{2,2}[ -.]?[0-9]{4,4}(x[0-9]{1," szEXTENSION_NUMBER_LENGTH "})?$";

static const char g_szIPregex[] = "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"; //!< Regular expression for IP address validation.

static const char g_szAllDigits[] = "^[0-9][0-9]*$";
static const char g_szAllAlphaNumberic[] = "^[0-9A-Za-z][0-9A-Za-z]*$";
static const char g_szAllDigitsAndDots[] = "^[0-9][0-9\\.]*$";

// WLC 12/14-2022 - Tech support number is no longer used, however we are just commenting out in case it is need in the future.
//static const char TechSupportNumber[] = "18012879403";
static const char CustomerService3DigitNumber[] = "611";

CstiPhoneNumberValidator::CstiPhoneNumberValidator()
{
	m_szAreaCode[0] = '\0';

	m_PhoneNumbersWithRegionCode = new CstiRegEx (g_szPhoneNumbersWithRegionCode);

	m_PhoneNumbersWithoutRegionCode = new CstiRegEx (g_szPhoneNumbersWithoutRegionCode);

	m_PhoneNumbersWithoutRegionCodeAndExplicitExt = new CstiRegEx (g_szPhoneNumbersWithoutRegionCodeAndExplicitExt);

	m_PhoneNumberWithoutAreaCode = new CstiRegEx (g_szPhoneNumbersWithoutAreaCode);

	m_PhoneNumberWithoutAreaCodeAndExplicitExt = new CstiRegEx (g_szPhoneNumbersWithoutAreaCodeAndExplicitExt);

	m_AllDigits = new CstiRegEx (g_szAllDigits);

	m_AllAlphaNumeric = new CstiRegEx (g_szAllAlphaNumberic);

	m_AllDigitsAndDots = new CstiRegEx (g_szAllDigitsAndDots);

	m_IPAddress = new CstiRegEx (g_szIPregex);

	m_VRSDialStrings.resize (g_nNumberOfVRSDialStrings);

	for (int j = 0; j < g_nNumberOfVRSDialStrings; j++)
	{
		m_VRSDialStrings[j] = new CstiRegEx (g_szVRSDialStrings[j]);
	}
}

CstiPhoneNumberValidator::~CstiPhoneNumberValidator ()
{
	delete m_PhoneNumbersWithRegionCode;
	delete m_PhoneNumbersWithoutRegionCode;
	delete m_PhoneNumbersWithoutRegionCodeAndExplicitExt;
	delete m_PhoneNumberWithoutAreaCode;
	delete m_PhoneNumberWithoutAreaCodeAndExplicitExt;
	delete m_AllDigits;
	delete m_AllAlphaNumeric;
	delete m_AllDigitsAndDots;
	delete m_IPAddress;

	for (int j = 0; j < g_nNumberOfVRSDialStrings; j++)
	{
		delete m_VRSDialStrings[j];
	}
	m_VRSDialStrings.erase (m_VRSDialStrings.begin (), m_VRSDialStrings.end());
}


CstiPhoneNumberValidator *CstiPhoneNumberValidator::InstanceGet()
{
    static CstiPhoneNumberValidator localInstance;
    return &localInstance;
}


///
///\brief Strips all non-digit characters from the input string
///
static void StripNonDigits (
	const std::string &input,	///< The input string to strip
	std::string &output)		///< The output string
{
	output = input;

	output.erase(std::remove_if(output.begin (), output.end (), [](char c){return !isdigit(c);}), output.end ());
}


/*!\brief Checks if a string represents a valid dial string
*
*/
bool CstiPhoneNumberValidator::DialStringIsValid (
	const char *pszPhoneNumber)
{
	bool bIsValid = false;

	if (m_AllDigits->Match (pszPhoneNumber))
	{
		// All digits must be a phone number
		bIsValid = PhoneNumberIsValid (pszPhoneNumber);
	}
	else if (m_AllDigitsAndDots->Match (pszPhoneNumber))
	{
		// All digits and dots ('.') must be an IP address
		bIsValid = m_IPAddress->Match (pszPhoneNumber);
	}
	else
	{
		// If any other characters are present, we assume it's something
		// like a URL, so we don't do anything to validate it.
		bIsValid = true;
	}

	return bIsValid;
}

/*!\brief Checks if a string represents a valid phone number
 *
 */
bool CstiPhoneNumberValidator::PhoneNumberIsValid(
	const char *pszPhoneNumber)
{
	bool bIsValid = false;

	VRSDialStringCheck(pszPhoneNumber, &bIsValid);

	if (pszPhoneNumber &&
		(0 == strcmp (pszPhoneNumber, SUICIDE_LIFELINE) ||
		 0 == strcmp (pszPhoneNumber, CustomerService3DigitNumber)))
	{
		bIsValid = true;
	}

	if (!bIsValid)
	{
		if (m_PhoneNumbersWithRegionCode->Match (pszPhoneNumber)
			|| m_PhoneNumbersWithoutRegionCode->Match (pszPhoneNumber)
			|| m_PhoneNumbersWithoutRegionCodeAndExplicitExt->Match (pszPhoneNumber)
	      //|| m_PhoneNumberWithoutAreaCode->Match (pszPhoneNumber)
			|| m_PhoneNumberWithoutAreaCodeAndExplicitExt->Match (pszPhoneNumber))
		{
			bIsValid = true;
		}
	}

	return bIsValid;
}

/*! brief Check if the provided phone number string is one of our support numbers
* 
*/
bool CstiPhoneNumberValidator::PhoneNumberIsSupport(
	std::string dialString)
{
	// Unfortunately the only way we can identify a CIR call is if we compare it
	// to all known CIR numbers. After a number is retired, we still want to
	// keep it in this list for a while just in case a user has the old number 
	// saved somewhere and tries to use it.
	const std::vector<const char *> cirNumbers =
	{
		"8667566729",
		"8013868500",	// OBSOLETE
		"8017587588",	// OBSOLETE
		"cir.svrs.tv"	// OBSOLETE
	};

	for (auto &&number: cirNumbers)
	{
		if (DialStringContains(dialString, number))
		{
			return true;
		}
	}

	// 611 must match exactly
	if (dialString == CustomerService3DigitNumber)
	{
		return true;
	}

	return false;
}

/*! brief Helper to check if a dial string URI contains a given phone number.
* 
*/
bool CstiPhoneNumberValidator::DialStringContains (std::string dialString, const char* phoneNumber)
{
	return dialString.find (phoneNumber) != std::string::npos;
}

/*!\brief Adds a '1' and an area code if required
 *
 * If we decide the dial string is a VRS number, then we do nothing and
 * return stiRESULT_INVALID_PARAMETER.  Otherwise, we see if the number
 * is missing a '1' or an area code and add them.
 */
stiHResult CstiPhoneNumberValidator::PhoneNumberReformat(
	const std::string &oldPhoneNumber,
	std::string &newPhoneNumber)
{
    bool bIsVRS = false;
    return PhoneNumberReformat(oldPhoneNumber, newPhoneNumber, &bIsVRS);
}

/*!\brief Adds a '1' and an area code if required
 *
 * If we decide the dial string is a VRS number, then we do nothing and
 * return stiRESULT_INVALID_PARAMETER.  Otherwise, we see if the number
 * is missing a '1' or an area code and add them.
 */
stiHResult CstiPhoneNumberValidator::PhoneNumberReformat(
	const std::string &oldPhoneNumber,
	std::string &newPhoneNumber,
	bool *pbIsVRS)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	VRSDialStringCheck(oldPhoneNumber.c_str (), pbIsVRS);

	if (*pbIsVRS ||
		oldPhoneNumber == CustomerService3DigitNumber ||
		oldPhoneNumber == SUICIDE_LIFELINE)
	{
		//
		// The number is the SVRS Espanol dial string without a '1' in front, so add the '1'
		//
		if (oldPhoneNumber == stiSVRS_ESPANOL_PHONENUMBER_WITHOUT_COUNTRY_CODE)
		{
			//
			// Place the region code into the new phone number buffer.
			// Put the digits from the phone number passed in into the new phone number buffer.
			//
			newPhoneNumber = "1";
			newPhoneNumber += oldPhoneNumber;
		}
		else
		{
			//
			// The number is a VRS dial string, or 988, so just copy it to the new buffer.
			//
			newPhoneNumber = oldPhoneNumber;
		}
	}
	else
	{
		// Do we have a '1' in front?
		if (m_PhoneNumbersWithRegionCode->Match (oldPhoneNumber))
		{
			//
			// Put the digits from the phone number passed in into the new phone number buffer.
			//
			StripNonDigits (oldPhoneNumber, newPhoneNumber);
		}
		// Do we at least have an area code?
		else if (m_PhoneNumbersWithoutRegionCode->Match(oldPhoneNumber)
			|| m_PhoneNumbersWithoutRegionCodeAndExplicitExt->Match(oldPhoneNumber))
		{
			//
			// Place the region code into the new phone number buffer.
			//
			newPhoneNumber = "1";

			//
			// Put the digits from the phone number passed in into the new phone number buffer.
			//
			std::string tmpPhoneNumber;
			StripNonDigits (oldPhoneNumber, tmpPhoneNumber);
			newPhoneNumber += tmpPhoneNumber;
		}
		// We need to add an area code too
		else if (m_PhoneNumberWithoutAreaCodeAndExplicitExt->Match (oldPhoneNumber)
			/*|| m_PhoneNumberWithoutAreaCode->Match(pszOldPhoneNumber)*/)
		{
			//
			// If the area code is not set then issue an error.
			//
			stiTESTCOND (m_szAreaCode[0] != '\0', stiRESULT_ERROR);

			//
			// Place the region code and area code into the new phone number buffer.
			//
			newPhoneNumber = "1";
			newPhoneNumber += m_szAreaCode;

			//
			// Put the digits from the phone number passed in into the new phone number buffer.
			//
			std::string tmpPhoneNumber;
			StripNonDigits (oldPhoneNumber, tmpPhoneNumber);
			newPhoneNumber += tmpPhoneNumber;
		}
		else
		{
			// Who knows what they typed...
			stiTESTCOND_NOLOG(false, stiRESULT_NOT_A_PHONE_NUMBER);
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Sets local area code
 *
 * Sets local area code so that we can reformat phone numbers correctly.
 */
void CstiPhoneNumberValidator::LocalAreaCodeSet(
    const char *pszAreaCode)
{
    if (pszAreaCode)
    {
    	strncpy (m_szAreaCode, pszAreaCode, g_nAREA_CODE_LENGTH);
    	m_szAreaCode[g_nAREA_CODE_LENGTH] = '\0';
    }
}

/*!\brief Checks a dial string against the list of VRS dial strings.
 *
 * If the dial string matches one of the VRS dial strings then the
 * the return value is set to true, otherwise it is set to false.
 */
stiHResult CstiPhoneNumberValidator::VRSDialStringCheck (
	const char *pszDialString,			///< The dial string to check.
	bool *pbIsVRSDialString)			///< Is set to true if it is in the list, false otherwise.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = false;

	for (unsigned int i = 0; i < m_VRSDialStrings.size (); i++)
	{
		bFound = m_VRSDialStrings[i]->Match(pszDialString);

		if (bFound)
		{
			break;
		}
	}

	*pbIsVRSDialString = bFound;

	return hResult;
}
