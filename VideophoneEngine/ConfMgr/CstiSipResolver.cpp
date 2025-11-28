////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiSipResolver
//
//  File Name:	CstiSipResolver.cpp
//
//	Abstract: This class is used to resolve DNS names.  It will try SRV records
//  and A records.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiSipResolver.h"
#include "RvSipTransportDNS.h"
#include "RvSipTransport.h"
#include "stiTrace.h"
#include "CstiHostNameResolver.h"
#include "stiTools.h"

#define MAX_DNS_LIST_ENTRIES	15			// The maximum number of DNS list entries to support


// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(pThis,fmt,...) stiDEBUG_TOOL (g_stiSipResDebug, stiTrace ("SIP Resolver: %p/%s (%s): " fmt "\n", pThis, pThis->m_PrimaryProxyAddress.c_str(), pThis->m_pProxyAddress->c_str (), ##__VA_ARGS__); )

static const int DNS_RESOLUTION_RETRY_DELAY = 5000; // The amount of time to wait before retrying a DNS resolution in milliseconds


/*!\brief The constructor for the CstiSipResolver class
 *
 */
CstiSipResolver::CstiSipResolver (
	EResolverType eType,
	RvSipResolverMgrHandle hResolverMgr,
	RvSipTransportMgrHandle hTransportMgr,
	std::recursive_mutex &execMutex,
	HRPOOL hAppPool,
	const std::string &PrimaryProxyAddress,
	const std::string &AltProxyAddress,
	int nAllowedTransports,
	bool bAllowIPv6)
:
	m_eType (eType),
	m_bResolving (false),
	m_bCanceled (false),
	m_PrimaryProxyAddress (PrimaryProxyAddress),
	m_AltProxyAddress (AltProxyAddress),
	m_nAllowedTransports (nAllowedTransports),
	m_pProxyAddress (nullptr),
	m_eResolvedTransport (estiTransportUnknown),
	m_un16ResolvedPort (0),
	m_DnsRetryTimer (DNS_RESOLUTION_RETRY_DELAY),
	m_hResolverMgr (hResolverMgr),
	m_hTransportMgr (hTransportMgr),
	m_hAppPool (hAppPool),
	m_hResolver (nullptr),
	m_hDNSList (nullptr),
	m_execMutex (execMutex),
	m_bAllowIPv4 (true),
	m_bAllowIPv6 (bAllowIPv6),
	m_bIPv4Resolved (true),
	m_bIPv6Resolved (true)
{
	//
	// Always use IPv4 and never IPv6 for public
	//
	if (m_eType == ePublic)
	{
		m_bAllowIPv6 = false;
		m_bAllowIPv4 = true;
	}
	//
	// Create a Radvision sip resolver
	//
	RvStatus rvStatus = RvSipResolverMgrCreateResolver (m_hResolverMgr, (RvSipAppResolverHandle)this, &m_hResolver);
	stiASSERT (rvStatus == RV_OK);

}


/*!\brief The destructor for the CstiSipResolver class
 *
 */
CstiSipResolver::~CstiSipResolver()
{
	if (m_hDNSList)
	{
		RvSipTransportDNSListDestruct (m_hTransportMgr, m_hDNSList);
		m_hDNSList = nullptr;
	}

	if (m_hResolver)
	{
		RvSipResolverTerminate (m_hResolver);
		m_hResolver = nullptr;
	}
}


/*!\brief Locks the resolver object
 *
 * @return Success or failure
 */
stiHResult CstiSipResolver::Lock ()
{
	m_execMutex.lock ();
	return stiRESULT_SUCCESS;
}


/*!\brief Unlocks the resolver object
 *
 */
void CstiSipResolver::Unlock ()
{
	m_execMutex.unlock ();
}


/*!\brief Determines if the alternate proxy address is valid.
 *
 * An invalid alternate proxy is one where it is either empty or matches the primary address.
 *
 */
bool CstiSipResolver::AlternateProxyValid ()
{
	bool bResult = true;

	//
	// If the alternate proxy address matches the primary then
	// clear out the alternate string.
	//
	if (m_AltProxyAddress.empty() || m_AltProxyAddress == m_PrimaryProxyAddress)
	{
		bResult = false;
	}

	return bResult;
}


/*!\brief Returns the default port for the provided transport.
 *
 */
static uint16_t DefaultPortGet (
	RvSipTransport eTransport)
{
	uint16_t un16Port = nDEFAULT_SIP_LISTEN_PORT;

	if (eTransport == RVSIP_TRANSPORT_TLS)
	{
		un16Port = nDEFAULT_TLS_SIP_LISTEN_PORT;
	}

	return un16Port;
}


