//
//  SettingsViewController.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "SettingsViewController.h"
#import "Utilities.h"
#import "MyPhoneSetupViewController.h"
#import "ImportContactsViewController.h"
#import "ChangePasswordViewController.h"
#import "AppDelegate.h"
#import "Alert.h"
#import "TypeListController.h"
#import "PersonalInfoViewController.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "MyPhoneEditViewControllerNew.h"
#import "IstiRingGroupManager.h"
#import "SCIPropertyNames.h"
#import "SignMailGreetingPlayViewController.h"
#import "SCIAccountManager.h"
#import "SCIDefaults.h"
#import "SCIContactManager.h"
#import "MBProgressHUD.h"
#import "ntouch-Swift.h"
#import <AVFoundation/AVFoundation.h>
#import "PulseViewController.h"
#import "ntouch-Swift.h"
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>

static const CGFloat kContactPhotoThumbnailSize = 70.0;

@interface SettingsViewController () <UISearchResultsUpdating, ApplicationSharingCellDelegate>

@property (nonatomic, readonly) SCIVideophoneEngine *engine;
@property (nonatomic, readonly) SCIAccountManager *accountManager;
@property (nonatomic, readonly) SCIDefaults *defaults;
@property (nonatomic, strong) NSArray *settingsSectionsAndRowsArray;
@property (nonatomic, strong) NSString *rowToSelect;
@property (nonatomic, strong) UISearchController *searchController;
@property (nonatomic, strong) UIImage *profileImage;

@end

@implementation SettingsViewController

@synthesize audioEnabledSwitch;
@synthesize vcoActiveSwitch;
@synthesize vcoActiveLabel;
@synthesize autoRejectCallsSwitch;
@synthesize callWaitingSwitch;
@synthesize secureCallModeSwitch;
@synthesize tunnelingSwitch;
@synthesize nvpIpAddressTextField;
@synthesize blockCallerIDSwitch;
@synthesize blockAnonymousCallersSwitch;
@synthesize enableSpanishFeaturesSwitch;
@synthesize pulseEnableSwitch;

#pragma mark - Accessors

- (SCIVideophoneEngine *)engine {
	return [SCIVideophoneEngine sharedEngine];
}

- (SCIAccountManager *)accountManager {
	return [SCIAccountManager sharedManager];
}

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

#pragma mark - View lifecycle

- (void)viewDidLoad {
	[super viewDidLoad];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyUserAccountInfoUpdated:)
												 name:SCINotificationUserAccountInfoUpdated
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupModeChanged:)
												 name:SCINotificationRingGroupModeChanged
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyPropertyChanged:)
												 name:SCINotificationPropertyChanged
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyAutoSpeedModeChanged:)
												 name:SCINotificationAutoSpeedSettingChanged
	 											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyMaxSpeedsChanged:)
												 name:SCINotificationMaxSpeedsChanged
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingsBeforeGreetingChanged:)
												 name:SCINotificationRingsBeforeGreetingChanged
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyConnecting:)
												 name:SCINotificationConnecting
											   object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyTheme)
												 name:Theme.themeDidChangeNotification
											   object:nil];
	
	[self.tableView registerNib:TextInputTableViewCell.nib forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	[self.tableView registerNib:ApplicationSharingCell.nib forCellReuseIdentifier:ApplicationSharingCell.cellIdentifier];
	
	self.title = Localize(@"Settings");
	
	self.searchController = [[UISearchController alloc] initWithSearchResultsController:nil];
	self.searchController.searchResultsUpdater = self;
	self.searchController.obscuresBackgroundDuringPresentation = NO;
	self.searchController.searchBar.placeholder = Localize(@"Search Settings");
	
	if (@available(iOS 11.0, *)) {
		self.navigationItem.searchController = self.searchController;
		self.navigationItem.hidesSearchBarWhenScrolling = NO;
	} else {
		self.tableView.tableHeaderView = self.searchController.searchBar;
	}
	self.definesPresentationContext = YES;
}

- (void)applyTheme {
	self.view.backgroundColor = Theme.backgroundColor;
	[self setNeedsStatusBarAppearanceUpdate];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
	return Theme.statusBarStyle;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	self.searchController.searchBar.barStyle = Theme.searchBarStyle;
	self.searchController.searchBar.keyboardAppearance = Theme.keyboardAppearance;
	
	
	UIBarButtonItem *logoutButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Log Out")
																	 style:UIBarButtonItemStyleDone
																	target:self action:@selector(logoutButtonTapped:)];
	if (self.engine.isAuthenticated) {
		self.navigationItem.rightBarButtonItem = logoutButton;
	}
	
	[self applyTheme];
	
    [self loadTableData];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[self.navigationController setNavigationBarHidden:NO animated:YES];
	[self.searchController setActive:NO];
}

- (IBAction)logoutButtonTapped:(id)sender {
	[SCIENSController.shared unregister];
	[[SCIPropertyManager sharedManager] waitForSendWithTimeout: 5];
	[self.accountManager clientReauthenticate];
	[AnalyticsManager.shared trackUsage:AnalyticsEventLogout properties:nil];
    
	// Allow autoEnableSpanishFeatures again after logging out
	[[NSUserDefaults standardUserDefaults] setBool: NO forKey:kAutoEnableSpanishFeaturesTurnedOffByUserKey];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[self checkCoreOffline];
	[self.tableView reloadData];
}

- (void)checkCoreOffline {
	// If Core is Offline change selected tab to the Keypad
	if (![SCIVideophoneEngine sharedEngine].isConnected &&
		(self.isViewLoaded && self.view.window)) {
		[self.navigationController popToRootViewControllerAnimated:NO];
		[appDelegate.tabBarController setSelectedIndex:0];
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError
															object:[SCIVideophoneEngine sharedEngine]
														  userInfo:nil];
	}
}

