// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#import <Foundation/Foundation.h>
#if APPLICATION == APP_NTOUCH_MAC
#import <IOBluetooth/IOBluetooth.h>
#endif
#import "SCICentral.h"
#import "SCIPeripheral.h"
#import "CstiBluetooth.h"
#include "stiTools.h"
#include "stiDebugTools.h"
#import "SCIDefaults.h"

@interface SCICentral ()
{
	std::function<void(std::unique_ptr<BleIODevice>)> m_deviceDiscoveredCallback;
	std::function<void(const std::string&)> m_deviceConnectedCallback;
	std::function<void(const std::string&)> m_deviceDisconnectedCallback;
	std::function<void(const std::string&)> m_deviceRemovedCallback;
}

@end

@implementation SCICentral

static SCICentral *sharedInstance = nil;
+(SCICentral*)sharedInstance
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedInstance = [[self alloc] init];
	});
	return sharedInstance;
}

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

-(id)init
{
	self = [super init];
	if (self)
	{
		BOOL hasPairedDevices = false;
#if APPLICATION == APP_NTOUCH_IOS
		hasPairedDevices = (self.defaults.connectedPulseDevices.count > 0);
#endif
		NSDictionary *optionsDict = @{CBCentralManagerOptionShowPowerAlertKey : [NSNumber numberWithBool:hasPairedDevices]};
		centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil options:optionsDict];
		supportedUuids = [NSMutableArray arrayWithObjects:[CBUUID UUIDWithString:[NSString stringWithUTF8String:PULSE_SERVICE.c_str()]],nil];
		pairedDevices = [[NSArray alloc] init];
	}
	return self;
}

-(void)dealloc
{
}

-(void)setCallbacksForDeviceDiscovered: (std::function<void(std::unique_ptr<BleIODevice>)>)deviceDiscoveredCallback
					   deviceConnected: (std::function<void(const std::string&)>)deviceConnectedCallback
					deviceDisconnected: (std::function<void(const std::string&)>)deviceDisconnectedCallback
						 deviceRemoved: (std::function<void(const std::string&)>)deviceRemovedCallback
{
	m_deviceDiscoveredCallback = deviceDiscoveredCallback;
	m_deviceConnectedCallback = deviceConnectedCallback;
	m_deviceDisconnectedCallback = deviceDisconnectedCallback;
	m_deviceRemovedCallback = deviceRemovedCallback;
}

-(void)setScanFilter:(NSMutableArray*)uuids
{
	supportedUuids = uuids;
}

-(void)scan
{
	if ([self bluetoothEnabled])
	{
		if (@available(macOS 10.13, iOS 9.0, *)) {
			if (![centralManager isScanning]) {
				stiDEBUG_TOOL(g_stiBluetoothDebug,
							  stiTrace ("SCICentral: scan\n");
							  );
				
				self.isScanning = YES;
				
				[centralManager scanForPeripheralsWithServices:supportedUuids options:nil];
			}
		} else {
			if (!self.isScanning) {
				stiDEBUG_TOOL(g_stiBluetoothDebug,
							  stiTrace ("SCICentral: scan\n");
							  );
				
				self.isScanning = YES;
				
				[centralManager scanForPeripheralsWithServices:supportedUuids options:nil];
			}
		}
	}
}

-(void)scanForSeconds:(NSTimeInterval)seconds
{
    if ([self bluetoothEnabled])
    {
        if (@available(macOS 10.13, iOS 9.0, *)) {
            if (![centralManager isScanning]) {
                self.isScanning = YES;
                
                [centralManager scanForPeripheralsWithServices:supportedUuids options:nil];
                
                scanTimer = [NSTimer scheduledTimerWithTimeInterval:seconds repeats:NO block:^(NSTimer *timer)
                             {
                                 [timer invalidate];
                                 [self stopScan];
                             }];
            } else {
                if (!self.isScanning) {
                    self.isScanning = YES;
                    
                    [centralManager scanForPeripheralsWithServices:supportedUuids options:nil];
                    
                    scanTimer = [NSTimer scheduledTimerWithTimeInterval:seconds repeats:NO block:^(NSTimer *timer)
                                 {
                                     [timer invalidate];
                                     [self stopScan];
                                 }];
                }
            }
        }
    }
}

