/*!
* \file CstiTunnelManager.h
* \brief See CstiTunnelManager.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "tsc_control_api.h"
#include "tsc_socket_api.h"
#include "tsc_version.h"
#include "IstiTunnelManager.h"
#include "CstiEventQueue.h"
#include "stiConfDefs.h"
#include <string>

class CstiTunnelManager : public IstiTunnelManager, public CstiEventQueue
{
public:
	CstiTunnelManager ();

	CstiTunnelManager (const CstiTunnelManager &other) = delete;
	CstiTunnelManager (CstiTunnelManager &&other) = delete;
	CstiTunnelManager &operator= (const CstiTunnelManager &other) = delete;
	CstiTunnelManager &operator= (CstiTunnelManager &&other) = delete;

	~CstiTunnelManager() override;
	
	stiHResult Startup();
	stiHResult Shutdown ();

	int CreateUdpSocketWithRedundancy(
		uint32_t unRedundancyFactor, EstiBool bEnableLoadBalancing);

	void TunnelingEnabledSet (bool bEnable);
	stiHResult ServerSet ( const CstiServerInfo *pTunnelingServer );
	void ApplicationEventSet (IstiTunnelManager::EApplicationState eApplicationState) override;
// TSM Callbacks
	static void TunnelSocketInfoNotificationCallback(tsc_notification_data* data); 
	void NetworkChangeNotify() override;

public:
	CstiSignal<> tunnelTerminatedSignal;
	CstiSignal<std::string> tunnelEstablishedSignal;
	CstiSignal<std::string> publicIpResolvedSignal;
	CstiSignal<> failedToEstablishTunnelSignal;

private:
	void EventEstablishTunnel ();
	void EventTunnelingEnabledSet (bool enabled);
	void EventTunnelSocketInfo ();

private:
	
	bool TunnelingEnabledGet() { return m_bTunnelingEnabled; }
	uint64_t GetBestLocalTunnelAddress();
	stiHResult TunnelingAddressesUpdate ();
	
	static void tunnel_termination_info_notification_cb (tsc_notification_data* data);
	
	tsc_handle m_hTunnelHandle = nullptr;
	tsc_notification_handle m_hTunnelTerminationNotification = nullptr;
	CstiServerInfo *m_pTunnelingServer = nullptr;
	bool m_bTunnelingEnabled = false;
	bool m_bEnableDebug = false;
	FILE *m_fTscLogFile = nullptr;
	bool m_bTunnelEstablished = false;
	tsc_ip_port_address m_LocalAddress = {};
	tsc_ip_port_address m_RemoteAddress = {};
	tsc_transport m_Transport = {};
};
