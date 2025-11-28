// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <stdio.h>
#include <vector>
#include <string>
#include "BleDevice.h"
#include "CstiEventQueue.h"
#include "CstiEventTimer.h"
#include "BaseBluetooth.h"

struct CentralImpl;

enum class ManagerCommand : int
{
	SCAN_START = 0,
	SCAN_STOP,
};
	
class CstiBluetooth : public BaseBluetooth, public CstiEventQueue
{
public:
	CstiBluetooth ();
	virtual ~CstiBluetooth ();
	CstiBluetooth (const CstiBluetooth &) = delete;
	CstiBluetooth (CstiBluetooth &&) = delete;
	CstiBluetooth &operator= (const CstiBluetooth &) = delete;
	CstiBluetooth &operator= (CstiBluetooth &&) = delete;
	
	// Inherited via IstiBluetooth
	virtual stiHResult Enable (bool bEnable) override;
	virtual stiHResult Scan () override;
	virtual stiHResult ScanCancel () override;
	virtual stiHResult Pair (const BluetoothDevice&) override;
	virtual stiHResult PairCancel () override;
	virtual stiHResult Connect (BluetoothDevice) override;
	virtual stiHResult PairedDeviceRemove (BluetoothDevice) override;
	virtual stiHResult PairedListGet (std::list<BluetoothDevice> *) override;
	virtual stiHResult ScannedListGet (std::list<BluetoothDevice> *) override;
	virtual stiHResult PincodeSet (const char *pszPinCode) override;
	virtual bool Powered () override;
	virtual stiHResult PairedDevicesLog () override;
	virtual stiHResult DeviceRename(const BluetoothDevice& device, const std::string newName) override;
	virtual stiHResult Disconnect(BluetoothDevice device) override;
	
	// Inherited via IstiLightRing
	virtual void PatternSet (Pattern patternId, EstiLedColor color, int brightness,	int flasherBrightness) override;
	virtual void notificationPatternSet(NotificationPattern pattern) override;
	virtual void notificationsStart (int nDuration, int nBrightness) override;
	virtual void notificationsStop () override;
	virtual void Start (int nSeconds) override;
	virtual void Stop () override;
	
	virtual ISignal<>& lightRingStoppedSignalGet () override;
	virtual ISignal<>& notificationStoppedSignalGet () override;
	
	// Inherited via IstiStatusLED
	virtual stiHResult Start (EstiLed eLED, unsigned unBlinkRate) override;
	virtual stiHResult Stop (EstiLed eLED) override;
	virtual stiHResult Enable (EstiLed eLED, EstiBool bEnable) override;
	virtual stiHResult PulseNotificationsEnable (bool enable) override;
	
	// Pulse methods
	void pulsePresetRingPatternStart (
		Pattern pattern,
		EstiLedColor color,
		uint8_t brightness);
	void pulseCustomRingPatternStart (const std::vector<std::string> &frames);
	void pulseRingStop ();
	void pulseMissedSet (bool value);
	void pulseSignmailSet (bool value);
	void pulseEnableNotificationsSet (bool value);
	
	// Additional Methods
	void scanFilterSet (const std::vector<std::string> &uuids);
	void scanForSeconds (double seconds);
	
	// Default ring pattern, color, and brightness
	static constexpr Pattern DefaultPattern = Pattern::Flash;
	static constexpr EstiLedColor DefaultColor = estiLED_COLOR_WHITE;
	static constexpr uint8_t DefaultBrightness = 0xFF;
	
private:
	// Event Handlers
	void initializeEvent ();
	void scanEvent ();
	void scanCancelEvent ();
	void startEvent ();
	void stopEvent ();
	void pulsePresetRingPatternStartEvent (
		Pattern pattern,
		EstiLedColor color,
		uint8_t brightness);
	void pulseCustomRingPatternStartEvent (const std::vector<std::string> &frames);
	void pulseRingStopEvent ();
	void pulseMissedSetEvent ();
	void pulseSignmailSetEvent ();
	void pulseEnableNotificationsSetEvent (bool value);
	void scanFilterSetEvent (const std::vector<std::string> &uuids);
	void scanForSecondsEvent (double seconds);
	void deviceConnectedCallbackEvent (const std::string &identifier);
	void deviceDisconnectedCallbackEvent (const std::string &identifier);
	void deviceRemovedCallbackEvent (const std::string &identifier);
	
	void connectDevice (BleIODevice &device);
	void disconnectDevice (BleIODevice &device);
	
public:
	// Callbacks from central
	void deviceDiscoveredCallback (std::unique_ptr<BleIODevice> device); // TODO: Need event handler for this callback...unique_ptr issues
	void deviceConnectedCallback (const std::string &identifier);
	void deviceDisconnectedCallback (const std::string &identifier);
	void deviceRemovedCallback (const std::string &identifier);
	
	// Pulse Callback from device
	void pulseVerifyFirmware (BleDevice*);
	
private:
	
	void setCallbacks ();
	
	void updatePairedDevices ();
		
public: //signal getter implementations for IstiBluetooth
	
