//
//  PersonalInfoViewController
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "PersonalInfoViewController.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "MBProgressHUD.h"
#import "AddressFormatters.h"
#import "SCIUserAccountInfo.h"
#import "SCIUserAccountInfo+Convenience.h"
#import "SCIUserPhoneNumbers.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "ChangePasswordViewController.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import "ContactUtilities.h"
#import "TypeListController.h"
#import "UIImage+ProportionalFill.h"
#import "SCIAccountManager.h"
#import "ntouch-Swift.h"

@interface PersonalInfoViewController ()
{
    NSArray *personalInfoSectionsAndRowsArray;
}

@property (nonatomic, assign) BOOL displayContactImageActionSheet;
@property (nonatomic, strong) IBOutlet UIView *contactImageButtonContainerView;
@property (nonatomic, strong) IBOutlet UIButton *contactImageButton;
@property (nonatomic) IBOutlet UILabel *contactImageButtonLabel;

@end

@implementation PersonalInfoViewController

@synthesize profileImageActionSheet;
@synthesize profileImageCell;
@synthesize profileImage;
@synthesize imagePicker;
@synthesize headerView;
@synthesize footerView;
@synthesize nameLabel;
@synthesize localNumberLabel;
@synthesize tollFreeNumberLabel;

@synthesize devNameLabel;
@synthesize devLocalNumberLabel;
@synthesize devTollFreeNumberLabel;

@synthesize signMailNotify;
@synthesize signMailNotifySwitch;
@synthesize callerIDSwitch;
@synthesize savingInProgress;
@synthesize hasChangedFields = _hasChangedFields;
@synthesize isRingGroup;
@synthesize hasTollFreeNumber;
@synthesize showDeviceInfo;

@synthesize displayContactImageActionSheet;

#pragma mark - Table view data source

