/*******************************************************************************
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 *
 *  Module Name:	stiOSMacOSXNet
 *
 *  File Name:	stiOSMacOSXNet.c
 *
 *  Owner:		Isaac Roach
 *
 *	Abstract:
 *		This module contains the OS Abstraction / layering functions between the RTOS
 *		OS Abstraction functions and MacOSX network stack.
 *
 *******************************************************************************/

/*
 * Includes
 */

/* Cross-platform generic includes:*/
#include <stdio.h>
#include <string.h>			/* for strcpy () */

/* General include files:*/

/* Networking include files:*/
#include <sys/ioctl.h>		/* for ioctl () */
#include <unistd.h>			/* for close () */

/* OS Abstraction include files:*/
#include "stiOS.h"	/* public functions for External compatibility layer. */
#include "stiError.h"

/* DNS include files: */
#include <resolv.h>   /* for res_init() */

/* rtnetlink include files: */
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <net/if_dl.h>

#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>

#if !TARGET_OS_IPHONE
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>
#endif
#include <SystemConfiguration/SystemConfiguration.h>

#ifndef stiDISABLE_PROPERTY_MANAGER
#include "PropertyManager.h"
#endif

#include "cmPropertyNameDef.h"
#include "stiTools.h"

/*
 * Constants
 */
#define IPV4_ADDRESS_LENGTH	15
#define IPV6_ADDRESS_LENGTH	50
#define BUFSIZE 8192

/*
 * Typedefs
 */

/*
 * Forward Declarations
 */

/*
 * Globals
 */
static std::string g_adapterName;
std::string g_strMACAddress;  // iOS will populate this during launch with the MAC address stored in the KeyChain.

/*
 * Locals
 */

/*
 * Function Declarations
 */

void stiOSSetAdapterName(std::string name) {
	g_adapterName = name;
}

static EstiResult getLLAddrForName (const char* name, uint8_t aun8MACAddress[])
{
	EstiResult eResult = estiERROR;
	
	struct ifaddrs *ifaphead;
	if (0 == getifaddrs(&ifaphead)) {
		struct ifaddrs *ifap = NULL;
		for (ifap = ifaphead; ifap; ifap = ifap->ifa_next) {
			if (ifap->ifa_addr->sa_family == AF_LINK) {
				const char* ifa_name = name;
				if (0 == strncmp(ifap->ifa_name,ifa_name, strlen(ifa_name))) {
					struct sockaddr_dl *sdl = (struct sockaddr_dl *) ifap->ifa_addr;
					if (sdl) {
						memcpy(aun8MACAddress, LLADDR(sdl), sdl->sdl_alen);
					}
					eResult = estiOK;
					break;
				}
			}
		}
		freeifaddrs(ifaphead);
	}
	
	return (eResult);
} /* end getLLAddrForName */

std::string stiOSGetAdapterName (void)
{
#if TARGET_OS_IPHONE
	return g_adapterName.empty() ? "en0" : g_adapterName;
#else
	SCDynamicStoreRef	store;
	CFStringRef	key;
	CFDictionaryRef	globalDict;
	CFStringRef	primaryInterface = NULL;
	
	store = SCDynamicStoreCreate(NULL, CFSTR("getPrimary"), NULL, NULL);
	if (!store) {
		// shouldn't happen but...
	}
	
	key = SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL,
													 kSCDynamicStoreDomainState,
													 kSCEntNetIPv4);
	globalDict = (CFDictionaryRef)SCDynamicStoreCopyValue(store, key);
	CFRelease(key);
	if (globalDict) {
		primaryInterface = (CFStringRef)CFDictionaryGetValue(globalDict,
															 kSCDynamicStorePropNetPrimaryInterface);
		if (primaryInterface) {
			CFRetain(primaryInterface);
		}
		CFRelease(globalDict);
	}
	CFRelease(store);
	
	if (primaryInterface != NULL) {
		std::string interface;
		interface.resize(CFStringGetLength(primaryInterface) + 1);
		CFStringGetCString(
			primaryInterface,
			(char *)interface.data(),
			CFStringGetLength(primaryInterface) + 1,
			kCFStringEncodingUTF8);
		interface.pop_back(); // Pop the extra null terminator
		return interface;
	}
	return g_adapterName.empty() ? "en1" : "en0";
#endif
}

#if !TARGET_OS_IPHONE

