//
//  SCIProfileImagePrivacyLevel.h
//  ntouch
//
//  Created by Kevin Selman on 4/29/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCIProfileImagePrivacy.h"

typedef enum : NSUInteger
{
	SCIProfileImagePrivacyLevelPublic = 0,
	SCIProfileImagePrivacyLevelContactsOnly = 1,
	
} SCIProfileImagePrivacyLevel;

@interface SCIProfileImagePrivacy : NSObject

+ (NSString *)NSStringFromSCIProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)state;
+ (NSString *)ShortNSStringFromSCIProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)state;
+ (NSArray *)arrayOfProfileImagePrivacyLevels;

@end