- (void)loadTableData {
	NSMutableArray *mutablePersonalInfoSectionsAndRowsArray = [NSMutableArray array];
	//
	// Contact Photo Section
	//
	
	NSMutableArray *contactPhotoSectionArray = [NSMutableArray array];
    if ([[SCIVideophoneEngine sharedEngine] contactPhotosFeatureEnabled]) {
		[contactPhotoSectionArray addObjectsFromArray:[NSArray arrayWithObjects:@"contactPhotosEnabledRow", @"contactPhotoPrivacy", nil]];
    }
	NSDictionary *contactPhotoSectionArrayDict = @{ @"sectionNameKey" : @"contactPhotoSection", @"rows": contactPhotoSectionArray };
	[mutablePersonalInfoSectionsAndRowsArray addObject:contactPhotoSectionArrayDict];
	
	//
	// Notifications Section
	//
	NSMutableArray *notifcationsSectionArray = [NSMutableArray array];
    [notifcationsSectionArray addObjectsFromArray:[NSArray arrayWithObjects:@"signMailNotifyRow", @"email1Row", @"email2Row", nil]];
	
	NSDictionary *inputSectionArrayDict = @{ @"sectionNameKey" : @"notificationSection", @"rows": notifcationsSectionArray };
	[mutablePersonalInfoSectionsAndRowsArray addObject:inputSectionArrayDict];

	//
	// Caller ID Section
	//
	NSMutableArray *callIDSectionArray = [NSMutableArray array];
	
    if (self.hasTollFreeNumber) {
        [callIDSectionArray insertObject:@"callIDRow" atIndex:0];
    }
	
	NSDictionary *callIDSectionArrayDict = @{ @"sectionNameKey" : @"callIDSection", @"rows": callIDSectionArray };
    [mutablePersonalInfoSectionsAndRowsArray addObject:callIDSectionArrayDict];
	
	//
	// Device Info Section
	//
	NSMutableArray *deviceInfoSectionArray = [NSMutableArray array];
	
    if (self.isRingGroup) {
        [deviceInfoSectionArray insertObject:@"deviceInfoRow" atIndex:0];
    }
	
	NSDictionary *deviceInfoSectionArrayDict = @{ @"sectionNameKey" : @"deviceInfoSection", @"rows": deviceInfoSectionArray };
    [mutablePersonalInfoSectionsAndRowsArray addObject:deviceInfoSectionArrayDict];
	
	//
	// Password Section
	//
	NSArray *passwordSectionArray = [NSArray arrayWithObject:@"passwordButtonRow"];
	NSDictionary *passwordSectionArrayDict = @{ @"sectionNameKey" : @"passwordSection", @"rows": passwordSectionArray };
    [mutablePersonalInfoSectionsAndRowsArray addObject:passwordSectionArrayDict];
	
	personalInfoSectionsAndRowsArray = [mutablePersonalInfoSectionsAndRowsArray copy];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return [personalInfoSectionsAndRowsArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    //Number of rows it should expect should be based on the section
    NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    return [array count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    NSString *row = [array objectAtIndex:indexPath.row];
	
	static NSString *CellIdentifier = @"Cell";
	ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:CellIdentifier];
		cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
	}
	
	if ([row isEqualToString:@"contactPhotoPrivacy"]) {
		static NSString *CellIdentifier = @"ConactPhotoPrivacyCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		}
		cell.textLabel.text = Localize(@"Share Profile Photo");
		
		SCIProfileImagePrivacyLevel privacyLevel = [[SCIVideophoneEngine sharedEngine] profileImagePrivacyLevel];
		
		if (self.updateProfilePhotoPrivacyOnExit)
			privacyLevel = self.newPrivacyLevel;
		
		NSString *levelString = [SCIProfileImagePrivacy NSStringFromSCIProfileImagePrivacyLevel:privacyLevel];
		NSString *shortLevelString = [SCIProfileImagePrivacy ShortNSStringFromSCIProfileImagePrivacyLevel:privacyLevel];
		
		if (iPadB) {
			cell.detailTextLabel.text = Localize(levelString);
		} else {
			cell.detailTextLabel.text = Localize(shortLevelString);
		}
		
		return cell;
	}
	
	if ([row isEqualToString:@"contactPhotosEnabledRow"]) {
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:@"contactPhotosEnabledCell"];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:@"contactPhotosEnabledCell"];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
		}
		cell.switchControl.enabled = tableView.isEditing;
		cell.textLabel.text = Localize(@"Show Contact Photos");
		cell.switchControl.on = [[SCIVideophoneEngine sharedEngine] displayContactImages];
		self.contactPhotosEnabledSwitch = cell.switchControl;
		return cell;
	}
	
	if ([row isEqualToString:@"signMailNotifyRow"]) {
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:@"signMailNotifyCell"];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:@"signMailNotifyCell"];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
		}
		cell.switchControl.enabled = tableView.isEditing;
		cell.textLabel.text = Localize(@"Notify me for SignMail");
		cell.switchControl.on = self.signMailNotify;
		signMailNotifySwitch = cell.switchControl;
		return cell;
	}
	
	if ([row isEqualToString:@"email1Row"]) {
		TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
		textCell.textField.delegate = self;
		textCell.textField.textAlignment = NSTextAlignmentLeft;
		textCell.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
		textCell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
		textCell.textField.keyboardType = UIKeyboardTypeEmailAddress;
		textCell.textField.returnKeyType = UIReturnKeyDone;
		
		textCell.textLabel.text = Localize(@"Email address");
		self.email1TextField = textCell.textField;
		textCell.textField.placeholder = Localize(@"email");
		textCell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
		textCell.textField.text = self.email1;
		textCell.textField.accessibilityIdentifier = @"Email1TextField"; // For Automation
		return textCell;
	}
	
	if ([row isEqualToString:@"email2Row"]) {
		TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
		textCell.textField.delegate = self;
		textCell.textField.textAlignment = NSTextAlignmentLeft;
		textCell.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
		textCell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
		textCell.textField.keyboardType = UIKeyboardTypeEmailAddress;
		textCell.textField.returnKeyType = UIReturnKeyDone;
		
		textCell.textLabel.text = Localize(@"Email address");
		self.email2TextField = textCell.textField;
		textCell.textField.placeholder = Localize(@"email");
		textCell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
		textCell.textField.text = self.email2;
		textCell.textField.accessibilityIdentifier = @"Email2TextField"; // For Automation
		return textCell;
	}
	
	if ([row isEqualToString:@"callIDRow"]) {
		SwitchCell *cell = (SwitchCell *)[tableView dequeueReusableCellWithIdentifier:@"callerIDCell"];
		if (cell == nil) {
			cell = [[SwitchCell alloc] initWithReuseIdentifier:@"callerIDCell"];
			[cell.switchControl addTarget:self action:@selector(controlValueChanged:) forControlEvents:UIControlEventValueChanged];
		}
		cell.switchControl.enabled = tableView.isEditing;
		cell.textLabel.text = Localize(@"Use Toll-Free number");
		cell.switchControl.on = self.useTollFreeCallerID;
		callerIDSwitch = cell.switchControl;
		return cell;
	}
	
	if ([row isEqualToString:@"deviceInfoRow"]) {
		static NSString *CellIdentifier = @"DeviceInfoCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
		}
		if(showDeviceInfo) {
			cell.textLabel.text = Localize(@"Hide Account Info");
		}
		else {
			cell.textLabel.text = Localize(@"Show Account Info");
		}
		return cell;
	}
	
	if ([row isEqualToString:@"passwordButtonRow"]) {
		static NSString *CellIdentifier = @"PasswordCell";
		ThemedTableViewCell *cell = (ThemedTableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil) {
			cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
			cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		}
		cell.textLabel.text = Localize(@"Change Password");
		return cell;
	}
	
	return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section {
	if ([view isKindOfClass:UITableViewHeaderFooterView.class]) {
		UITableViewHeaderFooterView *headerView = (UITableViewHeaderFooterView *)view;
		headerView.textLabel.textColor  = Theme.tableHeaderTextColor;
	}
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
	NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];

	if ([sectionName isEqualToString:@"contactPhotoSection"]) {
		return [self headerView];
	}
	else {
		return nil;
	}
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
	NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
	
	if ([sectionName isEqualToString:@"deviceInfoSection"]) {
		if (self.showDeviceInfo) {
			return [self footerView].frame.size.height;
		}
	}
	else if ([sectionName isEqualToString:@"callIDSection"]) {
		if (self.hasTollFreeNumber)
			return UITableViewAutomaticDimension;
		else
			return 0.1;
	}
	else if ([sectionName isEqualToString:@"notificationSection"] && !self.hasTollFreeNumber ) {
		return 0.1;
	}
		
	return  UITableViewAutomaticDimension;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
	NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
	
	if ([sectionName isEqualToString:@"callIDSection"]) {
		if (self.hasTollFreeNumber)
			return UITableViewAutomaticDimension;
		else
			return 0.1;
	}
	else if ([sectionName isEqualToString:@"contactPhotoSection"]) {
		return self.headerView.frame.size.height;
	}
	else {
		return UITableViewAutomaticDimension;
	}
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
    NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
    
    if ([sectionName isEqualToString:@"deviceInfoSection"]) {
		if (self.showDeviceInfo)
			return [self footerView];
		else
			return nil;
    }
	else if ([sectionName isEqualToString:@"callIDSection"]) {
		if (!self.hasTollFreeNumber) {
			return [[UIView alloc] initWithFrame:CGRectMake(0, 0, 0.1, 0.1)];
		}
	}
	return nil;
}

