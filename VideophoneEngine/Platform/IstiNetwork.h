/*!
* \file IstiNetwork.h
* \brief Interface for the CstiNetwork class.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef ISTINETWORK_H
#define ISTINETWORK_H

#include "stiSVX.h"
//#include "stiDefs.h"
#include "stiError.h"
#include "ISignal.h"
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	#include "stiWirelessDefs.h"
#endif

//
// Constants
//
const int nMAX_HOST_NAME_LEN = 64; 			//! See linux/include/asm-arm/param.h
const int nMAX_SSID_LEN = 33;  				//! See NDIS_802_11_LENGTH_SSID in Platform/common/wireless/include/oid.h
const int nMAX_KEY_LEN = 256;  				//! Key Length (ie password)
const int nMAX_KEY_INDEX_LEN = 2;  			//! Key Index Length 
const int nMAX_PASSPHRASE_LEN = 256;		//! PassKey Phrase Length (ie password)
const int nMAX_PATH_LEN = 256;  			//! See ops.h
const int nMAX_AUTHENTICATION = 10;         //! Authentication type
const int nMAX_ENCRYPTION = 10;             //! Encryption type
const int nMAX_IDENTITY = 256;              //! User name for EAP connections
const int nMAX_PRIVATE_KEY_PASSWD = 256;    //! Private Key or password for EAP connections
const int nMAX_WIRELESS_PROTOCOL_LEN = 10;  //!
const int nMAX_MISC_LEN = 16;

/*! 
* \enum EstiWirelessType
* 
* \brief Which type of wireless security is being used for this connection.
* 
*/ 
enum EstiWirelessType
{
	estiWIRELESS_UNDEFINED = 0x0000,      //! No security specified
	estiWIRELESS_OPEN = 0x0001,      //! Open or no security
	estiWIRELESS_WEP  = 0x0002,      //! WEP encryption
	estiWIRELESS_WPA  = 0x0004,      //! WPA type security
	estiWIRELESS_EAP  = 0x0008       //! EAP (enterprise) security
};

enum EstiEapType
{
	estiEAP_NONE = 0,
	estiEAP_PEAP = 1,
	estiEAP_TLS = 2,
	estiEAP_TTLS = 3
};

enum EstiPhase2Type
{
	estiPHASE2_NONE = 0,
	estiPHASE2_MSCHAPV2 = 1,
	estiPHASE2_GTC = 2,
	estiPHASE2_TLS = 3
};

enum EstiIPv6Method
{
	estiIPv6MethodManual = 0,
	estiIPv6MethodAutomatic = 1
};

enum EstiServiceState
{
	estiServiceFailure = -1,
	estiServiceDisconnect = 0,
	estiServiceIdle,
	estiServiceAssociation,
	estiServiceConfiguration,
	estiServiceReady,
	estiServiceOnline
};


/*! 
 * \struct SstiNetworkSettings 
 *  
 * \brief 
 *  
 */
struct SstiNetworkSettings
{
	SstiNetworkSettings ()
	{
		szSsid[0] = '\0';
		szKey[0] = '\0';						//! deprecated
		szKeyIndex[0] = '\0';					//! deprecated
		szAuthentication[0] = '\0';			//! deprecated
		szEncryption[0] = '\0';				//! deprecated
		szIdentity[0] = '\0';					//! deprecated
		szPrivateKeyPasswd[0] = '\0';			//! deprecated
	}

	// Common
	std::string NetHostname;			//! The host name.  If empty the default will be used

	// IPv4
	bool bUseIPv4{true};		//! true or false to signify the desire to use IPv4
	bool bIPv4UseDHCP{true}; 	//! true or false to signify the desire to use DHCP in IPv4
	std::string IPv4IP;				//! The IP address or empty
	std::string IPv4SubnetMask;		//! The subnet mask or empty
	std::string IPv4NetGatewayIP;	//! The gateway or empty
	std::string IPv4DNS1;			//! The first DNS server
	std::string IPv4DNS2;			//! The second DNS server

	//IPv6
	bool bUseIPv6{false};		//! true or false to signify the desire to use IPv6
	EstiIPv6Method eIPv6Method{estiIPv6MethodAutomatic}; 	//! true or false to signify the desire to use DHCP in IPv6
	std::string IPv6IP;				//! The IP address or empty
	int nIPv6Prefix{0};				//! The prefix (equivalent to subnet mask)
	std::string IPv6GatewayIP;		//! The gateway or empty
	std::string IPv6DNS1;			//! The first DNS server
	std::string IPv6DNS2;			//! The second DNS server

