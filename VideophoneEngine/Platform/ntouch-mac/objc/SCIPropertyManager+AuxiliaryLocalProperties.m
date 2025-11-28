//
//  SCIPropertyManager+AuxiliaryLocalProperties.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+AuxiliaryLocalProperties.h"

static int const SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay;

static NSString * const SCIPropertyNameMaxTimeSinceCallCIRToDismissSuggestion;
static NSString * const SCIPropertyNameTimeBetweenCallCIRSuggestions;
static NSString * const SCIPropertyNameTimeBetweenPSMGPromotions;
static NSString * const SCIPropertyNameTimeBetweenWebDialPromotions;
static NSString * const SCIPropertyNameTimeBetweenContactPhotoPromotions;
static NSString * const SCIPropertyNameTimeBetweenSharedTextPromotions;

@implementation SCIPropertyManager (AuxiliaryLocalProperties)

- (NSTimeInterval)maximumIntervalSinceLastCIRCallToDismissCallCIRSuggestion
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameMaxTimeSinceCallCIRToDismissSuggestion
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}

- (NSTimeInterval)intervalBetweenCallCIRSuggestions
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameTimeBetweenCallCIRSuggestions
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}

- (NSTimeInterval)intervalBetweenPSMGPromotions
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameTimeBetweenPSMGPromotions
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}

- (NSTimeInterval)intervalBetweenWebDialPromotions
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameTimeBetweenWebDialPromotions
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}

- (NSTimeInterval)intervalBetweenContactPhotoPromotions
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameTimeBetweenContactPhotoPromotions
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}

- (NSTimeInterval)intervalBetweenSharedTextPromotions
{
	int intervalInt = [self intValueForPropertyNamed:SCIPropertyNameTimeBetweenSharedTextPromotions
								   inStorageLocation:SCIPropertyStorageLocationPersistent
										 withDefault:SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay];
	return (NSTimeInterval)intervalInt;
}


@end

static int const SCIPropertyManagerAuxiliaryLocalPropertiesSecondsPerDay = 60 * 60 * 24;

static NSString * const SCIPropertyNameMaxTimeSinceCallCIRToDismissSuggestion = @"MaxTimeSinceCallCIRToDismissSuggestion";
static NSString * const SCIPropertyNameTimeBetweenCallCIRSuggestions = @"TimeBetweenCallCIRSuggestions";
static NSString * const SCIPropertyNameTimeBetweenPSMGPromotions = @"TimeBetweenPSMGPromotions";
static NSString * const SCIPropertyNameTimeBetweenWebDialPromotions = @"TimeBetweenWebDialPromotions";
static NSString * const SCIPropertyNameTimeBetweenContactPhotoPromotions = @"TimeBetweenContactPhotoPromotions";
static NSString * const SCIPropertyNameTimeBetweenSharedTextPromotions = @"TimeBetweenSharedTextPromotions";

