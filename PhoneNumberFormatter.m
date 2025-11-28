//
//  PhoneNumberFormatter.m
//  ntouch
//
//  Created by Unknown on 5/20/10.
//  Copyright 2010 Sorenson Communications. All rights reserved.
//

#import "PhoneNumberFormatter.h"

@implementation PhoneNumberFormatter

- (id)init {
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
	
	predefinedFormats = [[NSDictionary alloc] initWithObjectsAndKeys:
						 usPhoneFormats, @"us",
						 ukPhoneFormats, @"uk",
						 jpPhoneFormats, @"jp",
						 nil];
	return self;
}

- (NSString *)format:(NSString *)phoneNumber withLocale:(NSString *)locale {
	NSArray *localeFormats = [predefinedFormats objectForKey:locale];
	if (localeFormats == nil) return phoneNumber;
	NSString *input = [self strip:phoneNumber];
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
					if (next < '0' || next > '9') {
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
			res = temp;
			break;
		} else {
		}
	}
	if ([res length] == 0) {
		return input;
	}
	return res;
}

- (NSString *)strip:(NSString *)phoneNumber {
	NSMutableString *res = [[NSMutableString alloc] init];
	for (int i = 0; i < [phoneNumber length]; i++) {
		char next = [phoneNumber characterAtIndex:i];
		if ([self canBeInputByPhonePad:next])
			[res appendFormat:@"%c", next];
	}
	return res;
}

- (BOOL)canBeInputByPhonePad:(char)c {
	if (c == '+' || c == '*' || c == '#') return YES;
	if (c >= '0' && c <= '9') return YES;
	return NO;
}


@end