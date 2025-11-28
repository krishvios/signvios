// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#import "BleIODevice.h"
#import "SCIPeripheral.h"

struct BleIODeviceImpl
{
	SCIPeripheral *wrappedPeripheral = nullptr;
};

BleIODevice::BleIODevice (void *device, const std::vector<std::string> &advertisedServiceUuids)
:
m_impl (new BleIODeviceImpl),
m_advertisedServiceUuids (advertisedServiceUuids)
{
	m_impl->wrappedPeripheral = [[SCIPeripheral alloc] initWithCBPeripheral:(CBPeripheral*)device];
}

BleIODevice::~BleIODevice ()
{
	if (m_impl)
	{
		[m_impl->wrappedPeripheral release];
		delete m_impl;
	}
}

std::string BleIODevice::idGet ()
{
	return [[m_impl->wrappedPeripheral.identifier UUIDString] UTF8String];
}

std::string BleIODevice::nameGet ()
{
	NSString *name = m_impl->wrappedPeripheral.name;
	if ([name length] > 0)
	{
		return [name UTF8String];
	}
	else
	{
		return std::string ();
	}
}

std::vector<std::string> BleIODevice::advertisedServicesGet ()
{
	return m_advertisedServiceUuids;
}

void BleIODevice::servicesDiscover ()
{
	[m_impl->wrappedPeripheral discoverServices];
}

void BleIODevice::characteristicNotifySet (const std::string &serviceUuid, const std::string &characUuid, bool value)
{
	NSString *serviceUuidString = [NSString stringWithUTF8String:serviceUuid.c_str ()];
	NSString *characUuidString = [NSString stringWithUTF8String:characUuid.c_str ()];
	[m_impl->wrappedPeripheral setNotifyValue:value forCharacteristic:characUuidString ofService:serviceUuidString];
}

void BleIODevice::characteristicRead (const std::string &serviceUuid, const std::string &characUuid)
{
	NSString *serviceUuidString = [NSString stringWithUTF8String:serviceUuid.c_str ()];
	NSString *characUuidString = [NSString stringWithUTF8String:characUuid.c_str ()];
	[m_impl->wrappedPeripheral readCharacteristic:characUuidString ofService:serviceUuidString];
}

void BleIODevice::characteristicWrite (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &data)
{
	NSString *serviceUuidString = [NSString stringWithUTF8String:serviceUuid.c_str ()];
	NSString *characUuidString = [NSString stringWithUTF8String:characUuid.c_str ()];
	NSData *writeData = [NSData dataWithBytes:data.data() length:data.size()];
	
	if (data.size())
	{
		[m_impl->wrappedPeripheral writeCharacteristic:characUuidString ofService:serviceUuidString withValue:writeData];
	}
}

void BleIODevice::callbacksSet (
	std::function<void(const std::string &serviceUuid)> serviceAddedCallback,
	std::function<void(const std::string &serviceUuid, const std::string &characUuid)> characteristicAddedCallback,
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)> descriptorAddedCallback,
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, bool enabled)> characteristicStartNotifyCallback,
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)> characteristicValueChangedCallback,
	std::function<void(const std::string &serviceUuid, const std::string &characUuid)> characteristicWrittenCallback)
{
	[m_impl->wrappedPeripheral setCallbacksForServiceAdded:serviceAddedCallback
									   characteristicAdded:characteristicAddedCallback
										   descriptorAdded:descriptorAddedCallback
							  characteristicStartNotifySet:characteristicStartNotifyCallback
								characteristicValueChanged:characteristicValueChangedCallback
									 characteristicWritten:characteristicWrittenCallback];
}

void* BleIODevice::objcPeripheralGet () const
{
	return m_impl->wrappedPeripheral;
}
