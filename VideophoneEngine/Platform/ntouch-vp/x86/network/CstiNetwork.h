/*!
* \file CstiNetwork.h
* \brief Consolidation of all network tasks.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef CSTINETWORK_H
#define CSTINETWORK_H

#include "stiDefs.h"
#include "stiSVX.h"
#include "BaseNetwork.h"
#include "CstiDHCP.h"
//#include "IstiUPnP.h		// ACTION - NETWORK - Implement
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
//#include "stiError.h"
#ifdef stiWIRELESS
//#include "IstiWireless.h"
#include "stiOSLinuxWd.h"
#endif

/*
 * forward declaration
 */
class CstiBluetooth;

class CstiNetwork : public BaseNetwork, public CstiOsTaskMQ
{

public:

	CstiNetwork (CstiBluetooth *);

	CstiNetwork (const CstiNetwork &other) = delete;
	CstiNetwork (CstiNetwork &&other) = delete;
	CstiNetwork &operator= (const CstiNetwork &other) = delete;
	CstiNetwork &operator= (CstiNetwork &&other) = delete;

	~CstiNetwork () override;

	stiHResult Initialize ();

	stiHResult Startup () override;

	stiHResult Shutdown () override;

	stiHResult WaitForShutdown () override;

	EstiState StateGet () const override;
	EstiError ErrorGet () const override;
	virtual void ErrorClear ()
	{
		m_eError = estiErrorNone;
	}

#if defined(stiWIRELESS)
	stiHResult WAPListGet (WAPListInfo *) const override { return stiRESULT_SUCCESS; }
	bool WirelessAvailable () const override { return false; }
	int WirelessInUse() override { return 0; }
#endif

#if 0
#ifdef stiWIRELESS
	//virtual stiHResult WAPConnect (SstiSSIDListEntry *);
	virtual stiHResult WAPListGet (WAPListInfo *) const;
	virtual stiHResult WirelessAvailable () const;
	virtual stiHResult WirelessDeviceStart ();
	virtual stiHResult WirelessDeviceStop ();
	virtual stiHResult WirelessSignalStrengthGet (int *);
	virtual stiHResult WirelessDeviceConnected(bool *);
	virtual stiHResult WirelessParamCmdSet(char * pszCommand) const;
	virtual int WirelessInUse(void);
#endif
#endif
	std::string localIpAddressGet(
		EstiIpAddressType addressType,
		const std::string& destinationIpAddress) const override;

	stiHResult SettingsGet (SstiNetworkSettings *pstSettings) const override;
	stiHResult SettingsSet (const SstiNetworkSettings *pstSettings, unsigned int *punRequestId) override;
	void InCallStatusSet (bool bSet) override;

	stiHResult EthernetCableStatusGet (bool *pbConnected) override;
	std::string NetworkDeviceNameGet () const override;

	EstiServiceState ServiceStateGet() override
	{
		return(estiServiceOnline);
	}

private:
	stiHResult EthernetCableConnectedGet (bool *pbConnected);

	// Helper methods to query for network device information on the x86 host
	std::string FirstNetworkDeviceNameGet(NetworkType type) const;
	std::list<SstiNetworkDevice> NetworkDevicesGet() const;

	void networkInfoSet (
		NetworkType type,
		const std::string &data) override {};

	NetworkType networkTypeGet () const override { return NetworkType::Wired; };
	std::string networkDataGet () const override { return ""; };

private:
	enum EEvent
	{
		eWIRELESS_CONNECT_OPEN = estiMSG_NEXT,
		eWIRELESS_CONNECT_WEP,
		eWIRELESS_CONNECT_WPAPSK,
		eWIRELESS_CONNECT_WPAEAP,
		eWIRELESS_DEVICE_START,
		eWIRELESS_DEVICE_STOP,
		eSETTINGS_SET,
		eNETWORK_CONNECTON_LOST,
	};

	struct SState
	{
		EstiState eState;
		int nNegotiateCnt;	// Used to handle multiple calls to change the state to negotiating
	};

	SstiNetworkSettings m_Settings;
	SstiNetworkSettings m_BackupSettings;
	unsigned int m_unNextRequestId {0};
	unsigned int m_unDHCPRequestId {0};
	
	SState m_stState{};
	EstiError m_eError {estiErrorNone};
	CstiDHCP m_oDHCP;
//	IstiUPnP* m_IstiUPnP;		// ACTION - NETWORK - Need to provide linkage
	stiWDOG_ID m_wdConnectionLost {nullptr}; // Used to manage lost connections
	bool m_bConnected {false};

//#ifdef stiWIRELESS
//	IstiWireless* m_pWireless;
//
//	stiHResult WirelessConnect(const char *);
//	stiHResult WirelessConnect(const WEP *);
//	stiHResult WirelessConnect(const WpaPsk *);
//	stiHResult WirelessConnect(const WpaEap *);
//#endif

	static stiHResult DHCPCallback (
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t CallbackFromId);

	stiHResult NetStartSettingGet (
		bool *pbUseDHCP
	#if defined stiWIRELESS
		,enum NetworkType *peNetworkDeviceType
	#endif
		);

	stiHResult NetstartSettingsBackup ();

	void NetstartSettingsRestore ();

	stiHResult NetstartSettingsUpdate (
		const SstiNetworkSettings *pstSettings);

	std::string m_NetworkSettingsFile;
	std::string m_NetworkSettingsBackupFile;

	void StateSet (EstiState eState);
	int Task () override;
	stiDECLARE_EVENT_MAP(CstiNetwork);
	stiDECLARE_EVENT_DO_NOW(CstiNetwork);

	//
	// Event Handlers
	//
	stiHResult EventTaskShutdown (CstiEvent *poEvent);
	EstiResult EventNetstartSettingsUpdate (CstiEvent *poEvent);
	EstiResult EventUPnPConfigSet (CstiEvent *poEvent);
	EstiResult EventTimer (CstiEvent *poEvent);
	//EstiResult EventWAPConnectOpen (CstiEvent *poEvent);
	//EstiResult EventWAPConnectWEP (CstiEvent *poEvent);
	//EstiResult EventWAPConnectWpaPsk (CstiEvent *poEvent);
	//EstiResult EventWAPConnectWpaEap (CstiEvent *poEvent);
#if 0
#ifdef stiWIRELESS
	EstiResult EventWirelessDeviceStart (CstiEvent *poEvent);
	EstiResult EventWirelessDeviceStop (CstiEvent *poEvent);
//	EstiResult EventNetworkConnectionLost (CstiEvent *poEvent);

//	static void ConnectionLostTimerFunc (void *pParam);
#endif
#endif
	EstiResult EventSettingsSet (CstiEvent *poEvent);

};

#endif // #ifndef CSTINETWORK_H
