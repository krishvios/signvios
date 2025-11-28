//
//  SCIProviderAgreementType.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/9/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCICoreVersion;

typedef NS_ENUM(NSUInteger, SCIProviderAgreementType) {
	SCIProviderAgreementTypeNone = 0,
	SCIProviderAgreementTypeStatic,
	SCIProviderAgreementTypeDynamic,
	SCIProviderAgreementTypeUnknown = NSUIntegerMax,
};

#ifdef __cplusplus
extern "C" {
#endif

extern SCIProviderAgreementType SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(SCICoreVersion *coreVersion, BOOL dnyamicAgreementsFeatureEnabled);
	
#ifdef __cplusplus
}
#endif
