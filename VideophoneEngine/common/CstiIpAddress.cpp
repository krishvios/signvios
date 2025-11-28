/*!
* \file CstiIpAddress.cpp
*
* This Module contains a class used for analysis of ip address strings.
* It does things like validation, CIDR masks, and splitting ip and port information.
* Works with both IPv4 and IPv6 addresses.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/

//
// Includes
//
#include "CstiIpAddress.h"
#include <cstring>
#include "stiSVX.h"
#include <cstdio>
#include "stiTrace.h"

//
// Constants
//

// DBG_ERR_MSG is a tool to add file/line infromation to an stiTrace
// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
// TODO: add a debug tool
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (0, stiTrace (fmt "\n", ##__VA_ARGS__); )
#define DBG_ERR_MSG(fmt,...) stiTrace (HERE " ERROR: " fmt "\n", ##__VA_ARGS__)

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
// Locals
//

//
// Function Definitions
//


/*! \brief Construct an object with an initial address specified by the string parameter
 *
 */
CstiIpAddress::CstiIpAddress (
	const std::string &addressString)
{
	assign (addressString);
}


/*! \brief Clear the contents.
* 
* NOTE: that this places the address in the invalid state.
* To make it valid you need to assign() a new value to it.
*
* \retval nothing
*/
void CstiIpAddress::clear ()
{
	m_eType = estiTYPE_IPV4;
	memset (m_anValues, 0, sizeof (m_anValues));
	m_bValid = false;
	m_nPort = 0;
	m_nZone = 0;
	m_OriginalAddress.clear ();

}


/*! \brief Assigns a text-representation of an ip address to this class.
*
* Can accept IPv4 or IPv6 addresses, with or without port number attached.
*
* Performs validation as it goes, and will set the class' valid indicator accordingly.
*/
CstiIpAddress& CstiIpAddress::operator=(const std::string &addressString)
{
	if (!addressString.empty ())
	{
		assign (addressString);
	}
	else
	{
		clear ();
	}

	return *this;
}


/*! \brief Assigns another ip address to this class.
*
* \retval stiHResult
*/
stiHResult CstiIpAddress::assign (const CstiIpAddress &Other)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_eType = Other.m_eType;
	for (int i = 0; i < NumValuesGet (); i++)
	{
		m_anValues[i] = Other.m_anValues[i];
	}
	m_nPort = Other.m_nPort;
	m_nZone = Other.m_nZone;
	m_OriginalAddress = Other.m_OriginalAddress;
	m_bValid = Other.m_bValid;
	
	return hResult;
}


