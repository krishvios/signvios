//
//  MyPhoneEditDetailViewController.mm
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "MyPhoneEditDetailViewController.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIUserAccountInfo.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIAccountManager.h"
#import "ntouch-Swift.h"

#include "IstiRingGroupManager.h"


@implementation MyPhoneEditDetailViewController

@synthesize infoItem;
@synthesize phoneNumberTextField;
@synthesize descriptionTextField;
@synthesize isEditMode;
@synthesize allowsRemove;

enum {
	endPointSection,
	buttonSection,
	tableSections
};

enum {
	phoneNumberRow,
	descriptionRow,
	endPointRows
};

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return tableSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	switch (section) {
		case endPointSection: return 2;
		case buttonSection: return (allowsRemove && isEditMode) ? 1 : 0;
		default: return 0;
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	TextInputTableViewCell *cell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier: TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
	cell.textField.delegate = self;
	cell.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
	cell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
	cell.textField.enabled = YES;
	
	switch (indexPath.section) {
		case endPointSection:
		{
			cell.textField.autocapitalizationType = UITextAutocapitalizationTypeWords;
			cell.editing = YES;
			switch (indexPath.row) {
				case phoneNumberRow:
					cell.textLabel.text = Localize(@"Phone Number");
					cell.textField.keyboardType = UIKeyboardTypePhonePad;
					phoneNumberTextField = cell.textField;
					phoneNumberTextField.text = infoItem.phone ? FormatAsPhoneNumber(infoItem.phone) : @"";
					phoneNumberTextField.enabled = !isEditMode;
					break;
				case descriptionRow: {
					cell.textLabel.text = Localize(@"Description");
					cell.textField.keyboardType = UIKeyboardTypeDefault;
					descriptionTextField = cell.textField;
					descriptionTextField.text = infoItem.name;
					descriptionTextField.placeholder = Localize(@"example: Kitchen, Joe's Phone, etc.");
					descriptionTextField.enabled = (self.displayMode != IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY);
				}	break;
				default:
					break;
				}
		} break;
		case buttonSection: {
			if(isEditMode) {
				ThemedTableViewCell *buttonCell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"ButtonCell"];
				if (buttonCell == nil) {
					buttonCell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ButtonCell"];
					buttonCell.textLabel.textAlignment = NSTextAlignmentCenter;
					buttonCell.textLabel.font = [UIFont boldSystemFontOfSize:14];
					buttonCell.textLabel.textColor = Theme.tableCellTintColor;
				}
				buttonCell.textLabel.text = Localize(@"Remove Phone");
				return buttonCell;
			}
		} break;
		default: break;
	}
	return cell;
}

