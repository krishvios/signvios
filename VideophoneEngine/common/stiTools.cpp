/*!
* \file stiTools.cpp
*
* This Module contains miscellaneous functions that are used as supporting
* functions for carrying out the necessary duties of the overall
* project.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/

//
// Includes
//
#include "stiTools.h"
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdio>
#include "stiTrace.h"
#include "stiOS.h"
#include "CstiHostNameResolver.h"
#include "CstiRegEx.h"
#include "CstiIpAddress.h"
#include <openssl/evp.h>
#include "stiBase64.h"

//
// Constants
//
static const uint32_t g_unNumberOfSecondsInDay = 60 * 60 * 24;
static const uint32_t g_unNumberOfSecondsInHour = 60 * 60; // The number of seconds in an hour.
static const uint32_t g_unNumberOfSecondsInMinute = 60; // The number of seconds in a minute.

static const char g_szRegexURL[] = "^(ftp|http|file)://([^/]+)(/.*)";
#define NAT64_PREFIX "64:ff9b:" // well-known prefix for NAT64 defined in RFC 6052
#define NAT64_PREFIX_TMOBILE "2607:7700:" // T-Mobile prefix for NAT64
#define NAT64_PREFIX_TMOBILE_SPRINT_BOOST "2600:0:" // Prefix seen used by T-Mobile networks used by various providers.

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


/*! \brief Returns the NAL Unit and its size.
*
* \param pun8Buffer A pointer to the buffer to search
* \param un32BufferLength the length of the buffer pointed to by pun8Buffer.
* \param ppun8NALUnit A pointer used to return the location of the NAL unit.
* \param pun32HeaderLength A pointer used to return the length of the NAL unit header.
*/
void ByteStreamNALUnitFind (
	uint8_t *pun8Buffer,
	uint32_t un32BufferLength,
	uint8_t **ppun8NALUnit,
	uint32_t *pun32HeaderLength)
{
	uint8_t *pun8NALUnit = nullptr;
	uint32_t un32HeaderLength = 0;
	uint8_t *pun8BufferEnd = pun8Buffer + un32BufferLength - 4; // Subtract off 4 bytes so we don't read past the end of the buffer.

	//
	// Loop through the buffer looking for three or four byte headers
	// or until we come to the end of the buffer.
	//
	for (; pun8Buffer < pun8BufferEnd; ++pun8Buffer)
	{
		if (pun8Buffer[0] == 0
		 && pun8Buffer[1] == 0)
		{
			if (pun8Buffer[2] == 0
			 && pun8Buffer[3] == 1)
			{
				//
				// Four byte header found
				//
				pun8NALUnit = pun8Buffer;
				un32HeaderLength = 4;
				break;
			}
			else if (pun8Buffer[2] == 1)
			{
				//
				// Three byte header found
				//
				pun8NALUnit = pun8Buffer;
				un32HeaderLength = 3;
				break;
			}
		}
	}

	*ppun8NALUnit = pun8NALUnit;
	*pun32HeaderLength = un32HeaderLength;
}

/*! \brief Checks if szNumber is a valid canonical phone number.
*
* Valid canonical phone numbers can only contain the characters '0', '1', '2',
* '3', '4', '5', '6', '7', '8', '9', '+', '-', '(', ')' and ' '.
*
* \retval estiTRUE If szNumber is a canonical phone number.
* \retval estiFALSE Otherwise.
*/
EstiBool CanonicalNumberValidate (
	const char* szNumber)    //!< The input string which may be a canonical phone number.
{
	stiLOG_ENTRY_NAME (CanonicalNumberValidate);

	EstiBool bValidate = estiTRUE;

	// Did we get a valid pointer?
	if (nullptr == szNumber)
	{
		bValidate = estiFALSE;
	}  // end if
	else
	{
		// A blank string is invalid.
		if (0 == strlen (szNumber))
		{
			bValidate = estiFALSE;
		}  // end if
		else
		{
			unsigned int nStartPos = 0;

			// Only the first character can be the plus "+" symbol.
			if ('+' == szNumber[0])
			{
				// Initialize nStartPos to 1 to skip the "+" symbol
				nStartPos = 1;
			} // end if
			// Check each character and make sure it's a valid character.
			for (size_t i = nStartPos; i < strlen (szNumber); i++)
			{
				if (!isdigit(szNumber[i]) &&
					'-' != szNumber[i] && '(' != szNumber[i] &&
					')' != szNumber[i] && ' ' != szNumber[i])
				{
					bValidate = estiFALSE;
					break;
				} // end if
			} // end for
		}  // end else
	}  // end else

	return (bValidate);
} // end CanonicalNumberValidate


/*! \brief Checks if szAlias is a valid E164 alias (phone number).
*
* Valid E164 aliases can only contain the characters '0', '1', '2', '3',
* '4', '5', '6', '7', '8', '9', '#', '*' and ',' (comma).
*
* \retval estiTRUE If szAlias is an E164 alias.
* \retval estiFALSE Otherwise.
*/
EstiBool E164AliasValidate (
	const char* szAlias) //!< The input string which may be an E164 alias.
{
	stiLOG_ENTRY_NAME (E164AliasValidate);

	EstiBool bE164AliasValidate = estiTRUE;

	// Did we get a valid pointer?
	if (nullptr == szAlias)
	{
		bE164AliasValidate = estiFALSE;
	}  // end if
	else
	{
		// A blank string is invalid.
		if (0 == strlen (szAlias))
		{
			bE164AliasValidate = estiFALSE;
		}  // end if
		else
		{
			// Check each character and make sure it's a valid character.
			for (int i = 0; i < (int) strlen (szAlias); i++)
			{
				if (!isdigit(szAlias[i]) &&
					'#' != szAlias[i] && '*' != szAlias[i] &&
					',' != szAlias[i])
				{
					bE164AliasValidate = estiFALSE;
					break;
				} // end if
			} // end for
		}  // end else
	}  // end else

	return (bE164AliasValidate);
} // end E164AliasValidate


/*! \brief Check if szAddress is a valid Email address.
*
* The email format that is being checked is "X@X.X" where "X" is any non
* empty string.
*
* \retval estiTRUE If szAddress is an Email address.
* \retval estiFALSE Otherwise
*/
EstiBool EmailAddressValidate (
	const char* pszAddress) //!< The input string which may be an email address.
{
	stiLOG_ENTRY_NAME (EmailAddressValidate);

	size_t    AtLength = 0;
	int nDotCount = 0;
	const char *pszAtPosition;
	const char *pszDotPosition;
	const char *pszDomain;
	EstiBool    bEmailAddressValidate = estiFALSE;  // default to not an Email
													// Address

	// Did we get a valid pointer?
	if (nullptr != pszAddress)
	{
		// Is there a '@' in the string?
		if (nullptr != (pszAtPosition = strchr (pszAddress, '@')))
		{
			// Is this segment greater than 0 characters?
			AtLength = pszAtPosition - pszAddress;
			if (0 < AtLength)
			{
				//
				// Make sure there isn't another '@' symbol.
				//
				if (strchr (pszAtPosition + 1, '@') == nullptr)
				{
					//
					// Make sure each segment of the domain is sepearator by a dot.
					//
					pszDomain = pszAtPosition + 1;
					
					for (;;)
					{
						pszDotPosition = strchr (pszDomain, '.');

						// Is there a '.' in the string?
						if (nullptr == pszDotPosition)
						{
							//
							// The length must be greater than zero (dots at the
							// end are not allowed) and there must have been one
							// or more dots found.
							//
							if (strlen (pszDomain) > 0 && nDotCount > 0)
							{
								bEmailAddressValidate = estiTRUE;
							}
							
							break;
						}
						else
						{
							++nDotCount;
							
							// Is this segment greater than 0 characters?
							if (pszDotPosition - pszDomain == 0)
							{
								break;
							}
							
							pszDomain = pszDotPosition + 1;
							
						} // end if
					}
				}
			} // end if
		} // end if
	}  // end if

	return bEmailAddressValidate;
} // end EmailAddressValidate


/*! \brief Check if szAddress is a valid IPv4 address.
*
* The IP address format is "xxx.xxx.xxx.xxx" where "xxx" is any number
* between 0 and 255.
* \retval estiTRUE If szAddress is an IP address.
* \retval estiFALSE Otherwise.
*/
EstiBool IPv4AddressValidate (
	const char *pszAddress)  //!< The address sent in.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;

	hResult = address.assign (pszAddress);

	// Must be valid AND not have a port value
	return ((!stiIS_ERROR (hResult)) &&
		(address.PortGet () <= 0) &&
		address.IpAddressTypeGet () == estiTYPE_IPV4)
		? estiTRUE : estiFALSE;
}


/*! \brief Check if szAddress is a valid IP address.
*
* The IP address and port format is "xxx.xxx.xxx.xxx:ppp" where "xxx" is any number
* between 0 and 255 and ppp, if present, is any number from 0 to 65535.
* \retval estiTRUE If szAddress is an IP address with optional port.
* \retval estiFALSE Otherwise.
*/
EstiBool IPv4AddressValidateEx (
	const char *pszAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;

	hResult = address.assign (pszAddress);

	return ((!stiIS_ERROR (hResult) &&
		address.IpAddressTypeGet () == estiTYPE_IPV4)
		) ? estiTRUE : estiFALSE;
}


