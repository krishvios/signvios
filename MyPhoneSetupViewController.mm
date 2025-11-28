//
//  MyPhoneSetupViewController.mm
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "MyPhoneSetupViewController.h"
#import "MyPhoneEditViewControllerNew.h"
#import "Utilities.h"
#import "Alert.h"
#import "MBProgressHUD.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "IstiRingGroupManager.h"
#import "SCIContactManager.h"
#import "SCIVideoPlayer.h"
#import "SCIDefaults.h"
#import "ntouch-Swift.h"


@implementation MyPhoneSetupViewController

@synthesize phoneNumberTextField;
@synthesize descriptionTextField;
@synthesize passwordTextField;
@synthesize confirmPasswordTextField;
@synthesize phoneDescription;
@synthesize password;
@synthesize confirmPassword;

enum {
	cirSection,
	setupSection,
	tableSections
};

enum {
	phoneNumberRow,
	descriptionRow,
	adminPasswordRow,
	adminPasswordConfirmRow,
	setupSectionRows
};

enum {
	cirButtonRow,
	cirSectionRows
};


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return tableSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	switch (section) {
		case setupSection: return [self isReadOnlyMode] ? 0 : setupSectionRows;
		case cirSection: return cirSectionRows;
		default: return 0;
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	TextInputTableViewCell *cell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
	cell.textField.delegate = self;
	cell.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
	cell.textField.autocorrectionType = UITextAutocorrectionTypeNo;

	switch (indexPath.section) {
		case setupSection:
		{
			switch (indexPath.row) {
				case phoneNumberRow: {
					cell.textLabel.text = Localize(@"myPhone Number");
					NSString *preferredNumber = [[[SCIVideophoneEngine sharedEngine] userAccountInfo] preferredNumber];
					cell.textField.text = FormatAsPhoneNumber(preferredNumber);
					phoneNumberTextField = cell.textField;
					[cell.textField setEnabled:NO];
					cell.textField.secureTextEntry = NO;
					cell.textField.autocapitalizationType = UITextAutocapitalizationTypeWords;
				} break;
				case descriptionRow: {
					cell.textLabel.text = Localize(@"Phone Description");
					cell.textField.text = self.phoneDescription;
					descriptionTextField = cell.textField;
					cell.textField.placeholder = Localize(@"ntouch Mobile");
					cell.textField.secureTextEntry = NO;
					cell.textField.autocapitalizationType = UITextAutocapitalizationTypeWords;
				} break;
				case adminPasswordRow: {
					cell.textLabel.text = Localize(@"myPhone Password");
					cell.textField.text = self.password;
					passwordTextField = cell.textField;
					cell.textField.placeholder = Localize(@"required");
					cell.textField.secureTextEntry = YES;
					cell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
				}break;
				case adminPasswordConfirmRow: {
					cell.textLabel.text = Localize(@"Confirm Password");
					cell.textField.text = self.confirmPassword;
					confirmPasswordTextField = cell.textField;
					cell.textField.placeholder = Localize(@"required");
					cell.textField.secureTextEntry = YES;
					cell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
				}break;
			}
		} break;
		case cirSection:
		{
			switch (indexPath.row) {
				case cirButtonRow: {
					ThemedTableViewCell *buttonCell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"ButtonCell"];
					if (buttonCell == nil) {
						buttonCell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ButtonCell"];
						buttonCell.textLabel.textAlignment = NSTextAlignmentCenter;
						buttonCell.textLabel.font = [UIFont boldSystemFontOfSize:17];
					}
					buttonCell.textLabel.text = Localize(@"Call Customer Care");
					return buttonCell;
				} break;
			}
		} break;
		default: break;
	}
	return cell;
}

#pragma mark - Table view delegate



- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return UITableViewAutomaticDimension;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForHeaderInSection:(NSInteger)section {
	return 50.0;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	NSString *title = Localize(@"myphone.welcome.title");
	NSString *subTitle = Localize(@"myphone.welcome.msg");
	NSString *subTitle2 = Localize(@"myphone.setup.msg");
	NSString *cirTitle = Localize(@"myphone.customercare.msg");
	
	SectionHeaderView *header = (SectionHeaderView *)[self.tableView dequeueReusableHeaderFooterViewWithIdentifier:SectionHeaderView.reuseIdentifier];
	
	switch (section) {
		case cirSection: {
			if ([self isReadOnlyMode]) {
				header.textLabel.text = title;
				header.subTextLabel.text = cirTitle;
				return header;
			}
			else {
				header.textLabel.text = title;
				header.subTextLabel.text = subTitle;
				return header;
			}
		} break;
		case setupSection: {
			if ([self isReadOnlyMode]) {
				return nil;
			}
			else {
				header.textLabel.hidden = YES;
				header.subTextLabel.text = subTitle2;
				return header;
			}
		} break;
		default: return nil;
	}
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
	return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	switch (indexPath.section) {
		case cirSection: {
			switch (indexPath.row) {
				case cirButtonRow: {
					
					
					if (!g_NetworkConnected) {
						AlertWithTitleAndMessage(Localize(@"Network Error"), Localize(@"call.err.requiresNetworkConnection"));
					}
					else {
						NSString *phone = [SCIContactManager customerServicePhoneFull];
						[SCICallController.shared makeOutgoingCallTo:phone dialSource:SCIDialSourceUIButton];
					}
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"call_cir" }];
				} break;
			}
		}
	}
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	switch (indexPath.row) {
		case descriptionRow: return YES; break;
		case adminPasswordRow: return YES; break;
		case adminPasswordConfirmRow: return YES; break;
		default: return NO;	break;
	}
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

