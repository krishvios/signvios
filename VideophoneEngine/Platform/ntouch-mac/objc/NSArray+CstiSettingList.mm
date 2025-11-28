//
//  NSArray+CstiSettingList.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSArray+CstiSettingList.h"
#import "SCISettingItem.h"
#import "SCISettingItem+SstiSettingItem.h"
#import <vector>

@implementation NSArray (CstiSettingList)

+ (NSArray *)arrayWithCstiSettingList:(const std::vector<SstiSettingItem> &)settingList
{
	if (!settingList.empty()) {
		NSMutableArray *array = [NSMutableArray array];
		
		for (auto item : settingList) {
			SCISettingItem *settingItem = [[SCISettingItem alloc] initWithSstiSettingItem:&item];
			[array addObject:settingItem];
		}
		return [array copy];
	}
	return nil;
}

@end