-(void)stopScan
{
	if (scanTimer && scanTimer.valid)
	{
		[scanTimer invalidate];
	}
	
	if (@available(macOS 10.13, iOS 9.0, *)) {
		if (centralManager.isScanning) {
			stiDEBUG_TOOL(g_stiBluetoothDebug,
						  stiTrace ("SCICentral: stopScan\n");
						  );
			
			[centralManager stopScan];
			
			self.isScanning = NO;
		}
	} else {
		if (self.isScanning) {
			stiDEBUG_TOOL(g_stiBluetoothDebug,
						  stiTrace ("SCICentral: stopScan\n");
						  );
			
			[centralManager stopScan];
			
			self.isScanning = NO;
		}
	}
}

-(void)connectPairedDevices
{
#if APPLICATION == APP_NTOUCH_MAC
	NSArray *deviceIDsToConnect = [self getPairedDeviceIDs];
	pairedDevices = [centralManager retrievePeripheralsWithIdentifiers:deviceIDsToConnect];
#else // APPLICATION == APP_NTOUCH_IOS
	NSArray *deviceIDsToConnect = [self.defaults connectedPulseDevices];
	if (nil != deviceIDsToConnect)
	{
		pairedDevices = [centralManager retrievePeripheralsWithIdentifiers:deviceIDsToConnect];
	}
#endif

	for (CBPeripheral *peripheral in pairedDevices)
	{
		[centralManager connectPeripheral:peripheral options:nil];
	}
}

-(void)connectDevice:(BleIODevice&)device
{
	SCIPeripheral *peripheral = (__bridge SCIPeripheral*)device.objcPeripheralGet ();
	[centralManager connectPeripheral:peripheral.cbPeripheral options:nil];
}

-(void)disconnectDevice:(BleIODevice&)device
{
	SCIPeripheral *peripheral = (__bridge SCIPeripheral*)device.objcPeripheralGet ();
	[centralManager cancelPeripheralConnection:peripheral.cbPeripheral];
}

#if APPLICATION == APP_NTOUCH_MAC
-(NSMutableArray*)getPairedDeviceIDs
{
	// Get currently paired devices
	NSMutableArray *pairedDeviceAddresses = [[NSMutableArray alloc] init];
	NSArray *pairedIOBTDevices = [IOBluetoothDevice pairedDevices];
	
	for (IOBluetoothDevice *device in pairedIOBTDevices)
	{
		[pairedDeviceAddresses addObject:device.addressString];
	}
	
	// For those devices that are paired, determine if they are BLE and, if so, connect them
	// using their UUIDs obtained from com.apple.bluetooth.plist
	NSMutableArray *pairedDeviceIDs = [[NSMutableArray alloc] init];
	NSURL *fileUrl = [NSURL fileURLWithPath:@"/Library/Preferences/com.apple.bluetooth.plist"];
	if (fileUrl)
	{
		NSData *data = [NSData dataWithContentsOfURL:fileUrl];
		if (data)
		{
			NSDictionary *bluetoothDict = [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nil error:nil];
			if (bluetoothDict)
			{
				NSDictionary *bleCachedDevices = [bluetoothDict objectForKey:@"CoreBluetoothCache"];
				
				if (bleCachedDevices)
				{
					for (NSString *address in pairedDeviceAddresses)
					{
						for (NSString *deviceID in bleCachedDevices)
						{
							NSDictionary *deviceData = bleCachedDevices[deviceID];
							NSString *deviceAddress = [deviceData objectForKey:@"DeviceAddress"];
							if ([address isEqualToString:deviceAddress])
							{
								[pairedDeviceIDs addObject:[[NSUUID UUID] initWithUUIDString:deviceID]];
								break;
							}
						}
					}
				}
			}
		}
	}
	
	return pairedDeviceIDs;
}
#endif

