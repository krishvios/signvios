///
///  \file CstiWireless.cpp
///  \brief Declaration of the Wireless interface
///
///    Class Name:	CstiWireless
///
///    File Name:	CstiWireless.cpp
///
///
///  \date Tue Oct 27, 2009
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2000 – 2015 Sorenson Communications, Inc. -- All rights reserved
///

//
// Includes
//
#include "CstiWireless.h"
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include <stdlib.h>
#include "stiWirelessDefs.h"
using namespace std;
#include "rtmp_type.h"
#include "oid.h"
//#include "wlib.h"		// Header

//
// Constants
//

// -------------------------- VARIABLES --------------------------

//
//  Meta-data about all the standard Wireless Extension request we
//  know about.

// -------------------------- CONSTANTS --------------------------

// Type of headers we know about (basically union iwreq_data)
#define IW_HEADER_TYPE_NULL	0	// Not available
#define IW_HEADER_TYPE_CHAR	2	// char [IFNAMSIZ]
#define IW_HEADER_TYPE_UINT	4	// __u32
#define IW_HEADER_TYPE_FREQ	5	// struct iw_freq
#define IW_HEADER_TYPE_ADDR	6	// struct sockaddr
#define IW_HEADER_TYPE_POINT	8	// struct iw_point
#define IW_HEADER_TYPE_PARAM	9	// struct iw_param
#define IW_HEADER_TYPE_QUAL	10	// struct iw_quality

// Handling flags
// Most are not implemented. I just use them as a reminder of some
//  cool features we might need one day ;-)
#define IW_DESCR_FLAG_NONE	0x0000	// Obvious
// Wrapper level flags
#define IW_DESCR_FLAG_DUMP	0x0001	// Not part of the dump command
#define IW_DESCR_FLAG_EVENT	0x0002	// Generate an event on SET
#define IW_DESCR_FLAG_RESTRICT	0x0004	// GET : request is ROOT only
// SET : Omit payload from generated iwevent
#define IW_DESCR_FLAG_NOMAX	0x0008	// GET : no limit on request size
// Driver level flags
#define IW_DESCR_FLAG_WAIT	0x0100	// Wait for driver event


//  Structure used for parsing event streams, such as Wireless Events
//  and scan results
typedef struct stream_descr
{
	char *    end;		  //  End of the stream
	char *    current;	  //  Current event in stream of events
	char *    value;	  //  Current value in event
} stream_descr;


typedef struct wscan_state
{
	// State
	int           nAp_num;	  // Access Point number 1->N
	int           nVal_index;  // Value in table 0->(N-1)
} wscan_state;



typedef struct iw_statistics   iwstats;
typedef struct iw_range     	iwrange;
typedef struct iw_param     	iwparam;
typedef struct iw_freq      	iwfreq;
typedef struct iw_quality   	iwqual;
typedef struct iw_priv_args 	iwprivargs;
typedef struct sockaddr     	sockaddr;

struct iw_ioctl_description
{
	__u8    header_type;	// NULL, SCI_point or other
	__u8    token_type;		// Future
	__u16   token_size;		// Granularity of payload
	__u16   min_tokens;		// Min acceptable token number
	__u16   max_tokens;		// Max acceptable token number
	__u32   flags;			// Special handling of the request
};


