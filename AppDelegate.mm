//
//  AppDelegate.m
//  Video Center
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2016 Sorenson Communications, Inc. -- All rights reserved
//

#import "AppDelegate.h"
#import "Alert.h"
#import "Utilities.h"
#import "ContactUtilities.h"
#import "MyRumble.h"
#import "SettingsViewController.h"
#import "SCIContactManager.h"
#import "SCICallListManager.h"
#import "SCIMessageDownloadURLItem.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIRingGroupInfo.h"
#import "SCIItemId.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import "UIDevice+Resolutions.h"
#import "SCIEmergencyAddress.h"
#import "SCIDynamicProviderAgreementStatus.h"
#import "SCIDynamicProviderAgreementContent.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCIStaticProviderAgreementStatus.h"
#import "SCICoreVersion.h"
#import "NSBundle+SCIAdditions.h"
#import "SCIProviderAgreementType.h"
#import "SCIAccountManager.h"
#import "SCIDefaults.h"
#import "MBProgressHUD.h"
#import "PhotoManager.h"
#import <CoreSpotlight/CoreSpotlight.h>
#import <CallKit/CallKit.h>
#import <Intents/Intents.h>
#import <SystemConfiguration/CaptiveNetwork.h>
#import "SCISettingItem.h"
#import "UIViewController+Top.h"
#include "stiSVX.h"
#include "cmPropertyNameDef.h"
#include <AudioToolbox/AudioToolbox.h>
#import <FirebaseCore/FIRApp.h>
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>
#import "SCINotifications.h"
#import "ntouch-Swift.h"
#import "stiOSMacOSXNet.h"
#import "stiTools.h"
#include "NSString+LanguageCode.h"
#import <Pendo/PendoManager.h>
#import <BackgroundTasks/BackgroundTasks.h>
#import "LaunchDarklyConfigurator.h"

const int kCellularBitRate = 2560;
const int kWifiBitRate = 5120;
const int kWifiVgaBitRate = 10240;
const int kWifi720pBitRate = 20480;

// defaults for older hardware
int g_H264Profile = estiH264_BASELINE;
int g_H264Level = estiH264_LEVEL_1_2;

bool g_NetworkConnected = true;
bool startupLogged = false;

std::string ipv4Address = "";
std::string ipv6Address = "";
bool backgroundedSinceNetworkChange = false;


@interface AppDelegate () <SCINetworkMonitorDelegate>
@end

@implementation AppDelegate  

@synthesize window;
@synthesize networkingCount;
@synthesize showHUD;
@synthesize updateRequired;
@synthesize phonebookViewController;
@synthesize myRumble;
@synthesize myRumblePatterns;
@synthesize emergencyInfoViewController;
@synthesize settingsViewController;
@synthesize currentOrientation;
@synthesize lastDialString;

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

-(void) redirectSCILogToDocumentFolder {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *fileName =[NSString stringWithFormat:@"%@.log",[NSDate date]];
	NSString *logFilePath = [documentsDirectory stringByAppendingPathComponent:fileName];
	freopen([logFilePath cStringUsingEncoding:NSASCIIStringEncoding],"a+",stderr);
}

- (NSDictionary *)myRumblePatterns {
	if (!myRumblePatterns) {
		NSString *path = [[NSBundle mainBundle] pathForResource:@"myRumble" ofType:@"plist"];
		myRumblePatterns = [[NSDictionary alloc] initWithContentsOfFile:path];
	}
	return myRumblePatterns;
}

- (MyRumble *)myRumble {
    if (!myRumble)
        myRumble = [[MyRumble alloc] init];
    return myRumble;
}

- (void)presentUpdateRequired {
	updateRequired = YES;
	[[SCIAccountManager sharedManager] cancelLoginTimeout];
	[[SCIVideophoneEngine sharedEngine] setAllowIncomingCallsMode:NO];
	[[SCIVideophoneEngine sharedEngine] logout];
	UpdateRequiredViewController *controller = [[UpdateRequiredViewController alloc] initWithStyle:UITableViewStyleGrouped];
	UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:controller];
	
	if (iPadB)
		navController.modalPresentationStyle = UIModalPresentationFormSheet;
	
	[self.tabBarController.selectedViewController presentViewController:navController animated:YES completion:nil];
}



extern EstiBool g_bIsATT3G;

- (NSUInteger) getTargetBitRateForStatus:(SCINetworkType)netType {
	g_bIsATT3G = estiFALSE;
	NSUInteger targetBitRate = 0;
	
	if (SCIAutoSpeedModeLegacy == [SCIVideophoneEngine sharedEngine].networkAutoSpeedMode || SCIAutoSpeedModeLimited == [[SCIVideophoneEngine sharedEngine]networkAutoSpeedMode]) {
		targetBitRate = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRate"];
	}
	
	if (!targetBitRate) {
		targetBitRate = kCellularBitRate;
		if (SCINetworkTypeCellular == netType ||
			SCINetworkTypeOther == netType) {
			targetBitRate = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRateMobile"];
			if (!targetBitRate) {
				targetBitRate = kCellularBitRate;
			}
			CTTelephonyNetworkInfo *netinfo = [[CTTelephonyNetworkInfo alloc] init];
			CTCarrier *carrier = [netinfo subscriberCellularProvider];
			if (carrier && ([carrier.carrierName isEqualToString:@"AT&T"] ||
							[carrier.carrierName isEqualToString:@"Sprint"])) {
				//g_bIsATT3G = estiTRUE;
			}
		}
		else if (SCINetworkTypeWifi == netType ||
				 SCINetworkTypeWired == netType) {
			if (g_H264Level >= estiH264_LEVEL_3_1) {
				targetBitRate = kWifi720pBitRate;
			}
			else if (g_H264Level >= estiH264_LEVEL_2_1) {
				targetBitRate = kWifiVgaBitRate;
			}
			else {
				targetBitRate = kWifiBitRate;
			}
		}
	}
	else {
		if (SCINetworkTypeCellular == netType) {
			targetBitRate = [[NSUserDefaults standardUserDefaults] integerForKey:@"targetBitRateMobile"];
			if (!targetBitRate) {
				targetBitRate = kCellularBitRate;
			}
			//g_bIsATT3G = estiTRUE;
		}
		else {
			//g_bIsATT3G = estiFALSE;
		}
	}
	
	// Crashlytics logging
	if (netType == SCINetworkTypeWifi) {
		[[FIRCrashlytics crashlytics] setCustomValue:@"WiFi" forKey:@"NetworkType"];
	}
	else if (netType == SCINetworkTypeCellular) {
		[[FIRCrashlytics crashlytics] setCustomValue:@"Cellular" forKey:@"NetworkType"];
	}
	else if (netType == SCINetworkTypeWired) {
		[[FIRCrashlytics crashlytics] setCustomValue:@"Wired" forKey:@"NetworkType"];
	}
	else if (netType == SCINetworkTypeOther) {
		[[FIRCrashlytics crashlytics] setCustomValue:@"Wired" forKey:@"NetworkType"];
	}
	else {
		[[FIRCrashlytics crashlytics] setCustomValue:@"NotReachable" forKey:@"NetworkType"];
	}
	
	return targetBitRate;
}

