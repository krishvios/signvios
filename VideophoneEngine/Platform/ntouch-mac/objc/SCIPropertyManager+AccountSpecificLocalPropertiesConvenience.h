//
//  SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"

@class SCIVideophoneEngine;

@interface SCIPropertyManager (AccountSpecificLocalPropertiesConvenience)

- (SCIVideophoneEngine *)engine;
- (NSString *)userId;

- (NSString *)accountSpecificPropertyNameForPropertyName:(NSString *)propertyName;


- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
   preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
  postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock;

- (void)removeValueForAccountSpecificPropertyNamed:(NSString *)name
								 inStorageLocation:(SCIPropertyStorageLocation)location;
- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location;
- (id)valueOfType:(SCIPropertyType)type
 forAccountSpecificPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
	  withDefault:(id)defaultValue;

- (void)setStringValue:(NSString *)value
forAccountSpecificPropertyNamed:(NSString *)name
	 inStorageLocation:(SCIPropertyStorageLocation)location;
- (NSString *)stringValueForAccountSpecificPropertyNamed:(NSString *)name
									   inStorageLocation:(SCIPropertyStorageLocation)location
											 withDefault:(NSString *)defaultValue;

- (void)setIntValue:(int)value
forAccountSpecificPropertyNamed:(NSString *)name
  inStorageLocation:(SCIPropertyStorageLocation)location;
- (int)intValueForAccountSpecificPropertyNamed:(NSString *)name
							 inStorageLocation:(SCIPropertyStorageLocation)location
								   withDefault:(int)defaultValue;

@end

extern NSString * const SCIPropertyManagerKeyUserID;
