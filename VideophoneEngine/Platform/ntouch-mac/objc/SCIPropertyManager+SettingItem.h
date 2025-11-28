//
//  SCIPropertyManager+SettingItem.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"
#import "SCISettingItem.h"

@interface SCIPropertyManager (SettingItem)

- (void)setValueFromSettingItem:(SCISettingItem *)settingItem
			  inStorageLocation:(SCIPropertyStorageLocation)location;
- (void)setValueFromSettingItem:(SCISettingItem *)settingItem
			  inStorageLocation:(SCIPropertyStorageLocation)location
				  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
				 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock;
- (SCISettingItem *)settingItemOfType:(SCISettingItemType)type
					fromPropertyNamed:(NSString *)propertyName
		   fromValueInStorageLocation:(SCIPropertyStorageLocation)location
						  withDefault:(id)defaultValue;

@end

#ifdef __cplusplus
extern "C" {
#endif
	SCIPropertyType SCIPropertyTypeFromSCISettingItemType(SCISettingItemType type);
#ifdef __cplusplus
}
#endif
