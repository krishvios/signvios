//
//  SCIAccountManager.m
//  ntouch
//
//  Created by Kevin Selman on 5/8/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCIAccountManager.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "stiSVX.h"
#import "ContactUtilities.h"
#import "IstiVideophoneEngine.h"
#import "MyPhoneEditViewControllerNew.h"
#import "SCIContactManagerCpp.h"
#import "SCICallListManager.h"
#import "SCICallListManager+UserInterfaceProperties.h"
#import "SCIBlockedManager.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine+UserInterfaceProperties_ProviderAgreementStatus.h"
#import "SCIEmergencyAddress.h"
#import "SCIMessageViewer.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIRingGroupInfo.h"
#import "SettingsViewController.h"
#import "propertydef.h"
#import "MBProgressHUD.h"
#import "SCIMessageViewer.h"
#import "SCIMessageViewer+AsynchronousCompletion.h"
#import "FCCUserRegistrationViewController.h"
#import "UINavigationControllerLocked.h"
#import "SCIPropertyManager+AuxiliaryAccountSpecificLocalProperties.h"
#import "DismissibleiPadKeyboardViewController.h"
#import "NSArray+Additions.h"
#import "SCIProviderAgreementManager.h"
#import "ProviderAgreementInterfaceManager.h"
#import "SCICoreVersion.h"
#import "SCIProviderAgreementType.h"
#import "SCIVideoPlayer.h"
#import "SCIDefaults.h"
#import "SCICoreEvent.h"
#import "SCIPropertyNames.h"
#import "SCIPropertyManager+CustomTexts.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <CoreSpotlight/CoreSpotlight.h>
#import "ntouch-Swift.h"
#import "DebugViewController.h"
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>
#import "PropertyManager.h"
#import "UIViewController+Top.h"
#import "YelpManager.h"
#import "LaunchDarklyConfigurator.h"

#define kRetryModalViewSeconds 2.0
#define kRetryDetectDuplicateId 5.0
#define kUserPhoneNumber @"kUserPhoneNumber"
#define kUserPIN @"kUserPIN"
#define kUserAutoSignIn @"kUserAutoSignIn"
#define kUserSavePIN @"kUserSavePIN"
#define kUserMyRumblePattern @"kUserMyRumblePattern"
#define kHasMigratedFromCoreData @"kHasMigratedFromCoreData"
#define kLastTeaserViewTime @"kLastTeaserViewTime"

static int SAccountKVOContext = 0;

@interface SCIAccountManager ()

@property (nonatomic, readonly) SCIVideophoneEngine *engine;
@property (nonatomic, readonly) SCIDefaults	*defaults;
@property (nonatomic, strong) UIAlertController *phoneNumberChangedAlert;
@property (nonatomic, strong) SCICoreEvent *currentCoreEvent;
@property (nonatomic, assign) BOOL hasLoggedStartupMetrics;

@end


@implementation SCIAccountManager

@synthesize coreOptions;
@synthesize viewStates;
@synthesize coreIsConnected;
@synthesize agreementsAwaitingAcceptance = _agreementsAwaitingAcceptance;
@synthesize providerAgreementStatusAwaitingUpdate = _providerAgreementStatusAwaitingUpdate;
@synthesize providerAgreementTypeNumber = _providerAgreementTypeNumber;
@synthesize messageToLog;
@synthesize eulaAgreementState;
@synthesize redirectedCallAlert;
@synthesize askMeVcoAlert;

@synthesize updateVersion;
@synthesize updateVersionUrl;
@synthesize isCreatingMyPhoneGroup;
@synthesize endpointSelectionController;
@synthesize timeInterval;
@synthesize checkTime;
@synthesize checkForUpdateTimer;
@synthesize latestMissedCallNotification;

NSDate *clientAuthenticatedChangedDate;
int clientAuthenticatedChangedNumber;

static SCIAccountManager *sharedManager = nil;
+ (SCIAccountManager *)sharedManager
{
	if (!sharedManager)
		sharedManager = [[SCIAccountManager alloc] init];
	return sharedManager;
}

- (id)init
{
    self = [super init];
    if (self) {
		
	}
    return self;
}

#pragma mark - Accessors

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

- (UIViewController *)topViewController {
	return [UIViewController topViewController];
}

- (void)open {
	
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	LaunchDarklyConfigurator *ld = [LaunchDarklyConfigurator shared];
	
	[engine registerStandardPropertyManagerChangeNotifications];
	[engine startObservingRingGroupProperties];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyAuthenticatedDidChange:)
												 name:SCINotificationAuthenticatedDidChange
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyUserAccountInfoUpdated:)
												 name:SCINotificationUserAccountInfoUpdated
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyConnected:)
												 name:SCINotificationConnected
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyConnecting:)
												 name:SCINotificationConnecting
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyDisplayProviderAgreement:)
												 name:SCINotificationDisplayProviderAgreement
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyBlockedNumberWhitelisted:)
												 name:SCINotificationBlockedNumberWhitelisted
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyBlockedListItemAddError:)
												 name:SCINotificationBlockedListItemAddError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyContactListItemEditError:)
												 name:SCINotificationContactListItemEditError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyContactListItemDeleteError:)
												 name:SCINotificationContactListItemDeleteError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyBlockedListItemEditError:)
												 name:SCINotificationBlockedListItemEditError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyLoggedIn:)
												 name:SCINotificationLoggedIn
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyEmergencyAddressUpdated:)
												 name:SCINotificationEmergencyAddressUpdated
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(userLoggedIntoOtherDevice:)
												 name:SCINotificationUserLoggedIntoAnotherDevice
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupInvitationReceived:)
												 name:SCINotificationRingGroupInvitationReceived
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupRemoved:)
												 name:SCINotificationRingGroupRemoved
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyPasswordChanged:)
												 name:SCINotificationPasswordChanged
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupCreated:)
												 name:SCINotificationRingGroupCreated
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyPropertyChanged:)
												 name:SCINotificationPropertyChanged
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyVersionCheckRequired:)
												 name:SCINotificationUpdateVersionCheckRequired
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyFailedToEstablishTunnel:)
												 name:SCINotificationFailedToEstablishTunnel
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyCallCIRDismissible:)
												 name:SCINotificationSuggestionCallCIR
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyCallCIRNotDismissible:)
												 name:SCINotificationRequiredCallCIR
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyUserRegistrationDataRequired:)
												 name:SCINotificationUserRegistrationDataRequired
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyAgreementChanged:)
												 name:SCINotificationAgreementChanged
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyFavoriteListError:)
												 name:SCINotificationFavoriteListError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyFavoritesChanged:)
												 name:SCINotificationFavoritesChanged
											   object:[SCIContactManager sharedManager]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyConferencingReadyStateChanged:)
												 name:SCINotificationConferencingReadyStateChanged
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyCoreOfflineGenericError:)
												 name:SCINotificationCoreOfflineGenericError
											   object:engine];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyUpdateDeviceLocationType:) name:SCINotificationDeviceLocationTypeChanged object:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingsBeforeGreetingChanged:)
												 name:SCINotificationRingsBeforeGreetingChanged
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyTeaserVideoDetailsChanged:)
												 name:SCINotificationTeaserVideoDetailsChanged
											   object:ld];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifySignVideoUpdateRequired:)
												 name:SCINotificationSignVideoUpdateRequired
											   object:ld];
	
	[engine addObserver:self forKeyPath:SCIKeyProviderAgreementAccepted options:0
				context:&SAccountKVOContext];
	
	
	[engine addObserver:self forKeyPath:SCIKeyLastCIRCallTime options:0
				context:&SAccountKVOContext];
	
	self.coreIsConnected = NO;
	self.state = [NSNumber numberWithInteger:CSSignedOut];
	
	SCILog(@"APP coreOptions = %@", [self.coreOptions description]);
	[[SCIVideophoneEngine sharedEngine] launchWithOptions:self.coreOptions];
	
	// Set default Pulse notifications enabled in platform
	[[SCIVideophoneEngine sharedEngine] pulseEnable:self.defaults.pulseEnable];
	
	// Tell the manager classes to get cached data.
	[SCICallListManager.sharedManager refresh];
	[SCIContactManager.sharedManager refresh];
	
	// Grab the MAC address and set it for logging.
	NSString *uniqueIdentifier = @"";
	uniqueIdentifier = [[SCIVideophoneEngine sharedEngine] macAddress];
	if (uniqueIdentifier.length)
	{
		[[FIRCrashlytics crashlytics] setUserID:uniqueIdentifier];
	}
	
	NSString *version = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
	[[SCIVideophoneEngine sharedEngine] setSoftwareVersion:version];
	
	// Enable Call Waiting
	BOOL useCallWaiting = [[SCIVideophoneEngine sharedEngine] callWaitingEnabled];
	[[SCIVideophoneEngine sharedEngine] setCallWaitingEnabled:useCallWaiting];
	
	// Set rejectIncomingCalls if DND is set
	if ([[SCIVideophoneEngine sharedEngine] doNotDisturbEnabled]) {
		[[SCIVideophoneEngine sharedEngine] setRejectIncomingCalls:YES];
	}
	
	[appDelegate setProfileLevel];
	
	[SCIContactManager sharedManager];
	[[SCICallController shared] restorePrivacyFromDefaults];
	[SCIBlockedManager sharedManager];
	[SCIMessageViewer sharedViewer];
	[[SCIMessageViewer sharedViewer] startObservingNotifications];
	SCIProviderAgreementManager *providerAgreementManager = [SCIProviderAgreementManager sharedManager];
	providerAgreementManager.interfaceManager = [ProviderAgreementInterfaceManager sharedManager];
	
	[[SCICallListManager sharedManager] startObservingUserInterfaceProperties];
	self.latestMissedCallNotification = [[[SCICallListManager sharedManager] latestMissedCallTime] laterDate:[NSDate date]];
	[[SCICallListManager sharedManager] addObserver:self
										 forKeyPath:@"items"
											options:0
											context:&SAccountKVOContext];
	[[SCICallListManager sharedManager] addObserver:self
										 forKeyPath:SCICallListManagerKeyHasUnviewedMissedCalls
											options:0
											context:&SAccountKVOContext];
	
	// Configure update check to occur once a day.
	self.timeInterval = 3600.0 * 24.0;
	self.checkTime = nil;
	[self startUpdateCheckTimer];
	[self resetClientAuthenticatedChanged];
	[self autoSignIn:YES];
}

