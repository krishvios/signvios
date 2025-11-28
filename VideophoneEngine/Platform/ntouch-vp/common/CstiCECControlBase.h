// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2017 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiCECControlBase.h
 * \brief See CstiCECControlBase.cpp
 */

#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiMonitorTask.h"
#include "stiTools.h"
#include "stiTrace.h"
#include <cstdint>
#include <mutex>

#include <libcec/cec.h>

#define CEC_ENABLED

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
namespace CEC
{
	class ICECAdapter;
}

class CstiCECControlBase : public CstiEventQueue
{

public:
	CstiCECControlBase();

	CstiCECControlBase(const CstiCECControlBase &other) = delete;
	CstiCECControlBase(CstiCECControlBase &&other) = delete;
	CstiCECControlBase &operator= (const CstiCECControlBase &other) = delete;
	CstiCECControlBase &operator= (CstiCECControlBase &&other) = delete;

	~CstiCECControlBase() override;

	virtual stiHResult Startup();

	stiHResult Initialize(CstiMonitorTask *pMonitorTask, const std::string &device, const std::string &name);

	// power on/off the tv
	stiHResult displayPowerSet(bool on);
	// make us the active signal provider
	stiHResult activeSet();
	// does the tv support cec
	bool cecSupported() const;
	// is the tv 'on'
	bool tvPowered() const;
	// get the vendor name of the tv
	std::string tvVendorGet();

public:
	CstiSignal<int> cecSupportedSignal;
	CstiSignal<const CEC::cec_keypress &> cecKeyPressSignal;

protected:
	CEC::ICECCallbacks m_callbacks {};

	/*
	 * callbacks needed for libcec
	 */
	static int cecLogMessage (void *self, CEC::cec_log_message message);
	static int cecKeyPress (void *self, const CEC::cec_keypress key);
	static int cecCommand (void *self, CEC::cec_command command);
	static int cecConfigurationChanged (void *self, CEC::libcec_configuration config);
	static int cecAlert (void *self, CEC::libcec_alert alert, CEC::libcec_parameter p);
	static int cecMenuStateChanged (void *self, CEC::cec_menu_state state);
	static void cecSourceActivated (void *self, CEC::cec_logical_address addr, uint8_t state);

private:
	stiHResult cecConnect(const std::string &device, const std::string &name);
	void powerCheck();
	void uint16_tPush(CEC::cec_command &command, uint16_t &val);

	/*
	 * event handlers
	 */
	void eventHdmiHotPlugStatusChanged();
	void eventCECSourceActivated(const CEC::cec_logical_address &addr, const uint8_t &state);
	void eventCECConfigurationChanged(const CEC::libcec_configuration &config);
	void eventCECCommand(const CEC::cec_command &command);

	stiHResult eventDisplayPowerSet(bool);
	stiHResult eventActiveSet();

private:
	CEC::cec_logical_address m_logicalAddress {CEC::CECDEVICE_UNREGISTERED};
	CEC::cec_logical_address m_activeSource {CEC::CECDEVICE_UNREGISTERED};
	std::unique_ptr<CEC::libcec_configuration> m_cecConfig {};
	CEC::ICECAdapter *m_adapter {nullptr};

	uint16_t m_physicalAddress {0};
	CstiSignalConnection m_hdmiOutHotPlugSignalConnection {};

	uint8_t m_cecVersion {0};
	CEC::cec_vendor_id m_vendorId {CEC::CEC_VENDOR_UNKNOWN};		//vendor id of the television we are connected to

	bool m_tvPowered {false};
	bool m_cecSupported {false};
	bool m_hdmiConnected {false};

	CstiMonitorTask *m_monitorTask {nullptr};
};
