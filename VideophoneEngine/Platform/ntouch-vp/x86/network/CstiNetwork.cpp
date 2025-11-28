/*!
* \file CstiNetwork.cpp
* \brief Consolidation of all network tasks.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#include "CstiNetwork.h"
#include "stiTools.h"
#include "stiError.h"
#include "stiTaskInfo.h"
#include "stiTrace.h"
//#include "IstiWireless.h"
#include <fstream>
#include <cstdlib>
#include <regex.h>
#include <sys/stat.h>
#include "CstiExtendedEvent.h"
#if APPLICATION == APP_NTOUCH_ANDROID
#include <sys/wait.h>
#include <sys/syscall.h>
#endif
#include <sstream>
#include <linux/wireless.h>
#include <ifaddrs.h>

extern "C"
{
	#include "stiOSLinuxNet.h"
}

//
// Constants
//

static const char g_szDefaultHostName[] = "SORENSONVP";
//#define NETWORK_SETTINGS			PERSISTENT_DATA "/networking/ipdata.sh"
//#define NETWORK_SETTINGS_BACKUP		PERSISTENT_DATA "/networking/ipdata.backup"
#define NETWORK_SCRIPT		"/etc/netstart.sh"

static const int nNETSTART_RESULT_DHCP_FAILURE = 11;
static const int nNETSTART_RESULT_SUBNET_MASK_FAILURE = 12;
static const int nNETSTART_RESULT_ROUTE_FAILURE = 13;

const unsigned int unCHECK_CONNECTION_INTERVAL = 3000; // 3 seconds (in milliseconds)


//
// Locals
//
stiEVENT_MAP_BEGIN (CstiNetwork)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiNetwork::EventTaskShutdown)
stiEVENT_DEFINE (estiEVENT_TIMER, CstiNetwork::EventTimer)
#if 0
#ifdef stiWIRELESS
//stiEVENT_DEFINE (eWIRELESS_CONNECT_OPEN, CstiNetwork::EventWAPConnectOpen)
//stiEVENT_DEFINE (eWIRELESS_CONNECT_WEP, CstiNetwork::EventWAPConnectWEP)
//stiEVENT_DEFINE (eWIRELESS_CONNECT_WPAPSK, CstiNetwork::EventWAPConnectWpaPsk)
//stiEVENT_DEFINE (eWIRELESS_CONNECT_WPAEAP, CstiNetwork::EventWAPConnectWpaEap)
stiEVENT_DEFINE (eWIRELESS_DEVICE_START, CstiNetwork::EventWirelessDeviceStart)
stiEVENT_DEFINE (eWIRELESS_DEVICE_STOP, CstiNetwork::EventWirelessDeviceStop)
#endif
#endif
stiEVENT_DEFINE (eSETTINGS_SET, CstiNetwork::EventSettingsSet)
stiEVENT_MAP_END (CstiNetwork)
stiEVENT_DO_NOW (CstiNetwork)

#define NET_DEVICE_LEN 10

static const uint8_t un8IP_OCTET_MAX = 255;		// The largest number valid in any
													// segment of an IP address.
static const int nIP_OCTET_LENGTH = 3;				// Longest length of an IP octet


stiHResult IPAddressToIntegerConvert (
	const char *pszAddress,	///< The string containing the IP address to convert.
	uint32_t *pun32Value); ///< The value of the IP address as an integer
static bool IPOctetValidate (
	const char *pszOctet);

/*!
 * \brief Constructor
 */
CstiNetwork::CstiNetwork (CstiBluetooth *)
:
	CstiOsTaskMQ (
		nullptr,
		0,
		reinterpret_cast<size_t>(this), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		stiNETWORK_MAX_MESSAGES_IN_QUEUE,
		stiNETWORK_MAX_MSG_SIZE,
		stiNETWORK_TASK_NAME,
		stiNETWORK_TASK_PRIORITY,
		stiNETWORK_STACK_SIZE),
	m_oDHCP (DHCPCallback, reinterpret_cast<size_t>(this)) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

{
	stiLOG_ENTRY_NAME (CstiNetwork::CstiNetwork);

//#ifdef stiWIRELESS
//	m_pWireless = IstiWireless::InstanceGet ();
//#endif
	m_stState.eState = estiIDLE;
	m_stState.nNegotiateCnt = 0;

	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream NetworkSettingsFile;
	std::stringstream NetworkSettingsBackupFile;

	NetworkSettingsFile << DynamicDataFolder << "networking/ipdata.sh";
	NetworkSettingsBackupFile << DynamicDataFolder << "networking/ipdata.backup";

	m_NetworkSettingsFile = NetworkSettingsFile.str ();
	m_NetworkSettingsBackupFile = NetworkSettingsBackupFile.str ();

} // end CstiNetwork::CstiNetwork


CstiNetwork::~CstiNetwork ()
{
	Shutdown ();
	WaitForShutdown ();

	if (m_wdConnectionLost)
	{
		stiOSWdDelete (m_wdConnectionLost);
		m_wdConnectionLost = nullptr;
	}
}


///\brief Read the settings file to determine the DHCP setting
///
stiHResult CstiNetwork::NetStartSettingGet (
	bool *pbUseDHCP 
#if defined stiWIRELESS
	,enum NetworkType *peNetworkDeviceType
#endif
	)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bDHCPResult = true;
	regex_t RegexParser;
	int nRead;  stiUNUSED_ARG (nRead);

#if defined stiWIRELESS
	NetworkType  eDeviceResult = NetworkType::Wired;
	*peNetworkDeviceType = NetworkType::Wired;
