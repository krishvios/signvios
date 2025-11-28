//
//  SCIPropertyManager+AuxiliaryAccountSpecificLocalProperties.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager+AuxiliaryAccountSpecificLocalProperties.h"
#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenience.h"
#import "SCIPropertyManager+AccountSpecificLocalPropertiesConvenienceAdditions.h"

static NSString * const SCIPropertyNameComponentLastReceivedTime;
static NSString * const SCIPropertyNameComponentLastRemindLaterTime;
static NSString * const SCIPropertyNameComponentMayAlertAgain;

static NSString * const SCIPropertyNameSuffixEventShowCallCIRSuggestion;
static NSString * const SCIPropertyNameSuffixEventShowPSMGPromotion;
static NSString * const SCIPropertyNameSuffixEventShowWebDialPromotion;
static NSString * const SCIPropertyNameSuffixEventShowContactPhotosPromotion;
static NSString * const SCIPropertyNameSuffixEventShowSharedTextPromotion;

@implementation SCIPropertyManager (AuxiliaryAccountSpecificLocalProperties)

#pragma mark - Generic

- (NSDate *)lastStartTimeForEventNamed:(NSString *)eventName
{
	return [self dateForAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentLastReceivedTime
															 key:eventName
											   inStorageLocation:SCIPropertyStorageLocationPersistent];
}
- (void)setLastStartTime:(NSDate *)receivedTime forEventNamed:(NSString *)eventName
{
	[self setDate:receivedTime
forAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentLastReceivedTime
			  key:eventName
	 inStorageLocation:SCIPropertyStorageLocationPersistent];
}

- (NSDate *)lastRemindLaterTimeForEventNamed:(NSString *)eventName
{
	return [self dateForAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentLastRemindLaterTime
															 key:eventName
											   inStorageLocation:SCIPropertyStorageLocationPersistent];
}
- (void)setLastRemindLaterTime:(NSDate *)remindTime forEventNamed:(NSString *)eventName
{
	[self setDate:remindTime
forAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentLastRemindLaterTime
			  key:eventName
inStorageLocation:SCIPropertyStorageLocationPersistent];
}

- (BOOL)mayAlertAgainForEventNamed:(NSString *)eventName
{
	return [self boolForAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentMayAlertAgain
															 key:eventName
											   inStorageLocation:SCIPropertyStorageLocationPersistent
													 withDefault:YES];
}
- (void)setMayAlertAgain:(BOOL)mayAlertAgain forEventNamed:(NSString *)eventName
{
	[self setBool:mayAlertAgain
forAccountSpecificPropertyWithNameComponent:SCIPropertyNameComponentMayAlertAgain
			  key:eventName
inStorageLocation:SCIPropertyStorageLocationPersistent];
}

#pragma mark - LastReceivedTime

- (NSDate *)lastStartTimeShowCallCIRSuggestion
{
	return [self lastStartTimeForEventNamed:SCIPropertyNameSuffixEventShowCallCIRSuggestion];
}
- (void)setLastStartTimeShowCallCIRSuggestion:(NSDate *)lastStartTimeShowCallCIRSuggestion
{
	[self setLastStartTime:lastStartTimeShowCallCIRSuggestion forEventNamed:SCIPropertyNameSuffixEventShowCallCIRSuggestion];
}

