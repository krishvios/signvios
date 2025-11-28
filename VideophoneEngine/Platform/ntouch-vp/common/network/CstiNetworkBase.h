/*!
* \file CstiNetworkBase.h
* \brief Consolidation of all network tasks.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015-2019 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once

#include "stiDefs.h"
#include "stiSVX.h"
#include "BaseNetwork.h"
#include "CstiEventQueue.h"

#include "stiTools.h"

#include "NetworkManagerInterface.h"
#include "TechnologyInterface.h"
#include "ServiceInterface.h"
#include "CstiNetworkConnmanInterface.h"
#include "CstiTimer.h"
#include "CstiBluetooth.h"

#include "DbusError.h"
#include "DbusCall.h"

constexpr int RECONNECT_TIMEOUT_MS  = 30000;   // wait 30 seconds
constexpr int RESET_TIMEOUT_MS = 2000;			// wait 2 seconds on resets

enum class resetState {
	unknown,
	shuttingDown,
	restarting,
};

class CstiNetworkBase : public BaseNetwork, public CstiEventQueue
{

public:

	CstiNetworkBase (CstiBluetooth *bluetooth);

	~CstiNetworkBase () override;

	stiHResult Initialize (std::string pathName, std::string stopCommand, std::string startCommand);

	stiHResult Startup () override;

//	virtual stiHResult Shutdown ();

	EstiState StateGet () const override;
	EstiError ErrorGet () const override;

	stiHResult WAPListGet (WAPListInfo *) const override;
	bool WirelessAvailable () const override;
	int WirelessInUse() override;

	std::string localIpAddressGet(EstiIpAddressType addressType, const std::string& destinationIpAddress) const override;

	stiHResult SettingsGet (SstiNetworkSettings *pstSettings) const override;
	stiHResult SettingsSet (const SstiNetworkSettings *pstSettings, unsigned int *punRequestId) override;
	void InCallStatusSet (bool bSet) override;

	stiHResult EthernetCableStatusGet (bool *pbConnected) override;
	stiHResult WirelessScan() override;

	/*
	 * if the access point needs some input to finish the connection set them here.
	 */
	stiHResult WirelessCredentialsSet(const char *pszPassphrase, const char *pszUsername) override;

	/*!
	 * \brief connect a service
	 * \param ServiceType   wired or wireless
	 * \param ServiceName   if wireless the ssid name, else NULL
	 * \param Type          if hidden AP what kind of security
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	stiHResult ServiceConnect(
		NetworkType eServiceType,
		const char *pszServiceName,
		int nType) override;

	/*!
	 * \brief cancel an in-progress wireless connection
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	stiHResult NetworkConnectCancel() override;

	/*!
	 * \brief toggle wireless power
	 *
	 * \returns stiRESULT_SUCCESS or an error code
	 */
	stiHResult WirelessEnable(bool bState) override;

	/*!
	 * \brief gets the signal strength for the named wireless access point
	 *
	 * \returns signal strength (0-100)
	 */
	 int WirelessSignalStrengthGet(const char *szName) override;

	/*!
	 * \brief return the state of the current service
	 */
	EstiServiceState ServiceStateGet() override;

	/*!
	 * \brief return the interface name of the wired network
	 *
	 * \return Returns a std::string with the active network device name
	 */
	std::string NetworkDeviceNameGet () const override;

	const char *stateNameGet(EstiState state);

	void networkInfoSet (
		NetworkType type,
		const std::string &data) override;

	std::string WirelessMacAddressGet() override;
	std::list<std::string> RememberedServicesGet() override;
	stiHResult RememberedServiceRemove(std::string ssid) override;

	NetworkType networkTypeGet () const override;
	std::string networkDataGet () const override;