- (void) setBitRateForStatus: (SCINetworkType)netType {
	NSUInteger targetBitRate = [self getTargetBitRateForStatus:netType];
	targetBitRate *= 100; // globals are in 10s of KB/s
	[[SCIVideophoneEngine sharedEngine] setMaximumRecvSpeed:targetBitRate];
	[[SCIVideophoneEngine sharedEngine] setMaximumSendSpeed:targetBitRate];
	[[SCIVideophoneEngine sharedEngine] setMaxAutoSpeeds:targetBitRate sendSpeed:targetBitRate];
}

- (void) setBitRateSetting: (NSUInteger)bitRateInKbps {
	SCILog(@"Start - bitRateInKbps: %d", bitRateInKbps);
	[[NSUserDefaults standardUserDefaults] setInteger:bitRateInKbps*10 forKey:@"targetBitRate"];
	[[NSUserDefaults standardUserDefaults] synchronize];
	[self setBitRateForStatus:self.networkMonitor.bestInterfaceType];
}

- (void) setMobileBitRateSetting: (NSUInteger)bitRateInKbps {
	SCILog(@"Start - bitRateInKbps: %d", bitRateInKbps);
	[[NSUserDefaults standardUserDefaults] setInteger:bitRateInKbps*10 forKey:@"targetBitRateMobile"];
	[[NSUserDefaults standardUserDefaults] synchronize];
	[self setBitRateForStatus:self.networkMonitor.bestInterfaceType];
}

-(void) setProfileLevel {
	SCIDeviceVersion *version = [[UIDevice currentDevice] version];
	g_H264Profile = estiH264_MAIN;

	if ([version isiPhone5]) {
		g_H264Level = estiH264_LEVEL_2_2;
	} else {
		// Any newer devices will likely support a higher level
		g_H264Level = estiH264_LEVEL_3_1;
	}
	
	[[SCIVideophoneEngine sharedEngine] setCodecCapabilities:true h264Level:g_H264Level];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	[FIRApp configure]; // Start Crashlytics
	// TODO: We should only be loading the bare minimum here.
	// Maintain the main functionality of the app : Make / Receive calls.
	// Do Not Load SignMail manager or video center managers or any other object that is not yet needed.
	
	self.networkMonitor = [SCINetworkMonitor new];
	self.networkMonitor.delegate = self;
	
	[Theme applyTheme];
	
	self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"Main-Universal" bundle:nil];
	
	self.topLevelUIController = [[TopLevelUIController alloc] init];
	self.startTopLevelUIUtilities = [[StartTopLevelUIUtilities alloc] initWithTopLevelUIController:self.topLevelUIController window:window storyboard:storyboard];
	[self.startTopLevelUIUtilities execute];
	self.tabBarController = self.topLevelUIController.tabBarController;
	
	NSLog(@"didFinishLaunchingWithOptions launchOptions: %@", [launchOptions description]);
	self.launchURL = launchOptions[UIApplicationLaunchOptionsURLKey];
	self.launchOptions = launchOptions;
	self.launchDateTime = [NSDate date];
	
	showHUD = NO;
    
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(orientationChanged:)
												 name: UIDeviceOrientationDidChangeNotification object: nil];

	// Register for VoIP notifications
	[self registerForPushKitNotifications];
	
	// Register for push notifications
	[SCINotificationManager.shared registerInteractiveNotifications];
	[UIApplication.sharedApplication registerForRemoteNotifications];
	
	NSDictionary *coreOptions = [SCILaunchOptions processPropertiesWithLaunchOptions:launchOptions];
	[[LaunchDarklyConfigurator shared] setupLDClient:coreOptions];
	[SCIAccountManager sharedManager].coreOptions = coreOptions;

#ifdef stiDEBUG
	[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"appRestarted"]];
	
	UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
	notification.title = @"Debug: ntouch has restarted";
	notification.sound = UNNotificationSound.defaultSound;
	
	UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"appRestarted" content:notification trigger:nil];
	[UNUserNotificationCenter.currentNotificationCenter
	 addNotificationRequest:request
	 withCompletionHandler:^(NSError * _Nullable error) {
		 if (error != nil) {
			 NSLog(@"Error posting notification: %@", error);
		 }
	 }];
#endif
	
//    if (application.isProtectedDataAvailable) {
		dispatch_once(&protectedDataOnceToken, ^{
				[self initializeProtectedDataItems];
		});
//	}
	
	self.callWindowController = [[CallWindowController alloc] init];
	SCICallController.shared.delegate = self.callWindowController;

	if (@available(iOS 13.0, *)) {
		// Specifying nil to use a background queue: https://developer.apple.com/documentation/backgroundtasks/bgtaskscheduler/register(fortaskwithidentifier:using:launchhandler:)?language=objc
		[[BGTaskScheduler sharedScheduler] registerForTaskWithIdentifier:@"com.sorenson.ntouch.background-heartbeat" usingQueue:nil launchHandler:^(__kindof BGTask * _Nonnull task) {
			[SCIVideophoneEngine.sharedEngine heartbeat];
			[NSThread sleepForTimeInterval:3.0f];
			[self handleBackgroundProcessing:task];
		}];
	}
	return YES;
}

- (void)handleBackgroundProcessing:(BGTask *)task API_AVAILABLE(ios(13.0)) {
	[self scheduleBackgroundProcessing];
	NSOperation *operation = [[NSOperation alloc] init];
	task.expirationHandler = ^{
		[operation cancel];
	};
	__weak NSOperation *weakOperation = operation;
	operation.completionBlock = ^{
		[task setTaskCompletedWithSuccess:!weakOperation.isCancelled];
	};
	NSOperationQueue *queue = [[NSOperationQueue alloc] init];
	[queue addOperation:operation];
}

- (void)scheduleBackgroundProcessing API_AVAILABLE(ios(13.0)) {
	LaunchDarklyConfigurator *ld = [LaunchDarklyConfigurator shared];
	BOOL periodicHeartbeatEnabled = ld.periodicBackgroundHeartbeat;
	int intervalMinutes = ld.periodicBackgroundInterval;
	if (periodicHeartbeatEnabled) {
		BGProcessingTaskRequest *request = [[BGProcessingTaskRequest alloc]initWithIdentifier:@"com.sorenson.ntouch.background-heartbeat"];
		request.earliestBeginDate = [[NSDate alloc] initWithTimeIntervalSinceNow:intervalMinutes * 60];
		request.requiresNetworkConnectivity = YES;
		NSError *error = nil;
		BOOL success = [[BGTaskScheduler sharedScheduler] submitTaskRequest:request error:&error];
		if (success) {
			[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"AppEvent=SuccessfullySubmittedBackgroundProcessingTask"];
		} else {
			[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:[NSString stringWithFormat:@"AppEvent=UnsuccessfulySubmittedBackgroundProcessingTaskWithError: %@", error.localizedDescription]];
		}
	}
}

