// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <memory>
#include <boost/algorithm/string.hpp>
#include "stiOS.h"
#include "stiOSWin32.h"
#include "stiOSWin32Net.h"
#ifndef stiDISABLE_PROPERTY_MANAGER
#include "PropertyManager.h"
#endif
#include "stiTrace.h"
#include "stiError.h"
#include "stiDefs.h"
#include "IstiNetwork.h"

#ifdef NTDDI_VERSION
#define PREVIOUS_NTDII NTDDI_VERSION
#undef NTDDI_VERSION
#endif
#define NTDDI_VERSION NTDDI_LONGHORN

#include <iphlpapi.h>
#include <list>
#include <string>


#ifdef PREVIOUS_NTDII
#undef NTDDI_VERSION
#define NTDDI_VERSION PREVIOUS_NTDII
#endif

#pragma comment(lib, "IPHLPAPI.lib")

typedef struct _IP_ADAPTER_INFO_32 {
    struct _IP_ADAPTER_INFO_32* Next;
    DWORD ComboIndex;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    UINT AddressLength;
    uint8_t Address[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    __time32_t LeaseObtained;
    __time32_t LeaseExpires;
} IP_ADAPTER_INFO_32, *PIP_ADAPTER_INFO_32;


void GetAdapterAddresses(
	EstiIpAddressType ipAddressType,
	std::unique_ptr<uint8_t[]>& addressBuffer,
	std::vector<PIP_ADAPTER_ADDRESSES>& vpnAdapters,
	std::vector<PIP_ADAPTER_ADDRESSES>& wiredAdapters,
	std::vector<PIP_ADAPTER_ADDRESSES>& otherAdapters);

int initializeWinSock()
{
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD (2, 2);
	int err = WSAStartup (wVersionRequested, &wsaData);
	if (err != 0)
	{
		printf ("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	return 0;
}

static const int ADAPTER_BUFFER_SIZE = 15 * 1024;
//static unsigned char acAdapterBuffer[ADAPTER_BUFFER_SIZE];

PIP_ADAPTER_ADDRESSES getCurrentAdapterAddresses(PIP_ADAPTER_ADDRESSES pAddresses)
{
	ULONG outBufLen = ADAPTER_BUFFER_SIZE;

	ULONG family = AF_INET;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
	DWORD dwRetVal;
	dwRetVal = GetAdaptersAddresses (family, flags, NULL, pAddresses, &outBufLen);
	if (dwRetVal == NO_ERROR)
	{
		return pAddresses;
	}
	return NULL;
}

/* stiMACAddressValid - returns estiFALSE if MAC address is not present */
EstiBool stiMACAddressValid (
	IN const char* pszMACAddress)
{
	static unsigned char pts7_acAdapterBuffer[ADAPTER_BUFFER_SIZE]; //partially threadsafe 7
	if (0 == strcmpi(pszMACAddress, "00:00:00:00:00:00"))
	{
		return estiFALSE;
	}

	ULONG outBufLen = ADAPTER_BUFFER_SIZE;
	PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)pts7_acAdapterBuffer;

	ULONG family = AF_INET;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
	DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
	if (dwRetVal == NO_ERROR)
	{
		PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			static char acMacAddress[20];
			sprintf_s(acMacAddress, sizeof(acMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
				pCurrAddresses->PhysicalAddress[0], pCurrAddresses->PhysicalAddress[1], pCurrAddresses->PhysicalAddress[2],
				pCurrAddresses->PhysicalAddress[3], pCurrAddresses->PhysicalAddress[4], pCurrAddresses->PhysicalAddress[5]);

			if (!strcmpi(pszMACAddress, acMacAddress))
			{
				return estiTRUE;
			}
			pCurrAddresses = pCurrAddresses->Next;
		}
	}

	return estiFALSE;
}

/* stiOSGetUniqueID - see stiOSNet.h for header information*/
stiHResult 	stiOSGetUniqueID (
	std::string *uniqueID)
{
	if (uniqueID == nullptr)
	{
		return stiRESULT_INVALID_PARAMETER;
	}

	stiHResult hResult = stiRESULT_SUCCESS;
	*uniqueID = std::string ();

#if APPLICATION == APP_HOLDSERVER || APPLICATION == APP_MEDIASERVER || APPLICATION == APP_DHV_PC
#else
	WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance ();

	if (WillowPM::XM_RESULT_NOT_FOUND == pm->propertyGet ("MacAddress", uniqueID, WillowPM::PropertyManager::Persistent))
	{
		uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH];
		hResult = stiOSGetMACAddress (aun8MACAddress);
		if(stiRESULT_SUCCESS == hResult)
		{
			//Save MAC address for future use
			char pszUniqueID[nMAX_MAC_ADDRESS_WITH_COLONS_STRING_LENGTH];
			stiMACAddressFormatWithColons (aun8MACAddress, pszUniqueID);
			*uniqueID = pszUniqueID;

			pm->propertySet ("MacAddress", *uniqueID, WillowPM::PropertyManager::Persistent);
			pm->saveToPersistentStorage ();
		}
	}

#endif // APPLICATION == APP_HOLDSERVER || APPLICATION == APP_MEDIASERVER
	return hResult;
}

void stiMACAddressFormatWithoutColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress)
{
	sprintf (pszMACAddress, "%02X%02X%02X%02X%02X%02X",
			 aun8MACAddress[0], aun8MACAddress[1], aun8MACAddress[2],
			 aun8MACAddress[3], aun8MACAddress[4], aun8MACAddress[5]);
}

void stiMACAddressFormatWithColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress)
{
	sprintf (pszMACAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
			 aun8MACAddress[0], aun8MACAddress[1], aun8MACAddress[2],
			 aun8MACAddress[3], aun8MACAddress[4], aun8MACAddress[5]);
}

/* stiOSGetMACAddress - returns REAL MAC address (NO Tunnel, Loopback or VPN) */
stiHResult 	stiOSGetMACAddress (
	uint8_t aun8MACAddress[])
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;

	std::vector<PIP_ADAPTER_ADDRESSES> vpnAdapters;
	std::vector<PIP_ADAPTER_ADDRESSES> wiredAdapters;
	std::vector<PIP_ADAPTER_ADDRESSES> otherAdapters;

	GetAdapterAddresses (EstiIpAddressType::estiTYPE_IPV4, addressBuffer, vpnAdapters, wiredAdapters, otherAdapters);

	if (wiredAdapters.size() > 0)
	{
		preferredAdapterAddress = wiredAdapters.at(0);
	}
	else if (otherAdapters.size() > 0)
	{
		preferredAdapterAddress = otherAdapters.at(0);
	}

	stiTESTCOND(preferredAdapterAddress != nullptr, stiRESULT_ERROR);

	for (int i = 0; i < MAX_MAC_ADDRESS_LENGTH; i++)
	{
		aun8MACAddress[i] = preferredAdapterAddress->PhysicalAddress[i];
	}

STI_BAIL:
	preferredAdapterAddress = nullptr;

	return hResult;
}

void FormatIPv4Address(const char * ipv4Address, std::string* strIpAddress)
{
	char pszBuf[16];
	// For IPv4, our address will be in bytes 3-6.  Skip over the first two.
	snprintf(pszBuf, 16, "%d.%d.%d.%d", (unsigned char)ipv4Address[2], (unsigned char)ipv4Address[3], (unsigned char)ipv4Address[4], (unsigned char)ipv4Address[5]);
	strIpAddress->assign(pszBuf);
}


void GetAdapterAddresses (EstiIpAddressType ipAddressType, std::unique_ptr<uint8_t[]> &buffer, PIP_ADAPTER_ADDRESSES &addressList)
{
#if !defined(IPV6_ENABLED)
	assert (ipAddressType == EstiIpAddressType::estiTYPE_IPV4);
#endif
	auto family = AF_INET;
	auto flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
	auto size = ADAPTER_ADDRESSES_BUFFER_SIZE;
	auto result = 0UL;
	auto iterations = 0;
	constexpr auto MaxTries = 10U;
	
	addressList = nullptr;

	do
	{
		result = GetAdaptersAddresses (family, flags, NULL, (PIP_ADAPTER_ADDRESSES)buffer.get(), &size);
		if (result == ERROR_BUFFER_OVERFLOW)
		{
			buffer = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
		}
		else if (FAILED (result))
		{
			return;
		}
		else
		{
			addressList = (PIP_ADAPTER_ADDRESSES)buffer.get();
			break;
		}
		iterations++;
	} while (result == ERROR_BUFFER_OVERFLOW && iterations < MaxTries);

}


