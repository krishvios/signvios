
/*!
* \file ServiceInterface.h
* \brief manager connman network services
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once

#include "stiDefs.h"
#include "stiSVX.h"
#include "IstiNetwork.h"

#include <string>

#include "NetworkDbusInterface.h"
#include "TechnologyInterface.h"
#include "CstiNetworkConnmanInterface.h"
#include "CstiSignal.h"


#define STATE_NONE			0x000
#define STATE_READY			0x001
#define STATE_METHOD		0x002
#define STATE_ADDRESS		0x004
#define STATE_NETMASK		0x008
#define STATE_GATEWAY		0x010
#define STATE_NAMESERVERS	0x020
#define STATE_DOMAINS		0x040
#define STATE_TIMESERVERS	0x080

#define HAVE_IPV4 (STATE_METHOD | STATE_ADDRESS | STATE_NETMASK | STATE_GATEWAY)
#define STATE_ONLINE (STATE_READY | HAVE_IPV4 | STATE_NAMESERVERS)

extern const std::string g_hiddenSsid;

enum class connectionError
{
	none = 0,
	inProgress = 1,
	alreadyConnected = 2,
	notConnected = 3,
	ioError = 4,
	other = 5,
};

struct ipv4
{
	std::string method;
	std::string address;
	std::string netmask;
	std::string gateway;
};

//future use
struct ipv6
{
	std::string method;
	std::string address;
	uint8_t prefixLength{0};
	std::string gateway;
	std::string privacy;
};

struct ethernet
{
	std::string method;
	std::string interfaceName;
	std::string address;
	uint16_t mtu{0};
	uint16_t speed{0};
};

class NetworkService : public NetworkDbusInterface
{

public:

	NetworkService () = delete;

	NetworkService(
		DbusObject *dbusObject)
	:
		NetworkDbusInterface(dbusObject, CONNMAN_SERVICE_INTERFACE)
	{
	}

	~NetworkService() override;

	void added(
		GVariant *properties) override;

	void propertiesProcess(GVariant *properties);

	void propertySet(
		const std::string &property,
		GVariant *value);

	void propertyClear(
		const std::string &property);

	void serviceConnect();
	void serviceDisconnect();
	void serviceRemove();
	connectionError synchronousDisconnect();

	inline bool isHidden () { return (objectPathGet().find(g_hiddenSsid) != std::string::npos); };

	void enableDHCP ();
	void disableDHCP (
		const SstiNetworkSettings &settings);

public:			// signals
	static CstiSignal<std::shared_ptr<NetworkService>> addedSignal;
	static CstiSignal<std::shared_ptr<NetworkService>> serviceConnectedSignal;
	static CstiSignal<std::shared_ptr<NetworkService>, connectionError> serviceConnectionErrorSignal;
	static CstiSignal<std::shared_ptr<NetworkService>, bool> serviceDisconnectedSignal;

	static CstiSignal<std::shared_ptr<NetworkService>, bool> serviceOfflineStateChangedSignal;
	static CstiSignal<std::shared_ptr<NetworkService>> serviceOnlineStateChangedSignal;

	static CstiSignal<std::shared_ptr<NetworkService>, const std::string &> specificServiceGetSignal;

	static CstiSignal<> newWiredConnectedSignal;

	static CstiSignal<std::shared_ptr<NetworkService>, bool> propertySetFinishedSignal;

public:			//member variables

	static constexpr const char *methodDhcp = "dhcp";
	static constexpr const char *methodManual = "manual";
	static constexpr const char *methodAuto = "auto";
	static constexpr const char *methodOff = "off";
	static constexpr const char *method6to4 = "6to4";

	EstiServiceState m_serviceState{estiServiceDisconnect};
	std::string m_serviceError;
	std::string m_serviceName;
	technologyType m_technologyType {technologyType::unknown};
	EstiWirelessType m_securityType {estiWIRELESS_UNDEFINED};
	uint8_t m_signalStrength {0};
	bool m_favorite {false};
	bool m_immutable {false};
	bool m_autoConnect {false};
	bool m_roaming {false};			// not used;
	std::vector<std::string> m_nameServers;
	std::vector<std::string> m_timeServers;
	std::vector<std::string> m_domains;
	struct ipv4 m_ipv4 {};
	struct ipv6 m_ipv6 {};
	struct ethernet m_ethernet {};

	bool m_useDhcp {false};
	bool m_cancelled {false};

	uint8_t m_progressState {0};
	uint8_t m_lastProgressState {0};

	bool m_disconnecting {false};

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	void eventServiceChanged(
		CstiNetworkService *proxy,
		const std::string &name,
		GVariant *value);

	static void serviceChangedCallback(
		CstiNetworkService *proxy,
		const gchar *name,
		GVariant *value,
		gpointer userData);

	void propertySetFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void propertyClearFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void serviceConnectFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void serviceDisconnectFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void serviceRemoveFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void serviceStateCheck ();
	void stateSet(std::string state);
	void typeSet(std::string type);
	void securitySet(std::string security);
	void stringListGet(GVariant *values, std::vector<std::string> *list);
	void ipv4Parse(GVariant *value);
	void ipv6Parse(GVariant *values);
	void ethernetParse(GVariant *values);

	static constexpr const char *propertyState = "State";

	static constexpr const char *propertyError = "Error";
	static constexpr const char *propertyName = "Name";
	static constexpr const char *propertyType = "Type";
	static constexpr const char *propertySecurity = "Security";
	static constexpr const char *propertyStrength = "Strength";
	static constexpr const char *propertyFavorite = "Favorite";
	static constexpr const char *propertyImmutable = "Immutable";
	static constexpr const char *propertyAutoConnect = "AutoConnect";
	static constexpr const char *propertyRoaming = "Roaming";			// not interested in this
	static constexpr const char *propertyNameServers = "Nameservers";						// readonly
	static constexpr const char *propertyNameServersConfig = "Nameservers.Configuration";	// read/write
	static constexpr const char *propertyTimeServers = "Timeservers";						// readonly
	static constexpr const char *propertyTimeServersConfig = "Timeservers.Configuration";	// read/write
	static constexpr const char *propertyDomains = "Domains";								// readonly
	static constexpr const char *propertyDomainsConfig = "Domains.Configuration";			// read/write

	static constexpr const char *propertyIpv4 = "IPv4";										// all readonly
	static constexpr const char *propertyIpv4Method = "Method";
	static constexpr const char *propertyAddress = "Address";								// used here and ipv6
	static constexpr const char *propertyIpv4Netmask = "Netmask";
	static constexpr const char *propertyGateway = "Gateway";								// ipv4 and ipv6
	static constexpr const char *propertyIpv4Config = "IPv4.Configuration";					// read/write, use same sub properties as above

	static constexpr const char *propertyIpv6 = "IPv6";
	static constexpr const char *propertyMethod = "Method";								// ipv6 and proxy
	static constexpr const char *propertyIpv6PrefixLength = "PrefixLength";
	static constexpr const char *propertyIpv6Privacy = "Privacy";
	static constexpr const char *propertyIpv6Config = "Ipv6.Configuration";

	static constexpr const char *stateIdle = "idle";
	static constexpr const char *stateFailure = "failure";
	static constexpr const char *stateAssociation = "association";
	static constexpr const char *stateConfiguration = "configuration";
	static constexpr const char *stateReady = "ready";
	static constexpr const char *stateDisconnect = "disconnect";
	static constexpr const char *stateOnline = "online";
/*
 * right now we don't do anything with proxies or providers
 */
