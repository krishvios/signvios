//
//  SCICallListManager.h
//  ntouchMac
//
//  Created by Adam Preble on 2/16/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCICallListItem.h"

extern NSNotificationName const SCINotificationCallListItemsChanged;
extern NSNotificationName const SCINotificationAggregateCallListItemsChanged;

@interface SCICallListManager : NSObject

@property (class, readonly) SCICallListManager *sharedManager;

@property (nonatomic, strong) NSArray *items;
@property (nonatomic, strong) NSArray *aggregatedItems;

- (void)addItem:(SCICallListItem *)item;
- (BOOL)removeItem:(SCICallListItem *)item;
- (BOOL)removeItemWithoutNotification:(SCICallListItem *)item;
- (BOOL)removeItemsInArray:(NSArray *)items;
- (BOOL)clearCallsOfType:(SCICallType)callType;
- (BOOL)clearAllCalls;

- (NSArray *)itemsForNumber:(NSString *)number;

- (void)refresh;

- (void)updateNamesAndNotify:(BOOL)notify;
- (SCICallListItem *)mostRecentDialedCallListItem;
- (NSUInteger)missedCallsAfter:(NSDate *)date;
- (NSArray *)missedCallListItemsAfter:(NSDate *)date;

- (NSString *)nameForNumber:(NSString *)number;

- (NSDate *)latestMissedCallTime;

@end
