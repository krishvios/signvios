//
//  SCIProviderAgreementState.h
//  ntouch
//
//  Created by Nate Chandler on 9/17/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIStaticProviderAgreementState.h"
#import "SCIDynamicProviderAgreementState.h"

typedef NS_ENUM(NSUInteger, SCIProviderAgreementState) {
	SCIProviderAgreementStateUnknown,
	SCIProviderAgreementStateNone,
	SCIProviderAgreementStateRejected,
	SCIProviderAgreementStateCancelled,
	SCIProviderAgreementStateExpired,
	SCIProviderAgreementStateNeedsAcceptance,
	SCIProviderAgreementStateAccepted,
};

#ifdef __cplusplus
extern "C" {
#endif

extern SCIProviderAgreementState SCIProviderAgreementStateFromSCIStaticProviderAgreementState(SCIStaticProviderAgreementState staticState);
extern SCIProviderAgreementState SCIProviderAgreementStateFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementState dynamicState);

#ifdef __cplusplus
}
#endif