#if 0
	static constexpr const char *propertyProxy = "Proxy";
	static constexpr const char *propertyProxyUrl = "URL";
	static constexpr const char *propertyProxyServers = "Servers";
	static constexpr const char *propertyProxyExcludes = "Excludes";
	static constexpr const char *propertyProxyConfig = "Proxy.Configuration";

	static constexpr const char *propertyProvider = "Provider";
	static constexpr const char *propertyProviderHost = "Host";
	static constexpr const char *propertyProviderDomain = "Domain";
	// providerName and providerType can use prior named properties
#endif

	static constexpr const char *propertyWifi = "wifi";
	static constexpr const char *propertyEthernet = "Ethernet";
	static constexpr const char *propertyEthernetInterface = "Interface";
	static constexpr const char *propertyEthernetMtu = "MTU";
	static constexpr const char *propertyBluetooth = "bluetooth";
	static constexpr const char *propertySecurityNone = "none";
	static constexpr const char *propertySecurityWep = "wep";
	static constexpr const char *propertySecurityEap = "ieee8021x";
	// method and interface use prior named properties
	// speed and duplex deprecated and unavailable

	long int m_serviceChangedSignalHandlerId = 0;
	CstiNetworkService *m_serviceProxy = nullptr;

	std::queue<GVariant *> m_queuedProperties;

	void blockHandle();

	bool m_blocked {false};
};