- (UIView *)headerView
{
	if (!headerView) {
		self.headerView = [[[NSBundle mainBundle]loadNibNamed:@"PersonalInfoHeaderView" owner:self options:nil] lastObject];
	}

	headerView.backgroundColor = [UIColor clearColor];
	UIImage *headerProfileImage = nil;

	// Shift labels to the left margin if Contact Photos are disabled.  15pt from left.
	BOOL contactsEnabled = [[SCIVideophoneEngine sharedEngine] contactPhotosFeatureEnabled];
	if (!contactsEnabled) {
		self.contactImageButtonContainerView.hidden = YES;
		
		CGRect nameFrame = self.nameLabel.frame;
		nameFrame.origin.x = 15;
		self.nameLabel.frame = nameFrame;
		
		CGRect numberFrame = self.localNumberLabel.frame;
		numberFrame.origin.x = 15;
		self.localNumberLabel.frame = numberFrame;
		
		CGRect tollFreeFrame = self.tollFreeNumberLabel.frame;
		tollFreeFrame.origin.x = 15;
		self.tollFreeNumberLabel.frame = tollFreeFrame;
	
	}
	else if (self.profileImage) {
		headerProfileImage = self.profileImage;
	}
	else {
		headerProfileImage = [UIImage imageNamed:@"avatar_default"];
	}
	
	self.nameLabel.textColor = Theme.textColor;
	self.localNumberLabel.textColor = Theme.textColor;
	self.tollFreeNumberLabel.textColor = Theme.textColor;
	
	[self.contactImageButton setImage:headerProfileImage forState:UIControlStateNormal];
	self.contactImageButtonContainerView.layer.cornerRadius = self.contactImageButtonContainerView.frame.size.width / 2;
	self.contactImageButtonContainerView.layer.borderColor = [UIColor blackColor].CGColor;
	self.contactImageButtonContainerView.clipsToBounds = YES;
	if (@available(iOS 11.0, *)) {
		self.contactImageButton.imageView.accessibilityIgnoresInvertColors = YES;
	}
	
	// Inset header for iPad
	UIView *labelsView = [headerView.subviews objectAtIndex:0];
	labelsView.backgroundColor = [UIColor clearColor];
	
	return headerView;
}

