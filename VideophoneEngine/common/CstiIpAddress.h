/*!
* \file CstiIpAddress.h
* \brief See CstiIpAddress.cpp.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef STIPADDRESSPARSER_H
#define STIPADDRESSPARSER_H

//
// Includes
//
#include "stiError.h"
#include <string>

//
// Constants
//

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
// Function Declarations
//


class CstiIpAddress
{
public:
	enum EstiFormatting
	{
		eFORMAT_IP = 0x01, ///! Include the ip address itself.  (This is implicit for now, but should be used.)
		eFORMAT_ALWAYS_USE_BRACKETS = 0x02, ///! Will always put ipv6 address inside [] brackets.  Otherwise it will only use brackets if there is a port.
		eFORMAT_NO_CONDENSE = 0x04, ///! Return a full ipv6 address with all hex digits present as opposed to :: shortening and dropping 0 prefixes
		eFORMAT_ALLOW_ZONE = 0x08, ///! Whether to include an ipv6 zone if defined
		eFORMAT_ALLOW_PORT = 0x10 ///! Whether to include a port number if defined
	};

	CstiIpAddress () = default;
	~CstiIpAddress () = default;

	CstiIpAddress (const CstiIpAddress &other) = default;
	CstiIpAddress (CstiIpAddress &&other) = default;
	CstiIpAddress &operator=(const CstiIpAddress &other) = default;
	CstiIpAddress &operator=(CstiIpAddress &&other) = default;
	CstiIpAddress &operator=(const std::string &addressString);

	CstiIpAddress (const std::string &addressString);

	stiHResult assign (const std::string &addressString);
	stiHResult assign (const CstiIpAddress &Other);
	EstiIpAddressType IpAddressTypeGet () const { return m_eType; }
	bool ValidGet() const { return m_bValid; }
	unsigned int PortGet () { return m_nPort; }
	unsigned int ZoneGet () { return m_nZone; }
	std::string AddressGet (unsigned int formatting) const;
	std::string OriginalAddressGet () { return m_OriginalAddress; }
	void clear ();
	bool operator==(const CstiIpAddress &other);
	bool operator==(const std::string &other);
	bool IPv4AddressIsPrivate ();

private:
	EstiIpAddressType m_eType {estiTYPE_IPV4};
	unsigned short m_anValues[8] = {};
	unsigned int m_nPort {0};
	unsigned int m_nZone {0};
	std::string m_OriginalAddress;
	bool m_bValid {false};

	stiHResult assignIPv4 (const char *pszAddressString);
	std::string AddressGetIPv4(unsigned int mFormatting) const;
	stiHResult assignIPv6 (const char *pszAddressString);
	std::string AddressGetIPv6 (unsigned int mFormatting) const;

	int NumValuesGet() const;
};


#endif // STIPADDRESSPARSER_H