/*! \brief Check if szAddress is a valid IPv6 address.
*
* \retval estiTRUE If szAddress is an IP address.
* \retval estiFALSE Otherwise.
*/
EstiBool IPv6AddressValidate(const char *pszAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;

	hResult = address.assign (pszAddress);

	// Must be valid AND not have a port value
	return ((!stiIS_ERROR (hResult)) &&
		(address.PortGet () <= 0) &&
		address.IpAddressTypeGet () == estiTYPE_IPV6)
		? estiTRUE : estiFALSE;
}


/*! \brief Check if szAddress is a valid IPv6 address.
*
* \retval estiTRUE If szAddress is an IP address.
* \retval estiFALSE Otherwise.
*/
EstiBool IPv6AddressValidateEx(const char *pszAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;

	hResult = address.assign (pszAddress);

	return ((!stiIS_ERROR (hResult) &&
		address.IpAddressTypeGet () == estiTYPE_IPV6)
		) ? estiTRUE : estiFALSE;
}


/*! \brief Check if szAddress is a valid IP address.
*
* The IP address format is ipv4 or ipv6
*
* \retval estiTRUE If szAddress is an IP address.
* \retval estiFALSE Otherwise.
*/
EstiBool IPAddressValidate (
	const char *pszAddress)  //!< The address sent in.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;
	
	hResult = address.assign (pszAddress);
	
	// Must be valid AND not have a port value
	return ((!stiIS_ERROR (hResult)) && (address.PortGet () <= 0)) ? estiTRUE : estiFALSE;
} // end IPAddressValidate


/*! \brief Check if szAddress is a valid IP address.
*
* The IP address and port format is "xxx.xxx.xxx.xxx:ppp" or
* "[xx::xx]:ppp" where ppp, if present, is any number from 0 to 65535.
* \retval estiTRUE If szAddress is an IP address with optional port.
* \retval estiFALSE Otherwise.
*/
EstiBool IPAddressValidateEx (
	const char *pszAddress)
{	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress address;
	
	hResult = address.assign (pszAddress);
	
	return (!stiIS_ERROR (hResult)) ? estiTRUE : estiFALSE;
} // end IPAddressValidateEx


/*! \brief Converts IPv4 and IPv6 addresses between binary and text form
 *
 * Takes an addrinfo struct and returns a string containing the address.
 *
 * \return hResult
 */
stiHResult stiInetAddrToString (
	const addrinfo *addr,
	std::string *IPAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char szIPVAddress[stiIPV6_ADDRESS_LENGTH + 1];
	const char *pszIPSuccess = nullptr;
	void *sockAddr;
	
	if (addr->ai_family == AF_INET)
	{
		sockAddr = &((struct sockaddr_in *)addr->ai_addr)->sin_addr;
	}
	else if (addr->ai_family == AF_INET6)
	{
		sockAddr = &((struct sockaddr_in6 *)addr->ai_addr)->sin6_addr;
	}
	else
	{
		stiTHROW(stiRESULT_INVALID_PARAMETER);
	}
	
	pszIPSuccess = inet_ntop (addr->ai_family, sockAddr, szIPVAddress, sizeof (szIPVAddress));
	
	stiTESTCOND(pszIPSuccess, stiRESULT_ERROR);
	
	*IPAddress = szIPVAddress;
	
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Look up and resolve a DNS name to its IP address.
*
* This method does a DNS lookup to translate a name into an IP address.
*
* \return A std::string containing the IP address or empty if not found.
*/
stiHResult stiDNSGetHostByName (
	const char *pszName,  //!< The name to be resolved.
	const char *pszAltName, //!< The alternate name to be resolved
	std::string *pHost)//!< The returned result value
{
	stiLOG_ENTRY_NAME (stiDNSGetHostByName);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::vector<std::string> Results;
	CstiHostNameResolver *poResolver = CstiHostNameResolver::getInstance ();
	addrinfo *pstAddrInfo = nullptr;

	poResolver->Resolve (pszName, pszAltName, &pstAddrInfo);
	stiTESTCOND_NOLOG (nullptr != pstAddrInfo, stiRESULT_DNS_COULD_NOT_RESOLVE);

	hResult = poResolver->IpListFromAddrinfoCreate (pstAddrInfo, &Results);
	stiTESTCOND_NOLOG (!stiIS_ERROR (hResult) && !Results.empty(), stiRESULT_DNS_COULD_NOT_RESOLVE);
		
	stiDEBUG_TOOL (g_stiDNSDebug,
		stiTrace ("\tAddresses in Presentation Order:\n");
		for (auto it = Results.begin (); it != Results.end (); ++it)
		{
			stiTrace ("\t\t%s\n", it->c_str ());
		}  // end for
	); // end stiDEBUG_TOOL
		
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_IOS || APPLICATION == APP_DHV_ANDROID
	// Get a random selection from the list
	pHost->assign (Results.at (rand () % Results.size ()));
#else
	// Get the first element from the list
	pHost->assign (Results.at (0));
#endif

STI_BAIL:

	return hResult;
} // end stiDNSGetHostByName

/*! \brief Populate a sockaddr_storage for an IP address, port, and socket type.
 *
 * This method uses getaddrinfo which will automatically determine whether the IP address is IPv4 or IPv6
 *
 * \return A sockaddr_storage containing the sockaddr_in or sockaddr_in6 depending on the input address
 */
stiHResult stiSockAddrStorageCreate (
	const char *pszIPAddress,
	uint16_t un16Port,
	int eSockType,
	sockaddr_storage *pstSockAddrStorage)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	struct addrinfo addrInfoHints{}, *pstAddrInfo;
	char szPort[NI_MAXSERV];
	
	sprintf(szPort, "%d", un16Port);
	memset(&addrInfoHints, 0, sizeof(addrInfoHints));
	addrInfoHints.ai_family = AF_UNSPEC;
	addrInfoHints.ai_socktype = eSockType;
	addrInfoHints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
	
	nResult = getaddrinfo (pszIPAddress, szPort, &addrInfoHints, &pstAddrInfo);
	stiTESTCONDMSG (nResult == 0, stiRESULT_ERROR, "stiSockAddrStorageCreate - getaddrinfo: ERROR %d, \"%s\" on %s:%d\n", nResult, gai_strerror (nResult), pszIPAddress, un16Port);
	
	// Note, we're taking the first valid address, there may be more than one
	memcpy (pstSockAddrStorage, pstAddrInfo->ai_addr, pstAddrInfo->ai_addrlen);
	freeaddrinfo (pstAddrInfo);
	
STI_BAIL:
	
	return hResult;
}

/*! \brief Convert NAT64 IPv6 address to IPv4.
 *
 * This method converts IPv6 address provided by IPv4/IPv6 Translator to IPv4
 * by checking for well-known prefix defined in RFC 6052
 *
 * 12/27/2016: T-Mobile appears to be using the prefix 2607:7700 so check for that as well
 *
 * \return A std::string containing the mapped IPv4 address
 */
stiHResult stiMapIPv6AddressToIPv4 (std::string ipAddress, std::string *mappedIpAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Get rid of the [] at the start and end of the IPV6 address in order to try and convert it to IPV4.
	CstiIpAddress IpAddress (ipAddress);
	stiTESTCOND (IpAddress.ValidGet(), stiRESULT_ERROR);

	ipAddress = IpAddress.AddressGet (CstiIpAddress::eFORMAT_IP);

	if (IpAddress.IpAddressTypeGet() == estiTYPE_IPV6)
	{
		struct sockaddr_storage serverAddr{};
		hResult = stiSockAddrStorageCreate (ipAddress.c_str (), 0, SOCK_STREAM, &serverAddr);
		stiTESTRESULT ();

		if (serverAddr.ss_family == AF_INET6)
		{
			auto addr6 = (struct sockaddr_in6*)&serverAddr;
			auto addr = (unsigned char*)&addr6->sin6_addr;
			
			char szTemp[INET_ADDRSTRLEN];
			sprintf (szTemp, "%d.%d.%d.%d", addr[12], addr[13], addr[14], addr[15]);
			mappedIpAddress->assign (szTemp);
		}
	}
	else if (IpAddress.IpAddressTypeGet() == estiTYPE_IPV6)
	{
		stiASSERTMSG(false,"stiMapIPv6AddressToIPv4 asked to map unknown prefix. IPv6 = %s", ipAddress.c_str());
	}
	
STI_BAIL:
	
	return hResult;
}

/*! \brief Convert IPv4 address to IPv6.
 *
 * This method converts IPv4 address to IPv6 by asking getaddrinfo to resolve the IPv4 address.
 *
 * \return A std::string containing the mapped IPv4 address
 */