-(UIView *)footerView
{
	if (!footerView) {
		//Load PersonalInfoFooterView
		self.footerView = [[[NSBundle mainBundle]loadNibNamed:@"PersonalInfoFooterView" owner:self options:nil] lastObject];
		
	}
	
	footerView.backgroundColor = [UIColor clearColor];
	UIView *labelsView = [footerView.subviews objectAtIndex:0];
	labelsView.backgroundColor = [UIColor clearColor];
	
	SCIUserAccountInfo *userAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
	self.devNameLabel.text = userAccountInfo.name;
	self.devLocalNumberLabel.text = FormatAsPhoneNumber(userAccountInfo.localNumber);
	
	if (isRingGroup) {
		self.devTollFreeNumberLabel.text = FormatAsPhoneNumber(userAccountInfo.ringGroupTollFreeNumber);
	}
	else {
		self.devTollFreeNumberLabel.text = FormatAsPhoneNumber(userAccountInfo.tollFreeNumber);
	}
	
	// Inset header for iPad
	if(iPadB) {
		float widthFactor = 0.75;
		if (UIDeviceOrientationIsPortrait(appDelegate.currentOrientation))
            widthFactor = 0.85;
		int width = self.view.frame.size.width;
		
		CGRect frame = labelsView.frame;
		frame.size.width = width * widthFactor;
		frame.origin.x = (width - frame.size.width) / 2;
		labelsView.frame = frame;
	}
	return footerView;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:section];
    NSString *sectionName = [dictionary objectForKey:@"sectionNameKey"];
    
    if ([sectionName isEqualToString:@"contactPhotoSection"]) {
        return Localize(@"Contact Photos");
    }
	if ([sectionName isEqualToString:@"notificationSection"]) {
        return Localize(@"Notifications");
    }
    if ([sectionName isEqualToString:@"callIDSection"] && self.hasTollFreeNumber) {
        return Localize(@"Caller ID");
    }
    if ([sectionName isEqualToString:@"deviceInfoSection"] && self.isRingGroup) {
        return Localize(@"Account Information");
    }
    if ([sectionName isEqualToString:@"passwordSection"]) {
        return Localize(@"Password");
    }
    
    return @"";
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    NSString *row = [array objectAtIndex:indexPath.row];
	
	if ([row isEqualToString:@"contactPhotoPrivacy"]) {
        NSMutableDictionary *contactPhotoPrivacyEditingItem = [[NSMutableDictionary alloc] init];
        [contactPhotoPrivacyEditingItem setValue:Localize(@"Share Profile Photo") forKey:kTypeListTitleKey];

        // Get currently selected value.
		SCIProfileImagePrivacyLevel privacyLevel = [[SCIVideophoneEngine sharedEngine] profileImagePrivacyLevel];
		
		if (self.updateProfilePhotoPrivacyOnExit) {
			privacyLevel = self.newPrivacyLevel;
		}
		
		NSString *levelString = [SCIProfileImagePrivacy NSStringFromSCIProfileImagePrivacyLevel:privacyLevel];
        [contactPhotoPrivacyEditingItem setValue:Localize(levelString) forKey:kTypeListTextKey];
		TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
        controller.types = [SCIProfileImagePrivacy arrayOfProfileImagePrivacyLevels];
        controller.editingItem = contactPhotoPrivacyEditingItem;
		controller.selectionBlock = ^(NSMutableDictionary *editingItem) {
			// the TypeListViewController just returned
			
			NSNumber *temp = [editingItem valueForKey:kTypeListIndexKey];
			self.updateProfilePhotoPrivacyOnExit = YES;
			self.newPrivacyLevel = (SCIProfileImagePrivacyLevel)[temp integerValue];
			[self.tableView reloadData];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"photo_sharing_changed" }];
		};
        [self.navigationController pushViewController:controller animated:YES];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"share_profile_photo" }];
        return;
    }

	
	if ([row isEqualToString:@"deviceInfoRow"]) {
		self.showDeviceInfo = !self.showDeviceInfo; // This animation needs work, consider putting everything in a single xib than can be resized
		[self.tableView beginUpdates];
		if (showDeviceInfo) {
			[self.tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationRight];
			[self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationRight];
			[self.tableView insertRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationRight];
			[self.tableView insertSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationRight];
		} else {
			[self.tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationLeft];
			[self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationLeft];
			[self.tableView insertRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationLeft];
			[self.tableView insertSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationLeft];
		}
		[self.tableView endUpdates];
	}

	if ([row isEqualToString:@"passwordButtonRow"]) {
		if([SCIAccountManager sharedManager].state.boolValue) {
			ChangePasswordViewController *passwordController = [[ChangePasswordViewController alloc] initWithStyle:UITableViewStyleGrouped];
			passwordController.isMyPhonePassword = self.isRingGroup;
			[self.navigationController pushViewController:passwordController animated:YES];
		}
		else {
			Alert(Localize(@"Not logged in"), Localize(@"personal.info.require.login"));
		}
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"change_password" }];
	}
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	NSDictionary *dictionary = [personalInfoSectionsAndRowsArray objectAtIndex:indexPath.section];
    NSArray *array = [dictionary objectForKey:@"rows"];
    NSString *row = [array objectAtIndex:indexPath.row];
	
	if	([row isEqualToString:@"email1Row"] ||
		 [row isEqualToString:@"email2Row"] )
		return YES;
	else
		return NO;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

