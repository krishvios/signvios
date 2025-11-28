// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#include "CstiHelios.h"
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <openssl/aes.h>
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "stiRemoteLogEvent.h"
#include "GattCharacteristicInterface.h"
#include "stiTrace.h"


static const std::string pulseServiceWriteUuid = PULSE_WRITE_CHARACTERISTIC_UUID;
static const std::string pulseServiceReadUuid =  PULSE_READ_CHARACTERISTIC_UUID;
static const std::string pulseServiceAuthenticationUuid =  PULSE_AUTHENTICATE_CHARACTERISTIC_UUID;
static const std::string AUTHENTICATE_KEY = ">T*u(eoT?EqQ!_y9";
static const std::vector<uint8_t> AUTHORIZED = {'A','U','T','H','O','R','I','Z','E','D'};

#define PULSE_SERVICE_MAX_COMMAND_TOTAL_LENGTH 128

static const std::string PULSE_SERVICE_PATTERN_START_COMMAND = "PATTERN_START";
static const std::string PULSE_SERVICE_FRAME_COMMAND = "FRAME";

static const std::string PULSE_SERVICE_PRESET_PATTERN_START_COMMAND = "PRESET_PATTERN_START";

static const std::string PULSE_SERVICE_PATTERN_STOP_COMMAND = "PATTERN_STOP";

static const std::string PULSE_SERVICE_MISSED_ON_COMMAND = "MISSED_ON";
static const std::string PULSE_SERVICE_MISSED_OFF_COMMAND = "MISSED_OFF";

static const std::string PULSE_SERVICE_SIGNMAIL_ON_COMMAND = "SIGNMAIL_ON";
static const std::string PULSE_SERVICE_SIGNMAIL_OFF_COMMAND = "SIGNMAIL_OFF";

static const std::string PULSE_SERVICE_INFO_GET_COMMAND = "INFO_GET";
static const std::string PULSE_SERVICE_INFO_RESPONSE_COMMAND = "INFO_RESPONSE";

static const int DAILY_LOG_INTERVAL = 60 * 60 * 24 * 1000; // 1 day in milliseconds
static const int CONNECTION_LOG_INTERVAL = 5 * 1000; // 5 seconds

const std::map<IstiLightRing::EstiLedColor, lr_led_rgb> heliosColorMap =
{
	{IstiLightRing::estiLED_COLOR_TEAL, 		{0x00, 0xe6, 0x6d}},
	{IstiLightRing::estiLED_COLOR_LIGHT_BLUE, 	{0x30, 0xff, 0xee}},
	{IstiLightRing::estiLED_COLOR_BLUE, 		{0x00, 0x00, 0xff}},
	{IstiLightRing::estiLED_COLOR_VIOLET, 		{0x78, 0x00, 0xff}},
	{IstiLightRing::estiLED_COLOR_MAGENTA, 		{0xf0, 0x00, 0xff}},
	{IstiLightRing::estiLED_COLOR_ORANGE, 		{0xff, 0x78, 0x00}},
	{IstiLightRing::estiLED_COLOR_YELLOW, 		{0xff, 0xff, 0x00}},
	{IstiLightRing::estiLED_COLOR_RED, 			{0xff, 0x00, 0x00}},
	{IstiLightRing::estiLED_COLOR_PINK, 		{0xff, 0x00, 0x78}},
	{IstiLightRing::estiLED_COLOR_GREEN, 		{0x00, 0xff, 0x00}},
	{IstiLightRing::estiLED_COLOR_LIGHT_GREEN, 	{0x00, 0xff, 0xa0}},
	{IstiLightRing::estiLED_COLOR_CYAN, 		{0x00, 0xf0, 0xff}},
	{IstiLightRing::estiLED_COLOR_WHITE, 		{0xff, 0xff, 0xff}}
};


