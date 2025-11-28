//
//  AppDelegate.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>
#import <AVFoundation/AVFoundation.h>
#import <PushKit/PushKit.h>
#import <WatchConnectivity/WatchConnectivity.h>
#import <UserNotifications/UserNotifications.h>

#define appDelegate ((AppDelegate *)[[UIApplication sharedApplication] delegate])

#define k911 @"911"
#define k411 @"411"
#define kTechnicalSupportByVideophone @"18012879403"
#define kTechnicalSupportByPhone @"18664966111"
#define kCustomerInformationRepresentativeByVideophone @"18013868500"
#define kCustomerInformationRepresentativeByPhone @"18667566729"
#define kCustomerInformationRepresentativeByN11 @"611"
#define kSVRSByPhone @"18663278877"
#define kSVRSEspanolByPhone @"18669877528"
#define kFieldHelpDeskByVideophone @"18017012222"
#define kFieldHelpDeskByPhone @"18665366788"
#define kSavedTabSelectedIndex @"kSavedTabSelectedIndex"
#define kForgotPasswordURL @"https://www.sorensonvrs.com/secured_pword_reset"
#define kAutoEnableSpanishFeaturesTurnedOffByUserKey @"kAutoEnableSpanishFeaturesTurnedOffByUserKey"

#define IS_IPAD (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad)
#define IS_IPHONE (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone)
#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

#ifdef stiDEBUG
	#define kTechnicalSupportByVideophoneUrl @"sip:18012879403@vrs-eng.qa.sip.svrs.net"
	#define kCustomerInformationRepresentativeByVideophoneUrl @"sip:18013868500@vrs-eng.qa.sip.svrs.net"
	#define kFieldHelpDeskByVideophoneUrl @"sip:vrs-eng.qa.sip.svrs.net"
	#define kSVRSByVideophoneUrl @"vrs-eng.qa.sip.svrs.net"
#else
	#define kTechnicalSupportByVideophoneUrl @"sip:18012879403@tech.svrs.tv"
	#define kCustomerInformationRepresentativeByVideophoneUrl @"sip:18013868500@cir.svrs.tv"
	#define kFieldHelpDeskByVideophoneUrl @"fhd.svrs.tv"
	#define kSVRSByVideophoneUrl @"call.svrs.tv"
#endif

extern bool g_NetworkConnected;

@class SignMailListViewController;
@class KeypadViewController;
@class EmergencyInfoViewController;
@class MyRumble;
@class SettingsViewController;
@class SCIAccountManager;
@class SCIDefaults;
@class PhonebookViewController;
@class SCIRecentsViewController;
@class StartTopLevelUIUtilities;
@class TopLevelUIController;
@class CallWindowController;
@class SCINetworkMonitor;

@interface AppDelegate : NSObject <UIApplicationDelegate, WCSessionDelegate, UNUserNotificationCenterDelegate> {
	
	UIWindow *window;
	NSInteger networkingCount;
	NSDictionary *myRumblePatterns;
    MyRumble *myRumble;
	EmergencyInfoViewController *emergencyInfoViewController;
	BOOL showHUD;
	BOOL dialByIPEnabled;
	dispatch_once_t protectedDataOnceToken;
}

@property (nonatomic, strong) IBOutlet UIWindow *window;
@property (nonatomic, strong) TopLevelUIController *topLevelUIController;
@property (nonatomic, strong) StartTopLevelUIUtilities *startTopLevelUIUtilities;
@property (nonatomic, strong) NSDictionary *launchURL;
@property (nonatomic, strong) NSDictionary *launchOptions;
@property (nonatomic, strong) UITabBarController *tabBarController;

@property (nonatomic, assign) BOOL disableToolBarAutoHide;
@property (nonatomic, assign) NSInteger networkingCount;


@property (nonatomic, strong) IBOutlet UINavigationController *messageNavigationController;
@property (nonatomic, strong) IBOutlet UINavigationController *sciRecentsNavigationController;
@property (nonatomic, strong) IBOutlet SCIRecentsViewController *sciRecentsViewController;
@property (nonatomic, strong) IBOutlet SignMailListViewController *signMailListViewController;
@property (nonatomic, strong) IBOutlet KeypadViewController *keypadViewController;
@property (nonatomic, strong) IBOutlet PhonebookViewController *phonebookViewController;
@property (nonatomic, strong) IBOutlet EmergencyInfoViewController *emergencyInfoViewController;
@property (nonatomic, strong) IBOutlet SettingsViewController *settingsViewController; // iPad only
@property (nonatomic, retain) CallWindowController *callWindowController;

@property (nonatomic, strong) NSDictionary *myRumblePatterns;
@property (nonatomic, strong) MyRumble *myRumble;

@property (nonatomic, assign) BOOL showHUD;
@property (nonatomic, assign) BOOL dialByIPEnabled;
@property (nonatomic, assign) BOOL updateRequired;

@property (nonatomic, assign) UIDeviceOrientation currentOrientation;

@property (nonatomic, strong) NSString *lastDialString;
@property (nonatomic, readonly) SCIAccountManager *accountManager;
@property (nonatomic, readonly) SCIDefaults	*defaults;
@property (nonatomic, strong) NSDate *launchDateTime;
@property (nonatomic, strong) NSDate *loginStartDateTime;
@property (nonatomic, strong) SCINetworkMonitor *networkMonitor;

- (NSString *)applicationName;
- (NSString *)applicationDocumentsDirectory;
- (UIViewController *)topViewController;
- (void)presentUpdateRequired;
- (void)configureForPublicMode;
- (void)updateVideoCenterTab;
- (void)sendNotificationTypes;
- (void)setBitRateSetting:(NSUInteger)bitRateInKbps;
- (void)setMobileBitRateSetting:(NSUInteger)bitRateInKbps;
- (void)setProfileLevel;
- (void)installDynamicShortcutItems;

- (void)logStartUpMetrics;
- (void)loginUsingCache:(bool)useCache
					withImmediateCallback:(void (^)(void))initialBlock
					  errorHandler:(void (^)(NSError *))errorBlock
					successHandler:(void (^)(void))successBlock;
- (void)logout;
- (void)networkDataToLog;
@end

extern NSString * const WatchKitCallHistoryRequestKey;
extern NSString * const WatchKitCallHistoryDataKey;
extern NSString * const WatchKitSignMailListRequestKey;
extern NSString * const WatchKitSignMailListDataKey;
