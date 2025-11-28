///
/// \file CstiNetwork.cpp
/// \brief Definition of the Network class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved
///


#include "stiTrace.h"
#include "CstiNetwork.h"
#include <ifaddrs.h>
//#include <sys/types.h>
#include <netdb.h>


CstiNetwork::CstiNetwork ()
{
}


CstiNetwork::~CstiNetwork ()
{
}

std::string CstiNetwork::localIpAddressGet(
	EstiIpAddressType eIpAddressType,
	const std::string& destinationIpAddress) const
{
	struct ifaddrs *pAddrs; // linked list
	char szIPAddress[NI_MAXHOST];
	int nResult;
	std::string ReturnIpAddress = "";
	
	const char * OSAdapterNameCstring = "eth0";  // Ethernet adapter on Ubuntu AWS
	nResult = getifaddrs (&pAddrs);
	if (nResult == 0)
	{
		nResult = 1;
		for (ifaddrs *pAddr = pAddrs; pAddr; pAddr = pAddr->ifa_next)
		{
			if (pAddr->ifa_addr)
			{
				if (strcmp (pAddr->ifa_name, OSAdapterNameCstring) == 0)
				{
					if (eIpAddressType == estiTYPE_IPV4 && pAddr->ifa_addr->sa_family == AF_INET)
					{
						nResult = getnameinfo (pAddr->ifa_addr,
											   sizeof (struct sockaddr_in),
											   szIPAddress, sizeof(szIPAddress), NULL, 0, NI_NUMERICHOST);
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
				}

				if (nResult == 0)
				{
					ReturnIpAddress = szIPAddress;
					break;
				}
			}
		}
		
		freeifaddrs (pAddrs);
	}

	return ReturnIpAddress;
}
