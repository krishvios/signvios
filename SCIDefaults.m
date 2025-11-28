//
//  SCIDefaults.m
//  ntouch
//
//  Created by Kevin Selman on 5/29/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCIDefaults.h"
#import "AppDelegate.h"
#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "UICKeyChainStore.h"
#import "ntouch-Swift.h"

@implementation SCIDefaults

@synthesize phoneNumber = _phoneNumber;
@synthesize PIN = _PIN;

NSString * const SCIDefaultPhoneNumber = @"SCIDefaultPhoneNumber";
NSString * const SCIDefaultPIN = @"SCIDefaultPIN";
NSString * const SCIDefaultAutoSignIn = @"SCIDefaultAutoSignIn";
NSString * const SCIDefaultSavePIN = @"SCIDefaultSavePIN";
NSString * const SCIDefaultMyRumblePattern = @"SCIDefaultMyRumblePattern";
NSString * const SCIDefaultMyRumbleFlasher = @"SCIDefaultMyRumbleFlasher";
NSString * const SCIDefaultPulseEnable = @"SCIDefaultPulseEnable";
NSString * const SCIDefaultPulseDeviceArray = @"SCIDefaultPulseDeviceArray";
NSString * const SCIDefaultAudioPrivacy = @"SCIDefaultAudioPrivacy";
NSString * const SCIDefaultVideoPrivacy = @"SCIDefaultVideoPrivacy";
NSString * const SCIDefaultSignMailScope = @"SCIDefaultSignMailScope";
NSString * const SCIDefaultSignMailSort = @"SCIDefaultSignMailSort";
NSString * const SCIDefaultRecentsScope = @"SCIDefaultRecentsScope";
NSString * const SCIDefaultVibrateOnRing = @"SCIDefaultVibrateOnRing";
NSString * const SCIDefaultCoreEventsArray = @"SCIDefaultCoreEventsArray";
NSString * const SCIDefaultLDAPUsername = @"SCIDefaultLDAPUsername";
NSString * const SCIDefaultLDAPPassword = @"SCIDefaultLDAPPassword";
NSString * const SCIDefaultGVCRotateHelpCount = @"SCIDefaultGVCRotateHelpCount";
NSString * const SCIDefaultAutoAnswerRings = @"SCIAutoAnswerRings";
NSString * const SCIDefaultWizardCompleted = @"wizardCompleted";

NSString * const SCIKeyChainServiceLDAP = @"SCIKeyChainServiceLDAP";
NSString * const SCIKeyChainServiceAccountHistory = @"SCIKeyChainServiceAccountHistory";


static SCIDefaults *sharedDefaults = nil;

+ (SCIDefaults *)sharedDefaults
{
	if (!sharedDefaults)
	{
		sharedDefaults = [[SCIDefaults alloc] init];
		NSURL *defaultPrefsFile = [[NSBundle mainBundle] URLForResource:@"DefaultPreferences" withExtension:@"plist"];
		NSDictionary *defaultPrefs = [NSDictionary dictionaryWithContentsOfURL:defaultPrefsFile];
		[[NSUserDefaults standardUserDefaults] registerDefaults:defaultPrefs];
	}
	return sharedDefaults;
}

- (NSString *)phoneNumber {
	NSString *phone = [UICKeyChainStore stringForKey:SCIDefaultPhoneNumber];

	if (phone)
		_phoneNumber = phone;
	
	return _phoneNumber;
}

- (void)setPhoneNumber:(NSString *)phone {
	[UICKeyChainStore setString:phone forKey:SCIDefaultPhoneNumber];
	_phoneNumber = phone;
}

- (NSString *)PIN {
	NSString *password = [UICKeyChainStore stringForKey:SCIDefaultPIN];
	
	if (password)
		_PIN = password;
	
	return _PIN;
}

- (void)setPIN:(NSString *)password {
	[UICKeyChainStore setString:password forKey:SCIDefaultPIN];
	_PIN = password;
}

- (BOOL)isAutoSignIn {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultAutoSignIn];
}

