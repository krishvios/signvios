//
//  SCIDynamicProviderAgreementStatus_Cpp.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/9/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIDynamicProviderAgreementStatus.h"
#import "CstiCoreResponse.h"

@interface SCIDynamicProviderAgreementStatus ()
+ (instancetype)statusWithSAgreement:(CstiCoreResponse::SAgreement)sAgreement;
@end

extern SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateFromEAgreementStatus(CstiCoreRequest::EAgreementStatus status);
extern CstiCoreRequest::EAgreementStatus SCICoreRequestAgreementStatusFromAgreementState(SCIDynamicProviderAgreementState state);
