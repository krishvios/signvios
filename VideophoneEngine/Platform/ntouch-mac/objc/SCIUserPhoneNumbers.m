//
//  SCIUserPhoneNumbers.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/3/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserPhoneNumbers.h"

@interface SCIUserPhoneNumbers ()
{
	SCIUserPhoneNumberType mPreferredPhoneNumberType;
	NSString *mSorensonPhoneNumber;
	NSString *mTollFreeLocalNumber;
	NSString *mLocalPhoneNumber;
	NSString *mHearingPhoneNumber;
	NSString *mRingGroupLocalPhoneNumber;
	NSString *mRingGroupTollFreePhoneNumber;
}
@end

@implementation SCIUserPhoneNumbers

@synthesize preferredPhoneNumberType = mPreferredPhoneNumberType;
@synthesize sorensonPhoneNumber = mSorensonPhoneNumber;
@synthesize tollFreePhoneNumber = mTollFreeLocalNumber;
@synthesize localPhoneNumber = mLocalPhoneNumber;
@synthesize hearingPhoneNumber = mHearingPhoneNumber;
@synthesize ringGroupLocalPhoneNumber = mRingGroupLocalPhoneNumber;
@synthesize ringGroupTollFreePhoneNumber = mRingGroupTollFreePhoneNumber;

#pragma mark - lifecycle methods

- (id)init
{
	self = [super init];
	if (self) {
		mPreferredPhoneNumberType = SCIUserPhoneNumberTypeUnknown;
	}
	return self;
}

#pragma mark - public API

- (NSString *)preferredPhoneNumber
{
	return [self phoneNumberOfType:[self preferredPhoneNumberType]];
}

- (NSString *)phoneNumberOfType:(SCIUserPhoneNumberType)phoneNumberType
{
	switch (phoneNumberType) {
		case SCIUserPhoneNumberTypeUnknown: {
			return nil;
		} break;
		case SCIUserPhoneNumberTypeSorenson: {
			return [self sorensonPhoneNumber];
		} break;
		case SCIUserPhoneNumberTypeTollFree: {
			return [self tollFreePhoneNumber];
		} break;
		case SCIUserPhoneNumberTypeLocal: {
			return [self localPhoneNumber];
		} break;
		case SCIUserPhoneNumberTypeHearing: {
			return [self hearingPhoneNumber];
		} break;
		case SCIUserPhoneNumberTypeRingGroupLocal: {
			return [self ringGroupLocalPhoneNumber];
		} break;
		case SCIUserPhoneNumberTypeRingGroupTollFree: {
			return [self ringGroupTollFreePhoneNumber];
		} break;
		default: {
			return nil;
		} break;
	}
}

- (SCIUserPhoneNumberType)typeOfPhoneNumber:(NSString *)phoneNumber
{
	if ([phoneNumber isEqualToString:[self sorensonPhoneNumber]]) {
		return SCIUserPhoneNumberTypeSorenson;
	} else if ([phoneNumber isEqualToString:[self tollFreePhoneNumber]]) {
		return SCIUserPhoneNumberTypeTollFree;
	} else if ([phoneNumber isEqualToString:[self localPhoneNumber]]) {
		return SCIUserPhoneNumberTypeLocal;
	} else if ([phoneNumber isEqualToString:[self hearingPhoneNumber]]) {
		return SCIUserPhoneNumberTypeHearing;
	} else if ([phoneNumber isEqualToString:[self ringGroupLocalPhoneNumber]]) {
		return SCIUserPhoneNumberTypeRingGroupLocal;
	} else if ([phoneNumber isEqualToString:[self ringGroupTollFreePhoneNumber]]) {
		return SCIUserPhoneNumberTypeRingGroupTollFree;
	} else {
		return SCIUserPhoneNumberTypeUnknown;
	}
}

@end

NSString *NSStringFromSCIUserPhoneNumberType(SCIUserPhoneNumberType phoneNumberType)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddType( type ) \
do { \
	[mutableDictionary setObject:[NSString stringWithUTF8String:#type] forKey:@((type))]; \
} while (0)
		
		AddType(SCIUserPhoneNumberTypeUnknown);
		AddType(SCIUserPhoneNumberTypeSorenson);
		AddType(SCIUserPhoneNumberTypeTollFree);
		AddType(SCIUserPhoneNumberTypeLocal);
		AddType(SCIUserPhoneNumberTypeHearing);
		AddType(SCIUserPhoneNumberTypeRingGroupLocal);
		AddType(SCIUserPhoneNumberTypeRingGroupTollFree);
				  
#undef AddType
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary[@(phoneNumberType)];
}