static kern_return_t FindEthernetInterfaces(io_iterator_t *matchingServices);
static kern_return_t GetMACAddress(io_iterator_t intfIterator, UInt8 *MACAddress, UInt8 bufferSize);

// Returns an iterator containing the primary (built-in) Ethernet interface. The caller is responsible for
// releasing the iterator after the caller is done with it.
static kern_return_t FindEthernetInterfaces(io_iterator_t *matchingServices)
{
	kern_return_t           kernResult;
	CFMutableDictionaryRef	matchingDict;
	CFMutableDictionaryRef	propertyMatchDict;
	
	// Ethernet interfaces are instances of class kIOEthernetInterfaceClass.
	// IOServiceMatching is a convenience function to create a dictionary with the key kIOProviderClassKey and
	// the specified value.
	matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);
	
	// Note that another option here would be:
	// matchingDict = IOBSDMatching("en0");
	// but en0: isn't necessarily the primary interface, especially on systems with multiple Ethernet ports.
	
	if (NULL == matchingDict) {
		printf("IOServiceMatching returned a NULL dictionary.\n");
	}
	else {
		// Each IONetworkInterface object has a Boolean property with the key kIOPrimaryInterface. Only the
		// primary (built-in) interface has this property set to TRUE.
		
		// IOServiceGetMatchingServices uses the default matching criteria defined by IOService. This considers
		// only the following properties plus any family-specific matching in this order of precedence
		// (see IOService::passiveMatch):
		//
		// kIOProviderClassKey (IOServiceMatching)
		// kIONameMatchKey (IOServiceNameMatching)
		// kIOPropertyMatchKey
		// kIOPathMatchKey
		// kIOMatchedServiceCountKey
		// family-specific matching
		// kIOBSDNameKey (IOBSDNameMatching)
		// kIOLocationMatchKey
		
		// The IONetworkingFamily does not define any family-specific matching. This means that in
		// order to have IOServiceGetMatchingServices consider the kIOPrimaryInterface property, we must
		// add that property to a separate dictionary and then add that to our matching dictionary
		// specifying kIOPropertyMatchKey.
		
		propertyMatchDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
													  &kCFTypeDictionaryKeyCallBacks,
													  &kCFTypeDictionaryValueCallBacks);
		
		if (NULL == propertyMatchDict) {
			printf("CFDictionaryCreateMutable returned a NULL dictionary.\n");
		}
		else {
			// Set the value in the dictionary of the property with the given key, or add the key
			// to the dictionary if it doesn't exist. This call retains the value object passed in.
			CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue);
			
			// Now add the dictionary containing the matching value for kIOPrimaryInterface to our main
			// matching dictionary. This call will retain propertyMatchDict, so we can release our reference
			// on propertyMatchDict after adding it to matchingDict.
			CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey), propertyMatchDict);
			CFRelease(propertyMatchDict);
		}
	}
	
	// IOServiceGetMatchingServices retains the returned iterator, so release the iterator when we're done with it.
	// IOServiceGetMatchingServices also consumes a reference on the matching dictionary so we don't need to release
	// the dictionary explicitly.
	kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, matchingServices);
	if (KERN_SUCCESS != kernResult) {
		printf("IOServiceGetMatchingServices returned 0x%08x\n", kernResult);
	}
	
	return kernResult;
}

