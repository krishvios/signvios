// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIMEDIATRANSPORTS_H
#define CSTIMEDIATRANSPORTS_H

#include "rvtransport.h"
#include "rvnettypes.h"
#include "stiSVX.h"
#include <string>

class CstiTransport
{
public:
	CstiTransport (
		RvTransport transport,
		bool releaseNeeded)
	{
		m_transport = transport;
		m_releaseNeeded = releaseNeeded;
	}

	~CstiTransport ()
	{
		// If we have a transport from calling RvIceCandidateRetrieveTransport
		// we are responsible for calling release.
		if (m_releaseNeeded)
		{
			m_releaseNeeded = false;
			RvTransportRelease (m_transport);
		}
	}

	CstiTransport (const CstiTransport &other) = delete;
	CstiTransport (CstiTransport &&other) = delete;
	CstiTransport &operator= (const CstiTransport &other) = delete;
	CstiTransport &operator= (CstiTransport &&other) = delete;

	RvTransport TransportGet () const
	{
		return m_transport;
	}

private:
	RvTransport m_transport = nullptr;
	bool m_releaseNeeded = false;
};


class CstiMediaAddresses
{
public:

	CstiMediaAddresses ()
	{
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtpAddress);
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtcpAddress);
	}

	void RtpAddressSet (
		const RvAddress *pRtpAddress)
	{
		RvAddressCopy (pRtpAddress, &m_RtpAddress);
	}

	const RvAddress *RtpAddressGet () const
	{
		return &m_RtpAddress;
	}

	void RtcpAddressSet (
		const RvAddress *pRtcpAddress)
	{
		RvAddressCopy (pRtcpAddress, &m_RtcpAddress);
	}

	const RvAddress *RtcpAddressGet () const
	{
		return &m_RtcpAddress;
	}

	void isValidSet (
		bool valid)
	{
		m_isValid = valid;
	}
	
	bool isValid () const
	{
		return m_isValid;
	}
	
private:

	RvAddress m_RtpAddress{};
	RvAddress m_RtcpAddress{};
	
	bool m_isValid = false;
};


class CstiMediaTransports
{
public:
	