	ISignal<BluetoothDevice> &deviceAddedSignalGet () override { return deviceAddedSignal; };
	ISignal<BluetoothDevice> &deviceConnectedSignalGet () override { return deviceConnectedSignal; };
	ISignal<> &deviceConnectFailedSignalGet () override { return deviceConnectFailedSignal; };
	ISignal<BluetoothDevice> &deviceDisconnectedSignalGet () override { return deviceDisconnectedSignal; };
	ISignal<> &deviceDisconnectFailedSignalGet () override { return deviceDisconnectFailedSignal; };
	ISignal<> &deviceRemovedSignalGet () override { return deviceRemovedSignal; };
	ISignal<> &scanCompleteSignalGet () override { return scanCompleteSignal; };
	ISignal<> &scanFailedSignalGet () override { return scanFailedSignal; };
	ISignal<> &pairingCompleteSignalGet () override { return pairingCompleteSignal; };
	ISignal<> &pairingFailedSignalGet () override { return pairingFailedSignal; };
	ISignal<> &pairingCanceledSignalGet () override { return pairingCanceledSignal; };
	ISignal<> &pairingInputTimeoutSignalGet () override { return pairingInputTimeoutSignal; };
	ISignal<> &pairedMaxCountExceededSignalGet () override { return pairedMaxCountExceededSignal; };
	ISignal<> &unpairCompleteSignalGet () override { return unpairCompleteSignal; };
	ISignal<> &unpairFailedSignalGet () override { return unpairFailedSignal; };
	ISignal<std::string> &displayPincodeSignalGet () override { return displayPincodeSignal; };
	ISignal<> &requestPincodeSignalGet () override { return requestPincodeSignal; };
	ISignal<> &deviceAliasChangedSignalGet () override { return deviceAliasChangedSignal; };
	
	CstiSignal<BluetoothDevice> deviceAddedSignal;
	CstiSignal<BluetoothDevice> deviceConnectedSignal;
	CstiSignal<> deviceConnectFailedSignal;
	CstiSignal<BluetoothDevice> deviceDisconnectedSignal;
	CstiSignal<> deviceDisconnectFailedSignal;
	CstiSignal<> deviceRemovedSignal;
	CstiSignal<> scanCompleteSignal;
	CstiSignal<> scanFailedSignal;
	CstiSignal<> pairingCompleteSignal;
	CstiSignal<> pairingFailedSignal;
	CstiSignal<> pairingCanceledSignal;
	CstiSignal<> pairingInputTimeoutSignal;
	CstiSignal<> pairedMaxCountExceededSignal;
	CstiSignal<> unpairCompleteSignal;
	CstiSignal<> unpairFailedSignal;
	CstiSignal<std::string> displayPincodeSignal;
	CstiSignal<> requestPincodeSignal;
	CstiSignal<> deviceAliasChangedSignal;
	
private:
	
	struct PulseState
	{
		bool notificationsEnabled = true;
		bool missedSet = false;
		bool signmailSet = false;
	};
	
	PulseState m_pulseState;
	
	std::vector<std::unique_ptr<BleDevice>> m_deviceList;
	std::vector<std::string> m_dfuDevicesWaitingOn;
	
	vpe::EventTimer m_scanTimer;
	vpe::EventTimer m_updatePairedDevicesTimer;
	CstiSignalConnection::Collection m_signalConnections;
	
	CentralImpl *m_impl = nullptr;
	
	struct StatusLed
	{
		bool m_state = false;
		bool m_enabled = true;
	};
	
	struct RgbColor
	{
		RgbColor (uint8_t r, uint8_t g, uint8_t b) :
		Red (r),
		Green (g),
		Blue (b) {}
		
		const uint8_t Red;
		const uint8_t Green;
		const uint8_t Blue;
	};
	
	Pattern m_pattern = DefaultPattern;
	EstiLedColor m_color = DefaultColor;
	uint8_t m_brightness = DefaultBrightness;
	StatusLed m_missedLed;
	StatusLed m_signMailLed;
	bool m_patternsAndColorsAllowed = false;
	
	const std::map<EstiLedColor, RgbColor> m_ledColorMap =
	{
		{estiLED_COLOR_TEAL, 		RgbColor(0x00, 0xe6, 0x6d)},
		{estiLED_COLOR_LIGHT_BLUE, 	RgbColor(0x30, 0xff, 0xee)},
		{estiLED_COLOR_BLUE, 		RgbColor(0x00, 0x00, 0xff)},
		{estiLED_COLOR_VIOLET, 		RgbColor(0x78, 0x00, 0xff)},
		{estiLED_COLOR_MAGENTA, 	RgbColor(0xf0, 0x00, 0xff)},
		{estiLED_COLOR_ORANGE, 		RgbColor(0xff, 0x78, 0x00)},
		{estiLED_COLOR_YELLOW, 		RgbColor(0xff, 0xff, 0x00)},
		{estiLED_COLOR_RED, 		RgbColor(0xff, 0x00, 0x00)},
		{estiLED_COLOR_PINK, 		RgbColor(0xff, 0x00, 0x78)},
		{estiLED_COLOR_GREEN, 		RgbColor(0x00, 0xff, 0x00)},
		{estiLED_COLOR_LIGHT_GREEN, RgbColor(0x00, 0xff, 0xa0)},
		{estiLED_COLOR_CYAN, 		RgbColor(0x00, 0xf0, 0xff)},
		{estiLED_COLOR_WHITE, 		RgbColor(0xff, 0xff, 0xff)}
	};
	
	 CstiSignal<> lightRingStoppedSignal;
	 CstiSignal<> notificationStoppedSignal;
};

