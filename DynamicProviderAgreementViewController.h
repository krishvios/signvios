//
//  DynamicProviderAgreementViewController.h
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "ProviderAgreementViewController.h"

@class DynamicProviderAgreementViewController;

@protocol DynamicProviderAgreementViewControllerDelegate <ProviderAgreementViewControllerDelegate>
- (void)dynamicProviderAgreementViewController:(DynamicProviderAgreementViewController *)viewController didAcceptAgreement:(SCIDynamicProviderAgreement *)agreement;
- (void)dynamicProviderAgreementViewController:(DynamicProviderAgreementViewController *)viewController didRejectAgreement:(SCIDynamicProviderAgreement *)agreement;
@end

@interface DynamicProviderAgreementViewController : ProviderAgreementViewController
@property (nonatomic) SCIDynamicProviderAgreement *agreement;

@property (nonatomic, getter = isLoading) BOOL loading;

@property (nonatomic, weak) id<DynamicProviderAgreementViewControllerDelegate> dynamicProviderDelegate;
@end
