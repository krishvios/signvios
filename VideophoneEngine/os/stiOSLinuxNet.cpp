/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSNetLinux
*
*  File Name:	stiOSNetLinux.c
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and Linux network stack.
*
*******************************************************************************/

/*
 * Includes
 */

/* Cross-platform generic includes:*/
#include <cstdio>
#include <cstring>			/* for strcpy () */

/* Linux general include files:*/

/* Linux networking include files:*/
#include <net/if.h>			/* for IFNAMSIZ */
#include <netinet/in.h>		/* for inreq, ifreq6 */
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
#include <ifaddrs.h>
#endif
#include <linux/sockios.h>	/* for SIOCGIFHWADDR */
#include <sys/ioctl.h>		/* for ioctl () */
#include <sys/select.h>
#include <unistd.h>			/* for close () */
#include <fcntl.h>

/* OS Abstraction include files:*/
#include "stiOS.h"	/* public functions for External compatibility layer. */

/* Linux DNS include files: */
#include <resolv.h>   /* for res_init() */

/* Linux rtnetlink include files: */
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
//#include <net/if.h>
#include <sys/socket.h>
//#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include "stiTrace.h"
#include "stiError.h"
#include "stiTools.h"
#include "IstiNetwork.h"

/*
 * Constants
 */
#define IPV4_ADDRESS_LENGTH	15
#define IPV6_ADDRESS_LENGTH	50
#define BUFSIZE 8192

/*
 * Typedefs
 */

// This guy is in linux/ipv6.h, but is also declared in netinet/in.h in an
// incomplete form so we must define it here to get around the conflict.
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
struct in6_ifreq {
	struct in6_addr ifr6_addr;
	__u32 ifr6_prefixlen;
	unsigned int ifr6_ifindex;
};
#endif
/*
 * Forward Declarations
 */

/*
 * Globals
 */
static int g_nUseWireless = USE_HARDWIRED_NETWORK_DEVICE;
static char g_strNetworkIntefaceName[IFNAMSIZ];

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
extern EstiResult stiAndroidGetUniqueId(char *);
extern char g_macAddress[18];
#endif

static std::string g_oUniqueID;

/*
 * Locals
 */

/*
 * Function Declarations
 */



#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
EstiResult stiAndroidMACToArray(IN char *pszMACAddress, OUT uint8_t aun8MACAddress[]) {
	EstiResult eResult = estiOK;
	int last = -1;

	uint8_t *a = aun8MACAddress;
	int rc = sscanf(pszMACAddress, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
					a + 0, a + 1, a + 2, a + 3, a + 4, a + 5,
					&last);
	if(rc != 6 || strlen(pszMACAddress) != last) {
		eResult = estiERROR;
	}
	return eResult;
}
#endif

/* stiOSSetNIC - */
void stiOSSetNIC (int nUseWireless)
{
	g_nUseWireless = nUseWireless;
} /* end stiOSSetNIC */

/* stiOSSetNIC - */
int stiOSGetNIC ()
{
	return g_nUseWireless;
} /* end stiOSSetNIC */

/* stiOSSetNICName - */
void stiOSSetCustomNIC (const char* name)
{
	g_nUseWireless = USE_CUSTOM_NETWORK_DEVICE;
	strncpy(g_strNetworkIntefaceName, name, IFNAMSIZ-1);
} /* end stiOSSetNICName */