#endif

	char *pszBuffer = nullptr;

	FILE *fp = fopen (m_NetworkSettingsFile.c_str (), "r");

	if (fp)
	{
		fseek (fp, 0, SEEK_END);

		//
		// Determine the size of the file.
		//
		long lFileSize = ftell (fp);
		fseek (fp, 0, SEEK_SET);

		//
		// Allocate a buffer large enough to read the file.
		//
		pszBuffer = new char [lFileSize + 1];

		stiTESTCOND (pszBuffer, stiRESULT_ERROR);

		//
		// Read in the file
		//
		nRead = fread (pszBuffer, lFileSize, sizeof (char), fp);
		pszBuffer[lFileSize] = '\0';

#ifdef stiWIRELESS
		//
		// Search for the string
		//
		const char szDevicePattern[] = "svDEVICE=ra0";

		regcomp(&RegexParser, szDevicePattern, REG_NOSUB);

		int nResult  = regexec(&RegexParser, pszBuffer, 0, nullptr, 0);

		if (nResult == REG_NOMATCH)
		{
			eDeviceResult = NetworkType::Wired;
		}
		else
		{
			eDeviceResult = NetworkType::WiFi;
		}

		regfree (&RegexParser);

		*peNetworkDeviceType = eDeviceResult;


#endif

		//
		// Search for the string
		//
		const char szDHCPPattern[] = "svUSE_DHCP=true";

		regcomp(&RegexParser, szDHCPPattern, REG_NOSUB);

		int nDHCPResult  = regexec(&RegexParser, pszBuffer, 0, nullptr, 0);

		bDHCPResult = nDHCPResult != REG_NOMATCH;

		regfree (&RegexParser);
	} 

	*pbUseDHCP = bDHCPResult;

STI_BAIL:

	if (fp)
	{
		fclose (fp);
	}


	if (pszBuffer)
	{
		delete [] pszBuffer;
		pszBuffer = nullptr;
	}

	return hResult;
}


/*!
* \brief Set the settings to match the settings on the NIC.
* \retval None
*/
void NICSettingsGet (
	SstiNetworkSettings *pSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// IPv4
	hResult = stiGetLocalIp (&(pSettings->IPv4IP), estiTYPE_IPV4);
	if (stiIS_ERROR (hResult))
	{
		pSettings->IPv4IP.clear ();
	}

	hResult = stiGetNetSubnetMaskAddress (&pSettings->IPv4SubnetMask, estiTYPE_IPV4);
	if (stiIS_ERROR (hResult))
	{
		pSettings->IPv4SubnetMask.clear ();
	}

	hResult = stiGetDefaultGatewayAddress (&pSettings->IPv4NetGatewayIP, estiTYPE_IPV4);
	if (stiIS_ERROR (hResult))
	{
		pSettings->IPv4NetGatewayIP.clear ();
	}

	hResult = stiGetDNSAddress (0, &pSettings->IPv4DNS1, estiTYPE_IPV4);
	if (stiIS_ERROR (hResult))
	{
		pSettings->IPv4DNS1.clear ();
	}

	hResult = stiGetDNSAddress (1, &pSettings->IPv4DNS2, estiTYPE_IPV4);
	if (stiIS_ERROR (hResult))
	{
		pSettings->IPv4DNS2.clear ();
	}

#ifdef IPV6_ENABLED
	// IPv6
	hResult = stiGetLocalIp (&pSettings->IPv6IP, estiTYPE_IPV6);
	if (stiIS_ERROR (hResult))
#endif
	{
		pSettings->IPv6IP.clear ();
	}

	/*
	There is no such thing as a subnet mask in ipv6
	hResult = stiGetNetSubnetMaskAddress (&pSettings->IPv6Prefix, estiTYPE_IPV6);
	if (stiIS_ERROR (hResult))
	{
		pSettings->nIPv6Prefix = 0;
	}
	*/

#ifdef IPV6_ENABLED
	hResult = stiGetDefaultGatewayAddress (&pSettings->IPv6GatewayIP, estiTYPE_IPV6);
	if (stiIS_ERROR (hResult))
#endif // IPV6_ENABLED
	{
		pSettings->IPv6GatewayIP.clear ();
	}

#ifdef IPV6_ENABLED
	hResult = stiGetDNSAddress (0, &pSettings->IPv6DNS1, estiTYPE_IPV6);
	if (stiIS_ERROR (hResult))
#endif
	{
		pSettings->IPv6DNS1.clear ();
	}

#ifdef IPV6_ENABLED
	hResult = stiGetDNSAddress (1, &pSettings->IPv6DNS2, estiTYPE_IPV6);
	if (stiIS_ERROR (hResult))
#endif
	{
		pSettings->IPv6DNS2.clear ();
	}
}


/*!
* \brief Initialize the network object with the settings passed in.
* \retval None
*/
stiHResult CstiNetwork::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char szNetHostName[128];

#if defined stiWIRELESS
	hResult = NetStartSettingGet (&m_Settings.bIPv4UseDHCP, &m_Settings.eNetwork);
	stiTESTRESULT ();

	stiOSSetNIC ((m_Settings.eNetwork==NetworkType::Wired)?USE_HARDWIRED_NETWORK_DEVICE:USE_WIRELESS_NETWORK_DEVICE);
#else
	hResult = NetStartSettingGet (&m_Settings.bIPv4UseDHCP);
	stiTESTRESULT ();
#endif

	NICSettingsGet (&m_Settings);
	gethostname (szNetHostName, sizeof (szNetHostName));
	m_Settings.NetHostname.assign (szNetHostName);
	m_oDHCP.Initialize (NetworkDeviceNameGet().c_str());

	// Create a watchdog timer for checking network connection
	m_wdConnectionLost = stiOSWdCreate ();
	stiTESTCOND (nullptr != m_wdConnectionLost, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


/*!
* \brief Start up the Network task
* \retval None
*/
stiHResult CstiNetwork::Startup ()
{
	//
	// Start the task
	//
	m_oDHCP.Startup ();

	CstiOsTaskMQ::Startup ();

	stiHResult hResult = SettingsSet(&m_Settings, nullptr);

	return hResult;
}


stiHResult CstiNetwork::Shutdown ()
{
	m_oDHCP.Shutdown ();
	return CstiOsTaskMQ::Shutdown ();
}


stiHResult CstiNetwork::WaitForShutdown ()
{
	m_oDHCP.WaitForShutdown ();
	return CstiOsTaskMQ::WaitForShutdown ();
}


///\brief Backups the network settings file
///
stiHResult CstiNetwork::NetstartSettingsBackup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::stringstream Command;
	
	int nSysStatus = 0;

	Command << "cp " << m_NetworkSettingsFile.c_str () << " " << m_NetworkSettingsBackupFile.c_str ();

	nSysStatus = system (Command.str ().c_str ());
	nSysStatus = WEXITSTATUS (nSysStatus);

	stiTESTCOND (nSysStatus == 0, stiRESULT_ERROR);
	
STI_BAIL:

	return hResult;
}


