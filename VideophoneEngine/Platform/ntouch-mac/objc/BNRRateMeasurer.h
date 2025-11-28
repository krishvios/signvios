//
//  BNRRateMeasurer.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface BNRRateMeasurer : NSObject
- (id)initWithSampleCapacity:(NSUInteger)count;
- (void)tick;
- (NSTimeInterval)averageRate;
@end