static const struct iw_ioctl_description standard_ioctl_descr[] = {
  /* 00 [SIOCSIWCOMMIT		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL, 0,0,0,0,0},
  /* 01 [SIOCGIWNAME		- SIOCIWFIRST] = */{IW_HEADER_TYPE_CHAR, 0,0,0,0,IW_DESCR_FLAG_DUMP},
  /* 02 [SIOCSIWNWID		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,0,0,0,0,IW_DESCR_FLAG_EVENT},
  /* 03 [SIOCGIWNWID		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,0,0,0,0,IW_DESCR_FLAG_DUMP},
  /* 04 [SIOCSIWFREQ		- SIOCIWFIRST] = */{IW_HEADER_TYPE_FREQ, 0,0,0,0,IW_DESCR_FLAG_EVENT},
  /* 05 [SIOCGIWFREQ		- SIOCIWFIRST] = */{IW_HEADER_TYPE_FREQ, 0,0,0,0,IW_DESCR_FLAG_DUMP},
  /* 06 [SIOCSIWMODE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_UINT, 0,0,0,0,IW_DESCR_FLAG_EVENT},
  /* 07 [SIOCGIWMODE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_UINT, 0,0,0,0,IW_DESCR_FLAG_DUMP},
  /* 08 [SIOCSIWSENS		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM},
  /* 09 [SIOCGIWSENS		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM},
  /* 10 [SIOCSIWRANGE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL},
  /* 11 [SIOCGIWRANGE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT, 0,1,0, sizeof(struct iw_range),IW_DESCR_FLAG_DUMP},
  /* 12 [SIOCSIWPRIV		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL},
  /* 13 [SIOCGIWPRIV		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL},// (handled directly by us)
  /* 14 [SIOCSIWSTATS		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL},
  /* 15 [SIOCGIWSTATS		- SIOCIWFIRST] = */{IW_HEADER_TYPE_NULL,0,0,0,0, IW_DESCR_FLAG_DUMP,},// (handled directly by us)
  /* 16 [SIOCSIWSPY			- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, sizeof(struct sockaddr),0,IW_MAX_SPY,},
  /* 17 [SIOCGIWSPY			- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, sizeof(struct sockaddr) + sizeof(struct iw_quality),0, IW_MAX_SPY,},
  /* 18 [SIOCSIWTHRSPY		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, sizeof(struct iw_thrspy), 1,1,},
  /* 19 [SIOCGIWTHRSPY		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, sizeof(struct iw_thrspy), 1,1,},
  /* 20 [SIOCSIWAP			- SIOCIWFIRST] = */{IW_HEADER_TYPE_ADDR,},
  /* 21 [SIOCGIWAP			- SIOCIWFIRST] = */{IW_HEADER_TYPE_ADDR,0,0,0,0, IW_DESCR_FLAG_DUMP,},
  /* 22 [SIOCSIWMLME		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1, sizeof(struct iw_mlme),sizeof(struct iw_mlme),},
  /* 23 [SIOCGIWAPLIST		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, sizeof(struct sockaddr) +sizeof(struct iw_quality),IW_MAX_AP, IW_DESCR_FLAG_NOMAX,},
  /* 24 [SIOCSIWSCAN		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,sizeof(struct iw_scan_req),},
  /* 25 [SIOCGIWSCAN		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,IW_SCAN_MAX_DATA,IW_DESCR_FLAG_NOMAX,},
  /* 26 [SIOCSIWESSID		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,IW_ESSID_MAX_SIZE + 1,IW_DESCR_FLAG_EVENT,},
  /* 27 [SIOCGIWESSID		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,IW_ESSID_MAX_SIZE + 1,IW_DESCR_FLAG_DUMP,},
  /* 28 [SIOCSIWNICKN		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,IW_ESSID_MAX_SIZE + 1,},
  /* 29 [SIOCGIWNICKN		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0,1,0,IW_ESSID_MAX_SIZE + 1,},
  /* 30 needed as spacer                     */{IW_HEADER_TYPE_NULL,0,0,0,0,0},
  /* 31 needed as spacer                     */{IW_HEADER_TYPE_NULL,0,0,0,0,0},
  /* 32 [SIOCSIWRATE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 33 [SIOCGIWRATE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 34 [SIOCSIWRTS			- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 35 [SIOCGIWRTS			- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 36 [SIOCSIWFRAG		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 37 [SIOCGIWFRAG		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 38 [SIOCSIWTXPOW		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 39 [SIOCGIWTXPOW		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 40 [SIOCSIWRETRY		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 41 [SIOCGIWRETRY		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 42 [SIOCSIWENCODE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,0,IW_ENCODING_TOKEN_MAX,IW_DESCR_FLAG_EVENT | IW_DESCR_FLAG_RESTRICT,},
  /* 43 [SIOCGIWENCODE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,0,IW_ENCODING_TOKEN_MAX,IW_DESCR_FLAG_DUMP | IW_DESCR_FLAG_RESTRICT,},
  /* 44 [SIOCSIWPOWER		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 45 [SIOCGIWPOWER		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 46 [SIOCSIWMODUL		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 47 [SIOCGIWMODUL		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 48 [SIOCSIWGENIE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,0,IW_GENERIC_IE_MAX,},
  /* 49 [SIOCGIWGENIE		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,0,IW_GENERIC_IE_MAX,},
  /* 50 [SIOCSIWAUTH		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 51 [SIOCGIWAUTH		- SIOCIWFIRST] = */{IW_HEADER_TYPE_PARAM,},
  /* 52 [SIOCSIWENCODEEXT 	- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,sizeof(struct iw_encode_ext), sizeof(struct iw_encode_ext) + IW_ENCODING_TOKEN_MAX,},
  /* 53 [SIOCGIWENCODEEXT 	- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,sizeof(struct iw_encode_ext),sizeof(struct iw_encode_ext) +IW_ENCODING_TOKEN_MAX,},
  /* 54 [SIOCSIWPMKSA 		- SIOCIWFIRST] = */{IW_HEADER_TYPE_POINT,0, 1,sizeof(struct iw_pmksa), sizeof(struct iw_pmksa),},
};
static const unsigned int standard_ioctl_num = (sizeof(standard_ioctl_descr) /
						sizeof(struct iw_ioctl_description));

//
//  Meta-data about all the additional standard Wireless Extension events
//  we know about.

static const struct iw_ioctl_description standard_event_descr[] = {
  /* 0 [IWEVTXDROP	- IWEVFIRST] = */ 				{IW_HEADER_TYPE_ADDR,},
  /* 1 [IWEVQUAL	- IWEVFIRST] = */ 				{IW_HEADER_TYPE_QUAL,},
  /* 2 [IWEVCUSTOM	- IWEVFIRST] = */ 				{IW_HEADER_TYPE_POINT,0, 1,0,IW_CUSTOM_MAX,},
  /* 3 [IWEVREGISTERED	- IWEVFIRST] = */ 			{IW_HEADER_TYPE_ADDR,},
  /* 4 [IWEVEXPIRED	- IWEVFIRST] = */ 				{IW_HEADER_TYPE_ADDR,},
  /* 5 [IWEVGENIE	- IWEVFIRST] = */ 				{IW_HEADER_TYPE_POINT,0, 1,0,IW_GENERIC_IE_MAX,},
  /* 6 [IWEVMICHAELMICFAILURE	- IWEVFIRST] = */ 	{IW_HEADER_TYPE_POINT,0, 1,0,sizeof(struct iw_michaelmicfailure),},
  /* 7 [IWEVASSOCREQIE	- IWEVFIRST] = */  			{IW_HEADER_TYPE_POINT,0, 1,0,IW_GENERIC_IE_MAX,},
  /* 8 [IWEVASSOCRESPIE	- IWEVFIRST] = */  			{IW_HEADER_TYPE_POINT,0, 1,0,IW_GENERIC_IE_MAX,},
  /* 9 [IWEVPMKIDCAND	- IWEVFIRST] = */  			{IW_HEADER_TYPE_POINT,0, 1,0,sizeof(struct iw_pmkid_cand),},
};

static const unsigned int standard_event_num = (sizeof(standard_event_descr) / sizeof(struct iw_ioctl_description));