///\brief Restores the network settings file
///
void CstiNetwork::NetstartSettingsRestore ()
{
	std::stringstream Command;

	Command << "cp " << m_NetworkSettingsBackupFile.c_str () << " " << m_NetworkSettingsFile.c_str ();

	int nSysStatus = system (Command.str ().c_str ()); stiUNUSED_ARG (nSysStatus);
}


///\brief Updates the netstart.sh script with the current settings.
//
// \param pstSettings, A structure containing the network settings to be applied.
//
stiHResult CstiNetwork::NetstartSettingsUpdate (
	const SstiNetworkSettings *pstSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	const char *pszNetHostname = pstSettings->NetHostname.c_str ();

	//
	// If the hostname is not set then use the default host name.
	//
	if (pszNetHostname[0] == '\0')
	{
		pszNetHostname = g_szDefaultHostName;
	}

	//
	// If DNS 1 is not set then move DNS 2 to the DNS 1
	// position and clear the DNS 2 position.
	//
	const char *pszDNS1 = pstSettings->IPv4DNS1.c_str ();
	const char *pszDNS2 = pstSettings->IPv4DNS2.c_str ();

	if (pszDNS1[0] == '\0')
	{
		pszDNS1 = pszDNS2;
		pszDNS2 = "";
	}

	FILE *pFile = fopen (m_NetworkSettingsFile.c_str (), "w"); // mode "w" is open at pos 0 and overwrite.

	stiTESTCOND(pFile, stiRESULT_UNABLE_TO_OPEN_FILE);

	fprintf (pFile, "#This file is autogenerated by CstiNetwork\n");
	fprintf (pFile, "#\n");
	fprintf (pFile, "svUSE_DHCP=%s\n", pstSettings->bIPv4UseDHCP ? "true" : "");
	fprintf (pFile, "svIP=%s\n", pstSettings->IPv4IP.c_str ());
	fprintf (pFile, "svNETMASK=%s\n", pstSettings->IPv4SubnetMask.c_str ());
	fprintf (pFile, "svGATEWAY=%s\n", pstSettings->IPv4NetGatewayIP.c_str ());
	fprintf (pFile, "svHOSTNAME=%s\n", pszNetHostname);
	fprintf (pFile, "svDNS1=%s\n", pszDNS1);
	fprintf (pFile, "svDNS2=%s\n", pszDNS2);
#ifdef stiWIRELESS
	fprintf (pFile, "svDEVICE=%s\n", NetworkDeviceNameGet().c_str());
#endif
	fclose (pFile);

	//
	// Make sure the file is executable.
	//
	chmod (m_NetworkSettingsFile.c_str (), S_IRUSR | S_IWUSR | S_IXUSR);
	
STI_BAIL:

	return hResult;
}


IstiNetwork::EstiError CstiNetwork::ErrorGet () const
{
	return m_eError;
}


/*!
* \brief Returns the current network state
* \retval the current network state
*/
IstiNetwork::EstiState CstiNetwork::StateGet () const
{
	Lock ();

	IstiNetwork::EstiState eState = m_stState.eState;

	Unlock ();

	return eState;
}


/*!
* \brief Sets the current network state
* \retval the current network state
*/
void CstiNetwork::StateSet (EstiState eState)
{
	bool bPerformChange = false;
	switch (eState)
	{
		case estiIDLE:
			// If we are currently negotiating, reduce the negotiating count by one.  If we are
			// back to 0, then change the state.
			if (estiNEGOTIATING == m_stState.eState &&
				--m_stState.nNegotiateCnt == 0)
			{
				bPerformChange = true;
			}
			break;

		case estiNEGOTIATING:
			// If we aren't already in the negotiating state, perform the change.
			if (estiNEGOTIATING != m_stState.eState)
			{
				bPerformChange = true;
			}
			// increment the counter in any case
			m_stState.nNegotiateCnt++;
			break;

		case estiERROR:
		default:
			// If we aren't already in the error state, return the count to 0 and
			// allow the change.
			if (estiERROR != m_stState.eState)
			{
				m_stState.nNegotiateCnt = 0;
				bPerformChange = true;
			}
			break;
	}
	if (bPerformChange)
	{
		stiDEBUG_TOOL (g_stiDHCPDebug,
			stiTrace ("<CstiNetwork::StateSet> Changing state from %s to %s\n",
				estiIDLE == m_stState.eState ? "Idle" :
				estiNEGOTIATING == m_stState.eState ? "Negotiating" : "Error",
				estiIDLE == eState ? "Idle" :
				estiNEGOTIATING == eState ? "Negotiating" : "Error");
		);
		m_stState.eState = eState;

		// Notify the application of the state change
		networkStateChangedSignal.Emit(eState);
	}
}


