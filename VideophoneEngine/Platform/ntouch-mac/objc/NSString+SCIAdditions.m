//
//  NSString+SCIAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 9/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSString+SCIAdditions.h"

@implementation NSString (SCIAdditions)

#if !defined(BNRCompare)
#define BNRCompare(A,B)	({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? NSOrderedAscending : (__a > __b ? NSOrderedDescending : NSOrderedSame); })
#endif

- (NSComparisonResult)sci_compareAlphaFirst:(NSString *)other
{
	// Compare self and other such that A comes before 0 or any other symbol.
	// Case insensitive.
	NSString *selfUpper = [self uppercaseString];
	NSString *otherUpper = [other uppercaseString];
	NSCharacterSet *letters = [NSCharacterSet letterCharacterSet];
	for (NSUInteger i = 0; i < MIN(self.length, other.length); i++)
	{
		unichar selfChar = [selfUpper characterAtIndex:i];
		unichar otherChar = [otherUpper characterAtIndex:i];
		BOOL selfIsLetter = [letters characterIsMember:selfChar];
		BOOL otherIsLetter = [letters characterIsMember:otherChar];
		if (selfIsLetter == otherIsLetter)
		{
			NSComparisonResult comparison = BNRCompare(selfChar, otherChar);
			if (comparison != NSOrderedSame) {
				return comparison;
			} else {
				continue;
			}
		}
		if (selfIsLetter)
			return NSOrderedAscending;
		else
			return NSOrderedDescending;
	}
	return BNRCompare(self.length, other.length);
}

- (NSArray *)sci_componentsOfLengthAtMost:(NSUInteger)maxLength
{
	NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
	
	NSString *unprocessedString = self;
	while (unprocessedString.length > 0) {
		NSString *component = nil;
		if (maxLength > unprocessedString.length) {
			component = unprocessedString;
			unprocessedString = nil;
		} else {
			component = [unprocessedString substringToIndex:maxLength];
			unprocessedString = [unprocessedString substringFromIndex:maxLength];
		}
		[mutableRes addObject:component];
	}
	
	return [mutableRes copy];
}


+ (instancetype)sci_stringWithWideCharacter:(unichar)character ofLength:(NSUInteger)length
{
	NSMutableString *mutableRes = [[NSMutableString alloc] init];
	
	for (NSUInteger index = 0; index < length; index++) {
		[mutableRes appendFormat:@"%C", character];
	}
	
	return [mutableRes copy];
}

+ (instancetype)sci_stringWithCharacter:(unichar)character ofLength:(NSUInteger)length
{
	NSMutableString *mutableRes = [[NSMutableString alloc] init];
	
	for (NSUInteger index = 0; index < length; index++) {
		[mutableRes appendFormat:@"%c", character];
	}
	
	return [mutableRes copy];
}

@end
