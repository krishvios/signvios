// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#import <Foundation/Foundation.h>
#import "CstiBluetooth.h"
#import "SCICentral.h"
#include "PulseDevice.h"
#include "DfuDevice.h"
#include "stiTools.h"
#include "stiDebugTools.h"

#define DEFAULT_SCAN_SECONDS 5.0
#define UPDATE_PAIRED_DEVICE_TIMEOUT 3000 // 3 seconds

struct CentralImpl
{
	SCICentral *wrappedCentral = nullptr;
};


CstiBluetooth::CstiBluetooth ()
:
CstiEventQueue ("CstiBluetooth"),
m_scanTimer (10000, this),
m_updatePairedDevicesTimer (UPDATE_PAIRED_DEVICE_TIMEOUT, this),
m_impl (new CentralImpl)
{
	m_signalConnections.push_back (m_scanTimer.timeoutSignal.Connect (
		[this]{
			scanCancelEvent ();
		}));
	
	m_signalConnections.push_back (m_updatePairedDevicesTimer.timeoutSignal.Connect (
		[this]{
			updatePairedDevices ();
			m_updatePairedDevicesTimer.restart ();
		}));
	
	CstiEventQueue::StartEventLoop ();
	PostEvent ([this]{initializeEvent ();});
}


CstiBluetooth::~CstiBluetooth ()
{
	// If connecting to Pulse devices with the OS controller, it is necessary
	// to shut off the ringer, missed LED, and signmail LED manually. It is done here
	// because the asynchronous implementation of bluetooth writing does not allow it
	// in the PulseDevice destructor.
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet () && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get();
			pulseDevice->allOff ();
		}
	}
	
	CstiEventQueue::StopEventLoop ();

	if (m_impl)
	{
		[m_impl->wrappedCentral release];
		delete m_impl;
	}
}


void CstiBluetooth::initializeEvent ()
{
	m_impl->wrappedCentral = [SCICentral sharedInstance];
	setCallbacks ();

	// If pairing through the OS, this recurring timer checks
	// for added devices while the app is running.
//	m_updatePairedDevicesTimer.restart ();
}