- (void)applicationProtectedDataDidBecomeAvailable:(UIApplication *)application {
	SCILog(@"");
	[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=iOSProtectedDataDidBecomeAvailable"];
	dispatch_once(&protectedDataOnceToken, ^{
			[self initializeProtectedDataItems];
	});
}

-(void)applicationProtectedDataWillBecomeUnavailable:(UIApplication *)application {
	SCILog(@"");
	[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=iOSProtectedDataWillBecomeUnavailable"];
}

- (void)initializeProtectedDataItems {
    [AnalyticsManager.shared trackUsage:AnalyticsEventInitialized properties:nil];
    
    // Restore previously selected tab
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSInteger selectedIndex = [defaults integerForKey:kSavedTabSelectedIndex];
    NSInteger settingsIndex = [self.tabBarController.viewControllers indexOfObject:settingsViewController.navigationController];
    
    if (selectedIndex < self.tabBarController.viewControllers.count && selectedIndex != settingsIndex) {
        self.tabBarController.selectedIndex = selectedIndex;
    }
    
	// Initialize VPE and managers
	[[SCIAccountManager sharedManager] open];
	
	[self networkMonitorDidChangePath:self.networkMonitor];
	
	[self updateVideoCenterTab];

    // Handle Remote Notifications
    if (self.launchOptions[UIApplicationLaunchOptionsRemoteNotificationKey]) {
        
        NSDictionary *notification = self.launchOptions[UIApplicationLaunchOptionsRemoteNotificationKey];
        NSDictionary *apsDict = [notification valueForKey:@"aps"];
        NSString *category = [apsDict valueForKey:@"category"];
        
        if (!category) {
            category = [notification valueForKey:@"uid"];
        }
        
        // Log notification was used to launch app.  // MOVE THIS TO HANDLE LOCAL IF WE DONT CARE IF APP WAS LAUNCHED
        if ([category isEqualToString:@"MISSED_CALL_CATEGORY"]) {
            [[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=MissedCallPushNotificationSelected"];
        }
        else if ([category isEqualToString:@"SIGNMAIL_CATEGORY"]) {
            [[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=SignMailPushNotificationSelected"];
            [[SCIAccountManager sharedManager] addCoreEventWithType:coreEventNewVideo];
        }
    }
    
	[self setBitRateForStatus:self.networkMonitor.bestInterfaceType];
	
	// Setup WatchOS Session delegate and activate session
	if ([WCSession isSupported]) {
		WCSession* session = [WCSession defaultSession];
		session.delegate = self;
		[session activateSession];
		
		NSString *watchPairedKey = @"AppleWatchPaired";
		NSInteger newValue = session.paired ? 1 : 0;
		NSInteger oldValue = [[SCIPropertyManager sharedManager] intValueForPropertyNamed:watchPairedKey inStorageLocation:SCIPropertyStorageLocationPersistent withDefault:0];
		
		if (newValue != oldValue) {
			[[SCIPropertyManager sharedManager] setIntValue:newValue forPropertyNamed:watchPairedKey inStorageLocation:SCIPropertyStorageLocationPersistent];
			[[SCIPropertyManager sharedManager] sendValueForPropertyNamed:watchPairedKey withScope:SCIPropertyScopePhone];
		}
	}
	
	// Set AudioSession Category, required for PIP playback
	NSError *error = nil;
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord
									 withOptions:AVAudioSessionCategoryOptionAllowBluetooth
										   error:&error];
	[[AVAudioSession sharedInstance] setMode:AVAudioSessionModeVideoChat error:&error];
	
	if (error) {
		[[FIRCrashlytics crashlytics] recordError:error];
		SCILog(@"Error setting AudioSession category: %@", [error localizedDescription]);
	}
}

- (void)applicationWillResignActive:(UIApplication *)application {

}

#pragma mark - QuickAction Framework

- (void)installDynamicShortcutItems {

	if ([[UIApplication sharedApplication] respondsToSelector:(@selector(setShortcutItems:))])
	{
		NSMutableArray *shortcutItems = [[NSMutableArray alloc] init];
		
		// Create a "+ Create Contact" shortcut item.
		UIApplicationShortcutIcon *icon = [UIApplicationShortcutIcon iconWithType:UIApplicationShortcutIconTypeAdd];
		UIMutableApplicationShortcutItem *item = [[UIMutableApplicationShortcutItem alloc]
												  initWithType:@"AddContactAction"
												  localizedTitle:@"Create New Contact"
												  localizedSubtitle:nil
												  icon:icon
												  userInfo:nil];
		[shortcutItems addObject:item];

		if ([SCIContactManager sharedManager].favoritesCount)
		{
			NSArray *favs = [SCIContactManager sharedManager].favorites;

			// Loop through favorites and create up to 3 ShortcutItems
			NSInteger favCount = 0;
			for (SCIContact *favorite in favs)
			{
				if (favCount == 3)
					break;
				else if ([favorite.name isEqualToString:@"Unavailable"])
					continue;

				// Get Phone Number.  This logic needs to move into SCIContact
				NSString *phoneNumber = nil;

				if (favorite.homeIsFavorite && favorite.homePhone.length) {
					phoneNumber = favorite.homePhone;
				}
				else if (favorite.workIsFavorite && favorite.workPhone.length) {
					phoneNumber = favorite.workPhone;
				}
				else if (favorite.cellIsFavorite && favorite.cellPhone.length) {
					phoneNumber = favorite.cellPhone;
				}
				
				if (!phoneNumber) {
					continue;
				}

				UIApplicationShortcutIcon *icon = [UIApplicationShortcutIcon iconWithTemplateImageName:@"FavoriteStarQuickAction.png"];
				UIMutableApplicationShortcutItem *favoriteItem = [[UIMutableApplicationShortcutItem alloc]
														  initWithType:@"SelectFavoriteAction"
														  localizedTitle:favorite.nameOrCompanyName
														  localizedSubtitle:FormatAsPhoneNumber(phoneNumber)
														  icon:icon
														  userInfo:@{ @"DialStringKey" : phoneNumber }];
				[shortcutItems addObject:favoriteItem];
				favCount++;
			}
		}

		// Add array of ShortcutItems to UIApplication
		[UIApplication sharedApplication].shortcutItems = shortcutItems;
	}
}

#pragma mark - PushKit Framework

- (void) registerForPushKitNotifications {
	SCILog(@"");
	PKPushRegistry *voipRegistry = [[PKPushRegistry alloc] initWithQueue:dispatch_get_main_queue()];
	voipRegistry.delegate = SCIENSController.shared;
	voipRegistry.desiredPushTypes = [NSSet setWithObject:PKPushTypeVoIP];
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
	
	if ([[url scheme] containsString:@"pendo"]) {
		[[PendoManager sharedManager] initWithUrl:url];
		return YES;
	}

	NSString* urlString = [url query];
	
    NSLog(@"url recieved: %@", url);
	NSLog(@"url string: %@", urlString);
    NSLog(@"query string: %@", [url query]);
    NSLog(@"host: %@", [url host]);
    NSLog(@"url path: %@", [url path]);
    NSDictionary *dict = [self parseQueryString:urlString];
    NSLog(@"query dict: %@", dict);
	
	NSString *numberToDial = [dict objectForKey:@"numberToDial"];
	
	// ntouch://?numberToDial=14085551212
	if(numberToDial && [url.scheme isEqualToString:@"ntouch"]) {
		[SCICallController.shared makeOutgoingCallTo:numberToDial dialSource:SCIDialSourceWebDial];
	}
	
	if ([url.scheme isEqualToString:@"ntouch"] &&
		[url.host isEqualToString:@"urlgen"] &&
		![url isEqual:self.launchURL]) {
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Failed to use URLgen"
																	   message:@"Please force quit the app before using URLgen"
																preferredStyle:UIAlertControllerStyleAlert];
		
		[alert addAction:[UIAlertAction actionWithTitle:@"Force Quit"
												  style:UIAlertActionStyleDefault
												handler:^(UIAlertAction * _Nonnull action) { exit(0); }]];
		
		[alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
												  style:UIAlertActionStyleCancel
												handler:nil]];

		[self.topViewController presentViewController:alert animated:YES completion:nil];
		return NO;
	}
	
    return YES;
}

- (NSDictionary *)parseQueryString:(NSString *)query {
    NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithCapacity:6];
    NSArray *pairs = [query componentsSeparatedByString:@"&"];
    
    for (NSString *pair in pairs) {
        NSArray *elements = [pair componentsSeparatedByString:@"="];
		if (elements.count >= 2) {
			NSString *key = [[elements objectAtIndex:0] stringByRemovingPercentEncoding];
			NSString *val = [[elements objectAtIndex:1] stringByRemovingPercentEncoding];
			[dict setObject:val forKey:key];
		}
    }
    return dict;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	[[SCIVideophoneEngine sharedEngine] heartbeat];
	[[SCIAccountManager sharedManager] remindMeLaterCheck];
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	NSLog(@"******** applicationDidEnterBackground");
	
	// Use UIBackgroundTask to send queued properties with timeout
	__block UIBackgroundTaskIdentifier propertySendTask = UIBackgroundTaskInvalid;
	propertySendTask = [application beginBackgroundTaskWithExpirationHandler: ^{
		dispatch_async(dispatch_get_main_queue(), ^{
			[application endBackgroundTask:propertySendTask];
			propertySendTask = UIBackgroundTaskInvalid;
		});
	}];
	
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0), ^{
		[[SCIVideophoneEngine sharedEngine] applicationDidEnterBackground];
		[SCIPropertyManager.sharedManager waitForSendWithTimeout:5];
		
		if (propertySendTask != UIBackgroundTaskInvalid) {
			[application endBackgroundTask:propertySendTask];
		}
	});
	
	[SCINotificationManager.shared updateAppBadge];
	backgroundedSinceNetworkChange = true;
	if (@available(iOS 13.0, *)) {
		[self scheduleBackgroundProcessing];
	}
}

