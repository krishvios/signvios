/*!
 * \file CstiTunnelManager.cpp
 * \brief The Tunnel Manager is responsible for interfacing with Acme TSM SDK
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 – 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "stiDefs.h"
#include "stiTrace.h"
#include "stiSVX.h"

#include "CstiTunnelManager.h"
#include "stiTaskInfo.h"
#include "stiTools.h"

#ifndef stiDISABLE_PROPERTY_MANAGER
#include "PropertyManager.h"
#endif
#include "CstiHostNameResolver.h"

#ifdef stiTUNNEL_MANAGER
#include "rvsockettun.h"
#include "rvares.h"
#include "rvrtpseli.h"
#endif


void RedundancyNotificationCallback(tsc_notification_data *notification)
{
	auto red_data = (tsc_notification_redundancy_info_data *)notification->data;
	
	if (red_data)
	{
		if (red_data->available == tsc_bool_true)
		{
			if (red_data->enabled == tsc_bool_true)
			{
				stiTrace("redundancy enabled on socket %d\n", red_data->socket);
			}
			else
			{
				stiTrace("redundancy disabled on socket %d\n", red_data->socket);
			}
		}
		else
		{
			stiTrace("redundancy not allowed on socket %d\n", red_data->socket);
		}
	}
}

void CstiTunnelManager::TunnelSocketInfoNotificationCallback(tsc_notification_data* data)
{
	if (!data)
	{
		stiTrace("%s: no notification data\n", __FUNCTION__);
		return;
	}
	
//	if (data->opaque != &m_hSocketInfoNotificationHandle)
//	{
//		stiTrace("%s: wrong handle\n", __FUNCTION__);
//		return;
//	}
	
	auto  socket_info = (tsc_tunnel_socket_info*)data->data;
	if (!socket_info)
	{
		stiTrace("%s: no socket info data\n", __FUNCTION__);
		return;
	}
	char local_addr[TSC_ADDR_STR_LEN];
	char remote_addr[TSC_ADDR_STR_LEN];
	char nat_ipport[TSC_ADDR_STR_LEN];

	if (!tsc_ip_port_address_to_str(&(socket_info->local_address), local_addr, TSC_ADDR_STR_LEN))
	{
		stiTrace ("Failed to convert tunnel local address\n");
	}
	if (!tsc_ip_port_address_to_str(&(socket_info->remote_address), remote_addr, TSC_ADDR_STR_LEN))
	{
		stiTrace ("Failed to convert tunnel remote address\n");
	}
	if (!tsc_ip_port_address_to_str (&(socket_info->nat_ipport), nat_ipport, TSC_ADDR_STR_LEN))
	{
		stiTrace ("Failed to convert tunnel NAT address\n");
	}
	
	auto pThis = static_cast<CstiTunnelManager*>(data->opaque);
	
	tsc_config config;
	tsc_error_code result = tsc_get_config(pThis->m_hTunnelHandle, &config);
	if (tsc_error_code_ok != result)
	{
		stiTrace ("Error getting tunnel info.\n");
	}
	
	char internal_addr[TSC_ADDR_STR_LEN];
	char public_addr[TSC_ADDR_STR_LEN];
	
	tsc_ip_address_to_str (&config.internal_address, internal_addr, TSC_ADDR_STR_LEN);
	tsc_ip_address_to_str ((tsc_ip_address *)&(socket_info->nat_ipport.address), public_addr, TSC_ADDR_STR_LEN);
	
	stiTrace ("Tunnel Established: local address %s, remote address %s, NAT address %s, Tunnel IP %s\n",
			  local_addr, remote_addr, nat_ipport, internal_addr);

	// Save these values to determine later if we need to reestablish the tunnel.
	pThis->m_LocalAddress = socket_info->local_address;
	pThis->m_RemoteAddress = socket_info->remote_address;
	pThis->m_Transport = socket_info->transport;
	
	pThis->PostEvent (
		std::bind(&CstiTunnelManager::EventTunnelSocketInfo, pThis));
}

const char test_ca[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDhjCCAm6gAwIBAgIQWF4nXBii641IuEntzqVi9TANBgkqhkiG9w0BAQUFADBC\n"
"MRMwEQYKCZImiZPyLGQBGRYDQ09NMRYwFAYKCZImiZPyLGQBGRYGU1JFTEFZMRMw\n"
"EQYDVQQDEwpTUkVMQVlDQTAxMB4XDTEyMDExOTIxNDUyN1oXDTE1MDExOTIxNTUy\n"
"NlowQjETMBEGCgmSJomT8ixkARkWA0NPTTEWMBQGCgmSJomT8ixkARkWBlNSRUxB\n"
"WTETMBEGA1UEAxMKU1JFTEFZQ0EwMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n"
"AQoCggEBAMoLlO0yYnKls/Ikf1byvZ216UYgi0z0fUjiDx6y3ZZRs+qdqQdWd/uF\n"
"puacyeGVF51Y9Ghu+STKF8xVbZ3y4rBBSn526LWj02ZZRP4uU8mc9EhuGlsI020g\n"
"wRQIuw+xrrBaSnadR21ftE+MVokYG3JryAVtXbeJnxzF3lTPqIAmiAnLGIr/5oEu\n"
"+hOyqse0fKsJUpWtzQ0Sm+NUOO247CEtcFQJBjUQxgp+PQO4wItw4alr3e/mc/T1\n"
"ZzmFg/Y99mWmU++JmSZdXkHtVQBu8VALpwjah6oLNUfBJOWLEUbiagrbMPnJEuZ0\n"
"3YTSSEncBLUofXhg6+yWHkimBFZplP0CAwEAAaN4MHYwCwYDVR0PBAQDAgGGMA8G\n"
"A1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFJozfW6yiuBjAY/0YHve6cJjz5uEMBIG\n"
"CSsGAQQBgjcVAQQFAgMBAAEwIwYJKwYBBAGCNxUCBBYEFPlfTQ3thA95jPlFF5q3\n"
"nkDoHR7VMA0GCSqGSIb3DQEBBQUAA4IBAQDBcKqo9hlY8HohNlv0FFQOu1tMakBF\n"
"EMEuZmcUeHtCXJeRnSypm8kzKCF/06BZd7stochSUmjm8u9WZLK1Rzw/VfKPq17a\n"
"jgaGnR9x7TFPUPW7W5M5R9+pUwWw7Ulc6djdzfLv3dZ4r1Wor7gRsxR+87Plr7UE\n"
"R0Mzf+C5oi7h2PveTGmObWJ/ZnTe8W8VEEi8kV0AbSNHI6coZUEwINptMI/vRNxH\n"
"B+E1Eju0mnX69fksSeBqzdcMu1Qj8iFg4libBNWqG8fXW7ehZF4ekv1io5PnF02r\n"
"IFpYn8YH/lnh2F5oQqsPQT1JXP+ZFs5FHohvFQL8nuWwGG+biOPajL/f\n"
"-----END CERTIFICATE-----\n";

const char test_cert[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDkDCCAvmgAwIBAgIJAJvnfRHuBOUiMA0GCSqGSIb3DQEBBQUAMIGNMQswCQYD\n"
"VQQGEwJVUzELMAkGA1UECBMCTUExEzARBgNVBAcTCmJ1cmxpbmd0b24xDTALBgNV\n"
"BAoTBGFjbWUxDDAKBgNVBAsTA3NlYzEZMBcGA1UEAwwQYWNtZTEwMUBhY21lLmNv\n"
"bTEkMCIGCSqGSIb3DQEJARYVc2VsZkBhY21lMTAxLmFjbWUuY29tMB4XDTExMDQw\n"
"NTE5MTQzNVoXDTExMDUwNTE5MTQzNVowgY0xCzAJBgNVBAYTAlVTMQswCQYDVQQI\n"
"EwJNQTETMBEGA1UEBxMKYnVybGluZ3RvbjENMAsGA1UEChMEYWNtZTEMMAoGA1UE\n"
"CxMDc2VjMRkwFwYDVQQDDBBhY21lMTAxQGFjbWUuY29tMSQwIgYJKoZIhvcNAQkB\n"
"FhVzZWxmQGFjbWUxMDEuYWNtZS5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJ\n"
"AoGBAK4tRNAMmLFSbGDzxjNuR46DdoPyVzimtuz4xY5T5z6Y2sCJglbIqA0x6ojc\n"
"o4RE3YUcifwJcS3MStlAPNG52NoWmJwU8n27SY05ODv0viF1twznI3ixUeb8mYIv\n"
"/CzJOZ1m+CCRIb3bvb41H8MuNN5Y/ukAVKnQ32yrrdoRnpp9AgMBAAGjgfUwgfIw\n"
"HQYDVR0OBBYEFLbP/35oJmbIsdUsDbjxxhwncqupMIHCBgNVHSMEgbowgbeAFLbP\n"
"/35oJmbIsdUsDbjxxhwncqupoYGTpIGQMIGNMQswCQYDVQQGEwJVUzELMAkGA1UE\n"
"CBMCTUExEzARBgNVBAcTCmJ1cmxpbmd0b24xDTALBgNVBAoTBGFjbWUxDDAKBgNV\n"
"BAsTA3NlYzEZMBcGA1UEAwwQYWNtZTEwMUBhY21lLmNvbTEkMCIGCSqGSIb3DQEJ\n"
"ARYVc2VsZkBhY21lMTAxLmFjbWUuY29tggkAm+d9Ee4E5SIwDAYDVR0TBAUwAwEB\n"
"/zANBgkqhkiG9w0BAQUFAAOBgQCF3EtKSF4rPVre16JaSHjX1GQ7hHvdfY46Klbu\n"
"3JW0Pc9sCV6euKggPmpIW6ZRN04LyUe7AH/x5KL/5Tp0jBzIr7FW5dggNnkoqJcf\n"
"RevoTLWgyoIPJfZ+eeoZnhqQudXwtDRuUNNSVXvJyBH5f8M1OVqbcumdA/PRT33M\n"
"N30rFA==\n" "-----END CERTIFICATE-----\n";



#ifdef TSC_DDT
void ddt_notification(tsc_notification_data *notification)
{
	auto ddt_data = (tsc_notification_ddt_info_data *)notification->data;
	
	if (ddt_data)
	{
		if (ddt_data->available == tsc_bool_true)
		{
			if (ddt_data->enabled == tsc_bool_true)
			{
				stiTrace("ddt enabled on socket %d\n", ddt_data->socket);
			}
			else
			{
				stiTrace("ddt disabled on socket %d\n", ddt_data->socket);
			}
		}
		else
		{
			stiTrace("ddt not allowed on socket %d\n", ddt_data->socket);
		}
	}
}
#endif

void CstiTunnelManager::tunnel_termination_info_notification_cb(tsc_notification_data* data)
{
	if (!data)
	{
		stiTrace("%s: no notification data\n", __FUNCTION__);
		return;
	}
	auto pThis = static_cast<CstiTunnelManager*>(data->opaque);
	pThis->tunnelTerminatedSignal.Emit ();
	stiTrace ("Tunnel terminated: %d\n", pThis->TunnelingEnabledGet());
	
	tsc_config *config = &(((tsc_notification_termination_info_data *)data->data)->config);
	
	stiTrace("Got tunnel termination info notification: tunnel %08X%08X terminated\n", config->tunnel_id.hi,
		   config->tunnel_id.lo);
}

CstiTunnelManager::CstiTunnelManager ()
:
	CstiEventQueue (stiTUN_TASK_NAME)
{
	
}

CstiTunnelManager::~CstiTunnelManager()
{
	Shutdown ();

	if (m_pTunnelingServer)
	{
		delete m_pTunnelingServer;
		m_pTunnelingServer = nullptr;
	}
}

stiHResult CstiTunnelManager::ServerSet ( const CstiServerInfo *pTunnelingServer )
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_pTunnelingServer)
	{
		delete m_pTunnelingServer;
		m_pTunnelingServer = nullptr;
	}
	if (pTunnelingServer)
	{
		m_pTunnelingServer = new CstiServerInfo (*pTunnelingServer);
	}
	return hResult;
}

void CstiTunnelManager::EventEstablishTunnel ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
#ifdef stiTUNNEL_MANAGER
	static tsc_tunnel_params tunparams;
	tsc_connection_params *pConParams;
	tsc_requested_config rconfig;
	tsc_handle tun = nullptr;
	uint32_t serverIP;
	addrinfo *pstAddrInfo = nullptr;
	sockaddr_in *pSockAddr_in = nullptr;
	std::string dynamicDir;
	
	
	RvAresSetTunPolicy(nullptr, RvTun_DISABLED);
	
	m_bTunnelEstablished = false;

	CstiHostNameResolver *poResolver = CstiHostNameResolver::getInstance ();
	hResult = poResolver->Resolve (m_pTunnelingServer->AddressGet(), nullptr, &pstAddrInfo);

	stiTESTRESULT ();
	
	if (!pstAddrInfo)
	{
		failedToEstablishTunnelSignal.Emit ();

		// we better get a HostTable from the prior call, otherwise we are done
		stiTESTCOND (pstAddrInfo && pstAddrInfo->ai_addr, stiRESULT_ERROR);
	}
	
	pSockAddr_in = (sockaddr_in *)pstAddrInfo->ai_addr;
	serverIP = pSockAddr_in->sin_addr.s_addr;
	//tsc_inet_pton (AF_INET, "209.169.226.27", &serverIP);//209.169.233.67", &serverIP);

	memset(&tunparams, 0, sizeof(tunparams));

	//The TSC code is dead locking while logging on Windows
	tsc_set_log_level(tsc_log_level_disabled);

	tsc_set_log_output(stderr);

	if (m_bEnableDebug)
	{
		stiOSDynamicDataFolderGet(&dynamicDir);

		sprintf (tunparams.pcap_capture.filename, "%stsc.pcap", dynamicDir.c_str());
		tunparams.pcap_capture.enabled = tsc_bool_true;
		tsc_set_log_level(tsc_log_level_debug);
		static char szTscLogFile[1024];
		sprintf(szTscLogFile, "%stsc.log", dynamicDir.c_str());
		m_fTscLogFile = fopen(szTscLogFile, "w");
		
		if(m_fTscLogFile != nullptr)
		{
			tsc_set_log_output(m_fTscLogFile);
		}
	}
	
	tunparams.max_connections = 3;
	tunparams.connection_timeout_max = 10;

	pConParams = &tunparams.connection_params[0];
	pConParams->server_address.address = ntohl (serverIP);
	pConParams->server_address.port = 444;
	pConParams->transport = tsc_transport_tcp;
	pConParams->buffering = tsc_transport_buffering_disabled;
#if APPLICATION == APP_NTOUCH_IOS
	pConParams->background_mode = tsc_bool_true;
#endif

	pConParams = &tunparams.connection_params[1];
	pConParams->server_address.address = ntohl (serverIP);
	pConParams->server_address.port = STANDARD_HTTP_PORT;
	pConParams->transport = tsc_transport_tcp;
	pConParams->buffering = tsc_transport_buffering_disabled;
#if APPLICATION == APP_NTOUCH_IOS
	pConParams->background_mode = tsc_bool_true;
#endif

	pConParams = &tunparams.connection_params[2];
	pConParams->server_address.address = ntohl (serverIP);
	pConParams->server_address.port = STANDARD_HTTPS_PORT;
	pConParams->transport = tsc_transport_tcp;
	pConParams->buffering = tsc_transport_buffering_disabled;
#if APPLICATION == APP_NTOUCH_IOS
	pConParams->background_mode = tsc_bool_true;
#endif

	tunparams.sec_config[0].read_from_file = tsc_bool_false;
	tunparams.sec_config[0].ca_count = 1;
	tunparams.sec_config[0].config_ca.ca_len = sizeof(test_ca);
	memcpy (tunparams.sec_config[0].config_ca.ca, test_ca, sizeof(test_ca));

	tunparams.sec_config[0].auth_disable = tsc_bool_false;
	tunparams.sec_config[0].cert_count = 1;
	tunparams.sec_config[0].config_cert.cert_len = sizeof(test_cert);
	memcpy (tunparams.sec_config[0].config_cert.cert, test_cert, sizeof(test_cert));

	memset(&rconfig, 0, sizeof(rconfig));
	tun = tsc_new_tunnel(&tunparams, &rconfig);
	stiTESTCONDMSG (tun, stiRESULT_ERROR, "tsc_new_tunnel failed");

	m_hTunnelHandle = tun;
	// Setup notification handlers
	tsc_notification_params params;
	params.opaque = this;
	tsc_notification_enable(tun, tsc_notification_tunnel_socket_info, TunnelSocketInfoNotificationCallback, &params);

	m_hTunnelTerminationNotification = tsc_notification_enable(tun, tsc_notification_tunnel_termination_info, tunnel_termination_info_notification_cb, &params);
	
#ifdef TSC_DDT
	tsc_notification_enable(tun, tsc_notification_ddt, ddt_notification, &params);
#endif
	
	tsc_notification_enable(tun, tsc_notification_redundancy, RedundancyNotificationCallback, &params);
	stiTESTCONDMSG (m_hTunnelHandle, stiRESULT_ERROR, "Failed to initialize tunnel");

STI_BAIL:

#endif //stiTUNNEL_MANAGER

	return;
}

void CstiTunnelManager::TunnelingEnabledSet (bool bEnable)
{
	PostEvent (
		std::bind(&CstiTunnelManager::EventTunnelingEnabledSet, this, bEnable));
}

void CstiTunnelManager::EventTunnelingEnabledSet (bool enabled)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_bTunnelingEnabled = enabled;

	if (m_bTunnelingEnabled)
	{
		//
		// If we already have a handle to a tunnel just reuse it
		//
		if (m_hTunnelHandle)
		{
			hResult = TunnelingAddressesUpdate ();
			stiTESTRESULT ();
		}
		else
		{
			//
			// Now establish the tunnel
			//
			EventEstablishTunnel ();
		}
	}
	
STI_BAIL:
	return;
}

stiHResult CstiTunnelManager::TunnelingAddressesUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	tsc_config config;
	tsc_error_code result;

	char internal_addr[TSC_ADDR_STR_LEN];
	char public_addr[TSC_ADDR_STR_LEN];
	tsc_tunnel_socket_info tunnel_info;

	result = tsc_get_config (m_hTunnelHandle, &config);
	stiTESTCOND (tsc_error_code_ok == result, stiRESULT_ERROR);

	// Get our NAT address to send to Core as our Public IP
	result = tsc_get_tunnel_socket_info (m_hTunnelHandle, &tunnel_info);
	
	if (result != tsc_error_code_ok)
	{
		failedToEstablishTunnelSignal.Emit ();
		stiTESTCOND (tsc_error_code_ok == result, stiRESULT_ERROR);
	}

	tsc_ip_address_to_str (&config.internal_address, internal_addr, TSC_ADDR_STR_LEN);
	tsc_ip_address_to_str ((tsc_ip_address *)&(tunnel_info.nat_ipport.address), public_addr, TSC_ADDR_STR_LEN);

	tunnelEstablishedSignal.Emit (std::string(internal_addr));
	publicIpResolvedSignal.Emit (std::string(public_addr));

STI_BAIL:

	return hResult;
}

void CstiTunnelManager::EventTunnelSocketInfo ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RvTunParams inConfig;
	RvTunParamsInit(&inConfig);
	RvTunH hRvTun;
	RvStatus rvStatus;
	
	tsc_config config;
	tsc_error_code result;
	
	result = tsc_get_config (m_hTunnelHandle, &config);
	stiTESTCOND (result == tsc_error_code_ok, stiRESULT_ERROR);

	if (!m_bTunnelEstablished)
	{
		RvTunParamsSetExtTunnelHandle (&inConfig, m_hTunnelHandle);

		rvStatus = RvTunEstablish (&hRvTun, &inConfig, -1, nullptr);
		if (rvStatus != RV_OK)
		{
			failedToEstablishTunnelSignal.Emit ();
			stiTESTRVSTATUS ();
		}

		/* Sets established tunnel as global tunnel */
		RvTunH oldTun = RvTunSet (hRvTun, RV_TRUE);
		if (oldTun)
		{
			RvTunDestroy (oldTun);
		}
		m_bTunnelEstablished = true;
	}

	hResult = TunnelingAddressesUpdate ();
	stiTESTRESULT();