stiHResult CstiBluetooth::Enable (bool enable)
{
	stiTrace ("CstiBluetooth::Enable Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::Scan ()
{
//	PostEvent ([this]{scanEvent ();});
	PostEvent ([this]{scanForSecondsEvent (DEFAULT_SCAN_SECONDS);});
	
	return stiRESULT_SUCCESS;
}


void CstiBluetooth::scanEvent ()
{
		m_deviceList.erase (std::remove_if (m_deviceList.begin (), m_deviceList.end (),
											[](const std::unique_ptr<BleDevice> &device){
												return device->pairedGet () == false;
											}),
							m_deviceList.end ());
	
		[m_impl->wrappedCentral scan];
}


stiHResult CstiBluetooth::ScanCancel ()
{
	PostEvent ([this]{
		m_scanTimer.stop ();
		scanCancelEvent ();
	});
	
	return stiRESULT_SUCCESS;
}


void CstiBluetooth::scanCancelEvent ()
{
	[m_impl->wrappedCentral stopScan];
}


stiHResult CstiBluetooth::Pair (const BluetoothDevice&)
{
	stiTrace ("CstiBluetooth::Pair Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::PairCancel ()
{
	stiTrace ("CstiBluetooth::PairCancel Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::Connect (BluetoothDevice bluetoothDevice)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	auto deviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
									[bluetoothDevice](const std::unique_ptr<BleDevice> &device){
										return bluetoothDevice.address == device->idGet();
									});
	
	if (deviceIter != m_deviceList.end ())
	{
		[m_impl->wrappedCentral connectDevice:*((*deviceIter)->bleIODeviceGet())];
	}
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::PairedDeviceRemove (BluetoothDevice)
{
	stiTrace ("CstiBluetooth::PairedDeviceRemove Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::PairedListGet (std::list<BluetoothDevice> *deviceList)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	deviceList->clear();
	
	for (auto &device : m_deviceList)
	{
		if (device->pairedGet() && device->isPulseDevice())
		{
			BluetoothDevice bluetoothDevice;
			bluetoothDevice.name = device->nameGet ();
			bluetoothDevice.address = device->idGet ();
			bluetoothDevice.connected = device->connectedGet ();
			bluetoothDevice.paired = true;
			
			deviceList->push_back(bluetoothDevice);
		}
	}
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::ScannedListGet (std::list<BluetoothDevice> *deviceList)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	deviceList->clear();
	
	for (auto &device : m_deviceList)
	{
		if (!device->pairedGet() && device->isPulseDevice())
		{
			BluetoothDevice bluetoothDevice;
			bluetoothDevice.name = device->nameGet ();
			bluetoothDevice.address = device->idGet ();
			bluetoothDevice.connected = device->connectedGet ();
			bluetoothDevice.paired = false;
			
			deviceList->push_back(bluetoothDevice);
		}
	}
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::PincodeSet (const char *pszPinCode)
{
	stiTrace ("CstiBluetooth::PincodeSet Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


bool CstiBluetooth::Powered ()
{
	stiTrace ("CstiBluetooth::Powered Not Implemented\n");
	
	return true;
}


stiHResult CstiBluetooth::PairedDevicesLog ()
{
	stiTrace ("CstiBluetooth::PairedDevicesLog Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiBluetooth::DeviceRename(const BluetoothDevice& device, const std::string newName)
{
	stiTrace ("CstiBluetooth::DeviceRename Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiBluetooth:: Disconnect(BluetoothDevice)
{
	stiTrace ("CstiBluetooth::Disconnect Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


//
// IstiLightRing implementation
//
void CstiBluetooth::PatternSet (Pattern patternId, EstiLedColor color, int brightness,	int flasherBrightness)
{
	PostEvent ([this, patternId, color, brightness](){
		m_pattern = patternId;
		m_color = color;
		m_brightness = brightness;
	});
}

void CstiBluetooth::notificationPatternSet(NotificationPattern pattern)
{
	
}

void CstiBluetooth::notificationsStart (int nDuration, int nBrightness)
{
	
}

void CstiBluetooth::notificationsStop ()
{
	
}

void CstiBluetooth::Start (int nSeconds)
{
	PostEvent ([this]{startEvent ();});
}


void CstiBluetooth::startEvent ()
{
	pulsePresetRingPatternStartEvent (m_pattern, m_color, m_brightness);
}


void CstiBluetooth::Stop ()
{
	PostEvent ([this]{stopEvent ();});
}

ISignal<>& CstiBluetooth::lightRingStoppedSignalGet ()
{
	return lightRingStoppedSignal;
}

ISignal<>& CstiBluetooth::notificationStoppedSignalGet ()
{
	return notificationStoppedSignal;
}

void CstiBluetooth::stopEvent ()
{
	pulseRingStop ();
}


//
// IstiStatusLED implementation
//
stiHResult CstiBluetooth::Start (EstiLed eLED, unsigned unBlinkRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (eLED)
	{
		case IstiStatusLED::estiLED_SIGNMAIL:
			PostEvent ([this]{pulseSignmailSet (true);});
			break;
			
		case IstiStatusLED::estiLED_MISSED_CALL:
			PostEvent ([this]{pulseMissedSet (true);});
			break;
			
		default:
			stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "Status LED (%i) not implemented in CstiBluetooth", eLED);
	}
	
STI_BAIL:
	return hResult;
}


stiHResult CstiBluetooth::Stop (EstiLed eLED)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (eLED)
	{
		case IstiStatusLED::estiLED_SIGNMAIL:
			PostEvent ([this]{pulseSignmailSet (false);});
			break;
			
		case IstiStatusLED::estiLED_MISSED_CALL:
			PostEvent ([this]{pulseMissedSet (false);});
			break;
			
		default:
			stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "Status LED (%i) not implemented in CstiBluetooth", eLED);
	}
	
STI_BAIL:
	return hResult;
}


stiHResult CstiBluetooth::Enable (EstiLed eLED, EstiBool bEnable)
{
	bool value = bEnable == estiTRUE;
	stiHResult hResult = stiRESULT_SUCCESS;
	StatusLed* led = nullptr;
	
	switch (eLED)
	{
		case IstiStatusLED::estiLED_SIGNMAIL:
			led = &m_signMailLed;
			break;
			
		case IstiStatusLED::estiLED_MISSED_CALL:
			led = &m_missedLed;
			break;
			
		default:
			stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "Status LED (%i) not implemented in CstiBluetooth", eLED);
	}
	
	if (led)
	{
		PostEvent ([this, led, value]() {
			led->m_enabled = value;
			pulseMissedSet (m_missedLed.m_state);
			pulseSignmailSet (m_signMailLed.m_state);
		});
	}
	
STI_BAIL:
	return hResult;
}


stiHResult CstiBluetooth::PulseNotificationsEnable (bool enable)
{
	PostEvent ([this, enable]{pulseEnableNotificationsSetEvent (enable);});
	return stiRESULT_SUCCESS;
}


//
// Pulse methods
//
void CstiBluetooth::pulsePresetRingPatternStart (
	Pattern pattern,
	EstiLedColor color,
	uint8_t brightness)
{
	PostEvent ([this,pattern,color,brightness]{pulsePresetRingPatternStartEvent (pattern, color, brightness);});
}


void CstiBluetooth::pulsePresetRingPatternStartEvent (
	Pattern pattern,
	EstiLedColor color,
	uint8_t brightness)
{
	if (!m_patternsAndColorsAllowed)
	{
		pattern = DefaultPattern;
		color = DefaultColor;
		brightness = DefaultBrightness;
	}
	
	uint8_t red = 0xFF, green = 0xFF, blue = 0xFF;
	auto iter = m_ledColorMap.find (color);
	if (iter != m_ledColorMap.end ())
	{
		red = iter->second.Red;
		green = iter->second.Green;
		blue = iter->second.Blue;
	}
	
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet() && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get ();
			if (pulseDevice->isAuthenticated ())
			{
				pulseDevice->presetRingPatternStart ((int)pattern, red, green, blue, brightness);
			}
		}
	}
}


void CstiBluetooth::pulseCustomRingPatternStart (const std::vector<std::string> &frames)
{
	PostEvent ([this,frames]{pulseCustomRingPatternStartEvent (frames);});
}


void CstiBluetooth::pulseCustomRingPatternStartEvent (const std::vector<std::string> &frames)
{
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet() && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get ();
			if (pulseDevice->isAuthenticated ())
			{
				pulseDevice->customRingPatternStart (frames);
			}
		}
	}
}


void CstiBluetooth::pulseRingStop ()
{
	PostEvent ([this]{pulseRingStopEvent ();});
}


void CstiBluetooth::pulseRingStopEvent ()
{
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet() && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get ();
			if (pulseDevice->isAuthenticated ())
			{
				pulseDevice->ringStop ();
			}
		}
	}
}


void CstiBluetooth::pulseMissedSet (bool value)
{
	PostEvent ([this,value]{
		m_pulseState.missedSet = value;
		if (m_pulseState.notificationsEnabled)
		{
			pulseMissedSetEvent ();
		}
	});
}


void CstiBluetooth::pulseMissedSetEvent ()
{
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet() && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get ();
			if (pulseDevice->isAuthenticated ())
			{
				if (m_pulseState.notificationsEnabled)
				{
					pulseDevice->missedSet (m_pulseState.missedSet);
				}
				else
				{
					pulseDevice->missedSet (false);
				}
			}
		}
	}
}


void CstiBluetooth::pulseSignmailSet (bool value)
{
	PostEvent ([this,value]{
		m_pulseState.signmailSet = value;
		if (m_pulseState.notificationsEnabled)
		{
			pulseSignmailSetEvent ();
		}
	});
}


void CstiBluetooth::pulseSignmailSetEvent ()
{
	for (auto &device : m_deviceList)
	{
		if (device->connectedGet() && device->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)device.get ();
			if (pulseDevice->isAuthenticated ())
			{
				if (m_pulseState.notificationsEnabled)
				{
					pulseDevice->signmailSet (m_pulseState.signmailSet);
				}
				else
				{
					pulseDevice->signmailSet (false);
				}
			}
		}
	}
}

void CstiBluetooth::pulseEnableNotificationsSet (bool value)
{
	PostEvent ([this,value]{pulseEnableNotificationsSetEvent (value);});
}

void CstiBluetooth::pulseEnableNotificationsSetEvent (bool value)
{
	m_pulseState.notificationsEnabled = value;
	
	pulseMissedSetEvent ();
	pulseSignmailSetEvent ();
}


void CstiBluetooth::scanFilterSet (const std::vector<std::string> &uuids)
{
	PostEvent ([this,uuids]{scanFilterSetEvent (uuids);});
}


void CstiBluetooth::scanFilterSetEvent (const std::vector<std::string> &uuids)
{
	NSMutableArray *filterUuids = [NSMutableArray arrayWithCapacity:uuids.size()];
	for (auto &uuid : uuids)
	{
		NSString *uuidString = [NSString stringWithUTF8String:uuid.c_str()];
		CBUUID *cbUuid = [CBUUID UUIDWithString:uuidString];
		[filterUuids addObject:cbUuid];
	}
	[m_impl->wrappedCentral setScanFilter:filterUuids];
}


void CstiBluetooth::scanForSeconds (double seconds)
{
	PostEvent ([this,seconds]{scanForSecondsEvent (seconds);});
}


void CstiBluetooth::scanForSecondsEvent (double seconds)
{
	m_deviceList.erase (std::remove_if (m_deviceList.begin (), m_deviceList.end (),
										[](const std::unique_ptr<BleDevice> &device){
											return device->pairedGet () == false;
										}),
						m_deviceList.end ());

	[m_impl->wrappedCentral scan];
	m_scanTimer.timeoutSet((int)(seconds*1000));
	m_scanTimer.restart ();
}


void CstiBluetooth::connectDevice (BleIODevice &device)
{
	[m_impl->wrappedCentral connectDevice:device];
}


void CstiBluetooth::disconnectDevice (BleIODevice &device)
{
	[m_impl->wrappedCentral disconnectDevice:device];
}


void CstiBluetooth::deviceDiscoveredCallback (std::unique_ptr<BleIODevice> device)
{
	// Using event queue mutex for now because of difficulty posting event with unique_ptr
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	std::string name (device->nameGet ());
	std::vector<std::string> advertisedServices (device->advertisedServicesGet ());
	
	auto uuidIter = std::find_if (advertisedServices.begin (), advertisedServices.end (),
								  [](const std::string &uuid){return uuid == PULSE_SERVICE;});
	
	if (uuidIter != advertisedServices.end ())
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("CstiBluetooth: Adding Pulse Device %s\n", name.c_str ());
		);

		std::function<void(BleDevice*)> callback = [this](BleDevice *bleDevice){pulseVerifyFirmware (bleDevice);};
		
		BluetoothDevice bluetoothDevice;
		bluetoothDevice.name = device->nameGet();
		bluetoothDevice.address = device->idGet();
		
		m_deviceList.push_back (sci::make_unique<PulseDevice> (std::move (device), callback, this));
		
		deviceAddedSignalGet ().Emit (bluetoothDevice);
	}
	else if (!name.empty() && !m_dfuDevicesWaitingOn.empty())
	{
		auto deviceNameIter = std::find_if (m_dfuDevicesWaitingOn.begin (), m_dfuDevicesWaitingOn.end (),
									  [name](const std::string &nameInList){return nameInList == name;});
		
		if (deviceNameIter != m_dfuDevicesWaitingOn.end ())
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("CstiBluetooth: Adding DFU Device %s\n", name.c_str ());
			);
			
			m_dfuDevicesWaitingOn.erase (deviceNameIter);
			
			if (m_dfuDevicesWaitingOn.empty ())
			{
				[m_impl->wrappedCentral stopScan];
			}
			
			auto& deviceToConnect = *(device.get ());
			
			m_deviceList.push_back (sci::make_unique<DfuDevice> (std::move (device), this)); // Pass firmware???
			
			connectDevice (deviceToConnect);
		}
		else
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
				stiTrace ("CstiBluetooth: Non Pulse device discovered while looking for DFU device\n");
			);
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("CstiBluetooth: Attempting to add unsupported device!!!\n");
		);
	}
}


