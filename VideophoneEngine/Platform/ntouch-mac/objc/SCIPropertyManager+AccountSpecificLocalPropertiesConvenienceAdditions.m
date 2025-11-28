//
//  SCIPropertyManager+AccountSpecificLocalPropertiesConvenienceAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenienceAdditions.h"
#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.h"

@implementation SCIPropertyManager (AccountSpecificLocalPropertiesConvenienceAdditions)

- (NSString *)propertyNameWithNameComponent:(NSString *)nameComponent key:(NSString *)key
{
	return [NSString stringWithFormat:@"%@_%@", nameComponent, key];
}

- (NSDate *)dateForAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
														key:(NSString *)key
										  inStorageLocation:(SCIPropertyStorageLocation)location
{
	NSDate *res = nil;
	
	NSString *propertyName = [self propertyNameWithNameComponent:nameComponent key:key];
	
	int dateInt = [self intValueForAccountSpecificPropertyNamed:propertyName
											  inStorageLocation:location
													withDefault:INT_MIN];
	
	if (dateInt != INT_MIN) {
		res = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)dateInt];
	}
	
	return res;
}
- (void)setDate:(NSDate *)date
forAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
			key:(NSString *)key
inStorageLocation:(SCIPropertyStorageLocation)location
{
	NSString *propertyName = [self propertyNameWithNameComponent:nameComponent key:key];
	
	if (date) {
		int dateInt = (int)[date timeIntervalSince1970];
		[self setIntValue:dateInt
forAccountSpecificPropertyNamed:propertyName
		inStorageLocation:location];
	} else {
		[self removeValueForAccountSpecificPropertyNamed:propertyName
									   inStorageLocation:location];
	}
	if (location == SCIPropertyStorageLocationPersistent) {
		[self saveToPersistentStorage];
	}
}

- (BOOL)boolForAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
													key:(NSString *)key
									  inStorageLocation:(SCIPropertyStorageLocation)location
											withDefault:(BOOL)defaultValue
{
	BOOL res = defaultValue;
	
	NSString *propertyName = [self propertyNameWithNameComponent:nameComponent key:key];
	int defaultValueInt = (defaultValue) ? 1 : 0;
	int valueInt = [self intValueForAccountSpecificPropertyNamed:propertyName
											   inStorageLocation:location
													 withDefault:defaultValueInt];
	res = (valueInt == 1) ? YES : NO;
	
	return res;
}
- (void)setBool:(BOOL)value
forAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
			key:(NSString *)key
inStorageLocation:(SCIPropertyStorageLocation)location
{
	int valueInt = (value) ? 1 : 0;
	NSString *propertyName = [self propertyNameWithNameComponent:nameComponent
															 key:key];
	[self setIntValue:valueInt
forAccountSpecificPropertyNamed:propertyName
	inStorageLocation:location];
	if (location == SCIPropertyStorageLocationPersistent) {
		[self saveToPersistentStorage];
	}
}

@end