STI_BAIL:
	return;
}

stiHResult CstiTunnelManager::Startup()
{
	StartEventLoop ();

#ifndef stiDISABLE_PROPERTY_MANAGER
	WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance();
	pm->propertyGet("EnableTunnelingDebug", &m_bEnableDebug, WillowPM::PropertyManager::Persistent);
	pm->propertySet("EnableTunnelingDebug", m_bEnableDebug, WillowPM::PropertyManager::Persistent);
#endif

	if (m_bTunnelingEnabled && !m_hTunnelHandle)
	{
		PostEvent (
			std::bind(&CstiTunnelManager::EventEstablishTunnel, this));
	}
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiTunnelManager::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiTunnelManager::Shutdown);
	
#ifdef stiTUNNEL_MANAGER
	RvTunH tun = RvTunGet();
	if (tun)
	{
		tsc_notification_disable(m_hTunnelTerminationNotification);
		m_hTunnelTerminationNotification = nullptr;

		RvTunDestroy(nullptr);
		//tsc_delete_tunnel(m_hTunnelHandle);  RvTunDestroy(0) destroys and frees memory of the tunnel referenced m_hTunnelHandle
		m_hTunnelHandle = nullptr;
	}
#endif

	StopEventLoop ();

	return stiRESULT_SUCCESS;
}