// Given an iterator across a set of Ethernet interfaces, return the MAC address of the last one.
// If no interfaces are found the MAC address is set to an empty string.
// In this sample the iterator should contain just the primary interface.
static kern_return_t GetMACAddress(io_iterator_t intfIterator, UInt8 *MACAddress, UInt8 bufferSize)
{
	io_object_t		intfService;
	io_object_t		controllerService;
	kern_return_t	kernResult = KERN_FAILURE;
	
	// Make sure the caller provided enough buffer space. Protect against buffer overflow problems.
	if (bufferSize < kIOEthernetAddressSize) {
		return kernResult;
	}
	
	// Initialize the returned address
	bzero(MACAddress, bufferSize);
	
	// IOIteratorNext retains the returned object, so release it when we're done with it.
	while ((intfService = IOIteratorNext(intfIterator)))
	{
		CFTypeRef	MACAddressAsCFData;
		
		// IONetworkControllers can't be found directly by the IOServiceGetMatchingServices call,
		// since they are hardware nubs and do not participate in driver matching. In other words,
		// registerService() is never called on them. So we've found the IONetworkInterface and will
		// get its parent controller by asking for it specifically.
		
		// IORegistryEntryGetParentEntry retains the returned object, so release it when we're done with it.
		kernResult = IORegistryEntryGetParentEntry(intfService,
												   kIOServicePlane,
												   &controllerService);
		
		if (KERN_SUCCESS != kernResult) {
			printf("IORegistryEntryGetParentEntry returned 0x%08x\n", kernResult);
		}
		else {
			// Retrieve the MAC address property from the I/O Registry in the form of a CFData
			MACAddressAsCFData = IORegistryEntryCreateCFProperty(controllerService,
																 CFSTR(kIOMACAddress),
																 kCFAllocatorDefault,
																 0);
			if (MACAddressAsCFData) {
				CFShow(MACAddressAsCFData); // for display purposes only; output goes to stderr
				
				// Get the raw bytes of the MAC address from the CFData
				//Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/System/Library/Frameworks/CoreFoundation.framework/Headers/CFData.h:45:6: Candidate function not viable: cannot convert argument of incomplete type 'CFTypeRef' (aka 'const void *') to 'CFDataRef' (aka 'const __CFData *')
				CFDataGetBytes((const __CFData*)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
				CFRelease(MACAddressAsCFData);
			}
			
			// Done with the parent Ethernet controller object so we release it.
			(void) IOObjectRelease(controllerService);
		}
		
		// Done with the Ethernet interface object so we release it.
		(void) IOObjectRelease(intfService);
	}
	
	return kernResult;
}

static void myCFDictionaryApplierFunction(const void *key, const void *value, void *context)
{
	// If the value is a CFDictionary and if it has a key of "InterfaceName" and the value of key is stiOSGetAdapterName, then return the value.
	CFStringRef interfaceName = NULL;
	CFStringRef OSAdapterName = CFStringCreateWithCString(NULL, stiOSGetAdapterName().c_str(), kCFStringEncodingUTF8);
	if (CFDictionaryGetTypeID() == CFGetTypeID(value) &&
		CFDictionaryGetValueIfPresent((CFDictionaryRef) value, kSCPropInterfaceName, (const void **)&interfaceName) &&
		kCFCompareEqualTo == CFStringCompare(OSAdapterName, interfaceName, 0))
	{
		*(CFTypeRef*) context = value;
	}
	
	CFRelease(OSAdapterName);
}

static CFTypeRef FindIPv4Config(CFStringRef key)
{
	CFTypeRef rval = NULL;
	// Create a Store to work with.
	SCDynamicStoreRef store = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("ntouchMac"), NULL, NULL);
	if (store) {
		// Create a pattern to find all IPv4 interfaces in the State and search the store.
		CFStringRef pattern = SCDynamicStoreKeyCreateNetworkServiceEntity(kCFAllocatorDefault, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetIPv4);
		if (pattern) {
			CFArrayRef patterns = CFArrayCreate(kCFAllocatorDefault, (const void **)&pattern, 1, &kCFTypeArrayCallBacks);
			if (patterns) {
				CFDictionaryRef values = SCDynamicStoreCopyMultiple(store, NULL, patterns);
				if (values) {
					// Go through all interfaces looking for the one we want based upon stiOSGetAdapterName.
					CFDictionaryRef interface = NULL;
					CFDictionaryApplyFunction(values, myCFDictionaryApplierFunction, &interface);
					// If there is an interface for stiOSGetAdapterName then find the poperty we want.
					if (interface && CFDictionaryGetValueIfPresent(interface, key, &rval))
						CFRetain(rval);
					CFRelease(values);
				}
				CFRelease(patterns);
			}
			CFRelease(pattern);
		}
		CFRelease(store);
	}
	return rval;
}
#endif

EstiResult stiGenerateUniqueID (char *pszUniqueID)
{
	EstiResult eResult = estiERROR;
	uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH]; // MAC address
	
	aun8MACAddress[0] = 0xFF;
	aun8MACAddress[1] = 0x00;
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
	aun8MACAddress[1] = 0x01;
#elif APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	aun8MACAddress[1] = 0x02;
#elif APPLICATION == APP_NTOUCH_MAC
	aun8MACAddress[1] = 0x03;
#elif APPLICATION == APP_NTOUCH_PC
	aun8MACAddress[1] = 0x04;
