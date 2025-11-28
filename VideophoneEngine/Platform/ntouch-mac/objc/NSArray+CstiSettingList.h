//
//  NSArray+CstiSettingList.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SstiSettingItem.h"
#include <vector>

@interface NSArray (CstiSettingList)
+ (NSArray *)arrayWithCstiSettingList:(const std::vector<SstiSettingItem> &)settingList;
@end