/*!
* \brief Subsystems communicate back to this class with this callback function.
* \retval none
*/
stiHResult CstiNetwork::DHCPCallback (
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	auto pThis = reinterpret_cast<CstiNetwork *>(CallbackParam); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	switch (n32Message)
	{
		case CstiDHCP::estiMSG_CB_DHCP_DECONFIG_STARTED:

			break;

		case CstiDHCP::estiMSG_CB_DHCP_DECONFIG_COMPLETED:

			break;

		case CstiDHCP::estiMSG_CB_DHCP_IPCONFIG_STARTED:

			break;

		case CstiDHCP::estiMSG_CB_DHCP_IPCONFIG_COMPLETED:
		{
			SstiNetworkSettings Settings;
			bool bSettingsChanged = false;
			
			NICSettingsGet (&Settings);

			pThis->Lock ();

			pThis->StateSet (estiIDLE);

			pThis->Unlock ();

			if (Settings.IPv4IP != pThis->m_Settings.IPv4IP
				 || Settings.IPv4SubnetMask != pThis->m_Settings.IPv4SubnetMask
				 || Settings.IPv4NetGatewayIP != pThis->m_Settings.IPv4NetGatewayIP
				 || Settings.IPv4DNS1 != pThis->m_Settings.IPv4DNS1
				 || Settings.IPv4DNS2 != pThis->m_Settings.IPv4DNS2)
			{
				bSettingsChanged = true;

				pThis->m_Settings.IPv4IP.assign (Settings.IPv4IP);
				pThis->m_Settings.IPv4SubnetMask.assign (Settings.IPv4SubnetMask);
				pThis->m_Settings.IPv4NetGatewayIP.assign (Settings.IPv4NetGatewayIP);
				pThis->m_Settings.IPv4DNS1.assign (Settings.IPv4DNS1);
				pThis->m_Settings.IPv4DNS2.assign (Settings.IPv4DNS2);
			}
			
			if (bSettingsChanged || pThis->m_unDHCPRequestId != 0)
			{
				pThis->networkSettingsChangedSignal.Emit ();
				pThis->m_unDHCPRequestId = 0;
			}

			break;
		}
	}

	return stiRESULT_SUCCESS;
}


///!
///! this will let us know if the cable is plugged in
///!
stiHResult CstiNetwork::EthernetCableConnectedGet(bool *pbConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bResult = false;

	FILE *pp;
	char line[100];
	std::string cmd = std::string("/sbin/ifconfig ") + NetworkDeviceNameGet();
	pp = popen(cmd.c_str(), "r");
	if (pp)
	{
		while (fgets(line, sizeof(line), pp) != nullptr )
		{
			//printf("%s\n",line);
			if ( strstr(line, "RUNNING") != nullptr )
			{
				bResult = true;
				break;
			}
		}
		pclose(pp);
	}

	*pbConnected = bResult;

	return  hResult;
}


stiHResult CstiNetwork::EthernetCableStatusGet (bool *pbConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	*pbConnected = m_bConnected;

	Unlock ();

	return hResult;
}

/*!
 * \brief Gets the active network device name, be it wired or wireless
 *
 * \return Returns a std::string with the active network device name
 */
std::string CstiNetwork::NetworkDeviceNameGet () const
{
	return FirstNetworkDeviceNameGet(m_Settings.eNetwork);
}


/*!
 * \brief Determines the network type of a given device
 *
 * \param deviceName Name of the network device
 * \param addressFamily AF_INET4 or AF_INET6 for IPv4 or IPv6
 *
 * \return Returns the network type of the device (i.e. wired or wireless)
 */
NetworkType NetworkTypeGet (
	const std::string &szDeviceName,
	int nAddressFamily)
{
	struct iwreq pwrq{};
	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, szDeviceName.c_str(), IFNAMSIZ);

	int sock = -1;
	NetworkType eNetworkType = NetworkType::None;

	if ((sock = socket(nAddressFamily, SOCK_STREAM, 0)) >= 0)
	{
		if (ioctl(sock, SIOCGIWNAME, &pwrq) == -1)
		{
			eNetworkType = NetworkType::Wired;
		}
		else
		{
			eNetworkType = NetworkType::WiFi;
		}

		close(sock);
	}

	return eNetworkType;
}


/*!
 * \brief Gets a list of all network devices on the host machine
 *
 * \return Returns a std::list<SstiNetworkDevice> populated with all network devices
 */
std::list<SstiNetworkDevice> CstiNetwork::NetworkDevicesGet() const
{
	std::list<SstiNetworkDevice> devices;
	struct ifaddrs *ifaddr = nullptr;

	if (getifaddrs(&ifaddr) == -1)
	{
		stiASSERT(estiFALSE);
	}

	for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
		{
			continue;
		}

		int family = ifa->ifa_addr->sa_family;

		if (family == AF_INET
#ifdef IPV6_ENABLED
			|| family == AF_INET6
#endif
		   )
		{
			std::string deviceName = ifa->ifa_name;

			if (deviceName == "lo")
			{
				continue;
			}

			SstiNetworkDevice device;
			device.name = deviceName;
			device.eAddressType = (family == AF_INET6) ? estiTYPE_IPV6 : estiTYPE_IPV4;
			device.networkType = NetworkTypeGet(deviceName, family);
			devices.push_back(device);
		}
	}

	if (ifaddr)
	{
		freeifaddrs(ifaddr);
	}

	return devices;
}


// Returns the name of the first network device of the given type
/*!
 * \brief Gets the first network device of a given network type
 *
 * \param type Specifies which type of device - wired or wireless
 *
 * \return Returns the name of the first network device of the given type
 */
std::string CstiNetwork::FirstNetworkDeviceNameGet(NetworkType type) const
{
	std::list<SstiNetworkDevice> devices = NetworkDevicesGet();
	std::list<SstiNetworkDevice>::const_iterator itr;
	std::string deviceName;

	for (itr = devices.begin(); itr != devices.end(); itr++)
	{
		if (itr->networkType == type)
		{
			deviceName = itr->name;
			break;
		}
	}

	return deviceName;
}


#if 0
#ifdef stiWIRELESS
/*!
* \brief Connect to the WAP device
* \retval estiOK
*/
stiHResult CstiNetwork::WirelessConnect (const char * pszESSID)
{
	stiHResult hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;

	StateSet (estiNEGOTIATING);

	if (pszESSID)
	{
		hResult = m_pWireless->Connect(pszESSID,0);
	}

	if (hResult == stiRESULT_SUCCESS)
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS, 0);
		StateSet (estiIDLE);
	}                        
	else
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE, 0);
		m_eError = estiErrorWirelessAccessPoint;
		StateSet (estiERROR);
	}

	return hResult;
}

