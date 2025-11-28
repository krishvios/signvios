//
//  PulseViewController.mm
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2019 Sorenson Communications, Inc. -- All rights reserved
//

#import "PulseViewController.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "SCIContactManager.h"
#import "SCIVideophoneEngine.h"
#import "SCIAccountManager.h"
#import "IstiBluetooth.h"
#import "ntouch-Swift.h"

enum {
	connectedDevicesSection,
	discoveredDevicesSection,
	sections
};

@interface PulseViewController () {
	std::list<BluetoothDevice> m_connectedDevices;
	std::list<BluetoothDevice> m_discoveredDevices;
	CstiSignalConnection::Collection m_signalConnections;
}
@end

@implementation PulseViewController

#pragma mark - Table view delegate

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return sections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	
	if (section == connectedDevicesSection)
	{
		return m_connectedDevices.size();
	}
	else if (section == discoveredDevicesSection)
	{
		return m_discoveredDevices.size() + 1;
	}
	return 0;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
	if (section == connectedDevicesSection)
	{
		return Localize(@"Connected Devices");
	}
	else if (section == discoveredDevicesSection)
	{
		return Localize(@"Discovered Devices");
	}
	return @"";
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
	return [self viewController:self viewForHeaderInSection:section];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
	return @"";
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	
	if (indexPath.section == connectedDevicesSection)
	{
		ThemedTableViewCell *cell = [self.tableView dequeueReusableCellWithIdentifier:@"ConnectedDeviceCell"];
		if (!cell) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ConnectedDeviceCell"];
			cell.textLabel.font = [UIFont boldSystemFontOfSize:18];
		}
		auto listIter = m_connectedDevices.begin();
		std::advance(listIter, indexPath.row);
		cell.textLabel.text = [NSString stringWithUTF8String:(*listIter).name.c_str()];
		
		return cell;
	}
	else if (indexPath.section == discoveredDevicesSection)
	{
		if (indexPath.row == 0)
		{
			ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"ScanButtonCell"];
			if (cell == nil) {
				cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ScanButtonCell"];
				cell.textLabel.textAlignment = NSTextAlignmentCenter;
				cell.textLabel.font = [UIFont boldSystemFontOfSize:14];
				cell.textLabel.textColor = Theme.tableCellTintColor;
			}
			
			cell.textLabel.text = Localize(@"Scan for Pulse Devices");
			return cell;
		}
		else
		{
			ThemedTableViewCell *cell = [self.tableView dequeueReusableCellWithIdentifier:@"DiscoveredDeviceCell"];
			if (!cell) {
				cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"DiscoveredDeviceCell"];
				cell.textLabel.font = [UIFont boldSystemFontOfSize:18];
			}
			auto listIter = m_discoveredDevices.begin();
			std::advance(listIter, indexPath.row - 1);
			cell.textLabel.text = [NSString stringWithUTF8String:(*listIter).name.c_str()];
			
			return cell;
		}
	}
	
	static NSString *emptyId = @"EmptyCell";
	ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:emptyId];
	if (cell == nil) {
		cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:emptyId];
	}
	return cell;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
	return UITableViewCellEditingStyleNone;
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
	return NO;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	return (tableView.editing) ? indexPath : nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	
	if (indexPath.section == connectedDevicesSection)
	{
		auto listIter = m_connectedDevices.begin();
		std::advance(listIter, indexPath.row);
		std::string title ("Unpair " + (*listIter).name + "?");
		AlertWithTitleAndMessage ([NSString stringWithUTF8String:title.c_str()], Localize(@"To unpair devices,\ngo to iOS Settings > Bluetooth"));
	}
	else if (indexPath.section == discoveredDevicesSection)
	{
		if (indexPath.row == 0)
		{
			IstiBluetooth::InstanceGet()-> Scan ();
			self.haveScanned = YES;
		}
		else
		{
			auto listIter = m_discoveredDevices.begin();
			std::advance(listIter, indexPath.row - 1);
			IstiBluetooth::InstanceGet()->Connect(*listIter);
		}
	}
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
	[super viewDidLoad];
	
	self.tableView.allowsSelectionDuringEditing = YES;
	
	IstiBluetooth *bluetooth = IstiBluetooth::InstanceGet();
	m_signalConnections.push_back(bluetooth->deviceAddedSignalGet().Connect([self](BluetoothDevice bluetoothDevice){[self deviceAddedEvent];}));
	m_signalConnections.push_back(bluetooth->deviceConnectedSignalGet().Connect([self](BluetoothDevice bluetoothDevice){[self deviceConnectedEvent];}));
	m_signalConnections.push_back(bluetooth->deviceDisconnectedSignalGet().Connect([self](BluetoothDevice bluetoothDevice){[self deviceDisconnectedEvent];}));
	
	self.haveScanned = NO;
	[self getConnectedDevices];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];	
	[self.tableView setEditing:YES];
	self.title = Localize(@"Pulse Devices");
	self.navigationItem.rightBarButtonItem.enabled = YES;
	[self.tableView reloadData];
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

-(void)getConnectedDevices
{
	IstiBluetooth::InstanceGet ()->PairedListGet(&m_connectedDevices);
}

-(void)getDiscoveredDevices
{
	IstiBluetooth::InstanceGet ()->ScannedListGet(&m_discoveredDevices);
}

-(void)deviceAddedEvent
{
	// If the user hasn't scanned, don't show discovered devices
	if (self.haveScanned)
	{
		dispatch_async(dispatch_get_main_queue(),^{
			[self getDiscoveredDevices];
			[self.tableView reloadData];
		});
	}
}

-(void)deviceConnectedEvent
{
	dispatch_async(dispatch_get_main_queue(),^{
		[self getConnectedDevices];
		// If the user hasn't scanned, don't show discovered devices
		if (self.haveScanned)
		{
			[self getDiscoveredDevices];
		}
		[self.tableView reloadData];
	});
}

-(void)deviceDisconnectedEvent
{
	dispatch_async(dispatch_get_main_queue(),^{
		[self getConnectedDevices];
		[self.tableView reloadData];
	});
}

@end