stiHResult stiMapIPv4AddressToIPv6 (const std::string& ipAddress, std::string *mappedIpAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct addrinfo addrInfoHints{}, *res0;
	int nResult;

	stiTESTCOND(mappedIpAddress != nullptr, stiRESULT_INVALID_PARAMETER);
	stiTESTCOND(IPv4AddressValidate(ipAddress.c_str()), stiRESULT_INVALID_PARAMETER);

	memset(&addrInfoHints, 0, sizeof(addrInfoHints));
	addrInfoHints.ai_family = AF_UNSPEC;
	addrInfoHints.ai_socktype = SOCK_STREAM;
	addrInfoHints.ai_flags = AI_V4MAPPED;
	nResult = getaddrinfo (ipAddress.c_str(), nullptr, &addrInfoHints, &res0);

	stiTESTCONDMSG (nResult == 0, stiRESULT_ERROR, "stiMapIPv4AddressToIPv6 - getaddrinfo: ERROR %d, \"%s\" on %s\n", nResult, gai_strerror (nResult), ipAddress.c_str());

	hResult = stiRESULT_ERROR;
	for (struct addrinfo *res = res0; res; res = res0->ai_next)
	{
		if (res->ai_addr->sa_family == AF_INET6)
		{
			hResult = stiInetAddrToString(res, mappedIpAddress);
			break;
		}
	}

	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

/*! \brief Look up and format our local IP address.
*
* \return hResult
*/
stiHResult stiGetLocalIp (
	std::string *pLocalIp,         //!< The buffer to place the IP address into.
	EstiIpAddressType eIpAddressType)
{
	stiLOG_ENTRY_NAME (stiGetLocalIp);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = stiOSGetIPAddress (pLocalIp, eIpAddressType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end stiGetLocalIp


/*! \brief Look up and format our network mask address.
*
* \return hResult
*/
stiHResult stiGetNetSubnetMaskAddress (
		std::string *pSubnetMask,         //!< The buffer to place the IP address into.
		EstiIpAddressType eIpAddressType)
{
	stiLOG_ENTRY_NAME (stiGetNetworkMask);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = stiOSGetNetworkMask (pSubnetMask, eIpAddressType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end stiGetNetSubnetMaskAddress


/*! \brief Look up and format our DNS address.
*
* \return hResult
*/
stiHResult stiGetDNSAddress (
	int nIndex,
	std::string *pDNSAddress, //!< The buffer to place the IP address into.
	EstiIpAddressType eIpAddressType)
{
	stiLOG_ENTRY_NAME (stiGetDNSAddress);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = stiOSGetDNSAddress (nIndex, pDNSAddress, eIpAddressType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end stiGetDNSAddress

/*! \brief Look up and format in dot notation our default gateway address.
*
* \return hResult
*/
stiHResult stiGetDefaultGatewayAddress (
		std::string *pDefaultGatewayAddress,         //!< The buffer to place the IP address into.
		EstiIpAddressType eIpAddressType)
{
	stiLOG_ENTRY_NAME (stiGetDefaultGatewayAddress);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = stiOSGetDefaultGatewayAddress (pDefaultGatewayAddress, eIpAddressType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end stiGetDefaultGatewayAddress


/*!
 * \brief Copy a file
 *
 * If `dstFileName` already exists, it is overwritten.
 *
 * \param srcFileName
 * \param dstFileName
 *
 * \return stiHResult
 */
stiHResult stiCopyFile (
	const std::string &srcFileName,
	const std::string &dstFileName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::ifstream src (srcFileName, std::ios::binary);
	std::ofstream dst (dstFileName, std::ios::binary | std::ios::trunc);

	stiTESTCONDMSG (src, stiRESULT_ERROR, "stiCopyFile unable to open source file")
	stiTESTCONDMSG (dst, stiRESULT_ERROR, "stiCopyFile unable to open destination file")

	dst << src.rdbuf ();

	stiTESTCONDMSG (dst, stiRESULT_ERROR, "stiCopyFile unable to write to destination file")

STI_BAIL:

	return hResult;
}


/*! \brief Compare two strings (case-insensitive).
*
* \retval Positive number to indicate that "first" would appear alphabetically
* after "last".
* \retval 0 indicates that the two strings are the same.
* \retval Negative number to indicate that "first" would appear alphabetically
* before "last".
*/
int stiStrICmp (
	const char *pString1, //!< The first string to compare.
	const char *pString2) //!< The second string to compare.
{
#ifdef WIN32
	return _stricmp (pString1, pString2);
#else
	return strcasecmp (pString1, pString2);
#endif
}


/*! \brief Compare up to the first "count" characters of two strings (case-insensitive).
*
* \retval Positive number to indicate that "first" would appear alphabetically
* after "last".
* \retval 0 indicates that the two strings are the same.
* \retval Negative number to indicate that "first" would appear alphabetically
* before "last".
*/
int stiStrNICmp (
	const char *pString1, //!< The first string to compare.
	const char *pString2,  //!< The second string to compare.
	size_t count)          //!< The maximum number of characters to compare.
{
#ifdef WIN32
	return _strnicmp (pString1, pString2, count);
#else
	return strncasecmp (pString1, pString2, count);
#endif
}


#ifdef WIN32
// Implement strptime under windows
static const char* kWeekFull[] =
{
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday"
};

static const char* kWeekAbbr[] =
{
	"Sun", "Mon", "Tue", "Wed",
	"Thu", "Fri", "Sat"
};

static const char* kMonthFull[] =
{
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

static const char* kMonthAbbr[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* _parse_num(const char* s, int low, int high, int* value)
{
	const char* p = s;
	for (*value = 0; *p != NULL && isdigit(*p); ++p)
	{
		*value = (*value) * 10 + static_cast<int>(*p) - static_cast<int>('0');
	}

	if (p == s || *value < low || *value > high)
	{
		return NULL;
	}

	return p;
}

static char* _strptime(const char *s, const char *format, struct tm *tm)
{
	while (*format != NULL && *s != NULL)
	{
		if (*format != '%')
		{
			if (*s != *format)
			{
				return NULL;
			}

			++format;
			++s;
			continue;
		}

		++format;
		int len = 0;
		switch (*format)
		{
			// weekday name.
			case 'a':
			case 'A':
				tm->tm_wday = -1;
				for (int i = 0; i < 7; ++i)
				{
					len = static_cast<int>(strlen(kWeekAbbr[i]));
					if (_strnicmp(kWeekAbbr[i], s, len) == 0)
					{
						tm->tm_wday = i;
						break;
					}

					len = static_cast<int>(strlen(kWeekFull[i]));
					if (_strnicmp(kWeekFull[i], s, len) == 0)
					{
						tm->tm_wday = i;
						break;
					}
				}
				if (tm->tm_wday == -1)
				{
					return NULL;
				}
				s += len;
				break;

			// month name.
			case 'b':
			case 'B':
			case 'h':
				tm->tm_mon = -1;
				for (int i = 0; i < 12; ++i)
				{
					len = static_cast<int>(strlen(kMonthAbbr[i]));
					if (_strnicmp(kMonthAbbr[i], s, len) == 0)
					{
						tm->tm_mon = i;
						break;
					}

					len = static_cast<int>(strlen(kMonthFull[i]));
					if (_strnicmp(kMonthFull[i], s, len) == 0)
					{
						tm->tm_mon = i;
						break;
					}
				}
				if (tm->tm_mon == -1)
				{
					return NULL;
				}
				s += len;
				break;

			  // month [1, 12].
			case 'm':
				s = _parse_num(s, 1, 12, &tm->tm_mon);
				if (s == NULL)
				{
					return NULL;
				}
				--tm->tm_mon;
				break;

			  // day [1, 31].
			case 'd':
			case 'e':
				s = _parse_num(s, 1, 31, &tm->tm_mday);
				if (s == NULL)
				{
					return NULL;
				}
				break;

			  // hour [0, 23].
			case 'H':
				s = _parse_num(s, 0, 23, &tm->tm_hour);
				if (s == NULL)
				{
					return NULL;
				}
				break;

			  // minute [0, 59]
			case 'M':
				s = _parse_num(s, 0, 59, &tm->tm_min);
				if (s == NULL)
				{
					return NULL;
				}
				break;

			  // seconds [0, 60]. 60 is for leap year.
			case 'S':
				s = _parse_num(s, 0, 60, &tm->tm_sec);
				if (s == NULL)
				{
					return NULL;
				}
				break;

			  // year [1900, 9999].
			case 'Y':
				s = _parse_num(s, 1900, 9999, &tm->tm_year);
				if (s == NULL)
				{
					return NULL;
				}
				tm->tm_year -= 1900;
				break;

			  // year [0, 99].
			case 'y':
				s = _parse_num(s, 0, 99, &tm->tm_year);
				if (s == NULL)
				{
					return NULL;
				}

				if (tm->tm_year <= 68)
				{
					tm->tm_year += 100;
				}
				break;

			  // arbitray whitespace.
			case 't':
			case 'n':
				while (isspace(*s))
				{
					++s;
				}
				break;

			  // '%'.
			case '%':
				if (*s != '%')
				{
					return NULL;
				}
				++s;
				break;

			  // All the other format are not supported.
			default:
				return NULL;
		}

		++format;
	}

	if (*format != NULL)
	{
		return NULL;
	}
	else
	{
		return const_cast<char*>(s);
	}
}

char* strptime(const char *buf, const char *fmt, struct tm *tm)
{
	return _strptime(buf, fmt, tm);
}
#endif  // WIN32

////////////////////////////////////////////////////////////////////////////////
//; ParseTime
//
// Description: Parse the time using the format string.
//
// Abstract:
//	Parses the time using the format string.  If the time
//	string does not match the format string time function returns false
//	otherwise it returns true.
//
// Returns:
//	bool - true if successful, false if not successful
//
static bool ParseTime(
	const char *pszTime,
	const char *pFormat,
	tm *pstTime)
{
	char *pszRemainder; // The point where strptime() choked on pszTime
	bool bSuccess = true;

	//
	// Parse the time using the format string.
	//
	pszRemainder = strptime (pszTime, pFormat, pstTime);
	if (pszRemainder == nullptr
	|| pszRemainder[0] != '\0')
	{
		bSuccess = false;
	}

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////
//; TimeConvert
//
// Description: Converts UTC string time into a local time_t value
//
// Abstract:
//	Converts a string time in Core or Sip format into a time_t
//	value. String MUST BE UTC / GMT!
//
// Returns:
//	time_t - the time represented by the string.
//
stiHResult TimeConvert (
	const char * pszTime,
	time_t *pTime)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct tm stTime{};
	memset (&stTime, 0, sizeof (struct tm));
	time_t ttReturn = 0;

	if (ParseTime (pszTime, "%m/%d/%Y %I:%M:%S %p", &stTime)
	|| ParseTime (pszTime, "%m/%d/%Y %H:%M:%S", &stTime)
	|| ParseTime (pszTime,  "%a, %d%n%b%n%Y %H:%M:%S", &stTime))
	{
		const int64_t MonthOffsets[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
		const int64_t nDaysInYear = 365;
		const int64_t nSecondsInDay = 86400;
		const int64_t nSecondsInHour = 3600;
		const int64_t nSecondsInMinute = 60;
		int64_t nLeapYears;

		//
		// Compute the number of leap years between the epoch and "now".
		// Years that are divisible by 100 are not leap years unless they are also divisible by 400.
		//
		nLeapYears = (stTime.tm_year - 68) / 4 - (stTime.tm_year / 100) + (stTime.tm_year + 300) / 400;

		//
		// If the current year is a leap year but we have not passed through february
		// then subtract it from the leap year count.
		//
		if ((((stTime.tm_year % 4) == 0 && (stTime.tm_year % 100) != 0)
		|| (stTime.tm_year % 400) == 0)
		&& stTime.tm_mon < 2)
		{
			--nLeapYears;
		}

		ttReturn = ((stTime.tm_year - 70) * nDaysInYear + stTime.tm_mday - 1
					+ MonthOffsets[stTime.tm_mon] + nLeapYears) * nSecondsInDay
					+ stTime.tm_hour * nSecondsInHour + stTime.tm_min * nSecondsInMinute + stTime.tm_sec;
					
		*pTime = ttReturn;
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
} // end TimeConvert


/*!
* \brief Returns the number of days between two time stamps.
*
*/
int TimeDifferenceInDays (
	time_t tTime1,
	time_t tTime2,
	EstiRounding eRoundMethod)
{
	int nDays;
	int64_t n64TimeDiff;
	
	//
	// Compute time difference between Time1 and Time2
	//
	double dTimeDiff = difftime(tTime1, tTime2);
	n64TimeDiff = (int64_t)dTimeDiff;
	
	//
	// Compute the number of days by dividing by
	// the number of seconds in a day.
	//
	if (estiROUND_UP == eRoundMethod)
	{
		n64TimeDiff += g_unNumberOfSecondsInDay - 1;
	} // end if
	nDays = (int)(n64TimeDiff / g_unNumberOfSecondsInDay);  // divide by number of seconds in a day...
	
	return nDays;
}

/*!
* \brief Returns the number of seconds between two time values.
*
*/
double TimeDifferenceInSeconds (
	const timespec &Time1,
	const timespec &Time2)
{
	timespec timeDifference{};
	timespecSubtract (&timeDifference, Time1, Time2);
	return static_cast<double>((timeDifference.tv_sec * stiNSEC_PER_SEC) + timeDifference.tv_nsec) / static_cast<double>(stiNSEC_PER_SEC);
}

/*!
* \brief Returns a char* to a string respresting dDiffTime.
* \param dDiffTime - The lapsed or difference time be converted to a string.
* \param pszBuff - A buffer to write the string to.
* \param nBuffLen - The length of the buffer passed in.
* \param unUnitsMask - The unit(s) to desired in the output (estiDAYS, estiHOURS, estiMINUTES, estiSECONDS).
* \param eRoundMethod - Flag to cause the smallest requested unit to be rounded up.
*/
char* TimeDiffToString (
	double dDiffTime,
	char* pszBuff,
	int nBuffLen,
	unsigned int unUnitsMask,
	EstiRounding eRoundMethod)
{
	char* pszRtnBuff = nullptr;

	// Validate the passed in values
	if (nullptr != pszBuff && nBuffLen > 0 && unUnitsMask)
	{
		char szDiff[96];
		char szTemp[48];

		EstiBool bShowDays = estiFALSE;
		EstiBool bShowHours = estiFALSE;
		EstiBool bShowMinutes = estiFALSE;
		EstiBool bShowSeconds = estiFALSE;
		EstiTIME_UNITS eSmallestUnit = estiDAYS;
		EstiBool bIsNegative = estiFALSE;
		if (0 > dDiffTime)
		{
			bIsNegative = estiTRUE;

			// Get the absolute value
			dDiffTime*=-1;
		} // end if

		szDiff[0] = '\0';

		// Break the difference into days, hours, minutes and seconds
		auto  nDays = (int)(dDiffTime / g_unNumberOfSecondsInDay);  // divide by number of seconds in a day...
		auto  nRemainder = (int)(dDiffTime - nDays * g_unNumberOfSecondsInDay);
		div_t stDiv = div (nRemainder, g_unNumberOfSecondsInHour);
		int nHours = stDiv.quot;
		stDiv = div (stDiv.rem, g_unNumberOfSecondsInMinute); // Calculate minutes
		int nMinutes = stDiv.quot;
		int nSeconds = stDiv.rem;

		// Was Days requested?
		if (0 != (estiDAYS & unUnitsMask))
		{
			eSmallestUnit = estiDAYS;
		} // end if

		// Was Hours requested?
		if (0 != (estiHOURS & unUnitsMask))
		{
			eSmallestUnit = estiHOURS;
		} // end if

		// Was Minutes requested?
		if (0 != (estiMINUTES & unUnitsMask))
		{
			eSmallestUnit = estiMINUTES;
		} // end if

		// Was Seconds requested?
		if (0 != (estiSECONDS & unUnitsMask))
		{
			eSmallestUnit = estiSECONDS;
		} // end if
		
		if (estiROUND_UP == eRoundMethod)
		{
			// If this is the smallest unit to be printed, round up.
			if (estiMINUTES == eSmallestUnit)
			{
				if (0 < nSeconds)
				{
					nSeconds = 0;
					nMinutes++;
					
					if (nMinutes >= 60)
					{
						nMinutes = 0;
						++nHours;
						
						if (nHours >= 24)
						{
							nHours = 0;
							++nDays;
						}
					}
				} // end if
			} // end if
			
			// If this is the smallest unit to be printed, round up.
			if (estiHOURS == eSmallestUnit)
			{
				if (0 < nMinutes || 0 < nSeconds)
				{
					nSeconds = 0;
					nMinutes = 0;
					nHours++;
					
					if (nHours >= 24)
					{
						nHours = 0;
						++nDays;
					}
				} // end if
			} // end if
			
			// If this is the smallest unit to be printed, round up.
			if (estiDAYS == eSmallestUnit)
			{
				if (0 < nHours || 0 < nMinutes || 0 < nSeconds)
				{
					nHours = 0;
					nMinutes = 0;
					nSeconds = 0;
					nDays++;
				} // end if
			} // end if
		} // end if

		// How many different units have been requested?
		int nUnitsRequested = 0;

		// Was Days requested?
		if (0 != (estiDAYS & unUnitsMask))
		{
			// Did we calculate any days?
			if (0 != nDays)
			{
				bShowDays = estiTRUE;
				nUnitsRequested++;
			} // end if
		} // end if

		// Was Hours requested?
		if (0 != (estiHOURS & unUnitsMask))
		{
			// Did we calculate any hours?
			if (0 != nHours)
			{
				nUnitsRequested++;
				bShowHours = estiTRUE;
			} // end if
		} // end if

		// Was Minutes requested?
		if (0 != (estiMINUTES & unUnitsMask))
		{
			// Did we calculate any minutes?
			if (0 != nMinutes)
			{
				nUnitsRequested++;
				bShowMinutes = estiTRUE;
			} // end if
		} // end if

		// Was Seconds requested?
		if (0 != (estiSECONDS & unUnitsMask))
		{
			// Did we calculate any seconds?
			if (0 != nSeconds)
			{
				nUnitsRequested++;
				bShowSeconds = estiTRUE;
			} // end if
		} // end if

		// Make sure that something will be printed in the event that
		// the value passed in (or the only unit requested) is 0;
		if (0 == dDiffTime || 0 == nUnitsRequested)
		{
			switch (eSmallestUnit)
			{
				case estiDAYS:
					bShowDays = estiTRUE;
					break;

				case estiHOURS:
					bShowHours = estiTRUE;
					break;

				case estiMINUTES:
					bShowMinutes = estiTRUE;
					break;

				case estiSECONDS:
				default:
					bShowSeconds = estiTRUE;
					break;
			} // end switch
		} // end if

		// If the value is negative, place a '-' sign at the beginning of the string
		if (bIsNegative)
		{
			sprintf (szDiff, "-");
		} // end if

		// Do we need to know the number of days?
		if (bShowDays)
		{
			sprintf (szTemp, "%d %s%s", nDays,
				1 == nDays ? "day" : "days",
				--nUnitsRequested > 1 ? ", " :
				nUnitsRequested == 1 ? " and " : "");
			strcat (szDiff, szTemp);
		} // end if
		else
		{
			// Since we aren't concerned with days, convert them to hours
			nHours += nDays * 24;
		} // end else

		// Do we need to know the number of hours?
		if (bShowHours)
		{
			sprintf (szTemp, "%d %s%s", nHours,
				1 == nHours ? "hour" : "hours",
				--nUnitsRequested > 1 ? ", " :
				nUnitsRequested == 1 ? " and " : "");
			strcat (szDiff, szTemp);
		} // end if
		else
		{
			// Since we aren't concerned with hours, convert them to minutes
			nMinutes += nHours * 60;
		} // end else

		// Do we need to know the number of minutes?
		if (bShowMinutes)
		{
			sprintf (szTemp, "%d %s%s", nMinutes,
				1 == nMinutes ? "minute" : "minutes",
				--nUnitsRequested > 1 ? ", " :
				nUnitsRequested == 1 ? " and " : "");
			strcat (szDiff, szTemp);
		} // end if
		else
		{
			// Since we aren't concerned with hours, convert them to minutes
			nSeconds += nMinutes * 60;
		} // end else

		// Do we need to know the number of seconds?
		if (bShowSeconds)
		{
			sprintf (szTemp, "%d %s", nSeconds,
				1 == nSeconds ? "second" : "seconds");
			strcat (szDiff, szTemp);
		} // end if

		// Copy the string to the passed in buffer
		strncpy (pszBuff, szDiff, nBuffLen);
		pszBuff[nBuffLen] = '\0';
		pszRtnBuff = pszBuff;
	} // end if

	return (pszRtnBuff);
}


/*!
* \brief Replaces the string pointed to by the first parameter with the string pointed to by the second parameter.
*
* Precondition: This method assumes that the first string was allocated using the C++ new method.
*
* \param ppszDest - A pointer to the string to replace.
* \param pszSrc - A pointer to the string to replace the first with.
*/
stiHResult ReplaceString(
	char **ppszDest,
	const char *pszSrc)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char *pszNewString = nullptr;

	if (pszSrc)
	{
		pszNewString = new char[strlen(pszSrc) + 1];

		stiTESTCOND(pszNewString != nullptr, stiRESULT_ERROR);

		strcpy(pszNewString, pszSrc);
	}

	if (*ppszDest)
	{
		delete [] *ppszDest;
		*ppszDest = nullptr;
	}

	*ppszDest = pszNewString;
	pszNewString = nullptr;

STI_BAIL:

	if (pszNewString)
	{
		delete [] pszNewString;
		pszNewString = nullptr;
	}

	return hResult;

}

/*!
* \brief
*
* \param const char *pszUrl        Incoming URL to parse
* \param std::string *pProtocol    i.e. ftp, http, https, etc.
* \param std::string *pServer      domain name e.g. www.sorenson.com
* \param std::string *pFile        What comes after the domain Name e.g. /local/xyz.combo
*/
bool ParseUrl (
	const char *pszUrl,
	std::string *pProtocol,
	std::string *pServer,
	std::string *pFile)
{
	bool bResult = false;
	CstiRegEx RegexURL(g_szRegexURL);
	std::string url = pszUrl;
	std::vector<std::string> results;
	if (RegexURL.Match (url, results))
	{
		pProtocol->assign (results[1]);
		pServer->assign (results[2]);
		pFile->assign (results[3]);
		bResult = true;
	}

	return bResult;
}

EstiPictureSize PictureSizeGet (
	unsigned int unCols,
	unsigned int unRows)
{
	EstiPictureSize ePictureSize = estiCUSTOM;

	// Is the requested size SQCIF?
	if (unstiSQCIF_ROWS == unRows
	 && unstiSQCIF_COLS == unCols)
	{
		ePictureSize = estiSQCIF;
	}  // end if

	// Is the requested size QCIF?
	else if (unstiQCIF_ROWS == unRows
	 && unstiQCIF_COLS == unCols)
	{
		ePictureSize = estiQCIF;
	}  // end else if

	// Is the requested size CIF?
	else if (unstiCIF_ROWS == unRows
	 && unstiCIF_COLS == unCols)
	{
		ePictureSize = estiCIF;
	}  // end else if

	// Is the requested size SIF?
	else if (unstiSIF_ROWS == unRows
		&& unstiSIF_COLS == unCols)
	{
		ePictureSize = estiSIF;
	}  // end else if

	return ePictureSize;
}


/*!
 * \brief Computes the difference between two timeval structures.
 * 
 * It is assumed that the timeval inputs are normalized.
 * 
 * \param result The result of the subtraction
 * \param x The first time value.
 * \param y The second time value.
 * 
 */
int timevalSubtract (
	timeval *pDifference,
	const timeval *pTime1,
	const timeval *pTime2)
{
	int nNegative = 0;
	
	if (pTime1->tv_usec < pTime2->tv_usec)
	{
		if (pTime1->tv_sec - 1 >= pTime2->tv_sec)
		{
			//
			// This is the case where T1 is greater than T2 but the usec
			// component of T1 is less than T2's usec component.  Because of this
			// we need to do a borrow.
			//
			pDifference->tv_usec = (pTime1->tv_usec + USEC_PER_SEC) - pTime2->tv_usec;
			pDifference->tv_sec = pTime1->tv_sec - 1 - pTime2->tv_sec;
		}
		else
		{
			//
			// This is the case that T1 is less than T2 so we need to reverse the roles and
			// subtract T1 from T2.  We already know that the T2 usec component is greater
			// than the usec component of T1 because of the first test above.
			//
			pDifference->tv_usec = pTime2->tv_usec - pTime1->tv_usec;
			pDifference->tv_sec = pTime2->tv_sec - pTime1->tv_sec;
		
			nNegative = 1;
		}
	}
	else
	{
		if (pTime1->tv_sec >= pTime2->tv_sec)
		{
			// This is the case where both components of T1 are greater than T2 so
			// we can do simple math without borrowing.
			//
			pDifference->tv_usec = pTime1->tv_usec - pTime2->tv_usec;
			pDifference->tv_sec = pTime1->tv_sec - pTime2->tv_sec;
		}
		else
		{
			//
			// Here we know that T2 is actually greater than T1 so the roles need
			// to be reveresed (subtract T1 from T2).  However, we must check to see
			// if a borrow needs to occur.  We know that the usec component of T2 is
			// less than *or* equal to T1's usec component so we need to check to see
			// if it *only* less than T1's usec component (and thus need to do a borrow).
			//
			if (pTime2->tv_usec < pTime1->tv_usec)
			{
				pDifference->tv_usec = (pTime2->tv_usec + USEC_PER_SEC) - pTime1->tv_usec;
				pDifference->tv_sec = pTime2->tv_sec - 1 - pTime1->tv_sec;
			}
			else
			{
				pDifference->tv_usec = pTime2->tv_usec - pTime1->tv_usec;
				pDifference->tv_sec = pTime2->tv_sec - pTime1->tv_sec;
			}
			
			nNegative = 1;
		}
	}
		
	return nNegative;
}

/*!
 * \brief Computes the sum of two timeval structures.
 * 
 * \param result The result of the addition
 * \param x The first time value.
 * \param y The second time value.
 * 
 */
void timevalAdd (
	timeval *pSum,
	const timeval *pTime1,
	const timeval *pTime2)
{
	pSum->tv_sec = pTime1->tv_sec + pTime2->tv_sec;
	pSum->tv_usec = pTime1->tv_usec + pTime2->tv_usec;
	
	pSum->tv_sec += pSum->tv_usec / USEC_PER_SEC;
	pSum->tv_usec = pSum->tv_usec % USEC_PER_SEC;
}


/*!
 * \brief Computes the difference between two timespec structures.
 * 
 * It is assumed that the timespec inputs are normalized.
 * 
 * \param result The result of the subtraction
 * \param x The first time value.
 * \param y The second time value.
 * 
 */
int timespecSubtract (
	timespec *pDifference,
	const timespec &Time1,
	const timespec &Time2)
{
	int nNegative = 0;
	
	if (Time1.tv_nsec < Time2.tv_nsec)
	{
		if (Time1.tv_sec - 1 >= Time2.tv_sec)
		{
			//
			// This is the case where T1 is greater than T2 but the usec
			// component of T1 is less than T2's usec component.  Because of this
			// we need to do a borrow.
			//
			pDifference->tv_nsec = (Time1.tv_nsec + stiNSEC_PER_SEC) - Time2.tv_nsec;
			pDifference->tv_sec = Time1.tv_sec - 1 - Time2.tv_sec;
		}
		else
		{
			//
			// This is the case that T1 is less than T2 so we need to reverse the roles and
			// subtract T1 from T2.  We already know that the T2 usec component is greater
			// than the usec component of T1 because of the first test above.
			//
			pDifference->tv_nsec = Time2.tv_nsec - Time1.tv_nsec;
			pDifference->tv_sec = Time2.tv_sec - Time1.tv_sec;
		
			nNegative = 1;
		}
	}
	else
	{
		if (Time1.tv_sec >= Time2.tv_sec)
		{
			// This is the case where both components of T1 are greater than T2 so
			// we can do simple math without borrowing.
			//
			pDifference->tv_nsec = Time1.tv_nsec - Time2.tv_nsec;
			pDifference->tv_sec = Time1.tv_sec - Time2.tv_sec;
		}
		else
		{
			//
			// Here we know that T2 is actually greater than T1 so the roles need
			// to be reveresed (subtract T1 from T2).  However, we must check to see
			// if a borrow needs to occur.  We know that the usec component of T2 is
			// less than *or* equal to T1's usec component so we need to check to see
			// if it *only* less than T1's usec component (and thus need to do a borrow).
			//
			if (Time2.tv_nsec < Time1.tv_nsec)
			{
				pDifference->tv_nsec = (Time2.tv_nsec + stiNSEC_PER_SEC) - Time1.tv_nsec;
				pDifference->tv_sec = Time2.tv_sec - 1 - Time1.tv_sec;
			}
			else
			{
				pDifference->tv_nsec = Time2.tv_nsec - Time1.tv_nsec;
				pDifference->tv_sec = Time2.tv_sec - Time1.tv_sec;
			}
			
			nNegative = 1;
		}
	}
		
	return nNegative;
}

/*!
 * \brief Computes the sum of two timespec structures.
 * 
 * \param result The result of the addition
 * \param x The first time value.
 * \param y The second time value.
 * 
 */
void timespecAdd (
	timespec *pSum,
	const timespec &Time1,
	const timespec &Time2)
{
	pSum->tv_sec = Time1.tv_sec + Time2.tv_sec;
	pSum->tv_nsec = Time1.tv_nsec + Time2.tv_nsec;
	
	pSum->tv_sec += pSum->tv_nsec / stiNSEC_PER_SEC;
	pSum->tv_nsec = pSum->tv_nsec % stiNSEC_PER_SEC;
}


/*!
 * \brief Split an ip:port string into two separate strings.
 * 
 * \param pszAddressAndPort the address to parse
 * \param pszAddress fill this string in with the address portion (you must make sure it is large enough)
 * \param pszPort fill this string in with the port portion if it exists (you must make sure it is large enough)
 * 
 * \return stiHResult
 */
stiHResult AddressAndPortParse (
	const char *pszAddressAndPort,
	char *pszAddress,
	char *pszPort)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (pszAddressAndPort)
	{
		const char *pszColon;
		const char *pszCloseBracket;
		const char *pszDot;
		
		pszColon = strrchr (pszAddressAndPort, ':');
		pszCloseBracket = strrchr (pszAddressAndPort, ']');
		pszDot = strrchr (pszAddressAndPort, '.');
		
		if (pszCloseBracket)
		{
			if (pszAddressAndPort[1] != '[')
			{
				stiTHROW (stiRESULT_ERROR);
			}
			
			strncpy (pszAddress, &pszAddressAndPort[1], pszCloseBracket - (pszAddressAndPort + 1));
			if (pszColon && pszColon > pszCloseBracket)
			{
				strcpy (pszPort, pszColon + 1);
			}
			else
			{
				pszPort[0] = '\0';
			}
		}
		else
		{
			if (pszColon && pszDot)
			{
				strncpy (pszAddress, pszAddressAndPort, pszColon - pszAddressAndPort);
				pszAddress[pszColon - pszAddressAndPort] = '\0';
				
				strcpy (pszPort, pszColon + 1);
			}
			else
			{
				strcpy (pszAddress, pszAddressAndPort);
				pszPort[0] = '\0';
			}
		}
	}
	else
	{
		pszAddress[0] = '\0';
		pszPort[0] = '\0';
		stiTHROW (stiRESULT_ERROR);
	}
	
STI_BAIL:
	
	return hResult;
}


/*!
 * \brief Split an ip:port string into two separate strings.
 * 
 * \param pszAddressAndPort the address to parse
 * \param pAddress Fills this string in with the address portion
 * \param pun16Port Contains the port portion if it exists.  Otherwise, it is set to zero.
 * 
 * \return stiHResult
 */
stiHResult AddressAndPortParse (
	const char *pszAddressAndPort,
	std::string *pAddress,
	uint16_t *pun16Port)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (pszAddressAndPort)
	{
		const char *pszColon;

		pszColon = strchr (pszAddressAndPort, ':');

		if (pszColon)
		{
			pAddress->assign (pszAddressAndPort, 0, pszColon - pszAddressAndPort);

			*pun16Port = atoi (pszColon + 1);
		}
		else
		{
			*pAddress = pszAddressAndPort;
			*pun16Port = 0;
		}
	}
	else
	{
		*pAddress = "";
		*pun16Port = 0;
	}

	return hResult;
}


stiHResult NameSplit (
	const char *pszFullName,
	char **pszFirstName,
	char **pszLastName)
{
	// Find the end of the string and look for the first space.
	// The word to the right of the space becomes the last name
	// and the word to the left of the space becomes the first name.
	// This allows for names of the form "John and Jane Doe" to become
	// "John and Jane" for the first name and "Doe" for the last name.
	//
	int nLength = strlen (pszFullName);

	if (nLength > 0)
	{
		int i = nLength - 1;

		//
		// Skip past any trailing white space
		//
		for (; i >= 0; i--)
		{
			if (!isspace (pszFullName[i]))
			{
				break;
			}
		}

		//
		// Find right most space
		//
		for (; i >= 0; i--)
		{
			if (isspace (pszFullName[i]))
			{
				break;
			}
		}

		if (i <= 0)
		{
			//
			// There was no space separating a potential first and last name
			// Just store the whole name into the first name field.
			//
			if (pszFirstName)
			{
				*pszFirstName = new char[nLength + 1];
				strcpy(*pszFirstName, pszFullName);
			}
		}
		else
		{
			//
			// We want to throw away the space between the first and last name.  When reassembling a first
			// and last name we will add the space back in.
			//
			if (pszFirstName)
			{
				*pszFirstName = new char[i + 1];
				strncpy(*pszFirstName, pszFullName, i);
				(*pszFirstName)[i] = 0;
			}

			if (pszLastName)
			{
				*pszLastName = new char[nLength - i + 1];
				strcpy(*pszLastName, &pszFullName[i + 1]);
			}
		}
	}

	return stiRESULT_SUCCESS;
}


/*!\brief Get a time value for use in duration calculations
 *
 * Note: This is not the time from the wall clock
 * and should not be used for call placement time within
 * a call history list
 *
 * \param pTime On return, contains a representation of the time
 */
void TimeGet (
	time_t *pTime)
{
#ifdef stiUSE_MONOTONIC_CLOCK
	//
	// Get the time from the monotonic clock
	// since it isn't affected by the setting of the
	// wall clock (i.e. stime)
	//
	timespec Time{};

	clock_gettime(CLOCK_MONOTONIC, &Time);

	*pTime = Time.tv_sec;
#else
	time (pTime);
#endif
}


/*!\brief Get a time value for use in duration calculations
 *
 * Note: This is not the time from the wall clock
 * and should not be used for call placement time within
 * a call history list
 *
 * \param pTimeSpec On return, contains a representation of the time
 */
void TimeGet (
	timespec *pTimeSpec)
{
#ifdef stiUSE_MONOTONIC_CLOCK
	//
	// Get the time from the monotonic clock
	// since it isn't affected by the setting of the
	// wall clock (i.e. stime)
	//
	clock_gettime(CLOCK_MONOTONIC, pTimeSpec);
#else
	timeval Time;
	gettimeofday (&Time, NULL);
	pTimeSpec->tv_sec = Time.tv_sec;
	pTimeSpec->tv_nsec = Time.tv_usec * NSEC_PER_USEC;
#endif
}

#if (SUBDEVICE != SUBDEV_RCU)
/*!\brief Decrypts a string buffer
 *
 * \param pszEncryptedString The encrypted string buffer
 * \param pun8Key The encryption key
 * \param pun8InitVector The initialization vector
 * \param pDecryptedString The decrypted string is stored in this buffer
 *
 */
stiHResult DecryptString (
	const char *pszEncryptedString,
	const unsigned char *pun8Key,
	const unsigned char *pun8InitVector,
	std::string *pDecryptedString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	unsigned char *pOutputBuffer = nullptr;
	int nOutputLength = 0;
	int nTmpLength = 0;
	char *pBase64Decoded = nullptr;
	int nBase64DecodedLength = 0;
	const int nEncryptedLength = strlen(pszEncryptedString);

	auto ctx = EVP_CIPHER_CTX_new();

	nResult = EVP_CipherInit(ctx, EVP_bf_cbc(), pun8Key, pun8InitVector, 0);

	//
	// Decode the Base64 string
	// The decoded string will always be shorter than the encoded string so allocate
	// a buffer that is equal in length to the encoded string.
	//
	stiTESTCOND(nEncryptedLength > 0, stiRESULT_ERROR);
	pBase64Decoded = new char [nEncryptedLength];

	stiBase64Decode (pBase64Decoded, &nBase64DecodedLength, (const char *)pszEncryptedString);

	//
	// Decrypt the encrypted string
	//
	pOutputBuffer = new unsigned char [nBase64DecodedLength + EVP_MAX_BLOCK_LENGTH];

	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nResult = EVP_CipherUpdate(ctx, pOutputBuffer, &nOutputLength, (unsigned char *)pBase64Decoded, nBase64DecodedLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nResult = EVP_CipherFinal_ex(ctx, &pOutputBuffer[nOutputLength], &nTmpLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nOutputLength += nTmpLength;

	pOutputBuffer[nOutputLength] = '\0';

	//
	// Copy the decrypted buffer to the output parameter.
	//
	*pDecryptedString += (char *)pOutputBuffer;

STI_BAIL:

	EVP_CIPHER_CTX_free(ctx);

	if (pOutputBuffer)
	{
		delete [] pOutputBuffer;
		pOutputBuffer = nullptr;
	}

	if (pBase64Decoded)
	{
		delete [] pBase64Decoded;
		pBase64Decoded = nullptr;
	}

	return hResult;
}


/*!\brief Encrypts a string buffer
 *
 * \param pszString The string buffer to be encrypted
 * \param pun8Key The encryption key
 * \param pun8InitVector The initialization vector
 * \param pEncryptedString The encrypted string is stored in this buffer
 */
stiHResult EncryptString (
	const char *pszString,
	const unsigned char *pun8Key,
	const unsigned char *pun8InitVector,
	std::string *pEncryptedString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	unsigned char *pOutputBuffer = nullptr;
	int nOutputLength = 0;
	int nTmpLength = 0;
	char *pBase64Buffer = nullptr;

	auto ctx = EVP_CIPHER_CTX_new();

	EVP_CipherInit(ctx, EVP_bf_cbc(), pun8Key, pun8InitVector, 1);

	pOutputBuffer = new unsigned char [strlen (pszString) + EVP_MAX_BLOCK_LENGTH];

	nResult = EVP_CipherUpdate(ctx, pOutputBuffer, &nOutputLength, (const unsigned char *)pszString, strlen (pszString));
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	pOutputBuffer[nOutputLength] = '\0';

	nResult = EVP_CipherFinal_ex(ctx, &pOutputBuffer[nOutputLength], &nTmpLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nOutputLength += nTmpLength;

	//
	// Base64 encode
	//
	pBase64Buffer = new char [nOutputLength * 2];

	stiBase64Encode (pBase64Buffer, (char *)pOutputBuffer, nOutputLength);

	//
	// Copy the encoded buffer to the output parameter.
	//
	*pEncryptedString = pBase64Buffer;

STI_BAIL:

	EVP_CIPHER_CTX_free(ctx);

	if (pOutputBuffer)
	{
		delete [] pOutputBuffer;
		pOutputBuffer = nullptr;
	}

	if (pBase64Buffer)
	{
		delete [] pBase64Buffer;
		pBase64Buffer = nullptr;
	}

	return hResult;
}
#endif


/*!\brief Trims whitespace characters from the left side of a string
 *
 * \param pString Pointer to string to be trimmed
 */
std::string *LeftTrim(
	std::string *pString)
{
	pString->erase(pString->begin(), std::find_if(pString->begin(), pString->end(), [](int c) { return !std::isspace(c); }));

	return pString;
}

/*!\brief Trims whitespace characters from the right side of a string
 *
 * \param pString Pointer to string to be trimmed
 */
std::string *RightTrim(
	std::string *pString)
{
	pString->erase(std::find_if(pString->rbegin(), pString->rend(), [](int c) { return !std::isspace(c); }).base(), pString->end());

	return pString;
}

/*!\brief Trims whitespace characters from both sides of a string
 *
 * \param pString Pointer to string to be trimmed
 */
std::string *Trim(
	std::string *pString)
{
	return LeftTrim(RightTrim(pString));
}


/*!\brief Copies memory from source to destination while avoiding buffer overruns and overlapped memory.
 *
 * Note that this function does not error if the destination buffer is smaller than the requested
 * number of bytes to copy but it does return the actual number of bytes that were copied. Callers may
 * check actual against expected number of bytes to copy and throw an error if they are different.
 *
 * \param pDstBuffer A pointer to the destination buffer
 * \param DstLength The length of the destination buffer
 * \param pSrcBuffer A pointer to the source buffer
 * \param SrcLength The number of bytes to copy
 */
size_t stiSafeMemcpy (
	void *pDstBuffer,
	size_t DstLength,
	const void *pSrcBuffer,
	size_t SrcLength)
{
	size_t CopyLength = 0;

	if (SrcLength <= DstLength)
	{
		//
		// Checks two cases for memory overlap:
		//
		// If the source buffer is higher in memory but the number of bytes to copy to the destination
		// buffer (SrcLength) causes the last destination memory location to be higher than the beginning
		// of the source buffer then there is memory overlap.
		//
		// If the destination buffer is higher in memory but the number of bytes to copy to the destination
		// buffer (SrcLength) causes the last source memory location to be higher than the beginning of the
		// destination buffer then there is memory overlap.
		//
		if (((unsigned char *)pSrcBuffer >= (unsigned char *)pDstBuffer && (unsigned char *)pSrcBuffer < ((unsigned char *)pDstBuffer + SrcLength))
		 || ((unsigned char *)pDstBuffer >= (unsigned char *)pSrcBuffer && pDstBuffer < ((unsigned char *)pSrcBuffer + SrcLength)))
		{
			//
			// Memory overlaps... using memmove
			//
			//stiTrace ("SafeMemcpy: overlapping memory detected.\n");
			memmove (pDstBuffer, pSrcBuffer, SrcLength);
		}
		else
		{
			memcpy (pDstBuffer, pSrcBuffer, SrcLength);
		}

		CopyLength = SrcLength;
	}
	else
	{
		stiASSERTMSG (false, "SourceLength = %d DestinationLength = %d\n", SrcLength, DstLength);

		memcpy (pDstBuffer, pSrcBuffer, DstLength);

		CopyLength = DstLength;
	}

	return CopyLength;
}


/*!
 * \brief splits a string into a vector of strings based on a delimiter
 */
std::vector<std::string> splitString (const std::string &str, char delimiter)
{
	std::stringstream ss(str);
	std::string token;
	std::vector<std::string> tokens;
	while (std::getline(ss, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

std::string productNameConvert (ProductType productType)
{
	static const std::string UNKNOWN_PRODUCT_NAME = "Sorenson Unknown";
	static const std::string IOS_PRODUCT_NAME = "Sorenson Videophone ntouch iOS";
	static const std::string IOS_DHV_PRODUCT_NAME = "Sorenson Videophone dhv iOS";
	static const std::string IOS_FBC_PRODUCT_NAME = "Sorenson Videophone FBC iOS";
	static const std::string IOS_VRI_PRODUCT_NAME = "Sorenson Videophone VRI iOS";
	static const std::string ANDROID_PRODUCT_NAME = "Sorenson Videophone AndroidMVRS";
	static const std::string ANDROID_DHV_PRODUCT_NAME = "Sorenson Videophone dhv Android";
	static const std::string PC_PRODUCT_NAME = "Sorenson Videophone ntouch PC";
	static const std::string MAC_PRODUCT_NAME = "Sorenson Videophone ntouch Mac";
	static const std::string MEDIA_SERVER_PRODUCT_NAME = "Sorenson Media Server";
	static const std::string MEDIA_BRIDGE_PRODUCT_NAME = "Sorenson Media Bridge";
	static const std::string HOLD_SERVER_PRODUCT_NAME = "Sorenson VRS Hold Server";
	static const std::string LOAD_TEST_PRODUCT_NAME = "Sorenson Registrar Load Test";
	static const std::string VP2_PRODUCT_NAME = "Sorenson Videophone VP-400";
	static const std::string VP3_PRODUCT_NAME = "Sorenson Videophone VP-500";
	static const std::string VP4_PRODUCT_NAME = "Sorenson Videophone VP-600";
	

	switch (productType)
	{
	case ProductType::Unknown:
		stiASSERTMSG (false, "Unknown Product Name");
		return UNKNOWN_PRODUCT_NAME;
	case ProductType::iOS:
		return IOS_PRODUCT_NAME;
	case ProductType::iOSDHV:
		return IOS_DHV_PRODUCT_NAME;
	case ProductType::iOSFBC:
		return IOS_FBC_PRODUCT_NAME;
	case ProductType::iOSVRI:
		return IOS_VRI_PRODUCT_NAME;
	case ProductType::Android:
		return ANDROID_PRODUCT_NAME;
	case ProductType::AndroidDHV:
		return ANDROID_DHV_PRODUCT_NAME;
	case ProductType::PC:
		return PC_PRODUCT_NAME;
	case ProductType::Mac:
		return MAC_PRODUCT_NAME;
	case ProductType::MediaServer:
		return MEDIA_SERVER_PRODUCT_NAME;
	case ProductType::MediaBridge:
		return MEDIA_BRIDGE_PRODUCT_NAME;
	case ProductType::HoldServer:
		return HOLD_SERVER_PRODUCT_NAME;
	case ProductType::LoadTestTool:
		return LOAD_TEST_PRODUCT_NAME;
	case ProductType::VP2:
		return VP2_PRODUCT_NAME;
	case ProductType::VP3:
		return VP3_PRODUCT_NAME;
	case ProductType::VP4:
		return VP4_PRODUCT_NAME;
	}
	stiASSERTMSG (false, "Unknown Product Name");
	return UNKNOWN_PRODUCT_NAME;
}

std::string phoneTypeConvert (ProductType productType)
{
	static const std::string UNKNOWN_PRODUCT_NAME = "NTOUCH-UNKNOWN";
#if APPLICATION == APP_NTOUCH_IOS
	static const std::string IOS_PRODUCT_NAME = "NTOUCH-IOS";
#else
	static const std::string IOS_PRODUCT_NAME = "DHV-IOS";
#endif
#if APPLICATION == APP_NTOUCH_ANDROID
	static const std::string ANDROID_PRODUCT_NAME = "NTOUCH-ANDROID";
#else
	static const std::string ANDROID_PRODUCT_NAME = "DHV-ANDROID";
#endif
	static const std::string IOS_FBC_PRODUCT_NAME = "FBC-IOS"; //Should never be sent to core.
	static const std::string IOS_VRI_PRODUCT_NAME = "VRI-IOS"; //Should never be sent to core.
	static const std::string PC_PRODUCT_NAME = "NTOUCH-PC";
	static const std::string MAC_PRODUCT_NAME = "NTOUCH-MAC";
	static const std::string MEDIA_SERVER_PRODUCT_NAME = "NTOUCH-MEDIASERVER"; //Should never be sent to core.
	static const std::string MEDIA_BRIDGE_PRODUCT_NAME = "NTOUCH-MEDIABRIDGE"; //Should never be sent to core.
	static const std::string HOLD_SERVER_PRODUCT_NAME = "NTOUCH-HOLDSERVER"; //Should never be sent to core.
	static const std::string LOAD_TEST_PRODUCT_NAME = "NTOUCH-LOADTEST"; //Should never be sent to core.
	static const std::string VP2_PRODUCT_NAME = "VP-400";
	static const std::string VP3_PRODUCT_NAME = "VP-500";
	static const std::string VP4_PRODUCT_NAME = "VP-600";

	switch (productType)
	{
	case ProductType::Unknown:
		stiASSERTMSG (false, "Unknown Product Type");
		return UNKNOWN_PRODUCT_NAME;
	case ProductType::iOS:
	case ProductType::iOSDHV:
		return IOS_PRODUCT_NAME;
	case ProductType::iOSFBC:
		return IOS_FBC_PRODUCT_NAME;
	case ProductType::iOSVRI:
		return IOS_VRI_PRODUCT_NAME;
	case ProductType::Android:
	case ProductType::AndroidDHV:
		return ANDROID_PRODUCT_NAME;
	case ProductType::PC:
		return PC_PRODUCT_NAME;
	case ProductType::Mac:
		return MAC_PRODUCT_NAME;
	case ProductType::MediaServer: //Should never be sent to core.
		stiASSERTMSG (false, "Media Server Product should not be sent to core");
		return MEDIA_SERVER_PRODUCT_NAME;
	case ProductType::MediaBridge: //Should never be sent to core.
		stiASSERTMSG (false, "Media Server Bridge should not be sent to core");
		return MEDIA_BRIDGE_PRODUCT_NAME;
	case ProductType::HoldServer: //Should never be sent to core.
		stiASSERTMSG (false, "Hold Server Product should not be sent to core");
		return HOLD_SERVER_PRODUCT_NAME;
	case ProductType::LoadTestTool: //Should never be sent to core.
		stiASSERTMSG (false, "Load Test Product should not be sent to core");
		return LOAD_TEST_PRODUCT_NAME;
	case ProductType::VP2:
		return VP2_PRODUCT_NAME;
	case ProductType::VP3:
		return VP3_PRODUCT_NAME;
	case ProductType::VP4:
		return VP4_PRODUCT_NAME;
	}
	stiASSERTMSG (false, "Unknown Product Type");
	return UNKNOWN_PRODUCT_NAME;
}


/*!
 * \brief Returns a string containing the name of the passed in video codec.
 */
std::string VideoCodecToString (EstiVideoCodec codec)
{
	static const std::string VIDEO_NONE = "None";
	static const std::string VIDEO_H263 = "H263";
	static const std::string VIDEO_H264 = "H264";
	static const std::string VIDEO_H265 = "H265";
	static const std::string VIDEO_RTX = "RTX";
	static const std::string VIDEO_UNKNOWN = "Unknown";
	
	switch (codec)
	{
		case estiVIDEO_NONE:
		{
			return VIDEO_NONE;
		}
		case estiVIDEO_H263:
		{
			return VIDEO_H263;
		}
		case estiVIDEO_H264:
		{
			return VIDEO_H264;
		}
		case estiVIDEO_H265:
		{
			return VIDEO_H265;
		}
		case estiVIDEO_RTX:
		{
			return VIDEO_RTX;
		}
	}
	stiASSERTMSG (false, "Unknown video codec (%d) passed in", codec);
	return VIDEO_UNKNOWN;
}


/*!
 * \brief Returns a string containing the name of the passed in audio codec.
 */
std::string AudioCodecToString (EstiAudioCodec codec)
{
	static const std::string AUDIO_NONE = "None";
	static const std::string AUDIO_G711_ALAW = "G711 ALAW";
	static const std::string AUDIO_G711_MULAW = "G711 MULAW";
	static const std::string AUDIO_G711_ALAW_R = "G711 ALAW Robust";
	static const std::string AUDIO_G711_MULAW_R = "G711 MULAW Robust";
	static const std::string AUDIO_G722 = "G722";
	static const std::string AUDIO_DTMF = "DTMF";
	static const std::string AUDIO_RAW = "AUDIO RAW";
	static const std::string AUDIO_AAC = "AUDIO AAC";
	static const std::string AUDIO_UNKNOWN = "Unknown";
	
	switch (codec)
	{
		case estiAUDIO_NONE:
		{
			return AUDIO_NONE;
		}
		case estiAUDIO_G711_ALAW:
		{
			return AUDIO_G711_ALAW;
		}
		case estiAUDIO_G711_MULAW:
		{
			return AUDIO_G711_MULAW;
		}
		case estiAUDIO_G711_ALAW_R:
		{
			return AUDIO_G711_ALAW_R;
		}
		case estiAUDIO_G711_MULAW_R:
		{
			return AUDIO_G711_MULAW_R;
		}
		case estiAUDIO_G722:
		{
			return AUDIO_G722;
		}
		case estiAUDIO_DTMF:
		{
			return AUDIO_DTMF;
		}
		case estiAUDIO_RAW:
		{
			return AUDIO_RAW;
		}
		case estiAUDIO_AAC:
		{
			return AUDIO_AAC;
		}
	}
	stiASSERTMSG (false, "Unknown audio codec (%d) passed in", codec);
	return AUDIO_UNKNOWN;
}


std::string InterfaceModeToString (EstiInterfaceMode mode)
{
	static const std::string STANDARD_MODE = "Standard";
	static const std::string PUBLIC_MODE = "Public";
	static const std::string KIOSK_MODE = "Kiosk";
	static const std::string INTERPRETER_MODE = "Interpreter";
	static const std::string TECH_SUPPORT_MODE = "TechSupport";
	static const std::string VRI_MODE = "VRI";
	static const std::string ABUSIVE_CALLER_MODE = "AbusiveCaller";
	static const std::string PORTED_MODE = "Ported";
	static const std::string HEARING_MODE = "Hearing";
	static const std::string UNKNOWN_MODE = "Unknown";
	
	switch (mode)
	{
		case estiSTANDARD_MODE:
		{
			return STANDARD_MODE;
		}
		case estiPUBLIC_MODE:
		{
			return PUBLIC_MODE;
		}
		case estiKIOSK_MODE:
		{
			return KIOSK_MODE;
		}
		case estiINTERPRETER_MODE:
		{
			return INTERPRETER_MODE;
		}
		case estiTECH_SUPPORT_MODE:
		{
			return TECH_SUPPORT_MODE;
		}
		case estiVRI_MODE:
		{
			return VRI_MODE;
		}
		case estiABUSIVE_CALLER_MODE:
		{
			return ABUSIVE_CALLER_MODE;
		}
		case estiPORTED_MODE:
		{
			return PORTED_MODE;
		}
		case estiHEARING_MODE:
		{
			return HEARING_MODE;
		}
		case estiINTERFACE_MODE_ZZZZ_NO_MORE:
		{
			break;
		}
	}
	
	stiASSERTMSG(false, "Unsupported interface mode passed in %d", mode);
	return UNKNOWN_MODE;
}


std::string NetworkTypeToString (NetworkType type)
{
	static const std::string NETWORK_NONE = "None";
	static const std::string NETWORK_WIRED = "Wired";
	static const std::string NETWORK_WIFI = "WiFi";
	static const std::string NETWORK_CELLULAR = "Cellular";
	static const std::string NETWORK_UNKNOWN = "Unknown";
	
	switch (type)
	{
		case NetworkType::None:
		{
			return NETWORK_NONE;
		}
		case NetworkType::Wired:
		{
			return NETWORK_WIRED;
		}
		case NetworkType::WiFi:
		{
			return NETWORK_WIFI;
		}
		case NetworkType::Cellular:
		{
			return NETWORK_CELLULAR;
		}
	}
	
	stiASSERTMSG(false, "Unsupported network type passed in %d", type);
	return NETWORK_UNKNOWN;
}

const char *torf(bool value)
{
	if (value)
	{
		return "true";
	}
	return "false";
}

void protocolSchemeChange (std::string &uri, const std::string &scheme)
{
	auto protocolStart = uri.find_first_of (':', 0);

	if (protocolStart > 0 && protocolStart != std::string::npos)
	{
		uri.replace (0, protocolStart, scheme);
	}
}

std::string formattedString (const char* format, ...)
{
	std::string formatted{};
	const int MAX_BUF_LEN = 1024;
	char buffer[MAX_BUF_LEN];

	va_list args, argsCopy;
	va_start (args, format);
	va_copy (argsCopy, args);

	int len = vsnprintf (buffer, sizeof (buffer), format, args);

	if (len > (signed)sizeof (buffer))
	{
		auto exactBuffer = new char[len + 1];
		(void)vsnprintf(exactBuffer, len + 1, format, argsCopy);
		formatted = exactBuffer;
		delete [] exactBuffer;
	}
	else
	{
		formatted = buffer;
	}

	va_end (argsCopy);
	va_end (args);
	return formatted;
}

std::string timepointToTimestamp (std::chrono::system_clock::time_point timepoint)
{
	auto coarse = std::chrono::system_clock::to_time_t (timepoint);
	auto fine = std::chrono::time_point_cast<std::chrono::milliseconds> (timepoint);
	auto ms = fine.time_since_epoch ().count () % 1000;

	char coarseTime[sizeof "00:00:00"];
	std::strftime (coarseTime, sizeof coarseTime, "%T", std::localtime (&coarse));

	std::stringstream buildTimestamp {};
	buildTimestamp << coarseTime << "." << ms;

	return buildTimestamp.str ();
}

// end file stiTools.cpp