/*! \brief Assigns a text-representation of an ip address to this class.
* 
* Can accept IPv4 or IPv6 addresses, with or without port number attached.
* 
* Performs validation as it goes, and will return an error if the ip is invalid.
*
* \retval stiHResult
*/
stiHResult CstiIpAddress::assign (
	const std::string &addressString) //!< The string representing an ip address
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nColonCount = 0;
	bool bIsIPv4 = true;
	bool bIsIPv6 = true;
	clear ();
	
	stiTESTCOND (!addressString.empty (), stiRESULT_ERROR);
	
	// quickly disposition the string
	// this does not completely imply validity.  we need to call assignIPv4 or assignIPv6 for that
	for (int i = 0; addressString[i]; i++)
	{
		if (addressString[i] < '0' || addressString[i] > '9')
		{
			if ((addressString[i] >= 'a' && addressString[i] <= 'f') || (addressString[i] >= 'A' && addressString[i] <= 'F'))
			{
				bIsIPv4 = false;	
			}
			else if (addressString[i] == '.')
			{
				bIsIPv6 = false;
			}
			else if (addressString[i] == ':')
			{
				nColonCount++;
				if (nColonCount > 1)
				{
					bIsIPv4 = false;	
				}
			}
			else if (addressString[i] == '[' || addressString[i] == ']' || addressString[i] == '%')
			{
				bIsIPv4 = false;	
			}
			else
			{
				bIsIPv4 = false;
				bIsIPv6 = false;
				break;
			}
		}
	}
	
	if (bIsIPv4 && !bIsIPv6)
	{
		hResult = assignIPv4 (addressString.c_str ());
		stiTESTCOND_NOLOG (!stiIS_ERROR (hResult), hResult);
	}
	else if (bIsIPv6 && !bIsIPv4)
	{
		hResult = assignIPv6 (addressString.c_str ());
		stiTESTCOND_NOLOG (!stiIS_ERROR (hResult), hResult);
	}
	else
	{
		DBG_MSG ("Unrecongizable ip address type \"%s\"", addressString.c_str ());
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Assigns a text-representation of an ip address to this class.
* 
* Can accept only IPv4 addresses, with or without port number attached.
* 
* Performs validation as it goes, and will return an error if the ip is invalid.
*
* \retval stiHResult
*/
stiHResult CstiIpAddress::assignIPv4 (
	const char *pszAddressString) //!< The string representing an IPv4 address
{
	char *pszOctets;
	char *pszIpPort;
	unsigned int nValuesIdx;
	int nValue;
	const int numValues {4};
	char *savePtr = nullptr;

	clear ();
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// create a local duplicate to work on
	auto pszAddress = new char[strlen (pszAddressString) + 1];
	char *pszAlloc = pszAddress; // keep the m_OriginalAddress pointer for delete[]
	strcpy (pszAddress, pszAddressString);
	
	// look for trailing tokens
	switch (pszAddress[strlen(pszAddress)-1])
	{
		case  ':':
			DBG_MSG ("Port missing.");
			stiTHROW_NOLOG (stiRESULT_ERROR);
			break;
			
		case '.':
			DBG_MSG ("Octet missing.");
			stiTHROW_NOLOG (stiRESULT_ERROR);
			break;
	}
	
	// split off any port
	while(*pszAddress==' ')
	{
		pszAddress++;
	}

	pszIpPort = strtok_r (pszAddress, ":", &savePtr);
	if (pszIpPort)
	{
		pszAddress = pszIpPort; // keep the first half
		pszIpPort = strtok_r (nullptr, ":", &savePtr);
		long nValue = 0;
		if (pszIpPort)
		{
			nValue = atol (pszIpPort);
			if (nValue > 65535 || nValue < 1)
			{
				DBG_MSG ("Invalid port \"%ld\"!", nValue);
				stiTHROW_NOLOG (stiRESULT_ERROR);
			}
		}
		m_nPort = nValue;
	}
	
	// split apart the ip address
	pszOctets = strtok_r (pszAddress, ".", &savePtr);
	nValuesIdx = 0;
	while (pszOctets)
	{	
		if (*pszOctets < '0' || *pszOctets > '9')
		{
			DBG_MSG ("Bad octet %s", pszOctets);
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
		nValue = atoi (pszOctets);
		if (nValue > 255 || nValue < 0)
		{
			DBG_MSG ("Bad octet %d", nValue);
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
		
		m_anValues[nValuesIdx % numValues] = nValue;
		nValuesIdx++;
		pszOctets = strtok_r (nullptr, ".", &savePtr);
	}
	
	if (nValuesIdx != numValues)
	{
		DBG_MSG ("Invalid address! Wrong size! (%d sections)", nValuesIdx);
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}
	
	m_OriginalAddress.assign (pszAddressString);
	m_eType = estiTYPE_IPV4;
	m_bValid = true;
	
STI_BAIL:

	delete[] pszAlloc;

	return hResult;
}


/*! \brief Assigns a text-representation of an ip address to this class.
* 
* Can accept only IPv6 addresses, with or without port number attached.
* 
* Performs validation as it goes, and will return an error if the ip is invalid.
*
* \retval stiHResult
*/

stiHResult CstiIpAddress::assignIPv6 (
	const char *pszAddressString) //!< The string representing an IPv6 address
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	clear ();
	unsigned int nValuesIdx = 0;
	char *pszPreColonColon;
	char *pszIpZone;
	char *pszColonColon;
	char *pszIpPort;
	const int numValues {8};
	char *savePtr = nullptr;
	
	// create a local duplicate to work on
	auto pszAddress = new char[strlen (pszAddressString) + 1];
	char *pszAlloc = pszAddress; // keep the m_OriginalAddress pointer for delete[]
	strcpy (pszAddress, pszAddressString);
	
	// first some sanity checks
	if (strlen(pszAddress)<3)
	{
		DBG_MSG ("Too little data.");
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}

	if (pszAddress[strlen(pszAddress)-1] == ':' && pszAddress[strlen(pszAddress)-2] != ':')
	{
		DBG_MSG ("Section missing.");
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}
	
	// split off any port
	while(*pszAddress == '[' || *pszAddress==' ')
	{
		pszAddress++;
	}
	pszIpPort = strtok_r (pszAddress, "]", &savePtr);
	if (pszIpPort)
	{
		pszAddress = pszIpPort; // keep the first half
		pszIpPort = strtok_r (nullptr, "]", &savePtr);
		if (pszIpPort)
		{
			while (*pszIpPort==' ')
			{
				pszIpPort++;
			}
			// split off any zone outside
			if (*pszIpPort=='%')
			{
				pszIpPort++;
				m_nZone = atoi (pszIpPort);
				while (*pszIpPort <= '9' && *pszIpPort >= '0')
				{
					pszIpPort++;
				}
			}
			// find the port
			if (*pszIpPort==':')
			{
				pszIpPort++;
				long nValue = atol (pszIpPort);
				if (nValue > 65535 || nValue < 1)
				{
					DBG_MSG ("Invalid port \"%ld\"!", nValue);
					stiTHROW_NOLOG (stiRESULT_ERROR);
				}
				m_nPort = nValue;
			}
		}
	}
	// split off any zone inside
	pszIpZone = strtok_r (pszAddress, "%", &savePtr);
	if (pszIpZone)
	{
		pszAddress = pszIpZone; // keep the first half
		pszIpZone = strtok_r (nullptr, "%", &savePtr);
		if (pszIpZone)
		{
			m_nZone = atoi (pszIpZone);
		}
	}
	// expand ::
	pszColonColon = strstr (pszAddress, "::");
	if (pszColonColon)
	{
		// point to the second half
		pszColonColon[0] = '\0';
		pszColonColon += 2;
		if (strstr (pszColonColon, "::"))
		{
			DBG_MSG ("Invalid address!  Multiple ::");
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
	}
	pszPreColonColon = strtok_r (pszAddress, ":", &savePtr);
	while (pszPreColonColon)
	{
		int nValue;
		sscanf (pszPreColonColon, "%x", &nValue);
		if (nValue > 0xffff || nValue < 0x0000)
		{
			DBG_MSG ("Invalid value \"%s\"!", pszPreColonColon);
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
		m_anValues[nValuesIdx % numValues] = nValue;
		nValuesIdx++;
		pszPreColonColon = strtok_r (nullptr, ":", &savePtr);
	}
	if (pszColonColon)
	{
		// count how many sections are after ::
		int nNumSections = 0;
		if (pszColonColon[0])
		{
			nNumSections++;
			for (int i = 0; pszColonColon[i]; i++)
			{
				if (pszColonColon[i] == ':')
				{
					nNumSections++;
				}
			}
		}
		
		if (nValuesIdx + nNumSections >= numValues)
		{
			DBG_MSG ("Invalid address!  Too long! (%d sections)", nValuesIdx + nNumSections + 1);
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
		
		// fill in the 0000 sections in between
		for (; nValuesIdx + nNumSections < numValues; nValuesIdx++)
		{
			m_anValues[nValuesIdx % numValues] = 0;
		}
		
		// parse the stuff after the ::
		char *pszPostColonColon = strtok_r (pszColonColon, ":", &savePtr);
		while (pszPostColonColon)
		{
			int nValue;
			sscanf (pszPostColonColon, "%x", &nValue);
			if (nValue > 0xffff || nValue < 0x0000)
			{
				DBG_MSG ("Invalid value \"%s\"!", pszPostColonColon);
				stiTHROW_NOLOG (stiRESULT_ERROR);
			}
			m_anValues[nValuesIdx % numValues] = nValue;
			nValuesIdx++;
			pszPostColonColon = strtok_r (nullptr, ":", &savePtr);
		}
	}
	if (nValuesIdx != numValues)
	{
		DBG_MSG ("Invalid address! Too short! (%d sections)", nValuesIdx);
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}
	
	m_OriginalAddress.assign (pszAddressString);
	m_eType = estiTYPE_IPV6;
	m_bValid = true;
	
STI_BAIL:

	delete[] pszAlloc;
	
	return hResult;
}


/*! \brief Returns the number of values available to the current ip address type. 
* 
* \retval num values
*/
int CstiIpAddress::NumValuesGet() const
{
	return m_bValid ? ((m_eType==estiTYPE_IPV4) ? 4 : 8) : 0;
}


/*! \brief Returns a properly-formatted string representation of the ip address. 
* 
* For IPv6, this can give you either properly condensed form or fully-expanded.
* 
* You can also specfify whether you want any port (and zone) included in the 
* result, or just the ip address
* 
* "Condensing" means specifically:
*	1) all leading zeros for (each section are omitted unless that section is exactly 0
*	2) two or more sections of all zeros can be condensed with a double-colon
*	3) the double-colon is used where it condenses the greatest number of zeroes
*	4) in the event of a tie, the double-colon goes to the leftmost run of zeroes
*
* IPv6 hex digits a-f are always returned in lower case
*
* \retval nothing
*/
std::string CstiIpAddress::AddressGet (
	unsigned int mFormatting //!< how the resulting address should be formatted
) const
{
	switch (m_eType)
	{
		case estiTYPE_IPV6:
			return AddressGetIPv6 (mFormatting);
		case estiTYPE_IPV4:
			return AddressGetIPv4 (mFormatting);
	}

	return {};
}


/*! \brief Compares this address to another, including any port/zone info. 
*
* \retval bool
*/
bool CstiIpAddress::operator==(const CstiIpAddress& other)
{
	bool bMatches = false;

	if (m_bValid && other.m_bValid)
	{
		if (m_eType == other.m_eType)
		{
			if (m_nPort == other.m_nPort)
			{
				if (m_nZone == other.m_nZone)
				{
					bMatches = true;
					const int numValues {NumValuesGet ()};
					for (int i = 0; i < numValues; i++)
					{
						if (m_anValues[i] != other.m_anValues[i])
						{
							bMatches = false;
							DBG_MSG ("different values");
							break;
						}
					}
				}
				else
				{
					DBG_MSG ("different zones");
				}
			}
			else
			{
				DBG_MSG ("different ports");
			}
		}
		else
		{
			DBG_MSG ("different types");
		}
	}
	else
	{
		DBG_MSG ("invalid");
	}
	
	return bMatches;
}


/*! \brief Compares this address to another, including any port/zone info. 
*
* \retval bool
*/
bool CstiIpAddress::operator==(const std::string &other)
{
	CstiIpAddress otherParser;
	otherParser.assign (other);
	
	return otherParser == *this;
}


/*! \brief Returns a properly-formatted string representation of the ip address. 
* 
* You can also specfify whether you want any port included in the 
* result, or just the ip address
*
* \retval nothing
*/
std::string CstiIpAddress::AddressGetIPv4 (
	unsigned int mFormatting //!< how the resulting address should be formatted
) const
{
	char szTmpBuf[7];
	const int numValues {NumValuesGet ()};
	std::string result;
	
	for (int i = 0; i < numValues; i++)
	{
		sprintf (szTmpBuf, "%d", m_anValues[i]);
		if (i != 0)
		{
			result.append (".");
		}
		result.append (szTmpBuf);
	}
	
	if (((eFORMAT_ALLOW_PORT & mFormatting) == eFORMAT_ALLOW_PORT) && m_nPort > 0)
	{
		sprintf (szTmpBuf, ":%d", m_nPort);
		result.append (szTmpBuf);
	}

	return result;
}


/*! \brief Returns a properly-formatted string representation of the ip address. 
* 
* This can give you either properly condensed form or fully-expanded.
* 
* You can also specfify whether you want any port (and zone) included in the 
* result, or just the ip address
* 
* "Condensing" means specifically:
*	1) all leading zeros for (each section are omitted unless that section is exactly 0
*	2) two or more sections of all zeros can be condensed with a double-colon
*	3) the double-colon is used where it condenses the greatest number of zeroes
*	4) in the event of a tie, the double-colon goes to the leftmost run of zeroes
*
* hex digits a-f are always returned in lower case
*
* \retval nothing
*/

std::string CstiIpAddress::AddressGetIPv6 (
	unsigned int mFormatting //!< how the resulting address should be formatted
) const
{
	bool bFirst = true;
	char szTmpBuf[8];
	const int numValues {NumValuesGet ()};
	std::string result;
	
	if ((eFORMAT_NO_CONDENSE & mFormatting) != eFORMAT_NO_CONDENSE)
	{ // use condensed form
		// find where all the zeroes are
		int selected_start=-1;
		int selected_run=1; // must be more than one section to keep it
		int current_start=-1;
		int current_run=0;
		
		for (int i = 0; i < numValues; i++)
		{
			if (current_start != -1)
			{
				if (m_anValues[i] != 0)
				{
					if (current_run > selected_run)
					{
						selected_start = current_start;
						selected_run = current_run;
					}
					current_start = -1;
				}
				else
				{
					current_run++;
				}
			}
			else if (m_anValues[i] == 0)
			{
				current_run = 1;
				current_start = i;
			}
		}
		if (current_start != -1) // ended in 0
		{
			if (current_run > selected_run)
			{
				selected_start = current_start;
				selected_run = current_run;
			}
		}
		// condense it
		if (selected_start == -1)
		{
			// no condensing possible. just print everyting!
			for (int i = 0; i < numValues; i++)
			{
				sprintf (szTmpBuf, "%x", m_anValues[i]);
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					result.append (":");
				}
				result.append (szTmpBuf);
			}
		}
		else  // we have condensing at selected_start
		{
			int i = 0;
			if (selected_start != 0)
			{
				for (; (int)i < selected_start; i++)
				{
					sprintf (szTmpBuf, "%x", m_anValues[i]);
					if (bFirst)
					{
						bFirst = false;
					}
					else
					{
						result.append (":");
					}
					result.append (szTmpBuf);
				}
				result.append (":");
				i += selected_run;
			}
			else
			{
				 // one more entry to make ::
				if (bFirst)
				{
					bFirst = false;
					result.append (":");
				}
				else
				{
					result.append ("::");
				}
				i = selected_run;
			}
			// add the stuff at the end
			if (i != numValues)
			{
				for (; i < numValues; i++)
				{
					sprintf (szTmpBuf, "%x", m_anValues[i]);
					if (bFirst)
					{
						bFirst = false;
					}
					else
					{
						result.append (":");
					}
					result.append (szTmpBuf);
				}
			}
			else
			{
				result.append (":"); // one more entry for trailing ::
			}
		}
	}
	else
	{ // use uncondensed form
		// this makes our job simpler.  just print everyting!
		for (int i = 0; i < numValues; i++)
		{
			sprintf (szTmpBuf, "%04x", m_anValues[i]);
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				result.append (":");
			}
			result.append (szTmpBuf);
		}
	}
	
	// Add on zone if requested and available
	if (((eFORMAT_ALLOW_ZONE & mFormatting) == eFORMAT_ALLOW_ZONE) && m_nZone != 0)
	{
		sprintf (szTmpBuf, "%%%d", m_nZone);
		result.append (szTmpBuf);
	}
	
	// Add on port if requested and available
	if (((eFORMAT_ALLOW_PORT & mFormatting) == eFORMAT_ALLOW_PORT) && m_nPort != 0)
	{
		result.insert (0, "[");
		sprintf (szTmpBuf, "]:%d", m_nPort);
		result.append (szTmpBuf);
	}
	else if ((eFORMAT_ALWAYS_USE_BRACKETS & mFormatting) == eFORMAT_ALWAYS_USE_BRACKETS)
	{ // If sqare brackets are requested and not already added due to adding the port
		result.insert (0, "[");
		result.append ("]");
	}
	
	return result;
}

bool CstiIpAddress::IPv4AddressIsPrivate ()
{
	if (m_eType != EstiIpAddressType::estiTYPE_IPV4)
	{
		return false;
	}
	// Addresses are stored in reverse order
	else if (m_anValues[0] == 10)
	{
		return true;
	}
	else if (m_anValues[0] == 172 && m_anValues[1] >= 16 && m_anValues[1] <= 31)
	{
		return true;
	}
	else if (m_anValues[0] == 192 && m_anValues[1] == 168)
	{
		return true;
	}
	else
	{
		return false;
	}
}
