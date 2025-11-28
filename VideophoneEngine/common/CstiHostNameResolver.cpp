////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential - Do not distribute
// Copyright 2000-2012 by Sorenson Communications, Inc. All rights reserved.
//
//  Class Name: CstiHostNameResolver
//
//  File Name:  CstiHostNameResolver.cpp
//
//  Abstract:
//	  This class implements the methods specific to resolving a name into a
//	  addrinfo structure and to obtain information about the resolved addresses.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiHostNameResolver.h"
#include "stiTrace.h"
#include "stiOS.h"
#include "stiTaskInfo.h"
#include <cstdio>
#include "stiTools.h"
#include "CstiIpAddress.h"
#include "stiRemoteLogEvent.h"
#include <mutex>

//
// Constants
//
const int nRESOLVED_LOOKUP_TTL = 180;  // Time (in seconds) that the lookup is valid to be used before a new lookup is attempted.
const int nPERSISTENT_STORE_DELTA = 10 * 60000;  // The time (in milliseconds) between storing the resolved data to persistent storage (if needed)
const char szDNS_PERSISTENT_FILE[] = "dns_lookups.dat";
const char szSRV_PREFIX[] = "SRV:"; // Precedes each SRV record in the data file

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
extern int androidSDKVersion;
#endif

//
// Locals
//
std::recursive_mutex lockMutex{};

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
const int nBUFF_INCREMENT = 512;

int CstiHostNameResolver::m_nBufferMultiplier = 1;
char CstiHostNameResolver::m_pszResolvedIP[1024];
#endif


/*! \brief Check if pszName is valid.
*
* The name can contain a-z or A-Z or 0-9 or '.' or '_' or '-', but '-' cannot be first.
*
* \retval estiTRUE If pszName is a DNS name. (This may not be a true DNS name because of the '_' character but we are allowing the '_' be stored so we need to allow it here too.
* \retval estiFALSE Otherwise.
*/
EstiBool NameValidate (
	const char *pszName)
{
	EstiBool bResult = estiTRUE;

	if (pszName != nullptr)
	{
		int i;

		bool startOfLabel = true;

		for (i=0; pszName[i]; i++)
		{
			// the first character of each segment must start with a letter or the underscore '_'.
			if (startOfLabel)
			{
				if (!isalpha(pszName[i]) //RFC1035 - Must start with a letter.

						// It can also begin with an underscore (_)
					 && pszName[i] != '_')
				{ 
					bResult = estiFALSE;
					break;
				}
				startOfLabel = false;
			}

			// If it isn't the start of the label, it can be any alpha-numeric, underscore or dash.
			else if (isalnum(pszName[i])
					|| pszName[i] == '_'
					|| pszName[i] == '-'
					)
			{
			}

			// The dot '.' character separates segments and lets us know we need to validate the next character again differently from the rest.
			else if (pszName[i] == '.')
			{
				startOfLabel = true;
			}

			// Any other character is invalid.
			else
			{
				bResult = estiFALSE;
				break;
			}
		}

		// If we ended up with unhandled characters or we processed less than three letters, it must be invalid.
		if (pszName[i] || i < 3)
		{
			bResult = estiFALSE;
		}
	}
	
	else
	{
		bResult = estiFALSE;
	}

	return bResult;
}


//
// Class Definitions
//
/*! \brief Constructor
 * 
 * \return none.
 */
CstiHostNameResolver::CstiHostNameResolver ()
{
	m_wdPersistentStoreTimer = stiOSWdCreate ();
}


/*! \brief Destructor
 * 
 * This method cleans up memory previously allocated.
 *
 * \return none.
 */
CstiHostNameResolver::~CstiHostNameResolver ()
{
	m_ResolvedCond.notify_all ();

	// If we have a timer for persistent storage, cancel and delete it.
	if (m_wdPersistentStoreTimer)
	{
		stiOSWdDelete (m_wdPersistentStoreTimer);
	}
	
	// If we need to write data to persistent store, do so now.
	if (m_bNeedToWrite)
	{
#ifndef stiDISABLE_DNS_CACHE_STORE
		ResolvedValuesStore ();
#endif
	}
	
	// Clean up the allocations and the std::map
	for (auto &entry: m_Host)
	{
		if (entry.second)
		{
			delete entry.second;
			entry.second = nullptr;
		}
	}
	
	m_Host.clear ();
}


/*! \brief Return a pointer to the singleton class object.
* 
* This class is intended to operate as a singleton object.  
*
* \return A pointer to the class object.
*/
CstiHostNameResolver* CstiHostNameResolver::getInstance()
{
	static CstiHostNameResolver localHostNameResolver;
	
	return &localHostNameResolver;
}


/*! \brief Initializes the resolver system.
* 
* This will be called when needed.  
*
* \return nothing.
*/
void CstiHostNameResolver::Initialize ()
{
	if (!m_bInitialized)
	{
#ifndef stiDISABLE_DNS_CACHE_STORE
		StoredValuesLoad ();
#endif
		
		stiHResult hResult = stiOSWdStart (m_wdPersistentStoreTimer,
					nPERSISTENT_STORE_DELTA,
					(stiFUNC_PTR) PersistentStoreWriteTimerFunc, (size_t)this);

		// Start a timer to update persistent storage
		if (stiIS_ERROR(hResult))
		{
			stiASSERT (false);
		}
		
		m_bInitialized = true;
	}
}


stiHResult CstiHostNameResolver::IPv6Enable (bool bIPv6Enabled)
{
	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	m_bIPv6Enabled = bIPv6Enabled;

	return stiRESULT_SUCCESS;
}


stiHResult CstiHostNameResolver::AddrInfoCreateAndLoad (
	SstiHost *pstHost,
	const char *pszIPAddress,
	uint16_t un16Port,
	time_t expires,
	int eSockType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiIpAddress IpAddress;
	addrinfo *pstAddrInfo = nullptr;
	
	stiTESTCOND (pstHost, stiRESULT_ERROR);
	
	IpAddress = pszIPAddress;
	
	pstHost->ExpiresTime = expires;
	
	pstHost->eType = SstiHost::eHostName;

	pstAddrInfo = new (addrinfo);
	memset (pstAddrInfo, 0, sizeof (addrinfo));
	
	pstAddrInfo->ai_socktype = eSockType;

	if (IpAddress.IpAddressTypeGet () == estiTYPE_IPV6)
	{
		auto pMappedSockAddr_in = new (sockaddr_in6);
		memset (pMappedSockAddr_in, 0, sizeof (struct sockaddr_in6));
		
		// Fill in the host information as if we received the response from getaddrinfo.
		pMappedSockAddr_in->sin6_family = AF_INET6;
		pMappedSockAddr_in->sin6_port = un16Port;
		inet_pton (AF_INET6, pszIPAddress, &(pMappedSockAddr_in->sin6_addr));
		pstAddrInfo->ai_addr = (sockaddr*)pMappedSockAddr_in;
		pstAddrInfo->ai_addrlen = sizeof (sockaddr_in6);
		pstAddrInfo->ai_family = AF_INET6;
	}
	else
	{
		auto pMappedSockAddr_in = new (sockaddr_in);
		memset (pMappedSockAddr_in, 0, sizeof (struct sockaddr_in));
		
		// Fill in the host information as if we received the response from getaddrinfo.
		((sockaddr*)pMappedSockAddr_in)->sa_family = AF_INET;
		pMappedSockAddr_in->sin_port = un16Port;
		inet_pton (AF_INET, pszIPAddress, &pMappedSockAddr_in->sin_addr);
		pstAddrInfo->ai_addr = (sockaddr*)pMappedSockAddr_in;
		pstAddrInfo->ai_addrlen = sizeof (sockaddr_in);
		pstAddrInfo->ai_family = AF_INET;
	}
	
	//
	// Add the new addrinfo to the end of the current list.
	//
	if (pstHost->pstAddrInfo)
	{
		//
		// Find the last node and add the new addrinfo structure.
		//
		for(addrinfo *pAddrInfo = pstHost->pstAddrInfo; pAddrInfo != nullptr; pAddrInfo = pAddrInfo->ai_next)
		{
			if (pAddrInfo->ai_next == nullptr)
			{
				pAddrInfo->ai_next = pstAddrInfo;
				break;
			}
		}
	}
	else
	{
		pstHost->pstAddrInfo = pstAddrInfo;
	}

STI_BAIL:
	
	return hResult;
}


