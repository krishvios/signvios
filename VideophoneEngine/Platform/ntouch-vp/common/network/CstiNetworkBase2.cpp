// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved

#include "CstiNetworkBase2.h"

CstiNetworkBase2::CstiNetworkBase2(CstiBluetooth *bluetooth)
:
	CstiNetworkBase(bluetooth)
{
}

stiHResult CstiNetworkBase2::WirelessScanningEnabledSet(bool bState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (bState == m_wifiScanningEnabled)
		return stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("CstiNetworkBase2::WirelessScanningEnabledSet() - Changing wifiScanningEnabled state from %d to %d\n", m_wifiScanningEnabled, bState);
	);

	bool wirelessEnabled = WirelessAvailable();

	if (bState)
	{
		if (!wirelessEnabled)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("CstiNetworkBase2::WirelessScanningEnabledSet() - Scanning Mode is enabled, so temporarily enable the wifi nic to allow scanning\n");
			);
			hResult = WirelessEnable(true);
			stiTESTRESULT ();

			m_wifiTempEnabled = true;
		}
	}
	else
	{
		bool wirelessInUse = WirelessInUse();

		if (wirelessEnabled && m_wifiTempEnabled && !wirelessInUse)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("CstiNetworkBase2::WirelessScanningEnabledSet() - Scanning Mode is now disabled, so reset wifi nic to disabled\n");
			);

			hResult = WirelessEnable(false);
			stiTESTRESULT ();
		}

		m_wifiTempEnabled = false;
	}

	m_wifiScanningEnabled = bState;
	
STI_BAIL:
	return hResult;
}

bool CstiNetworkBase2::WirelessScanningEnabledGet()
{
	return m_wifiScanningEnabled;
}


stiHResult CstiNetworkBase2::syncWifiPowerState()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// ensure that the active connection is online and in good condition
	if (m_currentService && !m_currentService->m_cancelled && !m_currentService->m_disconnecting && m_currentService->m_autoConnect &&
		m_error == EstiError::estiErrorNone && (m_currentService->m_progressState & STATE_ONLINE) == STATE_ONLINE)
	{
		if (m_currentService->m_technologyType == technologyType::wired)
		{
			if (m_wifiScanningEnabled)
			{
				m_wifiTempEnabled = true;
			}
			else
			{
				WirelessEnable(false);
			}
		}
		else if (m_currentService->m_technologyType == technologyType::wireless)
		{
			if (m_wifiScanningEnabled)
			{
				m_wifiTempEnabled = false;
			}
		}
	}

	return hResult;
}
