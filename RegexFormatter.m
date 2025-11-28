//
//  RegexKitLiteFormatter.m
//  RegexKitLiteFormatterTest
//
//  Created by Nate Chandler on 3/27/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import "RegexFormatter.h"
#import "ntouch-Swift.h"
#import <objc/runtime.h>

@interface NSString (RegexFormatterAdditions)
- (NSString *)largestTruncationMatchedByRegex:(NSString *)regex;
@end

@implementation NSString (RegexFormatterAdditions)
- (NSString *)largestTruncationMatchedByRegex:(NSString *)regex
{
	NSUInteger length = [self length];
	for (NSUInteger i = length; i > 0; i--) {
		NSString *truncation = [self substringToIndex:i - 1];
		if ([truncation isMatchedByRegex:regex]) {
			return truncation;
		}
	}
	return @"";
}
@end

@interface RegexFormatter ()
{
	NSString *mRegex;
	NSString *mFullStringRegex;
	NSString *mPartialStringRegex;
}
@end

@implementation RegexFormatter

@synthesize regex = mRegex;
@synthesize fullStringRegex = mFullStringRegex;
@synthesize partialStringRegex = mPartialStringRegex;

- (BOOL)isPartialStringValid:(NSString *)partialString newEditingString:(NSString *__autoreleasing *)newString errorDescription:(NSString *__autoreleasing *)error
{
	NSString *regex = self.partialStringRegex;
	if (!regex) regex = self.regex;
	
	if (regex) {
		BOOL isMatchedByRegex = [partialString isMatchedByRegex:regex];
		if (isMatchedByRegex) {
			return YES;
		} else {
			*newString = [partialString largestTruncationMatchedByRegex:regex];
			return NO;
		}
	}
	return YES;
}

- (BOOL)getObjectValue:(__autoreleasing id *)obj forString:(NSString *)string errorDescription:(NSString *__autoreleasing *)error
{
	if (obj && string)
	{
		*obj = [NSString stringWithString:string];
	}
	return YES;
}

- (NSString *)stringForObjectValue:(id)obj
{
	NSString *objectValue = (NSString *)obj;
	NSString *regex = self.fullStringRegex;
	if (!regex) regex = self.regex;
	if (regex) {
		BOOL isMatchedByRegex = [objectValue isMatchedByRegex:regex];
		if (isMatchedByRegex) {
			return objectValue;
		} else {
			return [objectValue largestTruncationMatchedByRegex:regex];
		}
	}
	return objectValue;
}
@end
