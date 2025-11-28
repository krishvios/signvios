//
//  NSThread+PerformBlock.m
//  YelpDemo
//
//  Created by Nate Chandler on 6/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSThread+PerformBlock.h"

@implementation NSThread (PerformBlock)
- (void)performBlock:(void (^)())block
{
	if ([[NSThread currentThread] isEqual:self]) {
		block();
	} else {
		[self performBlock:block waitUntilDone:NO];
	}
}

- (void)performBlock:(void (^)())block waitUntilDone:(BOOL)wait
{
	if (block) {
		[NSThread performSelector:@selector(bnr_runBlock:)
						 onThread:self
					   withObject:block
					waitUntilDone:wait];
	}
}

+ (void)bnr_runBlock:(void (^)())block
{
	block();
}

@end