/*! \brief Locates an address of the requested ai_family in  an addrinfo structure.
 *
 * This method searches all addresses in the addrinfo structure looking for one matching the ai_family specified.
 *
 * \param pstAddrInfo - a pointer to an addrinfo structure to be searched  (Required)
 * \param pstAddrInfoFound - a pointer of the found result. (Required)
 * \param nFamily - ai_family type we are looking for. (Required)
 * \return stiRESULT_SUCCESS or some other error result.
 */
stiHResult CstiHostNameResolver::AddrInfoByFamilyFind (addrinfo *pstAddrInfo, addrinfo **ppstAddrInfoFound, int nFamily)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = false;
	
	stiTESTCOND (pstAddrInfo, stiRESULT_INVALID_PARAMETER);
	stiTESTCOND (ppstAddrInfoFound, stiRESULT_INVALID_PARAMETER);
	
	while (pstAddrInfo && !bFound)
	{
		if (pstAddrInfo->ai_family == nFamily
			 || nFamily == AF_UNSPEC)
		{
			*ppstAddrInfoFound = pstAddrInfo;
			bFound = true;
		}
		else
		{
			pstAddrInfo = pstAddrInfo->ai_next;
		}
	}
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Look up and resolve a DNS name to its IP address.
* 
* This method looks in a std::map for the host names passed in.  If found and if they're still good, it returns a pointer to the 
* addrinfo structure found in the map.  If they are either not found or if they're old, a thread will be created for each name
* passed in.
* 
* The code then blocks until one of the resolutions come back, or if set, a timer expires.
*
* \param pszHostName - the name of the primary host to resolve.  (Required)
* \param pszAltHostName - the name of the alternate host to resolve. (Optional)
* \param ppstAddrInfo - the address of the addrinfo structure pointer ... we assign this pointer to the addrinfo structure containing the resolved address.
* \return stiRESULT_SUCCESS or some other error result.
*/
stiHResult CstiHostNameResolver::Resolve (const char *pszHostName, const char *pszAltHostName, addrinfo **ppstAddrInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	time_t CurrentTime = time (nullptr);
	*ppstAddrInfo = nullptr;
	
	// Make this method run inside of a mutex to keep from having more than one call from performing lookups at the same time
	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	
	// If this is the first time called, load the persistent data
	Initialize ();
	
	stiTESTCOND (pszHostName && *pszHostName, stiRESULT_ERROR);
	
	// Determine if what was passed in is an IP address already
	if (IPAddressValidate (pszHostName))
	{
		// Look to see if the host is already in our map
		auto it = m_Host.find (pszHostName);
		
		if (it != m_Host.end ())
		{
			// We already have it in our map.  Use it.
			*ppstAddrInfo = it->second->pstAddrInfo;
		}
		else
		{
			// Load it in the map ... it won't be stored off, however.
			auto pstHost = new (SstiHost);
			AddrInfoCreateAndLoad (pstHost, pszHostName, 0, 0, SOCK_STREAM);
			
			m_Host[pszHostName] = pstHost;
			
			*ppstAddrInfo = pstHost->pstAddrInfo;
		}
		
		stiDEBUG_TOOL (g_stiDNSDebug,
							stiTrace ("<CstiHostNameResolver::Resolve> Passed in name (%s) is an IP; returning it.\n", pszHostName);
		);
	}  // end if
	
	else if (pszAltHostName && IPAddressValidate (pszAltHostName))
	{
		// Look to see if the host is already in our map
		auto it = m_Host.find (pszAltHostName);
		
		if (it != m_Host.end ())
		{
			// We already have it in our map.  Use it.
			*ppstAddrInfo = it->second->pstAddrInfo;
		}
		else
		{
			// Load it in the map ... it won't be stored off, however.
			auto pstHost = new (SstiHost);
			AddrInfoCreateAndLoad (pstHost, pszAltHostName, 0, 0, SOCK_STREAM);
			
			m_Host[pszAltHostName] = pstHost;
			
			*ppstAddrInfo = pstHost->pstAddrInfo;
		}
		
		stiDEBUG_TOOL (g_stiDNSDebug,
							stiTrace ("<CstiHostNameResolver::Resolve> Passed in name (%s) is an IP; returning it.\n", pszAltHostName);
		);
	}
	
	else
	{
		// Look to see if the host is already in our map
		auto it = m_Host.find (pszHostName);
		auto itAlt = m_Host.end ();
		int nFamily = AF_INET;
		SstiHost *pstHost = nullptr;
		
		if (m_bIPv6Enabled)
		{
			nFamily = AF_INET6;
		}
		
		if (pszAltHostName && 0 < strlen (pszAltHostName))
		{
			itAlt = m_Host.find (pszAltHostName);
		}
		
		// Is the first host name in our map and has it NOT expired
		if (it != m_Host.end () && it->second->ExpiresTime > CurrentTime)
		{
			// Found the host name in our map, and it hasn't expired.
			pstHost = it->second;
			
			hResult = AddrInfoByFamilyFind (pstHost->pstAddrInfo, ppstAddrInfo, nFamily);
			
			if (stiIS_ERROR (hResult))
			{
				stiASSERTMSG (false, "CstiHostNameResolver Failed to find primary address by family.");
				
				*ppstAddrInfo = nullptr;
				hResult = stiRESULT_SUCCESS;
			}
			
			stiDEBUG_TOOL (g_stiDNSDebug,
				stiTrace ("<CstiHostNameResolver::Resolve> Found \"%s\" in map.  Using it\n", pszHostName);
			);
		}
		
		// If we didn't find the primary check for the second host name in our map and that it has it NOT expired
		if (itAlt != m_Host.end ()
					&& itAlt->second->ExpiresTime > CurrentTime
					&& !*ppstAddrInfo)
		{
			// Found the alternate host name in our map, and it hasn't expired.
			pstHost = itAlt->second;
			
			hResult = AddrInfoByFamilyFind (pstHost->pstAddrInfo, ppstAddrInfo, nFamily);
			
			if (stiIS_ERROR (hResult))
			{
				stiASSERTMSG (false, "CstiHostNameResolver Failed to find alternate address by family.");
				
				*ppstAddrInfo = nullptr;
				hResult = stiRESULT_SUCCESS;
			}
			
			stiDEBUG_TOOL (g_stiDNSDebug,
				stiTrace ("<CstiHostNameResolver::Resolve> Found \"%s\" in map.  Using it\n", pszAltHostName);
			);
		}
		
		// Didn't find an un-expired result in our std::map.  Do a lookup with DNS.
		if (!*ppstAddrInfo)
		{
			auto pstActiveLookups = new (SstiActiveLookups);
			pstActiveLookups->pstPrimary = new (SstiResolverInfo);
			pstActiveLookups->pstPrimary->HostName = pszHostName;
			pstActiveLookups->pstPrimary->pstActiveLookups = pstActiveLookups;
			pstActiveLookups->pHostNameResolver = this;
			
			
			stiOSTaskSpawn ("DNSLookup",
				stiDNS_TASK_PRIORITY,
				stiDNS_STACK_SIZE,
				ThreadTask,
				pstActiveLookups->pstPrimary);
			
			// If we have an alternate, look it up too
			if (pszAltHostName && 0 < strlen (pszAltHostName))
			{
				pstActiveLookups->pstAlternate = new (SstiResolverInfo);
				pstActiveLookups->pstAlternate->HostName = pszAltHostName;
				pstActiveLookups->pstAlternate->pstActiveLookups = pstActiveLookups;
				
				// Start a timer.  If it expires before we have received the response for the primary host, we will spawn the task for the alternate.
				pstActiveLookups->pstAlternate->wdLookupTimer = stiOSWdCreate ();
				
				if (pstActiveLookups->pstAlternate->wdLookupTimer)
				{

					stiHResult hResult = stiOSWdStart (pstActiveLookups->pstAlternate->wdLookupTimer,
											nRESOLVER_TIMEOUT / 3, // 1/3 of the overall time allowed for the full lookup
											(stiFUNC_PTR) PrimaryTimerExpiredFunc, (size_t)pstActiveLookups->pstAlternate);

					if (stiIS_ERROR (hResult))
					{
						// Since it failed to start the timer, we need to free the memory we created for this object.
						delete pstActiveLookups->pstAlternate;
						pstActiveLookups->pstAlternate = nullptr;
						
						stiASSERT (false);
					}
				}
			}
			
			// Load the map value if it was found
			if (it != m_Host.end ())
			{
				pstActiveLookups->pstFromMap = new (SstiResolverInfo);
				pstActiveLookups->pstFromMap->HostName = pszHostName;
				pstActiveLookups->pstFromMap->pstHost = (SstiHost*)it->second;
				pstActiveLookups->pstFromMap->pstActiveLookups = pstActiveLookups;
				
				// Start a timer.  If it expires before one or the other of the lookups return, we will use an old result (if present)
				pstActiveLookups->pstFromMap->wdLookupTimer = stiOSWdCreate ();
				
				if (pstActiveLookups->pstFromMap->wdLookupTimer)
				{
					stiHResult hResult = stiOSWdStart (pstActiveLookups->pstFromMap->wdLookupTimer,
											nRESOLVER_TIMEOUT,
											(stiFUNC_PTR) LookupTimerExpiredFunc, (size_t)pstActiveLookups->pstFromMap);

					if (stiIS_ERROR (hResult))
					{
						// Since it failed to start the timer, we need to free the memory we created for this object.
						delete pstActiveLookups->pstFromMap;
						pstActiveLookups->pstFromMap = nullptr;
						
						stiASSERT (false);
					}
				}
			}
			
			// If we didn't have a primary value in the map, check to see if we have an alternate value in the map and load it.
			else if (itAlt != m_Host.end ())
			{
				pstActiveLookups->pstFromMap = new (SstiResolverInfo);
				pstActiveLookups->pstFromMap->HostName = pszAltHostName;
				pstActiveLookups->pstFromMap->pstHost = (SstiHost*)itAlt->second;
				pstActiveLookups->pstFromMap->pstActiveLookups = pstActiveLookups;
				
				// Start a timer.  If it expires before one or the other of the lookups return, we will use an old result (if present)
				pstActiveLookups->pstFromMap->wdLookupTimer = stiOSWdCreate ();
				
				if (pstActiveLookups->pstFromMap->wdLookupTimer)
				{
					stiHResult hResult = stiOSWdStart (pstActiveLookups->pstFromMap->wdLookupTimer,
											nRESOLVER_TIMEOUT,
											(stiFUNC_PTR) LookupTimerExpiredFunc, (size_t)pstActiveLookups->pstFromMap);

					if (stiIS_ERROR (hResult))
					{
						// Since it failed to start the timer, we need to free the memory we created for this object.
						delete pstActiveLookups->pstFromMap;
						pstActiveLookups->pstFromMap = nullptr;
						
						stiASSERT (false);
					}
				}
			}

			stiDEBUG_TOOL (g_stiDNSDebug, 
				stiTrace ("<CstiHostNameResolver::Resolve> HostName = \"%s\", AltHostName = \"%s\", %sound in map.  Waiting on resolved address%s\n", 
							pszHostName, pszAltHostName, pstActiveLookups->pstFromMap ? "F" : "NOT f",
							(pstActiveLookups->pstFromMap && pstActiveLookups->pstFromMap->wdLookupTimer) ? " or timer" : "");
			);
			
			// Now we need to wait for one of the above to complete (or if set, for the timer to expire).
			bool bFound = false;
			std::list<SstiResolverInfo*>::iterator it;
			
			do 
			{
				m_ResolvedCond.wait (lock);
				
				// Loop through the list of Returned ResolverInfo structures and handle each one (giving the resolved address back to the 
				// calling thread.
				for (it = m_ResolverInfoReturned.begin (); !bFound && it != m_ResolverInfoReturned.end (); )
				{
					if ((pstActiveLookups->pstPrimary &&  pstActiveLookups->pstPrimary == *it) ||
						(pstActiveLookups->pstAlternate && pstActiveLookups->pstAlternate == *it) ||
						(pstActiveLookups->pstFromMap && pstActiveLookups->pstFromMap == *it))
					{
						// The resolved object has been found.
						
						// Set the return value to the addrinfo structure to use
						if ((*it)->pstHost)
						{
							*ppstAddrInfo = (*it)->pstHost->pstAddrInfo;
						}
						else
						{
							// One known reason we get here is that we failed to resolve the host name and we didn't have an alternate nor one in the map.
							*ppstAddrInfo = nullptr;
						}
						
						// Break the links between all the ActiveLookups structures and the ResolverInfo structures
						if (pstActiveLookups->pstPrimary)
						{
							pstActiveLookups->pstPrimary = nullptr;
						}
						
						if (pstActiveLookups->pstAlternate)
						{
							pstActiveLookups->pstAlternate = nullptr;
						}
						
						if (pstActiveLookups->pstFromMap)
						{
							pstActiveLookups->pstFromMap = nullptr;
						}
						
						// Remove from the list, delete the returned ResolverInfo structure and the ActiveLookups structure.
						delete *it;
						it = m_ResolverInfoReturned.erase (it);
						
						delete pstActiveLookups;
						pstActiveLookups = nullptr;
						
						// Since we found the match, this thread is done, drop out.
						bFound = true;
					}
					else
					{
						it++;
					}
				}
			} while (!bFound);
		}
	}
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Look up previously resolved DNS name(s).
* 
* This method looks in a std::map for the host names passed in.  If found, it returns a pointer to the 
* addrinfo structure found in the map.
* 
* \param pszHostName - the name of the primary host to resolve.  (Required)
* \param pszAltHostName - the name of the alternate host to resolve. (Optional)
* \param ppstAddrInfo - the address of the addrinfo structure pointer ... we assign this pointer to the addrinfo structure containing the resolved address.
* \param bAllowExpired - whether or not to include expired resolutions in the return value
* \return stiRESULT_SUCCESS or some other error result.
*/
stiHResult CstiHostNameResolver::ResolvedGet (const char *pszHostName, const char *pszAltHostName, addrinfo **ppstAddrInfo, bool bAllowExpired)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	time_t CurrentTime = time (nullptr);
	std::map <std::string, SstiHost*>::iterator it;
	
	// Make this method run inside of a mutex to keep from having more than one call from performing lookups at the same time
	std::unique_lock<std::recursive_mutex> lock (lockMutex);

	stiTESTCOND (pszHostName, stiRESULT_ERROR);
	
	// If this is the first time called, load the persistent data
	Initialize ();
	
	// Is the first host name in our map?
	it = m_Host.find (pszHostName);
	if (it != m_Host.end () &&
		(bAllowExpired || it->second->ExpiresTime > CurrentTime)
	)
	{
		// Found the host name in our map.
		*ppstAddrInfo = it->second->pstAddrInfo;
		
		stiDEBUG_TOOL (g_stiDNSDebug,
							stiTrace ("<CstiHostNameResolver::ResolvedGet> Found \"%s\" in map.  Using it\n", pszHostName);
		);
	}
	else
	{
		//
		// Return an error if were not given a alternate hostname to resolve.
		//
		stiTESTCOND_NOLOG (pszAltHostName, stiRESULT_VALUE_NOT_FOUND);

		// Is the second host name in our alternate map?
		it = m_Host.find (pszAltHostName);
		if (it != m_Host.end () &&
			(bAllowExpired || it->second->ExpiresTime > CurrentTime)
		)
		{
			// Found the alternate host name in our alternate map
			*ppstAddrInfo = it->second->pstAddrInfo;

			stiDEBUG_TOOL (g_stiDNSDebug,
								stiTrace ("<CstiHostNameResolver::ResolvedGet> Found \"%s\" in map.  Using it\n", pszAltHostName);
			);
		}
		else
		{
			// Didn't find a suitable result
			stiTESTCOND_NOLOG (false, stiRESULT_VALUE_NOT_FOUND);
		}
	}
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Look up previously resolved DNS name(s).
*
* This method looks in a std::map for the host name passed in.  If found, it adds the IP
* to the vector passed in.
*
* \param pszHostName The name of the primary host to resolve.  (Required)
* \param pIPRecords The vector that will contain the IP addresses if the hostname is found.
* \param bAllowExpired Whether or not to include expired resolutions in the return value
* \return stiRESULT_SUCCESS or some other error result.
*/
stiHResult CstiHostNameResolver::ResolvedGet (
	const char *pszHostName,
	std::vector<std::string> *pIPRecords,
	bool bAllowExpired)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	time_t CurrentTime = time (nullptr);
	std::map <std::string, SstiHost*>::iterator it;

	// Make this method run inside of a mutex to keep from having more than one call from performing lookups at the same time
	std::unique_lock<std::recursive_mutex> lock (lockMutex);

	stiTESTCOND (pszHostName, stiRESULT_ERROR);

	// If this is the first time called, load the persistent data
	Initialize ();

	// Is the host name in our map?
	it = m_Host.find (pszHostName);

	if (it != m_Host.end () && (bAllowExpired || it->second->ExpiresTime > CurrentTime))
	{
		//
		// Found the hostname.  Now iterate through the addrinfo structures and store the IPs into the vector
		// that the caller passed in.
		//
		for(addrinfo *pAddrInfo = it->second->pstAddrInfo; pAddrInfo != nullptr; pAddrInfo = pAddrInfo->ai_next)
		{
			const char *pszAlpha = nullptr;
			char szTmpBuf[INET6_ADDRSTRLEN];

			//
			// Determine if the addrinfo is an IPv4 or an IPv6 address and convert the bytes
			// to a string for storage into the vector.
			//
			auto pstSockaddrIn = (struct sockaddr_in *)pAddrInfo->ai_addr;
			if (pstSockaddrIn->sin_family == AF_INET6)
			{
				auto pSockAddrIn6 = (struct sockaddr_in6 *)pAddrInfo->ai_addr;
				pszAlpha = inet_ntop (AF_INET6, &pSockAddrIn6->sin6_addr, szTmpBuf, sizeof (szTmpBuf));
			}
			else
			{
				pszAlpha = inet_ntop (AF_INET, &pstSockaddrIn->sin_addr, szTmpBuf, sizeof (szTmpBuf));
			}

			//
			// If we successfully retreived an IP address then store it in the vector.
			//
			if (pszAlpha != nullptr)
			{
				pIPRecords->push_back (pszAlpha);
			}
		}
	}
	else
	{
		//
		// Return an error if were not given a alternate hostname to resolve.
		//
		stiTHROW_NOLOG (stiRESULT_VALUE_NOT_FOUND);
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiHostNameResolver::ResolvedSRVGet (
	const char *pszSRVName,
	const char *pszAltSRVName,
	std::vector<SstiSRVRecord> *pSRVRecords,
	bool bAllowExpired)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	time_t CurrentTime = time (nullptr);
	std::map <std::string, SstiHost*>::iterator it;
	std::string MapKey;

	// Make this method run inside of a mutex to keep from having more than one call from performing lookups at the same time
	std::unique_lock<std::recursive_mutex> lock (lockMutex);

	stiTESTCOND (pszSRVName, stiRESULT_ERROR);

	// If this is the first time called, load the persistent data
	Initialize ();

	MapKey = szSRV_PREFIX;
	MapKey += pszSRVName;

	// Is the first host name in our map?
	it = m_Host.find (MapKey);
	if (it != m_Host.end () &&
		(bAllowExpired || it->second->ExpiresTime > CurrentTime)
	)
	{
		// Found the host name in our map.
		*pSRVRecords = it->second->SRVRecords;

		stiDEBUG_TOOL (g_stiDNSDebug,
							stiTrace ("<CstiHostNameResolver::ResolvedSRVGet> Found \"%s\" in map.  Using it\n", pszSRVName);
		);
	}
	else
	{
		//
		// Return an error if were not given a alternate hostname to resolve.
		//
		stiTESTCOND_NOLOG (pszAltSRVName, stiRESULT_VALUE_NOT_FOUND);

		MapKey = szSRV_PREFIX;
		MapKey += pszAltSRVName;

		// Is the second host name in our alternate map?
		it = m_Host.find (MapKey);
		if (it != m_Host.end () &&
			(bAllowExpired || it->second->ExpiresTime > CurrentTime)
		)
		{
			// Found the alternate host name in our alternate map
			*pSRVRecords = it->second->SRVRecords;

			stiDEBUG_TOOL (g_stiDNSDebug,
								stiTrace ("<CstiHostNameResolver::ResolvedSRVGet> Found \"%s\" in map.  Using it\n", pszAltSRVName);
			);
		}
		else
		{
			// Didn't find a suitable result
			stiTESTCOND_NOLOG (false, stiRESULT_VALUE_NOT_FOUND);
		}
	}

STI_BAIL:

	return hResult;
}


#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
/*! \brief Gingerbread requires stiOSGetHostByName since getaddrinfo is broken
 *
 * \return 0 always.
 */
int CstiHostNameResolver::ResolveHostByName(CstiHostNameResolver::SstiResolverInfo *pstResolverInfo)
{
	int nBufSize;
	stiHResult hResult = stiRESULT_SUCCESS;
	hostent m_stHostTable;
	hostent *m_pstHostTable = NULL;
	char *pszHostBuffer = NULL;
	do
	{
		// If we already have an allocation, free it before we re-allocate.
		if (pszHostBuffer)
		{
			delete [] pszHostBuffer;
			pszHostBuffer = NULL;
		}
		nBufSize = m_nBufferMultiplier++ * nBUFF_INCREMENT;
		pszHostBuffer = new char[nBufSize];
		
		// See if the DNS can resolve the name.
		if (pszHostBuffer)
		{
			hResult = stiOSGetHostByName (pstResolverInfo->HostName.c_str(), &m_stHostTable, pszHostBuffer, nBufSize, &m_pstHostTable);
		}

		if (pszHostBuffer && hResult == stiRESULT_SUCCESS) 
		{
			char** pAddr = m_pstHostTable->h_addr_list;
			char buffer[1024];
			
			pstResolverInfo->pstHost->ExpiresTime = 0;
			pstResolverInfo->pstHost->bAllocatedByGetAddrInfo = false;
			pstResolverInfo->pstHost->pstAddrInfo = NULL;

			addrinfo *pCurrent = NULL;

			// Lock since we use pstActiveLookups below.
			std::lock_guard<std::recursive_mutex> lock_safe ( lockMutex );
			// Double check pstActiveLookups exists.
			if (!pstResolverInfo->pstActiveLookups)
			{
				hResult = stiRESULT_ERROR;
			}

			while(*pAddr && hResult == stiRESULT_SUCCESS)
			{
				snprintf(buffer, 1024, "%u.%u.%u.%u",
							static_cast<unsigned int>(static_cast<unsigned char>((*pAddr)[0])),
							static_cast<unsigned int>(static_cast<unsigned char>((*pAddr)[1])),
							static_cast<unsigned int>(static_cast<unsigned char>((*pAddr)[2])),
							static_cast<unsigned int>(static_cast<unsigned char>((*pAddr)[3])));
				strcpy(m_pszResolvedIP, buffer);
				++pAddr;

				// Fill in the host information as if we received the response from getaddrinfo.
				if (pCurrent != NULL)
				{
					// Chain to previous node.
					pCurrent->ai_next = new (addrinfo);
					pCurrent = pCurrent->ai_next;

					memset (pCurrent, 0, sizeof (addrinfo));

					sockaddr_in *pMappedSockAddr_in = new (sockaddr_in);
					memset (pMappedSockAddr_in, 0, sizeof (struct sockaddr_in));
#ifdef IPV6_ENABLED
					// when working with ipv6, don't forget this extra step
					pMappedSockAddr_in->sin6_len = sizeof (struct sockaddr_in); 
#endif // IPV6_ENABLED 
					pCurrent->ai_socktype = SOCK_STREAM;
					((sockaddr*)pMappedSockAddr_in)->sa_family = AF_INET;
					pMappedSockAddr_in->sin_port = 0;
					pMappedSockAddr_in->sin_addr.s_addr = inet_addr (m_pszResolvedIP);
					pCurrent->ai_addr = (sockaddr*)pMappedSockAddr_in;
					pCurrent->ai_family = AF_INET;
				}
				else
				{
					// Setup the first node.
					CstiHostNameResolver *pThis = pstResolverInfo->pstActiveLookups->pHostNameResolver;
					pThis->AddrInfoCreateAndLoad (pstResolverInfo->pstHost, m_pszResolvedIP, 0, 0, SOCK_STREAM);
				}
			}
		}
	} while (stiRESULT_BUFFER_TOO_SMALL == stiRESULT_CODE (hResult) && m_nBufferMultiplier < 5);
	
	// Decrement multiplier to what we actually used.
	m_nBufferMultiplier--;
	return hResult;
}
#endif

/*! \brief This code runs as a separate task to resolve the address passed in.
* 
* Once the resolution is complete, it calls the callback to alert the calling program.
*
* \return 0 always.
*/
void * CstiHostNameResolver::ThreadTask (
	void * param)
{
	auto pstResolverInfo = static_cast<CstiHostNameResolver::SstiResolverInfo *>(param);
	
	stiDEBUG_TOOL (g_stiDNSDebug,
		stiTrace ("<CstiHostNameResolver::ThreadTask> Resolving:%s\n", pstResolverInfo->HostName.c_str ());
	);  // end stiDEBUG_TOOL
	
	int nRet = -1;
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bGotByName = false;
	
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	// Both ICS and Gingerbread appear to have problems with getaddrinfo.  ICS problems
	// only manifest when a network is not available.
	bool bResolveHostByName = true;
	int nResolveResult = stiRESULT_ERROR;
#endif
	pstResolverInfo->pstHost = new (SstiHost);
	
	if (pstResolverInfo->pstHost)
	{
		addrinfo stHints{};
		memset(&stHints, 0, sizeof (stHints));
		//using AI_CANONNAME in the ai_flags below causes the getaddrinfo to take several seconds to return.
		// Using wireshark, the initial request seems to be very fast but then it turns around and kicks off
		// arpa requests.  It seems to be causing the return to getaddrinfo to take between 4-12 seconds.
//		stHints.ai_flags = AI_CANONNAME;  // AI_V4MAPPED | AI_ADDRCONFIG
		
		{
			// Lock the mutex
			std::unique_lock<std::recursive_mutex> lock (lockMutex);

			// Has the ActiveLookups structure been deleted?
			stiTESTCOND_NOLOG (pstResolverInfo->pstActiveLookups, stiRESULT_ERROR);

			if (pstResolverInfo->pstActiveLookups->pHostNameResolver->m_bIPv6Enabled)
			{
				stHints.ai_family = AF_UNSPEC; // Do not specify the address type so it will return AF_INET or AF_INET6.
			}
			else
			{
				stHints.ai_family = AF_INET; // Specify AF_INET so that we will only get IPv4 addresses.
			}
		}

		stHints.ai_socktype = SOCK_STREAM;		// todo - is this correct?  Should we just get call socket types (in many cases, 
															// this ends up having three results for one IP address 1 - Stream, 1 - DGram, 1 - Raw.
		
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
		if (bResolveHostByName && pstResolverInfo->pstActiveLookups != NULL)
		{
			// Gingerbread HTC devices have problems with getaddrinfo.  Use
			// get host by name for them instead.  This is the old method
			// that was used before the DNS failover code.
			bGotByName = true; /* Indicate not to use getaddrInfo */
			nResolveResult = ResolveHostByName(pstResolverInfo);
		}
		
		nRet = (nResolveResult == stiRESULT_SUCCESS) ? 0 : -1;
#endif

		if (!bGotByName)
		{
			// See if the DNS can resolve the name.
			nRet = getaddrinfo (pstResolverInfo->HostName.c_str (), nullptr, &stHints, &pstResolverInfo->pstHost->pstAddrInfo);
		}
		
		// Lock the mutex
		std::unique_lock<std::recursive_mutex> lock (lockMutex);
		
		// Has the ActiveLookups structure been deleted?
		stiTESTCOND_NOLOG (pstResolverInfo->pstActiveLookups, stiRESULT_ERROR);
		
		if (0 == nRet)
		{
			// Keep track that the addrinfo was allocated by getaddrinfo
			pstResolverInfo->pstHost->bAllocatedByGetAddrInfo = !bGotByName;
			
			// Set the time that it is valid
			pstResolverInfo->pstHost->ExpiresTime = time (nullptr) + nRESOLVED_LOOKUP_TTL;
			
			// getaddrinfo was successful, alert the class of the success.
			CstiHostNameResolver::ThreadCallback (
				CstiHostNameResolver::estiMSG_NAME_RESOLVED, 
				(size_t)pstResolverInfo);
		}
		else
		{
			// getaddrinfo failed, delete the structure that we allocated then alert the calling class that we failed.
			delete pstResolverInfo->pstHost;
			pstResolverInfo->pstHost = nullptr;
			
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
				// If we aren't using getaddrinfo just set the error as unknown.
				pstResolverInfo->ResolverError.assign ("Unknown Error");
#else
				pstResolverInfo->ResolverError.assign (gai_strerror (nRet));
#endif
			
			CstiHostNameResolver::ThreadCallback (
				CstiHostNameResolver::estiMSG_NAME_RESOLVE_FAILED, 
				(size_t)pstResolverInfo);
		}
	}
	else
	{
		std::unique_lock<std::recursive_mutex> lock (lockMutex);
		
		// Has the ActiveLookups structure been deleted?
		stiTESTCOND_NOLOG (pstResolverInfo->pstActiveLookups, stiRESULT_ERROR);
		
		pstResolverInfo->ResolverError.assign ("Missing Host");
		
		CstiHostNameResolver::ThreadCallback (
			CstiHostNameResolver::estiMSG_NAME_RESOLVE_FAILED, 
			(size_t)pstResolverInfo);
	}
	
STI_BAIL:
	
	if (stiIS_ERROR (hResult))
	{

		if (0 == nRet)
		{
			stiDEBUG_TOOL (g_stiDNSDebug,
								// We successfully resolved the address but it took too long so we won't be passing it to the callback.
								stiTrace ("<CstiHostNameResolver::ThreadTask> HostName %s was resolved ... but too late for use\n", pstResolverInfo->HostName.c_str ());
								);
			pstResolverInfo->ResolverError.assign ("Result Late");
			ResolverErrorLog (pstResolverInfo, pstResolverInfo->HostName.c_str (), pstResolverInfo->ResolverError.c_str ());
		}

		
		// Clean up allocated memory and pointers
		if (pstResolverInfo->pstHost)
		{
			if (pstResolverInfo->pstHost->pstAddrInfo)
			{
				freeaddrinfo (pstResolverInfo->pstHost->pstAddrInfo);
				pstResolverInfo->pstHost->pstAddrInfo = nullptr;
			}
			
			delete pstResolverInfo->pstHost;
			pstResolverInfo->pstHost = nullptr;
		}
		
		delete pstResolverInfo;
		pstResolverInfo = nullptr;
	}
	
	return 0;
}


/*! \brief Callback for the thread to notify the caller of completion.
* 
* This method is provided for the thread to inform the class of completion (success or failure).  
* 
* NOTE:  It is expected that the calling function has already locked this class mutex before calling 
* 			ThreadCallback.
*
* \return stiRESULT_SUCCESS always.
*/
stiHResult CstiHostNameResolver::ThreadCallback (
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam)		///< holds data specific to the message
{
	auto pstResolverInfo = (SstiResolverInfo*)MessageParam;
	CstiHostNameResolver *pThis = nullptr;
	SstiActiveLookups *pstActiveLookups;
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bSignalNeeded = false;
	stiTESTCOND (pstResolverInfo && pstResolverInfo->pstActiveLookups, stiRESULT_ERROR);
	pThis = pstResolverInfo->pstActiveLookups->pHostNameResolver;
	
	pstActiveLookups = pstResolverInfo->pstActiveLookups;
	
	switch (n32Message)
	{
		case CstiHostNameResolver::estiMSG_NAME_RESOLVED:
			if (pstResolverInfo->pstHost->pstAddrInfo)
			{
				stiDEBUG_TOOL (g_stiDNSDebug,
					stiTrace ("DNS resolved %s as follows:\n", pstResolverInfo->HostName.c_str ());
				
					addrinfo *pCurrent = pstResolverInfo->pstHost->pstAddrInfo;
					
					while (pCurrent)
					{
						if (nullptr != pCurrent->ai_addr)
						{
							char addressString[INET6_ADDRSTRLEN];
							stiTrace ("\tCanonical Name = %s\n", pCurrent->ai_canonname ? pCurrent->ai_canonname : "");
							auto pSockAddr_in = (sockaddr_in *)pCurrent->ai_addr;
							inet_ntop (pCurrent->ai_family, &pSockAddr_in->sin_addr, addressString, sizeof(addressString));
							stiTrace ("\t\t%s:%d, len:%d, Family:%s(%d) Sock Type:%s(%d)\n", 
										addressString,
										pSockAddr_in->sin_port,
							pCurrent->ai_addrlen,
							AF_INET== pCurrent->ai_family ? "AF_INET" :
							AF_INET6 == pCurrent->ai_family ? "AF_INET6" : "Unknown",
							pCurrent->ai_family,
							SOCK_STREAM == pCurrent->ai_socktype ? "SOCK_STREAM" :
							SOCK_DGRAM == pCurrent->ai_socktype ? "SOCK_DGRAM" : 
							SOCK_RAW == pCurrent->ai_socktype ? "SOCK_RAW" :
							SOCK_RDM == pCurrent->ai_socktype ? "SOCK_RDM" : "other",
							pCurrent->ai_socktype);
						}  // end if
						
						pCurrent = pCurrent->ai_next;
					}
				);  // stiDEBUG_TOOL
				
				// Add the entry to the std::map m_Host
				if (pstResolverInfo->pstHost->pstAddrInfo)
				{
					// Is it already in the std::map?
					pThis->MappedValueRemove (pstResolverInfo->HostName);
					
					// We are ready to add the map entry
					pThis->m_Host[pstResolverInfo->HostName] = pstResolverInfo->pstHost;
					
					// Keep track that we now need to re-write to persistent memory
					pThis->m_bNeedToWrite = true;
				}
				
				bSignalNeeded = true;
			}
			
			break;
			
		case CstiHostNameResolver::estiMSG_NAME_RESOLVE_FAILED:
		{
			stiDEBUG_TOOL (g_stiDNSDebug,
								stiTrace ("<CstiHostNameResolver::ThreadCallback> DNS FAILED to resolve %s:\n", pstResolverInfo->HostName.c_str ());
			);
			
			// If there are no other lookups being done, we need to return this object, even though it failed.  This will unblock the calling thread.
			if (((pstActiveLookups->pstAlternate == pstResolverInfo && !pstActiveLookups->pstPrimary) ||
				(pstActiveLookups->pstPrimary == pstResolverInfo && !pstActiveLookups->pstAlternate)) && 
				!pstActiveLookups->pstFromMap)
			{
				stiDEBUG_TOOL (g_stiDNSDebug,
									stiTrace ("\tNo other lookups and no previous value to fall back on.  Signalling CstiHostNameResolver::Resolve of failure\n");
				);
				
				ResolverErrorLog (nullptr, pstResolverInfo->HostName.c_str (), pstResolverInfo->ResolverError.c_str ());
				
				bSignalNeeded = true;
			}
			else
			{
				// Delete the ResolverInfoStructure, also set the pointer in the ActiveLookups structure to NULL
				
				// Is the returned structure the alternate or the primary?
				if (pstActiveLookups->pstAlternate == pstResolverInfo)
				{
					pstActiveLookups->pstAlternate = nullptr;
				}
				else if (pstActiveLookups->pstPrimary == pstResolverInfo)
				{
					pstActiveLookups->pstPrimary = nullptr;
				}
				
				// Log the failure and address being returned from the map.
				if (!pstActiveLookups->pstAlternate && !pstActiveLookups->pstPrimary)
				{
					pstActiveLookups->pstFromMap->ResolverError = pstResolverInfo->ResolverError;
				}
				
				delete pstResolverInfo;
				pstResolverInfo = nullptr;
			}
			
			break;
		}
		
		case CstiHostNameResolver::estiMSG_NAME_RESOLVE_TIMER_FIRED:
		{
			stiDEBUG_TOOL (g_stiDNSDebug,
								stiTrace ("<CstiHostNameResolver::ThreadCallback> Resolve Timer expired for %s.  Returning previous resolved address.\n", pstResolverInfo->HostName.c_str ());
			);

			ResolverErrorLog (pstResolverInfo, pstResolverInfo->HostName.c_str (), pstResolverInfo->ResolverError.c_str ());

			// Alert the calling thread (blocked in the ::Resolve method) that the timeout has occurred.
			bSignalNeeded = true;
			
			break;
		}
		case CstiHostNameResolver::estiMSG_PRIMARY_TIMER_FIRED:
		{
			// We haven't yet received a response for the primary lookup.  Spawn a task to lookup the alternate.
			stiOSTaskSpawn ("DNSLookupAlt",
				stiDNS_TASK_PRIORITY,
				stiDNS_STACK_SIZE,
				ThreadTask,
				pstActiveLookups->pstAlternate);
			
			break;
		}
			
		default:
			break;
	}
	
	if (bSignalNeeded)
	{
		pThis->m_ResolverInfoReturned.push_back (pstResolverInfo);
		
		// Alert the calling thread (blocked in the ::Resolve method) that we have resolved the address.
		pThis->m_ResolvedCond.notify_all ();
		
		// Break the links between all the ResolverInfo structures and the ActiveLookups structure
		if (pstActiveLookups->pstPrimary)
		{
			pstActiveLookups->pstPrimary->pstActiveLookups = nullptr;
		}
		if (pstActiveLookups->pstAlternate)
		{
			pstActiveLookups->pstAlternate->pstActiveLookups = nullptr;
		}
		if (pstActiveLookups->pstFromMap)
		{
			pstActiveLookups->pstFromMap->pstActiveLookups = nullptr;
		}
	}
	
STI_BAIL:
	
	return hResult;
}

void CstiHostNameResolver::ResolverErrorLog  (
	SstiResolverInfo *pstResolverInfo,
	const char *pszHostName,
	const char *pszError)
{
	std::string DNSAddress1;
	std::string DNSAddress2;
	std::string IPAddress;
	
	EstiIpAddressType eIpAddressType = estiTYPE_IPV4;
	
	if (pstResolverInfo && pstResolverInfo->pstHost->pstAddrInfo->ai_family == AF_INET6)
	{
		eIpAddressType = estiTYPE_IPV6;
	}
	
	stiGetDNSAddress (0, &DNSAddress1, eIpAddressType);
	stiGetDNSAddress (1, &DNSAddress2, eIpAddressType);
	
	if (pstResolverInfo)
	{
		stiInetAddrToString (pstResolverInfo->pstHost->pstAddrInfo, &IPAddress);
	}
	else
	{
		IPAddress = "Unavailable";
	}
	
	stiRemoteLogEventSend ("EventType = DNSLookupFailed Reason = \"%s\" HostName = \"%s\" IPAddress = \"%s\" DNSServer1 = \"%s\" DNSServer2 = \"%s\"",
								  pszError, pszHostName, IPAddress.c_str (), DNSAddress1.c_str (), DNSAddress2.c_str ());
}


/*!\brief Removes addrinfo structure that match the family type provided.
 *
 * \param pstHost The host entry that contains the addrinfo list.
 * \param nFamily The family type to be removed.
 */
void CstiHostNameResolver::AddrInfoByIPTypeClear (
	SstiHost *pstHost,
	int nFamily)
{
	addrinfo *pPrevAddrInfo = nullptr;
	addrinfo *pNextAddrInfo = nullptr;

	if (pstHost->pstAddrInfo == nullptr && !pstHost->SRVRecords.empty ())
	{
		stiTrace ("This is an SRV entry.\n");
	}

	for(addrinfo *pAddrInfo = pstHost->pstAddrInfo; pAddrInfo != nullptr; pAddrInfo = pNextAddrInfo)
	{
		pNextAddrInfo = pAddrInfo->ai_next;

		if (pAddrInfo->ai_family == nFamily)
		{
			if (pPrevAddrInfo)
			{
				pPrevAddrInfo->ai_next = pAddrInfo->ai_next;
			}
			else
			{
				pstHost->pstAddrInfo = pAddrInfo->ai_next;
			}

			pAddrInfo->ai_next = nullptr;

			if (pstHost->bAllocatedByGetAddrInfo)
			{
				freeaddrinfo (pAddrInfo);
				pAddrInfo = nullptr;
			}
			else
			{
				delete pAddrInfo->ai_addr;
				pAddrInfo->ai_addr = nullptr;

				delete pAddrInfo;
				pAddrInfo = nullptr;
			}
		}

		pPrevAddrInfo = pAddrInfo;
	}
}


/*! \brief A method to pass in a resolved addrinfo structure to be stored in persistent storage.
* 
* \return none.
*/
stiHResult CstiHostNameResolver::ResolvedAddressStore (const char *pszHostName, const std::vector<std::string> &IPRecords)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiHost *pstHost = nullptr;
	std::vector<std::string>::const_iterator i;
	bool bIPv4Removed = false;
	bool bIPv6Removed = false;
	time_t ResolutionTime = time (nullptr);

	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	
	stiTESTCOND (pszHostName && *pszHostName && !IPRecords.empty (), stiRESULT_ERROR);
		
	// If this is the first time called, load the persistent data
	Initialize ();

	//
	// Check to see if we have already mapped the hostname.
	// If so, use the existing name.  Otherwise, create a new one
	// and add it to the map.
	//
	{
		auto it = m_Host.find (pszHostName);
		if (it == m_Host.end ())
		{
			//
			// No hostname found.  Create one and add it to the map.
			//
			pstHost = new (SstiHost);
			bIPv4Removed = true;
			bIPv6Removed = true;
		}
		else
		{
			pstHost = it->second;
		}
	}

	for (i = IPRecords.begin (); i != IPRecords.end (); ++i)
	{
		if (!bIPv4Removed && IPv4AddressValidate ((*i).c_str ()))
		{
			//
			// We have an IPv4 address and we haven't cleared them from
			// the list yet so go ahead and clear them now before adding
			// new addresses.
			//
			AddrInfoByIPTypeClear (pstHost, AF_INET);

			bIPv4Removed = true;
		}
		else if (!bIPv6Removed)
		{
			//
			// We have an IPv6 address and we haven't cleared them from
			// the list yet so go ahead and clear them now before adding
			// new addresses.
			//
			AddrInfoByIPTypeClear (pstHost, AF_INET6);

			bIPv6Removed = true;
		}

		AddrInfoCreateAndLoad (pstHost, (*i).c_str (), 0, ResolutionTime + nRESOLVED_LOOKUP_TTL, SOCK_STREAM);
	}

	// Store in our std::map
	m_Host[pszHostName] = pstHost;

	// Set the flag to know we need to write to persistent memory
	m_bNeedToWrite = true;

STI_BAIL:

	return hResult;
}


