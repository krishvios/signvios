// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "stiGUID.h"
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctime>
#include "stiOS.h"
#include <cstdlib>
#include <cstdio>

#define MAX_UUID_LEN 37


static char g_szMacAddr[13] = "";


static unsigned long RandomNumberGet()
{
	static bool bSeeded = false;
	
	if (!bSeeded)
	{
		srand (static_cast<unsigned int>(time (nullptr)));
		bSeeded = true;
	}
	
	return rand();
}


std::string stiGUIDGenerate()
{
	static int nSequence = 0;

	if (*g_szMacAddr == 0)
	{
		uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH];
		stiOSGetMACAddress(aun8MACAddress);
		stiMACAddressFormatWithoutColons(aun8MACAddress, g_szMacAddr);
	}

	nSequence = (nSequence + 1) % (1 << 12);
	time_t timestamp = time(nullptr);
	unsigned long ulRandom = RandomNumberGet();

	char uuidStr[MAX_UUID_LEN];

	int nLength = snprintf(uuidStr, MAX_UUID_LEN, "%08x-%04x-1%03x-%04x-%s", (unsigned)timestamp, (unsigned short)(ulRandom >> 16),
			nSequence, (unsigned short)((ulRandom & 0x3fff) | 0x8000), g_szMacAddr);

	if (nLength >= MAX_UUID_LEN)
	{
		uuidStr[0] = '\0';
	}

	return uuidStr;
}

std::string stiGUIDGenerateWithoutTime ()
{
	if (*g_szMacAddr == 0)
	{
		uint8_t aun8MACAddress[MAX_MAC_ADDRESS_LENGTH];
		memset (aun8MACAddress, 0, sizeof (aun8MACAddress));
		stiOSGetMACAddress(aun8MACAddress);
		stiMACAddressFormatWithoutColons(aun8MACAddress, g_szMacAddr);
	}
	
	char uuidStr[MAX_UUID_LEN];

	int nLength = snprintf(uuidStr, MAX_UUID_LEN, "00000000-0000-1000-8000-%s", g_szMacAddr);

	if (nLength >= MAX_UUID_LEN)
	{
		uuidStr[0] = '\0';
	}
	
	return uuidStr;
		
}