/*!
* \brief Connect to the WAP device
* \retval estiOK
*/
stiHResult CstiNetwork::WirelessConnect (const WEP * pstWEP)
{
	stiHResult hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;

	StateSet (estiNEGOTIATING);

	if (pstWEP)
	{
		hResult = m_pWireless->Connect (pstWEP,0);
	}

	if (hResult == stiRESULT_SUCCESS)
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS, 0);
		StateSet (estiIDLE);
	}                        
	else
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE, 0);
		m_eError = estiErrorWirelessAccessPoint;
		StateSet (estiERROR);
	}

	return hResult;
}


/*!
* \brief Connect to the WAP device
* \retval estiOK
*/
stiHResult CstiNetwork::WirelessConnect (const WpaPsk * pstWpaPsk)
{
	stiHResult hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;

	StateSet (estiNEGOTIATING);

	if (pstWpaPsk)
	{
		hResult = m_pWireless->Connect (pstWpaPsk,1);
	}

	StateSet (estiIDLE);

	if (hResult == stiRESULT_SUCCESS)
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS, 0);
		StateSet (estiIDLE);
	}                        
	else if (hResult == stiRESULT_WIRELESS_CONNECTION_FAILURE)
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE, 0);
		m_eError = estiErrorWirelessAccessPoint;
		StateSet (estiERROR);
	}

	return hResult;
}


/*!
* \brief Connect to the WAP device
* \retval estiOK
*/
stiHResult CstiNetwork::WirelessConnect (const WpaEap * pstWpaEap)
{
	stiHResult hResult = stiRESULT_WIRELESS_CONNECTION_FAILURE;

	StateSet (estiNEGOTIATING);

	if (pstWpaEap)
	{
		hResult = m_pWireless->Connect (pstWpaEap,1);
	}

	if (hResult == stiRESULT_SUCCESS)
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS, 0);
		StateSet (estiIDLE);
	}                        
	else
	{
		//Callback (estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE, 0);
		m_eError = estiErrorWirelessAccessPoint;
		StateSet (estiERROR);
	}   		   

	return hResult;
}


/*!
* \brief Return which network is used.
* \retval int
*/
int CstiNetwork::WirelessInUse()
{
	return stiOSGetNIC(); 
}


/*!
* \brief Return a list of available WAPs
* \retval stiHResult
*/
stiHResult CstiNetwork::WAPListGet (
	WAPListInfo* pstWAPList) const
{
	return  m_pWireless->WAPListGet (pstWAPList);
}


/*!
* \brief Determine if the wireles driver is available
* \retval stiHResult
*/
stiHResult CstiNetwork::WirelessAvailable () const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pWireless->WirelessAvailable ();
	stiTESTRESULT()


STI_BAIL:
	 return  hResult;
}

stiHResult CstiNetwork::WirelessDeviceConnected(bool * bCarrier)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pWireless->WirelessDeviceCarrierGet(bCarrier);
	stiTESTRESULT()


STI_BAIL:
	 return  hResult;
}


/*!
* \brief Determine if the wireles driver is available
* \retval stiHResult
*/
stiHResult CstiNetwork::WirelessSignalStrengthGet (int *nStrength)
{

	if (m_pWireless->WirelessAvailable () ==  stiRESULT_WIRELESS_AVAILABLE)
	{
		return m_pWireless->WirelessSignalStrengthGet(nStrength);
	}
	else
	{
		return stiRESULT_WIRELESS_NOT_AVAILABLE;
	}
}


/*!
* \brief Start the wireless device
* \retval stiHResult
*/
stiHResult CstiNetwork::WirelessDeviceStart ()
{
	CstiEvent oEvent (eWIRELESS_DEVICE_START);

	return MsgSend (&oEvent, sizeof (oEvent));
}


EstiResult CstiNetwork::EventWirelessDeviceStart (CstiEvent *poEvent)
{
	StateSet (estiNEGOTIATING);
	EstiResult eResult = estiOK;
/*	if (stiIS_ERROR (m_pWireless->WirelessDeviceStart ())
	{
		eResult = estiERROR;
	}*/
	StateSet (estiIDLE);
	return eResult;
}

stiHResult CstiNetwork::WirelessParamCmdSet(char *pszCommand) const
{
	return m_pWireless->WirelessParamCmdSet(pszCommand);
}

/*!
* \brief Stop the wireless device
* \retval stiHResult
*/
stiHResult CstiNetwork::WirelessDeviceStop ()
{
	CstiEvent oEvent (eWIRELESS_DEVICE_STOP);

	return MsgSend (&oEvent, sizeof (oEvent));
}


EstiResult CstiNetwork::EventWirelessDeviceStop (CstiEvent *poEvent)
{
	StateSet (estiNEGOTIATING);
	EstiResult eResult = estiOK;
/*	if (stiIS_ERROR (m_pWireless->WirelessDeviceStop ())
	{
		eResult = estiERROR;
	}*/
	StateSet (estiIDLE);
	return eResult;
}

#endif // #ifdef stiWIRELESS
#endif