#pragma mark - UINavigationBar delegate methods

- (IBAction)saveButtonClicked:(id)sender {
	[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"personal_info_save" }];
	[self resignFirstResponders];


	NSString *errorTitle = Localize(@"Supplied email too long.");
	NSString *errorMessage =  Localize(@"personal.info.save.email.long");
	if (self.email1.length > 255) {
		Alert(errorTitle, errorMessage);
		[self.email1TextField becomeFirstResponder];
		return;
	}
	else if (self.email2.length > 255) {
		Alert(errorTitle, errorMessage);
		[self.email2TextField becomeFirstResponder];
		return;
	}
	
	EmailFormatter *emailFormatter = [[EmailFormatter alloc] init];

	
	errorTitle = Localize(@"Invalid Email Address");
	errorMessage = Localize(@"personal.info.save.email.incalid");
	NSString *formattedEmail1 = [emailFormatter stringForObjectValue:self.email1];
	NSString *formattedEmail2 = [emailFormatter stringForObjectValue:self.email2];
	
	if (self.email1 && ![formattedEmail1 isEqualToString:self.email1]) {
		Alert(errorTitle, errorMessage);
		[self.email1TextField becomeFirstResponder];
		return;
	}
	else if (self.email2 && ![formattedEmail2 isEqualToString:self.email2]) {
		Alert(errorTitle, errorMessage);
		[self.email2TextField becomeFirstResponder];
		return;
	}
	
	if (self.signMailNotify && [formattedEmail1 length] == 0 && [formattedEmail2 length] == 0) {
		Alert(Localize(@"Missing email address"), Localize(@"personal.info.save.email.missing"));
		[self.email1TextField becomeFirstResponder];
		return;
	}
	
	// Set Caller ID property
	[[SCIVideophoneEngine sharedEngine] setCallerIDNumberTypePrimitive:self.useTollFreeCallerID?SCICallerIDNumberTypeTollFree:SCICallerIDNumberTypeLocal];
	
	SCIUserAccountInfo *oldUserAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
	NSString *mainEmail = self.email1 ?: @"";
	NSString *pagerEmail = self.email2 ?: @"";
	BOOL signMailRegistrationRequired = NO;
	if (self.signMailNotify &&
		(![mainEmail isEqualToString:oldUserAccountInfo.mainEmail] ||
		 ![pagerEmail isEqualToString:oldUserAccountInfo.pagerEmail] ||
		 oldUserAccountInfo.signMailEnabledBool == NO) ) {
			signMailRegistrationRequired = YES;
	}
	
	self.savingInProgress = YES;
	[[SCIVideophoneEngine sharedEngine] setSignMailEmailPreferencesEnabled:self.signMailNotify
														  withPrimaryEmail:mainEmail
															secondaryEmail:pagerEmail completionHandler:^(NSError *error) {
		self.savingInProgress = NO;
		if (error) {
			Alert(Localize(@"Info was not saved."), Localize(@"personal.info.save.failed"));
		} else {
			if (signMailRegistrationRequired) {
				[[SCIVideophoneEngine sharedEngine] registerSignMail];
			}
			[self.navigationController popViewControllerAnimated:YES];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"email_addresses_saved" }];
		}
	}];
}

