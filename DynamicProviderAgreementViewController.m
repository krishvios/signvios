//
//  DynamicProviderAgreementViewController.m
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "DynamicProviderAgreementViewController.h"
#import "ProviderAgreementViewController_SubclassInterface.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCIDynamicProviderAgreementContent.h"

@implementation DynamicProviderAgreementViewController

#pragma mark - Subclass Interface

- (void)setLoading:(BOOL)loading
{
	_loading = loading;
	
	[self updateButtons];
}

- (BOOL)acceptActionEnabled
{
	return super.acceptActionEnabled && !self.loading;
}

- (BOOL)rejectActionEnabled
{
	return  super.rejectActionEnabled && !self.loading;
}

- (NSString *)agreementText
{
	return self.agreement.content.text;
}

- (ProviderAgreementViewControllerContentViewType)viewType
{
	return ProviderAgreementViewControllerContentViewTypeTextView;
}

- (void)didAcceptAgreement
{
	if ([self.dynamicProviderDelegate respondsToSelector:@selector(dynamicProviderAgreementViewController:didAcceptAgreement:)]) {
		[self.dynamicProviderDelegate dynamicProviderAgreementViewController:self didAcceptAgreement:self.agreement];
	}
}

- (void)didRejectAgreement
{
	if ([self.dynamicProviderDelegate respondsToSelector:@selector(dynamicProviderAgreementViewController:didRejectAgreement:)]) {
		[self.dynamicProviderDelegate dynamicProviderAgreementViewController:self didRejectAgreement:self.agreement];
	}
}

@end
