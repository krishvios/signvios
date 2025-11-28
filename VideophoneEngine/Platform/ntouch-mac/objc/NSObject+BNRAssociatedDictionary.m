//
//  NSObject+BNRAssociatedDictionary.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSObject+BNRAssociatedDictionary.h"
#import <objc/runtime.h>

@interface NSObject (BNRAssociatedDictionaryInternal)
- (NSMutableDictionary *)bnr_associatedDictionary;
- (NSObject *)bnr_associatedDictionaryLock;
@end

@implementation NSObject (BNRAssociatedDictionary)

- (void)bnr_setAssociatedObject:(id)object forKey:(id<NSCopying>)key
{
	NSMutableDictionary *associatedDictionary = self.bnr_associatedDictionary;
	@synchronized(self.bnr_associatedDictionaryLock) {
		if (object) {
			associatedDictionary[key] = object;
		} else {
			[self.bnr_associatedDictionary removeObjectForKey:key];
		}
	}
}
- (id)bnr_associatedObjectForKey:(id<NSCopying>)key
{
    return self.bnr_associatedDictionary[key];
}

- (id)bnr_associatedObjectGeneratedBy:(id (^)(void))generatorBlock forKey:(id<NSCopying>)key
{
	id res = nil;
	
	NSMutableDictionary *associatedDictionary = self.bnr_associatedDictionary;
	@synchronized(self.bnr_associatedDictionaryLock) {
		if (!(res = associatedDictionary[key])) {
			res = generatorBlock();
			associatedDictionary[key] = res;
		}
	}
	
	return res;
}

@end

@implementation NSObject (BNRAssociatedDictionaryInternal)
const char bnr_associatedDictionaryLockAssociatedObjectKey;
- (NSObject *)bnr_associatedDictionaryLockPrimitive
{
	return objc_getAssociatedObject(self, &bnr_associatedDictionaryLockAssociatedObjectKey);
}
- (void)bnr_setAssociatedDictionaryLockPrimitive:(NSObject *)associatedDictionaryLock
{
	objc_setAssociatedObject(self, &bnr_associatedDictionaryLockAssociatedObjectKey, associatedDictionaryLock, OBJC_ASSOCIATION_RETAIN);
}
- (NSObject *)bnr_generateAssociatedDictionaryLock
{
	NSObject *associatedDictionaryLock = [[NSObject alloc] init];
	[self bnr_setAssociatedDictionaryLockPrimitive:associatedDictionaryLock];
	return associatedDictionaryLock;
}

- (NSObject *)bnr_associatedDictionaryLock
{
	NSObject *res = nil;
	
	if (!(res = self.bnr_associatedDictionaryLockPrimitive)) {
		@synchronized(self) {
			res = [self bnr_generateAssociatedDictionaryLock];
		}
	}
	
	return res;
}

const char bnr_associatedDictionaryAssociatedObjectKey;
- (NSMutableDictionary *)bnr_associatedDictionaryPrimitive
{
    return objc_getAssociatedObject(self, &bnr_associatedDictionaryAssociatedObjectKey);
}
- (void)bnr_setAssociatedDictionaryPrimitive:(NSMutableDictionary *)associatedDictionary
{
    objc_setAssociatedObject(self, &bnr_associatedDictionaryAssociatedObjectKey, associatedDictionary, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
- (NSMutableDictionary *)bnr_generateAssociatedDictionary
{
    NSMutableDictionary *associatedDictionary = [[NSMutableDictionary alloc] init];
    [self bnr_setAssociatedDictionaryPrimitive:associatedDictionary];
    return associatedDictionary;
}

- (NSMutableDictionary *)bnr_associatedDictionary
{
    NSMutableDictionary *res = nil;
	
    @synchronized(self.bnr_associatedDictionaryLock) {
        if (!(res = [self bnr_associatedDictionaryPrimitive])) {
            res = [self bnr_generateAssociatedDictionary];
        }
    }
	
    return res;
}
@end