- (NSDate *)lastStartTimeShowPSMGPromotion
{
	return [self lastStartTimeForEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}
- (void)setLastStartTimeShowPSMGPromotion:(NSDate *)lastStartTimeShowPSMGPromotion
{
	[self setLastStartTime:lastStartTimeShowPSMGPromotion forEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}

- (NSDate *)lastStartTimeShowWebDialPromotion
{
	return [self lastStartTimeForEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}
- (void)setLastStartTimeShowWebDialPromotion:(NSDate *)lastStartTimeShowWebDialPromotion
{
	[self setLastStartTime:lastStartTimeShowWebDialPromotion forEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}

- (NSDate *)lastStartTimeShowContactPhotosPromotion
{
	return [self lastStartTimeForEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}
- (void)setLastStartTimeShowContactPhotosPromotion:(NSDate *)lastStartTimeShowContactPhotosPromotion
{
	[self setLastStartTime:lastStartTimeShowContactPhotosPromotion forEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}

- (NSDate *)lastStartTimeShowSharedTextPromotion
{
	return [self lastStartTimeForEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}
- (void)setLastStartTimeShowSharedTextPromotion:(NSDate *)lastStartTimeShowSharedTextPromotion
{
	[self setLastStartTime:lastStartTimeShowSharedTextPromotion forEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}


#pragma mark - LastRemindLaterTime

- (NSDate *)lastRemindLaterTimeShowCallCIRSuggestion
{
	return [self lastRemindLaterTimeForEventNamed:SCIPropertyNameSuffixEventShowCallCIRSuggestion];
}
- (void)setLastRemindLaterTimeShowCallCIRSuggestion:(NSDate *)lastRemindLaterTimeShowCallCIRSuggestion
{
	[self setLastRemindLaterTime:lastRemindLaterTimeShowCallCIRSuggestion forEventNamed:SCIPropertyNameSuffixEventShowCallCIRSuggestion];
}

- (NSDate *)lastRemindLaterTimeShowPSMGPromotion
{
	return [self lastRemindLaterTimeForEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}
- (void)setLastRemindLaterTimeShowPSMGPromotion:(NSDate *)lastRemindLaterTimeShowPSMGPromotion
{
	[self setLastRemindLaterTime:lastRemindLaterTimeShowPSMGPromotion
				   forEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}

- (NSDate *)lastRemindLaterTimeShowWebDialPromotion
{
	return [self lastRemindLaterTimeForEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}
- (void)setLastRemindLaterTimeShowWebDialPromotion:(NSDate *)lastRemindLaterTimeShowWebDialPromotion
{
	[self setLastRemindLaterTime:lastRemindLaterTimeShowWebDialPromotion forEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}

- (NSDate *)lastRemindLaterTimeShowContactPhotosPromotion
{
	return [self lastRemindLaterTimeForEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}
- (void)setLastRemindLaterTimeShowContactPhotosPromotion:(NSDate *)lastRemindLaterTimeShowContactPhotosPromotion
{
	[self setLastRemindLaterTime:lastRemindLaterTimeShowContactPhotosPromotion forEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}

- (NSDate *)lastRemindLaterTimeShowSharedTextPromotion
{
	return [self lastRemindLaterTimeForEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}
- (void)setLastRemindLaterTimeShowSharedTextPromotion:(NSDate *)lastRemindLaterTimeShowSharedTextPromotion
{
	[self setLastRemindLaterTime:lastRemindLaterTimeShowSharedTextPromotion forEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}

#pragma mark - MayAlertAgain

- (BOOL)doNotShowPSMGPromoPageAgain
{
	return ![self mayAlertAgainForEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}
- (void)setDoNotShowPSMGPromoPageAgain:(BOOL)doNotShowPSMGPromoPageAgain
{
	[self setMayAlertAgain:!doNotShowPSMGPromoPageAgain forEventNamed:SCIPropertyNameSuffixEventShowPSMGPromotion];
}

- (BOOL)doNotShowWebDialPromoPageAgain
{
	return ![self mayAlertAgainForEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}
- (void)setDoNotShowWebDialPromoPageAgain:(BOOL)doNotShowWebDialPromoPageAgain
{
	[self setMayAlertAgain:!doNotShowWebDialPromoPageAgain forEventNamed:SCIPropertyNameSuffixEventShowWebDialPromotion];
}

- (BOOL)doNotShowContactPhotosPromoPageAgain
{
	return ![self mayAlertAgainForEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}
- (void)setDoNotShowContactPhotosPromoPageAgain:(BOOL)doNotShowContactPhotosPromoPageAgain
{
	[self setMayAlertAgain:!doNotShowContactPhotosPromoPageAgain forEventNamed:SCIPropertyNameSuffixEventShowContactPhotosPromotion];
}

- (BOOL)doNotShowSharedTextPromoPageAgain
{
	return ![self mayAlertAgainForEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}
- (void)setDoNotShowSharedTextPromoPageAgain:(BOOL)doNotShowSharedTextPromoPageAgain
{
	[self setMayAlertAgain:!doNotShowSharedTextPromoPageAgain forEventNamed:SCIPropertyNameSuffixEventShowSharedTextPromotion];
}

@end

static NSString * const SCIPropertyNameComponentLastReceivedTime = @"LastReceivedTime";
static NSString * const SCIPropertyNameComponentLastRemindLaterTime = @"LastRemindLaterTime";
static NSString * const SCIPropertyNameComponentMayAlertAgain = @"MayAlertAgain";

static NSString * const SCIPropertyNameSuffixEventShowCallCIRSuggestion = @"ShowCallCIRSuggestion";
static NSString * const SCIPropertyNameSuffixEventShowPSMGPromotion = @"ShowPSMGPromotion";
static NSString * const SCIPropertyNameSuffixEventShowWebDialPromotion = @"ShowWebDialPromotion";
static NSString * const SCIPropertyNameSuffixEventShowContactPhotosPromotion = @"ShowContactPhotosPromotion";
static NSString * const SCIPropertyNameSuffixEventShowSharedTextPromotion = @"ShowSharedTextPromotion";