- (IBAction)cancelButtonClicked:(id)sender {
	if(self.hasChangedFields && !self.savingInProgress) {
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Unsaved Changes")
																	   message:Localize(@"personal.info.save.unsaved.changes")
																preferredStyle:UIAlertControllerStyleAlert];
		
		[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
			[self.navigationController popViewControllerAnimated:YES];
		}]];
		
		[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Save") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			[self saveButtonClicked:nil];
		}]];
		
		[self presentViewController:alert animated:YES completion:nil];
	}
	else
		[self.navigationController popViewControllerAnimated:YES];
}

- (void)fetchUserAccountInfo {
	SCIUserAccountInfo *userAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
	
	self.isRingGroup = ((userAccountInfo.ringGroupLocalNumber && [userAccountInfo.ringGroupLocalNumber length] > 0) ||
						(userAccountInfo.ringGroupTollFreeNumber && [userAccountInfo.ringGroupTollFreeNumber length] > 0));
	
	self.hasTollFreeNumber = ((userAccountInfo.tollFreeNumber && [userAccountInfo.tollFreeNumber length] > 0) ||
							  (userAccountInfo.ringGroupTollFreeNumber && [userAccountInfo.ringGroupTollFreeNumber length] > 0));
	
	self.email1 = userAccountInfo.mainEmail;
	self.email2 = userAccountInfo.pagerEmail;
	
	// Parse name into two labels.  First name on first label, then remaining names on second label.
	NSString *fullName = (isRingGroup) ? userAccountInfo.ringGroupName : userAccountInfo.name;
	self.nameLabel.text = fullName;
	NSString *local = (isRingGroup) ? userAccountInfo.ringGroupLocalNumber : userAccountInfo.localNumber;
	self.localNumberLabel.text = FormatAsPhoneNumber(local);
	NSString *tollFree = (isRingGroup) ? userAccountInfo.ringGroupTollFreeNumber : userAccountInfo.tollFreeNumber;
	self.tollFreeNumberLabel.text = FormatAsPhoneNumber(tollFree);
	self.useTollFreeCallerID = ([[SCIVideophoneEngine sharedEngine] callerIDNumberType] == SCICallerIDNumberTypeTollFree);
	self.signMailNotify = userAccountInfo.signMailEnabledBool;
	
	__weak PersonalInfoViewController *weakSelf = self;
	[[SCIVideophoneEngine sharedEngine] loadUserAccountPhotoWithCompletion:^(NSData *data, NSError *error) {
		UIImage *image = nil;
		if (data) {
			image = [[UIImage alloc] initWithData:data];
		} else {
			image = [UIImage imageNamed:@"avatar_default"];
		}
		weakSelf.profileImage = image;
		[self.tableView reloadData];
	}];
}

#pragma mark - UISwitch delegate methods

- (void)controlValueChanged:(id)sender {
	self.hasChangedFields = YES;
	UISwitch *switchControl = (UISwitch *)sender;
	
	if(switchControl == callerIDSwitch) {
		self.useTollFreeCallerID = switchControl.isOn;
	}
	else if (switchControl == signMailNotifySwitch) {
		self.signMailNotify = switchControl.isOn;
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"notify_for_signmail" }];
	}
	else if (switchControl == self.contactPhotosEnabledSwitch) {
		[[SCIVideophoneEngine sharedEngine] setDisplayContactImagesPrimitive:switchControl.isOn];
		[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"show_contact_photos" }];
	}
}

#pragma mark - TextField delegate methods

- (void)textFieldDidBeginEditing:(UITextField *)textField {
	self.hasChangedFields = YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	if(textField == self.email1TextField)
		self.email1 = textField.text;
	else if(textField == self.email2TextField)
		self.email2 = textField.text;

}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    self.hasChangedFields = YES;
	NSUInteger newLength = [textField.text length] + [string length] - range.length;

	if (textField == self.email1TextField || textField == self.email2TextField)
		return (newLength > 255) ? NO : YES;
	else
		return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (void)setHasChangedFields:(BOOL)hasChangedFields {
	if(hasChangedFields) {
		self.navigationItem.hidesBackButton = YES;
		UIBarButtonItem *cancelButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Cancel") style:UIBarButtonItemStyleDone target:self action:@selector(cancelButtonClicked:)];
		self.navigationItem.leftBarButtonItem = cancelButton;
		
		UIBarButtonItem *saveButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Save") style:UIBarButtonItemStyleDone target:self action:@selector(saveButtonClicked:)];
		self.navigationItem.rightBarButtonItem = saveButton;
	}
	else {
		self.navigationItem.leftBarButtonItem = nil;
		self.navigationItem.hidesBackButton = NO;
	}
	
	_hasChangedFields = hasChangedFields;
}