/* stiOSGetNICName - */
std::string stiOSGetNICName()
{
	std::string name;
#if APPLICATION != APP_NTOUCH_ANDROID  && APPLICATION != APP_DHV_ANDROID && SUBDEVICE != SUBDEV_RCU && APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL

	// First try getting the NIC name from the IstiNetwork interface
	IstiNetwork *pNetwork = IstiNetwork::InstanceGet ();
	if (pNetwork)
	{
		name = pNetwork->NetworkDeviceNameGet();
	}
#else
    //If it is android or VP RCU, lets just do what we have always done.
	name = "";
#endif

	// IstiNetwork didn't provide a name, fallback to hard-coded device name
	if (name.empty())
	{
		switch (g_nUseWireless)
		{
			case USE_HARDWIRED_NETWORK_DEVICE:
#if APPLICATION == APP_NTOUCH_VP2
				name = "eth0";
#elif APPLICATION == APP_NTOUCH_VP3
				name = "enp2s0";
#elif APPLICATION == APP_NTOUCH_VP4
				name = "eth0";
#endif
				break;

			case USE_WIRELESS_NETWORK_DEVICE:
#if APPLICATION == APP_NTOUCH_VP2// || APPLICATION == APP_NTOUCH_VP3
				name = "wlan0";
				//TODO: Add vp4?
#else
				name = "ra0";
#endif
				break;

			case USE_CUSTOM_NETWORK_DEVICE:
				name = g_strNetworkIntefaceName;

				break;
		}
	}
	return name;
} /* end stiOSGetNICName */


static stiHResult IPv4AddressGet (
	char *pSAData,
	std::string *pIPAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char szBuffer[IPV4_ADDRESS_LENGTH + 1];
	int nLength = 0;

	/* Copy the IP address to the return buffer */
	nLength = snprintf (szBuffer, sizeof(szBuffer), "%d.%d.%d.%d",
						(unsigned char) pSAData[2],
						(unsigned char) pSAData[3],
						(unsigned char) pSAData[4],
						(unsigned char) pSAData[5]);
	stiTESTCOND (nLength >= 0 && nLength < (signed)sizeof(szBuffer), stiRESULT_ERROR);

	*pIPAddress = szBuffer;

STI_BAIL:

	return hResult;
}


///\brief Get the MAC address of the specified device.
///
static EstiResult stiOSGetDeviceMACAddress (
	uint8_t aun8MACAddress[],
	const char *pszDevice)
{
	EstiResult eResult = estiERROR;

	struct ifreq stIfr{};
	int fdSock = -1;		/* generic raw socket desc.  */

	/* Is the character array valid? */
	if (nullptr != aun8MACAddress)
	{
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
		eResult = stiAndroidMACToArray(g_macAddress, aun8MACAddress);
#else
		/* Yes! Create a socket for getting the address */
		fdSock = socket (AF_INET, SOCK_DGRAM, 0);

		/* Did we get a valid socket? */
		if (fdSock >= 0)
		{
			strcpy (stIfr.ifr_name, pszDevice);

			/* Retrieve the structure containing information about the device*/
			if (ioctl(fdSock, SIOCGIFHWADDR, &stIfr) >= 0)
			{
				/* Copy the mac address to the return buffer */
				memcpy (aun8MACAddress, stIfr.ifr_hwaddr.sa_data, MAX_MAC_ADDRESS_LENGTH);
				eResult = estiOK;
			} /* end if */

			close (fdSock);
		} /* end if */
#endif
	} /* end if */

	return (eResult);
}


stiHResult stiOSGenerateUniqueID (
	std::string *pUniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	char szUniqueID[nMAX_UNIQUE_ID_LENGTH];

	//
	// There wasn't a MAC Address override present in the property table.
	// Retrieve the MAC Address from the device.
	//
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	eResult = stiAndroidGetUniqueId (szUniqueID);
	aLOGD("stiAndroidGetUniqueId: %s", szUniqueID);
#else
	uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH]; // MAC address

	eResult = stiOSGetDeviceMACAddress (aun8MACAddress, stiOSGetNICName().c_str());

	if (estiOK == eResult)
	{
		stiMACAddressFormatWithColons(aun8MACAddress, szUniqueID);
	} // end if
#endif

	stiTESTCOND (eResult == estiOK, stiRESULT_ERROR);

	*pUniqueID = szUniqueID;

STI_BAIL:

	return hResult;
}


stiHResult stiOSSetUniqueID (
	const char *pszUniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	g_oUniqueID = pszUniqueID;

	return hResult;
}


