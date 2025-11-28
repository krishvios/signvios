//
//  StaticProviderAgreementViewController.h
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "ProviderAgreementViewController.h"

@class SCIStaticProviderAgreementStatus;
@class StaticProviderAgreementViewController;

@protocol StaticProviderAgreementViewControllerDelegate <ProviderAgreementViewControllerDelegate>
- (void)staticProviderAgreementViewController:(StaticProviderAgreementViewController *)viewController didAcceptAgreementFromStatus:(SCIStaticProviderAgreementStatus *)status;
- (void)staticProviderAgreementViewController:(StaticProviderAgreementViewController *)viewController didRejectAgreementFromStatus:(SCIStaticProviderAgreementStatus *)status;
@end

@interface StaticProviderAgreementViewController : ProviderAgreementViewController
@property (nonatomic) SCIStaticProviderAgreementStatus *status;

@property (nonatomic, weak) id<StaticProviderAgreementViewControllerDelegate> staticProviderDelegate;
@end
