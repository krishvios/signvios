//
//  SCISettingItem.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
	SCISettingItemTypeString,
	SCISettingItemTypeInt,
	SCISettingItemTypeBool,
	SCISettingItemTypeEnum
} SCISettingItemType;

extern NSString *NSStringFromSCISettingItemType(SCISettingItemType type);

@interface SCISettingItem : NSObject

- (id)initWithName:(NSString *)name value:(id)value type:(SCISettingItemType)type;

@property (nonatomic, strong, readwrite) NSString *name;
@property (nonatomic, assign, readwrite) SCISettingItemType type;
@property (nonatomic, strong, readwrite) id value;

@end
