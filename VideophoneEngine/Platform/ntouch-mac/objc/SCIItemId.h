//
//  SCIItemId.h
//  ntouchMac
//
//  Created by Adam Preble on 5/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCIItemId : NSObject <NSCopying>

- (BOOL)isEqualToItemId:(SCIItemId *)other;

@property (readonly) NSString *string;

@end
