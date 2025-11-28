//
//  SCIPropertyManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 2/22/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"
#import "PropertyManager.h"
#import "propertydef.h"
#import "stiSVX.h"

@interface SCIPropertyManager ()
@property (nonatomic, readonly) WillowPM::PropertyManager *propertyManager;
@end

static std::string SCIPropertyManagerDefaultStringValueFromDefaultValue(id defaultValue);
static int SCIPropertyManagerDefaultIntValueFromDefaultValue(id defaultValue);
static WillowPM::PropertyManager::EStorageLocation StorageLocationFromSCIPropertyStorageLocation(SCIPropertyStorageLocation location);
static NSString *NSStringFromSCIPropertyType(SCIPropertyType type);
static Class ClassFromSCIPropertyType(SCIPropertyType type);
static EstiPropertyType EstiPropertyTypeFromSCIPropertyScope(SCIPropertyScope scope);

@implementation SCIPropertyManager

#pragma mark - Singleton

+ (SCIPropertyManager *)sharedManager
{
	static SCIPropertyManager *sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[SCIPropertyManager alloc] init];
	});
	return sharedManager;
}

#pragma mark - Private Accessors

- (WillowPM::PropertyManager *)propertyManager
{
	return WillowPM::PropertyManager::getInstance();
}

#pragma mark - Helpers

- (BOOL)_getStringValue:(NSString * __autoreleasing *)valueOut forPropertyNamed:(NSString *)name fromStorageLocation:(SCIPropertyStorageLocation)location withDefault:(NSString *)defaultValue
{
	BOOL success = NO;
	
	auto propertyName = std::string(name.UTF8String);
	auto defaultVal = SCIPropertyManagerDefaultStringValueFromDefaultValue(defaultValue);
	std::string propertyVal;
	WillowPM::PropertyManager::EStorageLocation eLocation = StorageLocationFromSCIPropertyStorageLocation(location);
	
	WillowPM::PropertyManager *propertyManager = self.propertyManager;
	int res = propertyManager->getPropertyString(propertyName, &propertyVal, defaultVal, eLocation);
	
	success = (res == 0);
	NSString *value = success ? (propertyVal.c_str() ? [NSString stringWithUTF8String:propertyVal.c_str()] : nil) : (defaultValue != nil && !defaultVal.empty() ? [NSString stringWithUTF8String:defaultVal.c_str()] : nil);
	
	if (valueOut) *valueOut = value;
	return success;
}

- (BOOL)_setStringValue:(NSString *)value forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	BOOL success = NO;
	
	auto propertyVal = std::string(value.UTF8String);
	auto propertyName = std::string(name.UTF8String);
	WillowPM::PropertyManager::EStorageLocation eLocation = StorageLocationFromSCIPropertyStorageLocation(location);
	
	WillowPM::PropertyManager *propertyManager = self.propertyManager;
	int res = propertyManager->setPropertyString(propertyName, propertyVal, eLocation);
	
	success = (res == 0);
	
	return success;
}

- (BOOL)_getIntValue:(NSNumber * __autoreleasing *)valueOut forPropertyNamed:(NSString *)name fromStorageLocation:(SCIPropertyStorageLocation)location withDefault:(NSNumber *)defaultValue
{
	BOOL success = NO;
	
	auto propertyName = std::string(name.UTF8String);
	int defaultVal = SCIPropertyManagerDefaultIntValueFromDefaultValue(defaultValue);
	int propertyVal = defaultVal;
	WillowPM::PropertyManager::EStorageLocation eLocation = StorageLocationFromSCIPropertyStorageLocation(location);
	
	WillowPM::PropertyManager *propertyManager = self.propertyManager;
	
	int res = propertyManager->getPropertyInt(propertyName, &propertyVal, defaultVal, eLocation);
	
	success = (res == 0);
	NSNumber *value = success ? [NSNumber numberWithInt:propertyVal] : [NSNumber numberWithInt:defaultVal];
	
	if (valueOut) *valueOut = value;
	return success;
}

- (BOOL)_setIntValue:(NSNumber *)value forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	BOOL success = NO;
	
	auto propertyName = std::string(name.UTF8String);
	int propertyVal = value.intValue;
	WillowPM::PropertyManager::EStorageLocation eLocation = StorageLocationFromSCIPropertyStorageLocation(location);
	
	WillowPM::PropertyManager *propertyManager = self.propertyManager;
	int res = propertyManager->setPropertyInt(propertyName, propertyVal, eLocation);
	
	success = (res == 0);
	
	return success;
}

