//
//  WatchSignMailItem.m
//  ntouch
//
//  Created by Kevin Selman on 6/26/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "WatchSignMailItem.h"

@implementation WatchSignMailItem

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
	self = [super init];
	
	if (self)
	{
		self.callerName = dictionary[@"callerName"];
		self.callTime = dictionary[@"callTime"];
		self.videoURL = [NSURL URLWithString:dictionary[@"videoURL"]];
		self.viewId = dictionary[@"viewId"];
		self.messageId = dictionary[@"messageId"];
		self.pausePoint = [dictionary[@"pausePoint"] floatValue];
	}
	
	return self;
}


@end