- (void)loadTableData
{
    NSMutableArray *mutableSettingsSectionsAndRowsArray = [NSMutableArray array];
	
	if ([[SCIVideophoneEngine sharedEngine] userAccountInfo] != nil) {
		NSMutableArray *headerArray = [[NSMutableArray alloc] init];
		
		if ([SCIVideophoneEngine sharedEngine].DeafHearingVideoEnabled) {
			[headerArray addObject: [[SettingItem alloc] initWithRowName:@"appSharing" searchWords:@""]];
		}
		
		[headerArray addObject: [[SettingItem alloc] initWithRowName:@"header" searchWords:@""]];
		[mutableSettingsSectionsAndRowsArray addObject:@{
			@"sectionNameKey": @"headerSection",
			@"rows": headerArray
		}];
	}
	
	NSMutableArray *setupSectionArray = [@[
	   [[SettingItem alloc] initWithRowName:@"personalInfoRow" searchWords:Localize(@"settings.search.personalInfoRow")],
	   [[SettingItem alloc] initWithRowName:@"setupEmergencyRow" searchWords:Localize(@"settings.search.setupEmergencyRow")],
	   [[SettingItem alloc] initWithRowName:@"myPhoneSettingsRow" searchWords:Localize(@"settings.search.myPhoneSettingsRow")],
	   [[SettingItem alloc] initWithRowName:@"importContactsRow" searchWords:Localize(@"settings.search.importContactsRow")],
	   [[SettingItem alloc] initWithRowName:@"ringsBeforeGreetingRow" searchWords:Localize(@"settings.search.ringsBeforeGreetingRow")],
	   [[SettingItem alloc] initWithRowName:@"signMailGreetingTypeRow" searchWords:Localize(@"settings.search.signMailGreetingTypeRow")],
	   
	   [[SettingItem alloc] initWithRowName:@"colorTheme" searchWords:Localize(@"settings.search.colorTheme")],
	   [[SettingItem alloc] initWithRowName:@"blockedNumbers" searchWords:Localize(@"settings.search.blockedNumbers")],
	   ] mutableCopy];

	if ([[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModeHearing) {
		[self removeRowName:@"setupEmergencyRow" fromArray:setupSectionArray];
	}
	
	if (![self isPersonalSignMailGreetingRowVisible]) {
		[self removeRowName:@"signMailGreetingTypeRow" fromArray:setupSectionArray];
	}
	
	if (![self isMyPhoneRowVisible]) {
		[self removeRowName:@"myPhoneSettingsRow" fromArray:setupSectionArray];
    }
   
    NSDictionary *setupSectionArrayDict = @{ @"sectionNameKey" : @"setupSection", @"rows": setupSectionArray };
    [mutableSettingsSectionsAndRowsArray addObject:setupSectionArrayDict];
    
	// Call Options Section
	NSMutableArray *callOptionsSectionArray = [@[
		[[SettingItem alloc] initWithRowName:@"secureCallModeRow" searchWords:Localize(@"settings.search.secureCallModeRow")],
		[[SettingItem alloc] initWithRowName:@"shareTextRow" searchWords:Localize(@"settings.search.shareTextRow")],
		[[SettingItem alloc] initWithRowName:@"audioRow" searchWords:Localize(@"settings.search.audioRow")],
		[[SettingItem alloc] initWithRowName:@"vcoRow" searchWords:Localize(@"settings.search.vcoRow")],
		[[SettingItem alloc] initWithRowName:@"autoRejectCallsRow" searchWords:Localize(@"settings.search.autoRejectCallsRow")],
		[[SettingItem alloc] initWithRowName:@"enableSpanishFeatures" searchWords:Localize(@"settings.search.enableSpanishFeatures")],
	] mutableCopy];
	
	if ([[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModeHearing) {
		[self removeRowName:@"vcoRow" fromArray:callOptionsSectionArray];
		[self removeRowName:@"vcoModeRow" fromArray:callOptionsSectionArray];
		[self removeRowName:@"enableSpanishFeatures" fromArray:callOptionsSectionArray];
	}
	
	NSDictionary *callOptionsSectionArrayDict =  @{ @"sectionNameKey" : @"callOptionsSection", @"rows": callOptionsSectionArray };
	[mutableSettingsSectionsAndRowsArray addObject:callOptionsSectionArrayDict];
	
    //deviceSettingsSection data
	NSMutableArray *deviceSettingsSectionArray = [@[
		[[SettingItem alloc] initWithRowName:@"callWaitingRow" searchWords:Localize(@"settings.search.callWaitingRow")],
		[[SettingItem alloc] initWithRowName:@"blockCallerIDRow" searchWords:Localize(@"settings.search.blockCallerIDRow")],
		[[SettingItem alloc] initWithRowName:@"blockAnonymousCallers" searchWords:Localize(@"settings.search.blockAnonymousCallers")],
		] mutableCopy];
	
	if ([[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModeHearing) {
		[self removeRowName:@"vcoRow" fromArray:deviceSettingsSectionArray];
		[self removeRowName:@"vcoModeRow" fromArray:deviceSettingsSectionArray];
		[self removeRowName:@"enableSpanishFeatures" fromArray:deviceSettingsSectionArray];
	}
	
	if ([[SCIVideophoneEngine sharedEngine] blockCallerIDEnabled] == NO) {
		[self removeRowName:@"blockCallerIDRow" fromArray:deviceSettingsSectionArray];
	}

    NSDictionary *deviceSettingsSectionArrayDict =  @{ @"sectionNameKey" : @"deviceSettingsSection", @"rows": deviceSettingsSectionArray };
    [mutableSettingsSectionsAndRowsArray addObject:deviceSettingsSectionArrayDict];
	
	//pulseSettingsSection data
	if (self.engine.isPulsePowered){
		NSMutableArray *pulseSettingsSectionArray = [@[
			[[SettingItem alloc] initWithRowName:@"pulseEnableRow" searchWords:Localize(@"settings.search.pulseEnableRow")],
			[[SettingItem alloc] initWithRowName:@"pulseManageDevicesRow" searchWords:Localize(@"settings.search.pulseManageDevicesRow")]
			] mutableCopy];
		
		NSDictionary *pulseSettingsSectionArrayDict = @{ @"sectionNameKey" : @"pulseSettingsSection", @"rows": pulseSettingsSectionArray };
		[mutableSettingsSectionsAndRowsArray addObject:pulseSettingsSectionArrayDict];
	}

    //networkSettingsSection data
	NSMutableArray *networkSettingsSectionArray = [@[
		[[SettingItem alloc] initWithRowName:@"tunnelingRow" searchWords:Localize(@"settings.search.tunnelingRow")],
		[[SettingItem alloc] initWithRowName:@"cellularVideoQualityRow" searchWords:Localize(@"settings.search.cellularVideoQualityRow")]
		] mutableCopy];
    
#ifdef stiENABLE_VP_REMOTE
	[networkSettingsSectionArray addObject: [[SettingItem alloc] initWithRowName:@"nvpIpAddressRow" searchWords:@"NVP IP Address Remote"]];
#endif
	
	SCIAutoSpeedMode mode = [[SCIVideophoneEngine sharedEngine]networkAutoSpeedMode];
	if (mode == SCIAutoSpeedModeLegacy) {
		[networkSettingsSectionArray insertObject:[[SettingItem alloc] initWithRowName:@"networkSpeedRow" searchWords:Localize(@"settings.search.networkSpeedRow")] atIndex:0];
	}
	else if (mode == SCIAutoSpeedModeLimited) {
		[networkSettingsSectionArray insertObject:[[SettingItem alloc] initWithRowName:@"networkSpeedReadOnlyRow" searchWords:Localize(@"settings.search.networkSpeedReadOnlyRow")] atIndex:0];
	}
	
    NSDictionary *networkSettingsSectionArrayDict =  @{ @"sectionNameKey" : @"networkSettingsSection", @"rows": networkSettingsSectionArray };
    [mutableSettingsSectionsAndRowsArray addObject:networkSettingsSectionArrayDict];
	
	// Other Section
	NSMutableArray *helpSettingsSectionArray = [@[
		 [[SettingItem alloc] initWithRowName:@"helpRow" searchWords:Localize(@"settings.search.helpRow")]
		 ] mutableCopy];
												
	NSDictionary *helpSettingsSectionArrayDict =  @{ @"sectionNameKey" : @"helpSettingsSection", @"rows": helpSettingsSectionArray };
	[mutableSettingsSectionsAndRowsArray addObject:helpSettingsSectionArrayDict];
	
	if (self.searchController.searchBar.text.length > 0) {
		self.settingsSectionsAndRowsArray = [[self performSearch:mutableSettingsSectionsAndRowsArray withString:self.searchController.searchBar.text] copy];
	}
	else {
		self.settingsSectionsAndRowsArray = [mutableSettingsSectionsAndRowsArray copy];
	}
}

- (void)removeRowName:(NSString *)rowToRemove fromArray:(NSMutableArray *)array {
	SettingItem *foundRow = nil;
	for (SettingItem *rowItem in array) {
		if ([rowItem.rowName isEqualToString:rowToRemove]) {
			foundRow = rowItem;
		}
	}
	if (foundRow) {
		[array removeObject:foundRow];
	}
}

- (BOOL)isMyPhoneRowVisible {

	BOOL isHearingMode = ([[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModeHearing);
	if (isHearingMode)
		return NO;
	
	BOOL myPhoneRowVisible = NO;
	NSInteger displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	switch (displayMode) {
		case IstiRingGroupManager::eRING_GROUP_DISABLED:
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_HIDDEN:
			myPhoneRowVisible = NO; break;
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY:
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_WRITE:
			myPhoneRowVisible = YES; break;
		default:
			myPhoneRowVisible = NO;
	}
	return myPhoneRowVisible;
}

- (BOOL)isPersonalSignMailGreetingRowVisible {
	BOOL personalSignMailGreetingRowVisible = [[SCIVideophoneEngine sharedEngine] personalSignMailGreetingFeatureEnabled];
	
	return personalSignMailGreetingRowVisible;
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	// Return the number of sections.
    return [self.settingsSectionsAndRowsArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    //Number of rows it should expect should be based on the section
    NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    return [array count];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:section];
    NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
    
    if ([sectionName isEqualToString:@"setupSection"]) {
        return Localize(@"Profile Settings");
    }
	if ([sectionName isEqualToString:@"callOptionsSection"]) {
		return Localize(@"Call Options");
	}
    if ([sectionName isEqualToString:@"deviceSettingsSection"]) {
        return Localize(@"Device Settings");
    }
	if ([sectionName isEqualToString:@"pulseSettingsSection"]) {
		return Localize(@"Pulse\u2122 Settings");
	}
    if ([sectionName isEqualToString:@"networkSettingsSection"]) {
        return Localize(@"Network Settings");
    }
	if ([sectionName isEqualToString:@"helpSettingsSection"]) {
		return Localize(@"Support");
	}
	if ([sectionName isEqualToString:@"searchSection"]) {
		return Localize(@"Search Results");
	}
    
    return @"";
	
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:section];
    NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
	if ([sectionName isEqualToString:@"helpSettingsSection"]) {
		NSString *phoneNumber = Localize(@"Not available.");
		if([SCIVideophoneEngine sharedEngine].userAccountInfo.preferredNumber.length)
			phoneNumber = FormatAsPhoneNumber([SCIVideophoneEngine sharedEngine].userAccountInfo.preferredNumber);
		
		NSString *name = Localize(@"ntouch® Mobile");//[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"];
		NSString *longVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
		NSString *shortVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
		
		NSString *title = [NSString stringWithFormat:Localize(@"settings.main.footer"), name, phoneNumber, name, shortVersion, longVersion];
		return title;
//		return [NSString stringWithFormat:Localize(@"Sorenson %@ Number:\n%@\n\n%@ for iOS Version %@ (%@)\n\nAll trademarks mentioned herein are the property of their respective owners.\n\nThis product may be covered by one or more patents listed on http://www.sorenson.com/patents."), name, phoneNumber, name, shortVersion, longVersion];
	}
	return nil;
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
	UITableViewHeaderFooterView *headerView = (UITableViewHeaderFooterView *)view;
	headerView.textLabel.textColor  = Theme.tableHeaderGroupedTextColor;
}

- (void)tableView:(UITableView *)tableView willDisplayFooterView:(UIView *)view forSection:(NSInteger)section
{
	NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:section];
	NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
	
	if ([sectionName isEqualToString:@"helpSettingsSection"]) {
		UITableViewHeaderFooterView *headerView = (UITableViewHeaderFooterView *)view;
		headerView.textLabel.textColor  = Theme.tableFooterGroupedTextColor;
	}
}

- (void)controlValueChanged:(id)sender {
	UISwitch *switchControl = (UISwitch*)sender;
	if (switchControl == secureCallModeSwitch) {
		[SCIVideophoneEngine sharedEngine].secureCallMode = switchControl.isOn ? SCISecureCallModePreferred : SCISecureCallModeNotPreferred;
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"secure_call_mode" }];
	}
	else if (switchControl == callWaitingSwitch) {
		[[SCIVideophoneEngine sharedEngine] setCallWaitingEnabled:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"call_waiting" }];
	}
	else if (switchControl == autoRejectCallsSwitch) {
		[[SCIVideophoneEngine sharedEngine] setDoNotDisturbEnabled:switchControl.isOn];
		if(switchControl.isOn) {
			[SCIENSController.shared unregister];
			AlertWithTitleAndMessage(Localize(@"Do Not Disturb"), Localize(@"settings.main.autoRejectCallsSwitch"));
		}
		else {
			[SCIENSController.shared register];
		}
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"do_not_disturb" }];
	}
	else if (switchControl == audioEnabledSwitch) {
		
		BOOL audioEnabled = switchControl.isOn;
		[[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
			if (granted) {
				[SCIAccountManager sharedManager].audioAndVCOEnabled = audioEnabled;
				NSInteger targetBitRateSetting = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRate"];
				if (audioEnabled && targetBitRateSetting > 0 && targetBitRateSetting < 1920) {
					[appDelegate setBitRateSetting:192];
					Alert(Localize(@"Speed Changed"), Localize(@"settings.main.speed.192"));
				}
			}
			else {
				dispatch_async(dispatch_get_main_queue(), ^{
					[SCIAccountManager sharedManager].audioAndVCOEnabled = NO;
					[self.audioEnabledSwitch setOn:NO animated:YES];
					[self.tableView performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
					
					UIAlertController *viewController = [UIAlertController alertControllerWithTitle:Localize(@"Microphone Privacy")
																							message:Localize(@"settings.main.micophone.noaccess")
																					 preferredStyle:UIAlertControllerStyleAlert];
					[viewController addAction:[UIAlertAction actionWithTitle:Localize(@"Open Settings") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
						NSURL *settingsURL = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
						[UIApplication.sharedApplication openURL:settingsURL options:@{} completionHandler:nil];
					}]];
					[viewController addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil]];
					[appDelegate.topViewController presentViewController:viewController animated:YES completion:nil];
				});
			}
			
		}];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"use_voice" }];
	}
	else if (switchControl == vcoActiveSwitch) {
		[[SCIVideophoneEngine sharedEngine] setVcoActivePrimitive:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"use_voice_for_vrs" }];
	}
	else if (switchControl == tunnelingSwitch) {
		[[SCIVideophoneEngine sharedEngine] setTunnelingEnabled:switchControl.isOn];
		[[FIRCrashlytics crashlytics] setCustomValue:[NSNumber numberWithBool:switchControl.isOn] forKey:@"Tunneling_Enabled"];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"tunneling" }];
	}
	else if (switchControl == blockCallerIDSwitch) {
		[[SCIVideophoneEngine sharedEngine] setBlockCallerID:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"hide_caller_id" }];
	}
	else if (switchControl == blockAnonymousCallersSwitch) {
		[[SCIVideophoneEngine sharedEngine] setBlockAnonymousCallers:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"block_anonymous_calls" }];
	}
	else if (switchControl == enableSpanishFeaturesSwitch)
	{
		if (!switchControl.isOn) {
			[[NSUserDefaults standardUserDefaults] setBool: YES forKey:kAutoEnableSpanishFeaturesTurnedOffByUserKey];
			[[NSUserDefaults standardUserDefaults] synchronize];
		}
		[[SCIVideophoneEngine sharedEngine] setSpanishFeaturesPrimitive:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"spanish_features" }];
	}
	else if (switchControl == pulseEnableSwitch) {
		self.defaults.pulseEnable = switchControl.isOn;
		[[SCIVideophoneEngine sharedEngine] pulseEnable:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"pulse" }];
	}
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	if (textField == self.nvpIpAddressTextField) {
		[[NSUserDefaults standardUserDefaults] setObject:textField.text forKey:@"AnswerOnVPKey"];
		[[NSUserDefaults standardUserDefaults] synchronize];
	}
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {

    NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
	SettingItem *row = nil;

	// Fixes condition when data has change during reload process.  check bounds
	if (indexPath.row <= array.count -1)
		row = [array objectAtIndex:indexPath.row];
	
	static NSString *CellIdentifier = @"SizeableCell";
	
	if ([row.rowName isEqualToString:@"appSharing"]) {
		ApplicationSharingCell *cell = (ApplicationSharingCell *)[tableView dequeueReusableCellWithIdentifier:ApplicationSharingCell.cellIdentifier forIndexPath:indexPath];
		cell.delegate = self;
		return cell;
	}
	
	if ([row.rowName isEqualToString:@"header"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"HeaderCell" forIndexPath:indexPath];
		
		SCIUserAccountInfo *userAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
		BOOL isRingGroup = ((userAccountInfo.ringGroupLocalNumber && [userAccountInfo.ringGroupLocalNumber length] > 0) ||
							(userAccountInfo.ringGroupTollFreeNumber && [userAccountInfo.ringGroupTollFreeNumber length] > 0));
		NSString *fullName = (isRingGroup) ? userAccountInfo.ringGroupName : userAccountInfo.name;
		NSString *local = (isRingGroup) ? userAccountInfo.ringGroupLocalNumber : userAccountInfo.localNumber;
		cell.textLabel.text = fullName;
		cell.detailTextLabel.text = FormatAsPhoneNumber(local);
		
		if (!self.profileImage) {
			self.profileImage = [[UIImage imageNamed:@"avatar_default"] resize:CGSizeMake(kContactPhotoThumbnailSize, kContactPhotoThumbnailSize)];
			[[SCIVideophoneEngine sharedEngine] loadUserAccountPhotoWithCompletion:^(NSData *data, NSError *error) {
					if (data) {
						self.profileImage = [[[UIImage alloc] initWithData:data] resize:CGSizeMake(kContactPhotoThumbnailSize, kContactPhotoThumbnailSize)];
						cell.imageView.image = self.profileImage;
					}
			}];
		}
		else {
			cell.imageView.image = self.profileImage;
		}

		return cell;
	}
    if ([row.rowName isEqualToString:@"personalInfoRow"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
        cell.textLabel.text = Localize(@"Personal Information");
		cell.detailTextLabel.text = @"";
        return cell;
    }
    if ([row.rowName isEqualToString:@"setupEmergencyRow"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
        cell.textLabel.text = Localize(@"911 Location Information");
		cell.detailTextLabel.text = @"";
        return cell;
    }
    if ([row.rowName isEqualToString:@"importContactsRow"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
        cell.textLabel.text = Localize(@"Contacts");
		cell.detailTextLabel.text = @"";
        return cell;
    }
    if ([row.rowName isEqualToString:@"secureCallModeRow"]) {
		static NSString *CellIdentifier = @"secureCallModeSwitchCell";
        SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
            [cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
            secureCallModeSwitch = cell.switchControl;
        }
        cell.textLabel.text = Localize(@"Encrypt My Calls");
		SCISecureCallMode callMode = [SCIVideophoneEngine sharedEngine].secureCallMode;
		switch (callMode) {
			case SCISecureCallModeRequired:
				cell.switchControl.enabled = NO;
				cell.switchControl.on = YES;
				break;
			case SCISecureCallModePreferred:
				cell.switchControl.enabled = YES;
				cell.switchControl.on = YES;
				break;
			case SCISecureCallModeNotPreferred:
				cell.switchControl.enabled = YES;
				cell.switchControl.on = NO;
				break;
		}
        return cell;
    }

    if ([row.rowName isEqualToString:@"callWaitingRow"]) {
		static NSString *CellIdentifier = @"callWaitingSwitchCell";
        SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
            [cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
            callWaitingSwitch = cell.switchControl;
        }
        cell.textLabel.text = Localize(@"Call Waiting Enabled");
        cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] callWaitingEnabled];
        return cell;
    }
    if ([row.rowName isEqualToString:@"autoRejectCallsRow"]) {
		
		static NSString *CellIdentifier = @"autoRejectSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
            [cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
            autoRejectCallsSwitch = cell.switchControl;
        }
        cell.textLabel.text = Localize(@"Do Not Disturb (Reject Incoming Calls)");
        cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] doNotDisturbEnabled];
        return cell;
    }
	if ([row.rowName isEqualToString:@"audioRow"]) {
		
		static NSString *CellIdentifier = @"audioSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			audioEnabledSwitch = cell.switchControl;
		}
		cell.textLabel.text = Localize(@"Use voice");
		
		BOOL audioAndVCOEnabled = [SCIAccountManager sharedManager].audioAndVCOEnabled;
		cell.switchControl.on = audioAndVCOEnabled;
		return cell;
	}
	if ([row.rowName isEqualToString:@"vcoRow"]) {
		
		static NSString *CellIdentifier = @"vcoSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			self.vcoActiveSwitch = cell.switchControl;
			self.vcoActiveLabel = cell.textLabel;
			cell.indentationLevel = 1;
		}
		
		BOOL audioAndVCOEnabled = [SCIAccountManager sharedManager].audioAndVCOEnabled;
		self.vcoActiveLabel.text = Localize(@"Use voice for VRS calls");
		self.vcoActiveSwitch.enabled = audioAndVCOEnabled;
		self.vcoActiveLabel.enabled = audioAndVCOEnabled;
		// Rule: Disable vcoActiveSwitch if audio is off
		self.vcoActiveSwitch.on = audioAndVCOEnabled ? [SCIVideophoneEngine sharedEngine].vcoActive : NO;
		
		return cell;
	}
	if ([row.rowName isEqualToString:@"blockCallerIDRow"]) {
		static NSString *CellIdentifier = @"BlockCallerIDSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			blockCallerIDSwitch = cell.switchControl;
		}
		cell.textLabel.text = Localize(@"Hide My Caller ID");
        cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] blockCallerID];
		return cell;
	}
	if ([row.rowName isEqualToString:@"blockAnonymousCallers"]) {
		static NSString *CellIdentifier = @"BlockAnonymousCallersSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			blockAnonymousCallersSwitch = cell.switchControl;
		}
		cell.textLabel.text = Localize(@"Block Anonymous Callers");
		cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] blockAnonymousCallers];
		return cell;
	}
	if ([row.rowName isEqualToString:@"enableSpanishFeatures"]) {
		static NSString *CellIdentifier = @"EnableSpanishFeaturesSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			enableSpanishFeaturesSwitch = cell.switchControl;
		}
		cell.textLabel.text = Localize(@"Enable Spanish Features");
		cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] SpanishFeatures];
		return cell;
	}
    if ([row.rowName isEqualToString:@"shareTextRow"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
        cell.textLabel.text = Localize(@"Saved Text");
		cell.detailTextLabel.text = @"";
        return cell;
    }
	if ([row.rowName isEqualToString:@"colorTheme"]) {
		static NSString *CellIdentifier = @"colorThemeCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		}
		
		cell.textLabel.text = Localize(@"Color Theme");
		cell.detailTextLabel.text = Localize(Theme.name);
		return cell;
	}
	if ([row.rowName isEqualToString:@"blockedNumbers"]) {
		static NSString *CellIdentifier = @"blockedNumbers";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		}
		cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
		cell.textLabel.text = Localize(@"Blocked Numbers");
		return cell;
	}
    if ([row.rowName isEqualToString:@"myPhoneSettingsRow"]) {
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
        cell.textLabel.text = Localize(@"myPhone");
		cell.detailTextLabel.text = @"";
        return cell;
    }
	if ([row.rowName isEqualToString:@"pulseEnableRow"]) {
		static NSString *CellIdentifier = @"PulseSwitchCell";
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
			pulseEnableSwitch = cell.switchControl;
		}
		cell.textLabel.text = Localize(@"Enable Pulse Notifications");
		cell.switchControl.on = self.defaults.pulseEnable;
		return cell;
	}
	if ([row.rowName isEqualToString:@"pulseManageDevicesRow"]) {
		static NSString *CellIdentifier = @"PulseManageDevicesCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			[cell setSelectionStyle:UITableViewCellSelectionStyleBlue];
		}
		cell.textLabel.text = Localize(@"Manage Pulse Devices");
		cell.detailTextLabel.text = @"";
		return cell;
	}
    if ([row.rowName isEqualToString:@"ringsBeforeGreetingRow"]) {
        static NSString *CellIdentifier = @"ringsBeforeGreetingCell";
        ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
        }
        
        cell.textLabel.text = Localize(@"Play SignMail greeting after");
        
		NSUInteger ringsCount = [SCIVideophoneEngine sharedEngine].ringsBeforeGreeting;
		NSString *ring = Localize(@"Ring");
		NSString *rings = Localize(@"Rings");
		NSString *ringCountStr = [NSString stringWithFormat: @"%ld %@", (long)ringsCount, (ringsCount == 1 ? ring : rings)];
		
        cell.detailTextLabel.text = ringCountStr;
        return cell;
    }
    if ([row.rowName isEqualToString:@"signMailGreetingTypeRow"]) {
		static NSString *CellIdentifier = @"signMailGreetingTypeCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.detailTextLabel.text = @"";
		}
		
		cell.textLabel.text = Localize(@"SignMail Greeting");
		NSUInteger greetingType = [[SCIVideophoneEngine sharedEngine] signMailGreetingPreference];
		NSString *detailText = [self GreetingTypeStringFromSCIType:[NSNumber numberWithUnsignedInteger:greetingType]];
		cell.detailTextLabel.text = detailText ? Localize(detailText) : @"";
		return cell;
    }
    if ([row.rowName isEqualToString:@"networkSpeedRow"]) {
        static NSString *CellIdentifier = @"NetworkSpeedCell";
        ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
        }
		
		cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
		cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		cell.selectionStyle = UITableViewCellSelectionStyleDefault;
        cell.textLabel.text = Localize(@"Network Speed");
        NSInteger targetBitRateSetting = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRate"];
        if (targetBitRateSetting == 0)
            cell.detailTextLabel.text = Localize(@"Auto");
        else
            cell.detailTextLabel.text = [NSString stringWithFormat:@"%ld Kbps", (long)targetBitRateSetting / 10];
        return cell;
    }
	if ([row.rowName isEqualToString:@"networkSpeedReadOnlyRow"]) {
		static NSString *CellIdentifier = @"NetworkSpeedCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
		}
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		cell.textLabel.text = Localize(@"Network Speed");
		cell.accessoryType = UITableViewCellAccessoryNone;
		cell.editingAccessoryType = UITableViewCellAccessoryNone;

		NSInteger maxSend = [SCIVideophoneEngine sharedEngine].maximumSendSpeed;
		NSInteger maxRecv = [SCIVideophoneEngine sharedEngine].maximumRecvSpeed;
		cell.detailTextLabel.text = [NSString stringWithFormat:@"%ld / %ld Kbps", (long)maxSend / 1000, (long)maxRecv / 1000];
		
		return cell;
	}
    if ([row.rowName isEqualToString:@"tunnelingRow"]) {
		static NSString *CellIdentifier = @"tunnelingSwitchCell";
        SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (cell == nil) {
            cell = [[SwitchCell alloc] initWithReuseIdentifier:CellIdentifier];
            [cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
            tunnelingSwitch = cell.switchControl;
        }
        cell.textLabel.text = Localize(@"Tunneling Enabled");
        cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] tunnelingEnabled];
        return cell;
    }
	if ([row.rowName isEqualToString:@"cellularVideoQualityRow"]) {
		static NSString *CellIdentifier = @"MobileQualityCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
		}
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		cell.textLabel.text = Localize(@"Cellular Video Quality");
		cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
		cell.editingAccessoryType = UITableViewCellAccessoryNone;
		cell.detailTextLabel.text = Localize(@"Good");
		
		NSInteger targetBitRateMobile = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRateMobile"];
		if (targetBitRateMobile > 2560) {
			cell.detailTextLabel.text = Localize(@"Better");
		}
		if (targetBitRateMobile > 5120) {
			cell.detailTextLabel.text = Localize(@"Best");
		}
		return cell;
	}
	if ([row.rowName isEqualToString:@"helpRow"]) {
		static NSString *CellIdentifier = @"helpCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
		}

		cell.textLabel.text = Localize(@"Call Customer Care");
		cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
		
		return cell;
	}
