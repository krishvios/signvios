//
//  PhoneNumberFormatter.m
//  ntouchMac
//
//  Created by Adam Preble on 2/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIPhoneNumberFormatter.h"

@interface NSString (PhoneNumberFormatter)
- (NSUInteger)numberOfCharactersInCharacterSet:(NSCharacterSet *)characterSet inRange:(NSRange)range;
- (NSUInteger)numberOfCharactersInCharacterSet:(NSCharacterSet *)characterSet priorToLocation:(NSUInteger)index;
- (NSUInteger)locationAfterCharacters:(NSUInteger)numberOfCharacters inCharacterSet:(NSCharacterSet *)characterSet;
- (NSUInteger)locationAntecedantToCharacterAtIndex:(NSUInteger)index;
- (NSUInteger)locationSubsequentToCharacterAtIndex:(NSUInteger)index;
- (NSUInteger)indexOfCharacterSubsequentToLocation:(NSUInteger)location;
- (NSUInteger)indexOfCharacterAntecedantToLocation:(NSUInteger)location;
- (NSString *)stringByDeletingCharacterAtIndex:(NSUInteger)index;
- (NSString *)stringByDeletingLastCharacter;
@end

@implementation NSString (PhoneNumberFormatter)
- (NSUInteger)numberOfCharactersInCharacterSet:(NSCharacterSet *)characterSet inRange:(NSRange)range
{
	NSUInteger number = 0;
	NSString *substring = [self substringWithRange:range];
	for (NSUInteger index = 0; index < [substring length]; index++) {
		number += [characterSet characterIsMember:[substring characterAtIndex:index]];
	}
	return number;
}

- (NSUInteger)numberOfCharactersInCharacterSet:(NSCharacterSet *)characterSet priorToLocation:(NSUInteger)location
{
	return [self numberOfCharactersInCharacterSet:characterSet inRange:NSMakeRange(0, location)];
}

- (NSUInteger)locationAfterCharacters:(NSUInteger)numberOfCharacters inCharacterSet:(NSCharacterSet *)characterSet
{
	if (numberOfCharacters == 0) return 0;
	NSUInteger location = 0;
	for (location = 0; location <= [self length]; location++) {
		NSUInteger numberPrior = [self numberOfCharactersInCharacterSet:characterSet priorToLocation:location];
		if (numberPrior == numberOfCharacters) return location;
	}
	return NSNotFound;
}

- (NSUInteger)locationAntecedantToCharacterAtIndex:(NSUInteger)index
{
	if (/*index < 0 || */index >= [self length]) return NSNotFound;
	return index;
}

- (NSUInteger)locationSubsequentToCharacterAtIndex:(NSUInteger)index
{
	if (/*index < 0 || */index >= [self length]) return NSNotFound;
	return index + 1;
}

- (NSUInteger)indexOfCharacterSubsequentToLocation:(NSUInteger)location
{
	if (/*location < 0 || */location >= [self length]) return NSNotFound;
	return location;
}

- (NSUInteger)indexOfCharacterAntecedantToLocation:(NSUInteger)location
{
	if (/*location <= 0 || */location > [self length]) return NSNotFound;
	return location - 1;
}

- (NSString *)stringByDeletingCharacterAtIndex:(NSUInteger)index
{
	NSString *firstHalf, *secondHalf;
	firstHalf = (index > 0) ? [self substringToIndex:index] : @"";
	secondHalf = (index < [self length]) ? [self substringFromIndex:index + 1] : @"";
	return [NSString stringWithFormat:@"%@%@", firstHalf, secondHalf];
}

- (NSString *)stringByDeletingLastCharacter
{
	return [self stringByDeletingCharacterAtIndex:[self length] - 1];
}
@end

@interface SCIPhoneNumberFormatter ()
+ (NSString *)format:(NSString *)phoneNumber withLocale:(NSString *)locale;
+ (NSCharacterSet *)phonePadCharacterSet;
+ (NSString *)strip:(NSString *)phoneNumber;
+ (BOOL)shouldFormatString:(NSString *)string;
@end

@implementation SCIPhoneNumberFormatter

- (NSString *)stringForObjectValue:(id)obj
{
	NSString *languageCode = [[[NSLocale currentLocale] objectForKey:NSLocaleCountryCode] lowercaseString];
	return [self.class format:obj withLocale:languageCode];
}

- (BOOL)getObjectValue:(__autoreleasing id *)obj forString:(NSString *)string errorDescription:(NSString *__autoreleasing *)error
{
	NSString *result = nil;
	if ([self.class shouldFormatString:string]) {
		result = [self.class strip:string];
	} else {
		result = [NSString stringWithString:string];
	}
	*obj = result;
	return result != nil;
}

