//
//  ChangePasswordViewController.mm
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "ChangePasswordViewController.h"
#import "Utilities.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine.h"
#import "SCIDefaults.h"
#import "ntouch-Swift.h"

enum {
	changePasswordSection,
	submitPasswordSection,
	numberOfSections
};

enum {
	oldPasswordRow,
	changePasswordRow,
	confirmPasswordRow,
	ChangePasswordSectionRows
};

enum {
	submitPasswordRow,
	submitPasswordSectionRows
};

@interface ChangePasswordViewController ()

@property (nonatomic, readonly) SCIDefaults *defaults;

@end

@implementation ChangePasswordViewController

@synthesize isMyPhonePassword;
@synthesize oldPasswordTextField;
@synthesize changePasswordTextField;
@synthesize	confirmPasswordTextField;
@synthesize enteredNewPassword;

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

#pragma mark - Table view delegate

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return numberOfSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	switch (section) {
		case changePasswordSection:
			return ChangePasswordSectionRows;
		case submitPasswordSection:
			return submitPasswordSectionRows;
		default:
			return 0;
	}
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	switch (section) {
		case changePasswordSection:
			return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
		case submitPasswordSection:
		default:
			return -1;
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
	return @"";
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	switch (indexPath.section) {
		case changePasswordSection: {
			switch (indexPath.row) {
				case oldPasswordRow: {
					TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
					oldPasswordTextField = textCell.textField;
					oldPasswordTextField.delegate = self;
					textCell.textLabel.text = Localize(@"Old Password");
					textCell.textField.accessibilityIdentifier = @"Old Password";
					textCell.textField.placeholder = Localize(@"required");
					textCell.textField.keyboardType = UIKeyboardTypeDefault;
					textCell.textField.secureTextEntry = YES;
					textCell.textField.text = oldPasswordTextField.text;
					return textCell;
				} break;
				case changePasswordRow: {
					TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
					changePasswordTextField = textCell.textField;
					changePasswordTextField.delegate = self;
					textCell.textLabel.text = Localize(@"New Password");
					textCell.textField.accessibilityIdentifier = @"New Password";
					textCell.textField.placeholder = Localize(@"required");
					textCell.textField.keyboardType = UIKeyboardTypeDefault;
					textCell.textField.secureTextEntry = YES;
					textCell.textField.text = changePasswordTextField.text;
					return textCell;
				} break;
				case confirmPasswordRow: {
					TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
					confirmPasswordTextField = textCell.textField;
					confirmPasswordTextField.delegate = self;
					textCell.textLabel.text = Localize(@"Confirm Password");
					textCell.textField.accessibilityIdentifier = @"Confirm Password";
					textCell.textField.placeholder = Localize(@"required");
					textCell.textField.keyboardType = UIKeyboardTypeDefault;
					textCell.textField.secureTextEntry = YES;
					textCell.textField.text = confirmPasswordTextField.text;
					return textCell;
				} break;
					
				default: 
					break;
			}
		}
		case submitPasswordSection: {
			ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"ButtonCell"];
			if (cell == nil) {
				cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ButtonCell"];
				cell.accessoryType = UITableViewCellAccessoryNone;
			}
			cell.textLabel.text = Localize(@"Change Password");
			return cell;
			break;
		}
	}
	return [[UITableViewCell alloc] init];
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    // Removing text field character limit.  Causes confusion.
	//NSUInteger newLength = [textField.text length] + [string length] - range.length;
    //return (newLength > 15) ? NO : YES;
	
	return YES;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	return (tableView.editing) ? indexPath : nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
	switch (indexPath.section) {
		case submitPasswordSection: {
			switch (indexPath.row) {
				case submitPasswordRow: {
					// Verify old password
					SCILog(@"Verify old password, PIN: %@", self.defaults.PIN);
					if( (!isMyPhonePassword && ![oldPasswordTextField.text isEqualToString:self.defaults.PIN]) ) {
						Alert(Localize(@"Old Password"), Localize(@"change.pass.old.incorrect"));
						break;
					}
					// Verify length of new password
					else if (changePasswordTextField.text.length < 1) {
						Alert(Localize(@"New Password"), Localize(@"change.pass.new.blank"));
						break;
					}
					// Verify length of new password
					else if (changePasswordTextField.text.length > 15) {
						Alert(Localize(@"New Password"), Localize(@"change.pass.new.15.chars"));
						break;
					}
					else if (ValidateInputField(changePasswordTextField.text, passwordRegEx)) {
						Alert(Localize(@"New Password"), Localize(@"change.pass.new.standard.chars"));
					}
					// Verify new passord and confirm fields match
					else if (![changePasswordTextField.text isEqualToString:confirmPasswordTextField.text]) {
						Alert(Localize(@"Confirm Password"), Localize(@"change.pass.conf.notmatch"));
						break;
					}
					else {
						self.enteredNewPassword = changePasswordTextField.text;
						if(isMyPhonePassword) {
							// Perform password change for myPhone Account
							SCIUserAccountInfo *userInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
							__weak ChangePasswordViewController *weakSelf = self;
							[[SCIVideophoneEngine sharedEngine] validateRingGroupCredentialsWithPhone:userInfo.ringGroupLocalNumber password:oldPasswordTextField.text completion:^(NSError *error) {
								if (error) {
									Alert(Localize(@"Old Password"), Localize(@"change.pass.old.incorrect"));
								} else {
									[[SCIVideophoneEngine sharedEngine] setRingGroupPassword:weakSelf.changePasswordTextField.text completion:^(NSError *error) {
										if (error) {
											Alert(@"Change Password", [error.userInfo objectForKey:NSLocalizedDescriptionKey]);
										} else {
											[weakSelf displayPasswordChangedAlert];
										}
									}];
								}
							}];

						}
						else {
							// Perform password change for normal account
							[[SCIVideophoneEngine sharedEngine] changePassword:changePasswordTextField.text completionHandler:^(NSError *error) {
								if(error)
									Alert(Localize(@"Change Password"), Localize(@"change.pass.problem"));
								else {
									[self displayPasswordChangedAlert];
								}
							}];
							
							break;
						}
					}
				}
			}
		}
	}
	
	// Deselect button, so it doesnt appear highlighted.
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (void)displayPasswordChangedAlert {
	
	if(self.navigationController.visibleViewController == self && !self.presentedViewController)
	{
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"New Password Saved")
																	   message:Localize(@"change.pass.new.saved")
																preferredStyle:UIAlertControllerStyleAlert];
		
		[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")  style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			[alert dismissViewControllerAnimated:YES completion:nil];
			[self.navigationController popViewControllerAnimated:YES];
		}]];
		
		[self presentViewController:alert animated:YES completion:nil];
		
		// Update saved login PIN with new one.
		if(self.defaults.savePIN) {
			self.defaults.PIN = self.enteredNewPassword;
		}
	}
}

