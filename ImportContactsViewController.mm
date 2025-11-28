//
//  ImportContactsViewController.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "ImportContactsViewController.h"
#import "Utilities.h"
#import "AppDelegate.h"
#import "ContactUtilities.h"
#import "Alert.h"
#import "LoadingView.h"
#import "MyRumble.h"
#import "SCIContact.h"
#import "SCIContactManager.h"
#import "SCIContactManager+PhotoConvenience.h"
#import "NSRunLoop+PerformBlock.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIDefaults.h"
#import "ntouch-Swift.h"

enum {
	importPhotosSection,
	exportContactsSection,
	importFromVideophoneSection,
	ldapCredentialsSection,
	sections
};

enum {
	importContactsRow,
	exportContactsRow,
	importExportRows
};

enum {
	importFromAddressBookRow,
	importFromSorensonRow,
	importFromRows
};

enum {
	phoneNumberRow,
	PINRow,
	importButtonRow,
	importFromVideophoneRows
};

enum {
	ldapUserNameRow,
	ldapPasswordRow,
	ldapUpdateButtonRow,
	ldapRows
};

@interface ImportContactsViewController ()

@property (nonatomic, strong) NSString * importPhoneNumber;
@property (nonatomic, strong) NSString * importPIN;

@property (nonatomic, retain) UITextField *phoneNumberTextField;
@property (nonatomic, retain) UITextField *PINTextField;
@property (nonatomic, retain) UITextField *uniqueIDTextField;
@property (nonatomic, strong) UITextField *ldapUserNameTextField;
@property (nonatomic, strong) UITextField *ldapPasswordTextField;

@property (nonatomic, readonly, getter=isExportContactsEnabled) BOOL exportContactsEnabled;

@end

@implementation ImportContactsViewController

@synthesize loadingView;
@synthesize phoneNumberTextField;
@synthesize PINTextField;
@synthesize shouldContinue;
@synthesize isImporting;
@synthesize importPhotoContact;
@synthesize contactPhotosEnabled;

- (BOOL)isExportContactsEnabled {
	return [ContactExporter respondsToSelector:@selector(exportAllContactsWithCompletion:)]
		&& [SCIVideophoneEngine sharedEngine].exportContactsFeatureEnabled;
}

#pragma mark - Activity Indicator

- (void) showActivityIndicator {
	UIView *targetView = self.navigationController.view;
	targetView = self.navigationController.parentViewController.view;
	loadingView = [LoadingView loadingViewInView:targetView withMessage:Localize(@"Importing...")];
}

- (void) dismissActivityIndicator {
	if (loadingView != nil) {
		[loadingView removeView];
		loadingView = nil;
	}
}

#pragma mark - Initialization

- (id)initWithStyle:(UITableViewStyle)style {
	if ((self = [super initWithStyle:style])) {
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyImportComplete:)
													 name:SCINotificationContactListImportComplete
												   object:[SCIVideophoneEngine sharedEngine]];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyImportError:)
													 name:SCINotificationContactListImportError
												   object:[SCIVideophoneEngine sharedEngine]];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(applicationWillResignActive:)
													 name:UIApplicationWillResignActiveNotification
												   object:nil];
		
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyLDAPCredentialsInvalid:)
													 name:SCINotificationLDAPCredentialsInvalid
												   object:[SCIVideophoneEngine sharedEngine]];
		
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyLDAPServerUnavailable:)
													 name:SCINotificationLDAPServerUnavailable
												   object:[SCIVideophoneEngine sharedEngine]];
	}
	return self;
}


#pragma mark - View lifecycle

- (void) cancelImport {
    [UIApplication sharedApplication].idleTimerDisabled = NO;
	[self dismissActivityIndicator];
	Alert(Localize(@"Import Cancelled"), Localize(@"Error connecting to server."));
}

- (void) cancelImportTimeout {
    [UIApplication sharedApplication].idleTimerDisabled = NO;
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelImport) object:nil];
}

