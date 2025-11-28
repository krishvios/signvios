//
//  SCICoreVersion.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCICoreVersion.h"

static NSString *SCICoreVersionKeyForVersionNumberAtIndex(NSUInteger index);

@implementation SCICoreVersion

#pragma mark - Factory Methods

+ (instancetype)coreVersionFromString:(NSString *)string
{
	SCICoreVersion *res = [[self alloc] init];
	
	NSArray *components = [string componentsSeparatedByString:@"."];
	NSUInteger index = 0;
	NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
	formatter.numberStyle = NSNumberFormatterNoStyle;
	for (NSString *component in components) {
		NSNumber *versionNumber = [formatter numberFromString:component];
		NSUInteger version = [versionNumber unsignedIntegerValue];
		[res setVersionNumber:version atIndex:index];
		index++;
	}
	
	return res;
}

#pragma mark - Accessing by index

- (void)setVersionNumber:(NSUInteger)number atIndex:(NSUInteger)index
{
	NSString *key = SCICoreVersionKeyForVersionNumberAtIndex(index);
	[self setValue:@(number) forKey:key];
}

- (NSUInteger)versionNumberAtIndex:(NSUInteger)index
{
	NSString *key = SCICoreVersionKeyForVersionNumberAtIndex(index);
	return [[self valueForKey:key] unsignedIntegerValue];
}

#pragma mark - Public API:

- (NSString *)createStringRepresentation
{
	return [NSString stringWithFormat:@"%lu.%lu.%lu.%lu", (unsigned long)self.majorVersionNumber, (unsigned long)self.minorVersionNumber, (unsigned long)self.buildNumber, (unsigned long)self.revisionNumber];
}

#pragma mark - Debug

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p | %@>", NSStringFromClass(self.class), self, [self createStringRepresentation]];
}

@end


NSString * const SCICoreVersionKeyMajorVersionNumber = @"majorVersionNumber";
NSString * const SCICoreVersionKeyMinorVersionNumber = @"minorVersionNumber";
NSString * const SCICoreVersionKeyBuildNumber = @"buildNumber";
NSString * const SCICoreVersionKeyRevisionNumber = @"revisionNumber";

static NSString *SCICoreVersionKeyForVersionNumberAtIndex(NSUInteger index)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetKeyForIndex( key , index ) \
do { \
	mutableDictionary[@((index))] = key; \
} while(0)
		
		
		SetKeyForIndex(SCICoreVersionKeyMajorVersionNumber, 0);
		SetKeyForIndex(SCICoreVersionKeyMinorVersionNumber, 1);
		SetKeyForIndex(SCICoreVersionKeyBuildNumber, 2);
		SetKeyForIndex(SCICoreVersionKeyRevisionNumber, 3);
		
		
#undef SetKeyForIndex
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary[@(index)];
}