#ifdef stiENABLE_VP_REMOTE
    if ([row.rowName isEqualToString:@"nvpIpAddressRow"]) {
		TextInputTableViewCell *cell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
		cell.textField.delegate = self;
		cell.textField.textAlignment = NSTextAlignmentLeft;
		cell.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
		cell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
		cell.textField.enabled = YES;
        
        cell.textField.keyboardType = UIKeyboardTypeNumbersAndPunctuation;//UIKeyboardTypeDefault;
        cell.textField.autocapitalizationType = UITextAutocapitalizationTypeWords;
        cell.textLabel.text = Localize(@"nVP IP Address");
        self.nvpIpAddressTextField = cell.textField;
        // might crash in low memory
        //addressCell.textField.placeholder = Localize(@"required");
        NSString *ip = [[NSUserDefaults standardUserDefaults] stringForKey:@"AnswerOnVPKey"];
        cell.textField.text = ip;
        return cell;
    }
#endif
	static NSString *emptyId = @"EmptyCell";
	ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:emptyId];
	if (cell == nil) {
		cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:emptyId];
	}
	return cell;
}

- (IBAction)changeGreetingType:(id)sender {
	Alert(Localize(@"Custom Greeting"), Localize(@"show change greeting type view."));
}


- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
#ifdef stiENABLE_VP_REMOTE
    NSDictionary *dictionary = [settingsSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    NSString *row = [array objectAtIndex:indexPath.row];
    
    if ([row isEqualToString:@"nvpIpAddressRow"]) {
        return YES;
    }
#endif
    
	return NO;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

#pragma mark - Helpers

- (NSString *)GreetingTypeStringFromSCIType:(NSNumber *)greetingType
{
	NSUInteger signMailGreetingType = [greetingType unsignedIntegerValue];
	switch (signMailGreetingType) {
		case SCISignMailGreetingTypeDefault: return @"Sorenson";	break;
		case SCISignMailGreetingTypePersonalVideoAndText: return Localize(@"Personal");	break;
		case SCISignMailGreetingTypePersonalVideoOnly: return Localize(@"Personal");	break;
		case SCISignMailGreetingTypePersonalTextOnly: return Localize(@"Text Only");	break;
		case SCISignMailGreetingTypeNone: return Localize(@"None");	break;
		default: return @"Sorenson";	break;
	}
}

#pragma mark - Application Sharing Delegate


- (void)didSelectApplicationSharing:(enum SharedApplications)share :(UIView *)sourceView {
	NSString *message = @"";
	switch (share) {
		case SharedApplicationsWavelloApplication:
			message = Localize(@"settings.main.shared.app.wavello");
			break;
		case SharedApplicationsNtouchApplication:
			message = Localize(@"settings.main.shared.app.ntouch");
			break;
	}

	UIActivityViewController *activityView = [[UIActivityViewController alloc] initWithActivityItems:@[message] applicationActivities:nil];
	activityView.popoverPresentationController.sourceView = sourceView;
	activityView.popoverPresentationController.sourceRect = sourceView.bounds;
	[self presentViewController:activityView animated:YES completion:nil];
}

#pragma mark - Table view delegate

extern int g_H264Level;

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[self.tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    NSDictionary *dictionary = [self.settingsSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    SettingItem *row = [array objectAtIndex:indexPath.row];
	
	if ([row.rowName isEqualToString:@"pulseManageDevicesRow"]) {
		PulseViewController *controller = [[PulseViewController alloc] initWithStyle:UITableViewStyleGrouped];
		[self.navigationController pushViewController:controller animated:YES];
	}
    if ([row.rowName isEqualToString:@"ringsBeforeGreetingRow"]) {
        TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
		
        NSUInteger ringsCount = [[SCIVideophoneEngine sharedEngine] ringsBeforeGreeting];
		NSString *ring = Localize(@"Ring");
		NSString *rings = Localize(@"Rings");
		
		NSString *currentValue = [NSString stringWithFormat: @"%ld %@", (long)ringsCount, (ringsCount == 1 ? ring : rings)];
        NSMutableDictionary *greetingEditingItem = [[NSMutableDictionary alloc] init];
        [greetingEditingItem setValue:Localize(@"Play SignMail greeting after") forKey:kTypeListTitleKey];
		
        NSMutableArray *ringCountArray = [[NSMutableArray alloc] init];
        for (int i = 5; i < 16; i++) {
			NSString *ringCountStr = [NSString stringWithFormat: @"%ld %@", (long)i, (i == 1 ? ring : rings)];
            [ringCountArray addObject: ringCountStr];
        }
        [greetingEditingItem setValue:currentValue forKey:kTypeListTextKey];
        controller.types = ringCountArray;
        controller.editingItem = greetingEditingItem;
		controller.selectionBlock = ^(NSMutableDictionary *greetingEditingItem) {
			
			NSString *ringCount = [greetingEditingItem valueForKey:kTypeListTextKey];
			NSUInteger newRingCount = [ringCount integerValue];
			[[SCIVideophoneEngine sharedEngine] setRingsBeforeGreetingPrimitive:newRingCount];
			
			[self.tableView reloadData];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"ringcount_changed" }];
		};
		
        [self.navigationController pushViewController:controller animated:YES];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"signmail_greeting_after" }];
        return;
    }
    if ([row.rowName isEqualToString:@"signMailGreetingTypeRow"]) {
        SignMailGreetingPlayViewController *signMailGreetingView = [[SignMailGreetingPlayViewController alloc] init];
        [self.navigationController pushViewController:signMailGreetingView animated:YES];
        return;
    }
    if ([row.rowName isEqualToString:@"personalInfoRow"] || [row.rowName isEqualToString:@"header"]) {
        PersonalInfoViewController *personalInfo = [[PersonalInfoViewController alloc] initWithStyle:UITableViewStyleGrouped];
        [self.navigationController pushViewController:personalInfo animated:YES];
        return;
    }
    if ([row.rowName isEqualToString:@"setupEmergencyRow"]) {
        appDelegate.emergencyInfoViewController = [[EmergencyInfoViewController alloc] initWithStyle:UITableViewStyleGrouped];
        [self.navigationController pushViewController:appDelegate.emergencyInfoViewController animated:YES];
        return;
    }
    if ([row.rowName isEqualToString:@"importContactsRow"]) {
        ImportContactsViewController *controller = [[ImportContactsViewController alloc] initWithStyle:UITableViewStyleGrouped];
        [self.navigationController pushViewController:controller animated:YES];
        return;
    }
    if ([row.rowName isEqualToString:@"shareTextRow"]) {
        ShareSavedTextViewController *shareTextView = [[ShareSavedTextViewController alloc] initWithStyle:UITableViewStyleGrouped];
        shareTextView.modalPresentationStyle = UIModalPresentationFullScreen;
        [self.navigationController pushViewController:shareTextView animated:YES];
        return;
    }
	if ([row.rowName isEqualToString:@"colorTheme"]) {
		TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
		NSMutableDictionary *themeEditingItem = [[NSMutableDictionary alloc] init];
		[themeEditingItem setValue:Localize(@"Color Theme") forKey:kTypeListTitleKey];
		[themeEditingItem setValue:Localize(Theme.name) forKey:kTypeListTextKey];
		controller.types = Theme.themeNames;
		controller.editingItem = themeEditingItem;
		controller.selectionBlock = ^(NSMutableDictionary *themeEditingItem) {
			NSString *themeName = [themeEditingItem valueForKey:kTypeListTextKey];
			NSInteger themeIndex = [Theme.themeNames indexOfObject:themeName];
			NSString *identifier = Theme.themeIdentifiers[themeIndex];
			Theme.currentIdentifier = identifier;
			[self.tableView reloadData];
		};
		
		[self.navigationController pushViewController:controller animated:YES];
		return;
	}
	if ([row.rowName isEqualToString:@"blockedNumbers"]) {
		BlockedViewController *blockedViewController = [[BlockedViewController alloc] initWithStyle:UITableViewStyleGrouped];
		[self.navigationController pushViewController:blockedViewController animated:YES];
	}

    if ([row.rowName isEqualToString:@"myPhoneSettingsRow"]) {
        [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
        if ([SCIAccountManager sharedManager].state.integerValue != CSSignedIn) {
            AlertWithTitleAndMessage(Localize(@"myPhone"), Localize(@"settings.main.myPhone.notloggedin"));
            return;
        }
		if (self.accountManager.eulaAgreementState != SCIProviderAgreementStateAccepted) {
			AlertWithTitleAndMessage(Localize(@"myPhone"), Localize(@"settings.main.myPhone.must.agreement"));
			return;
		}
        if([[SCIVideophoneEngine sharedEngine] isInRingGroup]) {
            MyPhoneEditViewControllerNew *myPhoneViewController = [[MyPhoneEditViewControllerNew alloc] init];
            [self.navigationController pushViewController:myPhoneViewController animated:YES];
        }
        else {
            MyPhoneSetupViewController *setupViewController = [[MyPhoneSetupViewController alloc] initWithStyle:UITableViewStyleGrouped];
            [self.navigationController pushViewController:setupViewController animated:YES];
        }
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"myphone" }];
        return;
    }
    if ([row.rowName isEqualToString:@"networkSpeedRow"]) {
		TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
        NSMutableDictionary *networkEditingItem = [[NSMutableDictionary alloc] init];
        [networkEditingItem setValue:Localize(@"Network Speed") forKey:kTypeListTitleKey];
        NSString *currentValue = Localize(@"Auto");
        NSInteger targetBitRate = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRate"];
        if (targetBitRate) {
            currentValue = [NSString stringWithFormat:@"%ld Kbps", (long)targetBitRate / 10];
        }
        [networkEditingItem setValue:currentValue forKey:kTypeListTextKey];
        
        if (g_H264Level > estiH264_LEVEL_1_2) {
            controller.types = [NSArray arrayWithObjects:
								Localize(@"Auto"),
                                @"128 Kbps",
                                @"192 Kbps",
                                @"256 Kbps",
                                @"320 Kbps",
                                @"384 Kbps",
                                @"448 Kbps",
                                @"512 Kbps",
                                @"576 Kbps",
                                @"640 Kbps",
                                @"704 Kbps",
                                @"768 Kbps",
                                @"832 Kbps",
                                @"896 Kbps",
                                @"960 Kbps",
                                @"1024 Kbps",
                                @"1152 Kbps",
                                @"1280 Kbps",
                                @"1408 Kbps",
                                @"1536 Kbps",
                                @"2048 Kbps",
                                nil];
        }
        else {
            controller.types = [NSArray arrayWithObjects:
								Localize(@"Auto"),
                                @"128 Kbps",
                                @"192 Kbps",
                                @"256 Kbps",
                                nil];
        }
        
        controller.editingItem = networkEditingItem;
		controller.selectionBlock = ^(NSMutableDictionary *editingItem) {
			NSString *speedValue = [editingItem valueForKey:kTypeListTextKey];
			int newTargetBitrateSetting = [speedValue intValue];
			if (newTargetBitrateSetting > 0 && newTargetBitrateSetting < 192 && [[SCIVideophoneEngine sharedEngine] audioEnabled]) {
				Alert(Localize(@"Network Speed too low for Audio"), Localize(@"Audio will be disabled."));
				[SCIAccountManager sharedManager].audioAndVCOEnabled = NO; // Rule: When bitrate < 192 disable Audio
			}
			
			[appDelegate setBitRateSetting:newTargetBitrateSetting];
			[self.tableView reloadData];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"network_speeds_changed" }];
		};
		
        [self.navigationController pushViewController:controller animated:YES];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"network_speed" }];
        return;
    }
	if ([row.rowName isEqualToString:@"cellularVideoQualityRow"]) {
		TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
		
		NSMutableDictionary *cellularVideoQualityEditingItem = [[NSMutableDictionary alloc] init];
		[cellularVideoQualityEditingItem setValue:Localize(@"Cellular Video Quality") forKey:kTypeListTitleKey];
		
		NSInteger targetBitRateMobile = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRateMobile"];
		NSString *currentValue = Localize(@"Good (least data)");
		if (targetBitRateMobile > 2560) {
			currentValue = Localize(@"Better (more data)");
		}
		if (targetBitRateMobile > 5120) {
			currentValue = Localize(@"Best (most data)");
		}
		[cellularVideoQualityEditingItem setValue:currentValue forKey:kTypeListTextKey];
		
		controller.types = [NSArray arrayWithObjects:
							Localize(@"Good (least data)"),
							Localize(@"Better (more data)"),
							Localize(@"Best (most data)"),
							nil];
		
		controller.editingItem = cellularVideoQualityEditingItem;
		controller.selectionBlock = ^(NSMutableDictionary *editingItem) {
			NSString *speedValue = [editingItem valueForKey:kTypeListTextKey];
			if ([speedValue hasPrefix:Localize(@"Good")]) {
				[appDelegate setMobileBitRateSetting:256];
			}
			else if ([speedValue hasPrefix:Localize(@"Better")]) {
				[appDelegate setMobileBitRateSetting:512];
			}
			else if ([speedValue hasPrefix:Localize(@"Best")]) {
				[appDelegate setMobileBitRateSetting:1024];
			}
		
			[self.tableView reloadData];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"cellular_quality_changed" }];
		};
		
		[self.navigationController pushViewController:controller animated:YES];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"cellular_video_quality" }];
		return;
	}
	if ([row.rowName isEqualToString:@"helpRow"]) {
		HelpViewController *helpViewController = [[HelpViewController alloc] initWithStyle:UITableViewStyleGrouped];
		[self.navigationController pushViewController:helpViewController animated:YES];
	}
}

