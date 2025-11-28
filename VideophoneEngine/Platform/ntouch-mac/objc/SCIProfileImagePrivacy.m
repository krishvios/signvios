//
//  SCIProfileImagePrivacyLevel.m
//  ntouch
//
//  Created by Kevin Selman on 4/30/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCIProfileImagePrivacy.h"

@implementation SCIProfileImagePrivacy

+ (NSString *)NSStringFromSCIProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)state
{
	switch (state) {
		case SCIProfileImagePrivacyLevelPublic: return @"Share with everyone";
		case SCIProfileImagePrivacyLevelContactsOnly: return @"Share only with contacts in my Phonebook";
		default: return @"Not Set";
	}
}

+ (NSString *)ShortNSStringFromSCIProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)state
{
	switch (state) {
		case SCIProfileImagePrivacyLevelPublic: return @"Everyone";
		case SCIProfileImagePrivacyLevelContactsOnly: return @"Contacts";
		default: return @"Not Set";
	}
}

+ (NSArray *)arrayOfProfileImagePrivacyLevels
{
	return @[[SCIProfileImagePrivacy NSStringFromSCIProfileImagePrivacyLevel:SCIProfileImagePrivacyLevelPublic],
			 [SCIProfileImagePrivacy NSStringFromSCIProfileImagePrivacyLevel:SCIProfileImagePrivacyLevelContactsOnly]];
}

@end

