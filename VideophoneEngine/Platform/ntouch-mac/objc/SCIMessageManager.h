//
//  SCIMessageManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/4/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIItemId;
@class SCIMessageCategory;
@class SCIMessageInfo;

@interface SCIMessageManager : NSObject

@property (class, readonly) SCIMessageManager *sharedManager;

@property (nonatomic, assign, readonly) BOOL isBusy;

//General
- (void)markMessage:(SCIMessageInfo *)message viewed:(BOOL)viewed;
- (void)deleteMessage:(SCIMessageInfo *)message;

//Video Center
- (NSArray *)channels;

//Sign Mail
- (NSArray *)signMail;
- (NSUInteger)signMailCapacity;
- (void)deleteAllSignMail;

@end

extern NSString * const SCIMessageManagerKeyBusy;

extern NSNotificationName const SCIMessageManagerNotificationSignMailChanged;
extern NSNotificationName const SCIMessageManagerNotificationChannelChanged;
extern NSString * const SCIMessageManagerNotificationKeyMessageCategory;
extern NSString * const SCIMessageManagerNotificationKeyMessageInfo;
