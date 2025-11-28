//
//  SCIStaticProviderAgreementState.h
//  ntouch
//
//  Created by Nate Chandler on 9/17/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

typedef NS_ENUM(NSUInteger, SCIStaticProviderAgreementState) {
	SCIStaticProviderAgreementStateNone,
	SCIStaticProviderAgreementStateAccepted,
	SCIStaticProviderAgreementStateExpired,
	SCIStaticProviderAgreementStateCancelled,
};
