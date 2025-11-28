////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiRoutingAddress
//
//  File Name:  CstiRoutingAddress.cpp
//
//  Abstract:
//  A class to represent and perform operations on a connectable routing address.
//
////////////////////////////////////////////////////////////////////////////////
#include "CstiRoutingAddress.h"
#include "stiTools.h"
#include <cctype>
#include <cstdio>
#include "CstiRegEx.h"

#define PROTOCOL_SIP "sip"
#define PROTOCOL_SIPS "sips"
#define PROTOCOL_MAX_LEN 4


/// \brief The constructor for the dial string object
///
CstiRoutingAddress::CstiRoutingAddress (
	const std::string &dialString) ///< The dial string to parse and verify
{
	DialStringSet (dialString);
}


EstiTransport CstiRoutingAddress::transporParamGet () const
{
	EstiTransport transport {estiTransportUnknown};

	auto iter = m_parameters.find("transport");

	if (iter != m_parameters.end ())
	{
		if (0 == stiStrICmp ((*iter).second.c_str (), "udp"))
		{
			transport = estiTransportUDP;
		}
		else if (0 == stiStrICmp ((*iter).second.c_str (), "tcp"))
		{
			transport = estiTransportTCP;
		}
		else if (0 == stiStrICmp ((*iter).second.c_str (), "tls"))
		{
			transport = estiTransportTLS;
		}
	}

	return transport;
}


void CstiRoutingAddress::parseParameters (
	const std::string &parametersString)
{
	auto parameters = splitString (parametersString, ';');

	m_parameters.clear ();

	for (const auto &parameter: parameters)
	{
		if (!parameter.empty ())
		{
			auto nameValuePair = splitString (parameter, '=');

			switch (nameValuePair.size ())
			{
				case 1:

					m_parameters.insert({nameValuePair[0], {}});

					break;

				case 2:

					m_parameters.insert({nameValuePair[0], nameValuePair[1]});

					break;

				default:

					stiASSERTMSG(false, "Parsing of URI parameter failed: %s", parameter.c_str ());
			}
		}
	}
}


/// \brief Set the contents of this dial string.
///
void CstiRoutingAddress::DialStringSet (
	const std::string &dialString) ///< The dial string to parse and verify
{
	m_eTransport = estiTransportUnknown;
	m_isValid = true;
	m_eProtocol = estiPROT_UNKNOWN;
	
	// Need to re-resolve ip addresses
	m_IpAddress.clear ();

	m_originalDialString = dialString;
	Trim(&m_originalDialString);

	const std::string protocolRegEx{"(([Ss][Ii][Pp][Ss]?):)?"};
	const std::string userRegEx{"(([^ \t\n@]+)@)?"};
	const std::string hostRegEx{"([^@; \t\n:]*)(:([0-9]{1,5}))?"};
	const std::string parametersRegEx{"((;[^; \t\n]+)*)"};

	CstiRegEx uriRegEx(protocolRegEx + userRegEx + hostRegEx + parametersRegEx);

	std::vector<std::string> captures;

	m_isValid = uriRegEx.Match(m_originalDialString, captures);
	
	if (m_isValid)
	{
		// Position of the regex captures within the capture vector
		const int protocolPosition {2};
		const int userPosition {4};
		const int serverPosition {5};
		const int portPosition {7};
		const int parameterPosition {8};

		parseParameters(captures[parameterPosition]);

		auto &protocol = captures[protocolPosition];

		if (stiStrICmp(protocol.c_str (), PROTOCOL_SIPS) == 0)
		{
			m_eProtocol = estiSIPS;
			m_eTransport = estiTransportTLS;
		}
		else if (protocol.empty () || stiStrICmp(protocol.c_str (), PROTOCOL_SIP) == 0)
		{
			m_eProtocol = estiSIP;
			m_eTransport = transporParamGet ();
		}

		m_user = captures[userPosition];
		m_server = captures[serverPosition];

		// Since domain names are case-insensitive, lowercase it for consistancy
		std::transform(m_server.begin(), m_server.end(), m_server.begin(), ::tolower);

		m_port = captures[portPosition];

		m_IpAddress = m_server;
		if (m_IpAddress.ValidGet ())
		{
			// Since only sip can do ipv6...
			if (m_eProtocol == estiPROT_UNKNOWN && m_IpAddress.IpAddressTypeGet () == estiTYPE_IPV6)
			{
				m_eProtocol = estiSIP;
				m_eTransport = transporParamGet ();
			}
		}
	}
}