- (void)updateCheck:(NSTimer *)timer {
	if ([self.checkTime timeIntervalSinceNow] < 0.0) {
		SCILog(@"updateCheck executing");
		self.checkTime = [[NSDate date] dateByAddingTimeInterval:timeInterval];
		[self checkUpdateUserInitiated:NO];

		//Clear LDAP credentials once per day.  When user visits tab they will reload from LDAP server.
#ifdef stiLDAP_CONTACT_LIST
		[[SCIVideophoneEngine sharedEngine] clearLDAPCredentialsAndContacts];
#endif
	}
}

- (void)startUpdateCheckTimer {
	self.checkTime = [[NSDate date] dateByAddingTimeInterval:timeInterval];
	
	if (checkForUpdateTimer) {
		[checkForUpdateTimer invalidate];
		checkForUpdateTimer = nil;
	}

	dispatch_async(dispatch_get_main_queue(), ^() {
		self.checkForUpdateTimer = [NSTimer scheduledTimerWithTimeInterval:3600 target:self selector:@selector(updateCheck:) userInfo:nil repeats:YES];
	});
}

- (void) stopUpdateCheckTimer {
	[checkForUpdateTimer invalidate];
	checkForUpdateTimer = nil;
}

- (void)stopRemindingCallCIR
{
	[self clearSuggestCallCIRProperties];
}

- (void)clearSuggestCallCIRProperties
{
	[[SCIPropertyManager sharedManager] setLastRemindLaterTimeShowCallCIRSuggestion:nil];
}

- (void)remindMeLaterCheck {
	if (SCICallController.shared.isCallInProgress) {
		[self performSelector:@selector(remindMeLaterCheck) withObject:nil afterDelay:kRetryModalViewSeconds];
		return;
	}
	
	NSDate *callCIRDismissibleRemindLaterDate = [[SCIPropertyManager sharedManager] lastRemindLaterTimeShowCallCIRSuggestion];
	
	if (callCIRDismissibleRemindLaterDate != nil && [[NSDate date] timeIntervalSince1970] > [callCIRDismissibleRemindLaterDate timeIntervalSince1970]) {
		[self displayCallCIRDismissible];
	}
}

- (void)displayCallCIRDismissible {
	// Fix for 24192, Check if this VC is already being presented and bail out.  Manually heartbeating endpoint
	// causes remindMeLaterCheck to execute again before new LastRemindLaterTime property is downloaded and updated.
	UIViewController *presentedVC = appDelegate.tabBarController.presentedViewController;
	if (presentedVC && [presentedVC isKindOfClass:[UINavigationController class]]) {
		UINavigationController *nc = (UINavigationController *)presentedVC;
		if ([nc.topViewController isKindOfClass:[CallCIRViewController class]]) {
			return;
		}
	}
	
	if([self canHandleCoreEventNow:nil]) {
		CallCIRViewController *callCIRDismissible = [[CallCIRViewController alloc] initWithCallCIRType: CallCIRTypeDismissible];
		
		if (iPadB) {
			UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:callCIRDismissible];
			navController.modalPresentationStyle = UIModalPresentationFormSheet;
			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
		else {
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:callCIRDismissible];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
	}
	else {
		[self performSelector:@selector(displayCallCIRDismissible) withObject:nil afterDelay:kRetryModalViewSeconds];
		return;
	}
}

- (void)displayCallCIRNotDismissible {
	if([self canHandleCoreEventNow:nil]) {
		CallCIRViewController *callCIRNotDismissible = [[CallCIRViewController alloc] initWithCallCIRType: CallCIRTypeNotDismissible];
		
		if (iPadB) {
			UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:callCIRNotDismissible];
			navController.modalPresentationStyle = UIModalPresentationFormSheet;
			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
		else {
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:callCIRNotDismissible];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
	}
	else {
		[self performSelector:@selector(displayCallCIRNotDismissible) withObject:nil afterDelay:kRetryModalViewSeconds];
		return;
	}
}

- (void)displaySignVideoTeaser:(NSDictionary *)details {
	if (SCICallController.shared.isCallInProgress) {
		[self performSelector:@selector(displaySignVideoTeaser:) withObject:details afterDelay:5.0];
		return;
	}
	
	int timeInterval = [[details objectForKey:@"timeInterval"] intValue];
	
	if ([self shouldDisplayTeaserVideo:timeInterval]) {
		NSString *urlString;
		if ([[[NSLocale currentLocale] languageCode]isEqualToString:@"es"]) {
			urlString = [details objectForKey:@"spanishVideoUrl"];
		} else {
			urlString = [details objectForKey:@"englishVideoUrl"];
		}
		
		NSURL *url = [[NSURL alloc] initWithString:urlString];
		SignVideoTeaserViewController *vc = [[SignVideoTeaserViewController alloc] initWithUrl:url];
		
		if (iPadB) {
			UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:vc];
			[navController setNavigationBarHidden:YES];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			if (![self.topViewController.presentingViewController isKindOfClass:[UINavigationController class]]) {
				[self.topViewController presentViewController:navController animated:YES completion:nil];
			}
		}
		else {
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:vc];
			[navController setNavigationBarHidden:YES];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			if (![self.topViewController.presentingViewController isKindOfClass:[UINavigationControllerLocked class]]) {
				[self.topViewController presentViewController:navController animated:YES completion:nil];
			}
		}
	}
}

- (BOOL)shouldDisplayTeaserVideo:(int)timeInterval {
	int lastTeaserViewTime = [[NSUserDefaults standardUserDefaults] integerForKey:kLastTeaserViewTime];
	int now = [NSDate date].timeIntervalSince1970;
	int timeIntervalSeconds = timeInterval * 60 * 60;
	
	if (lastTeaserViewTime) {
		int intervalSinceLastViewedSeconds = now - lastTeaserViewTime;
		return intervalSinceLastViewedSeconds >= timeIntervalSeconds ? YES : NO;
	} else {
		return YES;
	}
}

- (void)displaySignVideoUpdateRequired:(NSDictionary *)details {
	if (SCICallController.shared.isCallInProgress) {
		[self performSelector:@selector(displaySignVideoUpdateRequired:) withObject:details afterDelay:5.0];
		return;
	}
	
	BOOL enabled = [[details objectForKey:@"enabled"] boolValue];
	if (enabled) {
		SignVideoUpdateRequiredViewController *vc = [[SignVideoUpdateRequiredViewController alloc] init];
		
		if (iPadB) {
			UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:vc];
			[navController setNavigationBarHidden:YES];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			if (![self.topViewController isKindOfClass:[SignVideoUpdateRequiredViewController class]]) {
				[self.topViewController presentViewController:navController animated:YES completion:nil];
			}
		} else {
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:vc];
			[navController setNavigationBarHidden:YES];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			if (![self.topViewController isKindOfClass:[SignVideoUpdateRequiredViewController class]]) {
				[self.topViewController presentViewController:navController animated:YES completion:nil];
			}
		}
	} else {
		if ([self.topViewController isKindOfClass:[SignVideoUpdateRequiredViewController class]]) {
			[self.topViewController dismissViewControllerAnimated:YES completion:nil];
		}
	}
}

- (void)displayUserRegistrationDataRequired {
	if([self canHandleCoreEventNow:nil]) {
		FCCUserRegistrationViewController *FCCUserRegistration = [[FCCUserRegistrationViewController alloc] init];
		
		if (iPadB) {
			DismissibleiPadKeyboardViewController *navController = [[DismissibleiPadKeyboardViewController alloc] initWithRootViewController:FCCUserRegistration];
			navController.modalPresentationStyle = UIModalPresentationFormSheet;
			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
		else {
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:FCCUserRegistration];
			navController.modalPresentationStyle = UIModalPresentationFullScreen;
			[self.topViewController presentViewController:navController animated:YES completion:nil];
		}
	}
	else {
		[self performSelector:@selector(displayUserRegistrationDataRequired) withObject:nil afterDelay:kRetryModalViewSeconds];
		return;
	}
}

- (void)displayPasswordChangedAlert {
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Password Changed")
																   message:Localize(@"sci.am.password.changed")
															preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")  style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
		[alert dismissViewControllerAnimated:YES completion:^{
			
		}];
		
	}]];
	
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}