- (BOOL)isPartialStringValid:(NSString **)partialStringPtr proposedSelectedRange:(NSRangePointer)proposedSelRangePtr originalString:(NSString *)origString originalSelectedRange:(NSRange)origSelRange errorDescription:(NSString **)error
{
	NSString *returnString;
	NSRange returnRange;
	
	if (![self.class shouldFormatString:*partialStringPtr])
	{
		return YES;
	}
	
	NSCharacterSet *phonePadCharacterSet = [self.class phonePadCharacterSet];
	if (origSelRange.length == 1 && [*partialStringPtr length] < [origString length]) {
		//a [single-character] deletion has occurred.  we will do this deletion ourselves.
		
		//establish the context and the location within it at which the deletion was to take place
		NSString *originalString = [origString uppercaseString];
		NSRange originalRange = origSelRange;
		NSUInteger locationInOriginalString = originalRange.location + 1;
		
		//establish the mapped-to location in the stripped string where the deletion will take place
		NSUInteger locationInStrippedOriginalString = [originalString numberOfCharactersInCharacterSet:phonePadCharacterSet priorToLocation:locationInOriginalString];
		NSString *strippedOriginalString = [self.class strip:originalString];
		
		//perform the deletion in the stripped string
		NSString *strippedReturnString = [strippedOriginalString stringByDeletingCharacterAtIndex:[strippedOriginalString indexOfCharacterAntecedantToLocation:locationInStrippedOriginalString]];
		NSUInteger locationInStrippedReturnString = locationInStrippedOriginalString - 1;
		NSUInteger numberOfPriorCharactersInStrippedReturnString = locationInStrippedReturnString;
		
		//format the string to create the returnString; create the returnRange
		returnString = [self.class stringForPhoneNumber:strippedReturnString];
		NSUInteger locationInReturnString = [returnString locationAfterCharacters:numberOfPriorCharactersInStrippedReturnString inCharacterSet:phonePadCharacterSet];
		returnRange = NSMakeRange(locationInReturnString, 0);
	} else {
		//establish the context and the cursor location within it
		NSString *string = [*partialStringPtr uppercaseString];
		NSRange originalRange = *proposedSelRangePtr;
		NSUInteger locationInString = originalRange.location;
		
		//establish the stripped string and mapped-to location within it
		NSUInteger locationInStrippedString = [string numberOfCharactersInCharacterSet:phonePadCharacterSet priorToLocation:locationInString];
		NSUInteger numberOfPriorCharactersInStrippedString = locationInStrippedString;
		NSString *strippedString = [self.class strip:string];
		
		//formate the string to create the returnString; create the returnRange
		BOOL bailed;
		NSString *formattedStringAttempt = [[self.class stringForPhoneNumber:[strippedString stringByAppendingString:@"1"] bailedOut:&bailed] stringByDeletingLastCharacter];
		if (!bailed) {
			returnString = formattedStringAttempt;
		} else {
			returnString = [self.class stringForPhoneNumber:strippedString];
		}
		if (locationInString != [string length]) {
			NSUInteger locationInReturnString = [returnString locationAfterCharacters:numberOfPriorCharactersInStrippedString inCharacterSet:phonePadCharacterSet];
			returnRange = NSMakeRange(locationInReturnString, 0);
		} else {
			NSUInteger locationInReturnString = [returnString length];
			returnRange = NSMakeRange(locationInReturnString, 0);
		}
	}
		 
	*partialStringPtr = returnString;
	(*proposedSelRangePtr) = returnRange;
	return NO;
}

+ (NSString *)stringForPhoneNumber:(NSString *)phonenumber
{
	return [self stringForPhoneNumber:phonenumber bailedOut:nil];
}

+ (NSString *)stringForPhoneNumber:(NSString *)phonenumber bailedOut:(BOOL *)bailed
{
	NSString *languageCode = [[[NSLocale currentLocale] objectForKey:NSLocaleCountryCode] lowercaseString];
	return [self format:phonenumber withLocale:languageCode bailedOut:bailed];
}

+ (NSString *)phoneNumberForString:(NSString *)string
{
	NSString *res = nil;
	if ([self shouldFormatString:string]) {
		res = [self strip:string];
	} else {
		res = string;
	}
	return res;
}

// Methods adapted from ntouch iOS / PhoneNumberFormatter (not an NSFormatter).

+ (NSDictionary *)predefinedFormats
{
	static NSDictionary *formats = nil;
	if (!formats)
	{
		NSArray *usPhoneFormats = [[NSArray alloc] initWithObjects:
								   @"+1 (###) ###-####",
								   @"1 (###) ###-####",
								   @"011 $",
								   @"###-####",
								   @"(###) ###-####", nil];
		
		NSArray *ukPhoneFormats = [[NSArray alloc] initWithObjects:
								   @"+44 ##########",
								   @"00 $",
								   @"0### - ### ####",
								   @"0## - #### ####",
								   @"0#### - ######", nil];
		
		NSArray *jpPhoneFormats = [[NSArray alloc] initWithObjects:
								   @"+81 ############",
								   @"001 $",
								   @"(0#) #######",
								   @"(0#) #### ####", nil];
	
		formats = [[NSDictionary alloc] initWithObjectsAndKeys:
							usPhoneFormats, @"us",
							ukPhoneFormats, @"uk",
							jpPhoneFormats, @"jp",
							nil];
	}
	return formats;
}

