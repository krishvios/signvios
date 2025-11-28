//
//  SCIItemIdCpp.h
//  ntouchMac
//
//  Created by Adam Preble on 5/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//
#import "SCIItemId.h"

class CstiItemId;

@interface SCIItemId (Cpp)

+ (SCIItemId *)itemIdWithCstiItemId:(CstiItemId)cstiItemId;
+ (SCIItemId *)itemIdWithItemIdString:(const char *)itemIdString;

@property (readonly) CstiItemId cstiItemId;

@end