stiHResult CstiHostNameResolver::ResolvedSRVStore (const char *pszSRVName, const std::vector<SstiSRVRecord> &SRVRecords)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiHost *pstHost = nullptr;
	std::string MapKey;

	std::unique_lock<std::recursive_mutex> lock (lockMutex);

	stiTESTCOND (pszSRVName && *pszSRVName && !SRVRecords.empty (), stiRESULT_ERROR);

	// If this is the first time called, load the persistent data
	Initialize ();

	MapKey = szSRV_PREFIX;
	MapKey += pszSRVName;

	// Search for and remove the mapped value if it already exists
	MappedValueRemove (MapKey);

	// Allocate the memory and add it.
	pstHost = new (SstiHost);

	pstHost->eType = SstiHost::eSRV;
	pstHost->ExpiresTime = time (nullptr) + nRESOLVED_LOOKUP_TTL;
	pstHost->SRVRecords = SRVRecords;

	// Store in our std::map
	m_Host[MapKey] = pstHost;

	// Set the flag to know we need to write to persistent memory
	m_bNeedToWrite = true;

STI_BAIL:

	return hResult;
}


/*!
 * \brief Short live thread for watchdog callback.
 *
 * This is the code normally called from the watchdog callback
 * It must be on a thread instead to avoid keeping the watchdog locked
 * while it's executing (avoid watchdog deadlock)
 **/
