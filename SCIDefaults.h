//
//  SCIDefaults.h
//  ntouch
//
//  Created by Kevin Selman on 5/29/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIAccountObject;

@interface SCIDefaults : NSObject

@property (nonatomic, strong) NSString *phoneNumber;
@property (nonatomic, strong) NSString *PIN;
@property (nonatomic, assign) BOOL isAutoSignIn;
@property (nonatomic, assign) BOOL savePIN;
@property (nonatomic, assign) NSNumber *myRumblePattern;
@property (nonatomic, assign) NSNumber *myRumbleFlasher;
@property (nonatomic, assign) BOOL pulseEnable;
@property (nonatomic, assign) NSArray<NSUUID *> *connectedPulseDevices;
@property (nonatomic, assign) BOOL vibrateOnRing;
@property (nonatomic, assign) BOOL audioPrivacy;
@property (nonatomic, assign) BOOL videoPrivacy;
@property (nonatomic, assign) BOOL wizardCompleted;
@property (nonatomic, assign) NSNumber *signMailScope;
@property (nonatomic, assign) NSNumber *signMailSort;
@property (nonatomic, assign) NSNumber *recentsScope;
@property (nonatomic, strong) NSArray* coreEvents;
@property (nonatomic, assign) NSInteger GVCRotateHelpCount;

@property (class, readonly) SCIDefaults *sharedDefaults;

- (void)setLDAPUsername:(NSString *)LDAPUsername andLDAPPassword:(NSString *)LDAPPassword forUsername:(NSString *)username;
- (NSDictionary *)LDAPUsernameAndPasswordForUsername:(NSString *)username;

- (void)addAccountHistoryItem:(SCIAccountObject *)account;
- (void)removeAccountHistoryItem:(SCIAccountObject *)account;
- (void)removeAllAccountHistory;
@property (nonatomic, readonly) NSArray *accountHistoryList;

@property (nonatomic, assign) NSUInteger autoAnswerRings; // answer after N rings; 0 for disabled

@end
