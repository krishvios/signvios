// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiNetworkBase2.h
 * \brief See CstiNetworkBase2.cpp
 */

#pragma once

#include "CstiNetworkBase.h"

class CstiNetworkBase2 : public CstiNetworkBase
{
public:
	CstiNetworkBase2(CstiBluetooth *bluetooth);

	~CstiNetworkBase2() override = default;

	/*!
	 * \brief set scanning-enabled mode. If enabled, wifi is automatically enabled 
	 * to allow scans to run for the duration of scanning-enabled mode
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	stiHResult WirelessScanningEnabledSet(bool bState) override;

	/*!
	 * \brief get scanning-enabled mode
	 *
	 * \return stiRESULT_SUCCESS or an error code
	 */
	bool WirelessScanningEnabledGet() override;

protected:

	stiHResult syncWifiPowerState() override;

private:
	bool m_wifiScanningEnabled {false};
	bool m_wifiTempEnabled {false};
};
