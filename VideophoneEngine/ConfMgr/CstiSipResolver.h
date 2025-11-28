////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiSipResolver
//
//  File Name:	CstiSipResolver.h
//
//	Abstract:
//		See CstiSipResolver.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISIPRESOLVER_H
#define CSTISIPRESOLVER_H

#include "RvSipResolver.h"
#include <string>
#include <mutex>
#include "stiConfDefs.h"
#include "CstiSignal.h"
#include "CstiTimer.h"

enum EstiResolverEvent
{
	RESOLVE_EVENT_COMPLETE,
	RESOLVE_EVENT_FAILED,
	RESOLVE_OUT_OF_RESOURCES
};

class CstiSipResolver;

using CstiSipResolverSharedPtr = std::shared_ptr<CstiSipResolver>;
using CstiSipResolverWeakPtr = std::weak_ptr<CstiSipResolver>;

class CstiSipResolver : public std::enable_shared_from_this<CstiSipResolver>
{
public:

	enum EResolverType
	{
		ePublic = 0, // NOTE: unused functionality, needs to be removed
		eProxy, // Used for both agent and private domain registrations
	};

	CstiSipResolver (
		EResolverType eType,
		RvSipResolverMgrHandle hResolverMgr,
		RvSipTransportMgrHandle hTransportMgr,
		std::recursive_mutex &execMutex,
		HRPOOL hAppPool,
		const std::string &PrimaryProxyAddress,
		const std::string &AltProxyAddress,
		int nAllowedTransports,
		bool bAllowIPv6);

	~CstiSipResolver ();

	CstiSipResolver (const CstiSipResolver &other) = delete;
	CstiSipResolver (CstiSipResolver &&other) = delete;
	CstiSipResolver &operator= (const CstiSipResolver &other) = delete;
	CstiSipResolver &operator= (CstiSipResolver &&other) = delete;

	CstiSignal<EstiResolverEvent> eventSignal;

	stiHResult Resolve ();

	stiHResult Continue ();

	bool DNSListEmpty ();

	stiHResult Lock ();

	void Unlock ();

	EResolverType m_eType;
	bool m_bResolving;
	bool m_bCanceled;
	std::string m_PrimaryProxyAddress;
	std::string m_AltProxyAddress;
	int m_nAllowedTransports;
	std::string *m_pProxyAddress;
	EstiTransport m_eResolvedTransport;
	std::string m_ResolvedIPAddress;
	uint16_t m_un16ResolvedPort;
	CstiTimer m_DnsRetryTimer;

private:

	stiHResult ResolveAlt ();

	stiHResult SRVFromCacheResolve (
		const std::string &Address,
		bool bAllowExpired,
		bool *pbFound);

	stiHResult HostnameFromCacheResolve (
		const std::string &Hostname,
		bool bAllowExpired,
		bool *pbFound);

	stiHResult ResolveComplete (
		const char *pszIpAddress,
		uint16_t un16Port);

	bool AlternateProxyValid ();

	std::string m_QueriedName;
	RvSipTransport m_eQueriedTransport {RVSIP_TRANSPORT_UNDEFINED};

	RvSipResolverMgrHandle m_hResolverMgr;
	RvSipTransportMgrHandle m_hTransportMgr;
	HRPOOL m_hAppPool;

	RvSipResolverHandle m_hResolver;
	std::string m_SRVQueryString;
	std::string m_AltSRVQueryString;
	RvSipTransportDNSListHandle m_hDNSList;
	RvSipTransportDNSSRVElement m_SRVElement{};
	RvSipTransportDNSHostNameElement m_HostElement{};	// The information for the current Host to IP resolution process

	std::recursive_mutex &m_execMutex;

	bool m_bAllowIPv4;
	bool m_bAllowIPv6;
	bool m_bIPv4Resolved;
	bool m_bIPv6Resolved;

	stiHResult IPElementRetrieve ();

	stiHResult HostElementResolve ();

	stiHResult HostQueryStart (
		RvSipTransportAddressType eAddressType);

	stiHResult HostnameQueryFailureHandle (
		RvUint32 un32SrvElements,
		RvUint32 un32HostNameElements);

	stiHResult SRVElementResolve ();

	stiHResult SRVQueryStart ();

	stiHResult SRVElementsPush ();

	void AlternateSRVGet (
		const std::string &Address,
		std::string *pAlternateSRV);

	stiHResult SRVQueryFailureHandle ();

	// Keep self in scope while an outstanding callback is in place
	// NOTE: need to mutex protect this as it's referenced from several threads
	// NOTE: one drawback of this mechanism is that you can't destroy the object
	// while there's an outstanding callback (ie: if you want to cancel or interrupt)
	std::list<CstiSipResolverSharedPtr> m_outstandingCallbacks;
	void outstandingCallbackAdd ();
	void outstandingCallbackRemove ();

	static RvStatus RVCALLCONV ResolverCB (
		RvSipResolverHandle hResolver,
		RvSipAppResolverHandle hAppRslv,
		RvBool bError,
		RvSipResolverMode eMode);
};

#endif // CSTISIPRESOLVER_H