#pragma mark - Notifications

- (void)notifyUserAccountInfoUpdated:(NSNotification *)note { //SCINotificationUserAccountInfoUpdated
	self.profileImage = nil;
	[self loadTableData];
	[self.tableView reloadData];
}

- (void)notifyRingGroupModeChanged:(NSNotification *)note { //SCINotificationRingGroupModeChanged
	[self.tableView reloadData];
}

- (void)notifyPropertyChanged:(NSNotification *)note // SCINotificationPropertyChanged
{
	const char *propertyName = [[note.userInfo objectForKey:SCINotificationKeyName] UTF8String];
	static NSArray *namesOfPropertiesToObserve =
	@[SCIPropertyNameRingGroupEnabled,
		SCIPropertyNameRingGroupDisplayMode,
		SCIPropertyNameContactImagesEnabled,
		SCIPropertyNamePersonalGreetingEnabled,
		SCIPropertyNameBlockCallerIDEnabled,	// Block Caller ID switch
		SCIPropertyNameAutoRejectIncoming,		// Call Waiting switch
		SCIPropertyNameDoNotDisturbEnabled,		// Do Not Disturb switch
		SCIPropertyNameDisableAudio,			// Audio Enabled switch
		SCIPropertyNameBlockCallerID,			// Block Caller ID switch
		SCIPropertyNameBlockAnonymousCallers,	// Don't Accept Anonymous Calls switch
		SCIPropertyNameVCOPreference,			// Enable Audio/VCO switch
		SCIPropertyNameVCOActive,				// Use VCO for VRS switch
		SCIPropertyNameSpanishFeatures,			// Spanish Features switch
		SCIPropertyNameSecureCallMode];			// Encryption switch
	
	for (NSString *property in namesOfPropertiesToObserve) {
		if (strcmp(propertyName, [property UTF8String]) == 0) {
			SCILog(@"Reloaing Settings, %s property has changed.", propertyName);
			[self loadTableData];
			[self.tableView reloadData];
			return;
		}
	}
}

