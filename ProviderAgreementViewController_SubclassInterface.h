//
//  ProviderAgreementViewController_SubclassInterface.h
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "ProviderAgreementViewController.h"

typedef NS_ENUM(NSUInteger, ProviderAgreementViewControllerContentViewType) {
	ProviderAgreementViewControllerContentViewTypeTextView,
	ProviderAgreementViewControllerContentViewTypeWebView,
};

@interface ProviderAgreementViewController ()
@property (nonatomic, readonly) NSString *agreementText;
@property (nonatomic, readonly) NSAttributedString *attributedAgreementText;
@property (nonatomic, readonly) NSString *agreementHTML;
@property (nonatomic, readonly) ProviderAgreementViewControllerContentViewType viewType;
@property (nonatomic, readonly) BOOL acceptActionEnabled;
@property (nonatomic, readonly) BOOL rejectActionEnabled;
- (void)didAcceptAgreement;
- (void)didRejectAgreement;
- (void)updateButtons;
@end
