//
//  SCICallListManager+UserInterfaceProperties.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/8/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallListManager.h"

@interface SCICallListManager (UserInterfaceProperties)

//Observing
- (void)startObservingUserInterfaceProperties;
- (void)stopObservingUserInterfaceProperties;

//User Interface Properties
@property (nonatomic, assign, readonly) BOOL hasUnviewedMissedCalls;
- (void)setHasUnviewedMissedCallsPrimitive:(BOOL)hasUnviewedMissedCalls;
@property (nonatomic, readonly) NSUInteger unviewedMissedCallCount;

@end

extern NSString * const SCICallListManagerKeyHasUnviewedMissedCalls;
extern NSString * const SCICallListManagerKeyUnviewedMissedCallCount;