/*!\brief The method use to start the DNS resolution process
 *
 */
stiHResult CstiSipResolver::Resolve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	stiTESTCOND (!m_bResolving, stiRESULT_ERROR);

	stiTESTCOND (m_hResolver != nullptr, stiRESULT_ERROR);
	//
	// We want to start with an empty DNS list.  So, if we have a list then destroy it.
	//
	if (m_hDNSList != nullptr)
	{
		RvSipTransportDNSListDestruct (m_hTransportMgr, m_hDNSList);
		m_hDNSList = nullptr;
	}

	rvStatus = RvSipTransportDNSListConstruct (m_hTransportMgr, m_hAppPool, MAX_DNS_LIST_ENTRIES, &m_hDNSList);
	stiTESTRVSTATUS ();

	//
	// If the primary is an IP address then just use it.
	//
	if (IPAddressValidate (m_PrimaryProxyAddress.c_str ()))
	{
		m_pProxyAddress = &m_PrimaryProxyAddress;
		m_ResolvedIPAddress = m_PrimaryProxyAddress;

		//
		// Set the transport and port based on the allowed transports with
		// TLS being preferred over TCP.
		//
		if (m_nAllowedTransports & estiTransportTLS)
		{
			m_eResolvedTransport = estiTransportTLS;
			m_un16ResolvedPort = nDEFAULT_TLS_SIP_LISTEN_PORT;
		}
		else if (m_nAllowedTransports & estiTransportTCP)
		{
			m_eResolvedTransport = estiTransportTCP;
			m_un16ResolvedPort = nDEFAULT_SIP_LISTEN_PORT;
		}
		else
		{
			//
			// We shouldn't get here.  However, if we do then assert
			// and set the transport to TLS.
			//
			stiASSERT (estiFALSE);
			m_eResolvedTransport = estiTransportTLS;
			m_un16ResolvedPort = nDEFAULT_TLS_SIP_LISTEN_PORT;
		}

		// Set resolving to true so the resolver is not deleted prior to the event being handled.
		m_bResolving = true;

		eventSignal.Emit (RESOLVE_EVENT_COMPLETE);
	}
	else
	{
		hResult = SRVElementsPush ();
		stiTESTRESULT ();

		hResult = SRVElementResolve ();
		stiTESTRESULT ();
	}

STI_BAIL:
	
	if (rvStatus == RV_ERROR_OUTOFRESOURCES)
	{
		eventSignal.Emit (RESOLVE_OUT_OF_RESOURCES);
	}

	return hResult;
}


/*!\brief Pushes SRV records onto the DNS list
 *
 * Note: higher priority transports are pushed last.
 */
stiHResult CstiSipResolver::SRVElementsPush ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	RvSipTransportDNSSRVElement SRVElement;

	//
	// If TCP is allowed push the TCP SRV record onto the stack.
	//
	if (m_nAllowedTransports & estiTransportTCP)
	{
		sprintf (SRVElement.srvName, "_sip._tcp.%s", m_PrimaryProxyAddress.c_str ());
		SRVElement.protocol = RVSIP_TRANSPORT_TCP;
		SRVElement.order = 0;
		SRVElement.preference = 0;

		rvStatus = RvSipTransportDNSListPushSrvElement(m_hTransportMgr, m_hDNSList, &SRVElement);
		stiTESTRVSTATUS ();
	}

	//
	// If TLS is allowed push the TLS SRV record onto the stack.
	//
	if (m_nAllowedTransports & estiTransportTLS)
	{
		sprintf (SRVElement.srvName, "_sips._tls.%s", m_PrimaryProxyAddress.c_str ());
		SRVElement.protocol = RVSIP_TRANSPORT_TLS;
		SRVElement.order = 0;
		SRVElement.preference = 0;

		rvStatus = RvSipTransportDNSListPushSrvElement(m_hTransportMgr, m_hDNSList, &SRVElement);
		stiTESTRVSTATUS ();
	}
	
STI_BAIL:
	
	return hResult;
}


/*!\brief Substitutes the alternate host name into the provided SRV string.
 *
 * The substitution only takes place if the SRV record currently contains the primary host name.
 *
 * \param Address The current SRV string
 * \param pAlternateSRV The resulting SRV string after replacement.
 */