#define IMPORT_CONTACTS_TIMEOUT 120.0
- (void) startImportTimeout {
    [UIApplication sharedApplication].idleTimerDisabled = YES;
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(cancelImport) object:nil];
	[self performSelector:@selector(cancelImport) withObject:nil afterDelay:IMPORT_CONTACTS_TIMEOUT];
}

- (void)finishEditing {
	if ([phoneNumberTextField isFirstResponder])
		[phoneNumberTextField resignFirstResponder];
	else if ([PINTextField isFirstResponder])
		[PINTextField resignFirstResponder];

	[self setEditing:NO];
}

- (void)importFromVideophoneAccount {
	[self finishEditing];
	[self setEditing:YES];
	
	SCINetworkType netType = appDelegate.networkMonitor.bestInterfaceType;
	if (netType == SCINetworkTypeNone) {
		Alert(Localize(@"Network Error"), Localize(@"contacts.settings.no.network"));
	}
	else if (self.importPhoneNumber.length < 10 || self.importPhoneNumber.length > 11) {
		AlertWithTitleAndMessage(Localize(@"Invalid Phone Number"), Localize(@"login.err.number.digits"));
		[phoneNumberTextField becomeFirstResponder];
	}
	else if (!self.importPIN.length) {
		AlertWithTitleAndMessage(Localize(@"Invalid Password"), Localize(@"login.err.password.blank"));
		[PINTextField becomeFirstResponder];
	}
	else {
		[self showActivityIndicator];
		[[SCIContactManager sharedManager] importContactsForNumber:self.importPhoneNumber
														  password:self.importPIN];
	}
}

- (void)applicationWillResignActive:(NSNotification *)notification {
	SCILog(@"Start");
	PINTextField.text = @"";
	phoneNumberTextField.text = @"";
	self.importPhoneNumber = @"";
	self.importPIN = @"";
}