- (void)setIsAutoSignIn:(BOOL)isAutoSignIn {
	[[NSUserDefaults standardUserDefaults] setBool:isAutoSignIn forKey:SCIDefaultAutoSignIn];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (BOOL)savePIN {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultSavePIN];
}

- (void)setSavePIN:(BOOL)savePIN {
	[[NSUserDefaults standardUserDefaults] setBool:savePIN forKey:SCIDefaultSavePIN];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSNumber *)myRumblePattern {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultMyRumblePattern];
	return [NSNumber numberWithInteger:temp];
}

- (void)setMyRumblePattern:(NSNumber *)myRumblePattern {
	[[NSUserDefaults standardUserDefaults] setInteger:[myRumblePattern integerValue] forKey:SCIDefaultMyRumblePattern];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSNumber *)myRumbleFlasher {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultMyRumbleFlasher];
	return [NSNumber numberWithInteger:temp];
}

- (void)setMyRumbleFlasher:(NSNumber *)myRumbleFlasher {
	[[NSUserDefaults standardUserDefaults] setInteger:[myRumbleFlasher integerValue] forKey:SCIDefaultMyRumbleFlasher];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL)pulseEnable {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultPulseEnable];
}

-(void)setPulseEnable:(BOOL)pulseEnable {
	[[NSUserDefaults standardUserDefaults] setBool:pulseEnable forKey:SCIDefaultPulseEnable];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(NSArray<NSUUID*> *)connectedPulseDevices {
	NSData *deviceData = [[NSUserDefaults standardUserDefaults] objectForKey:SCIDefaultPulseDeviceArray];
	NSArray<NSUUID*> *devices = [NSKeyedUnarchiver unarchiveObjectWithData:deviceData];
	return devices;
}

-(void)setConnectedPulseDevices:(NSArray<NSUUID*> *)devices {
	NSData *deviceData = [NSKeyedArchiver archivedDataWithRootObject:devices];
	[[NSUserDefaults standardUserDefaults] setObject:deviceData forKey:SCIDefaultPulseDeviceArray];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL)vibrateOnRing {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultVibrateOnRing];
}

-(void)setVibrateOnRing:(BOOL)vibrateOnRing {
	[[NSUserDefaults standardUserDefaults] setBool:vibrateOnRing forKey:SCIDefaultVibrateOnRing];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL)audioPrivacy {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultAudioPrivacy];
}

-(void)setAudioPrivacy:(BOOL)privacy {
	[[NSUserDefaults standardUserDefaults] setBool:privacy forKey:SCIDefaultAudioPrivacy];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL)videoPrivacy {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultVideoPrivacy];
}

-(void)setVideoPrivacy:(BOOL)videoPrivacy {
	[[NSUserDefaults standardUserDefaults] setBool:videoPrivacy forKey:SCIDefaultVideoPrivacy];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL)wizardCompleted {
	return [[NSUserDefaults standardUserDefaults] boolForKey:SCIDefaultWizardCompleted];
}

-(void)setWizardCompleted:(BOOL)wizardCompleted {
	[[NSUserDefaults standardUserDefaults] setBool:wizardCompleted forKey:SCIDefaultWizardCompleted];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(NSNumber *)signMailScope {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultSignMailScope];
	return [NSNumber numberWithInteger:temp];
}

-(void)setSignMailScope:(NSNumber *)signMailScope {
	[[NSUserDefaults standardUserDefaults] setInteger:[signMailScope integerValue] forKey:SCIDefaultSignMailScope];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(NSNumber *)signMailSort {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultSignMailSort];
	return [NSNumber numberWithInteger:temp];
}

-(void)setSignMailSort:(NSNumber *)signMailsort {
	[[NSUserDefaults standardUserDefaults] setInteger:[signMailsort integerValue] forKey:SCIDefaultSignMailSort];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(NSNumber *)recentsScope {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultRecentsScope];
	return [NSNumber numberWithInteger:temp];
}