/// \brief Sets the ip address associated with this dial string
///
void CstiRoutingAddress::ipAddressSet(const std::string &ipAddress)
{
	m_IpAddress.assign(ipAddress);
}


/// \brief Gets the resolved location of the dial string (ip:port)
///
/// This function will either return the ip address given, or the one
/// looked up via DNS.
///
std::string CstiRoutingAddress::ResolvedLocationGet () const
{
	auto ipAddress = ipAddressStringGet ();

	if (!m_port.empty ())
	{
		ipAddress += ":" + m_port;
	}

	return ipAddress;
}


/// \brief Gets the original uri only with the ip address resolved
///
/// This function will either return the ip address given, or the one
/// looked up via DNS.
///
std::string CstiRoutingAddress::ResolvedURIGet () const
{
	std::string uri;


	switch (m_eProtocol)
	{
		case estiSIP:
			uri = PROTOCOL_SIP;
			break;
		case estiSIPS:
			uri = PROTOCOL_SIPS;
			break;
		case estiPROT_UNKNOWN:
		//default:
			break;
	}

	uri += ":";

	if (!m_user.empty ())
	{
		uri += m_user + "@";
	}

	uri += ipAddressStringGet ();

	if (!m_port.empty ())
	{
		uri += ":" + m_port;
	}

	if (estiTransportTCP == m_eTransport)
	{
		uri += ";transport=tcp";
	}

	return uri;
}


/// \brief Returns the ip address, whether resolved or supplied in the address
///
/// \returns nothing
///
std::string CstiRoutingAddress::ipAddressStringGet () const
{
	std::string ipAddress;

	if (m_IpAddress.ValidGet ())
	{
		ipAddress = m_IpAddress.AddressGet (CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS);
	}
	else if (!m_server.empty ())
	{
		ipAddress = m_server;
	}

	return ipAddress;
}

CstiIpAddress CstiRoutingAddress::ipAddressGet() const
{
	return m_IpAddress;
}


/// \brief Returns this dial string as a sip uri
///
/// \returns nothing
///
std::string CstiRoutingAddress::UriGet () const
{
	std::string uri;

	if (m_eProtocol == estiSIPS)
	{
		uri = "sips:";
	}
	else
	{
		uri = "sip:";
	}
	
	// Add the user
	if (!m_user.empty ())
	{
		uri += m_user + "@";
	}
	
	// Add the address
	if (m_IpAddress.ValidGet ())
	{
		// It's an ip address
		uri += m_IpAddress.AddressGet (CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS);
	}
	else
	{
		// It's a domain
		uri += m_server;
	}

	if (!m_port.empty ())
	{
		uri += ":" + m_port;
	}

	return uri;
}


/// \brief Compares whether or not this address matches with any of the ip:port combinations given
///
/// If the dial string is found within any of the addresses on any of the ports, will
/// return estiTRUE
///
/// \returns estiTRUE or estiFALSE
///
bool CstiRoutingAddress::SameAs (
	const std::vector<std::string> &addresses, ///< Array of ip address strings.  Last entry must be NULL.
	const std::vector<int> &ports ///< Array of port numbers.  Last entry must be NULL.
) const
{
	bool same {false};

	if (m_port.empty ())
	{
		same = estiTRUE;
	}
	else
	{
		try
		{
			int port = std::stoi (m_port);

			for (auto &p: ports)
			{
				if (p == port)
				{
					same = true;
					break;
				}
			}
		}
		catch (...)
		{
			stiASSERTMSG(false, "port is not an integer: %s\n", m_port.c_str ());
		}
	}

	// If we had a match on ports, check the ip addresses
	if (same)
	{
		auto ipAddress = ipAddressStringGet ();

		same = false;
		for (auto &address: addresses)
		{
			if (address == ipAddress)
			{
				same = true;
				break;
			}
		}
	}

	return same;
}

bool CstiRoutingAddress::hasParameter (
	const std::string &parameter) const
{
	return m_parameters.find(parameter) != m_parameters.end ();
}