void CstiSipResolver::AlternateSRVGet (
	const std::string &Address,
	std::string *pAlternateSRV)
{
	pAlternateSRV->clear ();

	if (AlternateProxyValid ())
	{
		size_t Position = Address.find (m_PrimaryProxyAddress);

		if (Position != std::string::npos && Address.compare (Position, std::string::npos, m_PrimaryProxyAddress) == 0)
		{
			*pAlternateSRV = Address;
			pAlternateSRV->replace (Position, m_PrimaryProxyAddress.length (), m_AltProxyAddress);
		}
	}
}


/*!\brief Looks into the cache for the primary and alternate domains to see if they are stored there
 *
 * @return Success if one of the domain names were found.  Otherwise, an error result is returned.
 */
stiHResult CstiSipResolver::SRVFromCacheResolve (
	const std::string &Address,
	bool bAllowExpired,
	bool *pbFound)
{
	stiHResult hResult;
	std::vector<SstiSRVRecord> SRVRecords;
	bool bFound = false;

	DBG_MSG (this, "Checking %scache for %s", bAllowExpired ? "" : "fresh ", Address.c_str ());

	hResult = CstiHostNameResolver::getInstance ()->ResolvedSRVGet (Address.c_str (), nullptr, &SRVRecords, bAllowExpired);

	if (stiIS_ERROR (hResult))
	{
		hResult = stiRESULT_SUCCESS;

		//
		// Could not find the SRV in the cache.  Try replacing the hostname portion
		// of the string with the alternate proxy name and try again.
		//
		std::string AlternateSRV;

		AlternateSRVGet (Address, &AlternateSRV);

		if (!AlternateSRV.empty ())
		{
			DBG_MSG (this, "Checking %scache for %s", bAllowExpired ? "" : "fresh ", AlternateSRV.c_str ());

			hResult = CstiHostNameResolver::getInstance ()->ResolvedSRVGet (AlternateSRV.c_str (), nullptr, &SRVRecords, bAllowExpired);

			if (!stiIS_ERROR (hResult))
			{
				m_QueriedName = AlternateSRV;
				m_pProxyAddress = &m_AltProxyAddress;
			}
			else
			{
				hResult = stiRESULT_SUCCESS;
			}
		}
	}

	//
	// If we have a list of SRV records then put them into the DNS list
	//
	if (!SRVRecords.empty ())
	{
		DBG_MSG (this, "Found %scached SRV records for %s", bAllowExpired ? "" : "freshly ", Address.c_str ());

		//
		// Since the vector is in order of priority but we are pushing the elements
		// onto the DNS list we need to traverse the vector in reverse order so that
		// the DNS list is in priority order.
		//
		std::vector<SstiSRVRecord>::const_reverse_iterator i;

		for (i = SRVRecords.rbegin (); i != SRVRecords.rend (); ++i)
		{
			RvSipTransportDNSHostNameElement HostElement;

			strncpy (HostElement.hostName, (*i).HostName.c_str (), sizeof (HostElement.hostName) - 1);
			HostElement.hostName[sizeof (HostElement.hostName) - 1] = '\0';
			HostElement.port = (*i).un16Port;
			HostElement.protocol = (RvSipTransport)(*i).un16Protocol;
			HostElement.priority = (*i).un16Priority;
			HostElement.weight = (*i).un16Weight;

			RvSipTransportDNSListPushHostElement(m_hTransportMgr, m_hDNSList, &HostElement);

			DBG_MSG (this, "SRV Record: %s %d %d %d %d", HostElement.hostName, HostElement.port, HostElement.protocol, HostElement.priority, HostElement.weight);
		}

		bFound = true;
	}

	*pbFound = bFound;

//STI_BAIL:

	return hResult;
}


/*!\brief Looks into the cache for the primary and alternate domains to see if they are stored there
 *
 * @return Success if one of the domain names were found.  Otherwise, an error result is returned.
 */