// CBCentralManagerDelegate methods
// Required. Invoked when the central manager’s state is updated.
-(void)centralManagerDidUpdateState:(CBCentralManager *)central
{
#if APPLICATION == APP_NTOUCH_IOS
	self.managerState = central.state;
#endif
	switch (central.state)
	{
		case CBManagerStateUnknown:
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("Central: Bluetooth State UNKNOWN\n");
			);
			break;
		case CBManagerStateResetting:
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("Central: Bluetooth State RESETTING\n");
			);
			break;
		case CBManagerStateUnsupported:
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("Central: Bluetooth State UNSUPPORTED\n");
			);
			break;
		case CBManagerStateUnauthorized:
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("Central: Bluetooth State UNAUTHORIZED\n");
			);
			break;
		case CBManagerStatePoweredOff:
			if ([self bluetoothEnabled])
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Central: Bluetooth Powered OFF\n");
				);
				
				[self setBluetoothEnabled:NO];
				
				if (@available(macOS 10.13, iOS 9.0, *)) {
					if (centralManager.isScanning) {
						[centralManager stopScan];
						self.isScanning = NO;
					}
				} else {
					if (self.isScanning) {
						[centralManager stopScan];
						self.isScanning = NO;
					}
				}
			}
			break;
		case CBManagerStatePoweredOn:
			if (![self bluetoothEnabled])
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Central: Bluetooth Powered ON\n");
				);

				[self setBluetoothEnabled:YES];
				
				[self connectPairedDevices];
			}
			break;
	}
}

// Invoked when the central manager discovers a peripheral while scanning.
-(void)centralManager:(CBCentralManager *)central
didDiscoverPeripheral:(CBPeripheral *)peripheral
	advertisementData:(NSDictionary<NSString *,id> *)advertisementData
				 RSSI:(NSNumber *)RSSI
{
	if (!peripheral.delegate)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Central: didDiscoverPeripheral %s\n", [peripheral.name UTF8String]);
		);
		
		if (m_deviceDiscoveredCallback)
		{
			std::vector<std::string> advertisedServiceUuids;
			for (CBUUID *uuid in [advertisementData objectForKey: @"kCBAdvDataServiceUUIDs"])
			{
				advertisedServiceUuids.push_back([uuid.UUIDString UTF8String]);
			}
			
			std::unique_ptr<BleIODevice> devicePtr = sci::make_unique<BleIODevice>((__bridge void*)peripheral, advertisedServiceUuids);
			
			m_deviceDiscoveredCallback (std::move (devicePtr));
			
			// Connect to those devices that are already paired
			for (CBPeripheral *pairedPeripheral in pairedDevices)
			{
				if (pairedPeripheral == peripheral)
				{
					[centralManager connectPeripheral:peripheral options:nil];
					break;
				}
			}
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("Central: didDiscoverPeripheral: Already received. This is likely due to a scan response.\n");
		);
	}
}

// Invoked when the central manager connects to a peripheral
-(void)centralManager:(CBCentralManager *)central
 didConnectPeripheral:(CBPeripheral *)peripheral
{
	// If the peripheral does not have a delegate, it was connected without discovery,
	// i.e. connected through the OS controller.
	// If it's a Pulse device, add a service uuid so it will be properly set up.
	if (!peripheral.delegate && m_deviceDiscoveredCallback)
	{
		std::string deviceName ([peripheral.name UTF8String]);
		std::vector<std::string> advertisedServiceUuids;

		if (deviceName.compare (0, 10, "SVRS Pulse") == 0)
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace ("Central: didConnectPeripheral: Adding Pulse device without having discovered it.\n");
			);

			advertisedServiceUuids.push_back (PULSE_SERVICE);
		
			std::unique_ptr<BleIODevice> devicePtr = sci::make_unique<BleIODevice>((__bridge void*)peripheral, advertisedServiceUuids);
		
			m_deviceDiscoveredCallback (std::move (devicePtr));
		}
		else
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
				stiTrace ("Central: didConnectPeripheral: An unsupported paired device (%s) was connected.\n", deviceName.c_str());
			);
		}
	}
	
	if (@available(macOS 10.13, iOS 7.0, *)) {
		[self deviceConnectedWithUuid:peripheral.identifier];
	} else {
		NSUUID *uuid = [peripheral valueForKey:@"identifier"];
		[self deviceConnectedWithUuid:uuid];
	}
}