void CstiBluetooth::deviceConnectedCallback (const std::string &identifier)
{
	PostEvent ([this,identifier]{deviceConnectedCallbackEvent (identifier);});
}


void CstiBluetooth::deviceConnectedCallbackEvent (const std::string &identifier)
{
	auto deviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
									[identifier](const std::unique_ptr<BleDevice> &device){return device->idGet () == identifier;});
	
	if (deviceIter != m_deviceList.end ())
	{
		// At least for now, because we aren't notified when a device is removed, a Pulse device
		// must be reset upon connecting because it may have been connected after being removed.
		if ((*deviceIter)->isPulseDevice ())
		{
			auto pulseDevice = (PulseDevice*)((*deviceIter).get());
			pulseDevice->reset ();
		}
		
		(*deviceIter)->pairedSet (true);
		(*deviceIter)->connectedSet (true);
		(*deviceIter)->servicesDiscover ();
		
		// Emit signal that device was connected
		BluetoothDevice bluetoothDevice;
		deviceConnectedSignalGet ().Emit (bluetoothDevice);
	}
}


void CstiBluetooth::deviceDisconnectedCallback (const std::string &identifier)
{
	PostEvent ([this,identifier]{deviceDisconnectedCallbackEvent (identifier);});
}


void CstiBluetooth::deviceDisconnectedCallbackEvent (const std::string &identifier)
{
	auto deviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
									[identifier](const std::unique_ptr<BleDevice> &device){return device->idGet () == identifier;});
	
	if (deviceIter != m_deviceList.end ())
	{
#if 0
		// For this, this UI would need to differentiate between paired and connected
		(*deviceIter)->connectedSet (false);
#else
		m_deviceList.erase (deviceIter);
#endif
		BluetoothDevice bluetoothDevice;
		deviceDisconnectedSignalGet ().Emit (bluetoothDevice);
	}
}