// Size (in bytes) of various events
static const int event_type_size[] = {
	IW_EV_LCP_PK_LEN,	// IW_HEADER_TYPE_NULL
	0,
	IW_EV_CHAR_PK_LEN,	// IW_HEADER_TYPE_CHAR
	0,
	IW_EV_UINT_PK_LEN,	// IW_HEADER_TYPE_UINT
	IW_EV_FREQ_PK_LEN,	// IW_HEADER_TYPE_FREQ
	IW_EV_ADDR_PK_LEN,	// IW_HEADER_TYPE_ADDR
	0,
	IW_EV_POINT_PK_LEN,	// Without variable payload
	IW_EV_PARAM_PK_LEN,	// IW_HEADER_TYPE_PARAM
	IW_EV_QUAL_PK_LEN,	// IW_HEADER_TYPE_QUAL
};


///  \brief These constants are used in authentication flags
/// 	These are the OUIs or Organizational Unique Identifier
/// 	They are used as flags when we get a list from the
///     driver of the WAPs in the area. They tell us about
/// 	the security used for each WAP
///
const unsigned long WPA_OUI_TYPE = 0x01F25000;
const unsigned long WPA_OUI = 0x00F25000;
const unsigned long WPA_OUI_1 = 0x00030000;
const unsigned long WPA2_OUI = 0x00AC0F00;
const unsigned long CISCO_OUI = 0x00964000;

#define DATA_BUFFER_SIZE 4096*2

