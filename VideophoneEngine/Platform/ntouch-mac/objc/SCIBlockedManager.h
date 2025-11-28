//
//  SCIBlockedManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIBlocked;

extern NSNotificationName const _Nonnull SCINotificationBlockedsChanged;
extern NSString * const _Nonnull SCINotificationKeyAddedBlocked;

typedef NSUInteger SCIBlockedManagerResult;

@interface SCIBlockedManager : NSObject {
	BOOL mWasAuthenticated;
}

@property (class, readonly, nonnull) SCIBlockedManager *sharedManager;

@property (nonatomic, strong, readwrite, null_unspecified) NSArray *blockeds;

- (SCIBlockedManagerResult)addBlocked:(null_unspecified SCIBlocked *)blocked;
- (SCIBlockedManagerResult)updateBlocked:(null_unspecified SCIBlocked *)blocked;
- (SCIBlockedManagerResult)removeBlocked:(null_unspecified SCIBlocked *)blocked;

- (void)refresh;

- (SCIBlocked * _Null_unspecified )blockedForNumber:(NSString * _Null_unspecified )number;

@end

extern SCIBlockedManagerResult SCIBlockedManagerResultUnknown;
extern SCIBlockedManagerResult SCIBlockedManagerResultError;
extern SCIBlockedManagerResult SCIBlockedManagerResultSuccess;
extern SCIBlockedManagerResult SCIBlockedManagerResultBlockListAlreadyContainsNumber;
extern SCIBlockedManagerResult SCIBlockedManagerResultBlockListFull;
extern SCIBlockedManagerResult SCIBlockedManagerResultCoreOffline;