CstiHelios::CstiHelios ()
:
	CstiEventQueue("CstiHelios"),
	m_dfu (this),
	m_dailyLogTimer (DAILY_LOG_INTERVAL, this),
	m_connectionLogTimer (CONNECTION_LOG_INTERVAL, this)
{
	// Handle DFU result signal
	//
	// Currently, we do the same thing whether the DFU succeeds or fails. This is done so that
	// we can connect to devices that need a firmware update but DFU is failing for some reason.
	//
	// If a DFU fails on a device that needs a firmware update, we cannot connect to the device.
	// Everytime we try to connect, a DFU is attempted which disconnects the device. This loop
	// continues until a DFU is successful.
	m_signalConnections.push_back (m_dfu.dfuResultSignal.Connect (
			[this](stiHResult hResult, std::string deviceAddress){

				stiDEBUG_TOOL (g_stiHeliosDfuDebug,
					stiTrace ("Helios handling DFU %s.\n", hResult == stiRESULT_SUCCESS ? "complete." : "failed.");
				);

				if (!m_devicesToUpdate.empty ())
				{
					auto deviceIter = std::find_if (m_devicesToUpdate.begin (), m_devicesToUpdate.end (),
										[deviceAddress](std::shared_ptr<SstiBluezDevice> device){
											return device->bluetoothDevice.address == deviceAddress;
										});

					if (deviceIter != m_devicesToUpdate.end ())
					{
						(*deviceIter)->firmwareVerified = true;
					}

					// Remove this device from the dfu list
					m_devicesToUpdate.erase (std::remove_if (m_devicesToUpdate.begin (), m_devicesToUpdate.end (),
												[deviceAddress](std::shared_ptr<SstiBluezDevice> device){
													return device->bluetoothDevice.address == deviceAddress;
												}),
												m_devicesToUpdate.end ());

					ProcessPotentialDfuList ();
				}
			}));

	// Set the path to the loggedDevices.dat file
	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream LoggedDevicesFile;
	
	LoggedDevicesFile << DynamicDataFolder << "bluetooth/loggedDevices.dat";
	
	m_loggedDevicesFile = LoggedDevicesFile.str ();

	// Connect the timers to a handler
	m_signalConnections.push_back (m_dailyLogTimer.timeoutSignal.Connect (
			std::bind(&CstiHelios::eventDailyLogTimer, this)));

	m_dailyLogTimer.start();

	m_signalConnections.push_back (m_connectionLogTimer.timeoutSignal.Connect (
			std::bind (&CstiHelios::eventConnectLogTimer, this)));
}


stiHResult CstiHelios::Initialize(CstiBluetooth *bluetoothObject)
{
	m_bluetoothObject = bluetoothObject;

	m_signalConnections.push_back (bluetoothObject->deviceConnectedSignal.Connect (
		[this](BluetoothDevice device){
			PostEvent ([this, device]{
					eventDeviceConnected (device);
				});
		}));

	m_signalConnections.push_back (bluetoothObject->deviceDisconnectedSignal.Connect (
		[this](BluetoothDevice device){
			PostEvent ([this, device]{
					eventDeviceDisconnected (device);
				});
		}));

	m_signalConnections.push_back (SstiGattCharacteristic::valueChangedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface, std::string uuid, const std::vector<uint8_t> &value)
		{
			if (uuid == pulseServiceAuthenticationUuid)
			{
				PostEvent ([this, deviceInterface, value]{eventAuthenticationCharacteristicChanged (deviceInterface, value);});
			}
			else if (uuid == pulseServiceReadUuid)
			{
				PostEvent ([this, deviceInterface, value]{eventMessageProcess (deviceInterface, value);});
			}
		}));

	loggedDevicesLoad ();

	m_colorMap = heliosColorMap;

	return(stiRESULT_SUCCESS);
}


CstiHelios::~CstiHelios()
{
	CstiEventQueue::StopEventLoop();
}


/*
 *\ brief - start this event loop
 */
stiHResult CstiHelios::Startup ()
{
	stiLOG_ENTRY_NAME (CstiHelios::Startup);

	CstiEventQueue::StartEventLoop();

	// In some situations, deviceConnected signals are emitted before
	// Helios has been created. In this case, these signals are missed and,
	// although connected, these devices are never authenticated.
	PostEvent ([this]{eventConnectedDevicesAuthenticate ();});

	return stiRESULT_SUCCESS;
}


