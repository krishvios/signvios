//
//  SCISettingItem+Convenience.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCISettingItem+Convenience.h"

@implementation SCISettingItem (Convenience)

#pragma mark - Specialized Factory Methods: Primitive Name, Primitive Value

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName utf8StringValue:(const char *)utf8StringValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] utf8StringValue:utf8StringName];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName intValue:(int)intValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] intValue:intValue];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName boolValue:(BOOL)boolValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] boolValue:boolValue];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName enumValue:(int)enumValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] enumValue:enumValue];
}

#pragma mark - Specialized Factory Methods: Primitive Name, Object Value

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName stringValue:(NSString *)stringValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] stringValue:stringValue];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName intNumberValue:(NSNumber *)intNumberValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] intNumberValue:intNumberValue];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName boolNumberValue:(NSNumber *)boolNumberValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] enumNumberValue:boolNumberValue];
}

+ (SCISettingItem *)settingItemWithUTF8StringName:(const char *)utf8StringName enumNumberValue:(NSNumber *)enumNumberValue
{
	return [self settingItemWithName:[NSString stringWithUTF8String:utf8StringName] enumNumberValue:enumNumberValue];
}

#pragma mark - Specialized Factory Methods: Object Names, Primitive Values

+ (SCISettingItem *)settingItemWithName:(NSString *)name utf8StringValue:(const char *)utf8StringValue
{
	return [self settingItemWithName:name stringValue:[NSString stringWithUTF8String:utf8StringValue]];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name intValue:(int)intValue
{
	return [self settingItemWithName:name intNumberValue:[NSNumber numberWithInt:intValue]];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name boolValue:(BOOL)boolValue
{
	return [self settingItemWithName:name boolNumberValue:[NSNumber numberWithBool:boolValue]];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name enumValue:(int)enumValue
{
	return [self settingItemWithName:name enumNumberValue:[NSNumber numberWithInt:enumValue]];
}

#pragma mark - Specialized Factory Methods: Object Names, Object Values

+ (SCISettingItem *)settingItemWithName:(NSString *)name stringValue:(NSString *)stringValue
{
	return [self settingItemWithName:name value:stringValue ofType:SCISettingItemTypeString];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name intNumberValue:(NSNumber *)intNumberValue
{
	return [self settingItemWithName:name value:intNumberValue ofType:SCISettingItemTypeInt];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name boolNumberValue:(NSNumber *)boolNumberValue
{
	return [self settingItemWithName:name value:boolNumberValue ofType:SCISettingItemTypeBool];
}

+ (SCISettingItem *)settingItemWithName:(NSString *)name enumNumberValue:(NSNumber *)enumNumberValue
{
	return [self settingItemWithName:name value:enumNumberValue ofType:SCISettingItemTypeEnum];
}

#pragma mark - "Designated" General Factory Method: Object Name, Object Value

+ (SCISettingItem *)settingItemWithName:(NSString *)name value:(id)value ofType:(SCISettingItemType)type
{
	return [[SCISettingItem alloc] initWithName:name value:value type:type];
}

@end