	CstiMediaTransports ()
	{
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtpLocalAddress);
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtcpLocalAddress);
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtpRemoteAddress);
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_RtcpRemoteAddress);
	}
	
	~CstiMediaTransports ()
	{
		if (m_RtpTransport)
		{
			m_RtpTransport = nullptr;
		}
		
		if (m_RtcpTransport)
		{
			m_RtcpTransport = nullptr;
		}
	}
	
	CstiMediaTransports (
		const CstiMediaTransports &Other)
	{
		RvAddressCopy (&Other.m_RtpLocalAddress, &m_RtpLocalAddress);
		RvAddressCopy (&Other.m_RtcpLocalAddress, &m_RtcpLocalAddress);
		
		RvAddressCopy (&Other.m_RtpRemoteAddress, &m_RtpRemoteAddress);
		RvAddressCopy (&Other.m_RtcpRemoteAddress, &m_RtcpRemoteAddress);

		m_RtpTransport = Other.m_RtpTransport;
		m_RtcpTransport = Other.m_RtcpTransport;

		m_eRtpType = Other.m_eRtpType;
		m_eRtcpType = Other.m_eRtcpType;
		
		m_isValid = Other.m_isValid;
		m_isRtcpMux = Other.m_isRtcpMux;
	}
	
	CstiMediaTransports (const CstiMediaTransports &&other) = delete;

	CstiMediaTransports &operator = (
		const CstiMediaTransports &Other)
	{
		if (this != &Other)
		{
			RtpLocalAddressSet (&Other.m_RtpLocalAddress);
			RtcpLocalAddressSet (&Other.m_RtcpLocalAddress);

			RtpRemoteAddressSet (&Other.m_RtpRemoteAddress);
			RtcpRemoteAddressSet (&Other.m_RtcpRemoteAddress);

			m_RtpTransport = Other.m_RtpTransport;
			m_RtcpTransport = Other.m_RtcpTransport;

			RtpTypeSet (Other.m_eRtpType);
			RtcpTypeSet (Other.m_eRtcpType);
			
			isValidSet(Other.m_isValid);
			isRtcpMuxSet(Other.m_isRtcpMux);
		}
		
		return *this;
	}
	
	CstiMediaTransports &operator= (CstiMediaTransports &&other) = delete;

	void RtpTransportSet (
		RvTransport rtpTransport,
		bool releaseNeeded)
	{
		m_RtpTransport = nullptr;
		if (rtpTransport)
		{
			m_RtpTransport = std::make_shared<CstiTransport>(rtpTransport, releaseNeeded);
		}
	}
	
	RvTransport RtpTransportGet () const
	{
		RvTransport transport = nullptr;
		
		if (m_RtpTransport)
		{
			transport =  m_RtpTransport->TransportGet();
		}
		
		return transport;
	}
	
	void RtcpTransportSet (
		RvTransport rtcpTransport,
		bool releaseNeeded)
	{
		m_RtcpTransport = nullptr;
		if (rtcpTransport)
		{
			m_RtcpTransport = std::make_shared<CstiTransport>(rtcpTransport, releaseNeeded);
		}
	}

	RvTransport RtcpTransportGet () const
	{
		RvTransport transport = nullptr;
		
		if (m_RtcpTransport)
		{
			transport =  m_RtcpTransport->TransportGet();
		}
		
		return transport;
	}
	
	void RtpLocalAddressSet (
		const RvAddress *pRtpAddress)
	{
		RvAddressCopy (pRtpAddress, &m_RtpLocalAddress);
	}
	
	const RvAddress *RtpLocalAddressGet () const
	{
		return &m_RtpLocalAddress;
	}
	
	void RtcpLocalAddressSet (
		const RvAddress *pRtcpAddress)
	{
		RvAddressCopy (pRtcpAddress, &m_RtcpLocalAddress);
	}
	
	const RvAddress *RtcpLocalAddressGet () const
	{
		return &m_RtcpLocalAddress;
	}
	
	void RtpRemoteAddressSet (
		const RvAddress *pRtpAddress)
	{
		RvAddressCopy (pRtpAddress, &m_RtpRemoteAddress);
	}

	const RvAddress *RtpRemoteAddressGet () const
	{
		return &m_RtpRemoteAddress;
	}

	void RtcpRemoteAddressSet (
		const RvAddress *pRtcpAddress)
	{
		RvAddressCopy (pRtcpAddress, &m_RtcpRemoteAddress);
	}

	const RvAddress *RtcpRemoteAddressGet () const
	{
		return &m_RtcpRemoteAddress;
	}

	stiHResult LocalAddressesGet (
		std::string *rtpAddress,
		int *rtpPort,
		std::string *rtcpAddress,
		int *rtcpPort) const
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvChar addrTxt1[stiIP_ADDRESS_LENGTH + 1];
		
		if (rtpAddress)
		{
			*rtpAddress = RvAddressGetString (RtpLocalAddressGet (), sizeof (addrTxt1), addrTxt1);
		}

		if (rtpPort)
		{
			*rtpPort = RvAddressGetIpPort (RtpLocalAddressGet ());
		}

		if (rtcpAddress)
		{
			*rtcpAddress = RvAddressGetString (RtcpLocalAddressGet (), sizeof (addrTxt1), addrTxt1);
		}
		
		if (rtcpPort)
		{
			*rtcpPort = RvAddressGetIpPort (RtcpLocalAddressGet ());
		}

		return hResult;
	}

	stiHResult RemoteAddressesGet (
		std::string* rtpAddress,
		int* rtpPort,
		std::string* rtcpAddress,
		int* rtcpPort) const
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvChar addrTxt1[stiIP_ADDRESS_LENGTH + 1];

		if (rtpAddress)
		{
			*rtpAddress = RvAddressGetString (RtpRemoteAddressGet (), sizeof (addrTxt1), addrTxt1);
		}

		if (rtpPort)
		{
			*rtpPort = RvAddressGetIpPort (RtpRemoteAddressGet ());
		}

		if (rtcpAddress)
		{
			*rtcpAddress = RvAddressGetString (RtcpRemoteAddressGet (), sizeof (addrTxt1), addrTxt1);
		}

		if (rtcpPort)
		{
			*rtcpPort = RvAddressGetIpPort (RtcpRemoteAddressGet ());
		}

		return hResult;
	}

	void RtpTypeSet (
		EstiMediaTransportType eType)
	{
		m_eRtpType = eType;
	}

	EstiMediaTransportType RtpTypeGet () const
	{
		return m_eRtpType;
	}

	void RtcpTypeSet (
		EstiMediaTransportType eType)
	{
		m_eRtcpType = eType;
	}

	EstiMediaTransportType RtcpTypeGet () const
	{
		return m_eRtcpType;
	}
	
	void isValidSet (
		bool valid)
	{
		m_isValid = valid;
	}
	
	bool isValid () const
	{
		return m_isValid;
	}
	
	void isRtcpMuxSet (bool mux)
	{
		m_isRtcpMux = mux;
	}
	
	bool isRtcpMux () const
	{
		return m_isRtcpMux;
	}

private:
	
	std::shared_ptr<CstiTransport> m_RtpTransport;
	std::shared_ptr<CstiTransport> m_RtcpTransport;
	
	RvAddress m_RtpLocalAddress{};
	RvAddress m_RtcpLocalAddress{};

	RvAddress m_RtpRemoteAddress{};
	RvAddress m_RtcpRemoteAddress{};
	
	EstiMediaTransportType m_eRtpType{estiMediaTransportUnknown};
	EstiMediaTransportType m_eRtcpType{estiMediaTransportUnknown};
	
	bool m_isValid = false;
	bool m_isRtcpMux = false;
};


#endif // CSTIMEDIATRANSPORTS_H