#endif
	
	srand (time(0));
	for (int i=2; i<6; i++)
	{
		aun8MACAddress[i] = rand ()%255;
	}
	
	stiMACAddressFormatWithColons (aun8MACAddress, pszUniqueID);
	
	eResult = estiOK;
	return (eResult);
}

EstiBool stiMACAddressIsValid (IN uint8_t aun8MACAddress[])
{
	EstiBool bResult = estiTRUE;
	
	// iOS 7 Blocks Mac address and returns: 02:00:00:00:00:00
	if (aun8MACAddress[0] == 2 && aun8MACAddress[1] == 0)
	{
		bResult = estiFALSE;
	}
	else if (aun8MACAddress[0] == 0 && aun8MACAddress[1] == 0)
	{
		bResult = estiFALSE;
	}
	
	return bResult;
}

/* stiOSGetUniqueID - see stiOSNet.h for header information*/
EstiResult stiOSGetUniqueID (std::string *pszUniqueID)
{
	EstiResult eResult = estiERROR;
	std::string pszMacAddress;
	char uniqueID[nMAX_UNIQUE_ID_LENGTH];
#ifndef stiDISABLE_PROPERTY_MANAGER
	WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance();
	pm->propertyGet(std::string(MAC_ADDRESS), &pszMacAddress);
#endif
	
	// iOS will update g_strMACAddress with a MAC address from the OS KeyChain
	// prior to creating the videophoneEngine instance.
	
	if (g_strMACAddress.size() == nMAX_MAC_ADDRESS_WITH_COLONS_STRING_LENGTH -1)
	{
		strcpy(uniqueID, g_strMACAddress.c_str());
		eResult = estiOK;
	}
	else if (pszMacAddress.length() == nMAX_MAC_ADDRESS_WITH_COLONS_STRING_LENGTH -1)
	{
		strcpy(uniqueID, pszMacAddress.c_str());
		eResult = estiOK;
	}
	else
	{
		uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH]; // MAC address
		eResult = getLLAddrForName("en0", aun8MACAddress);
		if (estiOK == eResult && estiTRUE == stiMACAddressIsValid(aun8MACAddress))
		{
			stiMACAddressFormatWithColons(aun8MACAddress, uniqueID);
		}
		else
		{
			eResult = stiGenerateUniqueID (uniqueID);
			
			// If we're forced to generate a MAC address, make sure it's at least consistent while the app is running
			g_strMACAddress = uniqueID;
		}
#ifndef stiDISABLE_PROPERTY_MANAGER
		if (eResult == estiOK)
		{
			pm->propertySet(MAC_ADDRESS, uniqueID, WillowPM::PropertyManager::Persistent);
			pm->saveToPersistentStorage();
		}
#endif
	}
	*pszUniqueID = uniqueID;
	return (eResult);
} /* end stiOSGetUniqueID */


/* stiOSGetMACAddress - see stiOSNet.h for header information*/
EstiResult stiOSGetMACAddress (uint8_t aun8MACAddress[])
{
	EstiResult eResult = estiERROR;
	eResult = getLLAddrForName("en0", aun8MACAddress);
	
	// iOS blocks access to the MAC address and returns: 02:00:00:00:00:00
	// Use the generated MAC address instead from stiOSGetUniqueID(..)
	// and convert it to the required type.
	if (!stiMACAddressIsValid(aun8MACAddress))
	{
		std::string uniqueID[nMAX_UNIQUE_ID_LENGTH];
		if (estiOK == stiOSGetUniqueID (uniqueID))
		{
			int nMACAddress[MAX_MAC_ADDRESS_LENGTH];
			int result = sscanf(uniqueID->c_str(), "%x:%x:%x:%x:%x:%x",
								&nMACAddress[0],
								&nMACAddress[1],
								&nMACAddress[2],
								&nMACAddress[3],
								&nMACAddress[4],
								&nMACAddress[5]);
			
			if (result == MAX_MAC_ADDRESS_LENGTH )
			{
				for (int i = 0; i < MAX_MAC_ADDRESS_LENGTH; i++)
				{
					aun8MACAddress[i] = nMACAddress[i];
				}
				eResult = estiOK;
			}
			else
			{
				eResult = estiERROR;
			}
		}
	}
	
	return (eResult);
} /* end stiOSGetMACAddress */


