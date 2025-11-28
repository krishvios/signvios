//
//  SCIPropertyManager+CustomTexts.m
//  ntouchMac
//
//  Created by Nate Chandler on 2/21/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+CustomTexts.h"
#import "SCIVideophoneEngine.h"
#import "SCIPropertyManager.h"
#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.h"
#import "NSArray+BNRAdditions.h"

typedef enum : NSUInteger {
	SCIPropertyManagerCustomTextTypeGreeting,
	SCIPropertyManagerCustomTextTypeInterpreter,
} SCIPropertyManagerCustomTextType;

static const NSUInteger kSCIPropertyManagerMaxGreetingTextsCount;
static const NSUInteger kSCIPropertyManagerMaxInterpreterTextsCount;
static NSString * const SCIPropertyManagerGreetingTextPropertyNamePrefix;
static NSString * const SCIPropertyManagerInterpreterTextPropertyNamePrefix;

static NSString *SCIPropertyManagerPropertyNamePrefixForCustomTextType(SCIPropertyManagerCustomTextType type);
static NSUInteger SCIPropertyManagerMaxPropertyCountForCustomTextType(SCIPropertyManagerCustomTextType type);
static NSString *SCIPropertyManagerArrayKeyForCustomTextType(SCIPropertyManagerCustomTextType type);

@implementation SCIPropertyManager(CustomTexts)

#pragma mark - Public API: Greeting Texts

+ (NSSet *)keyPathsForValuesAffectingGreetingTexts
{
	return [NSSet setWithObjects:SCIPropertyManagerKeyUserID, nil];
}
- (NSArray *)greetingTexts
{
	return [self customTextsOfType:SCIPropertyManagerCustomTextTypeGreeting];
}
- (void)setGreetingTexts:(NSArray *)greetingTexts
{
	[self setCustomTexts:greetingTexts ofType:SCIPropertyManagerCustomTextTypeGreeting];
}

+ (NSSet *)keyPathsForValuesAffectingCanAddGreetingText
{
	return [NSSet setWithObjects:SCIPropertyManagerKeyGreetingTexts, nil];
}
- (BOOL)canAddGreetingText
{
	return [self canAddCustomTextOfType:SCIPropertyManagerCustomTextTypeGreeting];
}

- (NSUInteger)maxGreetingTexts
{
	return [self maxCustomTextsOfType:SCIPropertyManagerCustomTextTypeGreeting];
}

- (void)addGreetingText:(NSString *)greeting
{
	[self addCustomText:greeting ofType:SCIPropertyManagerCustomTextTypeGreeting];
}

- (void)setGreetingText:(NSString *)greetingText atIndex:(NSUInteger)index
{
	[self setCustomText:greetingText ofType:SCIPropertyManagerCustomTextTypeGreeting atIndex:index];
}

- (void)removeGreetingText:(NSString *)greeting
{
	[self removeCustomText:greeting ofType:SCIPropertyManagerCustomTextTypeGreeting];
}

- (void)removeGreetingTextAtIndex:(NSUInteger)index
{
	[self removeCustomTextOfType:SCIPropertyManagerCustomTextTypeGreeting atIndex:index];
}

#pragma mark - Public API: Interpreter Texts

+ (NSSet *)keyPathsForValuesAffectingInterpreterTexts
{
	return [NSSet setWithObject:SCIPropertyManagerKeyUserID];
}
- (NSArray *)interpreterTexts
{
	return [self customTextsOfType:SCIPropertyManagerCustomTextTypeInterpreter];
}
- (void)setInterpreterTexts:(NSArray *)interpreterTexts
{
	[self setCustomTexts:interpreterTexts ofType:SCIPropertyManagerCustomTextTypeInterpreter];
}

- (NSUInteger)maxInterpreterTexts
{
	return [self maxCustomTextsOfType:SCIPropertyManagerCustomTextTypeInterpreter];
}

- (BOOL)canAddInterpreterTexts
{
	return [self canAddCustomTextOfType:SCIPropertyManagerCustomTextTypeInterpreter];
}

- (void)addInterpreterText:(NSString *)interpreterText
{
	[self addCustomText:interpreterText ofType:SCIPropertyManagerCustomTextTypeInterpreter];
}

- (void)setInterpreterText:(NSString *)interpreterText atIndex:(NSUInteger)index
{
	[self setCustomText:interpreterText ofType:SCIPropertyManagerCustomTextTypeInterpreter atIndex:index];
}

