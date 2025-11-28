//
//  BNRDispatchQueueWrapper.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/10/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "BNRDispatchQueueWrapper.h"

@implementation BNRDispatchQueueWrapper

@synthesize queue = mQueue;

+ (instancetype)wrapperForDispatchQueue:(dispatch_queue_t)queue
{
	return [[self alloc] initWithDispatchQueue:queue];
}

#pragma mark - Lifecycle

- (id)initWithDispatchQueue:(dispatch_queue_t)queue
{
	self = [super init];
	if (self) {
		self.queue = queue;
	}
	return self;
}

- (void)dealloc
{
	if (mQueue) {
		mQueue = nil;
	}
}

- (void)setQueue:(dispatch_queue_t)queue
{
	mQueue = queue;
}

@end
