//
//  ProviderAgreementViewController.m
//  ntouch
//
//  Created by Nate Chandler on 8/15/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "ProviderAgreementViewController.h"
#import "ProviderAgreementViewController_SubclassInterface.h"
#import "SCIVideophoneEngine.h"
#import "SCIDynamicProviderAgreementStatus.h"
#import "SCIDynamicProviderAgreementContent.h"
#import "Alert.h"
#import "Utilities.h"
#import "SCIDynamicProviderAgreement.h"
#import "UIViewController+SCIIOS7Additions.h"
#import "AppDelegate.h"
#import "ntouch-Swift.h"

@interface ProviderAgreementViewController () 
@property (nonatomic, readonly) SCIDynamicProviderAgreementStatus *agreementStatus;
@property (nonatomic, readonly) SCIDynamicProviderAgreementContent *agreementContent;

//View
@property (nonatomic) IBOutlet UIWebView *webView;
@property (nonatomic) IBOutlet UITextView *textView;
@property (nonatomic) UIBarButtonItem *acceptButton;
@property (nonatomic) UIBarButtonItem *rejectButton;

//Alert
@property (nonatomic) UIAlertController *rejectAlertView;

//State
@property (nonatomic, getter = isPromptingToReject) BOOL promptingToReject;
@end

@implementation ProviderAgreementViewController

#pragma mark - Lifecycle

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:NSStringFromClass([ProviderAgreementViewController class]) bundle:nibBundleOrNil];
    if (self) {
		UIBarButtonItem *rejectBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Reject") style:UIBarButtonItemStylePlain target:self action:@selector(rejectButtonTouchedUpInside:)];
		self.navigationItem.leftBarButtonItem = rejectBarButtonItem;
		self.rejectButton = rejectBarButtonItem;
		
		UIBarButtonItem *acceptBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Accept") style:UIBarButtonItemStylePlain target:self action:@selector(acceptButtonTouchedUpInside:)];
		self.navigationItem.rightBarButtonItem = acceptBarButtonItem;
		self.acceptButton = acceptBarButtonItem;
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyTheme) name:Theme.themeDidChangeNotification object:nil];
    }
    return self;
}

- (void)loadView
{
	[super loadView];
	
	self.edgesForExtendedLayout = UIRectEdgeNone;
	
	[self updateContentView];
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	[self updateButtons];
	[self applyTheme];
}

- (void)applyTheme
{
	self.textView.backgroundColor = Theme.backgroundColor;
	self.textView.textColor = Theme.textColor;
}

#pragma mark - Accessors: Convenience

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

#pragma mark - Accessors - Subclass Interface

- (NSAttributedString *)attributedAgreementText
{
	return [[NSAttributedString alloc] initWithString:self.agreementText attributes:nil];
}

- (NSString *)agreementText
{
	return nil;
}

- (NSString *)agreementHTML
{
	return nil;
}

- (ProviderAgreementViewControllerContentViewType)viewType
{
	return ProviderAgreementViewControllerContentViewTypeTextView;
}

#pragma mark - Accessors

- (void)setSubmitting:(BOOL)submitting
{
	_submitting = submitting;
	
	[self updateButtons];
}

- (void)setPromptingToReject:(BOOL)promptingToReject
{
	_promptingToReject = promptingToReject;
	
	[self updateButtons];
}

- (BOOL)acceptActionEnabled
{
	return !self.submitting && !self.promptingToReject;
}

- (BOOL)rejectActionEnabled
{
	return !self.submitting && !self.promptingToReject;
}

#pragma mark - Actions

- (IBAction)acceptButtonTouchedUpInside:(id)sender
{
	//This is a rather unfortunate way to prevent both button items from
	//	being pressed simultaneously.
	if (self.acceptActionEnabled) {
		[self didAcceptAgreement];
	}
}

- (IBAction)rejectButtonTouchedUpInside:(id)sender
{
	//This is a rather unfortunate way to prevent both button items from
	//	being pressed simultaneously.
	if (self.rejectActionEnabled) {
		if([self.agreementText isEqualToString:@"By clicking the \"Accept\" button, you hereby certify that you have a hearing or speech disability and need VRS to be able to communicate with other people.  You understand that the cost of VRS calls is paid for by contributions from other telecommunications users to the TRS Fund.\n\nYou further attest by clicking the \"Accept\" button that you are eligible to use VRS."])
		{
			[self didRejectAgreement];
		}
		else
		{
			UIAlertController *rejectAlertView = [UIAlertController alertControllerWithTitle: Localize(@"Are you sure?") message: Localize(@"Agreement.Message") preferredStyle:UIAlertControllerStyleAlert];
			
			[rejectAlertView addAction:[UIAlertAction actionWithTitle: Localize(@"No") style:UIAlertActionStyleCancel handler:^(UIAlertAction * action) {
				self.rejectAlertView = nil;
				self.promptingToReject = NO;
				[self updateButtons];
			}]];
			
			[rejectAlertView addAction:[UIAlertAction actionWithTitle: Localize(@"Yes") style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
				[self didRejectAgreement];
				self.rejectAlertView = nil;
			}]];
			
			self.rejectAlertView = rejectAlertView;
			self.promptingToReject = YES;
			
			[self presentViewController:rejectAlertView animated:YES completion:nil]; 
		}
	}
}

#pragma mark - Helpers: View Updating

- (void)updateButtons
{
	self.acceptButton.enabled = self.acceptActionEnabled;
	self.rejectButton.enabled = self.rejectActionEnabled;
}

- (void)updateContentView
{
	switch (self.viewType) {
		case ProviderAgreementViewControllerContentViewTypeTextView: {
			self.webView.hidden = YES;
			self.textView.hidden = NO;
			self.textView.text = self.agreementText;
		} break;
		case ProviderAgreementViewControllerContentViewTypeWebView: {
			self.webView.hidden = NO;
			self.textView.hidden = YES;
			[self.webView loadHTMLString:self.agreementHTML baseURL:nil];
		} break;
	}
}

#pragma mark - Subclass Interface

- (void)didAcceptAgreement
{
	
}

- (void)didRejectAgreement
{
	
}

#pragma mark - Rotation

- (BOOL)shouldAutorotate
{
	return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskAll;
}

@end