void GetAdapterAddresses(
	EstiIpAddressType ipAddressType,
	std::unique_ptr<uint8_t[]>& addressBuffer,
	std::vector<PIP_ADAPTER_ADDRESSES>& vpnAdapters,
	std::vector<PIP_ADAPTER_ADDRESSES>& wiredAdapters,
	std::vector<PIP_ADAPTER_ADDRESSES>& otherAdapters)
{
	PIP_ADAPTER_ADDRESSES address = nullptr;

	vpnAdapters.clear();
	wiredAdapters.clear();
	otherAdapters.clear();

	GetAdapterAddresses(ipAddressType, addressBuffer, address);

	while (address)
	{
		if (address->FirstUnicastAddress != nullptr &&
			address->FirstDnsServerAddress != nullptr &&
			address->OperStatus == IfOperStatusUp &&
			!address->ReceiveOnly &&
			address->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
		{
			if (address->IfType == IF_TYPE_TUNNEL || address->PhysicalAddress == nullptr || address->PhysicalAddress[0] == 0)
			{
				vpnAdapters.push_back(address);
			}
			else if (address->IfType == IF_TYPE_ETHERNET_CSMACD)
			{
				wiredAdapters.push_back(address);
			}
			else
			{
				otherAdapters.push_back(address);
			}
		}

		address = address->Next;
	}
}


stiHResult GetPreferredAdapterAddress (const PIP_ADAPTER_ADDRESSES adapterAddresses, PIP_ADAPTER_ADDRESSES &preferredAdapterAddress, const std::string& remoteIpAddress)
{
	auto hResult = stiRESULT_SUCCESS;
	preferredAdapterAddress = nullptr;
	DWORD index{}, result{};
	IPAddr dst{};

	auto currentAddress = adapterAddresses;

	inet_pton(AF_INET, remoteIpAddress.c_str(), &dst);

	result = GetBestInterface(dst, &index);
	stiTESTCOND(SUCCEEDED(result), stiRESULT_ERROR);

	while (currentAddress != nullptr)
	{
		if (currentAddress->IfIndex == index)
		{
			preferredAdapterAddress = currentAddress;
			break;
		}
		currentAddress = currentAddress->Next;
	}

	stiTESTCOND(currentAddress, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult stiOSGetIPAddress(std::string* localIpAddress, EstiIpAddressType addressType)
{
	stiHResult hResult{};

	stiTESTCOND(localIpAddress != nullptr, stiRESULT_INVALID_PARAMETER);

	*localIpAddress = IstiNetwork::InstanceGet()->localIpAddressGet(addressType, "");
	stiTESTCOND(!localIpAddress->empty(), stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}


stiHResult stiOSGetNetworkMask (std::string* p_acNetworkMask, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;
	unsigned long subnetMask = 0;
	char acNetworkMask[6];

	*p_acNetworkMask = "";

	GetAdapterAddresses (eIpAddressType, addressBuffer, addressList);
	stiTESTCOND_NOLOG (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();

	if (preferredAdapterAddress->FirstPrefix)
	{
		for (unsigned int i = 0; i < preferredAdapterAddress->FirstPrefix->Length; i++)
		{
			subnetMask |= 1 << i;
		}
	}

	acNetworkMask[0] = NULL;
	acNetworkMask[1] = NULL;
	acNetworkMask[2] = subnetMask & 0xFF;
	acNetworkMask[3] = subnetMask >> 8 & 0xFF;
	acNetworkMask[4] = subnetMask >> 16 & 0xFF;
	acNetworkMask[5] = subnetMask >> 24 & 0xFF;
	FormatIPv4Address (acNetworkMask, p_acNetworkMask);

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return hResult;
}

stiHResult stiOSGetDNSCount (int * pnNSCount)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;
	auto dnsServerAddressCount = 0;

	GetAdapterAddresses (estiTYPE_IPV4, addressBuffer, addressList);
	stiTESTCOND_NOLOG (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();

	for (auto pDnsAddress = preferredAdapterAddress->FirstDnsServerAddress; pDnsAddress != nullptr; pDnsAddress++)
	{
		dnsServerAddressCount++;
	}

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	*pnNSCount = dnsServerAddressCount;
	return hResult;
}

stiHResult stiOSGetDNSAddress (
	int nIndex,
	std::string* p_szDNSAddress,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;
	PIP_ADAPTER_DNS_SERVER_ADDRESS_XP dnsServerAddress = nullptr;
	auto dnsFound = false;

	*p_szDNSAddress = "";

	GetAdapterAddresses (eIpAddressType, addressBuffer, addressList);
	stiTESTCOND_NOLOG (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();

	dnsServerAddress = preferredAdapterAddress->FirstDnsServerAddress;
	for (int i = 0; dnsServerAddress != NULL; i++)
	{
		if (i == nIndex)
		{
			FormatIPv4Address (dnsServerAddress->Address.lpSockaddr->sa_data, p_szDNSAddress);
			dnsFound = true;
			break; //exit the for loop early because we found our index.
		}
		dnsServerAddress = dnsServerAddress->Next;
	}
	stiTESTCOND_NOLOG (dnsFound, stiRESULT_ERROR);

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return hResult;
}

stiHResult stiOSGetDefaultGatewayAddress (std::string* p_acGatewayAddress, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;

	*p_acGatewayAddress = "";

	GetAdapterAddresses (eIpAddressType, addressBuffer, addressList);
	stiTESTCOND_NOLOG (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();
	stiTESTCOND_NOLOG (preferredAdapterAddress->FirstGatewayAddress, stiRESULT_ERROR);

	FormatIPv4Address (preferredAdapterAddress->FirstGatewayAddress->Address.lpSockaddr->sa_data, p_acGatewayAddress);

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return hResult;
}


EstiBool stiOSGetDHCPEnabled ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<uint8_t[]> addressBuffer (new uint8_t[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;
	EstiBool result = estiFALSE;

	GetAdapterAddresses (estiTYPE_IPV4, addressBuffer, addressList);
	stiTESTCOND_NOLOG (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();

	result = preferredAdapterAddress->Dhcpv4Enabled ? estiTRUE : estiFALSE;

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return result;
}


int stiOSWrite(
	stiSocket Socket,
	const void *pcBuffer,
	size_t length)
{
	int flags = 0;
	int result = send (Socket, (char*)pcBuffer, static_cast<int>(length), flags);
	return result;
}


int stiOSRead(
	stiSocket Socket,
	void *pcBuffer,
	size_t maxLength)
{
	int flags = 0;
	return recv (Socket, (char*)pcBuffer, static_cast<int>(maxLength), flags);
}


stiHResult stiOSSocketShutdown (
	stiSocket Socket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	WSAEVENT closeEvent;
	closeEvent = WSACreateEvent ();

	if (WSAEventSelect (Socket, closeEvent, FD_CLOSE) == 0)
	{
		if (shutdown (Socket, SD_SEND) == 0)
		{
			//WSA_WAIT_TIMEOUT //Timed Out
			//WSA_WAIT_EVENT_0 //Success
			DWORD result = WSAWaitForMultipleEvents (1, &closeEvent, true, 10, false);
			switch (result)
			{
			case WSA_WAIT_TIMEOUT:
				//We timed out waiting for the event
				break;
			case WSA_WAIT_EVENT_0:
				char discardBuffer[100];
				//recv either returns >0 bytes read, 0 success, < 0 error
				//We are trying to read the bytes off the socket so we are just going to check if uint8_ts were read.
				while (recv (Socket, discardBuffer, 100, 0) > 0);
				break;
			}
		}
	}

	WSACloseEvent (closeEvent);
	return hResult;
}


int stiOSClose(
	stiSocket Socket)
{
	return closesocket (Socket);
}

stiSocket stiOSSocket(
	int nDomain,
	int nType,
	int nProtocol)	
{
	
	initializeWinSock ();

	SOCKET s = socket (nDomain, nType, nProtocol);

	return s;
}

stiHResult stiOSConnect(
	stiSocket Socket,
	const struct sockaddr *name,
	int namelen)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if(connect (Socket, name, namelen) == SOCKET_ERROR)
	{
		int error = WSAGetLastError ();
		switch(error)
		{
			case WSAEWOULDBLOCK:
				// With a nonblocking socket, the connection attempt cannot be completed immediately.
				// In this case, connect will return SOCKET_ERROR, and WSAGetLastError will return WSAEWOULDBLOCK.
				return stiRESULT_CONNECTION_IN_PROGRESS;
			case WSAECONNREFUSED:
			default:
				break;
		}

		stiTHROW_NOLOG (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

int stiOSSetsockopt(
	stiSocket Socket,
	int nLevel,
	int nOptName,
	const void *pOptVal,
	int nOptLen)
{
	int result = 0;
	if ((nOptName == SO_SNDTIMEO) || (nOptName == SO_RCVTIMEO))
	{
		//Windows version of setsockopt() expects the value to be in MILLI seconds, not seconds.
		DWORD optValue = *((DWORD *)pOptVal) * 1000;
		result = setsockopt (Socket, nLevel, nOptName, (const char *)(&optValue), sizeof(DWORD));
	}
	else {
		result = setsockopt (Socket, nLevel, nOptName, static_cast<const char*>(pOptVal), nOptLen);
	}
	
	return result;
}

int stiOSGetsockopt(
	stiSocket Socket,
	int nLevel,
	int nOptName,
	void *pOptVal,
	int *nOptLen)
{
	int result = getsockopt (Socket, nLevel, nOptName, static_cast<char*>(pOptVal), nOptLen);
	return result;
}

/*!\brief Tests a port to see if it's in use.
*
* \param port The port to test the connection on.
* \param ipAddress Address to attempt to connect on
*/
stiHResult stiOSPortUsageCheck(short int port, bool isTCP, const char *ipAddress, bool &portInUse)
{
	struct sockaddr_in client = { 0 };
	int sock = 0;
	int result = 0;
	int errorResult = 0;
	stiHResult hResult = stiRESULT_SUCCESS;
	client.sin_port = htons(port);
	result = inet_pton (AF_INET, ipAddress, &client.sin_addr);
	stiTESTCOND (result == 1, stiRESULT_ERROR);

	client.sin_family = AF_INET;

	sock = (int)socket(client.sin_family, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0);

	result = bind(sock, (struct sockaddr *) &client, sizeof(client));
	errorResult = WSAGetLastError();
	closesocket(sock);

	if (result != 0)
	{
		if (WSAEADDRINUSE == errorResult)
		{
			portInUse = true;
		}
		else
		{
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


int stiOSAvailablePortGet (
	const char * ipAddress,
	bool isTCP)
{
	struct sockaddr_in client = { 0 };
	int sock = 0;
	int result = 0;
	int portToBind = 0;
	// Passing 0 for the port will allow the OS to
	// give us any available port.
	client.sin_port = 0;
	client.sin_family = AF_INET;
	result = inet_pton (AF_INET, ipAddress, &client.sin_addr);
	if (result != 1)
	{
		return -1;
	}

	sock = (int)socket (client.sin_family, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0);

	result = bind (sock, (struct sockaddr *) &client, sizeof (client));

	if (result == 0)
	{
		socklen_t len = sizeof (client);
		getsockname (sock, (struct sockaddr *)&client, &len);
		portToBind = client.sin_port;
	}

	closesocket(sock);

	return portToBind;
}

void socketNonBlockingSet (stiSocket socket)
{
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If ulMode = 0, blocking is enabled;
	// If ulMode != 0, non-blocking mode is enabled.
	u_long ulMode = 1;
	ioctlsocket(socket, FIONBIO, &ulMode);
}


void socketBlockingSet (stiSocket socket)
{
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If ulMode = 0, blocking is enabled;
	// If ulMode != 0, non-blocking mode is enabled.
	u_long ulMode = 0;
	ioctlsocket(socket, FIONBIO, &ulMode);
}