/////////////////////////////////////////////////////////////////////////////////
///   \brief  CstiWireless::WapIsConnected()
///
///
/// Description: Unused, will be removed upon move to BSP layer
///
/// Abstract:
///
/// \return
/// Returns:
///	   stiRESULT_SUCCESS
///
stiHResult CstiWireless::WAPIsConnected(
	SstiSSIDListEntry *) /// SstiSSIDListEntry list entry contains data in order to allow
{
	return (stiRESULT_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////////
///   \brief  CstiWireless::WirelessAvailable()
///
///
/// Description:	Tests to let us know if the wireless is setup and operating
///
///
/// \return
/// Returns:
/// 	Success:    stiRESULT_SUCCESS
/// 	Failure:    stiRESULT_DEVICE_OPEN_FAILED
///
stiHResult  CstiWireless::WirelessSignalStrengthGet(int *nStrength)
{
	char str[2000];
	char cPercent[4];
	int nVal = 0;

	memset( cPercent, 0, sizeof(cPercent));
	ifstream pFile("/proc/net/wireless");
	pFile.getline(str, sizeof(str));
	pFile.getline(str,sizeof(str));
	pFile.getline(str,sizeof(str));
	strncpy(cPercent,&str[14],3);
	nVal = atoi(cPercent);

	if (nVal < 25)
	{
		*nStrength = 0;
	}
	else if (nVal < 50)
	{
		*nStrength = 1;
	}
	else if (nVal < 75)
	{
		*nStrength = 2;
	}
	else if (nVal >= 75)
	{
		*nStrength = 3;
	}

	//printf("\"%s\"\n",cPercent);
	//printf("\"%d\"\n",nVal);
	//printf("\"%d\"\n",nStrength);

	pFile.close();

	return stiRESULT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
///   \brief  CstiWireless::WirelessAvailable()
///
///
/// Description:	Tests to let us know if the wireless is setup and operating
///
///
/// \return
/// Returns:
/// 	Success:    stiRESULT_SUCCESS
/// 	Failure:    stiRESULT_DEVICE_OPEN_FAILED
///
stiHResult  CstiWireless::WirelessAvailable()
{


	int skfd; /* generic raw socket desc. */

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
	{
		return (stiRESULT_DEVICE_OPEN_FAILED);
	}

	int MediaState = 0;
	struct iwreq wrq;

	stiHResult retVal;

    strcpy(wrq.ifr_name, "ra0");
    wrq.u.data.length = sizeof(int);
    wrq.u.data.pointer = &MediaState;
    wrq.u.data.flags = OID_GEN_MEDIA_CONNECT_STATUS;

    if(ioctl(skfd, RT_PRIV_IOCTL, &wrq) < 0)
    {
    	fprintf(stderr, "Interface doesn't accept private ioctl...\n");
		close(skfd);
		return (stiRESULT_DEVICE_OPEN_FAILED);
    }



	if(MediaState == NdisMediaStateConnected)
	{
	   retVal = stiRESULT_WIRELESS_AVAILABLE;
	}
	else
	{
	   retVal = stiRESULT_WIRELESS_NOT_AVAILABLE;
	}

	close(skfd);
	return (retVal);

}


//
//  Parse, and display the results of a WPA or WPA2 IE or Information Element.
//
//
void ParseIE(unsigned char * iebuf, int buflen, SstiSSIDListEntry *listItem)
{
	int           ielen = iebuf[1] + 2;
	int           offset = 2; // Skip the IE id, and the length.
	unsigned char     wpa1_oui[3] = {0x00, 0x50, 0xf2};
	unsigned char     wpa2_oui[3] = {0x00, 0x0f, 0xac};
	unsigned char *   pWpa_oui;
	int           i;
	//uint16_t      ver = 0;
	uint16_t      cnt = 0;

	if(ielen > buflen)
	{
		ielen = buflen;
	}

	switch(iebuf[0])
	{
		case 0x30:			// WPA2
			if(ielen < 4) 	// Check if we have enough data
			{
				return;
			}
			pWpa_oui = wpa2_oui;
			break;

		case 0xdd:			// WPA
			pWpa_oui = wpa1_oui;
			// Not all IEs that start with 0xdd are WPA.
			// So check that the OUI is valid. Note : offset==2
			if((ielen < 8) || (memcmp(&iebuf[offset], pWpa_oui, 3) != 0) || (iebuf[offset + 3] != 0x01))
			{
				return;
			}
			// Skip the OUI type
			offset += 4;
			break;
		default:
			return;
	}

	// Pick version number (little endian)
	//int version = iebuf[offset] | (iebuf[offset + 1] << 8);

	offset += 2;

	if(iebuf[0] == 0xdd)
	{
		//printf("  WPA Version %d\n", version);
		listItem->bWPA1 = true;
	}
	if(iebuf[0] == 0x30)
	{
		//printf("  WPA2 Version %d\n", version);
		listItem->bWPA2 = true;
	}


	// Check if we are done
	if(ielen < (offset + 4))
	{
		// We have a short IE.  So we should assume TKIP/TKIP.
		//printf("  Group Cipher : TKIP\n");
		//printf("  Pairwise Cipher : TKIP\n");
		listItem->bTKIP = true;
		return;
	}

	// Next we have our group cipher.
	if(memcmp(&iebuf[offset], pWpa_oui, 3) == 0)
	{
		//printf("  Group Cipher : ");
		switch(iebuf[offset+3])
		{
			case 0:
				//printf("none");
				//listItem->bWPANONE  = true;
				listItem->bNone = true;
				break;
			case 1:
				//printf("WEP-40");
				listItem->bWEP = true;
				break;
			case 2:
				//printf("TKIP");
				listItem->bTKIP = true;
				break;
			case 3:
				//printf("WRAP");
				listItem->bAESWRAP = true;
				break;
			case 4:
				//printf("CCMP");
				listItem->bAESCCMP = true;
				break;
			case 5:
				//printf("WEP-104");
				listItem->bWEP = true;
				break;
			default:
				break;
		}
		//printf("\n");
	}
	offset += 4;

	// Check if we are done
	if(ielen < (offset + 2))
	{
		// We don't have a pairwise cipher, or auth method. Assume TKIP.
		//printf("  Pairwise Ciphers : TKIP\n");
		listItem->bTKIP = true;
		return;
	}

	// Otherwise, we have some number of pairwise ciphers.
	cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
	offset += 2;
	//printf("  Pairwise Ciphers (%d) :", cnt);

	if(ielen < (offset + 4*cnt))
	{
		return;
	}

	for(i = 0; i < cnt; i++)
	{
		if(memcmp(&iebuf[offset], pWpa_oui, 3) == 0)
		{
			switch(iebuf[offset+3])
			{
				case 0:
					//printf("none");
					//listItem->bWPANONE  = true;
					listItem->bNone  = true;
					break;
				case 1:
					//printf("WEP-40");
					listItem->bWEP = true;
					break;
				case 2:
					//printf("TKIP");
					listItem->bTKIP = true;
					break;
				case 3:
					//printf("WRAP");
					listItem->bAESWRAP = true;
					break;
				case 4:
					//printf("CCMP");
					listItem->bAESCCMP = true;
					break;
				case 5:
					//printf("WEP-104");
					listItem->bWEP = true;
					break;
				default:
					break;
			}

		}
		offset+=4;
	}
	//printf("\n");

	// Check if we are done
	if(ielen < (offset + 2))
	{
		return;
	}

	// Now, we have authentication suites.
	cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
	offset += 2;
	//printf("  Authentication Suites (%d) :", cnt);

	if(ielen < (offset + 4*cnt))
	{
		return;
	}

	for(i = 0; i < cnt; i++)
	{
		if(memcmp(&iebuf[offset], pWpa_oui, 3) == 0)
		{
			switch(iebuf[offset+3])
			{
				case 0:
					//printf("none");
					break;
				case 1:
					//printf("802.1x");
					listItem->bWPA2EAP = true;
					break;
				case 2:
					//printf("PSK");
					listItem->bWPA2PSK = true;
					break;
				default:
					break;
			}
		}
		offset+=4;
	}
	//printf("\n");

	// Check if we are done
	if(ielen < (offset + 1))
	{
		return;
	}

	//   Otherwise, we have capabilities bytes.
	//  For now, we only care about preauth which is in bit position 1 of the
	//  first byte.  (But, preauth with WPA version 1 isn't supposed to be
	//  allowed.) 8-)
	if(iebuf[offset] & 0x01)
	{
	   printf(" Preauthentication Supported\n");
	}
}



//
//  Process a generic IE and display the info in human readable form
//  for some of the most interesting ones.
//  For now, we only decode the WPA IEs.
//
void FindIE(unsigned char * buffer, int buflen, SstiSSIDListEntry * listItem)
{
	int offset = 0;

	// Loop on each IE, each IE is minimum 2 bytes
	while(offset <= (buflen - 2))
	{
		//printf("                    IE: ");

		// Check IE type
		switch(buffer[offset])
		{
			case 0xdd:	// WPA1 (and other)
			case 0x30:	// WPA2
				ParseIE(buffer + offset, buflen, listItem);
				break;
			default:
			   ;
		}
		// Skip over this IE to the next one in the list.
		offset += buffer[offset+1] + 2;
	}
}



stiHResult CstiWireless::WAPListGet(WAPListInfo *pListRef)	/// Reference to the WAP list
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int       		skfd = 0;					// socket handle
	struct          iwreq          wrq;
	struct          iw_scan_req    scanopt;			//  Options for 'set'
	int             nScanflags = 0;					//  Flags for scan
	unsigned char * buffer = NULL;					//  Results
	size_t          buflen = IW_SCAN_MAX_DATA * 4;	//  Return data buffer size
	struct 			iw_range range;
	//int           nHas_range;
	struct 			timeval  tv;					//  Select timeout
	int             nTimeout = 15000000;			//  15s

	range.we_version_compiled = 17;
	
	// Create a channel to the NET kernel.
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		return(stiRESULT_DEVICE_OPEN_FAILED);
	}


	// Init timeout value -> 250ms between set and first get
	tv.tv_sec = 0;
	tv.tv_usec = 550000;

	// Clean up set args
	memset(&scanopt, 0, sizeof(scanopt));

	wrq.u.data.pointer = NULL;
	wrq.u.data.flags = 0;
	wrq.u.data.length = 0;

	// Initiate Scanning
	strncpy(wrq.ifr_name, "ra0", IFNAMSIZ); //Set device name
	int ret = ioctl(skfd, SIOCSIWSCAN, &wrq);
	if( ret < 0)
	{
		if((errno != EPERM) || (nScanflags != 0))
		{
			fprintf(stderr, "%-8.16s  Interface doesn't support scanning : %s\n\n", "ra0", strerror(errno));
			stiTHROW (stiRESULT_ERROR);
		}
		//  If we don't have the permission to initiate the scan, we may
		//  still have permission to read left-over results.
		//  But, don't wait !!!
		tv.tv_usec = 0;
	}
	nTimeout -= tv.tv_usec;

	// Forever
	while(1)
	{
		fd_set rfds;		//   File descriptors for select
		int    last_fd = 0;		//   Last fd
		int    retVal = 0;

		// We must re-generate rfds each time
		FD_ZERO(&rfds);
		last_fd = -1;
		// In here, add the rtnetlink fd in the list
		// Wait until something happens
		retVal = select(last_fd + 1, &rfds, NULL, NULL, &tv);

		// Check if there was an error
		if(retVal < 0)
		{
			if(errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			//Unhandled signal - exit
			stiTHROW (stiRESULT_ERROR);
		}
		else if(retVal == 0) // Check if there was a timeout
		{
			buffer = (unsigned char * ) realloc(buffer, buflen);

			// Try to read the results
			wrq.u.data.pointer = buffer;
			wrq.u.data.flags = 0;
			wrq.u.data.length = buflen;
			if(ioctl(skfd, SIOCGIWSCAN, &wrq) < 0)	// error, not enough memory
			{
				// Check if buffer was too small (WE-17 only)
				if((errno == E2BIG) && (range.we_version_compiled > 16))
				{
					//  Some driver may return very large scan results
					// Check if the driver gave us any hints.
					if(wrq.u.data.length > buflen)
					{
						buflen = wrq.u.data.length;
					} else
					{
						buflen *= 2;
					}

					stiTHROW (stiRESULT_ERROR);
				}

				// Check if results not available yet
				if(errno == EAGAIN)
				{
					// Restart timer for only 100ms
					tv.tv_sec = 0;
					tv.tv_usec = 100000;
					nTimeout -= tv.tv_usec;
					if(nTimeout > 0)
					{
						continue;	// Try again
					}
				}

				// Bad error
				free(buffer);
				stiTHROW (stiRESULT_ERROR);
			} else
			{
				// We have the results, go to process them
				break;
			}
		} // if ret = 0
	} // while 1

	if(wrq.u.data.length) // we got something back
	{
		struct iw_event       iwe;
		struct stream_descr   stream;
		struct wscan_state	state = { 1, 0 };
		int    ret = 0;
		int    bSsidOk = false;
		SstiSSIDListEntry * listItem = NULL;

		// Set things up
		stream.current  = (char*)buffer;
		stream.end = (char*)(buffer + wrq.u.data.length);
		stream.value = NULL;

		do
		{
			ret = SciExtract_event_stream(&stream, &iwe, range.we_version_compiled);
			if(ret > 0)
			{
				//print_scanning_token(&stream, &iwe, &state,&range, nHas_range);
				//print_scanning_token( &iwe, &state, );
				//listItem.Privacy = pBssid->Privacy;

				// Now, let's decode the event
				switch(iwe.cmd)
				{
					case SIOCGIWENCODE:
						{
							if (bSsidOk)
							{
								//unsigned char   key[IW_ENCODING_TOKEN_MAX];
								if(iwe.u.data.pointer)
								{
									//memcpy(key, iwe.u.data.pointer, iwe.u.data.length);
								} else
								{
									iwe.u.data.flags |= IW_ENCODE_NOKEY;
								}
								// printf(" Encryption key:");
								if(iwe.u.data.flags & IW_ENCODE_DISABLED)
								{
									listItem->bPrivacy = false;
								}
								else if(iwe.u.data.flags & IW_ENCODE_NOKEY)
								{
									if(iwe.u.data.length <= 0)
									{
										listItem->bPrivacy = true;
									}
								}
								else if(iwe.u.data.flags & IW_ENCODE_OPEN)
								{
									listItem->bOPEN = true;
								}
							}
						}
						break;
					case SIOCGIWAP:
						//printf("\n\nWAP %02d - Address: %s\n", state->nAp_num, iw_saether_ntop(&event->u.ap_addr, buffer));
						state.nAp_num++;
						if(bSsidOk)
						{
							pListRef->push_back(*listItem);
							bSsidOk = false;
							delete listItem;
							listItem = NULL;
						}
						break;
					case SIOCGIWESSID:
						{
							char essid[IW_ESSID_MAX_SIZE+1];
							memset(essid, '\0', sizeof(essid));
							if((iwe.u.essid.pointer) && (iwe.u.essid.length))
							{
								memcpy(essid, iwe.u.essid.pointer, iwe.u.essid.length);
							}
							if(iwe.u.essid.flags)
							{
								if(essid[0])
								{
									//printf("\n\n**ESSID:\"%s\"\n", essid);
									listItem = new SstiSSIDListEntry;
									memset(listItem,0,sizeof(SstiSSIDListEntry));
									strcpy(listItem->SSID, essid );
									//pListRef->push_back(*listItem);
									bSsidOk = 1;
								}
								else
								{
									bSsidOk = 0;
								}

							}
						}
						break;
					case IWEVQUAL:
						{
							if(bSsidOk)
							{
								//printf("  Quality:%d\n", iwe.u.qual.qual);

								if (iwe.u.qual.qual < 25)
								{
									listItem->SignalStrength = 0;
								}
								else if (iwe.u.qual.qual < 50)
								{
									listItem->SignalStrength = 1;
								}
								else if (iwe.u.qual.qual < 75)
								{
									listItem->SignalStrength = 2;
								}
								else if (iwe.u.qual.qual >= 75)
								{
									listItem->SignalStrength = 3;
								}
							}
						}
						break;
					case IWEVGENIE:
						// Informations Elements are complex, let's do only some of them
						if(bSsidOk)
						FindIE((unsigned char *)iwe.u.data.pointer, iwe.u.data.length, listItem);
						break;
				}	 // switch(event->cmd)

			}
		}while(ret > 0);
		//printf("\n");
	}

	free(buffer);

	// Close the socket.
	close(skfd);

STI_BAIL:
	return(stiRESULT_DEVICE_OPEN_FAILED);

}




//
//
//  Extract the next event from the event stream.
//
int SciExtract_event_stream(struct stream_descr *   stream,	    // Stream of events
							struct iw_event *   	iwe,	    // Extracted event
							int    					we_version)
{
	const struct iw_ioctl_description *   descr = NULL;
	int       event_type = 0;
	unsigned int  event_len = 1;	  // Invalid
	char *    pointer;
	// Don't "optimise" the following variable, it will crash
	unsigned  cmd_index;	  // *MUST* be unsigned

	// Check for end of stream
	if((stream->current + IW_EV_LCP_PK_LEN) > stream->end)
	{
		return(0);
	}

	//  Extract the event header (to get the event id).
	//  Note : the event may be unaligned, therefore copy...
	memcpy((char *) iwe, stream->current, IW_EV_LCP_PK_LEN);

	// Check invalid events
	if(iwe->len <= IW_EV_LCP_PK_LEN)
	{
		return(-1);
	}

	// Get the type and length of that event
	if(iwe->cmd <= SIOCIWLAST)
	{
		cmd_index = iwe->cmd - SIOCIWFIRST;
		if(cmd_index < standard_ioctl_num)
		{
			descr = &(standard_ioctl_descr[cmd_index]);
		}
	} else
	{
		cmd_index = iwe->cmd - IWEVFIRST;
		if(cmd_index < standard_event_num)
		{
			descr = &(standard_event_descr[cmd_index]);
		}
	}
	if(descr != NULL)
	{
		event_type = descr->header_type;
	}
	// Unknown events -> event_type=0 => IW_EV_LCP_PK_LEN
	event_len = event_type_size[event_type];
	// Fixup for earlier version of WE
	//if((we_version <= 18) && (event_type == IW_HEADER_TYPE_POINT))
	//{
	//	event_len += IW_EV_POINT_OFF;
	//}

	// Check if we know about this event
	if(event_len <= IW_EV_LCP_PK_LEN)
	{
		// Skip to next event
		stream->current += iwe->len;
		return(2);
	}
	event_len -= IW_EV_LCP_PK_LEN;

	// Set pointer on data
	if(stream->value != NULL)
	{
		pointer = stream->value;			// Next value in event
	} else
	{
		pointer = stream->current + IW_EV_LCP_PK_LEN;	// First value in event
	}

	// Copy the rest of the event (at least, fixed part)
	if((pointer + event_len) > stream->end)
	{
		// Go to next event
		stream->current += iwe->len;
		return(-2);
	}
	// Fixup for WE-19 and later : pointer no longer in the stream
	// Beware of alignement. Dest has local alignement, not packed
	//if((we_version > 18) && (event_type == IW_HEADER_TYPE_POINT))
	if(event_type == IW_HEADER_TYPE_POINT)
	{
		memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF,pointer, event_len);
	}// else
	//{
	//	memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);
	//}

	// Skip event in the stream
	pointer += event_len;

	// Special processing for SCI_point events
	if(event_type == IW_HEADER_TYPE_POINT)
	{
		//printf("Warning! IW_HEADER_TYPE_POINT\n");
		// Check the length of the payload
		unsigned int  extra_len = iwe->len - (event_len + IW_EV_LCP_PK_LEN);
		if(extra_len > 0)
		{
			// Set pointer on variable part
			iwe->u.data.pointer = pointer;

			// Check that we have a descriptor for the command
			if(descr == NULL)
			{
				// Can't check payload -> unsafe...
				iwe->u.data.pointer = NULL;	  // Discard paylod
			}
			else
			{
				// Those checks are actually pretty hard to trigger,
				// because of the checks done in the kernel...

				unsigned int  token_len = iwe->u.data.length * descr->token_size;

				//  Ugly fixup for alignement issues.
				//  If the kernel is 64 bits and userspace 32 bits,
				//  we have an extra 4+4 bytes.
				//  Fixing that in the kernel would break 64 bits userspace.
			    if((token_len != extra_len) && (extra_len >= 4))
			    {
					printf("Warning! 64bit packets coming from wireless driver 1\n");
			    	__u16     alt_dlen = *((__u16 *) pointer);
			    	unsigned int  alt_token_len = alt_dlen * descr->token_size;
			    	if((alt_token_len + 8) == extra_len)
			    	{
			    		// Ok, let's redo everything
			    		pointer -= event_len;
			    		pointer += 4;
			    		// Dest has local alignement, not packed
			    		memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF,pointer, event_len);
			    		pointer += event_len + 4;
			    		iwe->u.data.pointer = pointer;
			    		token_len = alt_token_len;
			    	}
			    }

				// Discard bogus events which advertise more tokens than
				//  what they carry...
				if(token_len > extra_len)
				{
					iwe->u.data.pointer = NULL;	// Discard paylod
				}
				// Check that the advertised token size is not going to
				//  produce buffer overflow to our caller...
				if((iwe->u.data.length > descr->max_tokens) && !(descr->flags & IW_DESCR_FLAG_NOMAX))
				{
					iwe->u.data.pointer = NULL;	// Discard paylod
				}
				// Same for underflows...
				if(iwe->u.data.length < descr->min_tokens)
				{
					iwe->u.data.pointer = NULL;	// Discard paylod
				}
			}
		}
		else
		{
			// No data
			iwe->u.data.pointer = NULL;
		}

		// Go to next event
		stream->current += iwe->len;
	} else
	{
		// Ugly fixup for alignement issues.
		//  If the kernel is 64 bits and userspace 32 bits,
		//  we have an extra 4 bytes.
		//  Fixing that in the kernel would break 64 bits userspace.
		if((stream->value == NULL)&& ((((iwe->len - IW_EV_LCP_PK_LEN) % event_len) == 4)|| ((iwe->len == 12) && ((event_type == IW_HEADER_TYPE_UINT)|| (event_type == IW_HEADER_TYPE_QUAL))) ))
		{
			printf("Warning! 64bit packets coming from wireless driver 2\n");
			pointer -= event_len;
			pointer += 4;
			// Beware of alignement. Dest has local alignement, not packed
			memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);
			pointer += event_len;
		}


		// Is there more value in the event ?
		if((pointer + event_len) <= (stream->current + iwe->len))
		{
			// Go to next value
			stream->value = pointer;
		} else
		{
			// Go to next event
			stream->value = NULL;
			stream->current += iwe->len;
		}
	}
	return(1);
}


