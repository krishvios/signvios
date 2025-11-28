//
//  SCIStaticProviderAgreementStatus+SCISettingItem.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIStaticProviderAgreementStatus+SCISettingItem.h"
#import "SCISettingItem.h"
#import "SCIPropertyNames.h"

@implementation SCIStaticProviderAgreementStatus (SCISettingItem)

+ (instancetype)statusFromSettingItem:(SCISettingItem *)settingItem
{
	id res = nil;
	
	if (settingItem.type == SCISettingItemTypeString &&
		[settingItem.name isEqualToString:SCIPropertyNameProviderAgreementStatus]) {
		res = [SCIStaticProviderAgreementStatus providerAgreementStatusFromString:(NSString *)settingItem.value];
	}
	
	return res;
}

@end
