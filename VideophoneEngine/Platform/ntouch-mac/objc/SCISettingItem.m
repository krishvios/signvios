//
//  SCISettingItem.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCISettingItem.h"

@interface SCISettingItem ()
{
	NSString *mName;
	SCISettingItemType mType;
	id mValue;
}
@end

@implementation SCISettingItem

#pragma mark - Lifecycle

- (id)initWithName:(NSString *)name value:(id)value type:(SCISettingItemType)type
{
	self = [super init];
	if (self) {
		self.name = name;
		self.value = value;
		self.type = type;
	}
	return self;
}

#pragma mark - NSObject overrides

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p: %@, %@, %@>", [SCISettingItem class], self, self.name, NSStringFromSCISettingItemType(self.type), self.value];
}

@end

NSString *NSStringFromSCISettingItemType(SCISettingItemType type) 
{
#define HANDLECASE(type) case type: { return [NSString stringWithUTF8String:( #type )]; } break;
	switch (type) {
			HANDLECASE(SCISettingItemTypeString);
			HANDLECASE(SCISettingItemTypeInt);
			HANDLECASE(SCISettingItemTypeBool);
			HANDLECASE(SCISettingItemTypeEnum);
	}
#undef HANDLECASE
}