#pragma mark - TextField delegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

#pragma mark - View lifecycle

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
	self.title = Localize(@"Change Password");
	self.tableView.allowsSelectionDuringEditing = YES;
	
	[self.tableView registerNib:[UINib nibWithNibName:@"ButtonTableViewCell" bundle:nil] forCellReuseIdentifier:ButtonTableViewCell.kCellIdentifier];
	
	self.enteredNewPassword = @"";
	
	[self.tableView registerNib:TextInputTableViewCell.nib forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	self.tableView.rowHeight = UITableViewAutomaticDimension;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
	return Theme.statusBarStyle;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[self.tableView setEditing:YES];
	self.navigationItem.rightBarButtonItem.enabled = YES;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(applicationWillResignActive:)
												 name:UIApplicationWillResignActiveNotification
											   object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self.tableView setEditing:NO];
	oldPasswordTextField.text = @"";
	changePasswordTextField.text = @"";
	confirmPasswordTextField.text = @"";
}

- (void)applicationWillResignActive:(NSNotification *)notification {
	oldPasswordTextField.text = @"";
	changePasswordTextField.text = @"";
	confirmPasswordTextField.text = @"";
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	if (self.oldPasswordTextField.isFirstResponder)
		[self.oldPasswordTextField resignFirstResponder];
	else if (self.changePasswordTextField.isFirstResponder)
		[self.changePasswordTextField resignFirstResponder];
	else if (self.confirmPasswordTextField.isFirstResponder)
		[self.confirmPasswordTextField resignFirstResponder];
	
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 // to update our custom header/footer views
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


@end