/////////////////////////////////////////////////////////////////////////////////
///
///
///  WirelessParamCmdSet(char * pszCommand)
///	 Used to set commands to the wireless
///
///
#define WIRELESSIOCMD 0x8BE2;
#define WPA_SUPPLICANT_CONF 	PERSISTENT_DATA "/networking/wpa_supplicant.conf"

stiHResult  CstiWireless::WirelessParamCmdSet(char * pszCommand)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int skfd; /// generic raw socket desc.

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
	{
		return (stiRESULT_DEVICE_OPEN_FAILED);
	}

	struct 	iwreq  wrq;

	strncpy(wrq.ifr_name, "ra0", 4);
	wrq.u.data.length = strlen(pszCommand);
	wrq.u.data.pointer = pszCommand;
	wrq.u.data.flags = 0;

	//printf("wrq.u.data.pointer %s\n",pszCommand);
	// Perform the private ioctl
	int cmd = WIRELESSIOCMD;
	if(ioctl(skfd, cmd , &wrq) < 0)
	{
		fprintf(stderr, "Interface doesn't accept private ioctl...\n");
		fprintf(stderr, "(%X): %s\n", cmd, strerror(errno));
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	close(skfd);
	return (hResult);
}

stiHResult CstiWireless::Connect(const char * pszESSID,int nTestConnection)
{
	int retVal = -1;
	stiHResult hResult = stiRESULT_SUCCESS;
	char line[100];
	FILE *pp;
	int iLoop = 0;

	string script("network={\n");
	script += "\tssid=\"";
	if (pszESSID)
	{
		string sTmp = pszESSID;
		script +=  sTmp + "\"\n";
	}
	script += "\tkey_mgmt=NONE\n";
	script += "}\n";
	FILE *fp = fopen(WPA_SUPPLICANT_CONF, "w");
	if ( fp != NULL )
	{
		fprintf(fp, "%s", script.c_str());
		fclose(fp);
	}

	retVal = system("killall -q wpa_supplicant");
	retVal = WEXITSTATUS (retVal);
	//stiTESTCOND (retVal == 0, stiRESULT_ERROR);

	retVal = system("sleep 2;ifconfig ra0 up");
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	retVal = system("sleep 10;/usr/local/bin/wpa_supplicant -B -ira0 -c"WPA_SUPPLICANT_CONF);
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	if (nTestConnection)
	{
		bool bConnected = false;
		while (!bConnected && iLoop++ <= 500)
		{
			usleep(300);
			pp = popen("/sbin/iwpriv ra0 connStatus", "r");
			if (pp)
			{
				while ( fgets(line, sizeof(line), pp) != NULL )
				{
					if ( strstr(line, "Connected") != NULL )
					{
						printf(":: %s\n",line);
						bConnected = true;
					}
				}
				pclose(pp);
			}
		}

		if ( strstr(line, "Disconnected") != NULL )
		{
		 	printf(":: %s\n",line);
			stiTHROW (stiRESULT_WIRELESS_CONNECTION_FAILURE);
		}

	}

	STI_BAIL:

	return hResult;
}


