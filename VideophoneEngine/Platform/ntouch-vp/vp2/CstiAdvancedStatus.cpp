// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAdvancedStatus.h"
#include "CstiRegEx.h"
#include "constants.h"
#include <algorithm>
#include <string>
#include <list>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <cstdlib>
#include <rcu_api.h>


#define EPROM_BUFFER_LENGTH 64
#define SERIAL_NUMBER_BLOCK 0
#define LENS_TYPE_BLOCK 19

// Prototype
std::string HexToString (const std::string& in);

/*
 * do the initialization
 */
stiHResult CstiAdvancedStatus::initialize (
	CstiMonitorTask *monitorTask,
	CstiAudioHandler *audioHandler,
	CstiAudioInput *audioInput,
	CstiVideoInput *videoInput,
	CstiVideoOutput *videoOutput,
	CstiBluetooth *bluetooth)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiAdvancedStatusBase::initialize(
			monitorTask,
			audioHandler,
			audioInput,
			videoInput,
			videoOutput,
			bluetooth);
	stiTESTRESULT ();

	m_signalConnections.push_back (m_monitorTask->mfddrvdStatusChangedSignal.Connect (
		[this] ()
		{
			eventMfddrvdStatusChanged();
		}));

STI_BAIL:
	return hResult;
}


stiHResult CstiAdvancedStatus::mpuSerialSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//int nLength = 0;
	std::string sSerialNumber;
	const int halfMPUSerialNumberLength = 24;
	
	// Get MPU Serial Number
	FILE *pFile = nullptr;
	char szBuff[halfMPUSerialNumberLength + 1];  // Each half of MPU Serial number is 7 characters (size 2) separated by spaces
	
	// Get first half of MPU serial number
	if ((pFile = popen ("/usr/local/bin/i2cget 4 0x56 0x000c 8", "r")) == nullptr)
	{
		stiTHROW (stiRESULT_ERROR);
	}

	fgets (szBuff, sizeof(szBuff), pFile);
	pclose(pFile);
	pFile = nullptr;

	sSerialNumber += szBuff;
	
	// Get 2nd half of MPU serial number
	if ((pFile = popen ("/usr/local/bin/i2cget 4 0x56 0x0004 8", "r")) == nullptr)
	{
		stiTHROW (stiRESULT_ERROR);
	}

	fgets (szBuff, sizeof(szBuff), pFile);
	pclose(pFile);
	pFile = nullptr;
	
	sSerialNumber += szBuff;
	
	// This removes whitespace which includes:
	//space (0x20, ' ')
    //form feed (0x0c, '\f')
    //line feed (0x0a, '\n')
    //carriage return (0x0d, '\r')
    //horizontal tab (0x09, '\t')
    //vertical tab (0x0b, '\v') 
	sSerialNumber.erase (std::remove_if (sSerialNumber.begin(),
										 sSerialNumber.end (),
										 [](char x){return std::isspace(x);}),
										 sSerialNumber.end ());
	
	// Change to ASCII
	sSerialNumber = HexToString (sSerialNumber);

	// Remove whitespace
	sSerialNumber.erase (std::remove_if (sSerialNumber.begin (),
										 sSerialNumber.end (),
										 [](char x){return std::isspace(x);}),
										 sSerialNumber.end ());
	
	m_advancedStatus.mpuSerialNumber = sSerialNumber;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiAdvancedStatus::mpuSerialSet: MpuSerialNumber = %s\n", m_advancedStatus.mpuSerialNumber.c_str ());
	);

STI_BAIL:
	return hResult;
}

void CstiAdvancedStatus::staticStatusGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_statusMutex);
	rcuStaticStatusRead ();
}

void CstiAdvancedStatus::dynamicStatusGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_statusMutex);

	struct stat st {};
	if (stat("/sys/devices/platform/tegra-ehci.2/usb1/1-1", &st) > -1 && S_ISDIR(st.st_mode))
	{
		m_advancedStatus.cameraConnected = true;
	}
	else
	{
		m_advancedStatus.cameraConnected = false;
	}
	
	m_advancedStatus.audioJack = m_audioHandler->HeadphoneConnectedGet ();

	rcuDynamicStatusRead ();
}


