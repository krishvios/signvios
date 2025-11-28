// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#import <Foundation/Foundation.h>
#import "SCIPeripheral.h"
#include "stiTrace.h"
#include <string>
#include <vector>

@interface SCIPeripheral ()
{
	std::function<void(const std::string &serviceUuid)> m_serviceAddedCallback;
	std::function<void(const std::string &serviceUuid, const std::string &characUuid)> m_characteristicAddedCallback;
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)> m_descriptorAddedCallback;
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, bool enabled)> m_characteristicStartNotifyCallback;
	std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)> m_characteristicValueChangedCallback;
	std::function<void(const std::string &serviceUuid, const std::string &characUuid)> m_characteristicWrittenCallback;
}

@end

@implementation SCIPeripheral

-(id)initWithCBPeripheral: (CBPeripheral*)cbPeripheral
{
	self = [super init];
	if (self)
	{
		self.cbPeripheral = cbPeripheral;
		self.cbPeripheral.delegate = self;
	}
	return self;
}

-(void)dealloc
{
}

-(NSUUID*)identifier
{
	if (@available(macOS 10.13, iOS 7.0, *)) {
		return self.cbPeripheral.identifier;
	} else {
		return [self.cbPeripheral valueForKey:@"identifier"];
	}
}

-(NSString*)name
{
	return self.cbPeripheral.name;
}

-(CBPeripheralState)state
{
	return self.cbPeripheral.state;
}

-(void)discoverServices
{
	[self.cbPeripheral discoverServices:nil];
}

-(void)setNotifyValue: (bool)enabled forCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid
{
	CBCharacteristic *characteristic = [self getCharacteristic:characteristicUuid fromService:serviceUuid];
	if (characteristic)
	{
		[self.cbPeripheral setNotifyValue:enabled forCharacteristic:characteristic];
	}
}

-(void)readCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid
{
	CBCharacteristic *characteristic = [self getCharacteristic:characteristicUuid fromService:serviceUuid];
	if (characteristic)
	{
		[self.cbPeripheral readValueForCharacteristic:characteristic];
	}
}

-(void)writeCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid withValue: (NSData*)data
{
	CBCharacteristic *characteristic = [self getCharacteristic:characteristicUuid fromService:serviceUuid];
	if (characteristic)
	{
		[self.cbPeripheral writeValue:data forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
	}
}

-(CBService*)getService: (NSString*)serviceUuid
{
	for (CBService *service in self.cbPeripheral.services)
	{
		if ([serviceUuid isEqualToString:service.UUID.UUIDString])
		{
			return service;
		}
	}
	return nullptr;
}

-(CBCharacteristic*)getCharacteristic: (NSString*)characteristicUuid fromService: (NSString*)serviceUuid
{
	CBService *service = [self getService:serviceUuid];
	if (service)
	{
		for (CBCharacteristic* characteristic in [service characteristics])
		{
			if ([characteristicUuid isEqualToString:characteristic.UUID.UUIDString])
			{
				return characteristic;
			}
		}
	}
	return nullptr;
}

-(void)setCallbacksForServiceAdded: (std::function<void(const std::string &serviceUuid)>) serviceAddedCallback
			   characteristicAdded: (std::function<void(const std::string &serviceUuid, const std::string &characUuid)>) characteristicAddedCallback
				   descriptorAdded: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)>) descriptorAddedCallback
	  characteristicStartNotifySet: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, bool enabled)>) characteristicStartNotifyCallback
		characteristicValueChanged: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)>) characteristicValueChangedCallback
			 characteristicWritten: (std::function<void(const std::string &serviceUuid, const std::string &characUuid)>) characteristicWrittenCallback
{
	m_serviceAddedCallback = serviceAddedCallback;
	m_characteristicAddedCallback = characteristicAddedCallback;
	m_descriptorAddedCallback = descriptorAddedCallback;
	m_characteristicStartNotifyCallback = characteristicStartNotifyCallback;
	m_characteristicValueChangedCallback = characteristicValueChangedCallback;
	m_characteristicWrittenCallback = characteristicWrittenCallback;
}