void CstiHelios::eventDeviceConnected (BluetoothDevice device)
{
	auto deviceAddress = device.address;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("%s: enter: device address = %s\n", __func__, deviceAddress.c_str());
	);

	auto heliosDevice = m_bluetoothObject->pairedHeliosDeviceGet(deviceAddress);

	if (heliosDevice)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("eventDeviceConnected: A helios device was connected\n");
		);

		// Read the authentication characteristic value. This will cause a property change event on the
		// authentication characteristic, which will start the authentication process.
		m_bluetoothObject->gattCharacteristicRead (heliosDevice, pulseServiceAuthenticationUuid);

		// If there is nothing in the map then start the timer to send the log message
		// later (for the case where we have rapid connects/disconnects).
		if (m_connectMap.empty())
		{
			m_connectionLogTimer.restart ();
		}

		// Increment the connections count and indicate the last state was a connection
		m_connectMap[deviceAddress].connections++;
		m_connectMap[deviceAddress].lastConnectionState = connectionCounts::Connected;
	}
}


void CstiHelios::eventDeviceDisconnected (BluetoothDevice device)
{
	auto deviceAddress = device.address;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("%s: enter: device address = %s\n", __func__, deviceAddress.c_str());
	);

	auto heliosDevice = m_bluetoothObject->pairedHeliosDeviceGet (deviceAddress);

	if (heliosDevice)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("eventDeviceDisconnected: A helios device was disconnected\n");
		);

		heliosDevice->authenticated = false;

		// If there is nothing in the map then start the timer to send the log message
		// later (for the case where we have rapid connects/disconnects).
		if (m_connectMap.empty())
		{
			m_connectionLogTimer.restart ();
		}

		// Increment the connections count and indicate the last state was a disconnection
		m_connectMap[deviceAddress].disconnections++;
		m_connectMap[deviceAddress].lastConnectionState = connectionCounts::Disconnected;
	}
}


/*!\brief Handles the time event for the connection timer
 *
 */
void CstiHelios::eventConnectLogTimer ()
{
	// Log each of the entries in the map.
	for (auto &pair : m_connectMap)
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=Connections MAC=%s Connects=%d Disconnects=%d Last=%d",
				pair.first.c_str (), pair.second.connections, pair.second.disconnections,
				pair.second.lastConnectionState);
	}

	// Clear the map so that the timer will be started the next time
	// we have a connection or disconnection.
	m_connectMap.clear ();
}


/*!\brief Logs the currently paired devices and resets to the timer again
 *
 */
void CstiHelios::eventDailyLogTimer ()
{
	devicesLog ();

	m_dailyLogTimer.restart();
}


/*!\brief Loads the logged devices from the file on disk
 *
 */
void CstiHelios::loggedDevicesLoad ()
{
	FILE *file = fopen (m_loggedDevicesFile.c_str (), "r");

	if (file)
	{
		for (;;)
		{
			char storedMacAddress[64];
			char storedSerialNumber[64];
			std::chrono::system_clock::rep storedTimeChecked;

#if DEVICE == DEV_NTOUCH_VP3 || DEVICE == DEV_NTOUCH_VP4 || DEVICE == DEV_X86
			auto numRead = fscanf (file, "%s %s %ld\n", storedMacAddress, storedSerialNumber, &storedTimeChecked);
#else
			auto numRead = fscanf (file, "%s %s %lld\n", storedMacAddress, storedSerialNumber, &storedTimeChecked);
#endif

			if (numRead != 3)
			{
				break;
			}

			std::chrono::system_clock::duration d(storedTimeChecked);
			std::chrono::system_clock::time_point timeChecked(d);

			m_loggedDevices.emplace_back(storedMacAddress, storedSerialNumber, timeChecked);
		}

		fclose (file);
	}

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		for (auto &&device: m_loggedDevices)
		{
			stiTrace ("Loaded macAddress: %s, serialNumber: %s, timeChecked: %lld\n",
					  device.m_macAddress.c_str (), device.m_serialNumber.c_str (), device.m_timeChecked.time_since_epoch ().count ());
		}
	);


}


/*!\brief Stores the logged devices to the file on disk
 *
 */
void CstiHelios::loggedDevicesStore ()
{
	std::ofstream logFile;

	logFile.open (m_loggedDevicesFile);

	if (logFile.is_open ())
	{
		for (auto &&loggedDevice: m_loggedDevices)
		{
			logFile << loggedDevice.m_macAddress << " "
					<< loggedDevice.m_serialNumber << " "
					<< loggedDevice.m_timeChecked.time_since_epoch ().count () << "\n";
		}
	}
}


/*!\brief Returns the time parameter in a formatted string
 *
 */
