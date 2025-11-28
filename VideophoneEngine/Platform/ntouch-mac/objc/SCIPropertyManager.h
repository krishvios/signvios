//
//  SCIPropertyManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 2/22/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum SCIPropertyStorageLocation : NSUInteger {
	SCIPropertyStorageLocationPersistent,
	SCIPropertyStorageLocationTemporary,
} SCIPropertyStorageLocation;

typedef enum SCIPropertyType : NSUInteger {
	SCIPropertyTypeString,
	SCIPropertyTypeInt,
} SCIPropertyType;

typedef enum SCIPropertyScope : NSUInteger {
	SCIPropertyScopeUser,
	SCIPropertyScopePhone,
	SCIPropertyScopeUnknown = NSUIntegerMax,
} SCIPropertyScope;

@interface SCIPropertyManager : NSObject

@property (class, readonly) SCIPropertyManager *sharedManager;

- (void)saveToPersistentStorage;
- (void)sendValueForPropertyNamed:(NSString *)name
						withScope:(SCIPropertyScope)scope;
- (void)sendProperties;
- (void)waitForSendWithTimeout:(NSTimeInterval)timeout;
- (void)removeValueForPropertyNamed:(NSString *)name
				  inStorageLocation:(SCIPropertyStorageLocation)location;
- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location;
- (id)valueOfType:(SCIPropertyType)type
 forPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
	  withDefault:(id)defaultValue;

- (void)setValue:(id)value
		  ofType:(SCIPropertyType)type
forPropertyNamed:(NSString *)name
inStorageLocation:(SCIPropertyStorageLocation)location
   preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
  postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock;

- (void)setStringValue:(NSString *)value
	  forPropertyNamed:(NSString *)name
	 inStorageLocation:(SCIPropertyStorageLocation)location;
- (NSString *)stringValueForPropertyNamed:(NSString *)name
						inStorageLocation:(SCIPropertyStorageLocation)location
							  withDefault:(NSString *)defaultValue;

- (void)setIntValue:(int)value
   forPropertyNamed:(NSString *)name
  inStorageLocation:(SCIPropertyStorageLocation)location;
- (int)intValueForPropertyNamed:(NSString *)name
			  inStorageLocation:(SCIPropertyStorageLocation)location
					withDefault:(int)defaultValue;

@end

extern const int SCIPropertyManagerDefaultIntValue;
extern NSString * const SCIPropertyManagerDefaultStringValue;

#ifdef __cplusplus
extern "C" {
#endif
	id SCIDefaultValueForPropertyType(SCIPropertyType type);
#ifdef __cplusplus
}
#endif