- (void)removeInterpreterText:(NSString *)interpreterText
{
	[self removeCustomText:interpreterText ofType:SCIPropertyManagerCustomTextTypeInterpreter];
}

- (void)removeInterpreterTextAtIndex:(NSUInteger)index
{
	[self removeCustomTextOfType:SCIPropertyManagerCustomTextTypeInterpreter atIndex:index];
}

- (NSUInteger)countOfValidInterpreterTexts {
	return [self countOfValidTextsOfType:SCIPropertyManagerCustomTextTypeInterpreter];
}

#pragma mark - Helpers: Property Names

- (NSString *)customTextPropertyNameOfType:(SCIPropertyManagerCustomTextType)type atIndex:(NSUInteger)index
{
	return [NSString stringWithFormat:@"%@_%lu", SCIPropertyManagerPropertyNamePrefixForCustomTextType(type), (unsigned long)index];
}

- (NSArray *)customTextPropertyNamesOfType:(SCIPropertyManagerCustomTextType)type
{
	NSUInteger max = SCIPropertyManagerMaxPropertyCountForCustomTextType(type);
	NSMutableArray *mutableRes = [NSMutableArray arrayWithCapacity:max];
	
	for (NSUInteger i = 0; i < max; i++) {
		[mutableRes addObject:[self customTextPropertyNameOfType:type atIndex:i]];
	}
	
	return [mutableRes copy];
}

#pragma mark - Helpers: Array Data

- (NSUInteger)countOfValidTextsOfType:(SCIPropertyManagerCustomTextType)type {
	NSUInteger count = 0;
	NSArray *texts = [self customTextsOfType:type];
	for (NSString *text in texts)
	{
		if (text.length)
			count++;
	}
	return count;
}

- (BOOL)canAddCustomTextOfType:(SCIPropertyManagerCustomTextType)type
{
	NSString *key = SCIPropertyManagerArrayKeyForCustomTextType(type);
	NSArray *texts = [self valueForKey:key];
	NSUInteger count = texts.count;
	NSUInteger max = SCIPropertyManagerMaxPropertyCountForCustomTextType(type);
	return (count < max);
}

- (NSUInteger)maxCustomTextsOfType:(SCIPropertyManagerCustomTextType)type
{
	return SCIPropertyManagerMaxPropertyCountForCustomTextType(type);
}

#pragma mark - Helpers: Array Operations

- (NSArray *)customTextsOfType:(SCIPropertyManagerCustomTextType)type
{
	NSMutableArray *mutableRes = [NSMutableArray array];
	
	NSArray *propertyNames = [self customTextPropertyNamesOfType:type];
	for (NSString *propertyName in propertyNames) {
		NSString *property = [self stringValueForAccountSpecificPropertyNamed:propertyName
															inStorageLocation:SCIPropertyStorageLocationPersistent
																  withDefault:(NSString *)[NSNull null]];
		if (property) [mutableRes addObject:property];
	}
	
	return [mutableRes copy];
}

- (void)setCustomTexts:(NSArray *)customTexts ofType:(SCIPropertyManagerCustomTextType)type
{
	NSUInteger max = MIN(customTexts.count, SCIPropertyManagerMaxPropertyCountForCustomTextType(type));
	for (NSUInteger i = 0; i < max; i++) {
		[self _setCustomText:customTexts[i] ofType:type atIndex:i];
	}
	for (NSUInteger i = max; i < SCIPropertyManagerMaxPropertyCountForCustomTextType(type); i++) {
		[self _removeCustomTextOfType:type atIndex:i];
	}
}

- (void)addCustomText:(NSString *)text ofType:(SCIPropertyManagerCustomTextType)type
{
	NSString *key = SCIPropertyManagerArrayKeyForCustomTextType(type);
	NSArray *texts = [self valueForKey:key];
	NSArray *modifiedTexts = [texts arrayByAddingObject:text];
	[self setValue:modifiedTexts forKey:key];
}

- (void)setCustomText:(NSString *)text ofType:(SCIPropertyManagerCustomTextType)type atIndex:(NSUInteger)index
{
	[self changeCustomTextOfType:type withBlock:^{
		[self _setCustomText:text ofType:type atIndex:index];
	}];
}