std::string formattedTimeGet (std::chrono::system_clock::time_point time)
{
	// todo: replace with std::put_time when it becomes available.
	time_t authenticationTime = std::chrono::system_clock::to_time_t (time);
	tm tmAuthenticationTime = {};
	const auto timeBufferMaxLength = 64;
	char timeBuffer[timeBufferMaxLength];

	strftime (timeBuffer, sizeof (timeBuffer) - 1, "%m/%d/%Y %H:%M:%S",
			gmtime_r (&authenticationTime, &tmAuthenticationTime));

	return timeBuffer;
}


/*!\brief Gathers the serial numbers from each of the paired devices and reports them to core
 *
 */
void CstiHelios::devicesLog ()
{
	std::stringstream devices;
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::vector<LoggedDevice> loggedDevices;

	std::list<std::shared_ptr<SstiBluezDevice>> heliosDevices;
	m_bluetoothObject->heliosPairedDevicesGet(&heliosDevices);

	for (auto &&device: heliosDevices)
	{
		// If this isn't the first device in the string then add a vertical bar to separate this
		// new entry from the previous entry.
		if (!devices.str().empty())
		{
			devices << "|";
		}

		if (device->bluetoothDevice.connected)
		{
			// If it doesn't have a serial number then it should get
			// picked up in the next logging event after the endpoint receives the InfoGet response.
			if (!device->serialNumber.empty ())
			{
				devices << device->serialNumber;

				devices << ",";

				devices << formattedTimeGet (now);

				loggedDevices.emplace_back(device->bluetoothDevice.address, device->serialNumber, now);
			}
		}
		else
		{
			// This device is only paired. Lookup the endpoint's mac address in the previously logged
			// devices to fetch the serial number and the last time this device was logged.
			auto deviceIter = std::find_if (m_loggedDevices.begin (), m_loggedDevices.end (),
								[device](LoggedDevice loggeddevice) {
									return device->bluetoothDevice.address == loggeddevice.m_macAddress;
								});

			if (deviceIter != m_loggedDevices.end ())
			{
				devices << deviceIter->m_serialNumber;

				devices << ",";

				devices << formattedTimeGet (deviceIter->m_timeChecked);

				loggedDevices.emplace_back(deviceIter->m_macAddress, deviceIter->m_serialNumber, deviceIter->m_timeChecked);
			}
		}
	}

	if (loggedDevices != m_loggedDevices)
	{
		m_loggedDevices = loggedDevices;

		// Send the device information to core.
		WillowPM::PropertyManager::getInstance ()->propertySet (REMOTE_FLASHER_STATUS, devices.str ());
		WillowPM::PropertyManager::getInstance ()->PropertySend (REMOTE_FLASHER_STATUS, estiPTypePhone);

		loggedDevicesStore ();
	}
}


void CstiHelios::eventAuthenticationCharacteristicChanged (
	std::shared_ptr<SstiBluezDevice> device,
	const std::vector<uint8_t> &value)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (device)
	{
		const size_t nonceSize = 16;
						
		if (value == AUTHORIZED)
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("eventAuthenticationCharacteristicChanged: Received AUTHORIZED from helios device\n");
			);
			
			// Set device value to authorized
			device->authenticated = true;

			InfoGet (device);

			devicesLog ();

			stiRemoteLogEventSend ("EventType=Pulse Reason=\"Authenticated\" MAC=%s", device->bluetoothDevice.address.c_str ());
		}
		else if (value.size () == nonceSize)
		{
			// This must be the nonce
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("eventAuthenticationCharacteristicChanged: Helios nonce: ");
				for (auto &byte : value)
				{
					stiTrace ("%u ", byte);
				}
				stiTrace ("\n");
			);
			
			std::vector<uint8_t> response;
			response.resize (nonceSize);
			AES_KEY aesKey;
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			AES_set_encrypt_key (reinterpret_cast<const unsigned char*>(AUTHENTICATE_KEY.data ()), 128, &aesKey);
			AES_ecb_encrypt (value.data (), response.data (), &aesKey, AES_ENCRYPT);

			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("eventAuthenticationCharacteristicChanged: Encoded response: ");
				for (auto &byte : response)
				{
					stiTrace ("%u ", byte);
				}
				stiTrace ("\n");
			);
							
			hResult = m_bluetoothObject->gattCharacteristicWrite (
					device,
					pulseServiceAuthenticationUuid,
					reinterpret_cast<const char *>(response.data ()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					nonceSize);
			
			if (stiIS_ERROR(hResult))
			{
				stiASSERTMSG (estiFALSE, "GATT write failed\n");
			}
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Helios authentication value is not recognized\n");
		}
	}
}