/* stiOSGetUniqueID - see stiOSNet.h for header information*/
EstiResult 	stiOSGetUniqueID (
	std::string *uniqueID)
{
	EstiResult eResult = estiOK;

	if (uniqueID)
	{
		if (g_oUniqueID.length () > 0)
		{
			*uniqueID = g_oUniqueID;
		}
		else
		{
			eResult = estiERROR;
		}
	}
	else
	{
		eResult = estiERROR;
	}

	return (eResult);
} /* end stiOSGetUniqueID */


/* stiOSGetMACAddress - see stiOSNet.h for header information*/
EstiResult 	stiOSGetMACAddress (
	uint8_t aun8MACAddress[])
{
	EstiResult eResult = estiERROR;

	eResult = stiOSGetDeviceMACAddress (aun8MACAddress, stiOSGetNICName().c_str());

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
void stiMACAddressFormatWithColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress)
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
void stiMACAddressFormatWithoutColons(
	IN uint8_t aun8MACAddress[],
	OUT char *pszMACAddress)
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
EstiResult stiMACAddressGet (
	OUT char *pszMACAddress) // MAC Address
{
	EstiResult eResult = estiOK;
	uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH]; // MAC address

	if (nullptr != pszMACAddress)
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

	return (eResult);

} // end stiMACAddressGet


/* stiMACAddressValid - returns estiFALSE if MAC address is all zeros */
EstiBool stiMACAddressValid ()
{
	EstiBool bReturn = estiFALSE;
	uint8_t un8MACAddress[MAX_MAC_ADDRESS_LENGTH];

	auto result = stiOSGetMACAddress(un8MACAddress);

	if (result == estiOK)
	{
		// Is the MAC address all zeros?
		if (0U != (un8MACAddress[0] | un8MACAddress[1] | un8MACAddress[2]
			| un8MACAddress[3] | un8MACAddress[4] | un8MACAddress[5]))
		{
			bReturn = estiTRUE;
		}
	}

	return bReturn;
}