	NetworkType eNetwork{NetworkType::Wired};								//! The network type (wired, wireless)
	EstiWirelessType eWirelessNetwork{estiWIRELESS_OPEN};					//! The network authentication type (WPA,WPA2,WEP,EAP,Open)
	char szSsid[nMAX_SSID_LEN]{};                       //! Current SSID
	char szKey[nMAX_KEY_LEN]{};                         //! Deprecated
	char szKeyIndex[nMAX_KEY_INDEX_LEN]{};              //! Deprecated
	char szAuthentication[nMAX_AUTHENTICATION]{};       //! Deprecated
	char szEncryption[nMAX_ENCRYPTION]{};               //! Deprecated
	char szIdentity[nMAX_IDENTITY]{};                   //! Deprecated
	char szPrivateKeyPasswd[nMAX_PRIVATE_KEY_PASSWD]{}; //! Deprecated
	EstiEapType eEapType{estiEAP_NONE};					//! type of EAP connection for enterprise wap
	EstiPhase2Type ePhase2Type{estiPHASE2_NONE};		//! type of pahse 2 auth for enterprise wap
};


/*!
 * \struct SstiNetworkDevice
 *
 * \brief Describes the basics of a network device
 *
 */
struct SstiNetworkDevice
{
	std::string name;
	EstiIpAddressType eAddressType{estiTYPE_IPV4};
	NetworkType networkType{NetworkType::None};
};

//
// Forward Declarations
//


/*! 
*   \class IstiNetwork
* 
*/ 
class IstiNetwork
{
public:
	/*! 
	* \enum EstiState
	* 
	* \brief Denotes current network object state
	* 
	*/ 
	enum EstiState
	{
		estiUNKNOWN,
		estiIDLE,			//! No network changes are occurring.
		estiNEGOTIATING,	//! Network modifications are occurring.
							//! This could include UPnP mapping, DHCP negotiations, writing
							//! network files, restarting the protocol stack(s), restarting the
							//! network device, etc.
		estiERROR,
	};

	/*! 
	* \enum EstiError
	* 
	* \brief Error codes from getting a network connection
	* 
	*/ 
	enum EstiError
	{
		estiErrorNone,                 //! Error: None              
		estiErrorIP,                   //! Error: IP                
		estiErrorSubnetMask,           //! Error: SubnetMask        
		estiErrorGateway,              //! Error: Gateway           
		estiErrorGeneric,              //! Error: Generic           
		estiErrorDHCP,                 //! Error: DHCP              
		estiErrorDNS1,                 //! Error: DNS1              
		estiErrorDNS2,                 //! Error: DNS2              
		estiErrorWirelessAccessPoint   //! Error: Wireless Access Point
	};

	IstiNetwork (const IstiNetwork &other) = delete;
	IstiNetwork (IstiNetwork &&other) = delete;
	IstiNetwork &operator= (const IstiNetwork &other) = delete;
	IstiNetwork &operator= (IstiNetwork &&other) = delete;

	/*!
	 * \brief Obtain the handle to the network object.  Through this handle, all other calls will be made.
	 *
	 * \return The handle to the Network object.
	 */
	static IstiNetwork *InstanceGet ();

	/*!
	 * \brief Startup the network
	 *
	 * \return stiHResult
	 */
	virtual stiHResult Startup () = 0;

	/*!
	 * \brief Get the Network Settings
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult SettingsGet (SstiNetworkSettings *) const = 0;

	/*!
	 * \brief Set Settings
	 * 
	 * \param pstSettings
	 * \param punRequestId
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult SettingsSet (
		const SstiNetworkSettings *pstSettings,
		unsigned int *punRequestId) = 0;
		
	/*!
	 * \brief Set in call status
	 * 
	 * \param bSet
	 */
	virtual void InCallStatusSet (bool bSet) = 0;

	virtual void networkInfoSet (
		NetworkType type,
		const std::string &data) = 0;
	
	virtual NetworkType networkTypeGet () const = 0;
	virtual std::string networkDataGet () const = 0;
	virtual std::string localIpAddressGet(EstiIpAddressType addressType, const std::string& destinationIpAddress) const = 0;
	
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	/*!
	 * \brief Get Network State
	 *
	 * \return EstiState
	 */
	virtual EstiState StateGet () const = 0;

	/*!
	 * \brief Get Network Error
	 * 
	 * \return EstiError 
	 */
	virtual EstiError ErrorGet () const = 0;

#if defined stiWIRELESS
	//virtual stiHResult WAPConnect (SstiSSIDListEntry *) = 0;		/// Connects to a particular SSID

	/*!
	 * \brief Get Wireless Access Point List
	 *
	 * \return stiHResult
	 */
	virtual stiHResult WAPListGet (WAPListInfo *) const = 0;		///! Gets the Wireless Access Points seen by the radio

	/*!
	 * \brief Is wireless available?
	 *
	 * \return stiHResult
	 */
	virtual bool WirelessAvailable() const = 0;			///! Tests if wireless driver is actually available.