- (BOOL)_getValue:(id __autoreleasing *)valueOut ofType:(SCIPropertyType)type forPropertyNamed:(NSString *)name fromStorageLocation:(SCIPropertyStorageLocation)location withDefault:(id)defaultValue
{
	BOOL success = NO;
	id value = nil;
	
	switch (type) {
		case SCIPropertyTypeString: {
			success = [self _getStringValue:&value forPropertyNamed:name fromStorageLocation:location withDefault:defaultValue];
		} break;
		case SCIPropertyTypeInt: {
			success = [self _getIntValue:&value forPropertyNamed:name fromStorageLocation:location withDefault:defaultValue];
		} break;
	}
	
				   
	if (valueOut) *valueOut = value;
	return success;
}

- (BOOL)_setValue:(id)value ofType:(SCIPropertyType)type forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	BOOL success = NO;
	
	switch (type) {
		case SCIPropertyTypeString: {
			success = [self _setStringValue:value forPropertyNamed:name inStorageLocation:location];
		} break;
		case SCIPropertyTypeInt: {
			success = [self _setIntValue:value forPropertyNamed:name inStorageLocation:location];
		} break;
	}
	
	return success;
}

- (BOOL)_removeValueForPropertyNamed:(NSString *)name fromStorageLocation:(SCIPropertyStorageLocation)location
{
	BOOL success = NO;
	
	const char *pPropertyName = name.UTF8String;
	bool bAll = (location == SCIPropertyStorageLocationPersistent);
	int res = self.propertyManager->removeProperty(pPropertyName, bAll);
	
	success = (res == 0);
	
	return success;
}

- (void)_sendValueForPropertyNamed:(NSString *)name withScope:(SCIPropertyScope)scope
{
	const char *pPropertyName = name.UTF8String;
	EstiPropertyType nType = EstiPropertyTypeFromSCIPropertyScope(scope);
	
	self.propertyManager->PropertySend(pPropertyName, nType);
}

#pragma mark - Public API

- (void)saveToPersistentStorage
{
	self.propertyManager->saveToPersistentStorage();
}

- (void)sendValueForPropertyNamed:(NSString *)name withScope:(SCIPropertyScope)scope
{
	NSAssert1(name != nil, @"Called %s passing nil name.", __PRETTY_FUNCTION__);
	[self _sendValueForPropertyNamed:name withScope:scope];
}

- (void)sendProperties
{
	self.propertyManager->SendProperties();
}

- (void)waitForSendWithTimeout:(NSTimeInterval)timeout
{
	self.propertyManager->PropertySendWait(timeout * 1000);
}

- (void)removeValueForPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	NSAssert1(name != nil, @"Called %s passing nil name.", __PRETTY_FUNCTION__);
	
	[self _removeValueForPropertyNamed:name fromStorageLocation:location];
}

- (void)setValue:(id)value ofType:(SCIPropertyType)type forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	NSAssert1(name != nil, @"Called %s passing nil name.", __PRETTY_FUNCTION__);
	NSAssert3([value isKindOfClass:ClassFromSCIPropertyType(type)], @"Called %s passing %@ with value of type %@.", __PRETTY_FUNCTION__, NSStringFromSCIPropertyType(type), [value class]);
	
	__unused BOOL res = NO;
	if (value) {
		res = [self _setValue:value ofType:type forPropertyNamed:name inStorageLocation:location];
	} else {
		res = [self _removeValueForPropertyNamed:name fromStorageLocation:location];
	}
	
//	NSAssert3(res, @"%s failed to set %@ for %@.", __PRETTY_FUNCTION__, value, name);
}

- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
   preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
  postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
{
	id oldValue = nil;
	id newValue = nil;
	if (preStoreBlock || postStoreBlock) {
		oldValue = [self valueOfType:type
					forPropertyNamed:name
				   inStorageLocation:location
						 withDefault:nil];
		newValue = value;
	}
	
	if (preStoreBlock)
		preStoreBlock(oldValue, newValue);
	
	[self setValue:value
			ofType:type
  forPropertyNamed:name
 inStorageLocation:location];
	
	if (postStoreBlock)
		postStoreBlock(oldValue, newValue);
}

- (id)valueOfType:(SCIPropertyType)type forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location withDefault:(id)defaultValue
{
	NSAssert1(name != nil, @"Called %s passing nil name.", __PRETTY_FUNCTION__);
	
	if (!defaultValue) defaultValue = SCIDefaultValueForPropertyType(type);
	
	id value = nil;
	__unused BOOL res = [self _getValue:&value ofType:type forPropertyNamed:name fromStorageLocation:location withDefault:defaultValue];
	
//	NSAssert2(res, @"%s failed to get the value for %@.", __PRETTY_FUNCTION__, name);
	
	return value;
}