- (void)close {
	[self signOut];
	[self stopUpdateCheckTimer];
	
	[[SCIVideophoneEngine sharedEngine] stopObservingRingGroupProperties];
	[[SCIMessageViewer sharedViewer] stopObservingNotifications];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)signIn:(NSString *)userName password:(NSString *)password usingCache:(BOOL)usingCache {
	[[SCINotificationManager shared] didStartNetworking];
	[self startLoginTimeout];
	self.engine.username = userName;
	self.engine.password = password;
	self.engine.loginName = nil;
	
	[appDelegate loginUsingCache: usingCache withImmediateCallback:^{
		self.state = [NSNumber numberWithInteger:CSSigningIn];
		[[SCINotificationManager shared] didStopNetworking];
	} errorHandler:^(NSError *error) {
		if (error)
		{
			SCILog(@"Failed to login. Error: %@", error);
			self.state = [NSNumber numberWithInteger:CSSignedOut];
			self.defaults.isAutoSignIn = NO;
			self.hasLoggedStartupMetrics = YES;
			[self cancelLogin];
			
			dispatch_async(dispatch_get_main_queue(), ^{
				switch (error.code) {
					case SCICoreResponseErrorCodeOfflineAuthentication: {
						Alert(Localize(@"Login Failed"), Localize(@"login.err.offlineAuthentication"));
						break;
					}
					case SCICoreResponseErrorCodeUsernameOrPasswordInvalid: {
						Alert(Localize(@"Login Failed"), Localize(@"login.err.usernameOrPasswordInvalid"));
						break;
					}
					case SCICoreResponseErrorCodeCannotDetermineWhichLogin: {
						// Do Nothing. Endpoint Selection UI will be presented.
						break;
					}
					default: {
						Alert(Localize(@"Login Failed"), [NSString stringWithFormat:@"%@ %@",  Localize(@"login.err.generic"), error.localizedDescription]);
						break;
					}
				}
			});
			return;
		}
	} successHandler:^{
		SCILog(@"Logged in!");
		self.defaults.phoneNumber = userName;
		self.defaults.PIN = password;
		[self cancelLoginTimeout];
		[self indexContacts];
		self.state = [NSNumber numberWithInteger:CSSignedIn];
		
		// Turn on autoSignIn.  User has signed in.
		self.defaults.isAutoSignIn = YES;
	}];
}

- (void)autoSignIn:(BOOL)useCache {
	if (self.defaults.isAutoSignIn && self.defaults.phoneNumber.length && self.defaults.PIN.length && self.defaults.wizardCompleted) {
		[self signIn:self.defaults.phoneNumber password:self.defaults.PIN usingCache:useCache];
	}
	else {
		[self presentAccountViewWithCompletion:nil];
	}
}

- (void)signOut {
	[[AnalyticsManager shared] stop];
	[[SCIENSController shared] unregister];
	[[SCIVideophoneEngine sharedEngine] logout];
	
	self.currentCoreEvent = nil;
	self.state = [NSNumber numberWithInteger:CSSignedOut];
	
	// Blank pin on log off if Remember Password is not enabled.
	if(!self.defaults.savePIN)
		self.defaults.PIN = @"";
	
	// Dont autoSignIn next time, user has chosen to logout or logged out by core.
	self.defaults.isAutoSignIn = NO;
	[self resetClientAuthenticatedChanged];
}

- (void)presentAccountViewWithCompletion:(void (^)())block {
	if (SCICallController.shared.isEmergencyCallInProgress || [SCIVideoPlayer sharedManager].isPlaying) {
		[self performSelector:@selector(presentAccountViewWithCompletion:) withObject:block afterDelay:kRetryModalViewSeconds];
	}
	else if (SCICallController.shared.isCallInProgress) {
		[SCICallController.shared endAllCalls];
		[self performSelector:@selector(presentAccountViewWithCompletion:) withObject:block afterDelay:kRetryModalViewSeconds];
	}
	else {
		// If we're still Signing In, restart loginTimeOut.  A login error will reset us back to SignedOut state.
		if (self.state == [NSNumber numberWithInteger:CSSigningIn]) {
			NSLog(@"presentAccountView: Still logging in, restart loginTimeOut");
			[self startLoginTimeout];
			return;
		}
		
		// Update Required UI is up, dont display login.
		if (appDelegate.updateRequired) {
			return;
		}
		
		dispatch_async(dispatch_get_main_queue(), ^{
			
			[[SCIVideophoneEngine sharedEngine] setMaxCalls: 1];
			
			SCILoginViewController *loginViewController = [[SCILoginViewController alloc] init];
			UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:loginViewController];
			navController.navigationBarHidden = YES;

			if (@available(iOS 13.0, *)) {
				navController.modalInPresentation = YES;
			}
			
			if (iPadB) {
				navController.modalPresentationStyle = UIModalPresentationFormSheet;
			}
			else {
				navController.modalPresentationStyle = UIModalPresentationFullScreen;
			}
			
			UIViewController *rootViewController;
			if (@available(iOS 13.0, *))
			{
				rootViewController = [UIApplication.sharedApplication.windows firstObject].rootViewController;
			}
			else
			{
				rootViewController = UIApplication.sharedApplication.keyWindow.rootViewController;
			}
			if (rootViewController.presentedViewController) {
				[rootViewController.presentedViewController dismissViewControllerAnimated:YES completion:^{
					[rootViewController presentViewController:navController animated:YES completion:^{
						if (block)
							block();
					}];
				}];
			}
			else {
				[rootViewController presentViewController:navController animated:YES completion:^{
					if (block)
						block();
				}];
			}
		});
	}
}

- (void) cancelLogin {
	SCILog(@"Start");
	[self cancelLoginTimeout];
	self.state = [NSNumber numberWithInteger:CSSignedOut];
	self.defaults.isAutoSignIn = NO;
	self.hasLoggedStartupMetrics = YES;
	[self presentAccountViewWithCompletion:nil];
}

- (void) cancelLoginTimeout {
	SCILog(@"Start");
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelLogin) object:nil];
}

- (void) startLoginTimeout {
	SCILog(@"Start");
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelLogin) object:nil];
	[self performSelector:@selector(cancelLogin) withObject:nil afterDelay:55.0];
}

- (void)resetClientAuthenticatedChanged {
	clientAuthenticatedChangedNumber = 0;
	clientAuthenticatedChangedDate = nil;
}

- (void)detectDuplicateHardwareId {
	[self performSelector:@selector(resetClientAuthenticatedChanged) withObject:nil afterDelay:kRetryDetectDuplicateId];
	clientAuthenticatedChangedNumber++;
	
	if (clientAuthenticatedChangedDate) {
		clientAuthenticatedChangedDate = [NSDate date];
	}
	
	NSTimeInterval timePassedSinceFirstUpdated = [clientAuthenticatedChangedDate timeIntervalSinceNow] * -1;

	// If this scenario occurs, it's likely we're trying to use the same MAC address that's being used on a different device
	if (timePassedSinceFirstUpdated <= kRetryDetectDuplicateId && clientAuthenticatedChangedNumber >= 5) {
		[self resetClientAuthenticatedChanged];
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Update Device Info")
																	   message:Localize(@"duplicate.mac.detected.msg")
																preferredStyle:UIAlertControllerStyleAlert];
		
		[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")
												  style:UIAlertActionStyleDefault
												handler:^(UIAlertAction * _Nonnull action) {
			[self cancelLogin];
			[self signOut];
			[SCIVideophoneEngine.sharedEngine resetNetworkMACAddress];
		}]];
		[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
	}
}

- (BOOL)eulaWasRejected {
	return (self.eulaAgreementState == SCIProviderAgreementStateRejected);
}

#pragma mark CoreEvent queue manager

- (BOOL)playingAVideo {
	return [SCIVideoPlayer sharedManager].isPlaying;
}

- (BOOL)canHandleCoreEventNow:(SCICoreEvent *)event {
	
	if (event && event.type.integerValue == coreEventShowProviderAgreement)
	{
		return (![self playingAVideo] &&
				!SCICallController.shared.isCallInProgress &&
				!appDelegate.tabBarController.presentedViewController &&
				self.engine.isAuthorized &&
				!self.engine.isLoggingOut &&
				!self.currentCoreEvent &&
				[UIApplication sharedApplication].applicationState != UIApplicationStateBackground);
	}
	else
	{
		return (![self playingAVideo] &&
				!SCICallController.shared.isCallInProgress &&
				!self.currentCoreEvent &&
				!appDelegate.tabBarController.presentedViewController);
	}
}

