//
//  NSThread+PerformBlock.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSRunLoop (BNRPerformBlock)

- (void)bnr_performWithOrder:(NSUInteger)order modes:(NSArray *)modes block:(void (^)())block;
+ (void)bnr_performOnMainRunLoopWithOrder:(NSUInteger)order modes:(NSArray *)modes block:(void (^)())block;

// Perform the block with NSDefaultRunLoopMode.
// This allows the main queue to continue to process queued operations while this block
// is blocking. For example, while it is running a modal window *from inside a queued block*.
- (void)bnr_performBlock:(void (^)())block;
+ (void)bnr_performBlockOnMainRunLoop:(void (^)())block;

@end
