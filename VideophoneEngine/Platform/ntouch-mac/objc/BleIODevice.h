// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <deque>
#include <map>

static const std::string PULSE_SERVICE				 			= "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const std::string PULSE_WRITE_CHARACTERISTIC 			= "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const std::string PULSE_READ_CHARACTERISTIC 				= "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
static const std::string PULSE_AUTHENTICATE_CHARACTERISTIC 		= "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";

static const std::string PULSE_DFU_SERVICE						= "FE59";
static const std::string PULSE_DFU_CONTROL_POINT_CHARACTERISTIC = "8EC90001-F315-4F60-9FB8-838830DAEA50";
static const std::string PULSE_DFU_PACKET_CHARACTERISTIC 		= "8EC90002-F315-4F60-9FB8-838830DAEA50";
static const std::string PULSE_DFU_BUTTONLESS_CHARACTERISTIC	= "8EC90003-F315-4F60-9FB8-838830DAEA50";

struct BleIODeviceImpl;

class BleIODevice
{
public:
	BleIODevice (void *device, const std::vector<std::string> &advertisedServiceUuids);
	virtual ~BleIODevice ();
	
	BleIODevice (const BleIODevice &) = delete;
	BleIODevice (BleIODevice &&) = default;
	BleIODevice& operator= (const BleIODevice &) = delete;
	BleIODevice& operator= (BleIODevice &&) = default;
	
	void pairedSet (bool value) {m_paired = value;};
	bool pairedGet () {return m_paired;};
	
	void connectedSet (bool value) {m_connected = value;};
	bool connectedGet () {return m_connected;};
	
	std::string idGet ();
	std::string nameGet () ;
	std::vector<std::string> advertisedServicesGet ();
	
	void servicesDiscover ();
	
	void characteristicNotifySet (const std::string &serviceUuid, const std::string &characUuid, bool value);
	void characteristicRead (const std::string &serviceUuid, const std::string &characUuid);
	void characteristicWrite (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &data);
	
	void callbacksSet (
		std::function<void(const std::string &serviceUuid)> serviceAddedCallback,
		std::function<void(const std::string &serviceUuid, const std::string &characUuid)> characteristicAddedCallback,
		std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)> descriptorAddedCallback,
		std::function<void(const std::string &serviceUuid, const std::string &characUuid, bool enabled)> characteristicStartNotifyCallback,
		std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)> characteristicValueChangedCallback,
		std::function<void(const std::string &serviceUuid, const std::string &characUuid)> characteristicWrittenCallback);
	
	// Additional Methods
	void* objcPeripheralGet () const;
	
private:
	
	bool m_paired = false;
	bool m_connected = false;
	std::vector<std::string> m_advertisedServiceUuids;
	
	BleIODeviceImpl *m_impl = nullptr;
};