- (void)handleCoreEventNow:(SCICoreEvent *)coreEvent {
	// always call canHandleCoreEventNow first
	
	if (!coreEvent)
		return;
	
	switch (coreEvent.type.integerValue) {
			
		case coreEventUpdateRequired: {
			[appDelegate presentUpdateRequired];
			[self didFinishCoreEventWithType:coreEventUpdateRequired];
		} break;
			
		case coreEventPasswordChanged: {
			self.defaults.PIN = @"";
			self.defaults.isAutoSignIn = NO;
			[[SCIVideophoneEngine sharedEngine] logout];
			[self didFinishCoreEventWithType:coreEventPasswordChanged];
	
			self.state = [NSNumber numberWithInteger:CSSignedOut];
			[self presentAccountViewWithCompletion:^{
				[self displayPasswordChangedAlert];
			}];
		} break;
		
		case coreEventAddressChanged: {
			if (self.coreIsConnected) {
				[[SCIVideophoneEngine sharedEngine] loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:^(NSError * error)  {
					EmergencyInfoViewController *controller = [[EmergencyInfoViewController alloc] initWithStyle:UITableViewStyleGrouped];
					appDelegate.emergencyInfoViewController = controller;
					controller.isModal = YES;
					UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:controller];
					
					if (@available(iOS 13.0, *)) {
						navController.modalInPresentation = YES;
					}
					
					if (iPadB) {
						navController.modalPresentationStyle = UIModalPresentationFormSheet;
					}
					else {
						navController.modalPresentationStyle = UIModalPresentationFullScreen;
					}
					
					UIViewController *rootViewController;
					if (@available(iOS 13.0, *))
					{
						rootViewController = [UIApplication.sharedApplication.windows firstObject].rootViewController;
					}
					else
					{
						rootViewController = UIApplication.sharedApplication.keyWindow.rootViewController;
					}
					if (rootViewController.presentedViewController) {
						[rootViewController.presentedViewController dismissViewControllerAnimated:YES completion:^{
							[rootViewController presentViewController:navController animated:YES completion:nil];
						}];
					}
					else {
						[rootViewController presentViewController:navController animated:YES completion:nil];
					}
				}];
			}
		} break;
			
		case coreEventNewVideo: {
			// Dont switch tabs automatically anymore, tabs will switch if user opens app from notification.
			[self didFinishCoreEventWithType:coreEventNewVideo];
		} break;
		case coreEventShowProviderAgreement: {
			if (self.engine.isAuthorized && !self.engine.isLoggingOut) {
				SCIProviderAgreementType type = ^{
					SCIProviderAgreementType type = SCIProviderAgreementTypeNone;
					
					NSNumber *providerAgreementTypeNumber = self.providerAgreementTypeNumber;
					if (providerAgreementTypeNumber) {
						self.providerAgreementTypeNumber = nil;
						type = (SCIProviderAgreementType)providerAgreementTypeNumber.unsignedIntegerValue;
					} else {
						type = SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(self.engine.coreVersion, self.engine.dynamicAgreementsFeatureEnabled);
					}
					
					return type;
				}();
				switch (type) {
					case SCIProviderAgreementTypeStatic: {
						__weak SCIAccountManager *weak_self = self;
						[[SCIProviderAgreementManager sharedManager] showInterfaceAsPartOfLogInProcessForStaticProviderAgreement:self.providerAgreementStatusAwaitingUpdate
																										  agreementAcceptedBlock:^(BOOL result) {
																											  __strong SCIAccountManager *strong_self = weak_self;
																											  strong_self.providerAgreementStatusAwaitingUpdate = nil;
																											  if (result) {
																												  strong_self.eulaAgreementState = SCIProviderAgreementStateFromSCIStaticProviderAgreementState(SCIStaticProviderAgreementStateAccepted);
																												  [strong_self didFinishCoreEventWithType:coreEventShowProviderAgreement];
																											  } else  {
																												  [strong_self clientReauthenticate];
																											  }
																										  }];
					} break;
					case SCIProviderAgreementTypeDynamic: {
						__weak SCIAccountManager *weak_self = self;
						[[SCIProviderAgreementManager sharedManager] showInterfaceAsPartOfLogInProcessForDynamicProviderAgreements:self.agreementsAwaitingAcceptance
																										   agreementsAcceptedBlock:^(BOOL result) {
																											   __strong SCIAccountManager *strong_self = weak_self;
																											   strong_self.agreementsAwaitingAcceptance = nil;
																											   if (result) {
																												   strong_self.eulaAgreementState = SCIProviderAgreementStateFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementStateAccepted);
																												   [strong_self didFinishCoreEventWithType:coreEventShowProviderAgreement];
																											   } else {
																												   [strong_self clientReauthenticate];
																											   }
																										   }];
					} break;
					case SCIProviderAgreementTypeNone:
					case SCIProviderAgreementTypeUnknown: {
						//Do nothing.
					} break;
				}
			}
		}
			break;
		case coreEventLogMessage:
		{
			// If we have a message log it.
			if (messageToLog.length)
			{
				[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:messageToLog];
				messageToLog = @"";
			}
			[self didFinishCoreEventWithType:coreEventLogMessage];
			break;
		}
		case coreEventUIAddContact:
		{
			[(MainTabBarController *)appDelegate.tabBarController selectWithTab:TabContacts];
			UINavigationController *navController = (UINavigationController *)appDelegate.tabBarController.selectedViewController;
			[(PhonebookViewController *)navController.viewControllers[0] addNewContact];
			[self didFinishCoreEventWithType:coreEventUIAddContact];
		}
			break;
		default:
			break;
	}
}

- (void)tryToHandleNextCoreEventNow {
	SCILog(@"Start");
	if ([self.defaults.coreEvents count] > 0) {
			NSMutableArray *prioritizedEvents = [NSMutableArray arrayWithArray:self.defaults.coreEvents];
			NSSortDescriptor *descriptor = [NSSortDescriptor sortDescriptorWithKey:@"priority" ascending:YES];
			[prioritizedEvents sortUsingDescriptors:[NSArray arrayWithObject:descriptor]];
		
		if ([self canHandleCoreEventNow:[prioritizedEvents objectAtIndex:0]]) {
			self.currentCoreEvent = [prioritizedEvents objectAtIndex:0];
			if (self.currentCoreEvent)
				[self handleCoreEventNow:self.currentCoreEvent];
		}
		else
		{
			// try again later
			[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(tryToHandleNextCoreEventNow) object:nil];
			[self performSelector:@selector(tryToHandleNextCoreEventNow) withObject:nil afterDelay:kRetryModalViewSeconds];
		}
	}
}

- (void)addCoreEventWithType:(CoreEventType)coreEventType {
	
	// make sure the event type is not already in the queue
	for (SCICoreEvent *coreEvent in self.defaults.coreEvents) // save temp copy?
		if (coreEvent.type.integerValue == coreEventType) {
			SCILog(@"addCoreEventWithType is ignoring a duplicate CoreEvent of type: %d", coreEventType);
			return;
		}
	
	// persist the event in case the user kills the app
	
	NSMutableArray *newCoreEvents = self.defaults.coreEvents.mutableCopy;
	
	if (!newCoreEvents) {
		newCoreEvents = [[NSMutableArray alloc] init];
	}
	
	SCICoreEvent *coreEvent = [[SCICoreEvent alloc] init];
	coreEvent.type = [NSNumber numberWithInteger:coreEventType];
	coreEvent.priority = [NSNumber numberWithInteger:2];
	BOOL forceSaveData = YES;
	switch (coreEventType) {
		case coreEventUpdateRequired:
			coreEvent.priority = [NSNumber numberWithInteger:0];
			forceSaveData = NO;
			break;
		case coreEventPasswordChanged:
			coreEvent.priority = [NSNumber numberWithInteger:1];
			break;
		case coreEventAddressChanged:
			coreEvent.priority = [NSNumber numberWithInteger:2];
			break;
		case coreEventShowProviderAgreement:
			coreEvent.priority = [NSNumber numberWithInteger:2];
			break;
		case coreEventNewVideo:
			coreEvent.priority = [NSNumber numberWithInteger:2];
			forceSaveData = NO;
		case coreEventLogMessage:
			coreEvent.priority = [NSNumber numberWithInteger:2];
			break;
		case coreEventUIAddContact:
			coreEvent.priority = [NSNumber numberWithInteger:3];
			break;
		default:
			coreEvent.priority = [NSNumber numberWithInteger:3];
			break;
	}
	[newCoreEvents addObject:coreEvent];
	self.defaults.coreEvents = newCoreEvents;
	
	[self tryToHandleNextCoreEventNow];
}

- (void)didFinishCoreEventWithType:(CoreEventType)coreEventType {
	if (self.currentCoreEvent) {
		NSMutableArray *newCoreEvents = self.defaults.coreEvents.mutableCopy;
		SCICoreEvent *eventToRemove = nil;
		
		for (SCICoreEvent *coreEvent in newCoreEvents)
			if (coreEvent.type == [NSNumber numberWithInteger:coreEventType])
				eventToRemove = coreEvent;
		
		if (eventToRemove) {
			[newCoreEvents removeObject:eventToRemove];
		}

		self.currentCoreEvent = nil;
		self.defaults.coreEvents = newCoreEvents;
		[self tryToHandleNextCoreEventNow];
	}
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if (object == [SCIVideophoneEngine sharedEngine] && context == &SAccountKVOContext)  // Handled by Notification
	{
		if ([keyPath isEqualToString:SCIKeyLastCIRCallTime]) {
			[[SCIPropertyManager sharedManager] setLastRemindLaterTimeShowCallCIRSuggestion:nil];
		}
	}
}

#pragma mark - SCIVideophoneEngine Notifications

- (void)notifyAuthenticatedDidChange:(NSNotification *)note // SCINotificationAuthenticatedDidChange
{
//	[self detectDuplicateHardwareId]; // Disabling for now.  This is interfering with myPhone Add/Remove
	if ([SCIVideophoneEngine sharedEngine].isAuthenticated)
	{
		self.state = [NSNumber numberWithInteger:CSSignedIn];
		[self cancelLoginTimeout];
		
		if (SCIVideophoneEngine.sharedEngine.ringsBeforeGreeting < 5) {
			[SCIVideophoneEngine.sharedEngine setRingsBeforeGreetingPrimitive:5];
		}
	}
	else
	{
		self.state = [NSNumber numberWithInteger:CSSignedOut];
	}
}

