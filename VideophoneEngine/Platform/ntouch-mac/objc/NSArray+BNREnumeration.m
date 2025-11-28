//
//  NSArray+BNREnumeration.m
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSArray+BNREnumeration.h"
#import "NSArray+Additions.h"

@interface NSArray (BNREnumerationPrivate)
- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue index:(NSUInteger)index continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock;
@end

@implementation NSArray (BNREnumeration)

- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock
{
	[self bnr_enumerateOnQueue:queue index:0 continuation:continuationBlock];
}

- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock completion:(void (^)())completionBlock
{
	completionBlock = [completionBlock copy];
	continuationBlock = [continuationBlock copy];
	[self bnr_enumerateOnQueue:queue continuation:^(id object, NSUInteger index, void (^block)(void)) {
		if (object) {
			continuationBlock(object, index, block);
		} else {
			completionBlock();
		}
	}];
}

@end

@implementation NSArray (BNREnumerationPrivate)

- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue index:(NSUInteger)index continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock
{
	continuationBlock = [continuationBlock copy];
	
	id obj = nil;
	void (^completionBlock)(void) = nil;
	
	if (self.count) {
		obj = [self objectAtIndex:0];
		
		NSArray *tail = [self tailArray];
		completionBlock = ^ {
			[tail bnr_enumerateOnQueue:queue index:index+1 continuation:continuationBlock];
		};
	} else {
		index = NSNotFound;
	}
	
	if (queue) {
		dispatch_async(queue, ^{
			continuationBlock(obj, index, completionBlock);
		});
	} else {
		continuationBlock(obj, index, completionBlock);
	}
}

@end