//!
// \brief Formats a MAC address into a text string with colons.
//
// The function takes as input an array of 8 bit integers and formats
// the values into a text string with each value separated with colons.
//
// \param n8MACAddress Contains the MAC address values
// \param pszMACAddress Pointer to a string to write the formated text to.
// \return No return value
//
void stiMACAddressFormatWithColons (IN uint8_t aun8MACAddress[], OUT char *pszMACAddress)
{
	sprintf (pszMACAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
			 aun8MACAddress[0], aun8MACAddress[1], aun8MACAddress[2],
			 aun8MACAddress[3], aun8MACAddress[4], aun8MACAddress[5]);
}

//!
// \brief Formats a MAC address into a text string without colons.
//
// The function takes as input an array of 8 bit integers and formats
// the values into a text string without colons separating each value.
//
// \param n8MACAddress Contains the MAC address values
// \param pszMACAddress Pointer to a string to write the formated text to.
// \return No return value
//
void stiMACAddressFormatWithoutColons (IN uint8_t aun8MACAddress[], OUT char *pszMACAddress)
{
	sprintf (pszMACAddress, "%02X%02X%02X%02X%02X%02X",
			 aun8MACAddress[0], aun8MACAddress[1], aun8MACAddress[2],
			 aun8MACAddress[3], aun8MACAddress[4], aun8MACAddress[5]);
}

////////////////////////////////////////////////////////////////////////////////
//; stiMACAddressGet
//
// Description: Retrieves the MAC Address from the network interface device.
//
// Abstract:
//
// Returns:
// estiOK - success
// estiERROR - the char * parameter is a NULL value or stiOSGetMACAddress
// failed
//
EstiResult stiMACAddressGet (OUT char *pszMACAddress) // MAC Address
{
	EstiResult eResult = estiOK;
	
#if	!TARGET_OS_IPHONE
	kern_return_t	kernResult = KERN_SUCCESS;
	io_iterator_t	intfIterator;
	UInt8			MACAddress[kIOEthernetAddressSize];
	
	kernResult = FindEthernetInterfaces(&intfIterator);
	if (KERN_SUCCESS != kernResult) {
		printf("FindEthernetInterfaces returned 0x%08x\n", kernResult);
	}
	else
	{
		kernResult = GetMACAddress(intfIterator, MACAddress, sizeof(MACAddress));
		
		if (KERN_SUCCESS != kernResult)
		{
			printf("GetMACAddress returned 0x%08x\n", kernResult);
		}
		else
		{
			sprintf(pszMACAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
					MACAddress[0], MACAddress[1], MACAddress[2], MACAddress[3], MACAddress[4], MACAddress[5]);
		}
	}
	(void) IOObjectRelease(intfIterator);	// Release the iterator.
#else
	uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH]; // MAC address
	
	if (NULL != pszMACAddress)
	{
		if (estiOK == stiOSGetMACAddress (aun8MACAddress))
		{
			stiMACAddressFormatWithColons(aun8MACAddress, pszMACAddress);
		} // end if
		else
		{
			// Unable to determine MAC address, construct a default.
			strcpy (pszMACAddress, "");
		} // end else
	}
	else
	{
		eResult = estiERROR;
	}
#endif
	
	return (eResult);
	
} // end stiMACAddressGet

/* stiMACAddressValid - returns estiFALSE if MAC address is not present */
EstiBool stiMACAddressValid ()
{
	EstiBool bReturn = estiTRUE;
	uint8_t un8MACAddress[MAX_MAC_ADDRESS_LENGTH];
	
	stiOSGetMACAddress(un8MACAddress);
	
	// Is the MAC address all zeros?
	if (0U == (un8MACAddress[0] | un8MACAddress[1] | un8MACAddress[2]
			   | un8MACAddress[3] | un8MACAddress[4] | un8MACAddress[5]))
	{
		bReturn = estiFALSE;
	}
	
	return bReturn;
}