stiHResult CstiWireless::Connect(const WEP* Wep,int nTestConnection)
{
	int retVal = 0;
	stiHResult hResult = stiRESULT_SUCCESS;
	char line[100];
	FILE *pp;
	int iLoop = 0;

	string script("network={\n");
	script += "\tssid=\"";
	if (Wep->pszSsid)
	{
		string sTmp = Wep->pszSsid;
		script +=  sTmp + "\"\n";
	}

	if (strlen(Wep->pszKey_index))
	{
		char szTmp[50];
		sprintf(szTmp,"\twep_key%s=%s\n",Wep->pszKey_index,Wep->pszKey );
		script += szTmp;
	}
	else
	{
		char szTmp[50];
		sprintf(szTmp,"\twep_key0=%s\n",Wep->pszKey );
		script += szTmp;
	}
	script += "\tkey_mgmt=NONE\n";
	script += "}\n";
	FILE *fp = fopen(WPA_SUPPLICANT_CONF, "w");
	if ( fp != NULL )
	{
		fprintf(fp, "%s", script.c_str());
		fclose(fp);
	}

	retVal = system("killall -q wpa_supplicant");
	retVal = WEXITSTATUS (retVal);
	sleep(1);

	retVal = system("sleep 2;ifconfig ra0 up");
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);
	sleep(10);

	retVal = system("sleep 10;/usr/local/bin/wpa_supplicant -B -ira0 -c"WPA_SUPPLICANT_CONF);
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	//printf("%s",script.c_str());

	if (nTestConnection)
	{
		bool bConnected = false;
		while (!bConnected && iLoop++ <= 500)
		{
			usleep(300);
			pp = popen("/sbin/iwpriv ra0 connStatus", "r");
			if (pp)
			{
				while ( fgets(line, sizeof(line), pp) != NULL )
				{
					if ( strstr(line, "Connected") != NULL )
					{
						printf(":: %s\n",line);
						bConnected = true;
					}
				}
				pclose(pp);
			}
		}

		if ( strstr(line, "Disconnected") != NULL )
		{
		 	printf(":: %s\n",line);
			stiTHROW (stiRESULT_WIRELESS_CONNECTION_FAILURE);
		}

	}

	STI_BAIL:

	return hResult;
}

