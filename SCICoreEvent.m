//
//  SCICoreEvent.m
//  ntouch
//
//  Created by Kevin Selman on 6/18/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCICoreEvent.h"

@implementation SCICoreEvent

- (id)initWithCoder:(NSCoder *)decoder {
	if (self = [super init]) {
		self.priority = [decoder decodeObjectForKey:@"priority"];
		self.type = [decoder decodeObjectForKey:@"type"];
	}
	return self;
}

- (void)encodeWithCoder:(NSCoder *)encoder {
	[encoder encodeObject:self.priority forKey:@"priority"];
	[encoder encodeObject:self.type forKey:@"type"];

}

@end
