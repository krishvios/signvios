//
//  ProviderAgreementViewController.h
//  ntouch
//
//  Created by Nate Chandler on 8/15/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>

@class SCIDynamicProviderAgreement;

@class ProviderAgreementViewController;

@protocol ProviderAgreementViewControllerDelegate <NSObject>
@end

@interface ProviderAgreementViewController : UIViewController
@property (nonatomic, weak) id<ProviderAgreementViewControllerDelegate> providerDelegate;

@property (nonatomic, getter = isSubmitting) BOOL submitting;
@end
