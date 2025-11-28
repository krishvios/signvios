//
//  YelpCategory.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "YelpCategory.h"

@implementation YelpCategory

- (id)initWithJSONCategories:(NSDictionary *)categories
{
	self = [super init];
	if (self) {
		self.name = categories[@"title"];
		self.alias = categories[@"alias"];
	}
	return self;
}

@end
