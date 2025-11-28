//
//  BNRBackgroundRunLoopThread.h
//  YelpDemo
//
//  Created by Nate Chandler on 6/21/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface BNRBackgroundRunLoopThread : NSThread

- (void)addRunloopSourceAddingBlock:(void (^)(BNRBackgroundRunLoopThread *, NSRunLoop *))block launchImmediately:(BOOL)launchImmediately;
- (void)addRunloopSourceAddingBlock:(void (^)(BNRBackgroundRunLoopThread *thread, NSRunLoop *runloop))block;
- (void)exit:(id)sender waitUntilDone:(BOOL)block;

@end
