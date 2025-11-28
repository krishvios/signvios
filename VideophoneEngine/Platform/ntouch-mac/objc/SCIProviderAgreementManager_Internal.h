//
//  SCIProviderAgreementManager_Internal.h
//  ntouchMac
//
//  Created by Nate Chandler on 9/13/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIProviderAgreementManager.h"

typedef NS_ENUM(NSUInteger, SCIProviderAgreementManagerPresentedInterface)
{
	SCIProviderAgreementManagerPresentedInterfaceNone,
	SCIProviderAgreementManagerPresentedInterfaceDynamicProviderAgreement,
	SCIProviderAgreementManagerPresentedInterfaceStaticProviderAgreement,
};

typedef NS_ENUM(NSUInteger, SCIProviderAgreementManagerActivity) {
	SCIProviderAgreementManagerActivityNone,
	SCIProviderAgreementManagerActivityRejectingStaticAgreement,
	SCIProviderAgreementManagerActivityAcceptingStaticAgreement,
	SCIProviderAgreementManagerActivityAcceptingDynamicAgreement,
	SCIProviderAgreementManagerActivityRejectingDynamicAgreement,
	SCIProviderAgreementManagerActivityFetchingAgreements,
};

@class SCIDynamicProviderAgreement;

@interface SCIProviderAgreementManager ()

@property (nonatomic) SCIProviderAgreementManagerActivity activity;
@property (nonatomic) SCIProviderAgreementManagerPresentedInterface presentedInterface;

- (void)acceptDynamicAgreement:(SCIDynamicProviderAgreement *)agreement completion:(void (^)(BOOL success))completionBlock;
- (void)rejectDynamicAgreement:(SCIDynamicProviderAgreement *)agreement completion:(void (^)(BOOL success))completionBlock;
- (void)rejectStaticAgreementWithCompletion:(void (^)(BOOL success))completionBlock;
- (void)acceptStaticAgreementWithCompletion:(void (^)(BOOL success))completionBlock;

@end