	/*!
	 * \brief Is the wireless in use?
	 *
	 * \return int
	 */
	virtual int WirelessInUse() = 0;

	/*!
	 * \brief return the current network connection state
	 *
	 * \return state of the current service
	 */
	 virtual EstiServiceState ServiceStateGet() = 0;
#endif

	virtual stiHResult EthernetCableStatusGet (bool *pbConnected) = 0;

	/*!
	 * \brief Get the active Network Device Name
	 *
	 * \return std::string
	 */
	virtual std::string NetworkDeviceNameGet () const
	{
		return "";
	}

	/*!
	 * \brief set authentication values for wireless AP with security
	 *
	 * \param pszPassphrase - if the AP requires a passphrase
	 * \param pszUsername - enterprise AP requires a username
	 * \param pszSsid - 'mis-typed' hidden access point
	 *
	 * \return stiRESULT_SUCCESS
	 */
	virtual stiHResult WirelessCredentialsSet(const char *, const char *)
	{
		return (stiRESULT_SUCCESS);
	};

	/*!
	 * \brief ask the wireless nic to scan for new devices
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual stiHResult WirelessScan()
	{
		return (stiRESULT_SUCCESS);
	};

	/*!
	 * \brief set scanning-enabled mode. If enabled, wifi is automatically enabled 
	 * to allow scans to run for the duration of scanning-enabled mode
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual stiHResult WirelessScanningEnabledSet(bool)
	{
		return (stiRESULT_SUCCESS);
	};

	/*!
	 * \brief get scanning-enabled mode. 
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual bool WirelessScanningEnabledGet()
	{
		return true;
	};

	/*!
	 * \brief connect a service.  if the servicetype is wired the servicename should be a NULL
	 *             if the servicetype is wireless, the servicename will be the ssid
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual stiHResult ServiceConnect(NetworkType, const char *, int)
	{
		return (stiRESULT_SUCCESS);
	}

	/*!
	 * \brief cancel an in-progress wireless connection
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual stiHResult NetworkConnectCancel()
	{
		return (stiRESULT_SUCCESS);
	}

	/*!
	 * \brief power on or off the wifi nic
	 *
	 * \return stiRESULT_SUCCESS or an error
	 */
	virtual stiHResult WirelessEnable(bool)
	{
		return (stiRESULT_SUCCESS);
	}

	/*!
	 * \brief get the signal strength of the named wireless access point
	 *
	 * \return the signal strength (0-100)
	 */
	virtual int WirelessSignalStrengthGet(const char *pszName)
	{
		stiUNUSED_ARG (pszName);

		return (0);
	}

	/*!
	 * \brief get the mac address of the wireless network radio
	 *
	 * \return the string representation of the address
	 */
	virtual std::string WirelessMacAddressGet()
	{
		return "";
	}

	/*!
	 * \brief return a list of strings containing the SSIDs of services
	 * that we have connected to in the past
	 *
	 * return an empty list if no previous connections, otherwise a list of SSIDs
	 */
	virtual std::list<std::string> RememberedServicesGet()
	{
		return {};
	}

	/*!
	 * \brief remove a known service
	 *
	 * param - name of the service to remove
	 *
	 * return stiRESULT_SUCCESS on success otherwise failure
	 */
	virtual stiHResult RememberedServiceRemove(std::string)
	{
		return (stiRESULT_SUCCESS);
	}

#endif

public:			// signals
	virtual ISignal<EstiState>& networkStateChangedSignalGet () = 0;
	virtual ISignal<bool>& wirelessScanCompleteSignalGet () = 0;
	virtual ISignal<>& networkSettingsChangedSignalGet () = 0;
	virtual ISignal<>& networkSettingsErrorSignalGet () = 0;
	virtual ISignal<>& networkSettingsSetSignalGet () = 0;
	virtual ISignal<>& wiredNetworkDisconnectedSignalGet () = 0;
	virtual ISignal<>& wirelessNetworkDisconnectedSignalGet () = 0;
	virtual ISignal<>& enterpriseNetworkSignalGet () = 0;
	virtual ISignal<std::string>& networkNeedsCredentialsSignalGet () = 0;
	virtual ISignal<>& wirelessUpdatedSignalGet () = 0;
	virtual ISignal<>& networkUnsupportedSignalGet () = 0;
	virtual ISignal<>& wiredNetworkConnectedSignalGet () = 0;
	virtual ISignal<>& connmanResetSignalGet () = 0;
	virtual ISignal<>& wirelessPassphraseChangedSignalGet() = 0;

protected:
	IstiNetwork () = default;
	virtual ~IstiNetwork () = default;
};

#endif // #ifndef ISTINETWORK_H
