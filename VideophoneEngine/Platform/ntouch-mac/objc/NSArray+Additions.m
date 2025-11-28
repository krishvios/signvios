//
//  NSArray+Additions.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSArray+Additions.h"

@implementation NSArray (Additions)
- (NSArray *)filteredArrayUsingBlock:(BOOL (^)(id obj))block
{
	NSMutableArray *mutableFilteredArray = [[NSMutableArray alloc] init];
	for (id obj in self) {
		if (block(obj)) {
			[mutableFilteredArray addObject:obj];
		}
	}
	return [mutableFilteredArray copy];
}

- (NSArray *)tailArray
{
	if (!self.count) return self;
	NSMutableArray *mutableRes = [self mutableCopy];
	[mutableRes removeObjectAtIndex:0];
	return [mutableRes copy];
}
@end
