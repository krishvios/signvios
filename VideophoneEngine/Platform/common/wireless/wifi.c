#define LINUX
#define CONFIG_STA_SUPPORT
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h> /* for connect and socket*/
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include "rtmp_type.h"
#include "oid.h"

unsigned int ConvertRssiToSignalQuality(NDIS_802_11_RSSI RSSI);


int main()
{
	struct iwreq lwreq;
	int skfd, i = 0; /* generic raw socket desc. */

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
	return -1;

	char name[5]; // interface name
	char data[65535]; // command string
	struct iwreq wrq;
	PNDIS_WLAN_BSSID_EX  		pBssid;
	PNDIS_802_11_BSSID_LIST_EX	pBssidList;

	pBssidList = (PNDIS_802_11_BSSID_LIST_EX) malloc(65536);  //64k
	//pBssidList = (PNDIS_802_11_BSSID_LIST_EX) malloc(65535);
	if (pBssidList == NULL)
	{
		printf("pBssidList == NULL\n");
		close(skfd);
		return;
	}

	//Default setting:
        strcpy(data, "get_site_survey");
        strcpy(wrq.ifr_name, "ra0");
        wrq.u.data.length = strlen(data);
        wrq.u.data.pointer = data;
        wrq.u.data.flags = OID_802_11_BSSID_LIST_SCAN;
        if(ioctl(skfd, RTPRIV_IOCTL_GSITESURVEY, &wrq) < 0)
        {
        	fprintf(stderr, "Interface doesn't accept private ioctl...\n");
        	return -1;
        }
        printf("%s\n\n\n",data);

       // wrq.u.data.length = 0;
       // strcpy(wrq.ifr_name, "ra0");
       // wrq.u.data.pointer = 0;
       // wrq.u.data.flags = OID_802_11_BSSID_LIST_SCAN | OID_GET_SET_TOGGLE;
       // if(ioctl(skfd, RT_PRIV_IOCTL, &wrq) < 0)
       // {
       // 	fprintf(stderr, "Interface doesn't accept private ioctl BSSID_LIST_SCAN\n");
       // 	return -1;
       // }

	wrq.u.data.length = 65535;
	strcpy(wrq.ifr_name, "ra0");
	wrq.u.data.pointer = pBssidList;
	wrq.u.data.flags = OID_802_11_BSSID_LIST;
	if(ioctl(skfd, RT_PRIV_IOCTL, &wrq) < 0)
	{
		fprintf(stderr, "Interface doesn't accept private ioctl BSSID_LIST\n");
		return -1;
	}


	printf("#of ssids %d\n",pBssidList->NumberOfItems);

	printf("Error Number %d\n\n",errno);

	pBssid = (PNDIS_WLAN_BSSID_EX) pBssidList->Bssid;

	int j;
	for(j = 0; j < pBssidList->NumberOfItems; j++)
	{
		printf("%-32s  ",pBssid->Ssid.Ssid);
		//printf("%d  ",pBssid->Ssid.SsidLength);

		printf("%02x:%02x:%02x:%02x:%02x:%02x  ",
			pBssid->MacAddress[0], pBssid->MacAddress[1], pBssid->MacAddress[2],
			pBssid->MacAddress[3], pBssid->MacAddress[4], pBssid->MacAddress[5]);

		//printf("%i  ",pBssid->Privacy);
		printf("%3u%% ",ConvertRssiToSignalQuality(pBssid->Rssi));
		GetSecurity(pBssid);
		printf("\n");
		pBssid = (PNDIS_WLAN_BSSID_EX)((char *)pBssid + pBssid->Length);
	}




	close(skfd);
//	free(pBssidList);
	return 0;
}

unsigned int ConvertRssiToSignalQuality(NDIS_802_11_RSSI RSSI)
{
    unsigned int signal_quality;
    if (RSSI >= -50)
        signal_quality = 100;
    else if (RSSI >= -80)    // between -50 ~ -80dbm
        signal_quality = (unsigned int)(24 + (RSSI + 80) * 2.6);
    else if (RSSI >= -90)   // between -80 ~ -90dbm
        signal_quality = (unsigned int)((RSSI + 90) * 2.6);
    else    // < -84 dbm
        signal_quality = 0;

    return signal_quality;
}



#define WPA_OUI_TYPE		0x01F25000
#define WPA_OUI				0x00F25000
#define WPA_OUI_1			0x00030000
#define WPA2_OUI			0x00AC0F00
#define CISCO_OUI			0x00964000 // CCKM for Cisco, modify by candy 2006.11.24