void * CstiHostNameResolver::LookupTimerThreadCallbackTask (
	void * param)
{
	stiDEBUG_TOOL (g_stiDNSDebug,
						stiTrace ("<CstiHostNameResolver::LookupTimerThreadCallbackTask>\n");
	);  // end stiDEBUG_TOOL
	
	auto pstResolverInfo = (SstiResolverInfo *)param;
	
	// Delete the timer.  
	stiOSWdDelete (pstResolverInfo->wdLookupTimer);
	pstResolverInfo->wdLookupTimer = nullptr;
	
	// Lock the mutex
	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	
	// Has the ActiveLookups structure been deleted?
	if (pstResolverInfo->pstActiveLookups)
	{
		// Since we are falling back on this one ... update the TTL in the map for this entry so that we won't have to timeout again 
		// on lookups for this same entry for TTL.
		auto it = pstResolverInfo->pstActiveLookups->pHostNameResolver->m_Host.find (pstResolverInfo->HostName);
		if (it != pstResolverInfo->pstActiveLookups->pHostNameResolver->m_Host.end ())
		{
			it->second->ExpiresTime = time (nullptr) + nRESOLVED_LOOKUP_TTL;
		}
		
		pstResolverInfo->pstActiveLookups->pHostNameResolver->ThreadCallback (
			CstiHostNameResolver::estiMSG_NAME_RESOLVE_TIMER_FIRED, 
			(size_t)pstResolverInfo->pstActiveLookups->pstFromMap);
		
	}
	
	else
	{
		// We need to clean up memory and pointers here since we can't call the callback.
		delete pstResolverInfo;
		pstResolverInfo = nullptr;
	}
	
	return (nullptr);
}