- (void)notifyUserAccountInfoUpdated:(NSNotification *)note // SCINotificationUserAccountInfoUpdated
{
	SCILog(@"Start");
	SCIUserAccountInfo *info =[[SCIVideophoneEngine sharedEngine] userAccountInfo];
	BOOL isRingGroup = ((info.ringGroupLocalNumber && info.ringGroupLocalNumber.length > 0) ||
						(info.ringGroupTollFreeNumber && info.ringGroupTollFreeNumber.length > 0));
	NSString *activeNumber =  info.localNumber;
	
	[self tryToHandleNextCoreEventNow];
	
	if (!isRingGroup && activeNumber && self.defaults.phoneNumber && [activeNumber length] && [self.defaults.phoneNumber length]) {
		if (![self.defaults.phoneNumber isEqualToString:activeNumber]) {
			SCILog(@"Phone number change detected");
			self.defaults.phoneNumber = activeNumber;
			[self displayPhoneNumberChangedAlert];
		}
	}
}

- (void)notifyConnected:(NSNotification *)note // SCINotificationConnected
{
	SCILog(@"Start");
	[self coreServiceConnected];
}

- (void)notifyConnecting:(NSNotification *)note // SCINotificationConnecting
{
	SCILog(@"Start");
	[self coreServiceDisconnected];
}

- (void)notifyDisplayProviderAgreement:(NSNotification *)note // SCINotificationDisplayProviderAgreement
{
	SCILog(@"Start");
	SCIInterfaceMode interfaceMode = [[SCIVideophoneEngine sharedEngine] interfaceMode];
	if (interfaceMode != SCIInterfaceModePublic &&
		interfaceMode != SCIInterfaceModeInterpreter &&
		interfaceMode != SCIInterfaceModeTechSupport &&
		interfaceMode != SCIInterfaceModePorted) {
		[[SCIVideophoneEngine sharedEngine] setProviderAgreementAcceptedPrimitive:NO completionHandler:^(NSError *error) {
			if (!error) {
				[self displayProviderAgreement];
			} else {
				SCILog(@"Failed to reset the provider agreement.");
			}
		}];
	}
}

- (void)notifyAgreementChanged:(NSNotification *)note // SCINotificationAgreementChanged
{
	[self displayProviderAgreement];
}

- (void)notifyBlockedNumberWhitelisted:(NSNotification *)note // SCINotificationBlockedNumberWhitelisted
{
	SCILog(@"Start");
	Alert(Localize(@"blocked.add.error.title"), Localize(@"blocked.add.error.msg"));
}

- (void)notifyBlockedListItemAddError:(NSNotification *)note // SCINotificationBlockedListItemAddError
{
	SCILog(@"Start");
	Alert(Localize(@"blocked.add.error.title"), Localize(@"blocked.add.error.msg"));
}

- (void)notifyContactListItemEditError:(NSNotification *)note  // SCINotificationContactListItemEditError
{
	SCILog(@"Start");
	Alert(Localize(@"contact.edit.error.title"), Localize(@"contact.edit.error.msg"));
}

- (void)notifyContactListItemDeleteError:(NSNotification *)note  // SCINotificationContactListItemDeleteError
{
	SCILog(@"Start");
	Alert(Localize(@"contact.delete.error.title"), Localize(@"contact.delete.error.msg"));
}

- (void)notifyBlockedListItemEditError:(NSNotification *)note  // SCINotificationBlockedListItemEditError
{
	SCILog(@"Start");
	Alert(Localize(@"blocked.edit.error.title"), Localize(@"blocked.edit.error.msg"));
}

- (void)notifyCannotDetermineEndpoint:(NSNotification *)note // SCINotificationCannotDetermineEndpoint
{
	SCILog(@"Start");
	if (!self.endpointSelectionController) {
		endpointSelectionController = [[EndpointSelectionController alloc] init];
		endpointSelectionController.ringGroupInfos = [[note userInfo] objectForKey:SCINotificationKeyRingGroupInfos];
		[endpointSelectionController showEndpointSelectionMenu];
	}
}

- (void)notifyLoggedIn:(NSNotification *)note // SCINotificationLoggedIn
{
	NSLog(@"ntouch - Logged In");
#ifndef DEBUG
	NSString *groupUserId = [SCIVideophoneEngine sharedEngine].ringGroupUserId;
	NSString *phoneUserId = [SCIVideophoneEngine sharedEngine].phoneUserId;
	
	if (groupUserId.length > 0) {
		[[AnalyticsManager shared] startWithVisitorID:groupUserId];
	}
	else if (phoneUserId.length > 0) {
		[[AnalyticsManager shared] startWithVisitorID:phoneUserId];
	}
#endif

	SCIInterfaceMode interfaceMode = [SCIVideophoneEngine sharedEngine].interfaceMode;

	[self cancelLoginTimeout];
	[self remindMeLaterCheck];
	[self performSelector:@selector(tryToHandleNextCoreEventNow) withObject:nil afterDelay:kRetryModalViewSeconds];
	self.state = [NSNumber numberWithInteger:CSSignedIn];
	
	NSString *token = [SCIVideophoneEngine sharedEngine].deviceToken;
	[[SCIVideophoneEngine sharedEngine] sendDeviceToken:token];
	
	// Set "PushNotificationEnabledTypes" property in core to 0 when in Hearing Interface mode
	if (interfaceMode == SCIInterfaceModePublic) {
		[self.engine sendNotificationTypes:0];
	}
	else {
		// Send push notifcation type setting
		[appDelegate sendNotificationTypes];

	}
	
	// Save number of Saved Texts to Core
	NSInteger savedTextCount = [[SCIPropertyManager sharedManager] countOfValidInterpreterTexts];
	[[SCIVideophoneEngine sharedEngine] setSavedTextsCountPrimitive:savedTextCount];
	
	// Reconfigure UI for Public interface mode
	if (interfaceMode == SCIInterfaceModePublic) {
		[appDelegate configureForPublicMode];

	}
    #ifdef stiDEBUG
        [[SCIVideophoneEngine sharedEngine] setVrclEnabledPrimitive:YES];
    #endif
	
	[self setAudioAndVCOEnabled:self.audioAndVCOEnabled];
	
	// If app is in the foreground and logging in, request Location Services authorization
	if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined &&
		[UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		[[YelpManager sharedManager] requestWhenInUseAuthorization];
	}
	
	// Request user permission to receive Push Notifications
	[SCINotificationManager.shared requestPermissions];
}

- (void)notifyTeaserVideoDetailsChanged:(NSNotification *)note { // SCINotificationTeaserVideoDetailsChanged
	NSDictionary *details = note.userInfo;
	if (details) {
		NSNumber *enabled = [details objectForKey:@"enabled"];
		if ([enabled boolValue]) {
			dispatch_async(dispatch_get_main_queue(), ^{
				[self displaySignVideoTeaser:details];
			});
		}
	}
}

- (void)notifySignVideoUpdateRequired:(NSNotification *)note { // SCINotificationSignVideoUpdateRequired
	if (@available(iOS 15.1, *)) {
		dispatch_async(dispatch_get_main_queue(), ^{
			[self displaySignVideoUpdateRequired:note.userInfo];
		});
	}
}

-(void)indexContacts
{
	
	if([CSSearchableIndex class])
	{
		[[CSSearchableIndex defaultSearchableIndex] deleteSearchableItemsWithDomainIdentifiers:[NSArray arrayWithObject:@"com.sorenson.ntouch-contact"] completionHandler:^(NSError *__nullable error) {
			NSLog(@"Old contacts removed from index"); //prevent double entries
		}];
		
		NSArray *tempContacts = [SCIContactManager sharedManager].compositeContacts;
		for(SCIContact *contact in tempContacts)
		{
			NSMutableArray *searchKeys = [[NSMutableArray alloc] init];
			if ([contact.homePhone length] > 0 && [contact.homePhone characterAtIndex:0] == '1') {
				[searchKeys addObject:[UnformatPhoneNumber(contact.homePhone) substringFromIndex:1]];
				[searchKeys addObject:UnformatPhoneNumber(contact.homePhone)];
			}
			else if ([contact.homePhone length] > 0) {
				[searchKeys addObject:UnformatPhoneNumber(contact.homePhone)];
			}
			if ([contact.workPhone length] > 0 && [contact.workPhone characterAtIndex:0] == '1') {
				[searchKeys addObject:[UnformatPhoneNumber(contact.workPhone) substringFromIndex:1]];
				[searchKeys addObject:UnformatPhoneNumber(contact.workPhone)];
			}
			else if ([contact.workPhone length] > 0) {
				[searchKeys addObject:UnformatPhoneNumber(contact.workPhone)];
			}
			if ([contact.cellPhone length] > 0 && [contact.cellPhone characterAtIndex:0] == '1') {
				[searchKeys addObject:[UnformatPhoneNumber(contact.cellPhone) substringFromIndex:1]];
				[searchKeys addObject:UnformatPhoneNumber(contact.cellPhone)];
			}
			else if ([contact.cellPhone length] > 0) {
				[searchKeys addObject:UnformatPhoneNumber(contact.cellPhone)];
			}
			if (contact.name.length > 0 && contact.companyName)
			{
				[searchKeys addObject:contact.companyName];
			}
			
			
			if (searchKeys.count > 0) {
				CSSearchableItemAttributeSet *searchableItemDetails = [[CSSearchableItemAttributeSet alloc] initWithItemContentType:(NSString *)kUTTypeContact];
				searchableItemDetails.displayName = contact.nameOrCompanyName;
				searchableItemDetails.phoneNumbers = searchKeys;

				CSSearchableItem *item = [[CSSearchableItem alloc] initWithUniqueIdentifier:[searchKeys objectAtIndex:0] domainIdentifier:@"com.sorenson.ntouch-contact" attributeSet:searchableItemDetails];
				[[CSSearchableIndex defaultSearchableIndex] indexSearchableItems:@[item] completionHandler: ^(NSError * __nullable error) {
					NSLog(@"Search item indexed");
				}];
			}
		}
	}
}

