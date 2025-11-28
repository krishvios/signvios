//
//  SCIPropertyManager+AuxiliaryLocalProperties.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyManager.h"

@interface SCIPropertyManager (AuxiliaryLocalProperties)

@property (nonatomic, readonly) NSTimeInterval maximumIntervalSinceLastCIRCallToDismissCallCIRSuggestion;
@property (nonatomic, readonly) NSTimeInterval intervalBetweenCallCIRSuggestions;
@property (nonatomic, readonly) NSTimeInterval intervalBetweenPSMGPromotions;
@property (nonatomic, readonly) NSTimeInterval intervalBetweenWebDialPromotions;
@property (nonatomic, readonly) NSTimeInterval intervalBetweenContactPhotoPromotions;
@property (nonatomic, readonly) NSTimeInterval intervalBetweenSharedTextPromotions;

@end
