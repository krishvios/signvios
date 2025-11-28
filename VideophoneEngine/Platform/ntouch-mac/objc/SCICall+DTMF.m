//
//  SCICall+DTMF.m
//  ntouchMac
//
//  Created by Nate Chandler on 12/20/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICall+DTMF.h"

static SCIDTMFCode SCIDTMFCodeForUnichar(unichar character);

NSArray *SCIDTMFCodesForNSString(NSString *characters){
	NSMutableArray *mutableRes = [NSMutableArray array];
	for (NSUInteger i = 0; i < characters.length; i++) {
		unichar character = [characters characterAtIndex:i];
		SCIDTMFCode code = SCIDTMFCodeForUnichar(character);
		if (code != SCIDTMFCodeNone) {
			[mutableRes addObject:@(code)];
		}
	}
	return [mutableRes copy];
}

static NSDictionary *sSCIDTMFCodeToDisplayableNSStringDictionary = nil;
NSString *DisplayableNSStringForSCIDTMFCode(SCIDTMFCode code)
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableSCIDTMFCodeToDisplayableNSStringDictionary = [NSMutableDictionary dictionary];
		
		#define AddSCIDTMFCodeAndDisplayableNSStringToDictionary( sound , string ) \
		do { \
			[mutableSCIDTMFCodeToDisplayableNSStringDictionary setObject:string forKey:@((sound))]; \
		} while(0)
		
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeAsterisk, @"*");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeZero, @"0");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodePound, @"#");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeSeven, @"7");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeEight, @"8");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeNine, @"9");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeFour, @"4");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeFive, @"5");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeSix, @"6");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeOne, @"1");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeTwo, @"2");
		AddSCIDTMFCodeAndDisplayableNSStringToDictionary(SCIDTMFCodeThree, @"3");
		
		sSCIDTMFCodeToDisplayableNSStringDictionary = [mutableSCIDTMFCodeToDisplayableNSStringDictionary copy];
	});
	return sSCIDTMFCodeToDisplayableNSStringDictionary[@(code)];
}


static NSDictionary *sUnicharToSCIDTMFCodeDictionary = nil;
static SCIDTMFCode SCIDTMFCodeForUnichar(unichar character)
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableUnicharToSCIDTMFCodeDictionary = [NSMutableDictionary dictionary];
		
		#define AddUnicharAndSCIDTMFCodeToDictionary( character , sound ) \
		do { \
			[mutableUnicharToSCIDTMFCodeDictionary setObject:@((sound)) forKey:@((character))]; \
		} while(0)
		
		AddUnicharAndSCIDTMFCodeToDictionary(48, SCIDTMFCodeZero);
		AddUnicharAndSCIDTMFCodeToDictionary(49, SCIDTMFCodeOne);
		AddUnicharAndSCIDTMFCodeToDictionary(50, SCIDTMFCodeTwo);
		AddUnicharAndSCIDTMFCodeToDictionary(51, SCIDTMFCodeThree);
		AddUnicharAndSCIDTMFCodeToDictionary(52, SCIDTMFCodeFour);
		AddUnicharAndSCIDTMFCodeToDictionary(53, SCIDTMFCodeFive);
		AddUnicharAndSCIDTMFCodeToDictionary(54, SCIDTMFCodeSix);
		AddUnicharAndSCIDTMFCodeToDictionary(55, SCIDTMFCodeSeven);
		AddUnicharAndSCIDTMFCodeToDictionary(56, SCIDTMFCodeEight);
		AddUnicharAndSCIDTMFCodeToDictionary(57, SCIDTMFCodeNine);
		AddUnicharAndSCIDTMFCodeToDictionary(42, SCIDTMFCodeAsterisk);
		AddUnicharAndSCIDTMFCodeToDictionary(35, SCIDTMFCodePound);
		
		sUnicharToSCIDTMFCodeDictionary = [mutableUnicharToSCIDTMFCodeDictionary copy];
	});
	SCIDTMFCode res = SCIDTMFCodeNone;
	NSNumber *resNumber = sUnicharToSCIDTMFCodeDictionary[@(character)];
	if (resNumber) {
		res = (SCIDTMFCode)[resNumber integerValue];
	}
	return res;
}
