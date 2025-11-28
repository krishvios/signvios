//
//  SCICallListItem.h
//  ntouchMac
//
//  Created by Adam Preble on 2/15/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCICall.h"

typedef enum {
	SCICallTypeUnknown,
	SCICallTypeMissed,
	SCICallTypeAnswered,
	SCICallTypeDialed,
	SCICallTypeBlocked,
	SCICallTypeRecent
} SCICallType;

@interface SCICallListItem : NSObject <NSCopying>

@property (nonatomic) NSString *name;
@property (nonatomic) NSString *phone;
@property (nonatomic) NSString *callId;
@property (nonatomic) NSString *intendedDialString;
@property (nonatomic) SCICallType type;
@property (nonatomic) NSDate *date;
@property (nonatomic) NSTimeInterval duration;
@property (nonatomic) SCIDialMethod dialMethod;
@property (nonatomic, assign) NSUInteger groupedCount;
@property (nonatomic, strong) NSArray<SCICallListItem *> *groupedItems;

@property (nonatomic, readonly) NSString *typeAsString;
@property (nonatomic, readonly) BOOL isVRSCall;
@property (nonatomic, readonly) NSString *agentId;

+ (SCICallListItem *)itemWithName:(NSString *)name phone:(NSString *)phone type:(SCICallType)type date:(NSDate *)date duration:(NSTimeInterval)duration dialMethod:(SCIDialMethod)dialMethod callId:(NSString*)callId intendedDialString:(NSString*)intendedDialString;
+ (SCICallListItem *)itemWithCall:(SCICall *)call;
- (void)resetName;

@end