void CstiAdvancedStatus::eventMfddrvdStatusChanged ()
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiAdvancedStatus::eventMfddrvdStatusChanged got estiMSG_MFDDRVD_STATUS_CHANGED\n");
	);

	rcuStaticStatusRead ();
}


stiHResult CstiAdvancedStatus::rcuStaticStatusRead ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char cPacket[MAX_RCU_READ_BUFFER];
	
	prodinfo_packet *pProdInfo;
	drv_version_packet *pDrvInfo;
	
	int nResult = 0;
	
	hResult = m_monitorTask->MfddrvdRunningGet (&m_advancedStatus.rcuConnected);
	stiTESTRESULT ();
	
	if (m_advancedStatus.rcuConnected)
	{
		//
		// here we talk to mfddrv daemon to get some rcu info
		//
		char EpromReadBuffer[EPROM_BUFFER_LENGTH + 1];
		const int nByteNum = 0;
		size_t SizePacket = sizeof (EpromReadBuffer) - 1;
		
		//
		// Read the RCU Serial Number
		//

		m_advancedStatus.rcuSerialNumber.clear ();

		memset (EpromReadBuffer, '\0', sizeof(EpromReadBuffer));
		
		nResult = RcuEepromRead (EpromReadBuffer, &SizePacket, SERIAL_NUMBER_BLOCK, nByteNum);
		
		if (nResult != 0)
		{
			stiASSERTMSG (estiFALSE, "CstiAdvancedStatus::rcuStaticStatusRead: RcuEepromRead RcuSerialNumber failure: %d", nResult);
		}
		else
		{
			//
			// Null terminate the buffer just in case RcuEepromRead wrote too many characters.
			//
			EpromReadBuffer[sizeof(EpromReadBuffer) - 1] = '\0';

			//
			// Validate that what was retrieved is a serial number that complies with our serial number rules.
			//
			CstiRegEx serialNumberValidator (SERIAL_NUMBER_REGEX);

			if (serialNumberValidator.Match(EpromReadBuffer))
			{
				m_advancedStatus.rcuSerialNumber = EpromReadBuffer;

				Trim(&m_advancedStatus.rcuSerialNumber);

				stiDEBUG_TOOL (g_stiVideoInputDebug,
					stiTrace ("CstiAdvancedStatus::RcuStatusRead:m_advancedStatus.rcuSerialNumber = \"%s\"\n", m_advancedStatus.rcuSerialNumber.c_str ());
				);
			}
			else
			{
				stiASSERTMSG (estiFALSE, "RCUSerialNumber = \"%s\"", EpromReadBuffer);
			}
		}
		
		//
		// Read the RCU Lens Type
		//

		memset (EpromReadBuffer, '\0', sizeof (EpromReadBuffer));
		
		SizePacket = sizeof (EpromReadBuffer) - 1;
		
		nResult = RcuEepromRead (EpromReadBuffer, &SizePacket, LENS_TYPE_BLOCK, nByteNum);
		
		if (nResult != 0)
		{
			m_advancedStatus.rcuLensType.clear ();
			stiASSERTMSG (estiFALSE, "CstiAdvancedStatus::rcuStaticStatusRead: RcuEepromRead RcuLensType failure: %d", nResult);
		}
		else
		{
			m_advancedStatus.rcuLensType = EpromReadBuffer;
			
			Trim (&m_advancedStatus.rcuLensType);

			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace ("CstiAdvancedStatus::RcuStatusRead: m_advancedStatus.rcuLensType = \"%s\"\n", m_advancedStatus.rcuLensType.c_str ());
			);
		}
		
		//
		// Read the RCU Firmware Version
		//

		memset (cPacket, '\0', sizeof(cPacket));
		pProdInfo = (prodinfo_packet *)cPacket;
		pProdInfo->type = PACKET_TYPE_GET_FW_VERSION;
		nResult = RcuPacketSendRecieve (cPacket, sizeof(prodinfo_packet));
		
		if (nResult != 0)
		{
			m_advancedStatus.rcuFirmwareVersion.clear ();
			stiASSERTMSG (estiFALSE, "CstiAdvancedStatus::rcuStaticStatusRead: WARNING RcuPacketSendRecieve failed to send PACKET_TYPE_GET_FW_VERSION\n");
		}
		else
		{
			m_advancedStatus.rcuFirmwareVersion = (char*)pProdInfo->string;
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace ("CstiAdvancedStatus::RcuStatusRead: m_advancedStatus.rcuFirmwareVersion = %s\n", m_advancedStatus.rcuFirmwareVersion.c_str ());
			);
		}
		
		//
		// Read the RCU Driver Version
		//

		memset (cPacket, '\0', sizeof(cPacket));
		pDrvInfo = (drv_version_packet *)cPacket;
		pDrvInfo->type = PACKET_TYPE_GET_DRV_VERSION;
		nResult = RcuPacketSendRecieve (cPacket, sizeof(drv_version_packet));
		
		if (nResult != 0)
		{
			m_advancedStatus.rcuDriverVersion.clear ();
			stiASSERTMSG (estiFALSE, "CstiAdvancedStatus::rcuStaticStatusRead: WARNING RcuPacketSendRecieve failed to send PACKET_TYPE_GET_DRV_VERSION\n");
		}
		else
		{
			m_advancedStatus.rcuDriverVersion = pDrvInfo->data;
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace ("CstiAdvancedStatus::RcuStatusRead: m_advancedStatus.rcuDriverVersion = %s\n", m_advancedStatus.rcuDriverVersion.c_str ());
			);
		}
		
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("CstiAdvancedStatus::RcuStatusRead: not connected\n");
		);
		m_advancedStatus.rcuSerialNumber.clear ();
		m_advancedStatus.rcuLensType.clear ();
		m_advancedStatus.rcuFirmwareVersion.clear ();
		m_advancedStatus.rcuDriverVersion.clear ();
	}
	
