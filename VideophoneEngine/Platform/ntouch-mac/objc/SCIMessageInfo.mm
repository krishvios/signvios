//
//  SCIMessageInfo.m
//  ntouchMac
//
//  Created by Adam Preble on 5/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageInfo.h"

#import "stiSVX.h"
#import "CstiMessageInfo.h"
#import "SCIItemIdCpp.h"
#import "SCIItemId.h"
#import "CstiItemId.h"

@implementation SCIMessageInfo

@synthesize messageId, name, date, duration, viewed, dialString, pausePoint, previewImageURL, categoryId, typeId;

+ (SCIMessageInfo *)infoWithCstiMessageInfo:(CstiMessageInfo &)info category:(CstiItemId &)category
{
	SCIMessageInfo *output = [[SCIMessageInfo alloc] init];
	output.categoryId = [SCIItemId itemIdWithCstiItemId:category]; 
	output.messageId = [SCIItemId itemIdWithCstiItemId:info.ItemIdGet()];
	output.date = [NSDate dateWithTimeIntervalSince1970:info.DateTimeGet()];
	output.name = info.NameGet() ? [NSString stringWithUTF8String:info.NameGet()] : nil;
	output.duration = info.MessageLengthGet();
	output.pausePoint = info.PausePointGet();
	output.dialString = info.DialStringGet() ? [NSString stringWithUTF8String:info.DialStringGet()] : nil;
	output.interpreterId = info.InterpreterIdGet() ? [NSString stringWithUTF8String:info.InterpreterIdGet()] : nil;
	output.previewImageURL = info.PreviewImageURLGet() ? [NSString stringWithUTF8String:info.PreviewImageURLGet()]: nil;
	if (output.interpreterId.length == 0) {
		output.interpreterId = nil;
	}
	output.viewed = info.ViewedGet();
	output.typeId = info.MessageTypeIdGet();
	return output;
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@ %p name=%@ date=%@ viewed=%d dialString=%@ typeId=%lu>", NSStringFromClass([self class]), self, self.name, self.date, self.viewed, self.dialString, (unsigned long)self.typeId];
}

- (id)copyWithZone:(NSZone *)zone
{
	SCIMessageInfo *copy = [[SCIMessageInfo allocWithZone:zone] init];
	NSArray *keys = [NSArray arrayWithObjects:@"messageId", @"categoryId", @"date", @"name", @"duration", @"pausePoint", @"dialString", @"viewed", @"typeId", nil];
	for (NSString *key in keys)
	{
		[copy setValue:[self valueForKey:key] forKey:key];
	}
	return copy;
}

- (BOOL)isEqual:(id)object
{
	if (![object isMemberOfClass:[SCIMessageInfo class]]) {
		return NO;
	}
	
	SCIMessageInfo *comparand = (SCIMessageInfo *)object;
	return [self.messageId isEqualToItemId:comparand.messageId];
}

- (NSUInteger)hash
{
	return self.messageId.hash;
}

@end
