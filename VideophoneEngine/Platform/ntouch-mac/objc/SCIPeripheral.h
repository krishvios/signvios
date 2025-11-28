// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#import <CoreBluetooth/CoreBluetooth.h>
#import <functional>

NS_ASSUME_NONNULL_BEGIN

@interface SCIPeripheral : NSObject <CBPeripheralDelegate>

@property (retain) CBPeripheral *cbPeripheral;
@property (readonly) NSUUID *identifier;
@property (readonly) NSString *name;
@property (readonly) CBPeripheralState state;

-(id)initWithCBPeripheral: (CBPeripheral*)cbPeripheral;
-(void)dealloc;

-(void)discoverServices;

-(void)setNotifyValue: (bool)enabled forCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid;
-(void)readCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid;
-(void)writeCharacteristic: (NSString*)characteristicUuid ofService: (NSString*)serviceUuid withValue: (NSData*)data;

-(CBService*)getService: (NSString*)serviceUuid;
-(CBCharacteristic*)getCharacteristic: (NSString*)characteristicUuid fromService: (NSString*)serviceUuid;

-(void)setCallbacksForServiceAdded: (std::function<void(const std::string &serviceUuid)>) serviceAddedCallback
			   characteristicAdded: (std::function<void(const std::string &serviceUuid, const std::string &characUuid)>) characteristicAddedCallback
				   descriptorAdded: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)>) descriptorAddedCallback
	  characteristicStartNotifySet: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, bool enabled)>) characteristicStartNotifyCallback
		characteristicValueChanged: (std::function<void(const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)>) characteristicValueChangedCallback
			 characteristicWritten: (std::function<void(const std::string &serviceUuid, const std::string &characUuid)>) characteristicWrittenCallback;

@end

NS_ASSUME_NONNULL_END