#pragma mark - UIImagePicker

- (void) showImagePickerController:(BOOL)useCamera {
	
	if(!self.imagePicker)
		self.imagePicker = [[UIImagePickerController alloc] init];
	
	self.imagePicker.delegate = self;
	self.imagePicker.allowsEditing = YES;
	
	if(useCamera) {
		self.imagePicker.sourceType =  UIImagePickerControllerSourceTypeCamera;
		self.imagePicker.cameraDevice = UIImagePickerControllerCameraDeviceFront;
	}
	else {
		self.imagePicker.sourceType =  UIImagePickerControllerSourceTypePhotoLibrary;
		//self.imagePicker.sourceType =  UIImagePickerControllerSourceTypeSavedPhotosAlbum;
	}

	if(iPadB && !useCamera) {   // Only use PopOver when browsing for a picture on iPad
		imagePicker.modalPresentationStyle = UIModalPresentationPopover;
		imagePicker.popoverPresentationController.sourceView = self.contactImageButton;
		imagePicker.popoverPresentationController.sourceRect = self.contactImageButton.bounds;
		imagePicker.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionLeft;
	}
	else {
		self.navigationController.modalPresentationStyle = UIModalPresentationFormSheet;
	}

	dispatch_async(dispatch_get_main_queue(), ^{
		[self.navigationController presentViewController:self.imagePicker animated:YES completion:nil];
	});
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
	SCILog(@"Start");
	
	NSString *mediaType = [info objectForKey: UIImagePickerControllerMediaType];
    UIImage *originalImage, *editedImage, *imageToUse;
	
    if (CFStringCompare ((CFStringRef) mediaType, kUTTypeImage, 0) == kCFCompareEqualTo) {
        editedImage = (UIImage *) [info objectForKey: UIImagePickerControllerEditedImage];
        originalImage = (UIImage *) [info objectForKey: UIImagePickerControllerOriginalImage];
		
        if (editedImage) {
            imageToUse = editedImage;
        } else {
            imageToUse = originalImage;
        }

		[self setContactPhoto:imageToUse];
    }
	else {
		Alert(Localize(@"Image Error"), Localize(@"personal.info.image.error"));
	}
	
	[picker dismissViewControllerAnimated:YES completion:nil];
	
	self.imagePicker = nil;

}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
	[picker dismissViewControllerAnimated:YES completion:nil];
	self.imagePicker = nil;
}

- (void)useContactPicture {
	__weak PersonalInfoViewController *weakSelf = self;
	[ContactUtilities AddressBookAccessAllowedWithCallbackBlock:^(BOOL granted) {
		if (granted) {
			SCIUserAccountInfo *userAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
			NSString *myPhoneNumber;
			if(self.isRingGroup) {
				if(weakSelf.hasTollFreeNumber && self.useTollFreeCallerID)
					myPhoneNumber = userAccountInfo.ringGroupTollFreeNumber;
				else
					myPhoneNumber = userAccountInfo.ringGroupLocalNumber;
			}
			else
			{
				if(weakSelf.hasTollFreeNumber && self.useTollFreeCallerID)
					myPhoneNumber = userAccountInfo.tollFreeNumber;
				else
					myPhoneNumber = userAccountInfo.localNumber;
			}
			
			NSArray *contactsArray = [ContactUtilities findContactsByPhoneNumber:myPhoneNumber];
			
			if(contactsArray.count) {
				UIImage *imageToUse = nil;
				
				for (CNContact *contact in contactsArray) {
					imageToUse = [ContactUtilities imageForDeviceContact:contact toFitSize:CGSizeMake(320, 320)];
					
					if (imageToUse)
						break;
				}
				
				if(imageToUse)
					[self setContactPhoto:imageToUse];
				else
					Alert(Localize(@"No Contact Found"), [NSString stringWithFormat:Localize(@"personal.info.contact.not.found"), FormatAsPhoneNumber(myPhoneNumber)]);
			}
			else
				Alert(Localize(@"No Contact Found"), [NSString stringWithFormat:Localize(@"personal.info.contact.not.found"), FormatAsPhoneNumber(myPhoneNumber)]);
		}
	}];
}

