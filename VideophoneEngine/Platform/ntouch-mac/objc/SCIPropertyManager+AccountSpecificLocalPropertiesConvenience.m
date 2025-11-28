//
//  SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.h"
#import "SCIVideophoneEngine.h"

static NSString * const SCIPropertyManagerKeyPathEnginePhoneUserId;
static NSString * const SCIPropertyManagerKeyPathEngineRingGroupUserId;

@implementation SCIPropertyManager (AccountSpecificLocalPropertiesConvenience)

#pragma mark - Convenience

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

+ (NSSet *)keyPathsForValuesAffectingUserId
{
	return [NSSet setWithObjects:SCIPropertyManagerKeyPathEnginePhoneUserId, nil];
}
- (NSString *)userId
{
	return self.engine.phoneUserId;
}

#pragma mark - Public API

- (NSString *)accountSpecificPropertyNameForPropertyName:(NSString *)propertyName
{
	return [NSString stringWithFormat:@"%@_%@", propertyName, self.userId];
}

- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setValue:value ofType:type
  forPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
 inStorageLocation:location];
}
- (id)valueOfType:(SCIPropertyType)type
 forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
	  withDefault:(id)defaultValue
{
	return [self valueOfType:type
			forPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
		   inStorageLocation:location
				 withDefault:defaultValue];
}
- (void)removeValueForAccountSpecificPropertyNamed:(NSString *)name
								 inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self removeValueForPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
					inStorageLocation:location];
}

- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
   preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
  postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
{
	[self setValue:value
			ofType:type
  forPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
 inStorageLocation:location
	 preStoreBlock:preStoreBlock
	postStoreBlock:postStoreBlock];
}

- (void)setStringValue:(NSString *)value
	  forAccountSpecificPropertyNamed:(NSString *)name
	 inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setStringValue:value
		forPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
	   inStorageLocation:location];
}
- (NSString *)stringValueForAccountSpecificPropertyNamed:(NSString *)name
						inStorageLocation:(SCIPropertyStorageLocation)location
							  withDefault:(NSString *)defaultValue
{
	return [self stringValueForPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
						   inStorageLocation:location
								 withDefault:defaultValue];
}

- (void)setIntValue:(int)value
   forAccountSpecificPropertyNamed:(NSString *)name
  inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setIntValue:value
	 forPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
	inStorageLocation:location];
}
- (int)intValueForAccountSpecificPropertyNamed:(NSString *)name
			  inStorageLocation:(SCIPropertyStorageLocation)location
					withDefault:(int)defaultValue
{
	return [self intValueForPropertyNamed:[self accountSpecificPropertyNameForPropertyName:name]
						inStorageLocation:location
							  withDefault:defaultValue];
}

@end

NSString * const SCIPropertyManagerKeyUserID = @"userId";


static NSString * const SCIPropertyManagerKeyPathEnginePhoneUserId = @"self.engine.phoneUserId";
static NSString * const SCIPropertyManagerKeyPathEngineRingGroupUserId = @"self.engine.ringGroupUserId";
