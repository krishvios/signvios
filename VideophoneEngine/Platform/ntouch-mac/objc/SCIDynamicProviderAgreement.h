//
//  SCIDynamicProviderAgreement.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/19/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIDynamicProviderAgreementStatus;
@class SCIDynamicProviderAgreementContent;

@interface SCIDynamicProviderAgreement : NSObject
+ (instancetype)agreementWithStatus:(SCIDynamicProviderAgreementStatus *)status content:(SCIDynamicProviderAgreementContent *)content;
@property (nonatomic) SCIDynamicProviderAgreementStatus *status;
@property (nonatomic) SCIDynamicProviderAgreementContent *content;
@end