- (NSString *)applicationStateString {
	UIApplicationState state = [[UIApplication sharedApplication] applicationState];
	switch (state) {
		case UIApplicationStateActive: return @"Active";
		case UIApplicationStateInactive: return @"InActive";
		case UIApplicationStateBackground: return @"Background";
		default: return @"** Unknown Application State";
	}
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	SCILog(@"Start");
	[[SCIVideophoneEngine sharedEngine] applicationWillEnterForeground];
	[UIApplication sharedApplication].applicationIconBadgeNumber = 0;
	[[UNUserNotificationCenter currentNotificationCenter] removeAllDeliveredNotifications];
}

- (BOOL)application:(UIApplication *)application continueUserActivity:(NSUserActivity *)userActivity restorationHandler:(void(^)(NSArray<id<UIUserActivityRestoring>> * __nullable restorableObjects))restorationHandler {

	SCILog([userActivity activityType]);
	if ([[userActivity activityType] isEqualToString:CSSearchableItemActionType]) {
		// This activity represents an item indexed using Core Spotlight, so restore the context related to the unique identifier.
		// The unique identifier of the Core Spotlight item is set in the activity’s userInfo for the key CSSearchableItemActivityIdentifier.
		NSString *uniqueIdentifier = [userActivity.userInfo objectForKey:CSSearchableItemActivityIdentifier];
		
		SCIContact *contact;
		SCIContactPhone phone = SCIContactPhoneNone;
		[[SCIContactManager sharedManager] getContact:&contact phone:&phone forNumber:uniqueIdentifier];
		if(contact)
		{
			[(MainTabBarController *)self.tabBarController selectWithTab:TabContacts];
			UINavigationController *navController = (UINavigationController *)self.tabBarController.selectedViewController;
			[(PhonebookViewController *)navController.viewControllers[0] openContact:contact];
		}
		
		return YES;
	}
	else if ([[userActivity activityType] isEqualToString:INStartAudioCallIntentIdentifier] ||
		[[userActivity activityType] isEqualToString:INStartVideoCallIntentIdentifier]) {
		
		INInteraction *interaction = userActivity.interaction;
		INPerson *contact;
		NSString *dialString;
		
		if (@available(iOS 13.0, *))
		{
			INStartCallIntent *intent = (INStartCallIntent *)interaction.intent;
			INStartCallIntentResponse *intentResponse = (INStartCallIntentResponse *)interaction.intentResponse;
			contact = [intent.contacts firstObject];
							
			// We don't support starting group calls via siri.
			if (contact != nil && (intentResponse == nil || intentResponse.code == INStartCallIntentResponseCodeContinueInApp)) {
				dialString = UnformatPhoneNumber(contact.personHandle.value);
			}
		}
		else
		{
			INStartVideoCallIntent *intent = (INStartVideoCallIntent *)interaction.intent;
			INStartVideoCallIntentResponse *intentResponse = (INStartVideoCallIntentResponse *)interaction.intentResponse;
			contact = [intent.contacts firstObject];
			
			// We don't support starting group calls via siri.
			if (contact != nil && (intentResponse == nil || intentResponse.code == INStartVideoCallIntentResponseCodeContinueInApp)) {
				dialString = UnformatPhoneNumber(contact.personHandle.value);
			}
		}
		
		if (![SCICallController.shared.calls.firstObject.dialString isEqualToString:dialString]) {
			[SCICallController.shared makeOutgoingCallTo:dialString dialSource:SCIDialSourceDeviceContact];
			return YES;
		}
		else {
			return NO;
		}
		
		
	}
	else if ([userActivity.activityType isEqualToString:@"com.sorenson.ntouch.call"]) {
		NSString *dialString = userActivity.userInfo[@"dialString"];
		[SCICallController.shared makeOutgoingCallTo:dialString dialSource:SCIDialSourceWebDial];
		return YES;
	}
	
	return NO;
}

- (void)application:(UIApplication *)application performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem completionHandler:(void (^)(BOOL))completionHandler {
	
	SCILog(@"");
	if ([shortcutItem.type isEqualToString:@"AddContactAction"]) {
		if (nil != [[SCIVideophoneEngine sharedEngine] videophoneEngine]) {
			[(MainTabBarController *)self.tabBarController selectWithTab:TabContacts];
			UINavigationController *navController = (UINavigationController *)self.tabBarController.selectedViewController;
			[(PhonebookViewController *)navController.viewControllers[0] addNewContact];
		}
		else {
			[[SCIAccountManager sharedManager] addCoreEventWithType:coreEventUIAddContact];
		}
	}
	else if ([shortcutItem.type isEqualToString:@"SelectFavoriteAction"]) {
		id value = [shortcutItem.userInfo objectForKey:@"DialStringKey"];
		if ([value isKindOfClass:[NSString class]]) {
			NSString *phoneNumber = (NSString *)value;
			if (phoneNumber) {
				[SCICallController.shared makeOutgoingCallTo:phoneNumber dialSource:SCIDialSourcePushNotificationMissedCall];
			}
		}
	}
}