EstiResult CstiNetwork::EventTimer (
	CstiEvent *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	stiLOG_ENTRY_NAME (CstiNetwork::EventNetworkConnectionLost);

	auto  Timer = reinterpret_cast<stiWDOG_ID>((dynamic_cast<CstiExtendedEvent *>(poEvent))->ParamGet (0)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	
	if (Timer == m_wdConnectionLost)
	{
		bool bConnected = false;
		hResult = EthernetCableConnectedGet(&bConnected);

		if (bConnected)
		{
			if (!m_bConnected)
			{
				wiredNetworkConnectedSignal.Emit ();
				m_bConnected = true;
			}
		}
		else
		{
			if (m_bConnected)
			{
				wiredNetworkDisconnectedSignal.Emit ();
				m_bConnected = false;
			}
		}
		
		TimerStart (m_wdConnectionLost, unCHECK_CONNECTION_INTERVAL, 0);
	}
	
	return (estiOK);
} // end CstiConferenceManager::CollectStatsHandle


/*!
* \brief Shuts down the task.
* \param poEvent is a pointer to the event being processed.  Not used in this function.
* \retval none
*/
stiHResult CstiNetwork::EventTaskShutdown (
	CstiEvent *poEvent)  // The event to be handled
{
	stiLOG_ENTRY_NAME (CstiNetwork::EventShutdown);

	//
	// Call the base class shutdown.
	//
	return CstiOsTaskMQ::ShutdownHandle ();

} // end CstiNetwork::EventTaskShutdown


std::string CstiNetwork::localIpAddressGet(
	EstiIpAddressType addressType,
	const std::string& destinationIpAddress) const
{
	std::string ipAddress;

	stiASSERT(addressType == estiTYPE_IPV4);

	if (addressType == estiTYPE_IPV4)
	{
		SstiNetworkSettings settings;

		auto hResult = SettingsGet (&settings);

		if (!stiIS_ERROR(hResult))
		{
			ipAddress = settings.IPv4IP;
		}
	}

	return ipAddress;
}


stiHResult CstiNetwork::SettingsGet (
	SstiNetworkSettings *pstSettings) const
{
	stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);
	struct ifaddrs *pAddrs {nullptr}; // linked list
	char szIPAddress[NI_MAXHOST];

	*pstSettings = m_Settings;

	auto nResult = getifaddrs (&pAddrs);
	stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

	for (ifaddrs *pAddr = pAddrs; pAddr; pAddr = pAddr->ifa_next)
	{
		if (pAddr->ifa_addr)
		{
			if (pAddr->ifa_name == NetworkDeviceNameGet())
			{
				if (pAddr->ifa_addr->sa_family == AF_INET)
				{
					nResult = getnameinfo (pAddr->ifa_addr,
							sizeof (struct sockaddr_in),
							szIPAddress, sizeof(szIPAddress), nullptr, 0, NI_NUMERICHOST);

					pstSettings->bUseIPv4 = true;
					pstSettings->IPv4IP = szIPAddress;
				}
#ifdef IPV6_ENABLED
				else if (pAddr->ifa_addr->sa_family == AF_INET6)
				{
					nResult = getnameinfo (pAddr->ifa_addr,
							sizeof (struct sockaddr_in6),
							szIPAddress, sizeof(szIPAddress), NULL, 0, NI_NUMERICHOST);

					// finding ipv6 addresses in this way results in the adapter appended as the scope,
					// like fe80::baca:3aff:fe94:d4b2%eth0 so we need to strip that off
					char *pszPercent = strchr (szIPAddress, '%');
					if (pszPercent)
					{
						pszPercent[0] = '\0';
					}

					//
					// Append the scope as an integer and use brackets.
					//
					// Some versions of getnameinfo accept a flag that makes it return
					// the scope as an integer instead of an adapter name.
					//
					sockaddr_in6 *pIfaAddr = (sockaddr_in6 *)pAddr->ifa_addr;

					char buffer[IPV6_ADDRESS_LENGTH + 1];

					sprintf(buffer, "[%s]%%%d", szIPAddress, pIfaAddr->sin6_scope_id);

					pstSettings->IPv6IP = buffer;
					pstSettings->bUseIPv6 = true;
				}
#endif // IPV6_ENAVBLED
			}
		}
	}

STI_BAIL:

	if (pAddrs)
	{
		freeifaddrs (pAddrs);
	}

	return stiRESULT_SUCCESS;
}


/*!
* \brief Calls the proper scripts, etc to set the various network settings on the system
*/
stiHResult CstiNetwork::SettingsSet (
	const SstiNetworkSettings *pstSettings, ///< A structure containing the network settings to be applied.
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	++m_unNextRequestId;
	
	if (m_unNextRequestId == 0)
	{
		++m_unNextRequestId;
	}
	
	CstiEvent oEvent (eSETTINGS_SET, m_unNextRequestId);

	StateSet (estiNEGOTIATING);

	//
	// Save the new structure and send the event
	// only if a change is detected.
	//
	if (pstSettings != &m_Settings)
	{
		if (pstSettings->bIPv4UseDHCP)
		{
			m_BackupSettings = m_Settings;
			m_Settings.bIPv4UseDHCP = pstSettings->bIPv4UseDHCP;
#ifdef stiWIRELESS
			m_Settings.eNetwork = pstSettings->eNetwork;
			m_Settings.eWirelessNetwork = pstSettings->eWirelessNetwork;
			strcpy(m_Settings.szSsid, pstSettings->szSsid);
			strcpy(m_Settings.szKey, pstSettings->szKey);
			strcpy(m_Settings.szKeyIndex, pstSettings->szKeyIndex);
			strcpy(m_Settings.szAuthentication, pstSettings->szAuthentication);
			strcpy(m_Settings.szEncryption, pstSettings->szEncryption );
			strcpy(m_Settings.szIdentity, pstSettings->szIdentity);
			strcpy(m_Settings.szPrivateKeyPasswd, pstSettings->szPrivateKeyPasswd);
#endif
			m_Settings.NetHostname.assign (pstSettings->NetHostname);
		}
		else
		{
			m_BackupSettings = m_Settings;
			m_Settings = *pstSettings;
		}
	}

	hResult = MsgSend (&oEvent, sizeof (oEvent));

//STI_BAIL:

	if (punRequestId)
	{
		*punRequestId = m_unNextRequestId;
	}
	
	Unlock ();
	
	return hResult;
}


/*!
* \brief Validates the subnet mask.
*/
static stiHResult SubnetMaskValidate (
	const char *pszSubnetMask)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32SubnetMask;
	
	//
	// Make sure the one's complement of the subnet mask plus one is a power of 2.
	//
	hResult = IPAddressToIntegerConvert (pszSubnetMask, &un32SubnetMask);
	stiTESTRESULT ();
	
	stiTESTCOND (un32SubnetMask != 0, stiRESULT_ERROR);
	stiTESTCOND ((((~un32SubnetMask) + 1) & (~un32SubnetMask)) == 0, stiRESULT_ERROR);
	
STI_BAIL:

	return hResult;
}


