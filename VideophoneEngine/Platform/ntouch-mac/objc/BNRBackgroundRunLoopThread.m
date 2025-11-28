//
//  BNRBackgroundRunLoopThread.m
//  YelpDemo
//
//  Created by Nate Chandler on 6/21/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "BNRBackgroundRunLoopThread.h"
#import "BNRBackgroundRunLoopThread_Internal.h"
#import "NSThread+PerformBlock.h"

@interface BNRBackgroundRunLoopThread ()

@property (nonatomic) NSMutableArray *runloopSourceAddingBlocks;
@property (nonatomic) NSCondition *exitingCondition;
@property (nonatomic) NSPort *stayAlivePort;

@end

@implementation BNRBackgroundRunLoopThread

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.runloopSourceAddingBlocks = [[NSMutableArray alloc] init];
		self.exitingCondition = [[NSCondition alloc] init];
		self.stayAlivePort = [NSPort port];
	}
	return self;
}

- (void)dealloc
{
	[self.stayAlivePort invalidate];
}

- (void)main
{
	@autoreleasepool
	{
		do {
			self.runloop = [NSRunLoop currentRunLoop];
			[self.runloop addPort:self.stayAlivePort forMode:NSDefaultRunLoopMode];
				
			[self enumerateAndPopRunloopSourceAddingBlocksWithBlock:^(void (^addRunloopSourceAddingBlock)(BNRBackgroundRunLoopThread *, NSRunLoop *), BOOL *stop) {
				addRunloopSourceAddingBlock(self, self.runloop);
			}];
			
			[self.runloop run];
		} while (!self.isCancelled);
	}
}

- (void)cancel
{
	[self performBlock:^{
		[self.runloop removePort:self.stayAlivePort forMode:NSDefaultRunLoopMode];
	}
		 waitUntilDone:YES];
	[super cancel];
}

- (void)exit:(id)sender waitUntilDone:(BOOL)block
{
	if (block) {
		[self.exitingCondition lock];
	}
	
#if TARGET_OS_IPHONE
	[self performSelector:@selector(doExit:) onThread:self withObject:nil waitUntilDone:NO modes:@[NSRunLoopCommonModes, NSDefaultRunLoopMode, UITrackingRunLoopMode]];
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	[self performSelector:@selector(doExit:) onThread:self withObject:nil waitUntilDone:NO modes:@[NSRunLoopCommonModes, NSDefaultRunLoopMode, NSModalPanelRunLoopMode, NSEventTrackingRunLoopMode]];
#endif

	if (block) {
		[self.exitingCondition wait];
		[self.exitingCondition unlock];
	}
}

- (void)doExit:(id)sender
{
	[self.exitingCondition lock];
	[self.exitingCondition signal];
	[self.exitingCondition unlock];
	[NSThread exit];
}

#pragma mark - Public API

- (void)addRunloopSourceAddingBlock:(void (^)(BNRBackgroundRunLoopThread *, NSRunLoop *))block launchImmediately:(BOOL)launchImmediately
{
	if (block) {
		if (self.isExecuting) {
			[self performBlock:^{
				block(self, self.runloop);
			}
				 waitUntilDone:YES];
		} else {
			@synchronized(self.runloopSourceAddingBlocks)
			{
				[self.runloopSourceAddingBlocks addObject:block];
			}
			if (launchImmediately) {
				[self start];
			}
		}
	}
}

- (void)addRunloopSourceAddingBlock:(void (^)(BNRBackgroundRunLoopThread *thread, NSRunLoop *runloop))block
{
	[self addRunloopSourceAddingBlock:block launchImmediately:YES];
}

- (void)removeAllObjectsFromRunloopSourceAddingBlocks
{
	@synchronized(self.runloopSourceAddingBlocks)
	{
		[self.runloopSourceAddingBlocks removeAllObjects];
	}
}

- (NSUInteger)countOfRunloopSourceAddingBlocks
{
	NSUInteger res = 0;
	
	@synchronized(self.runloopSourceAddingBlocks)
	{
		res = self.runloopSourceAddingBlocks.count;
	}
	
	return res;
}

- (void)enumerateAddRunloopSourceAddingBlocksWithBlock:(void (^)(void (^addRunloopSourceAddingBlock)(BNRBackgroundRunLoopThread *, NSRunLoop *), BOOL *stop))block
{
	@synchronized(self.runloopSourceAddingBlocks)
	{
		for (void (^addRunloopSourceAddingBlock)(BNRBackgroundRunLoopThread *, NSRunLoop *) in self.runloopSourceAddingBlocks) {
			BOOL stop = NO;
			
			block(addRunloopSourceAddingBlock, &stop);
			
			if (stop) {
				break;
			}
				
		}
	}
}

- (void)enumerateAndPopRunloopSourceAddingBlocksWithBlock:(void (^)(void (^addRunloopSourceAddingBlock)(BNRBackgroundRunLoopThread *, NSRunLoop *), BOOL *stop))block
{
	@synchronized(self.runloopSourceAddingBlocks)
	{
		NSMutableArray *blocksToRemove = [[NSMutableArray alloc] init];
		
		for (void (^addRunloopSourceAddingBlock)(BNRBackgroundRunLoopThread *, NSRunLoop *) in self.runloopSourceAddingBlocks) {
			BOOL stop = NO;
			
			block(addRunloopSourceAddingBlock, &stop);
			
			[blocksToRemove addObject:addRunloopSourceAddingBlock];
			
			if (stop) {
				break;
			}
			
		}
		
		[self.runloopSourceAddingBlocks removeObjectsInArray:blocksToRemove];
	}

}

@end