GetSecurity(PNDIS_WLAN_BSSID_EX pBssid)
{

char tmpAuth[18], tmpEncry[18], tmpNetworkType[32];

memset(tmpAuth, 0x00, sizeof(tmpAuth));
memset(tmpEncry, 0x00, sizeof(tmpEncry));

BOOLEAN bTKIP = FALSE;
BOOLEAN bAESWRAP = FALSE;
BOOLEAN bAESCCMP = FALSE;
BOOLEAN bWPA = FALSE;
BOOLEAN bWPAPSK = FALSE;
BOOLEAN bWPANONE = FALSE;
BOOLEAN bWPA2 = FALSE;
BOOLEAN bWPA2PSK = FALSE;
BOOLEAN bWPA2NONE = FALSE;
BOOLEAN bCCKM = FALSE;

if ((pBssid->Length > sizeof(NDIS_WLAN_BSSID)) && (pBssid->IELength > sizeof(NDIS_802_11_FIXED_IEs)))
{
	UINT lIELoc = 0;
	PNDIS_802_11_FIXED_IEs pFixIE = (PNDIS_802_11_FIXED_IEs)pBssid->IEs;
	PNDIS_802_11_VARIABLE_IEs pVarIE = (PNDIS_802_11_VARIABLE_IEs)((char*)pFixIE + sizeof(NDIS_802_11_FIXED_IEs));
	lIELoc += sizeof(NDIS_802_11_FIXED_IEs);

	while (pBssid->IELength > (lIELoc + sizeof(NDIS_802_11_VARIABLE_IEs)))
	{

		if ((pVarIE->ElementID == 221) && (pVarIE->Length >= 16))
		{
			UINT* pOUI = (UINT*)((char*)pVarIE->data);
			if(*pOUI != WPA_OUI_TYPE)
			{
				lIELoc += pVarIE->Length;
				lIELoc += 2;
				pVarIE = (PNDIS_802_11_VARIABLE_IEs)((char*)pVarIE + pVarIE->Length + 2);

				if(pVarIE->Length <= 0)
					break;

				continue;
			}
			UINT* plGroupKey;
			USHORT* pdPairKeyCount;
			UINT* plPairwiseKey=NULL;
			UINT* plAuthenKey=NULL;
			USHORT* pdAuthenKeyCount;
			plGroupKey = (UINT*)((char*)pVarIE + 8);

			UINT lGroupKey = *plGroupKey & 0x00ffffff;
			if (lGroupKey == WPA_OUI)
			{
				lGroupKey = (*plGroupKey & 0xff000000) >> 0x18;
				if (lGroupKey == 2)
					bTKIP = TRUE;
				else if (lGroupKey == 3)
					bAESWRAP = TRUE;
				else if (lGroupKey == 4)
					bAESCCMP = TRUE;
			}
			else
			{
				lIELoc += pVarIE->Length;
				lIELoc += 2;
				pVarIE = (PNDIS_802_11_VARIABLE_IEs)((char*)pVarIE + pVarIE->Length + 2);

				if(pVarIE->Length <= 0)
					break;

				continue;
			}

			pdPairKeyCount = (USHORT*)((char*)plGroupKey + 4);
			plPairwiseKey = (UINT*) ((char*)pdPairKeyCount + 2);
			USHORT k = 0;
			for( k = 0; k < *pdPairKeyCount; k++)
			{
				UINT lPairKey = *plPairwiseKey & 0x00ffffff;
				if(lPairKey == WPA_OUI )//|| (lPairKey & 0xffffff00) == WPA_OUI_1)
				{
					lPairKey = (*plPairwiseKey & 0xff000000) >> 0x18;
					if(lPairKey == 2)
						bTKIP = TRUE;
					else if(lPairKey == 3)
						bAESWRAP = TRUE;
					else if(lPairKey == 4)
						bAESCCMP = TRUE;
				}
				else
					break;

				++plPairwiseKey;
			}

			pdAuthenKeyCount = (USHORT*)((char*)pdPairKeyCount + 2 + 4 * (*pdPairKeyCount));
			plAuthenKey = (UINT*)((char*)pdAuthenKeyCount + 2);
			for(k = 0; k < *pdAuthenKeyCount; k++)
			{
				UINT lAuthenKey = *plAuthenKey & 0x00ffffff;
				if(lAuthenKey == CISCO_OUI)
				{
					bCCKM = TRUE; // CCKM for Cisco
				}
				else if (lAuthenKey == WPA_OUI)
				{
					lAuthenKey = (*plAuthenKey & 0xff000000) >> 0x18;

					if(lAuthenKey == 1)
						bWPA = TRUE;
					else if(lAuthenKey == 0 || lAuthenKey == 2)
					{
						if(pBssid->InfrastructureMode)
							bWPAPSK = TRUE;
						else
							bWPANONE = TRUE;
					}
				}

				++plAuthenKey;

			}
		}
		else if(pVarIE->ElementID == 48 && pVarIE->Length >= 12)
		{
			UINT* plGroupKey;
			UINT* plPairwiseKey;
			USHORT* pdPairKeyCount;
			UINT* plAuthenKey;
			USHORT* pdAuthenKeyCount;
			plGroupKey = (UINT*)((char*)pVarIE + 4);

			UINT lGroupKey = *plGroupKey & 0x00ffffff;
			if(lGroupKey == WPA2_OUI)
			{
				lGroupKey = (*plGroupKey & 0xff000000) >> 0x18;
				if(lGroupKey == 2)
					bTKIP = TRUE;
				else if(lGroupKey == 3)
					bAESWRAP = TRUE;
				else if(lGroupKey == 4)
					bAESCCMP = TRUE;
			}
			else
			{
				lIELoc += pVarIE->Length;
				lIELoc += 2;
				pVarIE = (PNDIS_802_11_VARIABLE_IEs)((char*)pVarIE + pVarIE->Length + 2);

				if(pVarIE->Length <= 0)
					break;

				continue;
			}

			pdPairKeyCount = (USHORT*)((char*)plGroupKey + 4);
			plPairwiseKey = (UINT*)((char*)pdPairKeyCount + 2);
			USHORT k = 0;
			for( k = 0; k < *pdPairKeyCount; k++)
			{
				UINT lPairKey = *plPairwiseKey & 0x00ffffff;
				if(lPairKey == WPA2_OUI)
				{
					lPairKey = (*plPairwiseKey & 0xff000000) >> 0x18;
					if(lPairKey == 2)
						bTKIP = TRUE;
					else if(lPairKey == 3)
						bAESWRAP = TRUE;
					else if(lPairKey == 4)
						bAESCCMP = TRUE;
				}
				else
					break;

				++plPairwiseKey;
			}

			pdAuthenKeyCount = (USHORT*)((char*)pdPairKeyCount + 2 + 4 * *pdPairKeyCount);
			plAuthenKey = (UINT*)((char*)pdAuthenKeyCount + 2);

			for(k = 0; k < *pdAuthenKeyCount; k++)
			{
				UINT lAuthenKey = *plAuthenKey & 0x00ffffff;
				if(lAuthenKey == CISCO_OUI)
				{
					bCCKM = TRUE; // CCKM for Cisco
				}
				else if (lAuthenKey == WPA2_OUI)
				{
					lAuthenKey = (*plAuthenKey & 0xff000000) >> 0x18;
					if(lAuthenKey == 1)
						bWPA2 = TRUE;
					else if(lAuthenKey == 0 || lAuthenKey == 2)
					{
						if(pBssid->InfrastructureMode)
							bWPA2PSK = TRUE;
						else
							bWPA2NONE = TRUE;
					}
				}
				++plAuthenKey;
			}

		}

		lIELoc += pVarIE->Length;
		lIELoc += 2;
		pVarIE = (PNDIS_802_11_VARIABLE_IEs)((char*)pVarIE + pVarIE->Length + 2);

		if(pVarIE->Length <= 0)
			break;
	}
}

char strAuth[32], strEncry[32];
memset( strAuth, 0x00, sizeof(strAuth) );
memset( strEncry, 0x00, sizeof(strEncry) );
if(bCCKM)
	strcpy(strAuth, "CCKM; ");
if (bWPA)
	strcpy(strAuth, "WPA; ");
if (bWPAPSK)
	strcat(strAuth, "WPA-PSK; ");
if (bWPANONE)
	strcat(strAuth, "WPA-NONE; ");
if (bWPA2)
	strcat(strAuth, "WPA2; ");
if (bWPA2PSK)
	strcat(strAuth, "WPA2-PSK; ");
if (bWPA2NONE)
	strcat(strAuth, "WPA2-NONE; ");


if (strlen(strAuth) > 0)
{
	strncpy((char *)tmpAuth, strAuth, strlen(strAuth) - 2);
	strcpy(strAuth, (char *)tmpAuth);
}
else
{
	strcpy((char *)strAuth, "Unknown");
}

if(bTKIP)
	strcpy(strEncry, "TKIP; ");
if(bAESWRAP || bAESCCMP)
	strcat(strEncry, "AES; ");

if(strlen(strEncry) > 0)
{
	strncpy((char *)tmpEncry, strEncry, strlen(strEncry) - 2);
	strcpy(strEncry, (char *)tmpEncry);
}
else
{
	if (pBssid->Privacy)  // privacy value is on/of
		strcpy(strEncry, "WEP");
	else
	{
		strcpy(strEncry, "Not Use");
		strcpy(strAuth, "OPEN");
	}
}

printf(" %-18s ", strAuth);
printf(" %-18s ", strEncry);


}