#pragma mark - UINavigationBar delegate methods

- (IBAction)nextButtonClicked:(id)sender {
	if (descriptionTextField.text.length < 1) {
		Alert(Localize(@"myphone.error.create.title"), Localize(@"myphone.error.description.msg"));
		return;
	}
	else if(passwordTextField.text.length < 1) {  // more validation
		Alert(Localize(@"myphone.error.create.title"), Localize(@"myphone.error.password.msg"));
		return;
	}
	else if (![confirmPasswordTextField.text isEqualToString:passwordTextField.text]) {
		Alert(Localize(@"myphone.error.create.title"), Localize(@"myphone.error.password.match.msg"));
		return;
	}
	else {
		[descriptionTextField resignFirstResponder];
		[passwordTextField resignFirstResponder];
		[confirmPasswordTextField resignFirstResponder];
		
		[[MBProgressHUD showHUDAddedTo:appDelegate.tabBarController.view animated:YES] setLabelText:Localize(@"Creating Group")];
		__weak MyPhoneSetupViewController *weakSelf = self;
		[[SCIVideophoneEngine sharedEngine] createRingGroupWithAlias:descriptionTextField.text password:passwordTextField.text completion:^(NSError *error) {
			[MBProgressHUD hideHUDForView:appDelegate.tabBarController.view animated:YES];
			if(error)
				Alert(Localize(@"myPhone"), [error localizedDescription]);
			else {
				// Show edit view to add more phones.
				[SCIDefaults sharedDefaults].PIN = weakSelf.passwordTextField.text; // Save new password for next login.
				[[SCIVideophoneEngine sharedEngine] loadUserAccountInfo];
				MyPhoneEditViewControllerNew *editViewController = [[MyPhoneEditViewControllerNew alloc] init];
				editViewController.hasAuthenticated = YES;
				editViewController.isSetupMode = YES;
				[self.navigationController pushViewController:editViewController animated:YES];
			}
		}];
	}
}

- (IBAction)cancelButtonClicked:(id)sender {
	[self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - TextField delegate methods

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    NSUInteger newLength = [textField.text length] + [string length] - range.length;
    return (newLength > 50) ? NO : YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	if(textField == descriptionTextField)
		self.phoneDescription = descriptionTextField.text;
	else if(textField == passwordTextField)
		self.password = passwordTextField.text;
	else if(textField == confirmPasswordTextField)
		self.confirmPassword = confirmPasswordTextField.text;
}

#pragma mark - View lifecycle
- (BOOL)isReadOnlyMode {
	BOOL result = NO;
	NSInteger displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	switch (displayMode) {
		case IstiRingGroupManager::eRING_GROUP_DISABLED:
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_HIDDEN:
			result = NO; break;
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY:
			result = YES; break;
		case IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_WRITE:
			result = NO; break;
		default:
			result = YES;
	}
	return result;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	self.title = Localize(@"myPhone");
	self.tableView.allowsSelectionDuringEditing = YES;
	[self setEditing:YES];
	[self.tableView registerNib:TextInputTableViewCell.nib forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	[self.tableView registerNib:SectionHeaderView.nib forHeaderFooterViewReuseIdentifier:SectionHeaderView.reuseIdentifier];

	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyRingGroupModeChanged:)
												 name:SCINotificationRingGroupModeChanged
											   object:[SCIVideophoneEngine sharedEngine]];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	UIBarButtonItem *cancelButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
																				  target:self
																				  action:@selector(cancelButtonClicked:)];
	[self.navigationItem setLeftBarButtonItem:cancelButton animated:YES];
	
	UIBarButtonItem *nextButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Create Group")
																   style:UIBarButtonItemStyleDone
																  target:self
																  action:@selector(nextButtonClicked:)];
	if(![self isReadOnlyMode])
		[self.navigationItem setRightBarButtonItem:nextButton];

	[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"myphone_setup" }];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[self.navigationItem setHidesBackButton:YES];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}

-(NSUInteger)supportedInterfaceOrientations {
	if([self.descriptionTextField isFirstResponder])
		[self.descriptionTextField resignFirstResponder];
	else if([self.passwordTextField isFirstResponder])
		[self.passwordTextField resignFirstResponder];
	else if([self.confirmPasswordTextField isFirstResponder])
		[self.confirmPasswordTextField resignFirstResponder];
	
	return UIInterfaceOrientationMaskAll;
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - Notifications

- (void)notifyRingGroupModeChanged:(NSNotification *)note { //SCINotificationRingGroupModeChanged

	// Dismiss view if mode changed while in view.
	NSInteger displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	if(displayMode == IstiRingGroupManager::eRING_GROUP_DISABLED ||
	   displayMode == IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_HIDDEN)
	[self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Memory management

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
}

@end

