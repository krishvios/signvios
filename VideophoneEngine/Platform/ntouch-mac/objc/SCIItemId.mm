//
//  SCIItemId.m
//  ntouchMac
//
//  Created by Adam Preble on 5/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIItemId.h"

#import "stiDefs.h"
#import "CstiItemId.h"
#import <string>
#import <stdio.h>

@interface SCIItemId () {
	CstiItemId mItemId;
}
@end

@implementation SCIItemId

+ (SCIItemId *)itemIdWithCstiItemId:(CstiItemId)cstiItemId
{
	SCIItemId *output = [[SCIItemId alloc] init];
	output->mItemId = cstiItemId;
	return output;
}

+ (SCIItemId *)itemIdWithItemIdString:(const char *)itemIdString;
{
	SCIItemId *output = [[SCIItemId alloc] init];
	output->mItemId = CstiItemId(itemIdString);
	return output;
}

- (CstiItemId)cstiItemId
{
	return mItemId;
}

- (BOOL)isEqualToItemId:(SCIItemId *)other
{
	return (self.cstiItemId == other.cstiItemId);
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	if ([object isMemberOfClass:[SCIItemId class]]) {
		SCIItemId *comparand = (SCIItemId *)object;
		res = [self isEqualToItemId:comparand];
	}
	return res;
}

- (NSUInteger)hash
{
	uint32_t un32ItemId = 0;
	
	int itemIdType = self.cstiItemId.ItemIdTypeGet();
	stiHResult res;
	if (itemIdType == estiITEM_ID_TYPE_INT) {
		res = self.cstiItemId.ItemIdGet(&un32ItemId);
	} else if (itemIdType == estiITEM_ID_TYPE_CHAR)
	{
		std::string strItemId;
		res = self.cstiItemId.ItemIdGet(&strItemId);
		const char *pszItemIdBuf = strItemId.c_str();
		NSString *itemIdString = pszItemIdBuf ? [NSString stringWithUTF8String:pszItemIdBuf] : nil;
		sscanf(itemIdString.UTF8String, "%u", &un32ItemId);
	}
	return (NSUInteger)un32ItemId;
}

- (NSString *)string
{
	int itemIdType = self.cstiItemId.ItemIdTypeGet();
	if (itemIdType == estiITEM_ID_TYPE_INT) {
		uint32_t itemId = 0;
		stiHResult res = self.cstiItemId.ItemIdGet(&itemId);
		
		if (stiRESULT_SUCCESS == res) {
			return [@"i" stringByAppendingString:@(itemId).stringValue];
		}
		
	} else if (itemIdType == estiITEM_ID_TYPE_CHAR) {
		std::string itemId;
		stiHResult res = self.cstiItemId.ItemIdGet(&itemId);
		const char *itemIdBuf = itemId.c_str();

		if (stiRESULT_SUCCESS == res && itemIdBuf) {
			return [@"s" stringByAppendingString:[NSString stringWithUTF8String:itemIdBuf]];
		}
	}
	
	// Otherwise this is an invalid id.
	return nil;
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone *)zone
{
	SCIItemId *copy = [[SCIItemId alloc] init];
	copy->mItemId = self.cstiItemId;
	return copy;
}

@end