/* stiOSGetIPAddress - see stiOSNet.h for header information*/
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
stiHResult stiOSGetIPAddress (
	std::string *pIPAddress,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IstiNetwork *pNetwork = IstiNetwork::InstanceGet ();

	stiTESTCOND (pNetwork, stiRESULT_ERROR);

	{
#if APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL
		int nNet = pNetwork->WirelessInUse ();
		stiOSSetNIC (nNet);
#endif

		*pIPAddress = pNetwork->localIpAddressGet (eIpAddressType, {});
		stiTESTCOND (!pIPAddress->empty (), stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
} /* end stiOSGetIPAddress */
#else
extern stiHResult stiAndroidOSGetIPAddressFromUI (std::string *pIPAddress, EstiIpAddressType eIpAddressType);

stiHResult stiAndroidOSGetIPAddress(std::string *pIPAddress, EstiIpAddressType eIpAddressType) {
	stiHResult eResult = stiRESULT_ERROR;
	char acIPAddress[255];

	struct ifreq stIfr;
	int fdSock = -1;		/* generic raw socket desc.  */

	/* Is the character array valid? */
	if (NULL != acIPAddress)
	{
		/* Yes! Create a socket for getting the address */
		fdSock = socket (AF_INET, SOCK_DGRAM, 0);

		/* Did we get a valid socket? */
		if (fdSock >= 0)
		{
			int nRtn = 0;
			strcpy (stIfr.ifr_name, stiOSGetNICName().c_str());

			/* Retrieve the structure containing information about the device*/
			if ((nRtn = ioctl(fdSock, SIOCGIFADDR, &stIfr)) >= 0)
			{
				/* Copy the IP address to the return buffer */
				memcpy (acIPAddress, stIfr.ifr_addr.sa_data, INET_ADDRSTRLEN);
				if(strlen(acIPAddress) > 0)
				{
					pIPAddress->assign(acIPAddress);
				}
				else
				{
					stiAndroidOSGetIPAddressFromUI(pIPAddress, eIpAddressType);
				}
				eResult = stiRESULT_SUCCESS;
			} /* end if */

			close (fdSock);
		} /* end if */
	} /* end if */

	return (eResult);
}

stiHResult stiOSGetIPAddress (std::string *pIPAddress, EstiIpAddressType eIpAddressType)
{
	// Android doesn't have an easy way to use getifaddrs
	return stiAndroidOSGetIPAddress(pIPAddress, eIpAddressType);
}
#endif

stiHResult stiOSConnect (
	int nFd,
	const struct sockaddr *pstAddr,
	socklen_t nLen)
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

int stiOSClose(int nFd)
{
	return close (nFd);
}


stiHResult stiOSGetHostByName (
	const char *pszHostName,
	struct hostent *pstHostEnt,
	char *pszHostBuff,
	int nBufLen,
	struct hostent **ppstHostTable)
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);

// NOTE: If the method fails to return a valid host, the loop iteration count
//  should be increased rather than the retry time value.
#define RETRY_TIME 50000
#define RETRY_COUNT 3

	int nRetError;
	int nRet;
	for (int i = 0; i < RETRY_COUNT; ++i)
	{
		nRet = gethostbyname_r (pszHostName, pstHostEnt, pszHostBuff, nBufLen, ppstHostTable, &nRetError);
		if (nRet == ERANGE)
		{
			stiDEBUG_TOOL (g_stiDNSDebug,
				stiTrace ("**************** Buffer passed to gethostbyname_r is too small [%d] !!! ******************\n", nBufLen);
			);
			
			stiTESTCOND_NOLOG (false, stiRESULT_BUFFER_TOO_SMALL);
		}
		if (nRet == EAGAIN)
		{
			// Server was unavailable, sleep momentarily and try again
			usleep(RETRY_TIME);
		}
		else
		{
			break;
		}
	}
/*	if (0 != nRetError)
	{
		printf ("**************** gethostbyname_r returned  %d!!! ******************\n", nRetError);
	}*/

	if (*ppstHostTable)
	{
		hResult = stiRESULT_SUCCESS;
	}

STI_BAIL:

	return hResult;
}

#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
stiHResult stiOSGetDNSCount (
	int *pnNSCount, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int nResult = res_init ();
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	*pnNSCount = _res.nscount;

STI_BAIL:

	return hResult;
}
#else
extern stiHResult stiAndroidOSGetDNSCount(int * pnNSCount, const char *interfaceName);

stiHResult 	stiOSGetDNSCount (
							  int * pnNSCount, EstiIpAddressType eIpAddressType)
{
	// Android doesn't have an easy way to access res_init().
	// We'll access it from the Java apis.
	return stiAndroidOSGetDNSCount(pnNSCount, stiOSGetNICName().c_str());
}
#endif