- (void)setStringValue:(NSString *)value forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setValue:value ofType:SCIPropertyTypeString forPropertyNamed:name inStorageLocation:location];
}

- (NSString *)stringValueForPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location withDefault:(NSString *)defaultValue
{
	return [self valueOfType:SCIPropertyTypeString forPropertyNamed:name inStorageLocation:location withDefault:defaultValue];
}

- (void)setIntValue:(int)value forPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location
{
	[self setValue:@(value) ofType:SCIPropertyTypeInt forPropertyNamed:name inStorageLocation:location];
}

- (int)intValueForPropertyNamed:(NSString *)name inStorageLocation:(SCIPropertyStorageLocation)location withDefault:(int)defaultValue
{
	return [(NSNumber *)[self valueOfType:SCIPropertyTypeInt forPropertyNamed:name inStorageLocation:location withDefault:@(defaultValue)] intValue];
}

@end

static WillowPM::PropertyManager::EStorageLocation StorageLocationFromSCIPropertyStorageLocation(SCIPropertyStorageLocation location)
{
	WillowPM::PropertyManager::EStorageLocation res = WillowPM::PropertyManager::Persistent;
	
	switch (location) {
		case SCIPropertyStorageLocationPersistent: {
			res = WillowPM::PropertyManager::Persistent;
		} break;
		case SCIPropertyStorageLocationTemporary: {
			res = WillowPM::PropertyManager::Temporary;
		} break;
	}
	
	return res;
}

static NSString *NSStringFromSCIPropertyType(SCIPropertyType type)
{
	NSDictionary *typeToString = nil;

	if (!typeToString) {
		NSMutableDictionary *mutableTypeToString = [NSMutableDictionary dictionary];
		
#define ADD_TYPE( type ) [mutableTypeToString setObject:[NSString stringWithUTF8String:( #type )] forKey:@(type)]
		
		ADD_TYPE(SCIPropertyTypeString);
		ADD_TYPE(SCIPropertyTypeInt);
		
		typeToString = [mutableTypeToString copy];
	}
	
	return typeToString[@(type)];
}

static Class ClassFromSCIPropertyType(SCIPropertyType type)
{
	Class res = nil;
	
	switch (type) {
		case SCIPropertyTypeString: {
			res = [NSString class];
		} break;
		case SCIPropertyTypeInt: {
			res = [NSNumber class];
		} break;
	}
	
	return res;
}

static EstiPropertyType EstiPropertyTypeFromSCIPropertyScope(SCIPropertyScope scope)
{
	EstiPropertyType res = estiPTypeUser;
	
	switch (scope) {
		case SCIPropertyScopePhone: {
			res = estiPTypePhone;
		} break;
		case SCIPropertyScopeUser: {
			res = estiPTypeUser;
		} break;
		case SCIPropertyScopeUnknown: {
			//Default to user.  It might be better to throw an exception.
			res = estiPTypeUser;
		} break;
	}
	
	return res;
}

id SCIDefaultValueForPropertyType(SCIPropertyType type)
{
	id res = nil;
	
	switch (type) {
		case SCIPropertyTypeString: {
			res = @"";
		} break;
		case SCIPropertyTypeInt: {
			res = [NSNumber numberWithInt:0];
		} break;
	}
	
	return res;
}

static std::string SCIPropertyManagerDefaultStringValueFromDefaultValue(id defaultValue)
{
	std::string res;
	
	if (defaultValue) {
		if ((__bridge void *)defaultValue != (__bridge  void *)[NSNull null]) {
			NSString *defaultValueString = defaultValue;
			res = std::string(defaultValueString.UTF8String);
		} else {
			res = "";
		}
	} else {
		res = std::string(SCIPropertyManagerDefaultStringValue.UTF8String);
	}
	
	return res;
}

static int SCIPropertyManagerDefaultIntValueFromDefaultValue(id defaultValue)
{
	int res = 0;
	
	if (defaultValue) {
		if ((__bridge  void *)defaultValue != (__bridge  void *)[NSNull null]) {
			NSNumber *defaultValueNumber = defaultValue;
			res = defaultValueNumber.intValue;
		} else {
			res = 0;
		}
	} else {
		res = SCIPropertyManagerDefaultIntValue;
	}
	
	return res;
}

extern const int SCIPropertyManagerDefaultIntValue = WillowPM::DEFAULT_INT;
extern NSString * const SCIPropertyManagerDefaultStringValue = @(WillowPM::DEFAULT_STRING);