- (void)notifyConferencingReadyStateChanged:(NSNotification *)note // SCINotificationConferencingReadyStateChanged
{
	NSNumber *readyState = [note.userInfo objectForKey:SCINotificationKeyConferencingReadyState];
	if (readyState && [readyState boolValue] == YES) {
		// Log app startup and login time
		if (!self.hasLoggedStartupMetrics && self.defaults.isAutoSignIn && self.defaults.phoneNumber.length && self.defaults.PIN.length) {
			[appDelegate logStartUpMetrics];
			self.hasLoggedStartupMetrics = YES;
		}
	}
}


- (void)notifyEmergencyAddressUpdated:(NSNotification *)note //SCINotificationEmergencyAddressUpdated
{
	SCILog(@"Start");
	SCIEmergencyAddress *emergencyAddress = [[SCIVideophoneEngine sharedEngine] emergencyAddress];
	SCIEmergencyAddressProvisioningStatus status = [emergencyAddress status];
	
	if ((status == SCIEmergencyAddressProvisioningStatusUpdatedUnverified) ||
		(status == SCIEmergencyAddressProvisioningStatusNotSubmitted)) {
		
		[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"addressChanged"]];
		
		UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
		notification.title = Localize(@"911.notification.title");
		notification.sound = UNNotificationSound.defaultSound;
		notification.userInfo = @{
			@"CoreEvent": @(coreEventAddressChanged)
		};
		
		UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"addressChanged" content:notification trigger:nil];
		[UNUserNotificationCenter.currentNotificationCenter
		 addNotificationRequest:request
		 withCompletionHandler:^(NSError * _Nullable error) {
			 if (error != nil) {
				 NSLog(@"Error posting notification: %@", error);
			 }
		 }];
		
		[self addCoreEventWithType:coreEventAddressChanged];
	}
}

- (void)notifyRingGroupInvitationReceived:(NSNotification *)note // SCINotificationRingGroupInvitationReceived
{
	SCILog(@"Start");
	if (SCICallController.shared.isCallInProgress || [self playingAVideo] ||
		[UIApplication sharedApplication].applicationState != UIApplicationStateActive ||
		self.eulaAgreementState != SCIProviderAgreementStateAccepted)
	{
		// A user cannot join a myPhone group without having first accepted the
		// provider agreement. BUG: 23315. This notification is received before
		// ProviderAgreementStatus is received.  Delay this handler until self.eulaAgreementState is set.
		[self performSelector:@selector(notifyRingGroupInvitationReceived:) withObject:note afterDelay:5.0];
		return;
	}
	
	NSString *name = [[note userInfo] objectForKey:SCINotificationKeyName];
	NSString *localPhone = [[note userInfo] objectForKey:SCINotificationKeyLocalPhone];
	NSString *tollFreePhone = [[note userInfo] objectForKey:SCINotificationKeyTollFreePhone];
	
	MyPhoneInviteViewController *inviteViewController = [[MyPhoneInviteViewController alloc] init];
	inviteViewController.fromName = name;
	inviteViewController.fromLocalNumber = localPhone;
	inviteViewController.fromTollFreeNumber = tollFreePhone;
	UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:inviteViewController];
	navController.modalPresentationStyle = UIModalPresentationFormSheet;
	[self.topViewController presentViewController:navController animated:YES completion:nil];
}

- (void)notifyRingGroupRemoved:(NSNotification *)note // SCINotificationRingGroupRemoved
{
	SCILog(@"Start");
	if (SCICallController.shared.isCallInProgress || [self playingAVideo]) {
		[self performSelector:@selector(notifyRingGroupRemoved:) withObject:note afterDelay:5.0];
		return;
	}
	else {
		// Restore previous account phone number, optionally restore
		// previous account PIN if Save Password option is enabled.
		NSString *oldPhone = [[[SCIVideophoneEngine sharedEngine] userAccountInfo] localNumber];
		NSString *oldPin = @"";
		NSArray *oldAccounts = [SCIDefaults sharedDefaults].accountHistoryList;
		for (SCIAccountObject *accountObj in oldAccounts) {
			if ([accountObj.phoneNumber isEqualToString:oldPhone] &&
				[SCIDefaults sharedDefaults].savePIN) {
				oldPin = accountObj.PIN;
			}
		}
		
		self.defaults.phoneNumber = oldPhone;
		self.defaults.PIN = oldPin;
		
		[[SCIVideophoneEngine sharedEngine] logoutWithBlock:^{
			self.state = [NSNumber numberWithInteger:CSSignedOut];
			
			NSString *noteTitle = Localize(@"myphone.group.phone.removed.title");
			NSString *noteMsg = Localize(@"myphone.group.phone.removed.msg");
			
			// Attempt to auto sign-in using old account info, otherwise display Login UI
			[self autoSignIn:NO];
			
			[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"ringGroupRemoved"]];
			UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
			notification.title = noteTitle;
			notification.body = noteMsg;
			notification.categoryIdentifier = @"MYPHONE_MEMBER_REMOVED";
			notification.sound = UNNotificationSound.defaultSound;
			UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"ringGroupRemoved" content:notification trigger:nil];
			[UNUserNotificationCenter.currentNotificationCenter
			 addNotificationRequest:request
			 withCompletionHandler:^(NSError * _Nullable error) {
				 if (error != nil) {
					 NSLog(@"Error posting notification: %@", error);
				 }
			 }];
		}];
	}
}

- (void)notifyPasswordChanged:(NSNotification *)note // SCINotificationPasswordChanged
{
	SCILog(@"Start");
	NSDictionary *userInfo = note.userInfo;
	NSString *user = userInfo[@"user"];
	NSString *pass = userInfo[@"password"];

	if (self.isCreatingMyPhoneGroup) {
		if (userInfo && self.state.integerValue == CSSignedIn) {
			if ([user isEqualToString:self.defaults.phoneNumber] &&
				![pass isEqualToString:self.defaults.PIN]) {
				self.defaults.PIN = pass;
				self.defaults.phoneNumber = user;
				
				[[SCIVideophoneEngine sharedEngine] logoutWithBlock:^{
					self.currentCoreEvent = nil;
					self.state = [NSNumber numberWithInteger:CSSignedOut];
					[self autoSignIn:NO];
					[self resetClientAuthenticatedChanged];
					[self displayMyphoneWelcomeAlert];
				}];
			}
		}
	}
	else {
		if (userInfo && self.state.integerValue == CSSignedIn) {
			if ([user isEqualToString:self.defaults.phoneNumber] &&
				![pass isEqualToString:self.defaults.PIN]) {
				[self clientPasswordChanged];
			}
		}
		else {
			[self performSelector:@selector(displayPasswordChangedAlert) withObject:nil afterDelay:2.0];
		}
	}
}

- (void)notifyRingGroupCreated:(NSNotification *)note // SCINotificationRingGroupCreated
{
	SCILog(@"Start");
	if (SCICallController.shared.isCallInProgress || [self playingAVideo]) {
		[self performSelector:@selector(notifyRingGroupCreated:) withObject:note afterDelay:5.0];
		return;
	}
	self.isCreatingMyPhoneGroup = YES;
}

- (void)displayMyphoneWelcomeAlert {
	NSString *title = Localize(@"myphone.group.welcome.title");
	NSString *message = Localize(@"myphone.group.welcome.msg");
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Learn More") style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
		self.isCreatingMyPhoneGroup = NO;
		NSURL *userGuideURL = [NSURL URLWithString:@"https://www.sorensonvrs.com/user_guide"];
		[[UIApplication sharedApplication] openURL:userGuideURL options:{} completionHandler:nil];
	}]];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
		self.isCreatingMyPhoneGroup = NO;
	}]];
	
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}

- (void)notifyPropertyChanged:(NSNotification *)note // SCINotificationPropertyChanged
{
	const char *propertyName = [[note.userInfo objectForKey:SCINotificationKeyName] UTF8String];
	SCILog(@"PropertyManager property %s changed.", propertyName);
	if (strcmp(propertyName, PropertyDef::VideoMessageEnabled) == 0)
	{
		[appDelegate updateVideoCenterTab];
	}
	else if (strcmp(propertyName, [SCIPropertyNameDoNotDisturbEnabled UTF8String]) == 0)
	{
		BOOL dnd = [SCIVideophoneEngine sharedEngine].doNotDisturbEnabled;
		[[SCIVideophoneEngine sharedEngine] setDoNotDisturbEnabled:dnd];
		if (dnd == YES) {
			[SCIENSController.shared unregister];
		}
	}
	else if (strcmp(propertyName, [SCIPropertyNameVideoMessageEnabled UTF8String]) == 0)
	{
		[appDelegate updateVideoCenterTab];
	}
	else if (strcmp(propertyName, [SCIPropertyNameVCOPreference UTF8String]) == 0)
	{
		[SCIVideophoneEngine sharedEngine].audioEnabled = [self audioAndVCOEnabled];
		[[SCIVideophoneEngine sharedEngine] updateEngineForVCOType:self.engine.voiceCarryOverType];
	}
	else if (strcmp(propertyName, [SCIPropertyNameVCOActive UTF8String]) == 0)
	{
		BOOL vcoActive = [SCIVideophoneEngine sharedEngine].vcoActive;
		[[SCIVideophoneEngine sharedEngine] setVcoActivePrimitive:vcoActive];
	}
	
	[self autoEnableSpanishFeaturesIfRequired];
}

