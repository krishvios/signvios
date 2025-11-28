//
//  SCISettingItem+SstiSettingItem.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCISettingItem+SstiSettingItem.h"

static SCISettingItemType SCISettingItemTypeFromEstiSettingType(SstiSettingItem::EstiSettingType estiSettingType);

@implementation SCISettingItem (SstiSettingItem)

- (id)initWithSstiSettingItem:(SstiSettingItem *)sstiSettingItem
{
	self = [self init];
	if (self) {
		self.name = [NSString stringWithUTF8String:sstiSettingItem->Name.c_str()];
		SCISettingItemType type = SCISettingItemTypeFromEstiSettingType(sstiSettingItem->eType);
		self.type = type;
		
		NSString *stringValue = [NSString stringWithUTF8String:sstiSettingItem->Value.c_str()];
		switch (type) {
			case SCISettingItemTypeString: {
				self.value = stringValue;
			} break;
			case SCISettingItemTypeInt: {
				self.value = [NSNumber numberWithInt:[stringValue intValue]];
			} break;
			case SCISettingItemTypeBool: {
				self.value = [NSNumber numberWithBool:[stringValue boolValue]];
			} break;
			case SCISettingItemTypeEnum: {
				self.value = [NSNumber numberWithInt:[stringValue intValue]];
			} break;
		}
	}
	return self;
}

@end

static SCISettingItemType SCISettingItemTypeFromEstiSettingType(SstiSettingItem::EstiSettingType estiSettingType)
{
	switch (estiSettingType) {
		case SstiSettingItem::estiString: return SCISettingItemTypeString;
		case SstiSettingItem::estiInt: return SCISettingItemTypeInt;
		case SstiSettingItem::estiBool: return SCISettingItemTypeBool;
		case SstiSettingItem::estiEnum: return SCISettingItemTypeEnum;
	}
}