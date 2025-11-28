// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IstiBluetooth.h"
#include "IstiLightRing.h"
#include "IstiStatusLED.h"
#include <stdio.h>
#include <vector>
#include <string>
#include "BleDevice.h"
#include "CstiEventQueue.h"
#include "CstiEventTimer.h"
#include "BaseBluetooth.h"


	
	class CstiDisabledBluetooth : public BaseBluetooth
	{
	public:
		CstiDisabledBluetooth ();
		virtual ~CstiDisabledBluetooth ();
		CstiDisabledBluetooth (const CstiDisabledBluetooth &) = delete;
		CstiDisabledBluetooth (CstiDisabledBluetooth &&) = delete;
		CstiDisabledBluetooth &operator= (const CstiDisabledBluetooth &) = delete;
		CstiDisabledBluetooth &operator= (CstiDisabledBluetooth &&) = delete;
		
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
		
	public:
		// Callbacks from central
		void deviceDiscoveredCallback (std::unique_ptr<BleIODevice> device); // TODO: Need event handler for this callback...unique_ptr issues
		void deviceConnectedCallback (const std::string &identifier);
		void deviceDisconnectedCallback (const std::string &identifier);
		void deviceRemovedCallback (const std::string &identifier);
		
		// Pulse Callback from device
		void pulseVerifyFirmware (BleDevice*);
		
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
		
		CstiSignal<> lightRingStoppedSignal;
		CstiSignal<> notificationStoppedSignal;
		
	};
	