/*! \brief Called if the timer fires before the DNS lookup(s) have completed.
 * 
 * \param pParam - a pointer to the SstiResolverInfo
 * \return none.
 */
int CstiHostNameResolver::LookupTimerExpiredFunc (
	size_t param)
{
	stiOSTaskSpawn ("DNSLookupTimerExpired",
					stiDNS_TASK_PRIORITY,
					stiDNS_STACK_SIZE,
					LookupTimerThreadCallbackTask,
					(void*)param);
	return (0);
}


/*!
 * \brief Short live thread for watchdog callback.
 *
 * This is the code normally called from the watchdog callback
 * It must be on a thread instead to avoid keeping the watchdog locked
 * while it's executing (avoid watchdog deadlock)
 **/
void * CstiHostNameResolver::PrimaryTimerThreadCallbackTask (
	void * param)
{
	stiDEBUG_TOOL (g_stiDNSDebug,
						stiTrace ("<CstiHostNameResolver::PrimaryTimerThreadCallbackTask>\n");
	);  // end stiDEBUG_TOOL
	
	auto pstResolverInfo = (SstiResolverInfo *)param;
	
	// Delete the timer.  
	stiOSWdDelete (pstResolverInfo->wdLookupTimer);
	pstResolverInfo->wdLookupTimer = nullptr;
	
	// Lock the mutex
	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	if (pstResolverInfo->pstActiveLookups)
	{
		pstResolverInfo->pstActiveLookups->pHostNameResolver->ThreadCallback (
			CstiHostNameResolver::estiMSG_PRIMARY_TIMER_FIRED, 
			(size_t)pstResolverInfo->pstActiveLookups->pstAlternate);
	}
	else
	{
		// Need to delete the memory allocated for this
		delete pstResolverInfo;
		pstResolverInfo = nullptr;
	}
	
	return (nullptr);
}