void CstiBluetooth::deviceRemovedCallback (const std::string &identifier)
{
	PostEvent ([this,identifier]{deviceRemovedCallbackEvent (identifier);});
}


void CstiBluetooth::deviceRemovedCallbackEvent (const std::string &identifier)
{
	auto deviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
									[identifier](const std::unique_ptr<BleDevice> &device){return device->idGet () == identifier;});
	
	if (deviceIter != m_deviceList.end ())
	{
		m_deviceList.erase (deviceIter);
	}
}


/*
 *	Pulse Functions
 */
void CstiBluetooth::pulseVerifyFirmware (BleDevice *device)
{
	if (device->connectedGet ())
	{
		auto pulseDevice = (PulseDevice*)device;
		
		auto versionOnDevice = pulseDevice->firmwareVersionGet ();
		
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("CstiBluetooth: Pulse device %s has firmware version %s\n", device->nameGet ().c_str (), versionOnDevice.c_str ());
		);
		
#if 0 // The following is required when DFU is implemented
		// Get latest version of firmware on file
		std::string firmwareVersion ("1.0.0");
		
		if (versionOnDevice != firmwareVersion)
		{
			auto serialNumber = pulseDevice->serialNumberGet ();
			std::string bootloaderName = "DFU_" + serialNumber;
			
			m_dfuDevicesWaitingOn.push_back (bootloaderName);
			
			pulseDevice->restartInBootloader (bootloaderName);
			
			scanForSeconds (10.0);
		}
		else
#endif
		{
			// For newly connected device, set missed and signmail leds
			// Currently not saving ring state
			if (m_pulseState.notificationsEnabled)
			{
				pulseDevice->missedSet (m_pulseState.missedSet);
				pulseDevice->signmailSet (m_pulseState.signmailSet);
			}
		}
	}
}


void CstiBluetooth::setCallbacks ()
{
	[m_impl->wrappedCentral setCallbacksForDeviceDiscovered:[this](std::unique_ptr<BleIODevice> device){deviceDiscoveredCallback(std::move(device));}
											deviceConnected:[this](const std::string &identifier){deviceConnectedCallback(identifier);}
										 deviceDisconnected:[this](const std::string &identifier){deviceDisconnectedCallback(identifier);}
											  deviceRemoved:[this](const std::string &identifier){deviceRemovedCallback(identifier);}];
}

void CstiBluetooth::updatePairedDevices ()
{
	NSMutableArray *deviceIDs = [[NSMutableArray alloc] init];

	for (auto &device : m_deviceList)
	{
		// Don't add DFU devices to the paired device list.
		// They will be errantly removed
		std::string deviceName = device->nameGet ();
		if (device->pairedGet () && deviceName.compare (0,4,"DFU_") != 0)
		{
			[deviceIDs addObject:[[NSUUID UUID] initWithUUIDString:[NSString stringWithUTF8String:device->idGet ().c_str ()]]];
		}
	}
	
	[m_impl->wrappedCentral updatePairedDevices:deviceIDs];
}

