//
//  SCIProviderAgreementType.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/9/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIProviderAgreementType.h"
#import "SCICoreVersion.h"

SCIProviderAgreementType SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(SCICoreVersion *coreVersion, BOOL dnyamicAgreementsFeatureEnabled)
{
	SCIProviderAgreementType res = SCIProviderAgreementTypeUnknown;
	
	if (dnyamicAgreementsFeatureEnabled &&
		coreVersion.majorVersionNumber >= SCICoreMajorVersionNumber4) {
		res = SCIProviderAgreementTypeDynamic;
	} else {
		res = SCIProviderAgreementTypeStatic;
	}
	
	return res;
}
