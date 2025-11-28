//
//  SCIAccountManager.h
//  ntouch
//
//  Created by Kevin Selman on 5/8/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIProviderAgreementState.h"
#import "SCICoreEvent.h"

@class SCall;
@class SMessage;
@class SCoreEvent;
@class SService;
@class SSettings;
@class SCIStaticProviderAgreementStatus;
@class EndpointSelectionController;

typedef enum {
	CSSignedOut,
	CSSigningIn,
	CSSignedIn,
	CSSigningOut
} CSState;


@interface SCIAccountManager : NSObject

@property (nonatomic, strong) NSNumber *state;
@property (nonatomic, assign) BOOL versionCheckComplete;
@property (nonatomic, strong) NSMutableArray *viewStates;
@property (nonatomic, strong) NSDictionary *coreOptions;
@property (nonatomic, assign) BOOL coreIsConnected;
@property (nonatomic, strong) NSArray *agreementsAwaitingAcceptance;
@property (nonatomic, strong) SCIStaticProviderAgreementStatus *providerAgreementStatusAwaitingUpdate;
@property (nonatomic, assign) SCIProviderAgreementState eulaAgreementState;
@property (nonatomic, strong) NSNumber *providerAgreementTypeNumber;
@property (nonatomic, assign, readonly) BOOL eulaWasRejected;
@property (nonatomic, strong) UIAlertController *redirectedCallAlert;
@property (nonatomic, strong) UIAlertController *askMeVcoAlert;
@property (nonatomic, strong) NSString *messageToLog;

@property (strong) NSString * updateVersion;
@property (strong) NSString * updateVersionUrl;
@property (nonatomic, assign) BOOL isCreatingMyPhoneGroup;
@property (nonatomic, strong) EndpointSelectionController *endpointSelectionController;
@property (nonatomic, assign) NSTimeInterval timeInterval;
@property (nonatomic, strong) NSDate *checkTime;
@property (nonatomic, strong) NSTimer *checkForUpdateTimer;
@property (nonatomic, strong, readwrite) NSDate *latestMissedCallNotification;
@property (nonatomic, assign) BOOL audioAndVCOEnabled;

- (void)remindMeLaterCheck;

- (void)open;
- (void)close;
- (void)signIn:(NSString *)userName password:(NSString *)password usingCache:(BOOL)usingCache;
- (void)autoSignIn:(BOOL)useCache;
- (void)presentAccountViewWithCompletion:(void (^)())block;
- (void)cancelLogin;
- (void)cancelLoginTimeout;
- (void)startLoginTimeout;
- (void)signOut;
- (BOOL)playingAVideo;
- (void)addCoreEventWithType:(CoreEventType)coreEventType;
- (void)didFinishCoreEventWithType:(CoreEventType)coreEventType;

- (void)updateAvailableWithVersion:(NSString *)version updateUrl:(NSString *)updateUrl;
- (void)coreServiceConnected;
- (void)coreServiceDisconnected;
- (void)clientReauthenticate;
- (void)clientPasswordChanged;
- (void)displayProviderAgreement;
- (void)updateVersionCheckForce;
- (void)updateNetworkError;
- (void)updateNeeded;
- (void)updateUpToDate;
- (void)displayDebugLogging;
- (void)displaySignVideoTeaser:(NSDictionary *)details;


@property (class, readonly) SCIAccountManager *sharedManager;

- (void)indexContacts;

@end
