//
//  SCIMessageManagerInternal.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageManager.h"

@class SCIVideophoneEngine;
@class SCIMessageCategory;

@interface SCIMessageManager (Internal)
- (SCIVideophoneEngine *)videophoneEngine;
- (SCIMessageCategory *)signMailCategory;
@end