/*! \brief Called if the timer fires before the primary lookup has completed.
 * 
 * \param pParam - a pointer to the SstiResolverInfo
 * \return none.
 */
int CstiHostNameResolver::PrimaryTimerExpiredFunc (
	size_t param)
{
	// must execute actual callback not on the watchdog thread.
	stiOSTaskSpawn ("DNSPrimaryTimerExpired",
					stiDNS_TASK_PRIORITY,
					stiDNS_STACK_SIZE,
					PrimaryTimerThreadCallbackTask,
					(void*)param);
	return (0);
}


/*! \brief A method to write resolved addresses to a persistent data store.
* 
* \return stiRESULT_SUCCESS or stiRESULT_ERROR if a failure occurred.
*/
stiHResult CstiHostNameResolver::ResolvedValuesStore ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	
	std::map <std::string, SstiHost*>::iterator it;
	FILE *fp = nullptr;
	std::string oFile;
	stiOSDynamicDataFolderGet (&oFile);
	oFile += szDNS_PERSISTENT_FILE;
	
	fp = fopen (oFile.c_str (), "w");
	stiTESTCOND (fp, stiRESULT_UNABLE_TO_OPEN_FILE);
	
	for (it = m_Host.begin (); it != m_Host.end (); it++)
	{
		if (it->second->eType == SstiHost::eSRV)
		{
			std::vector<SstiSRVRecord>::const_iterator i;

			for (i = it->second->SRVRecords.begin (); i != it->second->SRVRecords.end (); ++i)
			{
				fprintf (fp, "%s\t%s\t%u\t%u\t%u\t%u\n", it->first.c_str (), (*i).HostName.c_str (), (*i).un16Port, (*i).un16Protocol, (*i).un16Priority, (*i).un16Weight);
			}
		}
		else
		{
			for(addrinfo *pAddrInfo = it->second->pstAddrInfo; pAddrInfo != nullptr; pAddrInfo = pAddrInfo->ai_next)
			{
				if (pAddrInfo->ai_family == AF_INET6)
				{
					char szTmpBuf[INET6_ADDRSTRLEN];
					auto pSockAddr_in6 = (struct sockaddr_in6 *)pAddrInfo->ai_addr;
					inet_ntop (AF_INET6, &(pSockAddr_in6->sin6_addr), szTmpBuf, sizeof (szTmpBuf));
					if (!IPv6AddressValidate (it->first.c_str ()))
					{
						fprintf (fp, "%s\t%s\t%u\n", it->first.c_str (), szTmpBuf, pSockAddr_in6->sin6_port);
					}
				}
				else
				{
					char addressString[INET_ADDRSTRLEN];
					auto pSockAddr_in = (sockaddr_in *)pAddrInfo->ai_addr;
					inet_ntop (AF_INET, &pSockAddr_in->sin_addr, addressString, sizeof (addressString));
					// Don't store off any names which are already IP addresses
					if (!IPv4AddressValidate (it->first.c_str ()))
					{
						fprintf (fp, "%s\t%s\t%u\n", it->first.c_str (), addressString, pSockAddr_in->sin_port);
					}
				}
			}
		}
	}
	
