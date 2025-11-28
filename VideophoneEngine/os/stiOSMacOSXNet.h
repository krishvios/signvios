/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSMacOSXNet
*
*	File Name:		stiOSMacOSXNet.h
*
*	Owner:			Isaac Roach
*
*	Abstract:
*		see stiOSMacOSXNet.c
*
*******************************************************************************/
#ifndef STIOMACOSXNET_H
#define STIOMACOSXNET_H

/*
 * Includes
 */
#include <netdb.h>

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
#define stiINVALID_SOCKET -1

#define 	stiFD_SET(fd, fdset) FD_SET (fd, fdset)
#define 	stiFD_CLR(fd, fdset) FD_CLR (fd, fdset)
#define 	stiFD_ZERO(fdset) FD_ZERO (fdset)
#define 	stiFD_ISSET(fd, fdset) FD_ISSET (fd, fdset)
#define 	stiFD_SETSIZE FD_SETSIZE
#if 0 /* //linux */
#define 	stiRESOLV_PARAMS RESOLV_PARAMS_S
#endif

#define MAX_IP_ADDRESS_LENGTH	6
#define MAX_MAC_ADDRESS_LENGTH	6

#define 	stiOSConnect(nFd, pstAddr, nLen) \
				(EstiResult)connect (nFd, pstAddr, nLen)

enum stiHResult stiOSGetHostByName (
	const char *pszHostName,
	struct hostent *pstHostEnt,
	char *pszHostBuff,
	int nBufLen,
	struct hostent **ppstHostTable);

EstiResult stiOSGetIPAddress (unsigned char acIPAddress[]);

std::string stiOSGetAdapterName();

void stiOSSetAdapterName(std::string name);

EstiResult stiOSGetMACAddress(
	uint8_t aun8MACAddress[]);

EstiResult 	stiOSGetUniqueID (
	uint8_t aun8MACAddress[]);

EstiResult stiMACAddressGet (
	OUT char * pszMACAddress);

EstiBool stiMACAddressValid (
	IN const char* pszMACAddress);

void stiMACAddressFormatWithColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

void stiMACAddressFormatWithoutColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress);

EstiResult 	stiOSGetNetworkMask (char acNetworkMask[]);
stiHResult 	stiOSGetDNSCount (int *pnNSCount);
EstiResult 	stiOSGetDNSAddress(IN int nIndex, OUT char szDNSAddress[]);
stiHResult 	stiOSGetDefaultGatewayAddress (std::string *pGatewayAddress, EstiIpAddressType eIpAddressType);
#define 	stiOSRead(nFd, pcBuffer, MaxBytes) \
				read (nFd, pcBuffer, MaxBytes)
#if 0 /* //linux */
#define 	stiOSResolvParamsGet(pResolvParams) \
				resolvParamsGet(pResolvParams)
#define 	stiOSResolvParamsSet(pResolvParams) \
				(EstiResult)resolvParamsSet(pResolvParams)
#endif
#define 	stiOSSelect(nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut) \
				select (nWidth, pstReadFds, pstWriteFds, pstExceptFds, pstTimeOut)
EstiResult 	stiOSSetIPAddress (char acIPAddress[]);
EstiResult 	stiOSSetNetworkMask (char acNetworkMask[]);

typedef int stiSocket;

#define 	stiOSSetsockopt(nSock, nLevel, nOptName, pOptVal, nOptLen) \
				(EstiResult)setsockopt (nSock, nLevel, nOptName, pOptVal, nOptLen)
#define 	stiOSGetsockopt(nSock, nLevel, nOptName, pOptVal, nOptLen) \
				(EstiResult)getsockopt (nSock, nLevel, nOptName, pOptVal, nOptLen)
#define 	stiOSSocket(nDomain, nType, nProtocol) \
				socket (nDomain, nType, nProtocol)
#define 	stiOSWrite(nFd, pcBuffer, nBytes)	\
				write (nFd, pcBuffer, nBytes)
#define		stiOSClose(nFd) \
				close (nFd)

void socketNonBlockingSet (int socket);

void socketBlockingSet (int socket);

stiHResult stiOSPortUsageCheck(short int port, bool isTCP, const char *ipAddress, bool &portInUse);
int stiOSAvailablePortGet(const char *ipAddress, bool isTCP);

#endif /* STIOMACOSXNET_H */
/* end file stiOSMacOSXNet.h */