/* stiOSGetIPAddress - see stiOSNet.h for header information*/
stiHResult stiOSGetIPAddress (std::string *pIPAddress, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	/* Is the character array valid? */
	stiTESTCOND (pIPAddress, stiRESULT_INVALID_PARAMETER);
	
	{
		struct ifaddrs *pAddrs; // linked list
		bool bFound = false;
		char szIPAddress[NI_MAXHOST];
		int nResult;
		
		std::string OSAdapterName = stiOSGetAdapterName();
		stiTrace("stiOSGetIPAddress on adapter: %s", OSAdapterName.c_str());
		nResult = getifaddrs (&pAddrs);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
		
		for (ifaddrs *pAddr = pAddrs; pAddr; pAddr = pAddr->ifa_next)
		{
			if (pAddr->ifa_addr)
			{
				if (strcmp (pAddr->ifa_name, OSAdapterName.c_str()) == 0)
				{
					if (eIpAddressType == estiTYPE_IPV4 && pAddr->ifa_addr->sa_family == AF_INET)
					{
						nResult = getnameinfo (pAddr->ifa_addr,
											   sizeof (struct sockaddr_in),
											   szIPAddress, sizeof(szIPAddress), NULL, 0, NI_NUMERICHOST);

						// Ignore this address if it has a self assigned IP address in the range of 169.x.x.x
						// We are seeing this when connecting to an IPv6 NAT64 network.  The IPv4 address
						// is never recieved via DHCP so after a few moments, a 169.x.x.x address shows up.
						// This may be a bug in iOS.
						if(stiStrNICmp(szIPAddress, "169", 3) == 0)
						{
							memset(szIPAddress, 0, sizeof(szIPAddress));
							continue;
						}
					}
					else if (eIpAddressType == estiTYPE_IPV6 && pAddr->ifa_addr->sa_family == AF_INET6)
					{
						nResult = getnameinfo (pAddr->ifa_addr,
											   sizeof (struct sockaddr_in6),
											   szIPAddress, sizeof(szIPAddress), NULL, 0, NI_NUMERICHOST);
					}
					else
					{
						continue;
					}
					
					if (nResult == 0 && (stiStrNICmp(szIPAddress, stiIPV6_LINK_LOCAL_PREFIX, strlen (stiIPV6_LINK_LOCAL_PREFIX)) != 0))
					{
						if (eIpAddressType == estiTYPE_IPV6)
						{
							// finding ipv6 addresses in this way results in the adapter appended as the scope,
							// like fe80::baca:3aff:fe94:d4b2%eth0 so we need to strip that off
							char *pszPercent = strchr (szIPAddress, '%');
							if (pszPercent)
							{
								pszPercent[0] = '\0';
							}
							
							*pIPAddress = szIPAddress;
						}
						else
						{
							*pIPAddress = szIPAddress;
						}
						
						printf("Found IP Address: %s\n", szIPAddress);
						bFound = true;
						break;
					}
				}
			}
		}
		
		freeifaddrs (pAddrs);
		
		stiTESTCOND (bFound, stiRESULT_ERROR);
	} /* end context */
	
	
STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG(estiFALSE, "Could not find IP Address for IPAddressType=%s Adapter=%s", eIpAddressType == estiTYPE_IPV4 ? "IPv4" : "IPv6", stiOSGetAdapterName().c_str());
	}
	
	return hResult;
} /* end stiOSGetIPAddress */


stiHResult stiOSConnect (int nFd, const struct sockaddr *pstAddr, socklen_t nLen)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int result = connect (nFd, pstAddr, nLen);

	if (result != 0)
	{
		// If there was an error and the error indicates "in progress"
		// or if an interrupt occurred and the socket is not blocking
		// then return "in progress". Otherwise, throw an error.
		if (EINPROGRESS == errno || EINTR == errno)
		{
			auto opts = fcntl (nFd, F_GETFL, 0);

			if (opts & O_NONBLOCK)
			{
				hResult = stiRESULT_CONNECTION_IN_PROGRESS;
			}
			else
			{
				stiTHROW_NOLOG (stiRESULT_ERROR);
			}
		}
		else
		{
			stiTHROW_NOLOG (stiRESULT_ERROR);
		}
	}

STI_BAIL:

	return hResult;
}


stiHResult stiOSGetHostByName (
	const char *pszHostName,
	struct hostent *pstHostEnt,
	char *pszHostBuff,
	int nBufLen,
	struct hostent **ppstHostTable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Force to read resolv.conf to update
	// the DNS information.
	// The res_init() function reads the configuration files
	// (see resolv+(8)) to get the default domain name, search
	// order and name server address(es).
	//	res_init ();
	
	*ppstHostTable = gethostbyname2 (pszHostName, AF_INET);
	if (0 == ppstHostTable)
	{
		printf ("**************** gethostbyname returned  %d!!! ******************\n", h_errno);
		hResult = stiRESULT_ERROR;
	}
	
	return hResult;
}

