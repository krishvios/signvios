//
//  NSDate+BNRAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/8/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSDate+BNRAdditions.h"

@implementation NSDate (BNRAdditions)

+ (id)bnr_notSoDistantFuture
{
	id res = nil;
	
	int secondsAfterUnixEpoch = INT32_MAX;
	NSTimeInterval timeIntervalAfterUnixEpoch = (NSTimeInterval)secondsAfterUnixEpoch;
	res = [NSDate dateWithTimeIntervalSince1970:timeIntervalAfterUnixEpoch];
	
	return res;
}

+ (id)bnr_notSoDistantPast
{
	id res = nil;
	
	int secondsBeforeUnixEpoch = INT32_MIN;
	NSTimeInterval timeIntervalBeforeUnixEpoch = (NSTimeInterval)secondsBeforeUnixEpoch;
	res = [NSDate dateWithTimeIntervalSince1970:timeIntervalBeforeUnixEpoch];
	
	return res;
}

@end