// Invoked when the central manager fails to connect to a peripheral
-(void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral
				error:(NSError *)error
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace ("Central: didFailToConnectPeripheral %s\n", [peripheral.name UTF8String]);
	);
}

// Invoked when the central manager disconnects from a peripheral
-(void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral
				error:(NSError *)error
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace ("Central: didDisconnectPeripheral %s\n", [peripheral.name UTF8String]);
	);

	if (m_deviceDisconnectedCallback)
	{
		if (@available(macOS 10.13, iOS 7.0, *)) {
			m_deviceDisconnectedCallback ([peripheral.identifier.UUIDString UTF8String]);
		} else {
			NSUUID *uuid = [peripheral valueForKey:@"identifier"];
			m_deviceDisconnectedCallback ([uuid.UUIDString UTF8String]);
		}
	}
	
	if (error)
	{
		switch (error.code)
		{
			case CBErrorConnectionTimeout:
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Disconnected due to a timeout.\n");
				);
				
				// On a timeout, call connect so reconnect will occur
				[centralManager connectPeripheral:peripheral options:nil];
				break;
			}
			case CBErrorPeripheralDisconnected:
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("The remote peripheral disconnected from us.\n");
				);
				
				if (@available(macOS 10.13, iOS 7.0, *)) {
					[self deviceRemovedWithUuid:peripheral.identifier];
				} else {
					NSUUID *uuid = [peripheral valueForKey:@"identifier"];
					[self deviceRemovedWithUuid:uuid];
				}
				break;
			}
			default:
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Unhandled disconnect error. CBError Code: %d\n", error.code);
				);
				break;
			}
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("User selected to disconnect from peripheral.\n");
		);
		
		if (@available(macOS 10.13, iOS 7.0, *)) {
			[self deviceRemovedWithUuid:peripheral.identifier];
		} else {
			NSUUID *uuid = [peripheral valueForKey:@"identifier"];
			[self deviceRemovedWithUuid:uuid];
		}
	}
}

-(void)deviceConnectedWithUuid:(NSUUID *)uuid
{
	if (m_deviceConnectedCallback)
	{
		m_deviceConnectedCallback ([uuid.UUIDString UTF8String]);
	}

#if APPLICATION == APP_NTOUCH_IOS
	// For iOS update connected devices default list
	NSMutableArray *savedDeviceIds = [NSMutableArray arrayWithArray:[self.defaults connectedPulseDevices]];
	bool deviceFound = false;
	for (NSUUID* identifier in savedDeviceIds)
	{
		if ([identifier isEqual:uuid])
		{
			deviceFound = true;
			break;
		}
	}
	
	if (!deviceFound)
	{
		[savedDeviceIds addObject:uuid];
		[self.defaults setConnectedPulseDevices:savedDeviceIds];
	}
#endif
}

-(void)deviceRemovedWithUuid:(NSUUID *)uuid
{
	if (m_deviceRemovedCallback)
	{
		m_deviceRemovedCallback ([uuid.UUIDString UTF8String]);
	}

#if APPLICATION == APP_NTOUCH_IOS
	// For iOS update connected devices default list
	NSMutableArray *savedDeviceIds = [NSMutableArray arrayWithArray:[self.defaults connectedPulseDevices]];
	[savedDeviceIds removeObject:uuid];
	[self.defaults setConnectedPulseDevices:savedDeviceIds];
#endif
}

@end