protected:

	/*!
	 * \brief Synchronize the wifi power state according to the current connection type and
	 * 			WirelessScanningEnabled value. It should be called after either value changes.
	 * 
	 * 			Note, this default implementation will cause scanning mode to always be enabled, 
	 * 			and wifi will never get turned off. This is how vp2 & 4 are configured.
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	virtual stiHResult syncWifiPowerState()
	{
		return (stiRESULT_SUCCESS);
	}

	std::shared_ptr<NetworkService> m_currentService;
	EstiError m_error = IstiNetwork::estiErrorNone;

private:

	struct SState
	{
		EstiState eState;
		int nNegotiateCnt;	// Used to handle multiple calls to change the state to negotiating
	};

	//
	// static functions
	//

	static gboolean ConnmanRequestInput(CstiNetworkAgent *, GDBusMethodInvocation *, gchar *, GVariant *, gpointer);
	static gboolean ConnmanRequestBrowser(CstiNetworkAgent *, GDBusMethodInvocation *, gchar *, gchar *, gpointer);
	static gboolean ConnmanReportError(CstiNetworkAgent *, GDBusMethodInvocation *, gchar *, gchar *, gpointer);
	static gboolean ConnmanRelease(CstiNetworkAgent *, GDBusMethodInvocation *, gchar *, gchar *, gpointer);

	void networkManagerCreate();

	void credentialsProcess (
		GDBusMethodInvocation *invocation,
		int inputType);

	static void ServicesChangedCallback(
		CstiNetworkManager *pDbusProxy,
		GVariant *Changed,
		char **Removed,
		gpointer userData);

	//
	// Event Handlers
	//
	void eventSettingsSet (unsigned int nextRequestId);
	void eventWirelessScan();
	void eventServiceConnect(int type, std::string ssid);
	void EventCurrentServiceDisconnected (std::shared_ptr<NetworkService>service);
	void eventNeedsCredentials (GDBusMethodInvocation *invocation, int inputType, std::string name);
	void eventInvalidKey (GDBusMethodInvocation *invocation, std::string);
	void eventServiceAdded (std::shared_ptr<NetworkService>);
	void eventServicesChanged (GVariant *changedServices, std::vector<std::string> removedServices);
	void eventNetworkConnectCancel ();
	void eventRequestBrowser (GDBusMethodInvocation *invocation, std::string serviceName);
	void eventNetworkManagerInterfaceAdded(std::shared_ptr<NetworkManagerInterface> networkManagerInterface);
	void eventScanFinished(bool success);
	void eventGetServices(GVariant *services);
	void eventGetServicesFinish(GVariant *services);
	void eventServicesRetrieved (GVariant *services);
	void eventTechnologiesRetrieved(GVariant *technologies);
	void eventTechnologyAdded(std::shared_ptr<NetworkTechnology>);
	void eventTechnologyRemoved(std::string objectPath);
	void eventTechnologySupplied(std::string objectPath, GVariant *properties);
	void eventRememberedServiceRemove(std::string ssid);

	void serviceOnlineStateChangedHandle(std::shared_ptr<NetworkService> service);
	void serviceOfflineStateChangedHandle(std::shared_ptr<NetworkService> service, bool);
	void serviceDisconnectedHandle(std::shared_ptr<NetworkService> service, bool success);
	void serviceConnectedHandle(std::shared_ptr<NetworkService> service);
	void serviceConnectionErrorHandle (std::shared_ptr<NetworkService> service, connectionError error);

	void specificServiceFoundHandle(const std::string &objectPath, GVariant *dict);

	void powerChangedSignalHandle(std::shared_ptr<NetworkTechnology>);

	void clearCredentialsInvocation ();
	void usePreviousService ();

	void processService (
		GVariant *variant);

	void serviceSignalsConnect();

	stiHResult ConnmanAgentUnregister();

	stiHResult newServiceConnect (bool disconnected);

	void stateSet (EstiState eState);

	bool settingsChanged();

	void objectRemove(
		std::string objectPath,
		std::string interfaceName);

	void wirelessMacAddressUpdate();
	void rememberedServicesUpdate();

	stiHResult eapPathGet(const char *ssid, std::string *eapPath);
	stiHResult enterpriseValuesSet();

	std::shared_ptr<NetworkService> serviceGet(std::string name, EstiWirelessType type);

	std::shared_ptr<NetworkService> serviceFromObjectPathGet (
		std::string objectPath);

	stiHResult doConnect (
		std::shared_ptr<NetworkService> service);

	stiHResult ConnmanInitialize ();
	void ConnmanUninitialize ();

	SstiNetworkSettings m_tmpSettings;
	
	SState m_stState = {IstiNetwork::estiUNKNOWN, 0};

	std::shared_ptr<DbusConnection> m_dbusConnection;

	unsigned long m_ServicesChangedSignalHandlerId = 0;

	std::list<std::shared_ptr<NetworkTechnology>> m_technologies;
	std::list<std::shared_ptr<NetworkService>> m_services;
	std::list<std::tuple<std::string, std::string>> m_rememberedServices;
	std::shared_ptr<NetworkService> m_newService;
	std::shared_ptr<NetworkService> m_oldService;
	std::shared_ptr<NetworkService> m_targetService;

	std::shared_ptr<NetworkManagerInterface> m_networkManager;

	std::string m_userName;
	std::string m_passPhrase;
	std::string m_lostService;
	std::string m_wirelessMacAddress;

	bool m_bCableConnected = false;
	bool m_useIpv6 = false;
	unsigned int m_nextRequestId = 0;

	bool m_bShutdown = false;
	bool m_newPassphrase = false;

	GDBusObjectManagerServer *m_pObjectManagerServer = nullptr;
	CstiNetworkObjectSkeleton *m_pObjectSkeleton = nullptr;
	CstiNetworkAgent *m_pAgentSkeleton = nullptr;
	unsigned long m_RequestInputSignalHandlerId = 0;
	unsigned long m_RequestBrowserSignalHandlerId = 0;
	unsigned long m_CancelSignalHandlerId = 0;
	unsigned long m_ReportErrorSignalHandlerId = 0;
	unsigned long m_ReleaseSignalHandlerId = 0;

	GDBusMethodInvocation *m_needsCredentialsInvocation = nullptr;
	int m_needsCredentialsInputType = 0;
	std::shared_ptr<NetworkService> m_needsCredentialsService;

	CstiTimer m_reconnectTimer {RECONNECT_TIMEOUT_MS};
	CstiTimer m_resetTimer {RESET_TIMEOUT_MS};
	void eventReconnectTimerTimeout();
	void eventResetTimerTimeout();

	bool m_reconnecting {false};
	bool m_waitForConnect {false};
	resetState m_resetState {resetState::unknown};
	std::string m_resetName;

	std::string m_connmanDirName;
	std::string m_connmanStopCommand;
	std::string m_connmanStartCommand;

	CstiSignalConnection::Collection m_signalConnections;
	CstiSignalConnection::Collection m_managerConnections;

	CstiBluetooth *m_bluetooth;
	bool m_bluetoothEnabled;
	bool m_bluetoothPowered {false};
	bool m_wifiPowered {false};
	bool m_pendingWifiScan {false};

};