- (void)autoEnableSpanishFeaturesIfRequired {
	const NSString *language = [[NSLocale preferredLanguages] firstObject];
	NSLog(@"Language of the system is: %@", language);
	if ([language containsString:@"es"]
		&& ![[NSUserDefaults standardUserDefaults] boolForKey:kAutoEnableSpanishFeaturesTurnedOffByUserKey])
	{
		NSLog(@"Spanish features settings %@", SCIPropertyNameSpanishFeatures);
		[[SCIVideophoneEngine sharedEngine] setSpanishFeaturesPrimitive: true];
	} else if (![language containsString:@"es"]) {
		[[NSUserDefaults standardUserDefaults] setBool: NO forKey:kAutoEnableSpanishFeaturesTurnedOffByUserKey];
		[[NSUserDefaults standardUserDefaults] synchronize];
	}
}

- (void)notifyVersionCheckRequired:(NSNotification *)note // SCINotificationUpdateVersionCheckRequired
{
	SCILog(@"Start");
	[self checkUpdateUserInitiated:NO];
}

- (void)notifyFailedToEstablishTunnel:(NSNotification *)note // SCINotificationFailedToEstablishTunnel
{
	SCILog(@"Start");
	Alert(Localize(@"Network Tunneling"), Localize(@"Unable to establish network tunnel."));
}

- (void)showCallCIRNotification {
	[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"callCIR"]];
	
	UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
	notification.title = Localize(@"Call Customer Care.");
	notification.sound = UNNotificationSound.defaultSound;
	
	UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"callCIR" content:notification trigger:nil];
	[UNUserNotificationCenter.currentNotificationCenter
	 addNotificationRequest:request
	 withCompletionHandler:^(NSError * _Nullable error) {
		 if (error != nil) {
			 NSLog(@"Error posting notification: %@", error);
		 }
	 }];
}


- (void)notifyCallCIRDismissible:(NSNotification *)note // SCINotificationSuggestionCallCIR
{
	[[SCIPropertyManager sharedManager] setLastRemindLaterTimeShowCallCIRSuggestion:[NSDate date]];
	
	SCILog(@"Start");
	[self showCallCIRNotification];
	[self displayCallCIRDismissible];
}

- (void)notifyCallCIRNotDismissible:(NSNotification *)note // SCINotificationRequiredCallCIR
{
	[[SCIVideophoneEngine sharedEngine] setAllowIncomingCallsMode:NO];
	SCILog(@"Start");
	[self showCallCIRNotification];
	[self displayCallCIRNotDismissible];
}

- (void)notifyUserRegistrationDataRequired:(NSNotification *)note // SCINotificationUserRegistrationDataRequired
{
	if ([[SCIVideophoneEngine sharedEngine] interfaceMode] != SCIInterfaceModeHearing) {
		SCILog(@"Start");
		[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"requireRegistration"]];
		
		//MARK: Localize 
		UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
		notification.title = Localize(@"FCC.notification.title");
		notification.sound = UNNotificationSound.defaultSound;
		
		UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"requireRegistration" content:notification trigger:nil];
		[UNUserNotificationCenter.currentNotificationCenter
		 addNotificationRequest:request
		 withCompletionHandler:^(NSError * _Nullable error) {
			 if (error != nil) {
				 NSLog(@"Error posting notification: %@", error);
			 }
		 }];
		
		[self displayUserRegistrationDataRequired];
	}
}

- (void)notifyFavoriteListError:(NSNotification *)note //SCINotificationFavoriteListError
{
	SCILog(@"Start");
	if ([[SCIContactManager sharedManager] favoritesFull]) {
		Alert(Localize(@"Favorites Full"), Localize(@"favorites.full.error.msg"));
	}
	else {
		Alert(Localize(@"Favorites"), Localize(@"Error updating favorites"));
	}
}

- (void)notifyFavoritesChanged:(NSNotification *)note //SCINotificationFavoritesChanged
{
	[appDelegate installDynamicShortcutItems];
}

- (void)notifyCoreOfflineGenericError:(NSNotification *)note //SCINotificationCoreOfflineGenericError
{
	Alert(Localize(@"core.offline.error.title"), Localize(@"core.offline.error.msg"));
}

- (void)notifyUpdateDeviceLocationType:(NSNotification *) note //SCINotificationDeviceLocationTypeChanged
{
	if([self canHandleCoreEventNow:nil]) {
		if ([SCIVideophoneEngine sharedEngine].deviceLocationType == SCIDeviceLocationTypeUpdate) {
			UIStoryboard *endpointLocationTypeStoryboard = [UIStoryboard storyboardWithName:@"EndpointLocationTypeViewController" bundle:nil];
			UIViewController *endpointLocationTypeViewController = [endpointLocationTypeStoryboard instantiateViewControllerWithIdentifier:@"EndpointLocationTypeViewController"];
			endpointLocationTypeViewController.modalPresentationStyle = UIModalPresentationFormSheet;
			if (@available(iOS 13, *)) {
				[endpointLocationTypeViewController setModalInPresentation: YES];
			}
			[self.topViewController presentViewController:endpointLocationTypeViewController animated:YES completion:nil];
		}
	} else {
		[self performSelector:@selector(notifyUpdateDeviceLocationType:) withObject:nil afterDelay:kRetryModalViewSeconds];
		return;
	}
}

- (void)notifyRingsBeforeGreetingChanged:(NSNotification *)note //SCINotificationRingsBeforeGreetingChanged
{
	if (SCIVideophoneEngine.sharedEngine.ringsBeforeGreeting < 5) {
			[SCIVideophoneEngine.sharedEngine setRingsBeforeGreetingPrimitive:5];
	}
}

- (void)checkUpdateUserInitiated:(BOOL)userInitiated
{
	NSString *marketingVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
	
	if ([marketingVersion containsString:@"experimental"]) {
		// This is an experimental build, it doesn't have a release train thus it has no updates.
		return;
	}
	
	NSString *version = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
	[[SCIVideophoneEngine sharedEngine] checkForUpdateFromVersion:version block:^(NSString *versionAvailable, NSString *updateURL, NSError *err) {
		if (versionAvailable && !err)
		{
			// If we're here, this means that the current version doesn't match what Core has. We could be ahead of or behind Core.
			// So we'll check to make sure that the version Core is reporting is > our version.
			if ([BuildVersion isVersionNewerCurrentVersion:version updateVersion:versionAvailable] == NO)
				versionAvailable = nil;
		}
		
		if (err)
		{
			if (userInitiated)
				Alert(@"Error Checking for Update", [NSString stringWithFormat:@"%@", [err localizedDescription]]);
		}
		else if (versionAvailable)
		{
			self.updateVersion = versionAvailable;
			self.updateVersionUrl = updateURL;
			[self updateNeeded];
		}
		else
		{
			[self updateUpToDate];
			if (userInitiated) {
				Alert(@"You are running the most recent version of ntouch.", @"No update is presently available.");
			}
		}
	}];
}

- (int)getBuildNumberFromString:(NSString*)versionString {
	int buildNumber = -1;
	NSArray *versionNumbers = [versionString componentsSeparatedByString:@"."];
	if (0 < [versionNumbers count]) {
		NSString *buildNumberString = [versionNumbers lastObject];
		buildNumber = [buildNumberString intValue];
	}
	return buildNumber;
}

- (void)updateAvailableWithVersion:(NSString *)version updateUrl:(NSString *)updateUrl {
	SCILog(@"Start");
	
	// update should be required (move to unit tests)
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"600"], (@"Version Test - 1.0.529 vs. 600"));
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"1.3.160"], (@"Version Test - 1.0.529 vs. 1.3.160"));
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"1.2"], (@"Version Test - 1.0.529 vs. 1.2"));
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"1.0.600"], (@"Version Test - 1.0.529 vs. 1.0.600"));
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"2.0"], (@"Version Test - 1.0.529 vs. 2.0"));
	NSAssert([BuildVersion isVersionNewerCurrentVersion:@"1.0.600" updateVersion:@"1.1.529"], (@"Version Test - 1.0.600 vs. 1.1.529"));
	// update should not be required (move to unit tests)
	NSAssert(![BuildVersion isVersionNewerCurrentVersion:@"1.0.529" updateVersion:@"1.0"], (@"Version Test - 1.0.529 vs. 1.0"));
	NSAssert(![BuildVersion isVersionNewerCurrentVersion:@"1.1.529" updateVersion:@"1.0.600"], (@"Version Test - 1.1.529 vs. 1.0.600"));
	
	// Assuming current version is 1.0.529
	// 530	= 1.0.530
	// 1.2	= 1.2.0
	// 1	= 1.0.1
	NSString *currentVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
	if ([BuildVersion isVersionNewerCurrentVersion:currentVersion updateVersion:version]) {
		self.updateVersion = version;
		self.updateVersionUrl = updateUrl;
		[self updateNeeded];
	}
	else {
		[self updateUpToDate];
	}
}

- (void)updateVersionCheckForce {
	SCILog(@"Start");
	[self checkUpdateUserInitiated:NO];
}

- (void)updateNetworkError {
	SCILog(@"Start");
	//AlertWithTitleAndMessage(Localize(@"Can’t Check Version"), Localize(@"A network error occurred."));
}

