//
//  SCIBlocked.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIItemId;

@interface SCIBlocked : NSObject <NSCopying>

+ (SCIBlocked *)blockedWithItemId:(SCIItemId *)itemId number:(NSString *)number title:(NSString *)title;

@property (nonatomic, strong, readwrite) SCIItemId *blockedId;
@property (nonatomic, strong, readwrite) NSString *title;
@property (nonatomic, strong, readwrite) NSString *number;
- (id)initWithItemId:(SCIItemId *)blockedId number:(NSString *)number title:(NSString *)title;

- (NSComparisonResult)compare:(SCIBlocked *)comparand;
- (BOOL)isEqualToBlocked:(SCIBlocked *)blocked;

@end
