//
//  StaticProviderAgreementViewController.m
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "StaticProviderAgreementViewController.h"
#import "ProviderAgreementViewController_SubclassInterface.h"

@implementation StaticProviderAgreementViewController

- (NSString *)agreementHTML
{
	NSString *eulaPath = [[NSBundle mainBundle] pathForResource:@"EULA" ofType:@"html"];
	NSString *eulaHTML = [NSString stringWithContentsOfFile:eulaPath encoding:NSUTF8StringEncoding error:NULL];
	return eulaHTML;
}

- (ProviderAgreementViewControllerContentViewType)viewType
{
	return ProviderAgreementViewControllerContentViewTypeWebView;
}

- (void)didAcceptAgreement
{
	if ([self.staticProviderDelegate respondsToSelector:@selector(staticProviderAgreementViewController:didAcceptAgreementFromStatus:)]) {
		[self.staticProviderDelegate staticProviderAgreementViewController:self didAcceptAgreementFromStatus:self.status];
	}
}

- (void)didRejectAgreement
{
	if ([self.staticProviderDelegate respondsToSelector:@selector(staticProviderAgreementViewController:didRejectAgreementFromStatus:)]) {
		[self.staticProviderDelegate staticProviderAgreementViewController:self didRejectAgreementFromStatus:self.status];
	}
}

@end