stiHResult stiOSGetDNSCount (int * pnNSCount)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int result = 0;
	struct __res_state res;
	memset(&res, 0, sizeof(res));
	result = res_ninit(&res);
	
	if (result == 0)
	{
		*pnNSCount = res.nscount;
	}
	
	return hResult;
	res_ndestroy(&res);
}
/* stiOSGetDNSAddress - see stiOSNet.h for header information*/
stiHResult 	stiOSGetDNSAddress(int nIndex, std::string *pDNSAddress, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int result = 0;
	struct __res_state res;
	memset(&res, 0, sizeof(res));
	result = res_ninit(&res);
	
	if (result == 0 && nIndex < res.nscount)
	{
		res_9_sockaddr_union *addr_union = (res_9_sockaddr_union *)malloc(res.nscount * sizeof(res_9_sockaddr_union));
		stiTESTCOND(addr_union, stiRESULT_MEMORY_ALLOC_ERROR);
		
		
		memset(addr_union, 0, res.nscount * sizeof(res_9_sockaddr_union));
		res_getservers(&res, addr_union, res.nscount);
		
		// Get the requested dns server of address type and index if it exists.
		if (eIpAddressType ==  estiTYPE_IPV4 &&
			addr_union[nIndex].sin.sin_family == AF_INET)
		{
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(addr_union[nIndex].sin.sin_addr), ip, INET_ADDRSTRLEN);
			*pDNSAddress = ip;
		}
		else if (eIpAddressType == estiTYPE_IPV6 &&
				 addr_union[nIndex].sin6.sin6_family == AF_INET6)
		{
			char ip[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(addr_union[nIndex].sin6.sin6_addr), ip, INET6_ADDRSTRLEN);
			*pDNSAddress = ip;
		}
		
		free (addr_union);
	}
	else
	{
		hResult = stiRESULT_ERROR;
	}

STI_BAIL:

	res_ndestroy(&res);
	return hResult;
} /* end stiOSGetDNSAddress */

/* stiOSGetDefaultGatewayAddress - see stiOSNet.h for header information*/
stiHResult stiOSGetDefaultGatewayAddress (std::string *pGatewayAddress, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_ERROR;
	
	if (eIpAddressType == estiTYPE_IPV4)
	{
#if !TARGET_OS_IPHONE
		CFStringRef router = (CFStringRef) FindIPv4Config(kSCPropNetIPv4Router);
#else
		CFStringRef router = CFSTR("192.168.0.1");
#endif
		if (router)
		{
			const char* cp = CFStringGetCStringPtr(router, kCFStringEncodingUTF8);
			char buffer[255];
			if (cp == NULL) {
				if (CFStringGetCString(router, buffer, 255, kCFStringEncodingUTF8))
					cp = buffer;
			}

			if (cp)
			{
				pGatewayAddress->assign(cp);
				hResult = stiRESULT_SUCCESS;
			}
			CFRelease(router);
		}
	}
	else if (eIpAddressType == estiTYPE_IPV6)
	{
		//TODO: Add support for IPV6
	}
	
	return hResult;
}


/* stiOSSetIPAddress - see stiOSNet.h for header information*/
EstiResult stiOSSetIPAddress (char acIPAddress[])
{
	EstiResult eResult = estiERROR;
	
	// Unsupported
	
	return (eResult);
} /* end stiOSSetIPAddress */


/* stiOSGetNetworkMask - see stiOSNet.h for header information*/
stiHResult stiOSGetNetworkMask (std::string *pNetworkMask, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_ERROR;
	std::string OSAdapterName = stiOSGetAdapterName();
	
	struct ifaddrs *ifaphead;
	if (0 == getifaddrs(&ifaphead)) {
		struct ifaddrs *ifap = NULL;
		for (ifap = ifaphead; ifap; ifap = ifap->ifa_next) {
			if (ifap->ifa_addr->sa_family == AF_INET) {
				if (0 == strncmp(ifap->ifa_name,OSAdapterName.c_str(), OSAdapterName.length())) {
					struct sockaddr_in *sdl = (struct sockaddr_in *) ifap->ifa_netmask;
					if (sdl)
					{
						char *address = inet_ntoa(*(struct in_addr *)&sdl->sin_addr);
						pNetworkMask->assign(address);
						hResult = stiRESULT_SUCCESS;
						break;
					}
				}
			}
		}
		freeifaddrs(ifaphead);
	}
	
	return hResult;
} /* end stiOSGetNetworkMask */

