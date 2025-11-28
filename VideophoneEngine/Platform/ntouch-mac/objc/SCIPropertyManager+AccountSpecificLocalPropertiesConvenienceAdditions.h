//
//  SCIPropertyManager+AccountSpecificLocalPropertiesConvenienceAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"

@interface SCIPropertyManager (AccountSpecificLocalPropertiesConvenienceAdditions)

- (NSString *)propertyNameWithNameComponent:(NSString *)nameComponent
										key:(NSString *)key;
- (NSDate *)dateForAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
														key:(NSString *)key
										  inStorageLocation:(SCIPropertyStorageLocation)location;
- (void)setDate:(NSDate *)date
forAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
			key:(NSString *)key
inStorageLocation:(SCIPropertyStorageLocation)location;

- (BOOL)boolForAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
													key:(NSString *)key
									  inStorageLocation:(SCIPropertyStorageLocation)location
											withDefault:(BOOL)defaultValue;
- (void)setBool:(BOOL)value
forAccountSpecificPropertyWithNameComponent:(NSString *)nameComponent
			key:(NSString *)key
inStorageLocation:(SCIPropertyStorageLocation)location;

@end