/*!
* \brief Validates the IP address.
*/
stiHResult IPValidate (
	const char *pszIPAddress,
	const char *pszSubnetMask)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress;
	uint32_t un32SubnetMask;
	
	hResult = IPAddressToIntegerConvert (pszIPAddress, &un32IPAddress);
	stiTESTRESULT ();
	
	//
	// Make sure the most significant octet is between 1 and 223, inclusive.
	//
	stiTESTCOND ((un32IPAddress & 0xFF000000) > 0 && (un32IPAddress & 0xFF000000) <= 0xDF000000,
				 stiRESULT_ERROR);
	
	//
	// Make sure the host address is not zero or all ones.
	//
	hResult = IPAddressToIntegerConvert (pszSubnetMask, &un32SubnetMask);
	stiTESTRESULT ();
	
	stiTESTCOND ((un32IPAddress & ~un32SubnetMask) != 0, stiRESULT_ERROR);
	stiTESTCOND ((un32IPAddress & ~un32SubnetMask) != ~un32SubnetMask, stiRESULT_ERROR);
	
STI_BAIL:

	return hResult;
}


/*!
* \brief Validates the IP address.
*/
stiHResult DNSIPValidate (
	const char *pszIPAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress;
	
	hResult = IPAddressToIntegerConvert (pszIPAddress, &un32IPAddress);
	stiTESTRESULT ();
	
	//
	// Make sure the most significant octet is between 1 and 223, inclusive.
	//
	stiTESTCOND ((un32IPAddress & 0xFF000000) > 0 && (un32IPAddress & 0xFF000000) <= 0xDF000000,
				 stiRESULT_ERROR);
	
STI_BAIL:

	return hResult;
}


static stiHResult DNSValidate (
	const char *pszDNS1,
	const char *pszDNS2,
	IstiNetwork::EstiError *peError)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (pszDNS1 && *pszDNS1)
	{
		hResult = DNSIPValidate(pszDNS1);
	
		if (stiIS_ERROR (hResult))
		{
			*peError = IstiNetwork::estiErrorDNS1;
			
			stiTHROW (stiRESULT_ERROR);
		}
	}
	
	if (pszDNS2 && *pszDNS2)
	{
		hResult = DNSIPValidate(pszDNS2);
	
		if (stiIS_ERROR (hResult))
		{
			*peError = IstiNetwork::estiErrorDNS2;
			
			stiTHROW (stiRESULT_ERROR);
		}
	}

STI_BAIL:

	return hResult;
}
		

/*!
* \brief Calls the proper scripts, etc to set the various network settings on the system
*/
EstiResult CstiNetwork::EventSettingsSet (
	CstiEvent *poEvent) ///< The event to handle.
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	unsigned int unRequestId = ((CstiEvent *)poEvent)->ParamGet ();
	
#if 0
#ifdef stiWIRELESS

	if (m_Settings.eNetwork == NetworkType::WiFi )
	{
		//stiHResult hRes = stiRESULT_WIRELESS_CONNECTION_FAILURE;
		switch (m_Settings.eWirelessNetwork)
		{
			case estiWIRELESS_OPEN:
			{
				hResult = WirelessConnect(m_Settings.szSsid);
				stiTESTRESULT ();
				break;
			}
			
			case estiWIRELESS_WEP:
			{
				WEP sWep;
				
				sWep.pszSsid = m_Settings.szSsid;
				sWep.pszKey_index = m_Settings.szKeyIndex;
				sWep.pszKey = m_Settings.szKey;

				hResult = WirelessConnect(&sWep);
				stiTESTRESULT ();

				break;
			}
			
			case estiWIRELESS_WPA:
			{
				WpaPsk sWpaPsk;
				
				sWpaPsk.pszSsid = m_Settings.szSsid;
				sWpaPsk.pszKey = m_Settings.szKey;
				sWpaPsk.pszAuthentication = m_Settings.szAuthentication;
				sWpaPsk.pszEncryption = m_Settings.szEncryption;

				hResult = WirelessConnect(&sWpaPsk);
				stiTESTRESULT ();

				break;
			}
			
			case estiWIRELESS_EAP:
			{
				WpaEap sWpaEAP;
				
				sWpaEAP.pszSsid = m_Settings.szSsid;
				sWpaEAP.pszPrivateKeyPasswd = m_Settings.szPrivateKeyPasswd;
				sWpaEAP.pszIdentity  = m_Settings.szIdentity;

				hResult = WirelessConnect(&sWpaEAP);
				stiTESTRESULT ();

				break;
			}
			
			default:
				break;
		}

	   // todo: find out if this is a change from wired to wire
	}
#endif
#endif

	if (m_Settings.bIPv4UseDHCP)
	{
		hResult = NetstartSettingsUpdate (&m_Settings);
		stiTESTRESULT ();


#if DEVICE != DEV_X86 && DEVICE != DEV_NTOUCH_ANDROID
		int nResult = sethostname (m_Settings.NetHostname.c_str (), m_Settings.NetHostname.size ()));
		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
