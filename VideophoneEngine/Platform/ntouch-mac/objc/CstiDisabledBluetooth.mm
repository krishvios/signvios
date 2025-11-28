// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#import <Foundation/Foundation.h>
#import "CstiDisabledBluetooth.h"
#import "SCICentral.h"
#include "PulseDevice.h"
#include "DfuDevice.h"
#include "stiTools.h"
#include "stiDebugTools.h"

CstiDisabledBluetooth::CstiDisabledBluetooth ()
{
	
}


CstiDisabledBluetooth::~CstiDisabledBluetooth ()
{

}


stiHResult CstiDisabledBluetooth::Enable (bool enable)
{
	stiTrace ("CstiDisabledBluetooth::Enable Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::Scan ()
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiDisabledBluetooth::ScanCancel ()
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiDisabledBluetooth::Pair (const BluetoothDevice&)
{
	stiTrace ("CstiDisabledBluetooth::Pair Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::PairCancel ()
{
	stiTrace ("CstiDisabledBluetooth::PairCancel Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::Connect (BluetoothDevice bluetoothDevice)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::PairedDeviceRemove (BluetoothDevice)
{
	stiTrace ("CstiDisabledBluetooth::PairedDeviceRemove Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::PairedListGet (std::list<BluetoothDevice> *deviceList)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::ScannedListGet (std::list<BluetoothDevice> *deviceList)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::PincodeSet (const char *pszPinCode)
{
	stiTrace ("CstiDisabledBluetooth::PincodeSet Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


bool CstiDisabledBluetooth::Powered ()
{	
	return false;
}


stiHResult CstiDisabledBluetooth::PairedDevicesLog ()
{
	stiTrace ("CstiDisabledBluetooth::PairedDevicesLog Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiDisabledBluetooth::DeviceRename(const BluetoothDevice& device, const std::string newName)
{
	stiTrace ("CstiDisabledBluetooth::DeviceRename Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiDisabledBluetooth::Disconnect(BluetoothDevice)
{
	stiTrace ("CstiDisabledBluetooth::Disconnect Not Implemented\n");
	
	return stiRESULT_SUCCESS;
}


//
// IstiLightRing implementation
//
void CstiDisabledBluetooth::PatternSet (Pattern patternId, EstiLedColor color, int brightness,	int flasherBrightness)
{

}

void CstiDisabledBluetooth::notificationPatternSet(NotificationPattern pattern)
{
	
}

void CstiDisabledBluetooth::notificationsStart (int nDuration, int nBrightness)
{
	
}

void CstiDisabledBluetooth::notificationsStop ()
{
	
}

void CstiDisabledBluetooth::Start (int nSeconds)
{
	
}

void CstiDisabledBluetooth::Stop ()
{
	
}

ISignal<>& CstiDisabledBluetooth::lightRingStoppedSignalGet ()
{
	return lightRingStoppedSignal;
}

ISignal<>& CstiDisabledBluetooth::notificationStoppedSignalGet ()
{
	return notificationStoppedSignal;
}


//
// IstiStatusLED implementation
//
stiHResult CstiDisabledBluetooth::Start (EstiLed eLED, unsigned unBlinkRate)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::Stop (EstiLed eLED)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::Enable (EstiLed eLED, EstiBool bEnable)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiDisabledBluetooth::PulseNotificationsEnable (bool enable)
{
	return stiRESULT_SUCCESS;
}


//
// Pulse methods
//
void CstiDisabledBluetooth::pulsePresetRingPatternStart (
												 Pattern pattern,
												 EstiLedColor color,
												 uint8_t brightness)
{

}

void CstiDisabledBluetooth::pulseCustomRingPatternStart (const std::vector<std::string> &frames)
{
	
}

void CstiDisabledBluetooth::pulseRingStop ()
{
	
}

void CstiDisabledBluetooth::pulseMissedSet (bool value)
{

}

void CstiDisabledBluetooth::pulseSignmailSet (bool value)
{

}

void CstiDisabledBluetooth::pulseEnableNotificationsSet (bool value)
{
	
}

void CstiDisabledBluetooth::scanFilterSet (const std::vector<std::string> &uuids)
{

}

void CstiDisabledBluetooth::scanForSeconds (double seconds)
{
	
}

void CstiDisabledBluetooth::deviceDiscoveredCallback (std::unique_ptr<BleIODevice> device)
{
	
}


void CstiDisabledBluetooth::deviceConnectedCallback (const std::string &identifier)
{
	
}

void CstiDisabledBluetooth::deviceDisconnectedCallback (const std::string &identifier)
{
	
}

void CstiDisabledBluetooth::deviceRemovedCallback (const std::string &identifier)
{
	
}

/*
 *	Pulse Functions
 */
void CstiDisabledBluetooth::pulseVerifyFirmware (BleDevice *device)
{
	
}