// CBPeripheralDelegate methods
// Invoked when services are discovered on a device
-(void)peripheral:(CBPeripheral *)peripheral
didDiscoverServices:(NSError *)error
{
	for (CBService *service in peripheral.services)
	{
//		stiTrace ("Discovered service %s for %s\n", [[service.UUID UUIDString] UTF8String], [peripheral.name UTF8String]);
		
		if (m_serviceAddedCallback)
		{
			m_serviceAddedCallback ([service.UUID.UUIDString UTF8String]);
		}
		
		[peripheral discoverCharacteristics:nil forService:service];
	}
}

// Invoked when characteristics are discovered for a service
-(void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
			error:(NSError *)error
{
	for (CBCharacteristic *characteristic in service.characteristics)
	{
//		stiTrace ("Discovered characteristic %s for %s\n", [[characteristic.UUID UUIDString] UTF8String], [peripheral.name UTF8String]);
		
		if (m_characteristicAddedCallback)
		{
			m_characteristicAddedCallback ([service.UUID.UUIDString UTF8String],
										   [characteristic.UUID.UUIDString UTF8String]);
		}
		
		[peripheral discoverDescriptorsForCharacteristic:characteristic];
	}
}

// Invoked when descriptors are discovered for a characteristic
-(void)peripheral:(CBPeripheral *)peripheral
didDiscoverDescriptorsForCharacteristic:(CBCharacteristic *)characteristic
			error:(NSError *)error
{	
	for (CBDescriptor *descriptor in characteristic.descriptors)
	{
//		stiTrace ("Discovered descriptor %s for %s\n", [[descriptor.UUID UUIDString] UTF8String], [peripheral.name UTF8String]);
		
		if (m_descriptorAddedCallback)
		{
			m_descriptorAddedCallback ([characteristic.service.UUID.UUIDString UTF8String],
									   [characteristic.UUID.UUIDString UTF8String],
									   [descriptor.UUID.UUIDString UTF8String]);
		}
	}
}

// Invoked when the notification state of a characteristic has changed
-(void)peripheral:(CBPeripheral *)peripheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
			error:(NSError *)error
{
	if (m_characteristicStartNotifyCallback)
	{
		m_characteristicStartNotifyCallback ([characteristic.service.UUID.UUIDString UTF8String],
											 [characteristic.UUID.UUIDString UTF8String],
											 characteristic.isNotifying);
	}
}

// Invoked when the value of a characteristic changes
-(void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
			error:(NSError *)error
{
	NSData *data = [characteristic value];
	std::vector<uint8_t> value;
	value.reserve([data length]);
	const uint8_t *bytes = (uint8_t*)[data bytes];
	for (auto i = 0; i <[data length]; i++)
	{
		value.push_back (bytes[i]);
	}
	
	if (m_characteristicValueChangedCallback)
	{
		m_characteristicValueChangedCallback ([characteristic.service.UUID.UUIDString UTF8String],
											  [characteristic.UUID.UUIDString UTF8String],
											  value);
	}
}

// Invoked when a characteristic value is written
-(void)peripheral:(CBPeripheral *)peripheral
didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
			error:(NSError *)error
{
	if (m_characteristicWrittenCallback)
	{
		m_characteristicWrittenCallback ([characteristic.service.UUID.UUIDString UTF8String],
										 [characteristic.UUID.UUIDString UTF8String]);
	}
}



// Invoked when the value of a descriptor changes
-(void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForDescriptor:(CBDescriptor *)descriptor
			error:(NSError *)error
{
	stiTrace ("Descriptor value changed, Not Implemented\n");
}



// Invoked when a descriptor value is written
-(void)peripheral:(CBPeripheral *)peripheral
didWriteValueForDescriptor:(CBDescriptor *)descriptor
			error:(NSError *)error
{
	stiTrace ("Wrote value to descriptor, Not Implemented\n");
}

@end
