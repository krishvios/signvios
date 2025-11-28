//
//  SCISettingItem+Convenience.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCISettingItem.h"

@interface SCISettingItem (Convenience)

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName utf8StringValue:(const char *)utf8StringValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName intValue:(int)intValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName boolValue:(BOOL)boolValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName enumValue:(int)enumValue;

//Specialized Constructors: Primitive Names, Object Values
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName stringValue:(NSString *)stringValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName intNumberValue:(NSNumber *)intNumberValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName boolNumberValue:(NSNumber *)boolNumberValue;
+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName enumNumberValue:(NSNumber *)enumNumberValue;

//Specialized Constructors: Object Names, Primitive Values
+ (SCISettingItem *)settingItemWithName:(NSString *)name utf8StringValue:(const char *)utf8StringValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name intValue:(int)intValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name boolValue:(BOOL)boolValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name enumValue:(int)enumValue;

//Specialized Constructors: Object Names, Object Values
+ (SCISettingItem *)settingItemWithName:(NSString *)name stringValue:(NSString *)stringValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name intNumberValue:(NSNumber *)intNumberValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name boolNumberValue:(NSNumber *)boolNumberValue;
+ (SCISettingItem *)settingItemWithName:(NSString *)name enumNumberValue:(NSNumber *)enumNumberValue;

//"Designated" General Factory Method:Object Name, Object Value
+ (SCISettingItem *)settingItemWithName:(NSString *)name value:(id)value ofType:(SCISettingItemType)type;

@end
