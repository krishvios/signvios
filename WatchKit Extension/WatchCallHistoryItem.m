//
//  WatchCallHistoryItem.m
//  ntouch
//
//  Created by Kevin Selman on 6/26/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "WatchCallHistoryItem.h"

@implementation WatchCallHistoryItem

@synthesize callerName;
@synthesize callTime;

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
	self = [super init];
	
	if (self)
	{
		callerName = dictionary[@"callerName"];
		callTime = dictionary[@"callTime"];
	}
	
	return self;
}


@end
