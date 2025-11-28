//
//  SCIBlocked.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIBlocked.h"
#import "SCIItemId.h"
#import "NSString+SCIAdditions.h"

@interface SCIBlocked ()
{
	SCIItemId *mBlockedId;
	NSString *mTitle;
	NSString *mNumber;
}

@end

@implementation SCIBlocked

@synthesize blockedId = mBlockedId;
@synthesize title = mTitle;
@synthesize number = mNumber;

+ (SCIBlocked *)blockedWithItemId:(SCIItemId *)itemId number:(NSString *)number title:(NSString *)title
{
	return [[SCIBlocked alloc] initWithItemId:itemId number:number title:title];
}

- (id)initWithItemId:(SCIItemId *)blockedId number:(NSString *)number title:(NSString *)title
{
	self = [super init];
	if (self) {
		self.blockedId = blockedId;
		self.number = number;
		self.title = title;
	}
	return self;
}

- (id)copyWithZone:(NSZone *)zone
{
	__typeof__(self) copy = [[self.class alloc] init];
	copy.blockedId = self.blockedId;
	copy.title = self.title;
	copy.number = self.number;
	return copy;
}


- (NSComparisonResult)compare:(SCIBlocked *)comparand
{
	return [self.title sci_compareAlphaFirst:comparand.title];
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	if ([object isMemberOfClass:[SCIBlocked class]])
	{
		res = [(SCIBlocked *)self isEqualToBlocked:object];
	}
	return res;
}

- (NSUInteger)hash
{
	return self.blockedId.hash;
}

- (BOOL)isEqualToBlocked:(SCIBlocked *)blocked
{
	return [self.blockedId isEqualToItemId:blocked.blockedId];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p title:%@ number:%@>", [self class], self, self.title, self.number];
}

@end