stiHResult CstiHelios::pulseServicePacketsClear (
	std::shared_ptr<SstiBluezDevice> device,
	const std::string &value)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::shared_ptr<SstiGattCharacteristic> characteristic = m_bluetoothObject->characteristicFromConnectedDeviceGet (device, pulseServiceWriteUuid);
	stiTESTCOND (characteristic, stiRESULT_ERROR);
	
	stiTESTCOND (!value.empty(), stiRESULT_ERROR);
	
	hResult = characteristic->WriteQueuePacketsRemove (value);
	stiTESTRESULT();
	
STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG (estiFALSE,
			"Error: clearing packets on %s",
			device->bluetoothDevice.name.c_str ());
	}
	
	return (hResult);
}


stiHResult CstiHelios::pulseServiceWrite (
	std::shared_ptr<SstiBluezDevice> device,
	const std::string &command,
	const std::string &arguments)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::stringstream commandString;
	
	auto characteristic = m_bluetoothObject->characteristicFromConnectedDeviceGet (device, pulseServiceWriteUuid);
	stiTESTCOND (characteristic, stiRESULT_ERROR);
	
	stiTESTCONDMSG (!command.empty (), stiRESULT_ERROR, "Error: command string is empty\n");

	commandString << command << " " << characteristic->WriteSequenceNumberGet ();

	if (!arguments.empty ())
	{
		commandString << " " << arguments;
	}

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("pulseServiceWrite: writing to device %s: service %s: value %s\n",
			device->bluetoothDevice.name.c_str(),
			characteristic->servicePathGet ().c_str(),
			commandString.str ().c_str ());
	);
	
	hResult = characteristic->Write (commandString.str ());
	stiTESTRESULT();
	
STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG (estiFALSE,
					"Error: writing to device %s: service %s: value %s\n",
					device->bluetoothDevice.name.c_str (),
					characteristic->servicePathGet ().c_str(),
					commandString.str ().c_str ());
	}

	return (hResult);
}

stiHResult CstiHelios::commandClear (
	const std::string &commmand)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::list<std::shared_ptr<SstiBluezDevice>> heliosDevices;

	authenticatedDevicesGet (&heliosDevices);

	for (auto device : heliosDevices)
	{
		hResult = pulseServicePacketsClear (device, commmand);
		stiASSERT (!stiIS_ERROR (hResult));
	}
	
	return (hResult);
}

stiHResult CstiHelios::commandSend (
	const std::string &commmand,
	const std::string &arguments)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::list<std::shared_ptr<SstiBluezDevice>> heliosDevices;

	authenticatedDevicesGet (&heliosDevices);

	for (auto device : heliosDevices)
	{
		hResult = pulseServiceWrite (
			device,
			commmand,
			arguments);
		stiASSERT (!stiIS_ERROR (hResult));
	}
	
	return (hResult);
}

stiHResult CstiHelios::CustomPatternStart (
	const std::vector<std::string> &frames)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::CustomPatternStart: frames = %d\n", frames.size ());
	);
	
	PostEvent (std::bind(&CstiHelios::eventCustomPatternStart, this, frames));

	return (hResult);
}

stiHResult CstiHelios::eventCustomPatternStart (
	const std::vector<std::string> &frames)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EHeliosEventReturnValue returnValue = eStartFailed;
	
	char buffer[PULSE_SERVICE_MAX_COMMAND_TOTAL_LENGTH];
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::eventCustomPatternStart: frames = %d\n", frames.size ());
	);
	
	if (!frames.empty())
	{
#if DEVICE == DEV_NTOUCH_VP3 || DEVICE == DEV_NTOUCH_VP4 || DEVICE == DEV_X86
		sprintf (buffer, "%ld", frames.size ());
#else
		sprintf (buffer, "%d", frames.size ());
#endif
		
		std::string arguments (buffer);
		
		commandClear (PULSE_SERVICE_PRESET_PATTERN_START_COMMAND);
		commandClear (PULSE_SERVICE_PATTERN_START_COMMAND);
		commandSend (PULSE_SERVICE_PATTERN_START_COMMAND, arguments);
				
		// Send each frame
		for (std::string frame : frames)
		{
			commandSend (PULSE_SERVICE_FRAME_COMMAND, frame);
		}
		
		returnValue = eStartComplete;
	}
	
	heliosAlertSignal.Emit (returnValue);
	
	return (hResult);
}