STI_BAIL:

	return hResult;
}

stiHResult CstiAdvancedStatus::rcuDynamicStatusRead ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char cPacket[MAX_RCU_READ_BUFFER];	// generic packet for sending data
	
	temperature_packet *pTempPkt;
	
	int nResult = 0;
	
	hResult = m_monitorTask->MfddrvdRunningGet (&m_advancedStatus.rcuConnected);
	stiTESTRESULT ();
	
	if (m_advancedStatus.rcuConnected)
	{
		memset (cPacket, '\0', sizeof (cPacket));
		pTempPkt = (temperature_packet *)cPacket;
		pTempPkt->type = PACKET_TYPE_GET_TEMP;
		nResult = RcuPacketSendRecieve (cPacket, sizeof(temperature_packet));
		
		if (nResult != 0)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiAdvancedStatus::rcuDynamicStatusRead: WARNING RcuPacketSendRecieve failed to send PACKET_TYPE_GET_TEMP\n");
		}
		
		m_advancedStatus.rcuTemp = pTempPkt->value.sensor_value;

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("CstiAdvancedStatus::rcuDynamicStatusRead: m_advancedStatus.rcuTemp = %X (%d)\n", m_advancedStatus.rcuTemp, m_advancedStatus.rcuTemp);
		);
	}
	else
	{
		m_advancedStatus.rcuTemp = 0;
	}
	
STI_BAIL:

	return hResult;
}

std::string HexToString (
	const std::string& in)
{
	std::string output;

	int nLength = in.length();
	
	stiASSERTMSG ((nLength & 1) == 0, "HexToString: length is not a multiple of two: length = %d", nLength);

	for (int i = 0; i < nLength; i += 2)
	{
		std::string ss;
		ss = in.substr (i, 2);
		
		ss = strtol(ss.c_str (), nullptr, 16);
		output += ss;
	}

	return output;
}