+ (NSCharacterSet *)phonePadCharacterSet
{
	static NSCharacterSet *charSet = nil;
	if (!charSet)
	{
		//DISCUSSION:	Is there any good reason for the "+" character to be in this character set?  (We don't support
		//				international phone numbers.)
		//				Per 13874, "+" are not to be included in dial strings imported from the address book.
		charSet = [NSCharacterSet characterSetWithCharactersInString:@"1234567890+*#.ABCDEFGHIJKLMNOPQRSTUVWXYZ"];
	}
	return charSet;
}

+ (BOOL)canBeInputByPhonePad:(char)c
{
	return [[self phonePadCharacterSet] characterIsMember:c];
}

+ (NSString *)strip:(NSString *)phoneNumber {
	NSMutableString *res = [[NSMutableString alloc] init];
	for (int i = 0; i < [phoneNumber length]; i++) {
		char next = [phoneNumber characterAtIndex:i];
		if ([self canBeInputByPhonePad:next])
			[res appendFormat:@"%c", next];
	}
	return [res copy];
}

+ (BOOL)shouldFormatString:(NSString *)string
{
	BOOL ret = YES;
	if (SCIPhoneNumberFormatterStringIsDomainDialString(string)) ret = NO;
	if (SCIPhoneNumberFormatterStringIsIPDialString(string)) ret = NO;
	if ([string numberOfCharactersInCharacterSet:[NSCharacterSet decimalDigitCharacterSet] inRange:NSMakeRange(0, [string length])] <= 3) ret = NO;
	return ret;
}

+ (NSString *)format:(NSString *)phoneNumber withLocale:(NSString *)locale
{
	return [self format:phoneNumber withLocale:locale bailedOut:nil];
}

+ (NSString *)format:(NSString *)phoneNumber withLocale:(NSString *)locale bailedOut:(BOOL *)bailed
{
	if (bailed) *bailed = NO;
	NSArray *localeFormats = [[self predefinedFormats] objectForKey:locale];
	if (localeFormats == nil) return phoneNumber;
	
	if (![self shouldFormatString:phoneNumber]) return phoneNumber;

	NSString *uppercase = [phoneNumber uppercaseString];
	NSString *input = [self strip:uppercase];
	NSString *res = nil;
	for (NSString *phoneFormat in localeFormats) {
		int i = 0;
		NSMutableString *temp = [[NSMutableString alloc] init];
		for (int p = 0; temp != nil && i < [input length] && p < [phoneFormat length]; p++) {
			char c = [phoneFormat characterAtIndex:p];
			BOOL required = [self canBeInputByPhonePad:c];
			char next = [input characterAtIndex:i];
			switch(c) {
				case '$':
					p--;
					[temp appendFormat:@"%c", next]; i++;
					break;
				case '#':
					if (!(next >= '0' && next <= '9') && !(next >= 'A' && next <= 'Z'))  {
						temp = nil;
						break;
					}
					[temp appendFormat:@"%c", next]; i++;
					break;
				default:
					if (required) {
						if (next != c) {
							temp = nil;
							break;
						}
						[temp appendFormat:@"%c", next]; i++;
					} else {
						[temp appendFormat:@"%c", c];
						if (next == c) i++;
					}
					break;
			}
		}
		if (i == [input length]) {
			res = [temp copy];
			break;
		} else {
		}
	}
	if ([res length] == 0) {
		if (bailed) *bailed = YES;
		return input;
	}
	return res;
}

@end

BOOL SCIPhoneNumberFormatterStringIsDomainDialString(NSString *string)
{
	BOOL res = NO;
	
	//TODO: Improve check.
	NSCharacterSet *colonSet = [NSCharacterSet characterSetWithCharactersInString:@":"];
	if ([string rangeOfCharacterFromSet:colonSet].location != NSNotFound) {
		res = YES;
	} else {
		res = NO;
	}
	
	return res;
}

BOOL SCIPhoneNumberFormatterStringIsIPDialString(NSString *string)
{
	BOOL res = NO;
	
	//TODO: Improve check.
	NSCharacterSet *dotSet = [NSCharacterSet characterSetWithCharactersInString:@"."];
	if ([string rangeOfCharacterFromSet:dotSet].location != NSNotFound) {
		res = YES;
	} else {
		res = NO;
	}
	
	return res;
}