-(void)setRecentsScope:(NSNumber *)recentsScope {
	[[NSUserDefaults standardUserDefaults] setInteger:[recentsScope integerValue] forKey:SCIDefaultRecentsScope];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

-(NSArray *)coreEvents {
	
	NSData *eventData = [[NSUserDefaults standardUserDefaults] objectForKey:SCIDefaultCoreEventsArray];
	NSArray *events = [NSKeyedUnarchiver unarchiveObjectWithData:eventData];
	return events;
}

- (void)setCoreEvents:(NSArray *)coreEvents {
	
	NSData *data = [NSKeyedArchiver archivedDataWithRootObject:coreEvents];
	[[NSUserDefaults standardUserDefaults] setObject:data forKey:SCIDefaultCoreEventsArray];
}

- (void)setLDAPUsername:(NSString *)LDAPUsername andLDAPPassword:(NSString *)LDAPPassword forUsername:(NSString *)username
{
	NSDictionary *LDAPUsernameAndPasswordDictionary = @{@"LDAPUsername":LDAPUsername, @"LDAPPassword":LDAPPassword};
	
	NSError *error;
	NSData *jsonData = [NSJSONSerialization dataWithJSONObject:LDAPUsernameAndPasswordDictionary
													   options:0 // Pass 0 if you don't care about the readability of the generated string
														 error:&error];
	NSString *jsonString;
	if (! jsonData) {
		NSLog(@"Got an error: %@", error);
	} else {
		jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
	}
	
	[UICKeyChainStore setString:jsonString forKey:username service:SCIKeyChainServiceLDAP];
}

- (NSDictionary *)LDAPUsernameAndPasswordForUsername:(NSString *)username
{
	NSString *res = nil;
	if (!username) {
		res = nil;
	} else if ([username length] == 0) {
		res = @"";
	} else {
		res = [UICKeyChainStore stringForKey:username service:SCIKeyChainServiceLDAP];
	}
	NSDictionary *json = nil;
	if (res) {
		NSError *jsonError;
		NSData *objectData = [res dataUsingEncoding:NSUTF8StringEncoding];
		json = [NSJSONSerialization JSONObjectWithData:objectData
											   options:NSJSONReadingMutableContainers
												 error:&jsonError];
	}
	return json;
}

- (void)addAccountHistoryItem:(SCIAccountObject *)account {
	[UICKeyChainStore setString:account.PIN forKey:account.phoneNumber service:SCIKeyChainServiceAccountHistory];
}

- (void)removeAccountHistoryItem:(SCIAccountObject *)account {
	[UICKeyChainStore removeItemForKey:account.phoneNumber service:SCIKeyChainServiceAccountHistory];
}

- (void)removeAllAccountHistory {
	[UICKeyChainStore removeAllItemsForService:SCIKeyChainServiceAccountHistory];
}

- (NSArray *)accountHistoryList {
	NSArray *allKeyChainItems = [UICKeyChainStore itemsForService:SCIKeyChainServiceAccountHistory accessGroup:nil];
	NSMutableArray *returnArray = [[NSMutableArray alloc] init];

	for (NSDictionary *item in allKeyChainItems) {
		if (item) {
			SCIAccountObject *account = [[SCIAccountObject alloc] init];
			account.phoneNumber = [item objectForKey:(__bridge id)kSecAttrAccount];
			NSData *data = [item objectForKey:(__bridge id)kSecValueData];
			account.PIN = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
			[returnArray addObject:account];
		}
	}
	return returnArray;
}

- (NSInteger)GVCRotateHelpCount {
	NSInteger temp = [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultGVCRotateHelpCount];
	return temp;
}

- (void)setGVCRotateHelpCount:(NSInteger)GVCRotateHelpCount {
	[[NSUserDefaults standardUserDefaults] setInteger:GVCRotateHelpCount forKey:SCIDefaultGVCRotateHelpCount];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSUInteger)autoAnswerRings
{
	return [[NSUserDefaults standardUserDefaults] integerForKey:SCIDefaultAutoAnswerRings];
}
- (void)setAutoAnswerRings:(NSUInteger)autoAnswerRings
{
	[[NSUserDefaults standardUserDefaults] setInteger:autoAnswerRings forKey:SCIDefaultAutoAnswerRings];
	[[NSUserDefaults standardUserDefaults] synchronize];
}


@end
