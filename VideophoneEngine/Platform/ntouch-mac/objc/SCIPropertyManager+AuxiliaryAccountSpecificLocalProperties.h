//
//  SCIPropertyManager+AuxiliaryAccountSpecificLocalProperties.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"

@interface SCIPropertyManager (AuxiliaryAccountSpecificLocalProperties)

- (NSDate *)lastStartTimeForEventNamed:(NSString *)eventName;
- (void)setLastStartTime:(NSDate *)receivedTime forEventNamed:(NSString *)eventName;
- (NSDate *)lastRemindLaterTimeForEventNamed:(NSString *)eventName;
- (void)setLastRemindLaterTime:(NSDate *)remindTime forEventNamed:(NSString *)eventName;
- (BOOL)mayAlertAgainForEventNamed:(NSString *)eventName;
- (void)setMayAlertAgain:(BOOL)mayAlertAgain forEventNamed:(NSString *)eventName;

@property (nonatomic) NSDate *lastStartTimeShowCallCIRSuggestion;
@property (nonatomic) NSDate *lastStartTimeShowPSMGPromotion;
@property (nonatomic) NSDate *lastStartTimeShowWebDialPromotion;
@property (nonatomic) NSDate *lastStartTimeShowContactPhotosPromotion;
@property (nonatomic) NSDate *lastStartTimeShowSharedTextPromotion;

@property (nonatomic) NSDate *lastRemindLaterTimeShowCallCIRSuggestion;
@property (nonatomic) NSDate *lastRemindLaterTimeShowPSMGPromotion;
@property (nonatomic) NSDate *lastRemindLaterTimeShowWebDialPromotion;
@property (nonatomic) NSDate *lastRemindLaterTimeShowContactPhotosPromotion;
@property (nonatomic) NSDate *lastRemindLaterTimeShowSharedTextPromotion;

@property (nonatomic) BOOL doNotShowPSMGPromoPageAgain;
@property (nonatomic) BOOL doNotShowWebDialPromoPageAgain;
@property (nonatomic) BOOL doNotShowContactPhotosPromoPageAgain;
@property (nonatomic) BOOL doNotShowSharedTextPromoPageAgain;

@end