void CstiTunnelManager::ApplicationEventSet (IstiTunnelManager::EApplicationState eApplicationState)
{
#ifdef stiTUNNEL_MANAGER
	auto  event = (tsc_app_event_type)eApplicationState;
	tsc_app_event(m_hTunnelHandle, event);
#endif
}

int CstiTunnelManager::CreateUdpSocketWithRedundancy(
	uint32_t unRedundancyFactor, EstiBool bEnableLoadBalancing)
{
	int rtp_socket = -1;
#ifdef stiTUNNEL_MANAGER
	/*
	 * Setup RTP UDP socket
	 */
	rtp_socket = tsc_socket (m_hTunnelHandle, AF_INET, SOCK_DGRAM, 0);

	if (unRedundancyFactor > 0)
	{
		tsc_notification_enable(m_hTunnelHandle, tsc_notification_redundancy, RedundancyNotificationCallback, nullptr);
		tsc_so_redundancy redundancy;
		memset(&redundancy, 0, sizeof(tsc_so_redundancy));
		redundancy.redundancy_factor = unRedundancyFactor;

		tsc_handle tunnel = tsc_get_tunnel(rtp_socket);
		tsc_tunnel_socket_info tunnel_info;
		tsc_get_tunnel_socket_info(tunnel, &tunnel_info); 

		if (tunnel_info.transport == tsc_transport_udp || tunnel_info.transport == tsc_transport_dtls)
		{
			redundancy.redundancy_method = tsc_so_redundancy_method_udp_dtls;
		}
		else
		{
			if (bEnableLoadBalancing)
			{
				redundancy.redundancy_method = tsc_so_redundancy_method_tcp_tls_load_balance;
			}
			else
			{
				redundancy.redundancy_method = tsc_so_redundancy_method_tcp_tls_fan_out;
			}
		}

		tsc_setsockopt(rtp_socket, SOL_SOCKET, SO_TSC_REDUNDANCY,
					   (char *)&redundancy, sizeof(tsc_so_redundancy));        
	}
#endif
	return rtp_socket;
}