- (void)removeCustomText:(NSString *)text ofType:(SCIPropertyManagerCustomTextType)type
{
	NSString *key = SCIPropertyManagerArrayKeyForCustomTextType(type);
	NSArray *texts = [self valueForKey:key];
	NSArray *modifiedTexts = [texts bnr_arrayByRemovingFirstInstanceOfObject:text];
	[self setValue:modifiedTexts forKey:key];
}

- (void)removeCustomTextOfType:(SCIPropertyManagerCustomTextType)type atIndex:(NSUInteger)index
{
	NSString *key = SCIPropertyManagerArrayKeyForCustomTextType(type);
	NSArray *texts = [self valueForKey:key];
	NSArray *modifiedTexts = [texts bnr_arrayByRemovingObjectAtIndex:index];
	[self setValue:modifiedTexts forKey:key];
}

- (void)changeCustomTextOfType:(SCIPropertyManagerCustomTextType)type withBlock:(void (^)(void))block
{
	NSString *key = SCIPropertyManagerArrayKeyForCustomTextType(type);
	[self willChangeValueForKey:key];
	block();
	[self didChangeValueForKey:key];
}

#pragma mark - Helpers

- (void)_setCustomText:(NSString *)greeting ofType:(SCIPropertyManagerCustomTextType)type atIndex:(NSUInteger)index
{
	[self setStringValue:greeting
forAccountSpecificPropertyNamed:[self customTextPropertyNameOfType:type atIndex:index]
	   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self saveToPersistentStorage];
}

- (void)_removeCustomTextOfType:(SCIPropertyManagerCustomTextType)type atIndex:(NSUInteger)index
{
	[self removeValueForAccountSpecificPropertyNamed:[self customTextPropertyNameOfType:type atIndex:index]
								   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self saveToPersistentStorage];
}

@end

static const NSUInteger kSCIPropertyManagerMaxGreetingTextsCount = 5;
static const NSUInteger kSCIPropertyManagerMaxInterpreterTextsCount = 10;

static NSString * const SCIPropertyManagerGreetingTextPropertyNamePrefix = @"GreetingText";
static NSString * const SCIPropertyManagerInterpreterTextPropertyNamePrefix = @"InterpreterText";

NSString * const SCIPropertyManagerKeyGreetingTexts = @"greetingTexts";
NSString * const SCIPropertyManagerKeyInterpreterTexts = @"interpreterTexts";

static NSString *SCIPropertyManagerPropertyNamePrefixForCustomTextType(SCIPropertyManagerCustomTextType type)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddPropertyNamePrefixForType( prefix , type) [mutableDictionary setObject:prefix forKey:@((type))]
		AddPropertyNamePrefixForType(SCIPropertyManagerGreetingTextPropertyNamePrefix, SCIPropertyManagerCustomTextTypeGreeting);
		AddPropertyNamePrefixForType(SCIPropertyManagerInterpreterTextPropertyNamePrefix, SCIPropertyManagerCustomTextTypeInterpreter);
#undef AddPropertyNamePrefixForType
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary[@(type)];
}

static NSUInteger SCIPropertyManagerMaxPropertyCountForCustomTextType(SCIPropertyManagerCustomTextType type)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddMaxCountForType( count, type ) [mutableDictionary setObject:@((count)) forKey:@((type))]
		AddMaxCountForType(kSCIPropertyManagerMaxGreetingTextsCount, SCIPropertyManagerCustomTextTypeGreeting);
		AddMaxCountForType(kSCIPropertyManagerMaxInterpreterTextsCount, SCIPropertyManagerCustomTextTypeInterpreter);
#undef AddMaxCountForType
		
		dictionary = [mutableDictionary copy];
	});
	
	NSNumber *number = dictionary[@(type)];
	return [number unsignedIntegerValue];
}

static NSString *SCIPropertyManagerArrayKeyForCustomTextType(SCIPropertyManagerCustomTextType type)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddKeyForType( key, type ) [mutableDictionary setObject:key forKey:@((type))]
		AddKeyForType(SCIPropertyManagerKeyGreetingTexts, SCIPropertyManagerCustomTextTypeGreeting);
		AddKeyForType(SCIPropertyManagerKeyInterpreterTexts, SCIPropertyManagerCustomTextTypeInterpreter);
#undef AddKeyForType
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary[@(type)];
}