- (void)viewDidLoad {
	[super viewDidLoad];

	// remove previously used phone number and PIN
	self.importPhoneNumber = @"";
	self.importPIN = @"";
	[self.tableView registerNib:[UINib nibWithNibName:@"TextInputTableViewCell" bundle:nil] forCellReuseIdentifier:TextInputTableViewCell.cellIdentifier];
	[self.tableView registerNib:SectionHeaderView.nib forHeaderFooterViewReuseIdentifier:SectionHeaderView.reuseIdentifier];

	self.tableView.rowHeight = UITableViewAutomaticDimension;

	if (iPad)
		self.tabBarController.preferredContentSize = kPopoverSize;

	self.contactPhotosEnabled = [[SCIVideophoneEngine sharedEngine] contactPhotosFeatureEnabled];
	
	self.title = Localize(@"Contacts");

	self.tableView.allowsSelectionDuringEditing = YES;
	[self setEditing:YES];

}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	opQ = [[NSOperationQueue alloc] init];
	
	[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"contacts" }];
}

 - (void)viewDidAppear:(BOOL)animated {
	 [super viewDidAppear:animated];
 }

 - (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
     phoneNumberTextField.text = @"";
     PINTextField.text = @"";
     self.importPhoneNumber = @"";
     self.importPIN = @"";
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 // to update our custom header/footer views
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidBeginEditing:(UITextField *)textField {
	if (textField == phoneNumberTextField)
		textField.text = self.importPhoneNumber;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	if (textField == phoneNumberTextField) {
		// make sure the phone number starts with a 1
		NSString *temp = textField.text;
		if (temp.length == 10 && [temp characterAtIndex:0] != '1')
			temp = [@"1" stringByAppendingString:temp];
		self.importPhoneNumber = UnformatPhoneNumber(temp);
		textField.text = FormatAsPhoneNumber(temp);
	}
	else if (textField == PINTextField)
		self.importPIN = textField.text;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return sections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {

	BOOL ldapEnabled = [[SCIVideophoneEngine sharedEngine] LDAPDirectoryEnabled];
	BOOL ldapCredRequired = [[[SCIVideophoneEngine sharedEngine] LDAPServerRequiresAuthentication] boolValue];
	switch (section) {
		case importPhotosSection:
			return contactPhotosEnabled ? importFromRows : 0;
		case exportContactsSection:
			return self.exportContactsEnabled ? importExportRows : 0;
		case importFromVideophoneSection:
			return importFromVideophoneRows;
		case ldapCredentialsSection:
			return (ldapCredRequired && ldapEnabled) ? ldapRows : 0;

		default:
			return 0;
	}
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	switch (section) {
		case importFromVideophoneSection: {
			return UITableViewAutomaticDimension;
		} break;
		case importPhotosSection:
		case exportContactsSection:
		case ldapCredentialsSection: {
			return -1;
		} break;
		default:
			return -1;
	}
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForHeaderInSection:(NSInteger)section {
	return 50.0;
}

- (UIView *) tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	if (section == importFromVideophoneSection) {
		SectionHeaderView *header = (SectionHeaderView *)[self.tableView dequeueReusableHeaderFooterViewWithIdentifier:SectionHeaderView.reuseIdentifier];
		header.textLabel.text = Localize(@"Import Videophone Contacts");
		header.subTextLabel.text = Localize(@"contacts.settings.import.video.header");
		return header;
	}
	else {
		return nil;
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {

	NSString *directoryName = [[SCIVideophoneEngine sharedEngine] LDAPDirectoryDisplayName];
	
	switch (section) {
		case importPhotosSection: {
			return contactPhotosEnabled ? Localize(@"Import Photos") : nil;
		}
		case exportContactsSection: {
			return self.exportContactsEnabled ? Localize(@"Import/Export Contacts") : nil;
		}
		case importFromVideophoneSection: {
			return nil;
		}
		case ldapCredentialsSection:
			return [self shouldShowLDAPSection] ? directoryName : nil;
		default:
			return nil;
	}
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
	switch (section) {
		case importFromVideophoneSection: {
			return;
		} break;
		case importPhotosSection:
		case exportContactsSection:
		case ldapCredentialsSection: {
			UITableViewHeaderFooterView *headerView = (UITableViewHeaderFooterView *)view;
			headerView.textLabel.textColor  = Theme.tableHeaderGroupedTextColor;
		} break;
		default:
			return;
	}
}

- (BOOL)shouldShowLDAPSection
{
	BOOL ldapEnabled = [[SCIVideophoneEngine sharedEngine] LDAPDirectoryEnabled];
	BOOL ldapCredRequired = [[[SCIVideophoneEngine sharedEngine] LDAPServerRequiresAuthentication] boolValue];

	return (ldapEnabled && ldapCredRequired);
}

// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
		
	// Configure the cell...
	switch (indexPath.section) {
			
		case importPhotosSection: {
			switch (indexPath.row) {
				case importFromAddressBookRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Import Photos From Contacts");
					return cell;
				} break;
				case importFromSorensonRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Import Photos From Sorenson");
					return cell;
				} break;
			}
		} break;
			
		case exportContactsSection: {
			switch (indexPath.row) {
				case exportContactsRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Export Contacts to Device");
					return cell;
				} break;
				case importContactsRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Import Contact from Device");
					return cell;
				} break;
			}
		} break;
			
		case importFromVideophoneSection: {

			switch (indexPath.row) {
				case phoneNumberRow: {
					TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
					phoneNumberTextField = textCell.textField;
					phoneNumberTextField.delegate = self;
					textCell.textLabel.text = Localize(@"Phone Number");
					textCell.textField.placeholder = Localize(@"required");
					textCell.textField.secureTextEntry = NO;
					textCell.textField.text = FormatAsPhoneNumber(self.importPhoneNumber);
					textCell.textField.keyboardType = UIKeyboardTypePhonePad;
					textCell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
					textCell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
					return textCell;
					break;
				}
				case PINRow: {
					TextInputTableViewCell *textCell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier];
					PINTextField = textCell.textField;
					PINTextField.delegate = self;
					textCell.textLabel.text = Localize(@"Password");
					textCell.textField.placeholder = Localize(@"required");
					textCell.textField.secureTextEntry = YES;
					textCell.textField.text = FormatAsPhoneNumber(self.importPhoneNumber);
					textCell.textField.keyboardType = UIKeyboardTypeNamePhonePad;
					textCell.textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
					textCell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
					textCell.textField.returnKeyType = UIReturnKeyDone;
					return textCell;
					break;
				}
				case importButtonRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Import From Videophone Account");
					return cell;
				}
			}
			return [[UITableViewCell alloc] init];
		} break;
		
		case ldapCredentialsSection: {
			switch (indexPath.row) {
				case ldapUserNameRow: {
					TextInputTableViewCell *cell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
					cell.textField.text = @"";
					cell.textField.enabled = NO;
					cell.selectionStyle = UITableViewCellSelectionStyleNone;
					cell.textLabel.text = Localize(@"Username");
					NSDictionary *ldapDict = [[SCIDefaults sharedDefaults] LDAPUsernameAndPasswordForUsername:[SCIVideophoneEngine sharedEngine].username];
					cell.textField.text = [ldapDict valueForKey:@"LDAPUsername"];
					return cell;
				}
				case ldapPasswordRow: {
					TextInputTableViewCell *cell = (TextInputTableViewCell *)[tableView dequeueReusableCellWithIdentifier:TextInputTableViewCell.cellIdentifier forIndexPath:indexPath];
					cell.textField.text = @"";
					cell.textField.enabled = NO;
					cell.selectionStyle = UITableViewCellSelectionStyleNone;
					cell.textLabel.text = Localize(@"Password");
					NSDictionary *ldapDict = [[SCIDefaults sharedDefaults] LDAPUsernameAndPasswordForUsername:[SCIVideophoneEngine sharedEngine].username];
					NSString *pwd = [ldapDict valueForKey:@"LDAPPassword"];
					cell.textField.text = [self createObfuscatedPasswordWithLength:pwd.length];
					return cell;
				}
				case ldapUpdateButtonRow: {
					ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ActionCell"];
					if (cell == nil) {
						cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ActionCell"];
					}
					cell.textLabel.text = Localize(@"Update Login Information");
					return cell;
				}
			}
		}

		default:
			break;
	}
	return nil;
}

