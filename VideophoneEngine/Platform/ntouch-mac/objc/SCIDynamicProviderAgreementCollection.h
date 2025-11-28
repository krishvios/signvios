//
//  SCIDynamicProviderAgreementCollection.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/26/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIDynamicProviderAgreementStatus.h"

@interface SCIDynamicProviderAgreementCollection : NSObject

+ (instancetype)collectionWithAgreements:(NSArray *)agreements fetchToken:(id)token;

@property (nonatomic, readonly) NSArray *agreements;
@property (nonatomic, readonly) SCIDynamicProviderAgreementState worstState;
@property (nonatomic, readonly) id fetchToken;

- (SCIDynamicProviderAgreementState)stateForAgreementWithPublicID:(NSString *)publicID;
- (void)setState:(SCIDynamicProviderAgreementState)state forDynamicProviderAgreementWithPublicID:(NSString *)publicID;

- (SCIDynamicProviderAgreement *)firstUnacceptedAgreement;

@end