#pragma mark - Table view delegate

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return [self tableView:tableView viewForFooterInSection:section].frame.size.height;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	switch (section) {
		case endPointSection: return [self viewController:self viewForHeaderInSection:section];
		case buttonSection: return nil;
		default: return nil;
	}
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
	switch (section) {
		case endPointSection: return [self viewController:self viewForFooterInSection:section];
		case buttonSection: return nil;
		default: return nil;
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	switch (section) {
		case endPointSection: return isEditMode ? Localize(@"Change Description") : nil;
		case buttonSection: return nil;
		default: return nil;
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
	NSString *tollFree = infoItem.tollfreePhone;
	
	switch (section) {
		case endPointSection: return tollFree.length ? [NSString stringWithFormat:@"%@ (%@)", FormatAsPhoneNumber(tollFree), Localize(@"toll-free")] : nil;
		case buttonSection: return nil;
		default: return nil;
	}
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	switch(indexPath.section) {
		case endPointSection: {
			
		} break;
		case buttonSection: {
			UIAlertController *actionSheet = [UIAlertController alertControllerWithTitle:Localize(@"myPhone.edit.remove.confirm")
																				 message:nil preferredStyle:UIAlertControllerStyleActionSheet];
			[actionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil]];
			[actionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Remove Phone")
															style:UIAlertActionStyleDestructive
														  handler:^(UIAlertAction * action) {
				__weak MyPhoneEditDetailViewController *weakSelf = self;
				[[SCIVideophoneEngine sharedEngine] removeRingGroupPhone:self.infoItem.phone completion:^(NSError *error) {
					if(error) {
						if(error.code == 73)
							Alert(Localize(@"myPhone.auth.err"), Localize(@"myPhone.edit.err.73"));
						else if(error.code == 82)
							Alert(Localize(@"myPhone Group Error"), Localize(@"myPhone.edit.err.82"));
						else if(error.code == 86)
							Alert(Localize(@"Error Removing myPhone Member"), Localize(@"myPhone.edit.err.86"));
						else if(error.code == 87)
							Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.87"));
						else if(error.code == 91)
							Alert(Localize(@"Error Removing myPhone Member"), Localize(@"myPhone.edit.err.91"));
						else if(error.code == 105)
							Alert(Localize(@"Error Removing myPhone Member"), Localize(@"myPhone.edit.err.105"));
						else
							Alert(Localize(@"Error Removing myPhone Member"), [error localizedDescription]);
					}
					else {
						SCIUserAccountInfo *accountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
						if([weakSelf.infoItem.phone isEqualToString:accountInfo.localNumber]) {
							// User removed themself, logout-login silently
							[[SCIVideophoneEngine sharedEngine] logoutWithBlock:^{
									NSString *phone = SCIDefaults.sharedDefaults.phoneNumber;
									NSString *pin = SCIDefaults.sharedDefaults.PIN;
									[[SCIAccountManager sharedManager] signIn:phone password:pin usingCache:NO];
								[self.navigationController popToRootViewControllerAnimated:YES];
							}];
						}
						else
							[self.navigationController popViewControllerAnimated:YES];
					}
					
				}];
			}]];
			
			UIPopoverPresentationController *popover = actionSheet.popoverPresentationController;
			if (popover)
			{
				popover.sourceView = appDelegate.tabBarController.view;
				popover.sourceRect = appDelegate.tabBarController.view.bounds;
				popover.permittedArrowDirections = 0;
			}
			
			[self presentViewController:actionSheet animated:YES completion:nil];
		} break;
		default: break;
	}
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	switch (indexPath.row) {
		case phoneNumberRow: return YES; break;
		case descriptionRow: return YES; break;
		default: return NO;	break;
	}
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (IBAction)saveButtonClicked:(id)sender {
	if(![[SCIVideophoneEngine sharedEngine] phoneNumberIsValid:phoneNumberTextField.text]) {
		Alert(Localize(@"Phone Number"), Localize(@"myPhone.edit.err.102"));
		return;
	}
	else if (descriptionTextField.text.length < 1) {
		Alert(Localize(@"Description"), Localize(@"myPhone.edit.err.81"));
		return;
	}
	else {
		if(isEditMode) {
			[[SCIVideophoneEngine sharedEngine] setRingGroupInfoWithPhone:UnformatPhoneNumber(phoneNumberTextField.text) description:descriptionTextField.text completion:^(NSError *error) {
				if(error) {
					if (error.code == 81)
						Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.81"));
					
					else if (error.code == 82)
						Alert(Localize(@"myPhone Group Error"), Localize(@"myPhone.edit.err.82"));
					
					else if (error.code == 87)
						Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.87"));
					
					else if (error.code == 102)
						Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.102"));
					
					else if (error.code == 103)
						Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.103"));
					
					else
						Alert(Localize(@"myPhone Member Error"),[error localizedDescription]);
				}
				else
					[self.navigationController popViewControllerAnimated:YES];
				
			}];
		}
		else {
			[[SCIVideophoneEngine sharedEngine] inviteRingGroupPhone:UnformatPhoneNumber(phoneNumberTextField.text) description:descriptionTextField.text completion:^(NSError *error) {
				if(error) {
					if (error.code == 74)
						Alert(Localize(@"Add Phone Error"), Localize(@"myPhone.edit.err.74"));
					
					else if (error.code == 75)
						Alert(Localize(@"Add Phone Error"), Localize(@"myPhone.edit.err.75"));
					
					else if (error.code == 81)
						Alert(Localize(@"myPhone Member Error"), Localize(@"myPhone.edit.err.81"));
					
					else
						Alert(Localize(@"Add Phone Error"), [error localizedDescription]);
				}
				else
					[self.navigationController popViewControllerAnimated:YES];
			}];
		
		}
	}
}

- (IBAction)cancelButtonClicked:(id)sender {
	[self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - TextField delegate methods

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField {
	if(isEditMode && textField == phoneNumberTextField)
		return NO;
	else 
		return (self.displayMode != IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY);
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	if (textField == phoneNumberTextField) {
		// make sure the phone number starts with a 1
		NSString *temp = textField.text;
		if (temp.length == 10 && ![[temp substringToIndex:1] isEqualToString:@"1"])
			temp = [@"1" stringByAppendingString:temp];
		
		textField.text = FormatAsPhoneNumber(temp);
		self.infoItem.phone = textField.text;
	}
	else if (textField == descriptionTextField)
		self.infoItem.name = textField.text;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    NSUInteger newLength = [textField.text length] + [string length] - range.length;
	
	if(textField == phoneNumberTextField)
		return (newLength > 20) ? NO : YES;
	else if (textField == descriptionTextField)
		return (newLength > 50) ? NO : YES;
	else
		return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}


#pragma mark - View lifecycle

- (void)viewDidLoad {
	[super viewDidLoad];
	
	self.tableView.rowHeight = 55;
	self.navigationItem.hidesBackButton = YES;
	self.tableView.allowsSelectionDuringEditing = YES;
	self.tableView.rowHeight = UITableViewAutomaticDimension;
	[self setEditing:YES];
	[self.tableView registerNib:TextInputTableViewCell.nib forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	
	UIBarButtonItem *cancelButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(cancelButtonClicked:)];
	[self.navigationItem setLeftBarButtonItem:cancelButton animated:YES];
	
	UIBarButtonItem *saveButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(saveButtonClicked:)];
	
	self.displayMode = [[SCIVideophoneEngine sharedEngine] ringGroupDisplayMode];
	
	if(self.displayMode != IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY)
		[self.navigationItem setRightBarButtonItem:saveButton animated:YES];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	if(infoItem) {
		phoneNumberTextField.text = infoItem.phone;
		descriptionTextField.text = infoItem.name;
		self.title = Localize(@"Edit Phone");
	}
	else
		self.title = Localize(@"Add Phone");
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	
	if(self.displayMode != IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY) {
		if(self.isEditMode)
			[descriptionTextField becomeFirstResponder];
		else
			[phoneNumberTextField becomeFirstResponder];
	}
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	if ([self.phoneNumberTextField isFirstResponder]) {
		[self.phoneNumberTextField resignFirstResponder];
	}
	else if ([self.descriptionTextField isFirstResponder]) {
		[self.descriptionTextField resignFirstResponder];
	}
	
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - Memory management

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
}

@end

