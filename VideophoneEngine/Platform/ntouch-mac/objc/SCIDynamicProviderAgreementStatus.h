//
//  SCIDynamicProviderAgreementStatus.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIDynamicProviderAgreementState.h"

@class SCIDynamicProviderAgreement;

@interface SCIDynamicProviderAgreementStatus : NSObject

+ (instancetype)statusWithStatusString:(NSString *)statusString agreementPublicID:(NSString *)agreementPublicID;
+ (instancetype)statusWithState:(SCIDynamicProviderAgreementState)state agreementPublicID:(NSString *)agreementPublicID;

@property (nonatomic) NSString *agreementPublicID;
@property (nonatomic) SCIDynamicProviderAgreementState state;

@end

#ifdef __cplusplus
extern "C" {
#endif
	
extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateFromStatusString(NSString *statusString);
extern NSString *SCIStatusStringFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementState state);

extern NSComparisonResult SCIDynamicProviderAgreementStateCompare(SCIDynamicProviderAgreementState leftHandSide, SCIDynamicProviderAgreementState rightHandSide);
extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateWorse(SCIDynamicProviderAgreementState first, SCIDynamicProviderAgreementState second);
extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateBetter(SCIDynamicProviderAgreementState first, SCIDynamicProviderAgreementState second);

extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateWorst(NSArray *states);
extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateBest(NSArray *states);
	
#ifdef __cplusplus
}
#endif