/* stiOSSetNetworkMask - see stiOSNet.h for header information*/
EstiResult stiOSSetNetworkMask (char acNetworkMask[])
{
	EstiResult eResult = estiERROR;
	
	// Unsupported
	
	return (eResult);
} /* end stiOSSetNetworkMask */

/* stiOSSocketShutdown - see stiOSLinuxNet.h for header information*/
stiHResult stiOSSocketShutdown(
	int nFd)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (shutdown (nFd, SHUT_WR) != -1)
	{
		for (;;)
		{
			fd_set FdSet;
			int nResult = 0;
			timeval TimeOut;

			FD_ZERO (&FdSet);
			FD_SET (nFd, &FdSet);

			TimeOut.tv_sec = 0;
			TimeOut.tv_usec = 10000; // 0.01 seconds

			nResult = select (nFd + 1, &FdSet, 0, 0, &TimeOut);

			if (nResult == 0)
			{
				//
				// Timeout expired.  Break out of the loop.
				//
				break;
			}
			else if (nResult > 0)
			{
				char Discard[100];

				//
				// There appears to be something to read. Read from the socket and discard the results.
				//
				nResult = stiOSRead (nFd, Discard, sizeof(Discard));

				if (nResult <= 0)
				{
					//
					// An error occurred or there was nothing to read so break out of the loop
					//
					break;
				}
			}
			else if (nResult != EINTR)
			{
				//
				// An unexpected error occurred. Break out of the loop.
				//
				break;
			}
		}
	}

	return hResult;
} /* end stiOSSocketShutdown */

/*!\brief Tests a port to see if it's in use.
 *
 * \param port The port to test the connection on.
 * \param ipAddress Address to attempt to connect on
 * \return returns 0 on success or errno value on failure.
 */
stiHResult stiOSPortUsageCheck(short int port, bool isTCP, const char *ipAddress, bool &portInUse)
{
	struct sockaddr_in client = { 0 };
	int sock = 0;
	int result = 0;
	int errorResult = 0;
	stiHResult hResult = stiRESULT_SUCCESS;
	client.sin_port = htons (port);
	client.sin_addr.s_addr = inet_addr (ipAddress);

	if (IPv6AddressValidate (ipAddress))
	{
		client.sin_family = AF_INET6;
	}
	else
	{
		client.sin_family = AF_INET;
	}

	sock = (int)socket (client.sin_family, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0);

	result = bind (sock, (struct sockaddr *) &client, sizeof (client));
	errorResult = errno;
	close(sock);

	if (result != 0)
	{
		switch(errorResult)
		{
			case EADDRINUSE:
				portInUse = true;
				break;
			default:
				stiTHROWMSG(stiRESULT_ERROR, "Bind failed with error: %d", errno);
		}
	}
	else
	{
		portInUse = false;
	}

STI_BAIL:

	return hResult;
}

/*!\brief Get a port that is not in use.
 *
 * \param ipAddress Address to attempt to connect on
 * \return returns available port number if successful or
 *          0 if no port is found.
 */
int stiOSAvailablePortGet(const char *ipAddress, bool isTCP)
{
	struct sockaddr_in client = { 0 };
	int sock = 0;
	int result = 0;
	int portToBind = 0;
	// Passing 0 for the port will allow the OS to
	// give us any available port.
	client.sin_port = 0;
	client.sin_addr.s_addr = inet_addr (ipAddress);
	
	if (IPv6AddressValidate (ipAddress))
	{
		client.sin_family = AF_INET6;
	}
	else
	{
		client.sin_family = AF_INET;
	}
	
	sock = (int)socket (client.sin_family, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	
	result = bind (sock, (struct sockaddr *) &client, sizeof (client));
	
	if (result == 0)
	{
		socklen_t len = sizeof(client);
		getsockname(sock, (struct sockaddr *)&client, &len);
		portToBind = client.sin_port;
	}

	close(sock);
	
	return portToBind;
}

int stiOSClose(int nFd)
{
	return close(nFd);
}

void socketNonBlockingSet (int socket)
{
	auto opts = fcntl (socket, F_GETFL, 0);
	fcntl (socket, F_SETFL, opts | O_NONBLOCK);
}


void socketBlockingSet (int socket)
{
	auto opts = fcntl (socket, F_GETFL, 0);
	fcntl (socket, F_SETFL, opts & (~O_NONBLOCK));
}