#pragma mark - Push Notification Delegate Methods

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {

	NSMutableString *mutableToken = [NSMutableString string];
	for (NSUInteger i = 0; i < [deviceToken length]; i++) {
		[mutableToken appendFormat:@"%02.2hhx", ((const char *)deviceToken.bytes)[i]];
	}
	NSString *token = [mutableToken copy];

	NSLog(@"Did Register for Remote Notifications with Device Token (%@)", token);

	self.engine.deviceToken = token;

	// Only send deviceToken and NotificationType if logged in, otherwise notifyLoggedIn will send it.
	if ([SCIVideophoneEngine sharedEngine].isAuthorized) {

		[[self engine] sendDeviceToken:self.engine.deviceToken];
		
		// Send push notifcation type setting
		[self sendNotificationTypes];
	}
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
	NSLog(@"Did Fail to Register for Remote Notifications");
	NSLog(@"%@, %@", error, error.localizedDescription);
	 [[FIRCrashlytics crashlytics] recordError:error];
	
	self.engine.deviceToken = @"";
	
	// Only send deviceToken and NotificationType if logged in, otherwise notifyLoggedIn will send it.
	if ([SCIVideophoneEngine sharedEngine].isAuthorized)
	{
		[[self engine] sendDeviceToken:self.engine.deviceToken];

		// Send push notifcation type setting
		[self sendNotificationTypes];
	}
}

- (void)sendNotificationTypes {
	// Set notification type property and send to Core.
	// Update:  Apple changed how to read these settings in UNUserNotificationCenter,
	// but we still need to preserve the format of the old UIUserNotificationType for Core.
	if (@available(iOS 10.0, *)) {
		[[UNUserNotificationCenter currentNotificationCenter] getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings * _Nonnull settings) {
			NSUInteger notificationType = 0;
			notificationType |= settings.badgeSetting == UNNotificationSettingEnabled ? 1 << 0 : 0 << 0;
			notificationType |= settings.soundSetting == UNNotificationSettingEnabled ? 1 << 1 : 0 << 1;
			notificationType |= settings.alertSetting == UNNotificationSettingEnabled ? 1 << 2 : 0 << 2;
			[self.engine sendNotificationTypes:notificationType];
		}];
	}
}

#pragma mark - WCSessionDelegate

-(void)session:(WCSession *)session activationDidCompleteWithState:(WCSessionActivationState)activationState error:(NSError *)error
{
	if (error) {
		NSLog(@"AppDelegate WCSession activation error: %@", error.localizedDescription);
	}

	// Trigger the watch app if installed to send us a push token

	BOOL alreadyTransferring = NO;
	for (WCSessionUserInfoTransfer *userInfoTransfer in session.outstandingUserInfoTransfers) {
		if ([userInfoTransfer.userInfo[@"ensStatus"] isEqual:@"NeedsUpdate"]) {
			alreadyTransferring = YES;
		}
	}

	if (!alreadyTransferring) {
		[session transferUserInfo:@{ @"ensStatus": @"NeedsUpdate" }];
	}
}

- (void)sessionDidBecomeInactive:(nonnull WCSession *)session {
    
}


- (void)sessionDidDeactivate:(nonnull WCSession *)session {
    
}

#pragma mark - WatchKit Delegate

- (void)session:(WCSession *)session didReceiveMessage:(NSDictionary<NSString *,id> *)message {
	if (message[@"watchDeviceToken"]) {
		SCIENSController.shared.watchDeviceToken = message[@"watchDeviceToken"];
	}
}

- (void)session:(WCSession *)session didReceiveMessage:(NSDictionary<NSString *, id> *)message replyHandler:(void(^)(NSDictionary<NSString *, id> *replyMessage))replyHandler  {
	SCILog(@"message: %@", message);
	
	if (message[WatchKitCallHistoryRequestKey])
	{
		NSArray *sortDescriptors = [NSArray arrayWithObjects:[NSSortDescriptor sortDescriptorWithKey:@"date" ascending:NO], nil];
		NSArray *callListArray = [[SCICallListManager sharedManager].items sortedArrayUsingDescriptors:sortDescriptors];
		
		NSMutableArray *returnArray = [[NSMutableArray alloc] init];
		NSInteger itemsToReturn = callListArray.count >= 20 ? 20 : callListArray.count;
		NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
		dateFormatter.dateFormat = @"MM-dd HH:mm";
		
		for (int i = 0; i < itemsToReturn; i++) {
			SCICallListItem *callItem = [callListArray objectAtIndex:i];
			NSString *itemTime = [dateFormatter stringFromDate:callItem.date];
			NSString *callerName = [ContactUtilities displayNameForSCICallListItem:callItem];
			NSDictionary *dItem = @{ @"callerName" : callerName, @"callTime" : itemTime };
			[returnArray addObject:dItem];
		}
		
		replyHandler( @{ WatchKitCallHistoryDataKey : (NSArray *)returnArray } );
		
	}
	else if(message[WatchKitSignMailListRequestKey])
	{
		SCIItemId *signMailCategory = [[SCIVideophoneEngine sharedEngine] signMailMessageCategoryId];
		NSArray *messages = [[SCIVideophoneEngine sharedEngine] messagesForCategoryId:signMailCategory];
		if (messages.count > 20) {
			messages = [messages objectsAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, 20)]];
		}
		
		// We populate the data asynchronously and we need to make sure the data has finished loading before sending it.
		dispatch_group_t group = dispatch_group_create();
		
		NSMutableArray *output = [NSMutableArray array];
		for (SCIMessageInfo *message in messages) {
			NSMutableDictionary *item = [NSMutableDictionary dictionary];
			item[@"callerName"] = [ContactUtilities displayNameForSCIMessageInfo:message];
			item[@"callTime"] = message.date;
			item[@"pausePoint"] = @(message.pausePoint);
			item[@"messageId"] = message.messageId.string;
			
			// Get the download URL
			dispatch_group_enter(group);
			[[SCIVideophoneEngine sharedEngine] requestDownloadURLsForMessage:message completion:^(NSArray *downloadURLs, NSString *viewId, NSError *error) {
				if (!error) {
					// Find the lowest bitrate h264 URL
					SCIMessageDownloadURLItem *urlItem = nil;
					for (SCIMessageDownloadURLItem *item in downloadURLs) {
						if ([item.m_eVideoCodec isEqualToString:@"h264"] && (urlItem == nil || item.maxBitRate < urlItem.maxBitRate)) {
							urlItem = item;
						}
					}
					
					item[@"videoURL"] = urlItem.downloadURL;
					item[@"viewId"] = viewId;
				}
				else {
					SCILog(@"Failed to request sign mail download URLs %@", error);
				}
				dispatch_group_leave(group);
			}];
			
			[output addObject:item];
		}
		
		dispatch_group_notify(group, dispatch_get_main_queue(), ^{ replyHandler(@{ WatchKitSignMailListDataKey : output }); });
	}
}

