//
//  SCIDynamicProviderAgreementContent.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIDynamicProviderAgreementContent.h"

@implementation SCIDynamicProviderAgreementContent

+ (instancetype)agreementContentWithPublicID:(NSString *)publicID text:(NSString *)text type:(NSString *)type
{
	SCIDynamicProviderAgreementContent *res = nil;
	
	if (publicID) {
		res = [[self alloc] init];
		res.publicID = publicID;
		res.text = text;
		res.type = type;
	}
	
	return res;
}

@end