#endif

		// If we are still waiting for a DHCP request, decrement our counter because
		// we will only receive one response.
		if (m_unDHCPRequestId)
		{
			--m_stState.nNegotiateCnt;
		}

		m_unDHCPRequestId = unRequestId;
		m_oDHCP.Acquire (NetworkDeviceNameGet().c_str());
	}
	else
	{
		m_oDHCP.Release ();

		hResult = NetstartSettingsBackup ();
		//stiTESTRESULT ();

		hResult = SubnetMaskValidate (m_Settings.IPv4SubnetMask.c_str ());
		
		if (stiIS_ERROR (hResult))
		{
			m_eError = estiErrorSubnetMask;
			
			networkSettingsErrorSignal.Emit ();

			StateSet (estiERROR);
		}
		else
		{
			hResult = IPValidate (m_Settings.IPv4IP.c_str (), m_Settings.IPv4SubnetMask.c_str ());
			
			if (stiIS_ERROR (hResult))
			{
				m_eError = estiErrorIP;
				
				networkSettingsErrorSignal.Emit ();

				StateSet (estiERROR);
			}
			else
			{
				hResult = DNSValidate (m_Settings.IPv4DNS1.c_str (), m_Settings.IPv4DNS2.c_str (), &m_eError);
				
				if (stiIS_ERROR (hResult))
				{
					networkSettingsErrorSignal.Emit ();

					StateSet (estiERROR);
				}
				else
				{
					hResult = NetstartSettingsUpdate (&m_Settings);
					stiTESTRESULT ();

			#if DEVICE != DEV_X86
					int nSysStatus = system (NETWORK_SCRIPT);
			#else
					int nSysStatus = 0;
			#endif
					nSysStatus = WEXITSTATUS (nSysStatus);

					if (nSysStatus != 0)
					{
						//
						// Something went wrong.  Restore the settings from the backups.
						//
						NetstartSettingsRestore ();
						m_Settings = m_BackupSettings;

						switch (nSysStatus)
						{
							case nNETSTART_RESULT_DHCP_FAILURE:

								m_eError = estiErrorDHCP;

								break;

							case nNETSTART_RESULT_SUBNET_MASK_FAILURE:

								m_eError = estiErrorSubnetMask;

								break;

							case nNETSTART_RESULT_ROUTE_FAILURE:

								m_eError = estiErrorGateway;

								break;

							default:

								m_eError = estiErrorGeneric;

								break;
						}

						networkSettingsErrorSignal.Emit ();

						StateSet (estiERROR);
					}
					else
					{

					#ifdef stiWIRELESS
						if (m_Settings.eNetwork == NetworkType::WiFi)
						{
							stiOSSetNIC(USE_WIRELESS_NETWORK_DEVICE);
						} else
						{
							stiOSSetNIC(USE_HARDWIRED_NETWORK_DEVICE);
						}
					#endif

						networkSettingsChangedSignal.Emit ();

						StateSet (estiIDLE);
					}
				}
			}
		}
	}

STI_BAIL:

	Unlock ();

	return estiOK;
}


/*!
* \brief Set the in-call status
* \retval none
*/
void CstiNetwork::InCallStatusSet (bool bSet)
{
	// ACTION - NETWORK - Need to implement.
}


int CstiNetwork::Task ()
{
	stiLOG_ENTRY_NAME (CstiNetwork::Task);

	//
	// Initialize loop variables
	//
	EstiBool bContinue = estiTRUE;
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	char buffer[stiNETWORK_MAX_MSG_SIZE + 1];

	if (m_wdConnectionLost)
	{
		TimerStart (m_wdConnectionLost, unCHECK_CONNECTION_INTERVAL, 0);
	}

	//
	// Task loop
	// As long as we don't get a SHUTDOWN message
	//
	while (bContinue)
	{
		//
		// Read a message from the message queue
		//
		if (stiRESULT_SUCCESS == MsgRecv (buffer, stiFP_MAX_MSG_SIZE))
		{
			//
			// A message was recieved. Process the message.
			//
			auto  poEvent = reinterpret_cast<CstiEvent*>(buffer); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

			//
			// Lookup and handle the event
			//
			hResult = EventDoNow (poEvent);

			if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
			{
				//
				// Set the continue flag to false
				//
				bContinue = estiFALSE;
			}

		}// end if (CstiOsTask::eOK == eMsgResult)

	}// end while (bContinue)

	return (0);
}


/*! \brief Check if IP Octet value is valid
*
* The Octet value must be between 0 and 255.
* \retval true If the octet value is between 0 and 255.
* \retval false Otherwise.
*/
static bool IPOctetValidate (
	const char *pszOctet)
{
	bool bValid = false;
	size_t Length = strlen (pszOctet);
	size_t i;

	if (Length >= 1 && Length <= (size_t)nIP_OCTET_LENGTH)
	{
		// Look at each character in the segment
		for (i = 0; i < Length; i++)
		{
			// Is the current character not a digit
			if (!isdigit (pszOctet[i]))
			{
				// yes! - not a valid ip address.
				break;
			} // end if
		} // end for

		// Is the number in this segment less than 256?
		if (i == Length && atoi (pszOctet) <= un8IP_OCTET_MAX)
		{
			bValid = true;
		} // end if
	}

	return bValid;
}


///!\brief Converts an integer into an IP address string
///
///\returns The IP address in integer form, zero if the IP address is invalid
///
stiHResult IPAddressToIntegerConvert (
	const char *pszAddress,	///< The string containing the IP address to convert.
	uint32_t *pun32Value) ///< The value of the IP address as an integer
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress = 0;

	// Did we get a valid pointer?
	stiTESTCOND (pszAddress, stiRESULT_ERROR);

	{
		int nBits = 24;
		const char *pszDotPosition;
		const char *pszPrevPosition = pszAddress;

		// Is there a period in the number?
		while (nBits > 0 && nullptr != (pszDotPosition = strchr (pszPrevPosition, '.')))
		{
			// Is this segment less than 3 digits and greater than 0 digits?
			size_t Length = pszDotPosition - pszPrevPosition;

			stiTESTCOND((size_t)nIP_OCTET_LENGTH >= Length && 0 < Length, stiRESULT_ERROR);

			char szSegment[nIP_OCTET_LENGTH + 1];
			strncpy (szSegment, pszPrevPosition, Length);
			szSegment[Length] = '\0';

			bool bResult = IPOctetValidate (szSegment);

			stiTESTCOND(bResult, stiRESULT_ERROR);

			uint32_t un32Value = atoi (szSegment);
			un32IPAddress |= (un32Value << nBits);
			nBits -= 8;

			pszPrevPosition = pszDotPosition + 1;
		} // end while

		stiTESTCOND (nBits == 0, stiRESULT_ERROR);

		//
		// Do we need to shift on the last octet
		// and is the last octet valid?
		//
		bool bResult = IPOctetValidate (pszPrevPosition);

		stiTESTCOND (bResult, stiRESULT_ERROR);

		// Yes.
		uint32_t un32Value = atoi (pszPrevPosition);
		un32IPAddress |= (un32Value << nBits);
	}  // end if

STI_BAIL:

	if (!stiIS_ERROR (hResult))
	{
		*pun32Value = un32IPAddress;
	}

	return hResult;
}
