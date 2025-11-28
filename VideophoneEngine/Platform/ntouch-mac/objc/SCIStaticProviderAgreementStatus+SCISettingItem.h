//
//  SCIStaticProviderAgreementStatus+SCISettingItem.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIStaticProviderAgreementStatus.h"

@class SCISettingItem;

@interface SCIStaticProviderAgreementStatus (SCISettingItem)

+ (instancetype)statusFromSettingItem:(SCISettingItem *)settingItem;

@end