#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
/* stiOSGetDNSAddress - see stiOSNet.h for header information*/
stiHResult 	stiOSGetDNSAddress (
	int nIndex,
	std::string *pDNSAddress,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;

	stiTESTCOND (pDNSAddress, stiRESULT_INVALID_PARAMETER);

	nResult = res_init ();
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	stiTESTCOND (nIndex < _res.nscount, stiRESULT_ERROR);

	{
		auto sa = (struct sockaddr *) &(_res.nsaddr_list[nIndex]);

		hResult = IPv4AddressGet (sa->sa_data, pDNSAddress);
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
} /* end stiOSGetDNSAddress */
#else
extern stiHResult stiAndroidOSGetDNSAddress(int nIndex, std::string *pDNSAddress, EstiIpAddressType eIpAddressType);

stiHResult 	stiOSGetDNSAddress (
	int nIndex,
	std::string *pDNSAddress,
	EstiIpAddressType eIpAddressType)
{
	// Android doesn't have an easy way to access res_init().
	// We'll access it from the Java apis.
	return stiAndroidOSGetDNSAddress(nIndex, pDNSAddress, eIpAddressType);
} /* end stiOSGetDNSAddress */
#endif

/* readSock - function called by stiOSGetDefaultGatewayAddress */
int readSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, flag = 0, msgLen = 0;

	do
	{
		/* Recieve response from kernel */
		if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
		{
			return -1;
		}
		nlHdr = (struct nlmsghdr *)bufPtr;

		/* Check if header is valid */
		if((NLMSG_OK(nlHdr, (unsigned)readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
		{
			return -1;
		}

		/* Check if its the last message */
		if(nlHdr->nlmsg_type == NLMSG_DONE)
		{
			flag = 1;
			break;
		}
		else
		{
			/* Move the buffer pointer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}

		if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
		{
			flag = 1;

			break;
		}
	}
	while((nlHdr->nlmsg_seq != (unsigned)seqNum) || (nlHdr->nlmsg_pid != (unsigned)pId) || (flag == 0));

	return msgLen;

}

/* stiOSGetDefaultGatewayAddress - see stiOSNet.h for header information*/
stiHResult stiOSGetDefaultGatewayAddress (
	std::string *pGatewayAddress,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int fdSock = -1;		/* netlink socket desc.  */
	int nResult;
	char msgBuf[BUFSIZE];
	struct nlmsghdr *nlMsg;
	struct rtmsg *rtMsg;
	int msgSeq = 0;
	int len = 0;
	bool bFoundGateway = false;

	stiTESTCOND (pGatewayAddress, stiRESULT_INVALID_PARAMETER);

	/* Create a socket for getting routing table (refer to rtnetlink) */
	fdSock = socket (PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	stiTESTCOND (fdSock > 0, stiRESULT_ERROR);

	/* Initialize the buffer */
	memset(msgBuf, 0, BUFSIZE);

	/* point the header and the msg structure pointers into the buffer */
	nlMsg = (struct nlmsghdr *)msgBuf;
	rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header*/
	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid = getpid();

	/* Send the request */
	nResult = send(fdSock, nlMsg, nlMsg->nlmsg_len, 0);
	stiTESTCOND (nResult != -1, stiRESULT_ERROR);

	/* Read the response */
	len = readSock(fdSock, msgBuf, msgSeq, getpid());
	stiTESTCOND (len >= 0, stiRESULT_ERROR);

	/* Parse the response */
	for(;NLMSG_OK(nlMsg, (unsigned)len);nlMsg = NLMSG_NEXT(nlMsg,len))
	{
		unsigned int val = 0;
		struct sockaddr_in dstAddr{};
		struct sockaddr_in gatewayAddr{};

		memset (&dstAddr, 0, sizeof (struct sockaddr_in));
		memset (&gatewayAddr, 0, sizeof (struct sockaddr_in));

		rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

		/* If the route is not for AF_INET or does not belong to main routing table then return. */
		if(rtMsg->rtm_family != AF_INET && rtMsg->rtm_family != AF_INET6)
		{
			//stiTrace("stiOSGetDefaultGatewayAddress AF_INET\n");
			break;
		}

		/* get the rtattr field */
		auto rtAttr = (struct rtattr *)RTM_RTA(rtMsg);

		int rtLen = RTM_PAYLOAD(nlMsg);

		for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen))
		{
			switch(rtAttr->rta_type)
			{
				case RTA_GATEWAY:
					val =  *(unsigned int *) RTA_DATA(rtAttr);
					gatewayAddr.sin_addr.s_addr = (in_addr_t) val;
					break;
				case RTA_DST:
					val =  *(unsigned int *) RTA_DATA(rtAttr);
					dstAddr.sin_addr.s_addr = (in_addr_t) val;
					break;
			} // end swith
		} // end for

		/* Check if it's a default gateway address */
		if (0 == dstAddr.sin_addr.s_addr)
		{
			auto sa = (struct sockaddr *) &gatewayAddr;

			hResult = IPv4AddressGet (sa->sa_data, pGatewayAddress);
			stiTESTRESULT ();

			bFoundGateway = true;
			break;
		}
	} // end for

	stiTESTCOND (bFoundGateway, stiRESULT_ERROR);

STI_BAIL:
	if (fdSock > 0)
	{
		close (fdSock);
	}

	return hResult;
}


/* stiOSSetIPAddress - see stiOSNet.h for header information*/
stiHResult 	stiOSSetIPAddress (
	const std::string IPAddress, EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int fdSock = -1;		/* generic raw socket desc.  */
	int nResult;
	struct ifreq stIfr{}; // Needed for ipv4 and ipv6

	stiTESTCOND (!IPAddress.empty (), stiRESULT_INVALID_PARAMETER);

	/* Create a socket for getting the address */
	fdSock = socket (
#ifdef IPV6_ENABLED
			eIpAddressType == estiTYPE_IPV6 ? AF_INET6 : AF_INET,
#else
			AF_INET,
#endif
			SOCK_DGRAM, 0);
	stiTESTCOND (fdSock >= 0, stiRESULT_ERROR);

	strcpy (stIfr.ifr_name, stiOSGetNICName().c_str());

	if (eIpAddressType == estiTYPE_IPV4)
	{
		/* Clear the buffer before setting */
		memset (stIfr.ifr_addr.sa_data, 0, sizeof(stIfr.ifr_addr.sa_data));

		/* Copy the IP address to the device info structure */
		memcpy (stIfr.ifr_addr.sa_data, IPAddress.c_str (), 4);

		/* Set the information in the device */
		nResult = ioctl(fdSock, SIOCSIFADDR, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}
#ifdef IPV6_ENABLED
	else if (eIpAddressType == estiTYPE_IPV6)
	{
		struct sockaddr_in6 socaddr6;
		struct in6_ifreq ifr6;

		memset(&socaddr6, 0, sizeof(struct sockaddr));
		socaddr6.sin6_family = AF_INET6;
		socaddr6.sin6_port = 0;

		nResult = inet_pton (AF_INET6, IPAddress.c_str (), (void *)&socaddr6.sin6_addr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		memcpy((char *) &ifr6.ifr6_addr, (char *)&socaddr6.sin6_addr, sizeof(struct in6_addr));

		nResult = ioctl (fdSock, SIOGIFINDEX, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		ifr6.ifr6_ifindex = stIfr.ifr_ifindex;
		ifr6.ifr6_prefixlen = 64; // TODO: Get this from the settings somehow

		nResult = ioctl (fdSock, SIOCSIFADDR, &ifr6);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		stIfr.ifr_flags |= IFF_UP | IFF_RUNNING;

		nResult = ioctl (fdSock, SIOCSIFFLAGS, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}
#endif // IPV6_ENABLED

STI_BAIL:
	if (fdSock >=0)
	{
		close (fdSock);
	}

	return hResult;
} /* end stiOSSetIPAddress */


/* stiOSGetNetworkMask - see stiOSNet.h for header information*/
stiHResult 	stiOSGetNetworkMask (
	std::string *pNetworkMask,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct ifreq stIfr{};
	int fdSock = -1;		/* generic raw socket desc.  */
	int nResult;

	stiTESTCOND (pNetworkMask, stiRESULT_INVALID_PARAMETER);

	/* Create a socket for getting the address */
	fdSock = socket (
#ifdef IPV6_ENABLED
			eIpAddressType == estiTYPE_IPV6 ? AF_INET6 : AF_INET,
#else
			AF_INET,
#endif
			SOCK_DGRAM, 0);
	stiTESTCOND (fdSock >= 0, stiRESULT_ERROR);

	strcpy (stIfr.ifr_name, stiOSGetNICName().c_str());

	/* Retrieve the structure containing information about the device*/
	// TODO: As far as I can tell, this ioctl is invalid for ipv6, but
	// I'm not sure how to get the ipv6 net prefix.
	nResult = ioctl(fdSock, SIOCGIFNETMASK, &stIfr);
	stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

	if (eIpAddressType == estiTYPE_IPV4)
	{
		hResult = IPv4AddressGet (stIfr.ifr_netmask.sa_data, pNetworkMask);
		stiTESTRESULT ();
	}
	else
	{
		// TODO: Need to format IPV6 Address

		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	if (fdSock >=0)
	{
		close (fdSock);
	}

	return hResult;
} /* end stiOSGetNetworkMask */


/* stiOSSetNetworkMask - see stiOSNet.h for header information*/
stiHResult stiOSSetNetworkMask (
	const std::string NetworkMask,
	EstiIpAddressType eIpAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	struct ifreq stIfr{};
	int fdSock = -1;		/* generic raw socket desc.  */
	int nResult;
	int nIpv6Prefix = 0;

	stiTESTCOND (!NetworkMask.empty (), stiRESULT_INVALID_PARAMETER);
#ifdef IPV6_ENABLED
	if (eIpAddressType == estiTYPE_IPV6)
	{
		nIpv6Prefix = atoi(NetworkMask.c_str ());
		stiTESTCOND (nIpv6Prefix<=128, stiRESULT_INVALID_PARAMETER);
		stiTESTCOND (nIpv6Prefix>0, stiRESULT_INVALID_PARAMETER);
	}
#endif // IPV6_ENABLED

	/* Create a socket for getting the address */
	fdSock = socket (eIpAddressType ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
	stiTESTCOND (fdSock >= 0, stiRESULT_ERROR);

	strcpy (stIfr.ifr_name, stiOSGetNICName().c_str());

	if (eIpAddressType == estiTYPE_IPV4)
	{
		/* Clear the buffer before setting */
		memset (stIfr.ifr_netmask.sa_data, 0, sizeof(stIfr.ifr_netmask.sa_data));

		/* Copy the IP address to the device info structure */
		memcpy (stIfr.ifr_netmask.sa_data, NetworkMask.c_str (), 4);

		/* Set the information in the device */
		nResult = ioctl(fdSock, SIOCSIFNETMASK, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}
	else
	{
		struct sockaddr_in6 socaddr6{};
		struct in6_ifreq ifr6{};

		memset(&socaddr6, 0, sizeof(struct sockaddr));
		socaddr6.sin6_family = AF_INET6;
		socaddr6.sin6_port = 0;

		// TODO: Get this from the settings somehow...
		nResult = inet_pton (AF_INET6, "::1", (void *)&socaddr6.sin6_addr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		memcpy((char *) &ifr6.ifr6_addr, (char *)&socaddr6.sin6_addr, sizeof(struct in6_addr));

		nResult = ioctl (fdSock, SIOGIFINDEX, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		ifr6.ifr6_ifindex = stIfr.ifr_ifindex;
		ifr6.ifr6_prefixlen = nIpv6Prefix;

		nResult = ioctl (fdSock, SIOCSIFADDR, &ifr6);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		stIfr.ifr_flags |= IFF_UP | IFF_RUNNING;

		nResult = ioctl (fdSock, SIOCSIFFLAGS, &stIfr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}

STI_BAIL:
	if (fdSock >=0)
	{
		close (fdSock);
	}

	return hResult;
} /* end stiOSSetNetworkMask */


/*!\brief Properly shutdown a socket
 *
 * This method properly shutdown a socket by first calling
 * shutdown and then ensuring the socket is flushed.  Without
 * doing this procedure proper handshaking between host and client
 * may not be done and data can be lost.
 */
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
			timeval TimeOut{};

			FD_ZERO (&FdSet);
			FD_SET (nFd, &FdSet);

			TimeOut.tv_sec = 0;
			TimeOut.tv_usec = 10000; // 0.01 seconds

			nResult = select (nFd + 1, &FdSet, nullptr, nullptr, &TimeOut);

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
}

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




/* end file stiOSNetLinux.c */