stiHResult CstiWireless::Connect(const WpaPsk* wpa,int nTestConnection)
{
	int retVal = -1;
	char szTmp[50];
	stiHResult hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;
	char line[100];
	FILE *pp;
	int iLoop = 0;

	string script("network={\n");
	script += "\tssid=\"";
	if (wpa->pszSsid)
	{
		string sTmp = wpa->pszSsid;
		script +=  sTmp + "\"\n";
	}
	script += "\tkey_mgmt=WPA-PSK\n";
	sprintf(szTmp,"\tpsk=\"%s\"\n", wpa->pszKey);
	script += szTmp;
	script += "}\n";
	FILE *fp = fopen(WPA_SUPPLICANT_CONF, "w");
	if ( fp != NULL )
	{
		fprintf(fp, "%s", script.c_str());
		fclose(fp);
	}

	//printf("%s",script.c_str());
	retVal = system("killall -q wpa_supplicant");
	retVal = WEXITSTATUS (retVal);
	sleep(1);

	retVal = system("sleep 2;ifconfig ra0 up");
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);
	sleep(1);

	retVal = system("sleep 10;/usr/local/bin/wpa_supplicant -B -ira0 -c"WPA_SUPPLICANT_CONF);
	retVal = WEXITSTATUS (retVal);

	sleep(10);

	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	if (nTestConnection)
	{
		bool bConnected = false;
		while (!bConnected && iLoop++ <= 500)
		{
			usleep(300);
			pp = popen("/sbin/iwpriv ra0 connStatus", "r");
			if (pp)
			{
				while ( fgets(line, sizeof(line), pp) != NULL )
				{
					if ( strstr(line, "Connected") != NULL )
					{
						printf(":: %s\n",line);
						bConnected = true;
					}
				}
				pclose(pp);
			}
		}

		if ( strstr(line, "Disconnected") != NULL )
		{
		 	printf(":: %s\n",line);
			stiTHROW (stiRESULT_WIRELESS_CONNECTION_FAILURE);
		}

	}

	STI_BAIL:

	return hResult;
}


