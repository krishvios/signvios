//
//  MyPhoneEditViewControllerNew.m
//  ntouch
//
//  Created by Kevin Selman on 9/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "MyPhoneEditViewControllerNew.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "MBProgressHUD.h"
#import "SCIRingGroupInfo.h"
#import "SCIVideophoneEngine.h"
#import "SCIUserAccountInfo.h"
#import "MyPhoneEditDetailViewController.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "IstiRingGroupManager.h"
#import "SCIContactManager.h"
#import "ntouch-Swift.h"

@interface MyPhoneEditViewControllerNew ()

@end

@implementation MyPhoneEditViewControllerNew

@synthesize ringGroupInfos;
@synthesize scrollView;
@synthesize callCIRButton;
@synthesize groupView;
@synthesize phone1View;
@synthesize phone2View;
@synthesize phone3View;
@synthesize phone4View;
@synthesize phone5View;
@synthesize ringGroupNumberTitleLabel;
@synthesize ringGroupNumberLabel;
@synthesize phone0NameLabel;
@synthesize phone0StatusLabel;
@synthesize phone1NameLabel;
@synthesize phone1StatusLabel;
@synthesize phone2NameLabel;
@synthesize phone2StatusLabel;
@synthesize phone3NameLabel;
@synthesize phone3StatusLabel;
@synthesize phone4NameLabel;
@synthesize phone4StatusLabel;
@synthesize detailButton0;
@synthesize detailButton1;
@synthesize detailButton2;
@synthesize detailButton3;
@synthesize detailButton4;
@synthesize addButton0;
@synthesize addButton1;
@synthesize addButton2;
@synthesize addButton3;
@synthesize addButton4;
@synthesize button0Label;
@synthesize button1Label;
@synthesize button2Label;
@synthesize button3Label;
@synthesize button4Label;

@synthesize selectedRingGroupInfo;
@synthesize hasAuthenticated;
@synthesize isSetupMode;

#pragma mark - View datasource

- (void)fetchRingGroupInfo {
	[self startFetchTimeout];
	
	[[SCIVideophoneEngine sharedEngine] fetchRingGroupInfoWithCompletion:^(NSArray *infos, NSError *error) {
		if(error)
			Alert(@"myPhone", [error localizedDescription]);
		else {
			self.ringGroupInfos = [infos mutableCopy];
			[self updateView];
		}
		[MBProgressHUD hideHUDForView:self.view animated:YES];
	}];
}

#pragma mark - UINavigationBar delegate methods

- (IBAction)doneButtonClicked:(id)sender {
	if(isSetupMode) {
		[self.navigationController popToRootViewControllerAnimated:YES];
	}
}

- (IBAction)callCirButtonClicked:(id)sender {
	if (!g_NetworkConnected) {
		AlertWithTitleAndMessage(Localize(@"Network Error"), Localize(@"call.err.requiresNetworkConnection"));
	}
	else {
		NSString *phone = [SCIContactManager customerServicePhoneFull];
		[SCICallController.shared makeOutgoingCallTo:phone dialSource:SCIDialSourceUIButton];
	}
}

#pragma mark - UI Actions

- (IBAction)detailsButtonClicked:(id)sender {
	UIButton *button = (UIButton*)sender;
	
	if(ringGroupInfos.count > button.tag)
		[self setSelectedRingGroupInfo:[ringGroupInfos objectAtIndex:button.tag]];
	else {
		Alert(Localize(@"myPhone"), Localize(@"myphone.phone.select.problem"));
		return;
	}
	
	if(!self.hasAuthenticated) {
		//Require password to edit group
		[self checkAuthentication];
	}
	else {
		// Navigate to RingGroup Setup view
		[self viewRingGroupDetails:[ringGroupInfos objectAtIndex:button.tag]];
	}
}

- (IBAction)addButtonClicked:(id)sender {
	if(!self.hasAuthenticated) {
		//Require password to edit group
		self.selectedRingGroupInfo = nil;
		[self checkAuthentication];
	}
	else {
		// Navigate to RingGroup Setup view
		[self viewRingGroupDetails:nil];
	}
}