#pragma mark - Navigation methods

- (UIViewController *)topViewController {
	return [UIViewController topViewController];
}

- (SCIAccountManager *)accountManager {
	return [SCIAccountManager sharedManager];
}

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

/**
 applicationWillTerminate: saves changes in the application's managed object context before the application terminates.
 */
- (void)applicationWillTerminate:(UIApplication *)application
{
	NSLog(@"******** applicationWillTerminate");
	[[SCIVideophoneEngine sharedEngine] setRestartReason:SCIRestartReasonTerminated];
	[AnalyticsManager.shared trackUsage:AnalyticsEventTerminated properties:nil];
	[[SCIVideophoneEngine sharedEngine] shutdown];
}

#pragma mark - Application's Documents directory

- (NSString *)applicationName {
	return [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"];
}

- (NSString *)bundleVersion {
	NSString *bundleVersion = [[NSBundle mainBundle] sci_bundleVersion];
	return bundleVersion;
}

- (NSString *)applicationDocumentsDirectory {
	return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
}

- (void)orientationChanged:(NSNotification *)notification {
	UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
	if (UIDeviceOrientationIsValidInterfaceOrientation(orientation)) {
		if(orientation != self.currentOrientation) {
			SCILog(@"Orientation changed");
			self.currentOrientation = (UIDeviceOrientation)orientation;
		}
	}
}

- (void)accessibilityTextSizeChanged:(NSNotification *)notification {
	[Theme reloadThemes];
}

- (void)displayApplicationBusyProgress:(NSString *)message {
	[[MBProgressHUD showHUDAddedTo:self.tabBarController.view animated:YES] setLabelText:message];
}

- (void)hideApplicationBusyProgress {
	[MBProgressHUD hideHUDForView:self.tabBarController.view animated:YES];
}

- (void)networkMonitorDidChangePath:(SCINetworkMonitor *)networkMonitor {
	if (networkMonitor.bestInterfaceName) {
		BOOL performNetworkChange = NO;
		NetworkType networkType = NetworkType::None;
		NSString *dataString = @"";
		
		std::string interfaceName = [networkMonitor.bestInterfaceName UTF8String];
		if (stiOSGetAdapterName().compare(interfaceName) != 0) {
			performNetworkChange = YES;
			stiOSSetAdapterName(interfaceName);
			SCILog(@"Network: Interface is different");
		}
		
		std::string tmp4;
		stiGetLocalIp(&tmp4, estiTYPE_IPV4);
		if (tmp4.compare(ipv4Address) != 0 || ipv4Address.compare("") == 0) {
			performNetworkChange = YES;
			ipv4Address = tmp4;
			SCILog(@"Network: IPv4 Address is different");
		}
		
		std::string tmp6;
		stiGetLocalIp(&tmp6, estiTYPE_IPV6);
		if (tmp6.compare(ipv6Address) != 0 || ipv6Address.compare("") == 0) {
			performNetworkChange = YES;
			ipv6Address = tmp6;
			SCILog(@"Network: IPv6 Address is different");
		}
		
		if (backgroundedSinceNetworkChange) {
			backgroundedSinceNetworkChange = false;
			performNetworkChange = YES;
		}
		
		switch (networkMonitor.bestInterfaceType) {
			case SCINetworkTypeNone: {
				g_NetworkConnected = false;
				networkType = NetworkType::None;
				dispatch_async(dispatch_get_main_queue(), ^{
					[[SCICallController shared] endAllCalls];
				});
				
				[[SCIVideophoneEngine sharedEngine] applicationHasNoNetwork];
			} break;
				
			case SCINetworkTypeWired:
				g_NetworkConnected = true;
				networkType = NetworkType::Wired;
				[[SCIVideophoneEngine sharedEngine] applicationHasNetwork];
				break;
				
			case SCINetworkTypeWifi:
				g_NetworkConnected = true;
				networkType = NetworkType::WiFi;
				[[SCIVideophoneEngine sharedEngine] applicationHasNetwork];
				break;
				
			case SCINetworkTypeOther:
				g_NetworkConnected = true;
				networkType = NetworkType::Wired;
				[[SCIVideophoneEngine sharedEngine] applicationHasNetwork];
				break;
				
			case SCINetworkTypeCellular:
				g_NetworkConnected = true;
				networkType = NetworkType::Cellular;
				[[SCIVideophoneEngine sharedEngine] applicationHasNetwork];
				dataString = [NSString stringWithFormat:@"Carrier=%@ CarrierType=%@", networkMonitor.telecomCarrier, networkMonitor.telecomRadioType];
				break;
		}
		
		// reset login start time to give an accurate login log
		if (startupLogged == NO && networkMonitor.bestInterfaceType != SCINetworkTypeNone) {
			self.loginStartDateTime = [NSDate date];
			startupLogged = YES;
		}
		
		if (performNetworkChange) {
			SCILog(@"Network Changed: %@, Notify VPE", networkMonitor.bestInterfaceName);
			[self setBitRateForStatus:networkMonitor.bestInterfaceType];
			[[SCIVideophoneEngine sharedEngine] restartConnection];
			
			IstiNetwork::InstanceGet ()->networkInfoSet (networkType, [dataString UTF8String]);
		}
		else {
			SCILog(@"Network Changed: %@, Duplicate change path event", networkMonitor.bestInterfaceName);
		}
		
		dispatch_async(dispatch_get_main_queue(), ^{
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINetworkReachabilityDidChange
																object:self
															  userInfo:@{SCINetworkReachabilityKey : [NSNumber numberWithInt:(int)networkType] }];
		});
	}
	else {
		g_NetworkConnected = false;
	}
}

#pragma mark - Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
#ifdef stiDEBUG
	[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"memoryWarning"]];
	
	UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
	notification.title = @"Debug: ntouch has received a Low Memory warning.";
	notification.sound = UNNotificationSound.defaultSound;
	
	UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"memoryWarning" content:notification trigger:nil];
	[UNUserNotificationCenter.currentNotificationCenter
	 addNotificationRequest:request
	 withCompletionHandler:^(NSError * _Nullable error) {
		 if (error != nil) {
			 NSLog(@"Error posting notification: %@", error);
		 }
	 }];
#endif
	
	[[NSURLCache sharedURLCache] removeAllCachedResponses];
	
	[[SCIContactManager sharedManager] clearThumbnailImages];
	
	[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent: @"AppEvent=LowMemoryWarning"];
}

