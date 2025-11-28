//
//  YelpRegion.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/2/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YelpRegion : NSObject

#if TARGET_OS_IPHONE
@property (nonatomic) CGPoint center;
@property (nonatomic) CGSize span;
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
@property (nonatomic) NSPoint center;
@property (nonatomic) NSSize span;
#endif





@end