- (void)notifyAutoSpeedModeChanged:(NSNotification *)note //SCINotificationAutoSpeedModeSettingChanged
{
	[self loadTableData];
	[self.tableView reloadData];
}

- (void)notifyMaxSpeedsChanged:(NSNotification *)note //SCINotificationMaxSpeedsChanged
{
	NSInteger maxSend = [SCIVideophoneEngine sharedEngine].maximumSendSpeed;
	[appDelegate setBitRateSetting:(long)maxSend / 1000];
	
	[self loadTableData];
	[self.tableView reloadData];
}

- (void)notifyRingsBeforeGreetingChanged:(NSNotification *)note //SCINotificationRingsBeforeGreetingChanged
{
	[self.tableView reloadData];
}

- (void)notifyConnecting:(NSNotification *)note // SCINotificationConnecting
{
	[self checkCoreOffline];
}

# pragma mark - SearchBar Delegate

- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
	if (@available(iOS 11.0, *)) {
		[self.tableView setContentOffset: CGPointMake(0, -self.tableView.adjustedContentInset.top) animated: YES];
	} else {
		[self.tableView setContentOffset: CGPointMake(0, -self.tableView.contentInset.top) animated: YES];
	}
	[self loadTableData];
	[self.tableView reloadData];
}

- (BOOL)searchBar:(UISearchBar *)searchBar shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text {
	return ([searchBar.text length] + [text length] - range.length > 64) ? NO : YES;
}

- (NSArray *)performSearch:(NSArray *)sectionsAndRows withString:(NSString *)searchText
{
	if (sectionsAndRows.count) {
		NSMutableArray *filteredArray = [[NSMutableArray alloc] init];
		for (NSDictionary *section in sectionsAndRows) {
			NSArray *rows = [section objectForKey:@"rows"];
			NSPredicate *bPredicate = [NSPredicate predicateWithFormat:@"SELF.searchWords contains[cd] %@", searchText];
			NSArray *foundRows = [rows filteredArrayUsingPredicate:bPredicate];
			
			if (foundRows.count) {
				[filteredArray addObjectsFromArray:foundRows];
			}
		}
		return [[NSArray alloc] initWithObjects: @{ @"sectionNameKey" : @"searchSection", @"rows": filteredArray}, nil];
	}
	else {
		return sectionsAndRows;
	}
}

@end