- (NSString *)createObfuscatedPasswordWithLength:(NSInteger)length {
	NSString *result = @"";
	for (int i = 0; i < length; i++) {
		result = [result stringByAppendingString:@"\u2022"];
	}
	return result;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];

	switch (indexPath.section) {
		case importPhotosSection:
			switch (indexPath.row) {
				case importFromAddressBookRow: {
					SCILog(@"Import from addressbook");
					[self importFromAddressBook];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"import_contact_photos" }];
				} break;
				case importFromSorensonRow: {
					SCILog(@"Import from Sorenson");
					[self importFromSorenson];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"import_sorenson_photos" }];
				} break;
			}
			break;
		case exportContactsSection:
			switch (indexPath.row) {
				case exportContactsRow: {
					SCILog(@"Export to addressbook");
					[ContactUtilities AddressBookAccessAllowedWithCallbackBlock:^(BOOL granted) {
						if (granted) {
							[ContactExporter exportAllContactsWithCompletion:^(NSError * _Nullable error) {
								if (error) {
									AlertWithError(error);
								}
								else {
									AlertWithMessage(Localize(@"Export Successful"));
								}
							}];
						}
					}];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"export_device_contacts" }];
				} break;
				case importContactsRow: {
					SCILog(@"Import from addressbook");
					CNContactPickerViewController *contactPicker = [[CNContactPickerViewController alloc] init];
					contactPicker.delegate = self;
					contactPicker.modalPresentationStyle = UIModalPresentationFormSheet;
					[self presentViewController:contactPicker animated:YES completion:nil];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"import_device_contacts" }];
				} break;
			}
			break;
		case importFromVideophoneSection:
			switch (indexPath.row) {
				case importButtonRow:
					if (appDelegate.networkMonitor.bestInterfaceType == SCINetworkTypeNone) {
						Alert(Localize(@"Network Error"), Localize(@"contacts.settings.no.network"));
					}
					else {
						[self importFromVideophoneAccount];
					}
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"import_from_vp" }];
					break;
				default:
					break;
			}
		break;
		case ldapCredentialsSection:
			switch (indexPath.row) {
				case ldapUpdateButtonRow: {
					NSDictionary *ldapDict = [[SCIDefaults sharedDefaults] LDAPUsernameAndPasswordForUsername:[SCIVideophoneEngine sharedEngine].username];
					
					UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Login")
																				   message:[NSString stringWithFormat:Localize(@"Enter login information")]
																			preferredStyle:UIAlertControllerStyleAlert];
					
					[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
						textField.placeholder = Localize(@"Username");
						textField.text = [ldapDict objectForKey:@"LDAPUsername"];
					}];
					[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
						textField.placeholder = Localize(@"Password");
						textField.secureTextEntry = YES;
						textField.text = [ldapDict objectForKey:@"LDAPPassword"];
					}];
					
					[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil]];
					[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Save") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
						NSString *userName = ((UITextField *)[alert.textFields objectAtIndex:0]).text;
						NSString *password = ((UITextField *)[alert.textFields objectAtIndex:1]).text;
						
						if (!userName.length) {
							Alert(Localize(@"Error"), Localize(@"Username cannot be blank."));
							return;
						}
						
						// Store new Username and Password in iOS KeyChain
						[[SCIDefaults sharedDefaults]setLDAPUsername:userName andLDAPPassword:password forUsername:[SCIVideophoneEngine sharedEngine].username];
						
#ifdef stiLDAP_CONTACT_LIST
						NSMutableDictionary *ldapCredentials = [[NSMutableDictionary alloc] init];
						[ldapCredentials setValue:userName forKey:@"LDAPUsername"];
						[ldapCredentials setValue:password forKey:@"LDAPPassword"];
						[[SCIVideophoneEngine sharedEngine] setLDAPCredentials:ldapCredentials completion:^{
							[self.tableView reloadData];
						}];
#endif
					}]];
					
					[self presentViewController:alert animated:YES completion:nil];
					[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"ldap_credentials" }];
				} break;
					
				default:
					break;
			}
		default:
			break;
	}
}

