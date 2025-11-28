/*!
 * \file CstiHostNameResolver.h
 * \brief Handles resolving names with DNS providing a thread-safe, re-entrant solution.
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2008-2012 by Sorenson Communications, Inc. All rights reserved.
 *
 */

#ifndef CSTIHOSTNAMERESOLVER_H
#define CSTIHOSTNAMERESOLVER_H
#include "stiError.h"
#include "stiSVX.h"
#include <string>
#include <condition_variable>
#include <map>
#include <list>
#include <vector>
#include <ctime>

//
// Forward declarations
//

struct SstiSRVRecord
{
	std::string HostName;
	uint16_t un16Port{0};
	uint16_t un16Protocol{0};
	uint16_t un16Priority{0};
	uint16_t un16Weight{0};
};


class CstiHostNameResolver
{
public:
	static const int nRESOLVER_TIMEOUT = 10000;  // The time (in milliseconds) that we will wait for a DNS resolution to come back.  After which, we will use previouly resolved values.

	static CstiHostNameResolver * getInstance ();
	stiHResult AddrInfoByFamilyFind (addrinfo *pstAddrInfo, addrinfo **ppstAddrInfoFound, int nIPType);
	stiHResult Resolve (const char *pszHostName, const char *pszAltHostName, addrinfo **ppstAddrInfo);
	stiHResult ResolvedSRVStore (const char *pszSRVName, const std::vector<SstiSRVRecord> &SRVRecords);
	stiHResult ResolvedSRVGet (const char *pszSRVName, const char *pszAltSRVName, std::vector<SstiSRVRecord> *pSRVRecords, bool bAllowExpired);
	stiHResult ResolvedAddressStore (const char *pszHostName, const std::vector<std::string> &IPRecords);
	stiHResult ResolvedGet (const char *pszHostName, const char *pszAltHostName, addrinfo **ppstAddrInfo, bool bAllowExpired);
	stiHResult ResolvedGet (const char *pszHostName, std::vector<std::string> *pIPRecords, bool bAllowExpired);
	stiHResult IpListFromAddrinfoCreate (const addrinfo *pstAddrInfo, std::vector<std::string> *pResults);
	
	stiHResult IPv6Enable (bool bIPv6Enabled);

protected:
	CstiHostNameResolver ();
	virtual ~CstiHostNameResolver ();
	
private:
	
	struct SstiHost
	{
		SstiHost () 
		{
			ExpiresTime = time (nullptr);
		};

		~SstiHost ()
		{
			// Clean up the old addrinfo structure .
			if (pstAddrInfo)
			{
				if (bAllocatedByGetAddrInfo)
				{
					freeaddrinfo (pstAddrInfo);
					pstAddrInfo = nullptr;
				}
				else
				{
					while (pstAddrInfo)
					{
						addrinfo *pNextAddrInfo = pstAddrInfo->ai_next;

						if (pstAddrInfo->ai_addr)
						{
							delete pstAddrInfo->ai_addr;
							pstAddrInfo->ai_addr = nullptr;
						}

						delete pstAddrInfo;
						pstAddrInfo = pNextAddrInfo;
					}
				}
			}
		}

		enum EType
		{
			eHostName,
			eSRV
		};

		EType eType{eHostName};
		addrinfo *pstAddrInfo{nullptr};
		bool bAllocatedByGetAddrInfo{false};	// Used to know how to free the memory; by the freeaddrinfo or delete method.
		time_t ExpiresTime;	// Expiration time

		//
		// Used for SRV results
		//
		std::vector<SstiSRVRecord> SRVRecords;
	};
	
	struct SstiActiveLookups;
	struct SstiResolverInfo
	{
		std::string HostName;
		SstiHost *pstHost{nullptr};  // Only used when passed back to the callback from the task thread
		SstiActiveLookups *pstActiveLookups{nullptr};  // Access to the parent structure
		stiWDOG_ID wdLookupTimer{nullptr};	// A lookup timer
		std::string ResolverError; // Error message
		
		SstiResolverInfo () = default;
	};
	
	struct SstiActiveLookups
	{
		SstiResolverInfo *pstPrimary{nullptr};
		SstiResolverInfo *pstAlternate{nullptr};
		SstiResolverInfo *pstFromMap{nullptr};
		CstiHostNameResolver *pHostNameResolver{nullptr};
		
		SstiActiveLookups () = default;
	};
	
	enum
	{
		estiMSG_NAME_RESOLVED = estiMSG_NEXT,
		estiMSG_NAME_RESOLVE_FAILED,
		estiMSG_NAME_RESOLVE_TIMER_FIRED,
		estiMSG_PRIMARY_TIMER_FIRED,
	};
		
	
	bool m_bInitialized{false};
	bool m_bNeedToWrite{false};
	std::condition_variable_any m_ResolvedCond;
	stiWDOG_ID m_wdPersistentStoreTimer;
	std::map<std::string, SstiHost*> m_Host;
	std::list<SstiResolverInfo*> m_ResolverInfoReturned;
	bool m_bIPv6Enabled{false};
	
	static void * ThreadTask (
		void * param);
	
	static stiHResult ThreadCallback(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam);	///< holds data specific to the message
	
	static void ResolverErrorLog (
		SstiResolverInfo *pstResolverInfo,
		const char *pszHostName,
		const char *pszError);

	stiHResult StoredValuesLoad ();

	stiHResult ResolvedValuesStore (); // If different from values previously stored, store the resolved addresses along with the corresponding host names; replacing old values.

	static void * PrimaryTimerThreadCallbackTask (
		void * param);
	
	static void * LookupTimerThreadCallbackTask (
		void * param);
	
	static int LookupTimerExpiredFunc (
		size_t param);

	static int PrimaryTimerExpiredFunc (
		size_t param);
	
	static int PersistentStoreWriteTimerFunc (
		size_t param);

	stiHResult MappedValueRemove (
		const std::string &HostName);
	
	void Initialize ();
	
	void AddrInfoByIPTypeClear (
		SstiHost *pstHost,
		int family);

	stiHResult AddrInfoCreateAndLoad (SstiHost *pstHost, const char *pszIPAddress, uint16_t un16Port, time_t expires, int eSockType);
	stiHResult SRVCreateAndLoad (SstiHost *pstHost, const char *pszHostName, uint16_t un16Port, time_t expires);
	
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	static int m_nBufferMultiplier;
	static char m_pszResolvedIP[1024];
	static int ResolveHostByName (SstiResolverInfo *pstResolverInfo);
#endif
};

#endif // #ifndef CSTIHOSTNAMERESOLVER_H