void CstiTunnelManager::NetworkChangeNotify()
{
	uint64_t bestLocalTunnelAddress = GetBestLocalTunnelAddress();
	if (bestLocalTunnelAddress != m_LocalAddress.address)
	{
		ApplicationEventSet(eApplicationSorensonHasNoNetwork);
		ApplicationEventSet(eApplicationSorensonHasNetwork);
	}
}

uint64_t CstiTunnelManager::GetBestLocalTunnelAddress()
{
	uint64_t localAddress = 0;
	int socketType = SOCK_STREAM;
	int ipProto = IPPROTO_TCP;

	if (m_Transport == tsc_transport_udp || m_Transport == tsc_transport_dtls)
	{
		socketType = SOCK_DGRAM;
		ipProto = IPPROTO_UDP;
	}

	stiSocket s = stiOSSocket(AF_INET, socketType, ipProto);

	struct sockaddr_in dest{};
	memset(&dest, 0, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(m_RemoteAddress.port);
	dest.sin_addr.s_addr = htonl(m_RemoteAddress.address);

	auto hResult = stiOSConnect(s, (struct sockaddr*)&dest, sizeof(struct sockaddr_in));

	if (!stiIS_ERROR (hResult))
	{
		struct sockaddr_in tunnelName{};
		socklen_t addrLen = sizeof(struct sockaddr_in);
		memset(&tunnelName, 0, sizeof(struct sockaddr_in));
		if (stiINVALID_SOCKET != getsockname(s, (struct sockaddr*)&tunnelName, &addrLen))
		{
			localAddress = ntohl(tunnelName.sin_addr.s_addr);
		}
	}

	stiOSClose(s);

	return localAddress;
}
// End file CstiTunnelManager.cpp