- (void)importFromAddressBook {
	[ContactUtilities AddressBookAccessAllowedWithCallbackBlock:^(BOOL granted) {
		if (granted) {
			[self showActivityIndicator];
			
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				__block BOOL completed = YES;
				self.shouldContinue = YES;
				NSUInteger importCount = [[SCIContactManager sharedManager] setPhotoIdentifierToFacebookForAllContactsIfFacebookPhotoFoundWithContactBlock:^(SCIContact *contact, BOOL *bail) {
					
					if (bail) *bail = !self.shouldContinue;
					
					if (self.shouldContinue) {
						[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[NSRunLoopCommonModes] block:^{
							self.importPhotoContact = contact;
						}];
					} else {
						completed = NO;
					}
				}];
				[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[NSRunLoopCommonModes] block:^{
					[self dismissActivityIndicator];
					
					NSString *msg;
					
					if (importCount == 1) {
						msg = [NSString stringWithFormat:Localize(@"Imported %zu contact photo."), (unsigned long)importCount];
					} else {
						msg = [NSString stringWithFormat:Localize(@"Imported %zu contact photos."), (unsigned long)importCount];
					}
					
					if (completed) {
						Alert(Localize(@"Import completed."), msg);
					} else {
						Alert(Localize(@"Import cancelled."), msg);
					}
				}];
			});
		}
	}];
}