- (void)configureForPublicMode
{
	BOOL publicModeEnabled = ([[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic);
	if (publicModeEnabled) {
		
		// Clear records in Recent Call Widget
		NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.sorenson.ntouch.recent-calls-extension-data-sharing"];
		[defaults setValue:nil forKey:@"recentSummary"];
		[defaults synchronize];
		
		[[SCIVideophoneEngine sharedEngine] setMaxCalls:1];
		[[SCIVideophoneEngine sharedEngine] setAudioEnabled:NO];
		
		SCILog(@"Changing UI for Public interface mode.");
		
		[(MainTabBarController*)self.tabBarController configureTabsForPublicMode];

		[self.engine sendNotificationTypes:0];
	}
}

- (void)updateVideoCenterTab {
	if (![[SCIVideophoneEngine sharedEngine] videoMessageEnabled]) {

		NSMutableArray *tabs = [NSMutableArray arrayWithArray:self.tabBarController.viewControllers];
		
		UINavigationController *signMailNav;
		UINavigationController *videoCenterNav;
		for (UIViewController *viewController in self.tabBarController.viewControllers) {
			if ([viewController isKindOfClass:UINavigationController.class]) {
				UINavigationController *nav = (UINavigationController *)viewController;
				if ([nav.topViewController isKindOfClass:SignMailListViewController.class]) {
					signMailNav = nav;
				}
			}
		}
				 
		if([self.tabBarController.viewControllers containsObject:signMailNav])
			[tabs removeObject:signMailNav];

		if([self.tabBarController.viewControllers containsObject:videoCenterNav])
			[tabs removeObject:videoCenterNav];
		
		[self.tabBarController setViewControllers:tabs animated:YES];
	}
}

- (void)logStartUpMetrics {
	
	startupLogged = YES;

	NSTimeInterval timeSinceLaunch = 0.0, timeSinceStartLogin = 0.0;
	if (self.launchDateTime) {
		timeSinceLaunch = fabs([self.launchDateTime timeIntervalSinceNow]);
	}
	if (self.loginStartDateTime) {
		timeSinceStartLogin = fabs([self.loginStartDateTime timeIntervalSinceNow]);
	}

	NSMutableString *logMessage = [NSMutableString stringWithFormat:@"EventType=AppReady TimeFromAppStart=%3.2f TimeFromLogin=%3.2f ",
										timeSinceLaunch, timeSinceStartLogin];

	if (self.networkMonitor.bestInterfaceType == SCINetworkTypeCellular) {
		CTCarrier *carrier = nil;
		NSString *carrierType = @"";
		CTTelephonyNetworkInfo *netinfo = [[CTTelephonyNetworkInfo alloc] init];
		if (netinfo) {
			carrier = [netinfo subscriberCellularProvider];
			carrierType = netinfo.currentRadioAccessTechnology;
		}
		NSString *carrierDetails = [NSString stringWithFormat:@" NetworkType=Cellular TelecomCarrier=%@ TelecomType=%@", carrier.carrierName, carrierType];
		[logMessage appendString:carrierDetails];
	}
	else if (self.networkMonitor.bestInterfaceType == SCINetworkTypeWifi) {
		[logMessage appendString:@"NetworkType=WiFi"];
		
		NSString *ssid = [self getCurrentWiFiSSID];
		if (ssid) {
			[logMessage appendFormat:@" SSID=\"%@\"", ssid];
		}
	}
	else if (self.networkMonitor.bestInterfaceType == SCINetworkTypeWired) {
		[logMessage appendString:@"NetworkType=Wired"];
	}
	else if (self.networkMonitor.bestInterfaceType == SCINetworkTypeOther) {
		[logMessage appendString:@"NetworkType=Other"];
	}
	else if (self.networkMonitor.bestInterfaceType == SCINetworkTypeNone) {
		[logMessage appendString:@"NetworkType=NotReachable"];
	}
    
    // iOS seems to only provide the app language no matter what the settings of the device are, if this changes in the future this would allow us to log both the app langauge and system language. For now we'll return a blank value
//    NSString * systemLanguage = [[[NSLocale preferredLanguages] firstObject] stringByReplacingOccurrencesOfString:@"-" withString:@"_"];
    NSString * systemLanguage = @"";
    [logMessage appendString: [NSString stringWithFormat:@" SystemLanguage=%@", systemLanguage]];
    NSString * uiLanguage = [[[[[NSBundle mainBundle] preferredLocalizations] objectAtIndex:0] LanguageCode] stringByReplacingOccurrencesOfString:@"-" withString:@"_"];
    [logMessage appendString: [NSString stringWithFormat: @" UILanguage=%@", uiLanguage]];

	[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:logMessage];
}

- (void)networkDataToLog
{
	if (self.networkMonitor)
	{
		NetworkType netType = NetworkType::None;
		NSMutableString *data = [[NSMutableString alloc] initWithString:@""];
		
		switch (self.networkMonitor.bestInterfaceType)
		{
			case SCINetworkTypeNone:
			{
				[data appendFormat:@"NetworkType=NotReachable"];
				break;
			}
			case SCINetworkTypeWifi:
			{
				netType = NetworkType::WiFi;
				[data appendFormat:@"NetworkType=WiFi"];
				NSString *ssid = [self getCurrentWiFiSSID];
				if (ssid) {
					[data appendFormat:@" SSID=\"%@\"", ssid];
				}
				break;
			}
			case SCINetworkTypeCellular:
			{
				CTCarrier *carrier = nil;
				NSString *carrierType = @"";
				CTTelephonyNetworkInfo *netinfo = [[CTTelephonyNetworkInfo alloc] init];
				if (netinfo) {
					carrier = [netinfo subscriberCellularProvider];
					carrierType = netinfo.currentRadioAccessTechnology;
					[data appendFormat:@"NetworkType=Cellular TelecomCarrier=%@ TelecomType=%@", carrier.carrierName, carrierType];
				}
				
				netType = NetworkType::Cellular;
				break;
			}
			case SCINetworkTypeWired:
				[data appendFormat:@"NetworkType=Wired"];
				break;
			case SCINetworkTypeOther:
				[data appendFormat:@"NetworkType=Other"];
				break;
			default:
				[data appendFormat:@"NetworkType=Unknown"];
		}
		
		IstiNetwork::InstanceGet()->networkInfoSet (netType, [data UTF8String]);
	}
}

- (NSString *)getCurrentWiFiSSID {
	NSString *wifiName = nil;
	NSArray *ifs = (__bridge_transfer id)CNCopySupportedInterfaces();
	for (NSString *ifnam in ifs) {
		NSDictionary *info = (__bridge_transfer id)CNCopyCurrentNetworkInfo((__bridge CFStringRef)ifnam);
		if (info[@"SSID"]) {
			wifiName = info[@"SSID"];
		}
	}
	return wifiName;
}

- (void)loginUsingCache:(bool)useCache
					withImmediateCallback:(void (^)(void))initialBlock
					  errorHandler:(void (^)(NSError *))errorBlock
					successHandler:(void (^)(void))successBlock
{
	__weak AppDelegate *weak_self = self;
	self.loginStartDateTime = [NSDate date];
	void (^loginBlock)(NSDictionary *, NSDictionary *, void (^proceedBlock)(void), void (^terminateBlock)(void)) = ^void(NSDictionary *accountStatusDictionary, NSDictionary *errorDictionary, void (^proceedBlock)(void), void (^terminateBlock)(void)) {
		__strong AppDelegate *strong_self = weak_self;
		if (initialBlock)
			initialBlock();
		
		NSError *loginError = errorDictionary[SCIErrorKeyLogin];
		if (loginError) {
			[[FIRCrashlytics crashlytics] recordError:loginError];
			errorBlock(loginError);
		} else {
			NSArray *ringGroupInfos = accountStatusDictionary[SCINotificationKeyRingGroupInfos];
			
			if (ringGroupInfos.count > 1) {
				strong_self.accountManager.endpointSelectionController = [[EndpointSelectionController alloc] init];
				strong_self.accountManager.endpointSelectionController.ringGroupInfos = ringGroupInfos;
				[strong_self.accountManager.endpointSelectionController showEndpointSelectionMenu];

				if (terminateBlock)
					terminateBlock();
				return;
			}
			else {
				strong_self.accountManager.endpointSelectionController = nil;
			}
			
			void (^afterCheckingProviderAgreementStatusBlock)(BOOL providerAgreementAccepted) = ^(BOOL providerAgreementAccepted) {
				if (providerAgreementAccepted) {
					if (proceedBlock) {
						proceedBlock();
					}
					
					if (successBlock) {
						successBlock();
					}
				} else {
					if (terminateBlock) {
						terminateBlock();
					}
				}
			};
			
			void (^afterCheckingEmergencyAddressBlock)(BOOL addressEntered) = nil;
			NSNumber *providerAgreementTypeNumber = accountStatusDictionary[SCINotificationKeyProviderAgreementType];
			SCIProviderAgreementType providerAgreementType = (SCIProviderAgreementType)providerAgreementTypeNumber.unsignedIntegerValue;
			switch (providerAgreementType) {
				case SCIProviderAgreementTypeStatic: {
					afterCheckingEmergencyAddressBlock = ^(BOOL addressEntered) {
						__strong AppDelegate *strong_self = weak_self;
						if (addressEntered) {
							SCIStaticProviderAgreementStatus *status = accountStatusDictionary[SCINotificationKeyProviderAgreementStatus];
							if (status) {
								SCIStaticProviderAgreementState state = ^{
									SCIStaticProviderAgreementState state = SCIStaticProviderAgreementStateExpired;
									
									if (status) {
										state = [status stateForCurrentSoftwareVersionString:self.bundleVersion];
									} else {
										state = SCIStaticProviderAgreementStateExpired;
									}
									return state;
								}();
								self.accountManager.eulaAgreementState = SCIProviderAgreementStateFromSCIStaticProviderAgreementState(state);
								switch (state) {
									case SCIStaticProviderAgreementStateNone:
									case SCIStaticProviderAgreementStateExpired:
									case SCIStaticProviderAgreementStateCancelled:
									{
										[strong_self.accountManager cancelLoginTimeout];
										strong_self.accountManager.providerAgreementTypeNumber = providerAgreementTypeNumber;
										strong_self.accountManager.providerAgreementStatusAwaitingUpdate = status;
									} break;
									case SCIStaticProviderAgreementStateAccepted: {
										//Do nothing.
									} break;
								}
							} else {
								self.accountManager.eulaAgreementState = SCIProviderAgreementStateFromSCIStaticProviderAgreementState(SCIStaticProviderAgreementStateAccepted);
							}
							afterCheckingProviderAgreementStatusBlock(YES);
						} else {
							if (terminateBlock)
								terminateBlock();
						}
					};
				} break;
				case SCIProviderAgreementTypeDynamic: {
					afterCheckingEmergencyAddressBlock = ^(BOOL addressEntered) {
						__strong AppDelegate *strong_self = weak_self;
						if (addressEntered) {
							NSArray *agreements = accountStatusDictionary[SCINotificationKeyAgreements];
							if (agreements) {
								NSArray *agreementStates = ^{
									NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
									
									for (SCIDynamicProviderAgreement *agreement in agreements) {
										[mutableRes addObject:@(agreement.status.state)];
									}
									
									return [mutableRes copy];
								}();
								SCIDynamicProviderAgreementState state = SCIDynamicProviderAgreementStateWorst(agreementStates);
								self.accountManager.eulaAgreementState = SCIProviderAgreementStateFromSCIDynamicProviderAgreementState(state);
								switch (state) {
									case SCIDynamicProviderAgreementStateUnknown:
									case SCIDynamicProviderAgreementStateNeedsAcceptance:
									case SCIDynamicProviderAgreementStateRejected: {
										[strong_self.accountManager cancelLoginTimeout];
										strong_self.accountManager.providerAgreementTypeNumber = providerAgreementTypeNumber;
										strong_self.accountManager.agreementsAwaitingAcceptance = [agreements mutableCopy];
										[[SCIAccountManager sharedManager] addCoreEventWithType:coreEventShowProviderAgreement];
									} break;
									case SCIDynamicProviderAgreementStateAccepted: {
										//Do nothing.
									} break;
								}
							} else {
								self.accountManager.eulaAgreementState = SCIProviderAgreementStateFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementStateAccepted);
							}
							if (afterCheckingProviderAgreementStatusBlock)
								afterCheckingProviderAgreementStatusBlock(YES);
						} else {
							if (terminateBlock)
								terminateBlock();
						}
					};
				} break;
				case SCIProviderAgreementTypeNone:
				case SCIProviderAgreementTypeUnknown: {
					if (self.engine.interfaceMode == SCIInterfaceModePublic) {
						afterCheckingEmergencyAddressBlock = ^(BOOL addressEntered) {
							if (addressEntered) {
								afterCheckingProviderAgreementStatusBlock(YES);
							} else {
								if (terminateBlock)
									terminateBlock();
							}
						};
					}
					else {
						//[NSException raise:@"Illegal provider agreement type." format:@"The accountStatusDictionary passed to the block passed to %@ contained an illegal value (%@) for the SCINotificationKeyProviderAgreementType (%@) key.", NSStringFromSelector(@selector(loginWithBlock:)),  providerAgreementTypeNumber, SCINotificationKeyProviderAgreementType];
					}
				} break;
			};
			
			SCIEmergencyAddress *emergencyAddress = accountStatusDictionary[SCINotificationKeyEmergencyAddress];
			if (emergencyAddress) {
				
				SCIEmergencyAddressProvisioningStatus status = emergencyAddress.status;
				switch (status) {
					case SCIEmergencyAddressProvisioningStatusUpdatedUnverified:
					case SCIEmergencyAddressProvisioningStatusNotSubmitted: {
						[strong_self.accountManager cancelLoginTimeout];
						[[SCIAccountManager sharedManager] addCoreEventWithType:coreEventAddressChanged];
					} break;
					case SCIEmergencyAddressProvisioningStatusSubmitted:
					case SCIEmergencyAddressProvisioningStatusUnknown: break;
				}
				
				if (afterCheckingEmergencyAddressBlock)
					afterCheckingEmergencyAddressBlock(YES);
			}
			else {
				// If emergencyAddress returns null we still need to call the afterCheckingEmercenyAddressBlock
				//  in order to finish logging in. This is especially true for cases where the interfaceMode is SCIInterfaceModeHearing.
				if (afterCheckingEmergencyAddressBlock)
					afterCheckingEmergencyAddressBlock(YES);
			}
		}
	};
	if (useCache) {
		[[SCIVideophoneEngine sharedEngine] loginUsingCacheWithBlock:loginBlock];
	}
	else {
		[[SCIVideophoneEngine sharedEngine] loginWithBlock:loginBlock];
	}
}

- (void)logout
{
	[self.accountManager clientReauthenticate];
}

@end