std::map<IstiLightRing::EstiLedColor, lr_led_rgb> CstiHelios::ColorMapGet ()
{
	return m_colorMap;
}


/*
 * Helios takes 8 bits of brightness and lightring uses 4 bits
 */
uint8_t CstiHelios::BrightnessMap (
	uint8_t brightnessIn)
{
	uint8_t brightnessOut;

	// Clamp max
	if (brightnessIn > 0x0f || brightnessIn == 0)
	{
		brightnessIn = 0x0f;
	}

	// TODO: Decide a mapping if we should honor brightness at all
	// It was decided to use full brightness all the time.
	// Helios has physical brightnes switches on the back side of the device
	//brightnessOut = brightness * 16;
	brightnessOut = 0xff;

	return brightnessOut;
}

stiHResult CstiHelios::PresetPatternStart (
	int pattern,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	unsigned char brightness)
{
	char buffer[PULSE_SERVICE_MAX_COMMAND_TOTAL_LENGTH];
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::PresetPatternStart: pattern = %d, r = %x, g = %x, b = %x, brightness = %d\n", pattern, r, g, b, brightness);
	);
	
	sprintf (buffer, "%d %2.2x %2.2x %2.2x %2.2x", pattern, r, g, b, brightness);
	
	std::string arguments (buffer);
	
	PostEvent (std::bind(&CstiHelios::eventPresetPatternStart, this, arguments));
	
	return (stiRESULT_SUCCESS);
}

stiHResult CstiHelios::eventPresetPatternStart (
	const std::string &arguments)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EHeliosEventReturnValue returnValue = eStartFailed;
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::eventPresetPatternStart: arguments = %s\n", arguments.c_str());
	);

	if (!arguments.empty ())
	{
		commandClear (PULSE_SERVICE_PRESET_PATTERN_START_COMMAND);
		commandClear (PULSE_SERVICE_PATTERN_START_COMMAND);
		commandSend (PULSE_SERVICE_PRESET_PATTERN_START_COMMAND, arguments);
	}
	
	returnValue = eStartComplete;
	
	heliosAlertSignal.Emit (returnValue);
	
	return (hResult);
}


stiHResult CstiHelios::Stop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent (std::bind(&CstiHelios::eventStop, this));

	return (hResult);
}


stiHResult CstiHelios::eventStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EHeliosEventReturnValue returnValue = eStopFailed;
	std::string arguments;
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::eventStop\n");
	);

	commandClear (PULSE_SERVICE_PRESET_PATTERN_START_COMMAND);
	commandClear (PULSE_SERVICE_PATTERN_START_COMMAND);
	commandSend (PULSE_SERVICE_PATTERN_STOP_COMMAND, arguments);

	returnValue = eStopComplete;

	heliosAlertSignal.Emit (returnValue);
	
	return (hResult);
}


stiHResult CstiHelios::SignMailSet (
	bool val)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent(std::bind(&CstiHelios::eventSignMailSet, this, val));

	return (hResult);
}

stiHResult CstiHelios::eventSignMailSet (
	bool val)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EHeliosEventReturnValue returnValue = eSignmailFailed;
	std::string arguments;
		
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::eventSignMailSet: val = %d\n", val);
	);

	auto signMailCommand = val == 0 ? PULSE_SERVICE_SIGNMAIL_OFF_COMMAND : PULSE_SERVICE_SIGNMAIL_ON_COMMAND;

	commandClear (PULSE_SERVICE_SIGNMAIL_ON_COMMAND);
	
	commandClear (PULSE_SERVICE_SIGNMAIL_OFF_COMMAND);
	
	commandSend (signMailCommand, arguments);

	returnValue = eSignmailComplete;
	
	heliosAlertSignal.Emit (returnValue);
	
	return(hResult);
}