- (void)importFromSorenson
{
	if (appDelegate.networkMonitor.bestInterfaceType == SCINetworkTypeNone) {
		Alert(Localize(@"Network Error"), Localize(@"contacts.settings.no.network"));
		return;
	}
	
	[self showActivityIndicator];
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		__block BOOL completed = YES;
		[[SCIContactManager sharedManager] setPhotoIdentifierToSorensonForAllContactsIfSorensonPhotoFoundWithContactBlock:^(SCIContact *contact, BOOL *bail) {
			self.shouldContinue = YES;
			if (bail) *bail = !self.shouldContinue;
			if (self.shouldContinue) {
				[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[NSRunLoopCommonModes] block:^{
					[self startImportTimeout];
					self.importPhotoContact = contact;
				}];
			} else {
				completed = NO;
			}
		}
	   completion:^(NSUInteger importCount) {
		   [NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[NSRunLoopCommonModes] block:^{
			   [self cancelImportTimeout];
			   [self dismissActivityIndicator];
			   
			   NSString *msg;
			   
			   if (importCount == 1) {
				   msg = [NSString stringWithFormat:Localize(@"Imported %zu contact photo."), (unsigned long)importCount];
			   } else {
				   msg = [NSString stringWithFormat:Localize(@"Imported %zu contact photos."), (unsigned long)importCount];
			   }
			   
			   if (completed) {
				   Alert(Localize(@"Import completed."), msg);
			   } else {
				   Alert(Localize(@"Import cancelled."), msg);
			   }
		   }];
	   }];
	});
}

- (BOOL)importDeviceContact:(CNContact *)deviceContact
{
	BOOL ableToImport = YES;
	SCIContact *contact = [[SCIContact alloc] init];
	NSMutableArray *unknownPhones = [[NSMutableArray alloc] init];
	NSMutableArray *phoneNumberArray = [[NSMutableArray alloc] init];
	NSMutableArray *phoneLabelArray = [[NSMutableArray alloc] init];
	
	for (CNLabeledValue *labeledValue in deviceContact.phoneNumbers) {
		// Label
		NSString *label = [CNLabeledValue localizedStringForLabel:labeledValue.label];
		if (!label)
		{
			label = @"phone";
		}
		CNPhoneNumber *phonenumber = labeledValue.value;
		[phoneLabelArray addObject:label];

		// Phone Number
		NSString *tempPhone = UnformatPhoneNumber(phonenumber.stringValue);
		if (tempPhone.length == 10 && ![[tempPhone substringToIndex:1] isEqualToString:@"1"]) {
			tempPhone = [@"1" stringByAppendingString:tempPhone];
		}
		[phoneNumberArray addObject:tempPhone];
	}
	
	// Look for Home/Work/Mobile first
	for (int i = 0; i < phoneNumberArray.count; i++)
	{
		NSString *label = [phoneLabelArray objectAtIndex:i];
		NSString *phone = UnformatPhoneNumber([phoneNumberArray objectAtIndex:i]);
		
		if (![[SCIVideophoneEngine sharedEngine] phoneNumberIsValid:phone])
			continue;
		
		if ([label isEqualToString:@"home"] && !contact.homePhone.length) {
			contact.homePhone = phone;
		}
		else if ([label isEqualToString:@"main"] && !contact.homePhone.length) {
			contact.homePhone = phone;
		}
		else if ([label isEqualToString:@"work"] && !contact.workPhone.length) {
			contact.workPhone = phone;
		}
		else if ([label isEqualToString:@"mobile"] && !contact.cellPhone.length) {
			contact.cellPhone = phone;
		}
		else {
			[unknownPhones addObject:phone];
		}
	}
	
	// Put remaining phone numbers in whatever is left
	for (NSString *string in unknownPhones) {
		if (!contact.homePhone.length)
			contact.homePhone = string;
		else if (!contact.workPhone.length)
			contact.workPhone = string;
		else if (!contact.cellPhone.length)
			contact.cellPhone = string;
	}
	
	if (!contact.homePhone.length &&
		!contact.workPhone.length &&
		!contact.cellPhone.length) {
		Alert(Localize(@"Can't Import"), Localize(@"contacts.settings.no.valid.numbers"));
		ableToImport = NO;
	}
	
	// Check if duplicate
	if ([[SCIContactManager sharedManager] contactExistsLikeContact:contact]) {
		Alert(Localize(@"Duplicate Contact"), Localize(@"contacts.settings.duplicate.contact"));
		ableToImport = NO;
	}
	
	// Save new Contact
	if (ableToImport) {
		UIImage *contactPhoto = [ContactUtilities imageForDeviceContact:deviceContact toFitSize:CGSizeMake(320, 320)];
		if (contactPhoto) {
			contact.photoIdentifier = PhotoManagerCustomImageIdentifier();
			[[PhotoManager sharedManager] storeCustomImage:contactPhoto forContact:contact];
		}
		contact.name = [[CNContactFormatter stringFromContact:deviceContact style:CNContactFormatterStyleFullName] trimmingUnsupportedCharacters];
		[[SCIContactManager sharedManager] addContact:contact];
		Alert(Localize(@"Import Contact"), Localize(@"Complete"));
	}

	return ableToImport;
}

