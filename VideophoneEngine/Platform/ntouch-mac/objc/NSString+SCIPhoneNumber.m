
//
//  NSString+SCIPhoneNumber.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSString+SCIPhoneNumber.h"
#import "ntouch-Swift.h"
#import "SCIVideophoneEngine.h"
#import "SCIPhoneNumberFormatter.h"

static NSString * const numericDialableNumberRegex = @"^[+]?[0-9]+$";
static NSString * const extendedNumericDialableNumberRegex = @"^[+]?[0-9]+[Xx][0-9]+$";
static NSString * const alphanumericDialableNumberRegex = @"^[+]?[0-9A-Za-z]+$";

@implementation NSString (SCIPhoneNumber)

- (NSString *)sci_dialableStringFromString
{
	NSString *res = nil;
	
	if ([[SCIVideophoneEngine sharedEngine] isNameRingGroupMember:self]) {
		res = self;
	} else  if (SCIPhoneNumberFormatterStringIsDomainDialString(self) ||
				SCIPhoneNumberFormatterStringIsIPDialString(self)) {
		res = self;
	} else {
		res = [self sci_stringByReplacingLettersWithPhoneNumberEquivalents];
		res = [SCIPhoneNumberFormatter strip:self];
		res = [self sci_dialableNumberFromPhoneNumber];
	}
	
	return res;
}

- (NSString *)sci_undecoratedNumberFromDialableNumber
{
	NSCharacterSet *decorationCharacterSet = [NSCharacterSet characterSetWithCharactersInString:@"+"];
	return [[self componentsSeparatedByCharactersInSet:decorationCharacterSet] componentsJoinedByString:@""];
}

- (NSString *)sci_stringByReplacingLettersWithPhoneNumberEquivalents
{
	NSUInteger length = self.length;
	
	NSMutableString *mutableRes = [NSMutableString stringWithCapacity:length];

	for (NSUInteger index = 0; index < length; index++)
	{
		unichar ch = [self characterAtIndex:index];
		[mutableRes appendFormat:@"%c", SCIPhoneNumberCharacterForCharacter(ch)];
	}
	
	return [mutableRes copy];
}

- (NSString *)sci_dialableNumberFromPhoneNumber
{
	NSString *phone = self;
	// 'phone' may be one of:
	// 18005551234
	// 18005551234X1234
	// 10.20.30.40
	// domain.example.com
	// 1800CONTACTS
	// We need to detect this last case and convert the letters to numbers.
	
	// If it has a period in it, it's an IP address or DNS name. Pass it through unchanged.
	if ([phone rangeOfString:@"."].location != NSNotFound)
		return phone;
	
	// All numbers? Pass it through.
	if ([phone isMatchedByRegex:numericDialableNumberRegex])
		return phone;
	
	// Extension? Pass it through.
	if ([phone isMatchedByRegex:extendedNumericDialableNumberRegex])
		return phone;
	
	// That leaves a phone number with characters that need to be decoded.
	if ([phone isMatchedByRegex:alphanumericDialableNumberRegex] && ([phone length] >= 10)) // Only convert character phone numbers if they are XXX-XXX-XXXX or larger(not including the "-").
	{
		return [self sci_stringByReplacingLettersWithPhoneNumberEquivalents];
	}
	else
	{
		SCILog(@"%s Phone number \"%@\" fell outside of the expected constraints.", __PRETTY_FUNCTION__, phone);
		return phone;
	}
}

- (BOOL)sci_phoneNumberIsDialable
{
	NSString *phone = self;
	return ([phone isMatchedByRegex:numericDialableNumberRegex] ||
			[phone isMatchedByRegex:extendedNumericDialableNumberRegex] ||
			[phone isMatchedByRegex:alphanumericDialableNumberRegex]);
}

@end

unichar SCIPhoneNumberCharacterForCharacter(unichar character)
{
	unichar res = 0;
	
	if (character >= '0' && character <= '9') {
		res = character;
	}
	else if (character == '+') {
		res = character;
	}
	else if ((character >= 'a' && character <= 'c') ||
			 (character >= 'A' && character <= 'C')) {
		res = (unichar)'2';
	}
	else if ((character >= 'd' && character <= 'f') ||
			 (character >= 'D' && character <= 'F')) {
		res = (unichar)'3';
	}
	else if ((character >= 'g' && character <= 'i') ||
			 (character >= 'G' && character <= 'I')) {
		res = (unichar)'4';
	}
	else if ((character >= 'j' && character <= 'l') ||
			 (character >= 'J' && character <= 'L')) {
		res = (unichar)'5';
	}
	else if ((character >= 'm' && character <= 'o') ||
			 (character >= 'M' && character <= 'O')) {
		res = (unichar)'6';
	}
	else if ((character >= 'p' && character <= 's') ||
			 (character >= 'P' && character <= 'S')) {
		res = (unichar)'7';
	}
	else if ((character >= 't' && character <= 'v') ||
			 (character >= 'T' && character <= 'V')) {
		res = (unichar)'8';
	}
	else if ((character >= 'w' && character <= 'z') ||
			 (character >= 'W' && character <= 'Z')) {
		res = (unichar)'9';
	}
	else {
		res = character;
	}
	
	return res;
}
