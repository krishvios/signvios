//
//  YelpRegion_Internal.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/2/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "YelpRegion.h"

@interface YelpRegion ()

+ (instancetype)regionWithJSONDictionary:(NSDictionary *)dictionary;
- (id)initWithJSONDictionary:(NSDictionary *)dictionary;

@end
