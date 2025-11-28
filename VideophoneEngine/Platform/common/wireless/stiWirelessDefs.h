///*
///* \file IstiWireless.h
///* \brief Declaration of the Wireless interface
///*
///* \date Tue Oct 27, 2009
///*
///* Copyright (C) 2009 Sorenson Communications, Inc. All rights reserved. **
///*
#ifndef STIWIRELESSDEFS_H
#define STIWIRELESSDEFS_H

//
// Includes
//

#include <vector>
#include <string>

//
// Globals
//

#define SECURITY_NONE 		0
#define SECURITY_PSK		1
#define SECURITY_WEP		2
#define SECURITY_WPS		4
#define SECURITY_IEE8021X	8

///*
///* \brief this Struture contains the data necessary to communicate with the wireless driver
///*
///* The wireless driver operates on a series of IOCTL calls which allows the user space to
///* talk to the driver.
///*


struct  SstiSSIDListEntry
{
	char SSID[33];
    char MacAddress[6]; ///BSSID
	/// to be 0-3 w/ 3 being strongest
	int  SignalStrength;
	int Security;
	// Encryption Types
	bool bPrivacy;        /// Denotes use of WEP
	bool bNone;        	  ///
	bool bOPEN;        	  ///
	bool bWEP;        	  ///
	bool bTKIP;           ///
	bool bAESWRAP;        ///
	bool bAESCCMP;        ///
						  ///
	bool bWPA1;           /// WPA
	bool bWPA2;           /// WPA2
	bool bWPAPSK;         /// WPA
	bool bWPAEAP;         /// WPA
	bool bWPA2EAP;        /// WPA2
	bool bWPA2PSK;        /// WPA2PSK Preshared key, usually from WAP
	bool bCCKM;           /// Unused as it comes from a protocol not in use
	//char strAuth[32], strEncry[32];
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	bool bImmutable;      /// only connect and disconnect are valid calls for this AP
#endif
};


struct OpenWAP
{
	const char * pszSsid;
};


struct WEP
{
	const char * pszSsid;
	const char * pszKey;
	const char * pszKey_index; 
};

// used to describe a connection via Wpa PSK (personal shared key connection via wireless)
struct WpaPsk
{
	const char * pszSsid;
	const char * pszKey;
	const char * pszAuthentication;
	const char * pszEncryption;
};

// used to describe a connection via WpaEap (enterprise connection via wireless)
struct WpaEap
{
	const char *  pszSsid;
	const char *  pszIdentity;
	const char *  pszPrivateKeyPasswd;
};
///*
///* \brief This list is used to contain the list info from the structure above.
///*			 A Standard Template list data structure is used to contain the data.

//typedef std::list<SstiSSIDListEntry> WAPListInfo;
typedef std::vector<SstiSSIDListEntry> WAPListInfo;



#endif // STIWIRELESSDEFS_H
