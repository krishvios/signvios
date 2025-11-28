//
//  NSArray+BNRAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSArray+BNRAdditions.h"

@implementation NSArray (BNRAdditions)
- (NSArray *)bnr_arrayByRemovingFirstInstanceOfObject:(id)object
{
	NSMutableArray *mutableRes = [self mutableCopy];
	for (NSUInteger i = 0; i < mutableRes.count; i++) {
		id obj = mutableRes[i];
		if ([obj isEqual:object]) {
			[mutableRes removeObjectAtIndex:i];
			break;
		}
	}
	return [mutableRes copy];
}

- (NSArray *)bnr_arrayByRemovingObject:(id)object
{
	NSMutableArray *mutableRes = [self mutableCopy];
	[mutableRes removeObject:object];
	return [mutableRes copy];
}

- (NSArray *)bnr_arrayByRemovingObjectAtIndex:(NSUInteger)index
{
	NSMutableArray *mutableRes = [self mutableCopy];
	[mutableRes removeObjectAtIndex:index];
	return [mutableRes copy];
}

- (NSArray *)bnr_arrayWithDifferenceInArray:(NSArray *)subtractand
{
	NSMutableArray *res = [NSMutableArray array];
	for (id obj in self) {
		if (![subtractand containsObject:obj]) {
			[res addObject:obj];
		}
	}
	return [res copy];
}

- (NSUInteger)bnr_indexBeforeIndex:(NSUInteger)index wrapping:(BOOL)wrapping
{
	NSUInteger newIndex = NSNotFound;
	
	NSUInteger count = self.count;
	
	if (count > 0) {
		if (index > 0) {
			if (index < count) {
				newIndex = index - 1;
			} else {
				newIndex = count - 1;
			}
		} else {
			if (wrapping) {
				newIndex = count - 1;
			} else {
				newIndex = 0;
			}
		}
	}
	
	return newIndex;
}

- (NSUInteger)bnr_indexAfterIndex:(NSUInteger)index wrapping:(BOOL)wrapping
{
	NSUInteger newIndex = NSNotFound;
	
	NSUInteger count = self.count;
	
	if (count > 0) {
		if (index < count) {
			if (index < count - 1) {
				newIndex = index + 1;
			} else {
				if (wrapping) {
					newIndex = 0;
				} else {
					newIndex = count - 1;
				}
			}
		} else {
			newIndex = count - 1;
		}
	}
	
	return newIndex;
}

- (NSArray *)bnr_arrayByReplacingObjectAtIndex:(NSUInteger)index withObject:(id)object
{
	NSMutableArray *mutableRes = [self mutableCopy];
	[mutableRes replaceObjectAtIndex:index withObject:object];
	return [mutableRes copy];
}

- (NSArray *)bnr_trucatedArrayWithAtMost:(NSInteger)maxNumberOfItems
{
	NSArray *res = nil;
	
	if (self.count > maxNumberOfItems) {
		res = [self subarrayWithRange:NSMakeRange(0, maxNumberOfItems)];
	} else {
		res = [self copy];
	}
	
	return res;
}

- (NSArray *)bnr_arrayByPerformingBlock:(id (^)(id object, NSUInteger index))block
{
	NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
	
	NSUInteger index = 0;
	for (id object in self) {
		[mutableRes addObject:block(object, index)];
		index++;
	}
	
	return [mutableRes copy];
}

- (id)bnr_firstObject
{
	id res = nil;
	
	if (self.count > 0) {
		res = self[0];
	} else {
		res = nil;
	}
	
	return res;
}

@end