/*
 * \brief queue up the event to change the state of the missed call indicator
 *
 * \return stiHResult
 */
stiHResult CstiHelios::MissedCallSet (
	bool val)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent (std::bind(&CstiHelios::eventMissedCallSet, this, val));

	return (hResult);
}

/*
 * \brief send messages to each Helios to change the state of the missed call indicator
 *
 * \return stiHResult
 */
stiHResult CstiHelios::eventMissedCallSet (
	bool val)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EHeliosEventReturnValue returnValue = eMissedCallFailed;
	std::string arguments;
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("CstiHelios::eventMissedCallSet: val = %d\n", val);
	);

	auto missedCallCommand = val == 0 ? PULSE_SERVICE_MISSED_OFF_COMMAND : PULSE_SERVICE_MISSED_ON_COMMAND;

	commandClear (PULSE_SERVICE_MISSED_ON_COMMAND);
	
	commandClear (PULSE_SERVICE_MISSED_OFF_COMMAND);
	
	commandSend (missedCallCommand, arguments);

	returnValue = eMissedCallComplete;
	
	heliosAlertSignal.Emit (returnValue);
	
	return (hResult);
}


void CstiHelios::eventConnectedDevicesAuthenticate ()
{
	std::list<std::shared_ptr<SstiBluezDevice>> heliosDevices;

	m_bluetoothObject->heliosPairedDevicesGet (&heliosDevices);

	for (auto &device : heliosDevices)
	{
		if (device->bluetoothDevice.connected && !device->authenticated)
		{
			// Read the authentication characteristic value. This will cause a property change event on the
			// authentication characteristic, which will start the authentication process.
			m_bluetoothObject->gattCharacteristicRead (device, pulseServiceAuthenticationUuid);
		}
	}
}


stiHResult CstiHelios::authenticatedDevicesGet (
	std::list<std::shared_ptr<SstiBluezDevice>> *heliosDevices)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = m_bluetoothObject->heliosDevicesGet (heliosDevices);
	stiTESTRESULT ();
	
	heliosDevices->erase (std::remove_if (heliosDevices->begin (), heliosDevices->end (),
							[](std::shared_ptr<SstiBluezDevice> device){
								return !device->authenticated;
							}),
							heliosDevices->end ());

STI_BAIL:
	
	return hResult;
}


/*
 * check the result from a protocol write.
 *
 * device is the paired device that issued the write.
 * if result is not null we are expecting a result string as well
 *
 * returns true if everything is fine.
 */
void CstiHelios::eventMessageProcess (
	std::shared_ptr<SstiBluezDevice> device,
	const std::vector<uint8_t> &value)
{
	std::string receivedResponse (value.begin (), value.end ());

	// Discard line returns at end of response
	while (receivedResponse.back () == '\n')
	{
		receivedResponse.pop_back ();
	}

	std::istringstream iss(receivedResponse);
	std::vector<std::string> tokens;
	std::copy(std::istream_iterator<std::string>(iss),
		 std::istream_iterator<std::string>(),
		 std::back_inserter(tokens));

	if (tokens[0] == PULSE_SERVICE_INFO_RESPONSE_COMMAND)
	{
		if (tokens.size() >= 4)
		{
			device->version = tokens[2];
			device->serialNumber = tokens[3];

			if (!device->firmwareVerified)
			{
				if (m_devicesToUpdate.empty ())
				{
					m_devicesToUpdate.push_back (device);

					ProcessPotentialDfuList ();
				}
				else
				{
					m_devicesToUpdate.push_back (device);
				}
			}
		}
	}
	else if (tokens[0] == "RESPONSE")
	{
		// Currently we do nothing with responses but perhaps we should be checking sequence numbers
	}
	else if (tokens[0] == "ALERT_SWITCH_HIGH")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=AlertSwitch Value=High MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "ALERT_SWITCH_MEDIUM")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=AlertSwitch Value=Medium MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "ALERT_SWITCH_LOW")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=AlertSwitch Value=Low MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "ALERT_SWITCH_OFF")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=AlertSwitch Value=Off MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "RGB_SWITCH_HIGH")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=RGBSwitch Value=High MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "RGB_SWITCH_MEDIUM")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=RGBSwitch Value=Medium MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "RGB_SWITCH_LOW")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=RGBSwitch Value=Low MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else if (tokens[0] == "RGB_SWITCH_OFF")
	{
		stiRemoteLogEventSend ("EventType=Pulse Reason=RGBSwitch Value=Off MAC=%s Serial=%s",
				device->bluetoothDevice.address.c_str (),
				device->serialNumber.c_str ());
	}
	else
	{
		stiASSERTMSG (false, "Unhandle message from Helios: %s", tokens[0].c_str ());
	}
}


