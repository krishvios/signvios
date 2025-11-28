//
//  SCIPropertyManager+SettingItem.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+SettingItem.h"
#import "SCISettingItem.h"
#import "SCISettingItem+Convenience.h"
#import "SCIPropertyNames.h"

@implementation SCIPropertyManager (SettingItem)

- (void)setValueFromSettingItem:(SCISettingItem *)settingItem inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setValueFromSettingItem:settingItem
					inStorageLocation:location
					preStoreBlock:nil
				   postStoreBlock:nil];
}

- (void)setValueFromSettingItem:(SCISettingItem *)settingItem
			  inStorageLocation:(SCIPropertyStorageLocation)location
				  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
				 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
{
	[self setValue:settingItem.value
			ofType:SCIPropertyTypeFromSCISettingItemType(settingItem.type)
  forPropertyNamed:settingItem.name
 inStorageLocation:location
	 preStoreBlock:preStoreBlock
	postStoreBlock:postStoreBlock];
}

- (SCISettingItem *)settingItemOfType:(SCISettingItemType)type fromPropertyNamed:(NSString *)propertyName fromValueInStorageLocation:(SCIPropertyStorageLocation)location withDefault:(id)defaultValue
{
	id value = [self valueOfType:SCIPropertyTypeFromSCISettingItemType(type)
				forPropertyNamed:propertyName
			   inStorageLocation:location
					 withDefault:defaultValue];
	return [SCISettingItem settingItemWithName:propertyName value:value ofType:type];
}

@end

SCIPropertyType SCIPropertyTypeFromSCISettingItemType(SCISettingItemType type)
{
	SCIPropertyType res = SCIPropertyTypeInt;
	
	switch (type) {
		case SCISettingItemTypeBool:
		case SCISettingItemTypeEnum:
		case SCISettingItemTypeInt: {
			res = SCIPropertyTypeInt;
		} break;
		case SCISettingItemTypeString: {
			res = SCIPropertyTypeString;
		} break;
	}
	
	return res;
}