stiHResult CstiWireless::Connect(const WpaEap* wpa,int nTestConnection)
{
	int retVal = -1;
	stiHResult hResult = stiRESULT_SUCCESS;//stiRESULT_WIRELESS_CONNECTION_FAILURE;
	char line[100];
	FILE *pp;
	int iLoop = 0;

	string script("network={\n");
	script += "\tssid=\"";
	if (wpa->pszSsid)
	{
		string sTmp = wpa->pszSsid;
		script +=  sTmp + "\"\n";
	}

	script += "	eap=PEAP\n";
	script += "\tkey_mgmt=WPA-EAP\n";
	script += "identity=\"";
	if (wpa->pszIdentity)
	{
		string sTmp = wpa->pszIdentity;
		script +=  sTmp + "\"\n";
	}
	script += "password=\"";
	if (wpa->pszPrivateKeyPasswd)
	{
		string sTmp = wpa->pszPrivateKeyPasswd;
		script +=  sTmp + "\"\n";
	}
	script += "pairwise=TKIP\n";
	script += "phase1=\"peaplabe=0\"\n";
	script += "phase2=\"auth=MSCHAPV2\"\n";
	script += "}\n";

	//printf("%s",script.c_str());

	FILE *fp = fopen(WPA_SUPPLICANT_CONF, "w");
	if ( fp != NULL )
	{
		fprintf(fp, "%s", script.c_str());
		fclose(fp);
	}

	retVal = system("killall -q wpa_supplicant");
	retVal = WEXITSTATUS (retVal);
	//stiTESTCOND (retVal == 0, stiRESULT_ERROR); // dont test for error, killall may 

	retVal = system("sleep 2; ifconfig ra0 up");
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	retVal = system("sleep 10;/usr/local/bin/wpa_supplicant -B -ira0 -c"WPA_SUPPLICANT_CONF);
	retVal = WEXITSTATUS (retVal);
	stiTESTCOND (retVal == 0, stiRESULT_WIRELESS_CONNECTION_FAILURE);

	if (nTestConnection)
	{
		bool bConnected = false;
		while (!bConnected && iLoop++ <= 500)
		{
			usleep(300);
			pp = popen("/sbin/iwpriv ra0 connStatus", "r");
			if (pp)
			{
				while ( fgets(line, sizeof(line), pp) != NULL )
				{
					if ( strstr(line, "Connected") != NULL )
					{
						printf(":: %s\n",line);
						bConnected = true;
					}
				}
				pclose(pp);
			}
		}

		if ( strstr(line, "Disconnected") != NULL )
		{
		 	printf(":: %s\n",line);
			stiTHROW (stiRESULT_WIRELESS_CONNECTION_FAILURE);
		}

	}

	STI_BAIL:

	return hResult;
}

stiHResult CstiWireless::WirelessDeviceCarrierGet(bool * bCarrier)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char line[100];
	FILE *pp;
	pp = popen("/sbin/iwpriv ra0 connStatus", "r");
	if (pp)
	{
		while ( fgets(line, sizeof(line), pp) != NULL )
		{
			//printf("%s\n",line);
			if ( strstr(line, "Connected") != NULL )
			{
				*bCarrier = true;
				//hResult = stiRESULT_WIRELESS_CONNECTION_SUCCESS;
			} else if ( strstr(line, "Disconnected") != NULL )
				{
					*bCarrier = false;
					//hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;
				}
		}
		pclose(pp);
	}
	return  hResult;
}