- (void) setContactPhoto:(UIImage *)imageToUse {
	const CGSize newSize = CGSizeMake(320, 320);
	UIImage *resizedImage = [imageToUse imageCroppedToFitSize:newSize];
	NSData *imageData = UIImageJPEGRepresentation(resizedImage, 1);

	SCILog(@"Final image size: %f x %f", resizedImage.size.width, resizedImage.size.height);
	[[SCIVideophoneEngine sharedEngine] setUserAccountPhoto:imageData completion:^(BOOL success, NSError *error) {
		if (success) {
			self.profileImage = resizedImage;
			[self.tableView reloadData];
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationUserAccountInfoUpdated object:[SCIVideophoneEngine sharedEngine]];
		} else {
			Alert(Localize(@"Photo Change Error"), [error localizedDescription]);
		}
	}];
}

- (IBAction)contactImageEditButtonClicked:(id)sender {

	if ([[SCIVideophoneEngine sharedEngine] contactPhotosFeatureEnabled]) {
		self.profileImageActionSheet = [UIAlertController alertControllerWithTitle:nil message:nil preferredStyle:UIAlertControllerStyleActionSheet];
		[self.profileImageActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Remove Photo") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			[self setContactPhoto:nil];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"remove_profile_photo" }];
		}]];
		[self.profileImageActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Use Device Contact Photo") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			[self useContactPicture];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"use_contacts_photo" }];
		}]];
		[self.profileImageActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Take Photo") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			PersonalInfoViewController * __weak weakSelf = self;
			[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
				PersonalInfoViewController *strongSelf = weakSelf;
				if (granted && strongSelf) {
					[strongSelf showImagePickerController:YES];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"take_profile_photo" }];
				}
			}];
		}]];
		[self.profileImageActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Choose Photo") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			[self showImagePickerController:NO];
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"choose_profile_photo" }];
		}]];
		[self.profileImageActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil]];
		
		UIPopoverPresentationController *popover = self.profileImageActionSheet.popoverPresentationController;
		if (popover)
		{
			popover.sourceView = self.contactImageButtonContainerView;
			popover.sourceRect = self.contactImageButtonContainerView.bounds;
			popover.permittedArrowDirections = UIPopoverArrowDirectionAny;
		}
		
		[self presentViewController:self.profileImageActionSheet animated:YES completion:nil];
	}
}

#pragma mark - Notifications

- (void)notifyUserAccountInfoUpdated:(NSNotification *)note { //SCINotificationUserAccountInfoUpdated
	[self fetchUserAccountInfo];
	[self.tableView reloadData];
}

#pragma mark - View lifecycle

- (void)resignFirstResponders {
	if(self.email1TextField.isFirstResponder)
		[self.email1TextField resignFirstResponder];
	else if(self.email2TextField.isFirstResponder)
		[self.email2TextField resignFirstResponder];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	[self.tableView registerNib:TextInputTableViewCell.nib forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];

	self.tableView.allowsSelectionDuringEditing = YES;
	[self setEditing:YES];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyUserAccountInfoUpdated:)
												 name:SCINotificationUserAccountInfoUpdated object:[SCIVideophoneEngine sharedEngine]];
	
	[self.tableView registerNib:[UINib nibWithNibName:@"TextInputTableViewCell" bundle:nil] forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	self.tableView.rowHeight = UITableViewAutomaticDimension;
	
	[self headerView];
	[self footerView];
	[self fetchUserAccountInfo];
	[self loadTableData];
	[self.tableView reloadData];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
	return Theme.statusBarStyle;
}


- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	self.title = Localize(@"Personal Information");
	
	[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"personal_information" }];
}

- (void)dealloc {
	if (self.updateProfilePhotoPrivacyOnExit)
		[[SCIVideophoneEngine sharedEngine] setProfileImagePrivacyLevel:self.newPrivacyLevel];
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	if(self.presentedViewController == self.profileImageActionSheet && iPadB) {
		[self dismissViewControllerAnimated:NO completion:nil];
		displayContactImageActionSheet = YES;
	}
	
	__weak PersonalInfoViewController *weakSelf = self;
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
		 
		 if (weakSelf.displayContactImageActionSheet) {
			 weakSelf.displayContactImageActionSheet = NO;
			 [self performSelector:@selector(contactImageEditButtonClicked:) withObject:nil afterDelay:0.75];
		 }
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

-(NSUInteger)supportedInterfaceOrientations {
	[self resignFirstResponders];
	return UIInterfaceOrientationMaskAll;
}

@end