stiHResult CstiSipResolver::HostnameFromCacheResolve (
	const std::string &Hostname,
	bool bAllowExpired,
	bool *pbFound)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::vector<std::string> IPRecords;
	bool bFound = false;

	hResult = CstiHostNameResolver::getInstance ()->ResolvedGet (Hostname.c_str (), &IPRecords, bAllowExpired);

	//
	// If we have a list of SRV records then put them into the DNS list
	//
	if (!IPRecords.empty ())
	{
		//
		// Since the vector is in order of priority but we are pushing the elements
		// onto the DNS list we need to traverse the vector in reverse order so that
		// the DNS list is in priority order.
		//
		std::vector<std::string>::const_reverse_iterator i;

		for (i = IPRecords.rbegin (); i != IPRecords.rend (); ++i)
		{
			RvSipTransportDNSIPElement IPElement;
			RvSipTransportAddressType eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;

			if ((*i).find (':') == std::string::npos)
			{
				eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
				IPElement.bIsIpV6 = false;
			}
			else
			{
				eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
				IPElement.bIsIpV6 = true;
			}

			//
			// If this IP address is of the version that we allow then add it to the DNS list.
			//
			if ((IPElement.bIsIpV6 && m_bAllowIPv6) || (!IPElement.bIsIpV6 && m_bAllowIPv4))
			{
				RvSipTransportConvertStringToIp (m_hTransportMgr, (RvChar *)(*i).c_str (), eAddressType, IPElement.ip);
				IPElement.port = m_HostElement.port;
				IPElement.protocol = m_HostElement.protocol;

				RvSipTransportDNSListPushIPElement(m_hTransportMgr, m_hDNSList, &IPElement);

				bFound = true;

				DBG_MSG (this, "IP Record: %s %s", Hostname.c_str (), (*i).c_str ());
			}
		}

		*pbFound = bFound;
	}

	return hResult;
}


/*!\brief Continues the resolution process by working on the next item in the DNS list.
 *
 */