stiHResult CstiHelios::InfoGet (
	std::shared_ptr<SstiBluezDevice> device)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("CstiHelios::InfoGet\n");
	);
	
	char data[PULSE_SERVICE_MAX_COMMAND_TOTAL_LENGTH];
	
	sprintf (data, "INFO_GET %d", m_sequenceNumber);

	hResult = m_bluetoothObject->gattCharacteristicWrite (
				device,
				pulseServiceWriteUuid,
				data,
				static_cast<int>(strlen(data) + 1));
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
}


stiHResult CstiHelios::FileAndVersionOnFileGet (
	std::string *filePath,
	std::string *fileVersion)
{
	// Assumes there is only one firmware file in the OS static directory
	// Otherwise the first file found will be used to determine version

	stiHResult hResult = stiRESULT_SUCCESS;

	std::string firmwarePath;
	stiOSStaticDataFolderGet(&firmwarePath);

	filePath->clear ();
	fileVersion->clear ();

	DIR *d = opendir (firmwarePath.c_str ());
	struct dirent *dir = nullptr;
	if (d)
	{
		std::list<std::string> fileList;
		while ((dir = readdir (d)) != nullptr)
		{
			std::string fileName (dir->d_name);

			if (fileName.compare (0, 6, "helios") == 0)
			{
				auto versionPos = fileName.find_last_of ('_');
				auto extensionPos = fileName.find (".zip");
				if (versionPos == std::string::npos ||
					extensionPos == std::string::npos)
				{
					continue;
				}

				versionPos++;

				*filePath = firmwarePath + '/' + fileName;
				*fileVersion = fileName.substr (versionPos, extensionPos - versionPos);

				break;
			}
		}

		if (filePath->empty ())
		{
			stiTHROWMSG (stiRESULT_ERROR, "Helios firmware was not found in %s\n", firmwarePath.c_str ());
		}
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "Error opening directory %s\n", firmwarePath.c_str ());
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiHelios::CheckVersionForDfu (std::shared_ptr<SstiBluezDevice> device)
{
	std::string filePath;
	std::string fileVersion;
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("CstiHelios::CheckVersionForDfu\n");
	);
	
	// Get file and version of firmware on file
	hResult = FileAndVersionOnFileGet (&filePath, &fileVersion);
	stiTESTRESULT ();
		
	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("CstiHelios::CheckVersionForDfu: Helios version: %s, File version: %s\n", device->version.c_str (), fileVersion.c_str ());
	);

	// If not the same, start dfu
	if (device->version != fileVersion)
	{
		stiTrace ("CstiHelios::CheckVersionForDfu: Helios firmware is out of date.\nStarting dfu with file: %s\n", filePath.c_str ());
		
		hResult = m_dfu.initiate (device, filePath, m_bluetoothObject);
		if (hResult == stiRESULT_SUCCESS)
		{
			hResult = stiRESULT_DFU_IN_PROGRESS;
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("CstiHelios::CheckVersionForDfu: Helios firmware is up to date. No dfu is required.\n");
		);
	}

STI_BAIL:

	return hResult;
}

void CstiHelios::ProcessPotentialDfuList ()
{
	stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
			stiTrace ("ProcessPotentialDfuList\n");
	);

	while (!m_devicesToUpdate.empty ())
	{
		// CheckVersionForDfu starts a dfu if needed unless a dfu is already in progress
		stiHResult hResult = CheckVersionForDfu (m_devicesToUpdate.front ());
		if (hResult == stiRESULT_DFU_IN_PROGRESS)
		{
			// If the device needs updated or a dfu is in progress, break out of the loop.
			// The remaining devices will be checked after the dfu is complete
			break;
		}

		// The device firmware is up to date or CheckVersion failed.
		// Either way pop this device from the list and try the next one.
		m_devicesToUpdate.front ()->firmwareVerified = true;
		m_devicesToUpdate.pop_front ();
	}
}
