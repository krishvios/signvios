// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IstiHelios.h"
#include "stiDefs.h"
#include "stiTools.h"

#include "CstiBluetooth.h"
#include "DeviceInterface.h"
#include "NordicDfu.h"

#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiTimer.h"
#include "ILightRingVP.h"
#include "LedApi.h"
#include <cstdint>
#include <list>
#include <mutex>
#include <condition_variable>
#include <vector>

class CstiHelios: public IstiHelios, public CstiEventQueue
{

public:

	CstiHelios ();
	~CstiHelios () override;

	CstiHelios (const CstiHelios &other) = delete;
	CstiHelios (CstiHelios &&other) = delete;
	CstiHelios &operator= (const CstiHelios &other) = delete;
	CstiHelios &operator= (CstiHelios &&other) = delete;

	virtual stiHResult Startup ();

	stiHResult Initialize (
		CstiBluetooth *bluetoothObject);

	/*!
	 * \brief Starts Helios Ringer with custom pattern
	 * 
	 * \return stiHResult 
	 */
	stiHResult CustomPatternStart (
		const std::vector<std::string> &frames) override;

	/*!
	 * \brief Get color map
	 *
	 * \return mapped brightness
	 */
	std::map<IstiLightRing::EstiLedColor, lr_led_rgb> ColorMapGet ();

	/*!
	 * \brief Map brightness
	 *
	 * arg1 - brighness
	 *
	 * \return mapped brightness
	 */
	uint8_t BrightnessMap (
		uint8_t brightnessIn);

	/*!
	 * \brief Starts Helios Ringer with standard pattern
	 * 
	 * arg1 - default pattern number
	 * arg2 - red
	 * arg3 - green
	 * arg4 - blue
	 * arg5 - brighness
	 * 
	 * \return stiHResult 
	 */

	stiHResult PresetPatternStart (
		int pattern,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		unsigned char brightness) override;

	/*!
	 * \brief Stops Helios Ringer.
	 * 
	 * \return stiHResult 
	 */
	stiHResult Stop () override;

	/*!
	 * \brief sets the SignMail light on or off
	 *
	 * \return stiHResult
	 */
	stiHResult SignMailSet (
		bool set) override;

	/*!
	 * \brief sets the  MissedCall light on or off
	 *
	 * \return stiHResult
	 */
	stiHResult MissedCallSet (
		bool set) override;

	CstiSignal<int> heliosAlertSignal;

private:
	
	const static std::string FIRMWARE_PATH;
	
	/*!
	 * \brief status returned for a Helios event
	 */
	enum EHeliosEventReturnValue
	{
		eStartComplete,
		eStartFailed,
		eStopComplete,
		eStopFailed,
		eSignmailComplete,
		eSignmailFailed,
		eMissedCallComplete,
		eMissedCallFailed,
	};

	enum class EGetType
	{
		Info,
		Other,
	};

	stiHResult authenticatedDevicesGet (
		std::list<std::shared_ptr<SstiBluezDevice>> *heliosDevices);

	stiHResult InfoGet (
		std::shared_ptr<SstiBluezDevice> device);
	
	stiHResult FileAndVersionOnFileGet (
		std::string *filePath, std::string *fileVersion);
	
	stiHResult CheckVersionForDfu (
		std::shared_ptr<SstiBluezDevice> device);
	
	void ProcessPotentialDfuList ();

	struct LoggedDevice
	{
		LoggedDevice () = default;

		LoggedDevice (
			const std::string &macAddress,
			const std::string &serialNumber,
			const std::chrono::system_clock::time_point &timeChecked)
		:
			m_macAddress (macAddress),
			m_serialNumber (serialNumber),
			m_timeChecked (timeChecked)
		{
		}

		bool operator== (const LoggedDevice &other) const
		{
			return m_macAddress == other.m_macAddress
				&& m_serialNumber == other.m_serialNumber
				&& m_timeChecked == other.m_timeChecked;
		}

		std::string m_macAddress;
		std::string m_serialNumber;
		std::chrono::system_clock::time_point m_timeChecked;
	};

	std::vector<LoggedDevice> m_loggedDevices;

	void loggedDevicesLoad ();
	void loggedDevicesStore ();
	void devicesLog ();

	void eventConnectedDevicesAuthenticate ();

	void eventDeviceConnected (BluetoothDevice device);

	void eventDeviceDisconnected (BluetoothDevice device);
	
	void eventAuthenticationCharacteristicChanged (
		std::shared_ptr<SstiBluezDevice> device,
		const std::vector<uint8_t> &value);

	stiHResult eventWriteComplete (
		std::string &objectPath);
	
	void eventMessageProcess (
		std::shared_ptr<SstiBluezDevice> device,
		const std::vector<uint8_t> &value);

	stiHResult pulseServicePacketsClear (
		std::shared_ptr<SstiBluezDevice> device,
		const std::string &value);
	
	stiHResult pulseServiceWrite (
		std::shared_ptr<SstiBluezDevice> device,
		const std::string &command,
		const std::string &arguments);
		
	stiHResult eventCustomPatternStart (
		const std::vector<std::string> &frames);
	stiHResult eventPresetPatternStart (
		const std::string &arguments);
	stiHResult eventStop ();
	stiHResult eventSignMailSet (
		bool val);
	stiHResult eventMissedCallSet (
		bool val);

	std::map<IstiLightRing::EstiLedColor, lr_led_rgb> m_colorMap;

	CstiBluetooth *m_bluetoothObject = nullptr;
	int m_sequenceNumber = 0;

	std::list<std::shared_ptr<SstiBluezDevice>> m_devicesToUpdate;

	NordicDfu m_dfu;

	vpe::EventTimer m_dailyLogTimer;
	vpe::EventTimer m_connectionLogTimer;
	void eventDailyLogTimer ();
	void eventConnectLogTimer ();
	std::string m_loggedDevicesFile;

	CstiSignalConnection::Collection m_signalConnections;

	// Structures used for logging connects and disconnects
	struct connectionCounts
	{
		enum ConnectionState
		{
			Disconnected = 0,
			Connected = 1
		};

		ConnectionState lastConnectionState = Disconnected;

		int connections = 0;
		int disconnections =0;
	};

	std::map<std::string, connectionCounts> m_connectMap;

	stiHResult commandClear (
		const std::string &commmand);
	
	stiHResult commandSend (
		const std::string &commmand,
		const std::string &arguments);
};