stiHResult CstiSipResolver::Continue ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvUint32 un32SrvElements = 0;
	RvUint32 un32HostNameElements = 0;
	RvUint32 un32IpAddrElements = 0;

	if (m_bCanceled)
	{
		//
		// This resolver has been canceled.  Just call the appropriate callback.
		//
		eventSignal.Emit (RESOLVE_EVENT_FAILED);
	}
	else
	{
		stiTESTCOND (!m_bResolving, stiRESULT_ERROR);

		DBG_MSG (this, "Continuing resolution process.");

		if (m_hDNSList)
		{
			rvStatus = RvSipTransportGetNumberOfDNSListEntries (m_hTransportMgr, m_hDNSList, &un32SrvElements, &un32HostNameElements, &un32IpAddrElements);
			stiTESTRVSTATUS ();
		}

		if (un32IpAddrElements > 0)
		{
			hResult = IPElementRetrieve ();
			stiTESTRESULT ();
		}
		else if (m_bAllowIPv4 && m_bIPv6Resolved && !m_bIPv4Resolved)
		{
			//
			// If we have resolved the host name for IPv6 but haven't yet resolved
			// it for IPv4 then resolve it for IPv4.
			// We don't need to check the other case (resolved for IPv4 but
			// not yet for IPv6) because host names will always be resolved for IPv6 first
			// or not at all.
			//
			hResult = HostQueryStart(RVSIP_TRANSPORT_ADDRESS_TYPE_IP);
			stiTESTRESULT ();
		}
		else if (un32HostNameElements > 0)
		{
			//
			// No IP elements in the list.
			// Try Host elements.
			//

			hResult = HostElementResolve ();
			stiTESTRESULT ();
		}
		else if (un32SrvElements > 0)
		{
			hResult = SRVElementResolve ();
			stiTESTRESULT ();
		}
		else
		{
			//
			// The DNS list has been exhausted.  Start the resolution process from the start.
			//
			DBG_MSG (this, "DNS List is empty.  Starting resolution process over.");

			hResult = Resolve ();
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Return true if the list is empty.
 *
 */
bool CstiSipResolver::DNSListEmpty ()
{
	bool bResult = true;

	if (m_hTransportMgr && m_hDNSList)
	{
		RvUint32 un32SrvElements = 0;
		RvUint32 un32HostNameElements = 0;
		RvUint32 un32IpAddrElements = 0;

		RvStatus rvStatus = RvSipTransportGetNumberOfDNSListEntries (m_hTransportMgr, m_hDNSList, &un32SrvElements, &un32HostNameElements, &un32IpAddrElements);

		if (rvStatus == RV_OK)
		{
			bResult = (un32SrvElements != 0 || un32HostNameElements != 0 || un32IpAddrElements != 0);
		}
	}

	return bResult;
}


/*!\brief Retrieves the next IP element in the DNS list and returns it to the upper layer.
 *
 */
stiHResult CstiSipResolver::IPElementRetrieve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvSipTransportDNSIPElement IPElement;
	char szResolvedIPAddress[RV_ADDRESS_MAXSTRINGSIZE + 1];
	RvSipTransportAddressType eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;

	rvStatus = RvSipTransportDNSListPopIPElement (m_hTransportMgr, m_hDNSList, &IPElement);
	stiTESTRVSTATUS ();

	//
	// Determine the IP version.
	//
	if (IPElement.bIsIpV6)
	{
		eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
		m_bIPv6Resolved = true;
	}
	else
	{
		m_bIPv4Resolved = true;
	}

	//
	// Retrieve the address.
	//
	rvStatus = RvSipTransportConvertIpToString (m_hTransportMgr, IPElement.ip, eAddressType,
									 sizeof (szResolvedIPAddress), szResolvedIPAddress);
	stiTESTRVSTATUS ();

	m_ResolvedIPAddress = szResolvedIPAddress;
	m_un16ResolvedPort = IPElement.port;

	if (IPElement.protocol == RVSIP_TRANSPORT_TCP)
	{
		m_eResolvedTransport = estiTransportTCP;
	}
	else if (IPElement.protocol == RVSIP_TRANSPORT_TLS)
	{
		m_eResolvedTransport = estiTransportTLS;
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

	DBG_MSG (this, "IP and Port found: %s:%d", m_ResolvedIPAddress.c_str (), m_un16ResolvedPort);

	// Set resolving to true so the resolver is not deleted prior to the event being handled.
	m_bResolving = true;

	eventSignal.Emit (RESOLVE_EVENT_COMPLETE);

STI_BAIL:

	return hResult;
}

/*! \brief hold ourselves in scope during outstanding radvision operation
 *
 * NOTE: must lock since the callbacks come on a different thread
 *
 * NOTE: must call this whenever setting up ResolverCB, since it calls
 * outstandingCallbackRemove ()
 */
void CstiSipResolver::outstandingCallbackAdd ()
{
	Lock ();
	m_outstandingCallbacks.push_back (shared_from_this ());
	Unlock ();
}

/*! \brief remove reference to ourselves when radvision comes back
 *
 * NOTE: must lock since the callbacks come on a different thread
 *
 * NOTE: this call must coincide with a previous call to
 * outstandingCallbackAdd ()
 */
void CstiSipResolver::outstandingCallbackRemove ()
{
	Lock ();
	m_outstandingCallbacks.pop_back ();
	Unlock ();
}


/*!\brief Starts a DNS query for the host name for the given address type.
 *
 * \param eAddressType The address type: IPv4, IPv6 or undefined
 */
stiHResult CstiSipResolver::HostQueryStart (
	RvSipTransportAddressType eAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	m_bResolving = true;
	outstandingCallbackAdd ();

	DBG_MSG (this, "Resolving %s as address type %s", m_HostElement.hostName,
			eAddressType == RVSIP_TRANSPORT_ADDRESS_TYPE_UNDEFINED ? "Any" :
			eAddressType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP ? "IPv4" :
			eAddressType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP6 ? "IPv6" : "Unknown");

	m_QueriedName = m_HostElement.hostName;

	if (m_QueriedName == m_PrimaryProxyAddress)
	{
		m_pProxyAddress = &m_PrimaryProxyAddress;
	}
	else if (m_QueriedName == m_AltProxyAddress)
	{
		m_pProxyAddress = &m_AltProxyAddress;
	}

	if (eAddressType != RVSIP_TRANSPORT_ADDRESS_TYPE_UNDEFINED)
	{
		rvStatus = RvSipResolverResolveExt (m_hResolver, RVSIP_RESOLVER_MODE_FIND_IP_BY_HOST, m_HostElement.hostName,
							RVSIP_RESOLVER_SCHEME_SIP, false, m_HostElement.port, m_HostElement.protocol,
							eAddressType, m_hDNSList, ResolverCB);
		stiTESTRVSTATUS ();
	}
	else
	{
		//
		// Let the SIP stack decide which IP version to resolve first.
		//
		rvStatus = RvSipResolverResolve (m_hResolver, RVSIP_RESOLVER_MODE_FIND_IP_BY_HOST, m_HostElement.hostName,
							RVSIP_RESOLVER_SCHEME_SIP, false, m_HostElement.port, m_HostElement.protocol,
							m_hDNSList, ResolverCB);
		stiTESTRVSTATUS ();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		m_bResolving = false;
		outstandingCallbackRemove ();
	}

	return hResult;
}


/*!\brief Starts the resolution process on the next host name in the DNS list.
 *
 */
stiHResult CstiSipResolver::HostElementResolve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvSipTransportAddressType eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_UNDEFINED;

	//
	// The host was used to resolve for all allowed IP protocols.
	// Retrieve a new host to begin the resolution process
	//
	m_bIPv4Resolved = false;
	m_bIPv6Resolved = false;

	rvStatus = RvSipTransportDNSListPopHostElement (m_hTransportMgr, m_hDNSList, &m_HostElement);
	stiTESTRVSTATUS ();

	if (!m_bAllowIPv6) // IPv6 is not allowed so force the resolution to IPv4
	{
		eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
	}
	else if (!m_bAllowIPv4)  // IPv4 is not allowed so force the resolution to IPv6
	{
		eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
	}

	//
	// Continue on to find the host ip.
	//
	hResult = HostQueryStart (eAddressType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*!\brief Starts an SRV DNS query.
 *
 */
stiHResult CstiSipResolver::SRVQueryStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	RvStatus rvStatus = RV_OK;

	m_bResolving = true;
	outstandingCallbackAdd ();

	rvStatus = RvSipResolverResolve (m_hResolver, RVSIP_RESOLVER_MODE_FIND_HOSTPORT_BY_SRV_STRING,
						  (RvChar *)m_QueriedName.c_str (), RVSIP_RESOLVER_SCHEME_SIP,
						  false, DefaultPortGet (m_eQueriedTransport), m_eQueriedTransport,
						  m_hDNSList, ResolverCB);
	stiTESTRVSTATUS ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		m_bResolving = false;
		outstandingCallbackRemove ();
	}

	return hResult;
}


/*!\brief Starts the resolution process on the next SRV element in the DNS list.
 *
 */
stiHResult CstiSipResolver::SRVElementResolve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	bool bFoundInCache = false;

	rvStatus = RvSipTransportDNSListPopSrvElement (m_hTransportMgr, m_hDNSList, &m_SRVElement);
	stiTESTRVSTATUS ();

	m_pProxyAddress = &m_PrimaryProxyAddress;
	m_QueriedName = m_SRVElement.srvName;
	m_eQueriedTransport = m_SRVElement.protocol;

	//
	// Check the cache to see if there was a recent lookup.
	//
	hResult = SRVFromCacheResolve (m_SRVElement.srvName, false, &bFoundInCache);
	stiTESTRESULT ();

	if (!bFoundInCache)
	{
		DBG_MSG (this, "No fresh cache. Starting SRV resolution for %s", m_QueriedName.c_str ());
		// Nothing fresh available.  Perform the request using Radvision instead.

		hResult = SRVQueryStart ();
		stiTESTRESULT ();
	}
	else
	{
		hResult = HostElementResolve ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}


/*!\brief Handles a failure with SRV query
 *
 */
stiHResult CstiSipResolver::SRVQueryFailureHandle ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// SRV query failed.  If this was the primary then substitute the primary name
	// with the alternate and try again.  If this was the alternate then
	// try to find the resolutions in the stale cache.  If that is not successful
	// then try to lookup the primary as a host record.
	//
	std::string AlternateSRV;

	AlternateSRVGet (m_QueriedName, &AlternateSRV);

	if (!AlternateSRV.empty ())
	{
		//
		// We have an alternate SRV.  Start the SRV query
		//
		m_pProxyAddress = &m_AltProxyAddress;
		m_QueriedName = AlternateSRV;

		hResult = SRVQueryStart ();
		stiTESTRESULT ();
	}
	else
	{
		//
		// We don't have an alternate SRV so try resolving the SRV
		// from the stale cache.
		//
		bool bFoundInCache = false;

		hResult = SRVFromCacheResolve (m_SRVElement.srvName, true, &bFoundInCache);
		stiTESTRESULT ();

		if (!bFoundInCache)
		{
			//
			// Continue on to find the host ip as an A/AAAA record.
			// Push the information onto the DNS list and let the normal process resolve the name.
			//

			RvSipTransportDNSHostNameElement HostElement;

			if (AlternateProxyValid ())
			{
				strncpy (HostElement.hostName, m_AltProxyAddress.c_str (), sizeof (HostElement.hostName) - 1);
				HostElement.hostName[sizeof (HostElement.hostName) - 1] = '\0';
				HostElement.port = DefaultPortGet (m_eQueriedTransport);
				HostElement.protocol = m_eQueriedTransport;
				HostElement.priority = 0;
				HostElement.weight = 0;

				RvSipTransportDNSListPushHostElement(m_hTransportMgr, m_hDNSList, &HostElement);
			}

			strncpy (HostElement.hostName, m_PrimaryProxyAddress.c_str (), sizeof (HostElement.hostName) - 1);
			HostElement.hostName[sizeof (HostElement.hostName) - 1] = '\0';
			HostElement.port = DefaultPortGet (m_eQueriedTransport);
			HostElement.protocol = m_eQueriedTransport;
			HostElement.priority = 0;
			HostElement.weight = 0;

			RvSipTransportDNSListPushHostElement(m_hTransportMgr, m_hDNSList, &HostElement);
		}

		hResult = HostElementResolve ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}


/*!\brief Handles failures for hostname queries.
 *
 * This function first checks to see if there are any stale entries in the cache for
 * the hostname that failed.  If there are then it will use those.  If not,
 * it will continue on with the next hostname or SRV resolution.  If the DNS list
 * is empty it will do a callback with an error.
 */
stiHResult CstiSipResolver::HostnameQueryFailureHandle (
	RvUint32 un32SrvElements,
	RvUint32 un32HostNameElements)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = true;

	//
	// If the hostname failed to resolve then indicate that both IPv4 and IPv6
	// resolutions were tried.  Radvision will try IPv6 (if enabled) and if there is no
	// answer then it will try IPv4.
	//
	m_bIPv4Resolved = true;
	m_bIPv6Resolved = true;

	//
	// Both the primary and alternate proxy domains failed to resolve.  Try resolving from
	// the cache.
	//
	DBG_MSG (this, "Hostname query for %s failed.  Trying cache.", m_QueriedName.c_str ());

	stiHResult hResult2 = HostnameFromCacheResolve (m_QueriedName, true, &bFound);

	if (stiIS_ERROR (hResult2))
	{
		DBG_MSG (this, "Could not resolve hostname %s from cache.", m_QueriedName.c_str ());

		//
		// No entry found in cache.  If there are hostnames or
		// SRV record still in the list then try resolving those.
		//
		if (un32HostNameElements > 0)
		{
			HostElementResolve ();
		}
		else if (un32SrvElements > 0)
		{
			SRVElementResolve();
		}
		else
		{
			eventSignal.Emit (RESOLVE_EVENT_FAILED);
		}
	}
	else
	{
		DBG_MSG (this, "Found cached IP records for %s", m_QueriedName.c_str ());


		hResult = IPElementRetrieve ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}


/*!\brief The callback method that is called when the Radvision stack completes a DNS resoltion task
 *
 * \param hResolver The handle to the Radvision Resolver object
 * \param hAppRslv The handle to the application resolver object (a pointer to a CstiSipResolver object)
 * \param bError Indicates whether or not an error occurred
 * \param eMode The mode of the DNS resolution task
 */
RvStatus RVCALLCONV CstiSipResolver::ResolverCB (
	RvSipResolverHandle hResolver,
	RvSipAppResolverHandle hAppRslv,
	RvBool bError,
	RvSipResolverMode eMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	auto pThis = (CstiSipResolver *)hAppRslv;
	RvSipTransportDNSListHandle hDNSList = nullptr;

	pThis->Lock ();

	pThis->m_bResolving = false;

	//
	// If this resolver was canceled then just perform the appropriate callback.
	//
	if (pThis->m_bCanceled)
	{
		pThis->eventSignal.Emit (RESOLVE_EVENT_FAILED);
	}
	else
	{
		RvSipResolverGetDnsList (hResolver, &hDNSList);

		RvUint32 un32SrvElements = 0;
		RvUint32 un32HostNameElements = 0;
		RvUint32 un32IpAddrElements = 0;

		RvSipTransportGetNumberOfDNSListEntries (pThis->m_hTransportMgr, hDNSList, &un32SrvElements, &un32HostNameElements, &un32IpAddrElements);

		if (bError)
		{
			//
			// Failed current operation
			//

			switch (eMode)
			{
				case RVSIP_RESOLVER_MODE_FIND_HOSTPORT_BY_SRV_STRING:
				{
					DBG_MSG (pThis, "SRV resolution failed for %s", pThis->m_QueriedName.c_str ());

					hResult = pThis->SRVQueryFailureHandle ();
					stiTESTRESULT ();

					break;
				}

				case RVSIP_RESOLVER_MODE_FIND_IP_BY_HOST:
				{
					hResult = pThis->HostnameQueryFailureHandle (un32SrvElements, un32HostNameElements);
					stiTESTRESULT ();

					break;
				}

				case RVSIP_RESOLVER_MODE_UNDEFINED:
				case RVSIP_RESOLVER_MODE_FIND_TRANSPORT_BY_NAPTR:
				case RVSIP_RESOLVER_MODE_FIND_TRANSPORT_BY_3WAY_SRV:
				case RVSIP_RESOLVER_MODE_FIND_HOSTPORT_BY_TRANSPORT:
				case RVSIP_RESOLVER_MODE_FIND_URI_BY_NAPTR:

					break;
			}
		}
		else
		{
			//
			// Successfully completed current operation
			//

			switch (eMode)
			{
				case RVSIP_RESOLVER_MODE_FIND_HOSTPORT_BY_SRV_STRING:
				{
					DBG_MSG (pThis, "SRV resolution succeeded for %s", pThis->m_QueriedName.c_str ());

					//
					// Retrieve and store the results
					//
					SstiSRVRecord SRVRecord;
					std::vector<SstiSRVRecord> SRVRecords;
					void *pRelative = nullptr;
					RvSipTransportDNSHostNameElement HostElement;

					for (;;)
					{
						rvStatus = RvSipTransportDNSListGetHostElement (
							pThis->m_hTransportMgr, pThis->m_hDNSList, RVSIP_NEXT_ELEMENT, &pRelative, &HostElement);

						if (rvStatus != RV_OK)
						{
							break;
						}

						SRVRecord.HostName = HostElement.hostName;
						SRVRecord.un16Port = HostElement.port;
						SRVRecord.un16Protocol = HostElement.protocol;
						SRVRecord.un16Priority = HostElement.priority;
						SRVRecord.un16Weight = HostElement.weight;

						SRVRecords.push_back(SRVRecord);

						DBG_MSG (pThis, "SRV Record: %s %d %d %d %d", HostElement.hostName, HostElement.port, HostElement.protocol, HostElement.priority, HostElement.weight);
					}

					if (!SRVRecords.empty ())
					{
						// Update our robust DNS system with the new information for the domain lookup
						CstiHostNameResolver::getInstance ()->ResolvedSRVStore (
							pThis->m_QueriedName.c_str (),
							SRVRecords);
					}

					hResult = pThis->HostElementResolve ();
					
					if (stiIS_ERROR(hResult))
					{
						hResult = pThis->HostnameQueryFailureHandle (un32SrvElements, un32HostNameElements);
						stiTESTRESULT ();
					}

					break;
				}

				case RVSIP_RESOLVER_MODE_FIND_IP_BY_HOST:
				{
					DBG_MSG (pThis, "Host-to-IP resolution succeeded for %s", pThis->m_QueriedName.c_str ());

					//
					// Retrieve and store the results
					//
					SstiSRVRecord SRVRecord;
					std::vector<std::string> IPRecords;
					void *pRelative = nullptr;
					RvSipTransportDNSIPElement IPElement;
					char szResolvedIPAddress[RV_ADDRESS_MAXSTRINGSIZE + 1];

					for (;;)
					{
						rvStatus = RvSipTransportDNSListGetIPElement (
							pThis->m_hTransportMgr, pThis->m_hDNSList, RVSIP_NEXT_ELEMENT, &pRelative, &IPElement);

						if (rvStatus != RV_OK)
						{
							break;
						}

						RvSipTransportAddressType eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;

						//
						// Determine the IP version.
						//
						if (IPElement.bIsIpV6)
						{
							eAddressType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
						}

						//
						// Retrieve the address.
						//
						rvStatus = RvSipTransportConvertIpToString (pThis->m_hTransportMgr, IPElement.ip, eAddressType,
														 sizeof (szResolvedIPAddress), szResolvedIPAddress);
						stiTESTRVSTATUS ();

						IPRecords.push_back(szResolvedIPAddress);

						DBG_MSG (pThis, "Address Record: %s %s", pThis->m_QueriedName.c_str (), szResolvedIPAddress);
					}

					// Update our robust DNS system with the new information for the domain lookup
					CstiHostNameResolver::getInstance ()->ResolvedAddressStore (
						pThis->m_QueriedName.c_str (),
						IPRecords);

					pThis->IPElementRetrieve ();

					break;
				}

				case RVSIP_RESOLVER_MODE_UNDEFINED:
				case RVSIP_RESOLVER_MODE_FIND_TRANSPORT_BY_3WAY_SRV:
				case RVSIP_RESOLVER_MODE_FIND_HOSTPORT_BY_TRANSPORT:
				case RVSIP_RESOLVER_MODE_FIND_URI_BY_NAPTR:
				case RVSIP_RESOLVER_MODE_FIND_TRANSPORT_BY_NAPTR:

					break;
			}
		}
	}

STI_BAIL:

	pThis->outstandingCallbackRemove ();

	pThis->Unlock ();

	return rvStatus;
}


