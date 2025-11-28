////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiRoutingAddress
//
//  File Name:  CstiRoutingAddress.h
//
//  Abstract:
//  See CstiRoutingAddress.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIROUTINGADDRESS_H
#define CSTIROUTINGADDRESS_H


#include "stiSVX.h"
#include "stiConfDefs.h"
#include "CstiIpAddress.h"
#include <string>

/*!
* \brief Represents and encapsulates a dialed string and its operations.
*
*/
class CstiRoutingAddress
{
public:
	CstiRoutingAddress () = default;
	CstiRoutingAddress (const CstiRoutingAddress &other) = default;
	CstiRoutingAddress (CstiRoutingAddress &&other) = default;
	CstiRoutingAddress (const std::string &dialString);

	CstiRoutingAddress& operator = (const CstiRoutingAddress &other) = default;
	CstiRoutingAddress& operator = (CstiRoutingAddress &&other) = default;

	~CstiRoutingAddress () = default;

	std::string originalStringGet () const
		{ return m_originalDialString; }
		
	std::string ipAddressStringGet () const;

	CstiIpAddress ipAddressGet() const;

	void ipAddressSet(const std::string &ipAddress);
	
	std::string UriGet () const;

	std::string ResolvedLocationGet () const;

	std::string ResolvedURIGet () const;
	
	std::string userGet () const
	{
		return m_user;
	}

	EstiProtocol protocolGet ()
	{
		return m_eProtocol;
	}

	inline bool ProtocolMatch (EstiProtocol eProtocol) const
		{ return (m_eProtocol == estiPROT_UNKNOWN || m_eProtocol == eProtocol); }

	bool SameAs (
		const std::vector<std::string> &addresses,
		const std::vector<int> &ports) const;

	EstiTransport TransportGet () const
		{ return m_eTransport; }

	inline bool IsValid () const
		{ return m_isValid; }

	bool hasParameter (
		const std::string &parameter) const;

private:
	void DialStringSet (const std::string &dialString);

	void parseParameters (
		const std::string &parametersString);

	EstiTransport transporParamGet () const;

	std::string m_originalDialString;
	CstiIpAddress m_IpAddress;
	std::string m_user;
	std::string m_server;
	std::string m_port;
	std::map<std::string, std::string> m_parameters;
	EstiTransport m_eTransport{estiTransportUnknown};
	bool m_isValid{true};
	EstiProtocol m_eProtocol{estiPROT_UNKNOWN};
};

#endif // CSTIROUTINGADDRESS_H
// end file CstiRoutingAddress.h
