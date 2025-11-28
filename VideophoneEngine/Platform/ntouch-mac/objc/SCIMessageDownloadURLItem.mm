//
//  SCIMessageDownloadURLItem.m
//  ntouch
//
//  Created by Kevin Selman on 3/25/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

#import "SCIMessageDownloadURLItem.h"


@implementation SCIMessageDownloadURLItem

+ (SCIMessageDownloadURLItem *)itemWithCstiMessageDownloadURLItem:(CstiMessageResponse::CstiMessageDownloadURLItem const&)item
{
	SCIMessageDownloadURLItem *output = [[SCIMessageDownloadURLItem alloc] init];
	output.downloadURL = [NSString stringWithUTF8String:item.m_DownloadURL.c_str()];
	output.fileSize = item.m_nFileSize;
	output.maxBitRate = item.m_nMaxBitRate;
	output.width = item.m_nWidth;
	output.height = item.m_nHeight;
	output.m_eVideoCodec = [NSString stringWithUTF8String:item.m_Codec.c_str()];
	output.m_eProfile = [NSString stringWithUTF8String:item.m_Profile.c_str()];
	output.m_eLevel = [NSString stringWithUTF8String:item.m_Level.c_str()];
	
	return output;
}

- (id)copyWithZone:(NSZone *)zone
{
	SCIMessageDownloadURLItem *copy = [[SCIMessageDownloadURLItem allocWithZone:zone] init];
	NSArray *keys = [NSArray arrayWithObjects:@"downloadURL", @"fileSize", @"maxBitRate", @"width", @"height", @"m_eVideoCodec", @"m_eProfile", @"m_eLevel", nil];
	for (NSString *key in keys)
	{
		[copy setValue:[self valueForKey:key] forKey:key];
	}
	return copy;
}

@end