- (void)checkAuthentication {
	//Require password to edit group
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"myPhone Password")
																   message:Localize(@"Enter your myPhone password:")
															preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
		textField.placeholder = Localize(@"Password");
		textField.secureTextEntry = YES;
	}];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil]];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
		NSString *passwordText = ((UITextField *)[alert.textFields objectAtIndex:0]).text;
		__weak MyPhoneEditViewControllerNew *weakSelf = self;
		[[SCIVideophoneEngine sharedEngine] validateRingGroupCredentialsWithPhone:[SCIVideophoneEngine sharedEngine].userAccountInfo.ringGroupLocalNumber password:passwordText completion:^(NSError *error) {
			if (error) {
				if(error.code == 2)
					Alert(Localize(@"myPhone.auth.err"), Localize(@"myPhone.edit.err.2"));
				else if(error.code == 73)
					Alert(Localize(@"myPhone.auth.err"), Localize(@"myPhone.edit.err.73"));
				else if(error.code == 99)
					Alert(Localize(@"myPhone.auth.err"), Localize(@"myPhone.edit.err.99"));
				else if(error.code == 100)
					Alert(Localize(@"myPhone.auth.err"), Localize(@"myPhone.edit.err.100"));
				else
					Alert(Localize(@"myPhone.auth.err"), [error localizedDescription]);
				
			} else {
				// Passed CredentialsValidate Navigate to myPhoneEditviewController
				self.hasAuthenticated = YES;
				[self viewRingGroupDetails:weakSelf.selectedRingGroupInfo];
			}
		}];
	}]];
	
	[self presentViewController:alert animated:YES completion:nil];
}

- (void)viewRingGroupDetails:(SCIRingGroupInfo *)ringGroupInfo {
	MyPhoneEditDetailViewController *detailViewController = [[MyPhoneEditDetailViewController alloc] initWithStyle:UITableViewStyleGrouped];
	detailViewController.isEditMode = (ringGroupInfo != nil);
	detailViewController.infoItem = ringGroupInfo;
	
	// Dont allow removing member if display mode is ReadOnly
	NSInteger displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	BOOL readOnly = (displayMode == IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY);
	
	detailViewController.allowsRemove = readOnly ? NO : (ringGroupInfos.count > 1);
	[self.navigationController pushViewController:detailViewController animated:YES];
}

#pragma mark - NSNotfications

- (void)notifyRingGroupChanged:(NSNotification *)note // SCINotificationRingGroupChanged
{
	SCILog(@"Start");
	//[self fetchRingGroupInfo];
	
	NSArray *infos = [[note userInfo] objectForKey:SCINotificationKeyRingGroupInfos];
	self.ringGroupInfos = infos;
	[self updateView];
}

- (void)notifyUserAccountInfoUpdated:(NSNotification *)note // SCINotificationUserAccountInfoUpdated
{
	SCILog(@"Start");
	[self updateView];
}

- (void)notifyRingGroupModeChanged:(NSNotification *)note //SCINotificationRingGroupModeChanged
{
	SCILog(@"Start");
	[self updateView];
}

#pragma mark - View lifecycle

- (void)updateView {
	SCILog(@"RingGroupInfos count: %d", self.ringGroupInfos.count);
	
	NSString * ringGroupNumber = [[SCIVideophoneEngine sharedEngine] userAccountInfo].preferredNumber;
	ringGroupNumberLabel.text = FormatAsPhoneNumber(ringGroupNumber);
	
	ringGroupNumberTitleLabel.textColor = Theme.tableCellTextColor;
	ringGroupNumberLabel.textColor = Theme.tableCellTextColor;
	
	// Configure Phone 0
	[self configureCell:phone0NameLabel statusLabel:phone0StatusLabel detailButton:detailButton0 buttonLabel:button0Label addButton:addButton0 phoneIndex:0];
	
	// Configure Phone 1
	[self configureCell:phone1NameLabel statusLabel:phone1StatusLabel detailButton:detailButton1 buttonLabel:button1Label addButton:addButton1 phoneIndex:1];

	// Configure Phone 2
	[self configureCell:phone2NameLabel statusLabel:phone2StatusLabel detailButton:detailButton2 buttonLabel:button2Label addButton:addButton2 phoneIndex:2];
	
	// Configure Phone 3
	[self configureCell:phone3NameLabel statusLabel:phone3StatusLabel detailButton:detailButton3 buttonLabel:button3Label addButton:addButton3 phoneIndex:3];
	
	// Configure Phone 4
	[self configureCell:phone4NameLabel statusLabel:phone4StatusLabel detailButton:detailButton4 buttonLabel:button4Label addButton:addButton4 phoneIndex:4];

}

