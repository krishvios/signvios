/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSLinuxNet
*
*	File Name:		stiOSLinuxNet.h
*
*	Owner:			Scot L. Brooksby
*
*	Abstract:
*		see stiOSLinuxNet.c
*
*******************************************************************************/
#ifndef STIOSLINUXNET_H
#define STIOSLINUXNET_H

/*
 * Includes
 */
#include <netdb.h>
#include <string>

/*
 * Constants
 */

/*
 * Typedefs
 */

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */
#define stiINVALID_SOCKET (-1)

#define 	stiFD_SET(fd, fdset) FD_SET (fd, fdset)
#define 	stiFD_CLR(fd, fdset) FD_CLR (fd, fdset)
#define 	stiFD_ZERO(fdset) FD_ZERO (fdset)
#define 	stiFD_ISSET(fd, fdset) FD_ISSET (fd, fdset)
#define 	stiFD_SETSIZE FD_SETSIZE
#if 0 /* //linux */
#define 	stiRESOLV_PARAMS RESOLV_PARAMS_S
#endif

//#define MAX_IP_ADDRESS_LENGTH	INET6_ADDRSTRLEN
#define MAX_MAC_ADDRESS_LENGTH	6
#define USE_WIRELESS_NETWORK_DEVICE 1
#define USE_HARDWIRED_NETWORK_DEVICE 0
#define USE_CUSTOM_NETWORK_DEVICE 2


stiHResult stiOSConnect (
	int nFd,
	const struct sockaddr *pstAddr,
	socklen_t nLen);


stiHResult stiOSGetHostByName (
	const char *pszHostName,
	struct hostent *pstHostEnt,
	char *pszHostBuff,
	int nBufLen,
	struct hostent **ppstHostTable);

stiHResult stiOSGetIPAddress (std::string *pIPAddress, EstiIpAddressType eIpAddressType);

int stiOSGetNIC ();

void stiOSSetNIC (int);

EstiResult stiOSGetMACAddress(
	uint8_t aun8MACAddress[]);

stiHResult stiOSGenerateUniqueID (
	std::string *pUniqueID);

stiHResult stiOSSetUniqueID (
	const char *pszUniqueID);

EstiResult 	stiOSGetUniqueID (
	std::string *uniqueID);

EstiResult stiMACAddressGet (
	OUT char * pszMACAddress);

EstiBool stiMACAddressValid ();

void stiMACAddressFormatWithColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

void stiMACAddressFormatWithoutColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

stiHResult 	stiOSGetNetworkMask (std::string *pNetworkMask, EstiIpAddressType eIpAddressType);
stiHResult 	stiOSGetDNSCount (int *pnNSCount);
stiHResult 	stiOSGetDNSAddress(int nIndex, std::string *pDNSAddress, EstiIpAddressType eIpAddressType);
stiHResult 	stiOSGetDefaultGatewayAddress (std::string *pGatewayAddress, EstiIpAddressType eIpAddressType);

#if 0 /* //linux */
#define 	stiOSResolvParamsGet(pResolvParams) \
				resolvParamsGet(pResolvParams)
#define 	stiOSResolvParamsSet(pResolvParams) \
				(EstiResult)resolvParamsSet(pResolvParams)
#endif
#define 	stiOSSelect(nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut) \
				select (nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut)
stiHResult 	stiOSSetIPAddress (std::string IPAddress, EstiIpAddressType eIpAddressType);
stiHResult 	stiOSSetNetworkMask (std::string NetworkMask, EstiIpAddressType eIpAddressType);

typedef int stiSocket;

inline int stiOSSetsockopt(
	int socket,
	int level,
	int optName,
	const void *optVal,
	socklen_t optLen)
{
	return setsockopt (socket, level, optName, optVal, optLen);
}


inline int stiOSGetsockopt(
	int socket,
	int level,
	int optName,
	void *optVal,
	socklen_t *optLen)
{
	return getsockopt (socket, level, optName, optVal, optLen);
}


void socketNonBlockingSet (int socket);


void socketBlockingSet (int socket);


inline stiSocket stiOSSocket(
	int domain,
	int type,
	int protocol)
{
	return socket (domain, type, protocol);
}

inline int stiOSWrite(
	int socket,
	const void *buffer,
		size_t numBytes)
{
	return write (socket, buffer, numBytes);
}


stiHResult stiOSSocketShutdown (int nFd);

int stiOSClose(int nFd);

stiHResult stiOSPortUsageCheck(
		short int port,
		bool istTCP,
		const char *ipAddress,
		bool &portInUse);

int stiOSAvailablePortGet(
		const char *ipAddress, bool isTCP);

#endif /* STIOSLINUXNET_H */
/* end file stiOSLinuxNet.h */
