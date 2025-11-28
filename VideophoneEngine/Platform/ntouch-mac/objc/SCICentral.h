// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#import <CoreBluetooth/CoreBluetooth.h>
#import "BleDevice.h"
#import "BleIODevice.h"
#import <functional>
#import <string>

NS_ASSUME_NONNULL_BEGIN

@interface SCICentral : NSObject <CBCentralManagerDelegate>
{
	CBCentralManager *centralManager;
	NSMutableArray<CBUUID*> *supportedUuids;
	NSArray<CBPeripheral*> *pairedDevices;
	NSTimer *scanTimer;
}

@property (class, readonly) SCICentral *sharedInstance;
@property () BOOL bluetoothEnabled;
@property (nonatomic) BOOL isScanning;
@property (nonatomic, assign) CBManagerState managerState API_UNAVAILABLE(macos);


+(SCICentral*)sharedInstance;
-(id)init;
-(void)dealloc;
-(void)setCallbacksForDeviceDiscovered: (std::function<void(std::unique_ptr<BleIODevice>)>)deviceDiscoveredCallback
					   deviceConnected: (std::function<void(const std::string&)>)deviceConnectedCallback
					deviceDisconnected: (std::function<void(const std::string&)>)deviceDisconnectedCallback
						 deviceRemoved: (std::function<void(const std::string&)>)deviceRemovedCallback;
-(void)setScanFilter:(NSMutableArray*)uuids;
-(void)scan;
-(void)stopScan;
-(void)connectPairedDevices;
-(void)connectDevice:(BleIODevice&)peripheral;
-(void)disconnectDevice:(BleIODevice&)peripheral;
#if APPLICATION == APP_NTOUCH_MAC
-(NSMutableArray*)getPairedDeviceIDs;
#endif

@end

NS_ASSUME_NONNULL_END