- (void)configureCell:(UILabel *)nameLabel
		  statusLabel:(UILabel *)statusLabel
		 detailButton:(UIButton *)detailButton
		  buttonLabel:(UILabel *)buttonLabel
			addButton:(UIButton *)addButton
		   phoneIndex:(int)index
{
	
	NSInteger displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	BOOL readOnly = (displayMode == IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY);
	
	BOOL hasInfo = (ringGroupInfos.count > index);
	SCIRingGroupInfo *info = ringGroupInfos.count > index ? [ringGroupInfos objectAtIndex:index] : nil;
	
	NSString *stateString = Localize(info.stateString);
	
	nameLabel.text = info.name ? info.name : @"";
	nameLabel.textColor = Theme.tableCellTextColor;
	statusLabel.text = stateString ? [NSString stringWithFormat:Localize(@"Status: %@"), stateString] : @"";
	statusLabel.textColor = Theme.tableCellSecondaryTextColor;
	detailButton.hidden = !hasInfo;
	detailButton.tintColor = Theme.tableCellTintColor;
	buttonLabel.hidden = readOnly ? YES : hasInfo;
	addButton.enabled = (ringGroupInfos.count ==  index);
	addButton.hidden = readOnly ? YES : hasInfo;
}

- (void) cancelFetch {
	[MBProgressHUD hideHUDForView:self.view animated:YES];
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelFetch) object:nil];
}

- (void) startFetchTimeout {
	[MBProgressHUD hideHUDForView:self.view animated:YES];
	[[MBProgressHUD showHUDAddedTo:self.view animated:YES] setLabelText:Localize(@"Loading")]; // Do a timeout
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelFetch) object:nil];
	[self performSelector:@selector(cancelFetch) withObject:nil afterDelay:15.0];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	[self setTitle:Localize(@"myPhone")];

	if(isSetupMode) {
		self.navigationItem.hidesBackButton = YES;
		UIBarButtonItem *doneButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(doneButtonClicked:)];
		[self.navigationItem setRightBarButtonItem:doneButton animated:YES];
	}
	
	callCIRButton.titleLabel.textColor = Theme.tintColor;
	
	phone0NameLabel.text = @"";
	phone1NameLabel.text = @"";
	phone2NameLabel.text = @"";
	phone3NameLabel.text = @"";
	phone4NameLabel.text = @"";
	phone0StatusLabel.text = @"";
	phone1StatusLabel.text = @"";
	phone2StatusLabel.text = @"";
	phone3StatusLabel.text = @"";
	phone4StatusLabel.text = @"";
	
	[self applyThemeToView:groupView];
	[self applyThemeToView:phone1View];
	[self applyThemeToView:phone2View];
	[self applyThemeToView:phone3View];
	[self applyThemeToView:phone4View];
	[self applyThemeToView:phone5View];

	[self setContentSize];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.view.backgroundColor = Theme.backgroundColor;
	self.titleLabel.textColor = Theme.textColor;
	self.explanationLabel.textColor = Theme.textColor;
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupChanged:)
												 name:SCINotificationRingGroupChanged object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyUserAccountInfoUpdated:)
												 name:SCINotificationUserAccountInfoUpdated object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupModeChanged:)
												 name:SCINotificationRingGroupModeChanged object:[SCIVideophoneEngine sharedEngine]];
	
	[self fetchRingGroupInfo];
}

-(void)viewDidAppear:(BOOL)animated	{
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	SCILog(@"Start");

	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applyBorderToView:(UIView *)updateView {
	updateView.layer.borderColor = [UIColor lightGrayColor].CGColor;
	updateView.layer.borderWidth = 1.0f;
	updateView.layer.cornerRadius = 10.0f;
}

- (void)applyThemeToView:(UIView *)updateView {
	[self applyBorderToView:updateView];
	
	updateView.backgroundColor = Theme.tableCellBackgroundColor;
}

- (void)setContentSize {
	UIScrollView *tempScrollView=(UIScrollView *)self.view;
    tempScrollView.contentSize=CGSizeMake(scrollView.bounds.size.width, scrollView.bounds.size.height);
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 if(iPadB)
			 [self setContentSize];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

@end