- (void)updateNeeded {
	SCILog(@"Start");
	if (SCICallController.shared.isCallInProgress || [self playingAVideo]) {
		[self performSelector:@selector(updateNeeded) withObject:nil afterDelay:kRetryModalViewSeconds];
	}
	else {
		[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"updateRequired"]];
		
		UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
		notification.title = Localize(@"update.ntouch.title");
		notification.sound = UNNotificationSound.defaultSound;
		notification.userInfo = @{
			@"CoreEvent": @(coreEventUpdateRequired)
		};
		
		UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"updateRequired" content:notification trigger:nil];
		[UNUserNotificationCenter.currentNotificationCenter
		 addNotificationRequest:request
		 withCompletionHandler:^(NSError * _Nullable error) {
			 if (error != nil) {
				 NSLog(@"Error posting notification: %@", error);
			 }
		 }];
		
		if (UIApplication.sharedApplication.applicationState == UIApplicationStateActive) {
			[appDelegate presentUpdateRequired];
		}
		else {
			[self addCoreEventWithType:coreEventUpdateRequired];
		}
	}
}

- (void)updateUpToDate {
	SCILog(@"[SAccount updateUpToDate]");
}

- (void)postOnMainThreadNotificationName:(NSString *)name userInfo:(NSDictionary *)info
{
	void (^block)() = ^{
		[[NSNotificationCenter defaultCenter] postNotificationName:name object:self userInfo:info];
	};
	if ([NSThread isMainThread])
		block();
	else
		dispatch_async(dispatch_get_main_queue(), block);
}

- (void)checkForUpdate {
	if (!self.versionCheckComplete) {
		self.versionCheckComplete = YES;
		[self checkUpdateUserInitiated:NO];
	}
}

- (void)coreServiceConnected {
	SCILog(@"Start");
	self.coreIsConnected = YES;
	[self performSelector:@selector(checkForUpdate) withObject:nil afterDelay:30.0];
}

- (void)coreServiceDisconnected {
	SCILog(@"Start");
	self.coreIsConnected = NO;
}

- (void)userLoggedIntoOtherDevice:(NSNotification *)note { //SCINotificationUserLoggedIntoAnotherDevice
	SCILog(@"Start");
	
	if (SCICallController.shared.isEmergencyCallInProgress) {
		[self performSelector:@selector(userLoggedIntoOtherDevice:) withObject:note afterDelay:kRetryModalViewSeconds];
	}
	else if (SCICallController.shared.isCallInProgress) {
		[SCICallController.shared endAllCalls];
		[self performSelector:@selector(userLoggedIntoOtherDevice:) withObject:note afterDelay:kRetryModalViewSeconds];
	}
	else {
		[self signOut];
		__weak SCIAccountManager *weak_self = self;
		[self presentAccountViewWithCompletion:^{
			
			__strong SCIAccountManager *strong_self = weak_self;
			if (strong_self == nil) {
				return;
			}
			
			[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"loggedOut", @"passwordChanged"]];
			
			UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
			notification.title = Localize(@"account.loggedin.other.device.title");
			notification.sound = UNNotificationSound.defaultSound;
			
			UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"loggedOut" content:notification trigger:nil];
			[UNUserNotificationCenter.currentNotificationCenter
			 addNotificationRequest:request
			 withCompletionHandler:^(NSError * _Nullable error) {
				 if (error != nil) {
					 NSLog(@"Error posting notification: %@", error);
				 }
			 }];
			
		 	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
				UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Logged Out")
																			   message:Localize(@"account.loggedin.other.device.title")
																		preferredStyle:UIAlertControllerStyleAlert];
				
				[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")  style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
				
				}]];
				
				[self.topViewController presentViewController:alert animated:YES completion:nil];
			}
		}];
	}
}

- (void)clientReauthenticate {
	SCILog(@"Start");
	[self signOut];
	[self presentAccountViewWithCompletion:nil];
}

- (void)clientAuthenticated {
	SCILog(@"Start");
}

- (void)displayProviderAgreement {
	SCILog(@"[SAccount displayProviderAgreement]");
	//
	// BUG 19487: When handling the DynamicProviderAgreements, dont attempt to display until isAuthorized=true.
	//
	if (SCICallController.shared.isCallInProgress || [self playingAVideo] || !self.engine.isAuthorized) {
		[self performSelector:@selector(displayProviderAgreement) withObject:nil afterDelay:kRetryModalViewSeconds];
	}
	else {
		[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"displayAgreement"]];
		
		UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
		notification.title = Localize(@"Please review the provider agreement.");
		notification.sound = UNNotificationSound.defaultSound;
		notification.userInfo = @{
			@"CoreEvent": @(coreEventShowProviderAgreement)
		};
		
		UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"displayAgreement" content:notification trigger:nil];
		[UNUserNotificationCenter.currentNotificationCenter
		 addNotificationRequest:request
		 withCompletionHandler:^(NSError * _Nullable error) {
			 if (error != nil) {
				 NSLog(@"Error posting notification: %@", error);
			 }
		 }];
		
		if (self.currentCoreEvent &&
			(self.currentCoreEvent.type.intValue == coreEventShowProviderAgreement)) {
			//Do nothing.  The SCIProviderAgreementManager will handle this.
		} else {
			[self addCoreEventWithType:coreEventShowProviderAgreement];
		}
	}
}

- (void)clientPasswordChanged {
	SCILog(@"[SAccount clientPasswordChanged]");
	[UNUserNotificationCenter.currentNotificationCenter removeDeliveredNotificationsWithIdentifiers:@[@"passwordChanged"]];
	
	UNMutableNotificationContent *notification = [UNMutableNotificationContent new];
	notification.title = Localize(@"account.password.changed.title");
	notification.sound = UNNotificationSound.defaultSound;
	notification.userInfo = @{
		@"CoreEvent": @(coreEventPasswordChanged)
	};
	
	UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"passwordChanged" content:notification trigger:nil];
	[UNUserNotificationCenter.currentNotificationCenter
	 addNotificationRequest:request
	 withCompletionHandler:^(NSError * _Nullable error) {
		 if (error != nil) {
			 NSLog(@"Error posting notification: %@", error);
		 }
	 }];

	[self addCoreEventWithType:coreEventPasswordChanged];
}

- (void)displayPhoneNumberChangedAlert {
	if (SCICallController.shared.isCallInProgress || [self playingAVideo]) {
		[self performSelector:@selector(displayPhoneNumberChangedAlert) withObject:nil afterDelay:kRetryModalViewSeconds];
	}
	else {
		NSString *message = Localize(@"account.phone.number.changed.msg");
		message = [message stringByAppendingFormat:@"%@", FormatAsPhoneNumber(self.defaults.phoneNumber)];
		self.phoneNumberChangedAlert = [UIAlertController alertControllerWithTitle:@"ntouch® Mobile" message:message preferredStyle:UIAlertControllerStyleAlert];
		
		[self.phoneNumberChangedAlert addAction:[UIAlertAction actionWithTitle:Localize(@"OK") style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
			self.phoneNumberChangedAlert = nil;
		}]];
		
		[appDelegate.topViewController presentViewController:self.phoneNumberChangedAlert animated:YES completion:nil];
	}
}

- (void)displayDebugLogging {
	[[SCIVideophoneEngine sharedEngine] saveFile];
	g_stiTraceDebug = 1;
	DebugViewController *debugViewController = [[DebugViewController alloc] initWithStyle:UITableViewStyleGrouped];
	UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:debugViewController];
	navController.modalPresentationStyle = UIModalPresentationFormSheet;
	[appDelegate.topViewController presentViewController:navController animated:YES completion:nil];
}

#define VOIP_PUSH_ENABLED @"VoipPushEnabled"
#define VOIP_PUSH_ENABLED_DEFAULT 1

- (void)setVoipPushEnabled:(BOOL)voipPushEnabled
{
	int voipPushEnabledInt = voipPushEnabled ? 1 : 0;
	WillowPM::PropertyManager::EStorageLocation eLocation = WillowPM::PropertyManager::Persistent;
	WillowPM::PropertyManager *propertyManager = WillowPM::PropertyManager::getInstance();
	NSString *propertyName = VOIP_PUSH_ENABLED;
	propertyManager->setPropertyInt([propertyName UTF8String], voipPushEnabledInt, eLocation);
	propertyManager->PropertySend([propertyName UTF8String], estiPTypeUser);
}

- (BOOL)audioAndVCOEnabled {
	SCIVoiceCarryOverType type = [SCIVideophoneEngine sharedEngine].voiceCarryOverType;
	BOOL vcoEnabled = (type == SCIVoiceCarryOverTypeOneLine);
	BOOL audioEnabled = [SCIVideophoneEngine sharedEngine].audioEnabled;
	return (vcoEnabled || audioEnabled);
}

- (void)setAudioAndVCOEnabled:(BOOL)audioAndVCOEnabled {
	if (audioAndVCOEnabled) {
		[[SCIVideophoneEngine sharedEngine] setVoiceCarryOverType:SCIVoiceCarryOverTypeOneLine];
		[SCIVideophoneEngine sharedEngine].audioEnabled = YES;
	}
	else {
		[[SCIVideophoneEngine sharedEngine] setVoiceCarryOverType:SCIVoiceCarryOverTypeNone];
		[SCIVideophoneEngine sharedEngine].audioEnabled = NO;
	}
}

- (void)heartBeatReceived {
	SCILog(@"Start");
	[self coreServiceConnected];
}

@end