STI_BAIL:
	
	if (fp)
	{
		fclose (fp);
		fp = nullptr;
	}
	
	return hResult;
}


/*! \brief A method to read previously resolved addresses from a persistent data store.
* 
* \return stiRESULT_SUCCESS or stiRESULT_ERROR if a failure occurred.
*/
stiHResult CstiHostNameResolver::StoredValuesLoad ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::unique_lock<std::recursive_mutex> lock (lockMutex);
	
	FILE *fp = nullptr;
	char szDNSName [48];
	char szDNSResult [48];
	uint16_t un16Port;
	int nRet;
	std::string oFile;
	stiOSDynamicDataFolderGet (&oFile);
	oFile += szDNS_PERSISTENT_FILE;

	fp = fopen (oFile.c_str (), "r");
	stiTESTCOND_NOLOG (fp, stiRESULT_UNABLE_TO_OPEN_FILE);
	
	do 
	{
		nRet = fscanf (fp, "%47s\t", szDNSName);

		if (nRet == 1)
		{
			//
			// If this is an SRV record entry then
			// set the type and strip off the SRV: portion
			//
			if (strncmp (szDNSName, szSRV_PREFIX, sizeof(szSRV_PREFIX) - 1) == 0)
			{
				uint16_t un16Protocol;
				uint16_t un16Priority;
				uint16_t un16Weight;

				nRet = fscanf (fp, "%47s\t%hu\t%hu\t%hu\t%hu\n", szDNSResult, &un16Port, &un16Protocol, &un16Priority, &un16Weight);

				if (nRet == 5)
				{
					char *pSRVName = &szDNSName[4];

					// We obtained a record.  Allocate the structures and store the read data
					if (NameValidate(pSRVName) && NameValidate(szDNSResult))
					{
						SstiSRVRecord SRVRecord;

						SRVRecord.HostName = szDNSResult;
						SRVRecord.un16Port = un16Port;
						SRVRecord.un16Protocol = un16Protocol;
						SRVRecord.un16Priority = un16Priority;
						SRVRecord.un16Weight = un16Weight;

						if (m_Host.find (szDNSName) == m_Host.end ())
						{
							auto pstHost = new (SstiHost);

							pstHost->eType = SstiHost::eSRV;
							pstHost->ExpiresTime = 0;
							pstHost->SRVRecords.push_back(SRVRecord);

							m_Host[szDNSName] = pstHost;
						}
						else // there already was one, so link this address to it
						{
							m_Host[szDNSName]->SRVRecords.push_back(SRVRecord);
						}
					}
				}
			}
			else
			{
				nRet = fscanf (fp, "%47s\t%hu\n", szDNSResult, &un16Port);

				if (nRet == 2)
				{
					// We obtained a record.  Allocate the structures and store the read data
					if (NameValidate(szDNSName) && IPAddressValidate(szDNSResult))
					{
						std::map<std::string, SstiHost*>::iterator i;

						i = m_Host.find (szDNSName);

						if (i == m_Host.end ())
						{
							auto pstHost = new (SstiHost);
							AddrInfoCreateAndLoad (pstHost, szDNSResult, un16Port, 0, SOCK_STREAM);

							m_Host[szDNSName] = pstHost;
						}
						else // there already was one, so link this address to it
						{
							AddrInfoCreateAndLoad (i->second, szDNSResult, un16Port, 0, SOCK_STREAM);
						}
					}
				}
			}
		}
		else
		{
			break;
		}
	} while (nRet != EOF);

