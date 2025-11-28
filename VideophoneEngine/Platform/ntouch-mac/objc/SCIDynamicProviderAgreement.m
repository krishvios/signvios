//
//  SCIDynamicProviderAgreement.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/19/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIDynamicProviderAgreement.h"

@implementation SCIDynamicProviderAgreement

+ (instancetype)agreementWithStatus:(SCIDynamicProviderAgreementStatus *)status content:(SCIDynamicProviderAgreementContent *)content
{
	SCIDynamicProviderAgreement *res = [[self alloc] init];
	
	res.status = status;
	res.content = content;
	
	return res;
}

@end
