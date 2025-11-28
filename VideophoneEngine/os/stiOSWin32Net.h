// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STIOSWIN32NET_H
#define STIOSWIN32NET_H

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <IPtypes.h>
#include <memory>
#include <cstdint>

constexpr unsigned long ADAPTER_ADDRESSES_BUFFER_SIZE = 15000UL;
void GetAdapterAddresses (EstiIpAddressType ipAddressType, std::unique_ptr<uint8_t[]>& addressBuffer, PIP_ADAPTER_ADDRESSES &addressList);
stiHResult GetPreferredAdapterAddress (const PIP_ADAPTER_ADDRESSES adapterAddresses, PIP_ADAPTER_ADDRESSES &preferredAdapterAddress, const std::string& remoteIpAddress);

#define stiINVALID_SOCKET INVALID_SOCKET 

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#define IS_VALID_SOCKET(a) ((a) != INVALID_SOCKET)

#define 	stiFD_SET(fd, fdset) FD_SET (fd, fdset)
#define 	stiFD_CLR(fd, fdset) FD_CLR (fd, fdset)
#define 	stiFD_ZERO(fdset) FD_ZERO (fdset)
#define 	stiFD_ISSET(fd, fdset) FD_ISSET (fd, fdset)
#define 	stiFD_SETSIZE FD_SETSIZE

#define MAX_MAC_ADDRESS_LENGTH	6

stiHResult stiOSGetMACAddress(
	uint8_t aun8MACAddress[]);

EstiBool stiMACAddressValid (
	IN const char* pszMACAddress);
	
stiHResult 	stiOSGetUniqueID(
	std::string *uniqueID);

stiHResult stiOSGetIPAddress(std::string* localIpAddress, EstiIpAddressType ipAddressType);
	
void stiMACAddressFormatWithColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

void stiMACAddressFormatWithoutColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

stiHResult 	stiOSGetNetworkMask(std::string* acNetworkMask, EstiIpAddressType eIpAddressType);

stiHResult 	stiOSGetDNSCount(int *pnNSCount);

stiHResult 	stiOSGetDNSAddress(int nIndex, std::string* szDNSAddress, EstiIpAddressType eIpAddressType);

stiHResult 	stiOSGetDefaultGatewayAddress(std::string* acGatewayAddres, EstiIpAddressType eIpAddressType);

EstiBool	stiOSGetDHCPEnabled();

typedef SOCKET stiSocket;
stiSocket stiOSSocket(
	int nDomain,
	int nType,
	int nProtocol);

int stiOSWrite(
	stiSocket Socket,
	const void *pcBuffer,
	size_t nBytes);

int stiOSRead(
	stiSocket Socket,
	void *pcBuffer,
	size_t MaxBytes);

stiHResult stiOSSocketShutdown (
	stiSocket Socket);

int stiOSClose(
	stiSocket Socket);

#define 	stiOSSelect(nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut) \
				select (nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut)

int stiOSSetsockopt(
	stiSocket Socket,
	int nLevel,
	int nOptName,
	const void *pOptVal,
	int nOptLen);

int stiOSGetsockopt(
	stiSocket Socket,
	int nLevel,
	int nOptName,
	void *pOptVal,
	int *nOptLen);

void socketNonBlockingSet (stiSocket socket);

void socketBlockingSet (stiSocket socket);

stiHResult stiOSConnect(
	stiSocket Socket,
	const struct sockaddr *name,
	int namelen);

stiHResult stiOSPortUsageCheck(
	short int port,
	bool isTCP,
	const char *ipAddress,
	bool &portInUse);

int stiOSAvailablePortGet (
	const char *ipAddress,
	bool isTCP);


#endif // STIOSWIN32NET_H