STI_BAIL:
	
	if (fp)
	{
		fclose (fp);
		fp = nullptr;
	}
	
	return hResult;
}


/*! \brief A method that looks for and removes a mapped value if it exists.
* 
* \return stiRESULT_SUCCESS.
*/
stiHResult CstiHostNameResolver::MappedValueRemove (
	const std::string &HostName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Check to see if we already have this in our map.
	auto it = m_Host.find (HostName);
	if (it != m_Host.end ())
	{
		// Now delete the SstiHost structure.
		delete it->second;
		it->second = nullptr;
		
		// Now remove the mapped entry.
		m_Host.erase (it);
	}
	
	return hResult;
}


/*! \brief Called when the timer fires indicating it is time to write to persistent storage.
* 
* \param pParam - a pointer to this class object
* \return none.
*/
int CstiHostNameResolver::PersistentStoreWriteTimerFunc (
	size_t param)
{
	int ret = 0;
	
	auto pThis = (CstiHostNameResolver*)param;

	// Write to storage
	if (pThis->m_bNeedToWrite)
	{
#ifndef stiDISABLE_DNS_CACHE_STORE
		pThis->ResolvedValuesStore ();
#endif
		pThis->m_bNeedToWrite = false;
	}
	
	stiHResult hResult = stiOSWdStart (pThis->m_wdPersistentStoreTimer,
			nPERSISTENT_STORE_DELTA,
			(stiFUNC_PTR)pThis->PersistentStoreWriteTimerFunc, (size_t)pThis);	// Start the timer again

	if (stiIS_ERROR (hResult))
	{
		ret = -1;
		stiASSERT (false);
	}
	
	return (ret);
}


/*! \brief Convert all the addresses contained in an addrinfo into a list of strings.
* 		This should be ipv6-capable if the flag is enabled
* 
* \param pstAddrInfo - a pointer to an addrinfo
* \param pResults - a pointer to vector that will have the results added to it
* \return none.
*/
stiHResult CstiHostNameResolver::IpListFromAddrinfoCreate (const addrinfo *pstAddrInfo, std::vector<std::string> *pResults)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bCreated = false;
	std::string IPAddress;
	
	stiTESTCOND (pstAddrInfo, stiRESULT_ERROR);

	while (pstAddrInfo && pResults)
	{
		if (pstAddrInfo->ai_addr)
		{
			// Only add this address if it is IPV4 or we are using IPV6.
#if APPLICATION != APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
			if (pstAddrInfo->ai_addr->sa_family != AF_INET6 || m_bIPv6Enabled)
#else
		   // On a NAT64 environment we can only support IPV6 addresses being returned on iOS.
			if ((pstAddrInfo->ai_addr->sa_family != AF_INET6 && !m_bIPv6Enabled)
				 || (pstAddrInfo->ai_addr->sa_family == AF_INET6 && m_bIPv6Enabled))
#endif
			{
				stiHResult hResult2 = stiRESULT_SUCCESS;
				hResult2 = stiInetAddrToString (pstAddrInfo, &IPAddress);
				
				// Only add a result if we did not get an error when attempting to get the IP address string.
				if (!stiIS_ERROR (hResult2))
				{
					pResults->push_back (IPAddress);
					bCreated = true;
				}
			}
		}
		
		pstAddrInfo = pstAddrInfo->ai_next;
	}
	
	stiTESTCOND (bCreated, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}
