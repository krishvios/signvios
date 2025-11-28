//
//  SCICall+Display.h
//  ntouchMac
//
//  Created by Adam Preble on 5/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICall.h"

@interface SCICall (Display)

@property (nonatomic, readonly) NSString *displayName;
@property (nonatomic, readonly) NSString *displayNumber;
@property (nonatomic, readonly) NSString *displayID;
@property (nonatomic, readonly) BOOL displayIsHoldable;

- (void)displayNameDidChange;
- (void)displayNumberDidChange;
- (void)displayIDDidChange;
- (void)displayIsHoldableDidChange;

@end