- (IBAction)cancel:(id)sender {
	self.shouldContinue = NO;
}

#pragma mark - CNContactPickerViewController Delegate

- (void)contactPickerDidCancel:(CNContactPickerViewController *)picker {
	SCILog(@"");
}

- (void)contactPicker:(CNContactPickerViewController *)picker didSelectContact:(CNContact *)contact {
	[picker dismissViewControllerAnimated:YES completion:^{
		[self importDeviceContact:contact];
	}];
}

#pragma mark - Notifications

- (void)notifyImportComplete:(NSNotification *)note //SCINotificationContactListImportComplete
{
	[self dismissActivityIndicator];
	NSUInteger count = [(NSNumber *)[[note userInfo] objectForKey:SCINotificationKeyCount] unsignedIntegerValue];
#ifdef stiTESTFLIGHT_ENABLED
	[TestFlight passCheckpoint:@"IMPORTED_CONTACTS"];
#endif
	AlertWithTitleAndMessage(Localize(@"Contacts imported."), [NSString stringWithFormat:Localize(@"%lu contacts were imported."), (unsigned long)count]);
}

- (void)notifyImportError:(NSNotification *)note //SCINotificationContactListImportError
{
	[self dismissActivityIndicator];
	AlertWithTitleAndMessage(Localize(@"Can't import contacts."), Localize(@"contacts.settings.sci.import.error"));
}

- (void)notifyLDAPCredentialsInvalid:(NSNotification *)note // SCINotificationLDAPCredentialsInvalid
{
	SCILog(@"Start");
	NSString *directoryName = [[SCIVideophoneEngine sharedEngine] LDAPDirectoryDisplayName];
	NSString *message = [NSString stringWithFormat:Localize(@"contacts.settings.sci.invalid.creds"), directoryName.length ? directoryName : @"Corporate Directory"];
	
	Alert(Localize(@"Directory Login Failed"), message);
}

- (void)notifyLDAPServerUnavailable:(NSNotification *)note //SCINotificationLDAPServerUnavailable
{
	SCILog(@"Start");
	NSString *directoryName = [[SCIVideophoneEngine sharedEngine] LDAPDirectoryDisplayName];
	NSString *message = [NSString stringWithFormat:Localize(@"contacts.settings.sci.no.DAP.server"), directoryName.length ? directoryName : @"Corporate"];
	
	Alert(Localize(@"Directory Server Unavailable"), message);
}

#pragma mark - Memory management

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
