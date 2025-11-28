//
//  FCCUserRegistrationViewController.m
//  ntouch
//
//  Created by J.R. Blackham on 8/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "FCCUserRegistrationViewController.h"
#import "Utilities.h"
#import "AppDelegate.h"
#import "SCIContactManager.h"
#import "Alert.h"
#import "SCIVideophoneEngine.h"
#import "SCIUserAccountInfo+Convenience.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIAccountManager.h"
#import "ntouch-Swift.h"

@interface FCCUserRegistrationViewController ()

@property (nonatomic) NSString *SSN;
@property (nonatomic) NSDate *DOB;
@property (nonatomic) BOOL hasSSN;

@property (nonatomic, readonly) BOOL submitButtonIsEnabled;
@property (nonatomic) NSDateFormatter *dateFormatter;
@property (nonatomic, strong) UIDatePicker *datePickerView;

@property (nonatomic) NSString *fullLegalName;
@property (nonatomic) NSString *localNumber;

@property (nonatomic, strong) IBOutlet UILabel *nameLabel;
@property (nonatomic, strong) IBOutlet UILabel *name;
@property (nonatomic, strong) IBOutlet UILabel *numberLabel;
@property (nonatomic, strong) IBOutlet UILabel *number;
@property (nonatomic, strong) IBOutlet UILabel *messageLabel;
@property (nonatomic, strong) IBOutlet UIView *contentView;
@property (nonatomic, strong) IBOutlet UITableView *tableView;
@property (nonatomic, strong) IBOutlet UIScrollView *scrollView;

@property (nonatomic, strong) UITextField *activeField;
@property (nonatomic, strong) UIViewController *datePickerPopoverController;


@end

@implementation FCCUserRegistrationViewController


enum {
	userRegistrationDataSSNSection,
	userRegistrationDataDOBSection,
	tableSections
};

enum {
	SSNRow, 
	hasSSNRow,
	userRegistrationDataSSNSectionRows
};

enum {
	DOBRow,
	userRegistrationDataDOBSectionRows
};

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
		self.hasSSN = YES;
		self.dateFormatter = [[NSDateFormatter alloc] init];
		[self.dateFormatter setDateFormat:@"MM/dd/yyyy"];
    }
    return self;
}

- (void)applyTheme
{
	self.view.backgroundColor = Theme.backgroundColor;
	self.tableView.backgroundColor = UIColor.clearColor;
	self.nameLabel.textColor = Theme.textColor;
	self.name.textColor = Theme.textColor;
	self.numberLabel.textColor = Theme.textColor;
	self.number.textColor = Theme.textColor;
	self.messageLabel.textColor = Theme.textColor; 
	self.datePickerView.backgroundColor = Theme.backgroundColor;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	
	if ([self respondsToSelector:@selector(edgesForExtendedLayout)])
        self.edgesForExtendedLayout = UIRectEdgeNone;

	SCIUserAccountInfo *userAccountInfo = [[SCIVideophoneEngine sharedEngine] userAccountInfo];
	if (userAccountInfo) {
		self.name.text = userAccountInfo.name;
		NSString *localNumber = userAccountInfo.localNumber;
		self.number.text = [localNumber length] > 0 ? FormatAsPhoneNumber(localNumber) : @"None";
	}
	self.nameLabel.text = Localize(@"Name");
	self.numberLabel.text = Localize(@"Local Number");
	self.title = Localize(@"FCC Registration");
	self.navigationController.navigationBarHidden = NO;
	
	UIBarButtonItem *submitButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Submit") style:UIBarButtonItemStyleDone target:self action:@selector(submit:)];
	self.navigationItem.rightBarButtonItem = submitButton;
	
	UIBarButtonItem *cancelButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Cancel") style:UIBarButtonItemStyleDone target:self action:@selector(logOut:)];
	self.navigationItem.leftBarButtonItem = cancelButton;
	
	self.messageLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
	if(iPadB) {
		[self.messageLabel sizeToFit];
	}
	else if ([UIScreen mainScreen].bounds.size.height > 480) {
		[self.messageLabel sizeToFit];
		CGRect tmpFrame = self.messageLabel.frame;
		tmpFrame.origin.y += 10;
		self.messageLabel.frame = tmpFrame;
		tmpFrame = self.contentView.frame;
		tmpFrame.size.height = 504;
		self.contentView.frame = tmpFrame;
	}
	
	if ([UIScreen mainScreen].bounds.size.height == 480) {
		CGRect tmpFrame = self.tableView.frame;
		tmpFrame.size.height += 22;
		tmpFrame.origin.y -= 22;
		self.tableView.frame = tmpFrame;
	}
	
	self.tableView.delegate = self;
	self.tableView.dataSource = self;
	
	self.scrollView.contentSize = self.contentView.bounds.size;
	
	
	[self createDatePicker];



	
	[self registerForKeyboardNotifications];
	
	[self applyTheme];
	
	[self.tableView registerNib:SideBySideTextInputTableViewCell.nib forCellReuseIdentifier:SideBySideTextInputTableViewCell.cellIdentifier];
	self.tableView.estimatedRowHeight = 66.0;
	self.tableView.rowHeight = UITableViewAutomaticDimension;
}

- (IBAction)logOut:(id)sender {
	/* open an alert with OK and Cancel buttons */
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:Localize(@"Logout now?") message:Localize(@"fcc.logout.message") preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil]];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"Logout") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
		[self dismissViewControllerAnimated:YES completion:^ {
			[[SCIAccountManager sharedManager] clientReauthenticate];
		}];
	}]];
	
	[self presentViewController:alert animated:YES completion:nil];
}

- (IBAction)submit:(id)sender {
	if (!g_NetworkConnected) {
		AlertWithTitleAndMessage(Localize(@"Network Error"), Localize(@"fcc.network.error.msg"));
	} else {
		
		[self.activeField resignFirstResponder];
		
		if (self.SSN == nil) {
			self.SSN = @"";
		}
		
		NSCharacterSet *numericOnlyStringSet = [NSCharacterSet decimalDigitCharacterSet];
		NSCharacterSet *SSNStringSet = [NSCharacterSet characterSetWithCharactersInString:self.SSN];
		
		if (((self.hasSSN == NO) || [self.SSN length] == 4) && ([numericOnlyStringSet isSupersetOfSet:SSNStringSet]) && ([self.DOB compare:[NSDate date]] == NSOrderedAscending)) {
			[[SCIVideophoneEngine sharedEngine] saveUserSSN:self.SSN DOB:[self.dateFormatter stringFromDate:self.DOB] andHasSSN:self.hasSSN  completionHandler:^(NSError *error) {
				if (error) {
					AlertWithTitleAndMessage(Localize(@"fcc.submit.error.title"), Localize(@"fcc.submit.error.msg"));
				} else {
					[[SCIVideophoneEngine sharedEngine] loadUserAccountInfo];
				}
			}];
			
			[self dismissViewControllerAnimated:YES completion:nil];
			
		} else {
			if (self.hasSSN != NO && [self.SSN length] != 4 || (![numericOnlyStringSet isSupersetOfSet:SSNStringSet])) {
				AlertWithTitleAndMessage(Localize(@"Invalid SSN"), Localize(@"fcc.error.invalid.ssn.msg"));
			}
			if ([self.DOB compare:[NSDate date]] != NSOrderedAscending) {
				AlertWithTitleAndMessage(Localize(@"Birthday Invalid"), Localize(@"fcc.error.invalid.bday"));
				
			}
		}
	}
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark - Notifications

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	return (textField.text.length + string.length <= 4);
}

- (void)registerForKeyboardNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWasShown:)
												 name:UIKeyboardDidShowNotification object:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillBeHidden:)
												 name:UIKeyboardWillHideNotification object:nil];
	
}

// Called when the UIKeyboardDidShowNotification is sent.
- (void)keyboardWasShown:(NSNotification*)aNotification
{
	if(!iPadB) {
		[self addButtonToKeyboard];
	}
    NSDictionary* info = [aNotification userInfo];
    CGSize kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
	
    UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
    [self.scrollView setContentInset:contentInsets];
    [self.scrollView setScrollIndicatorInsets:contentInsets];
	
    // If active text field is hidden by keyboard, scroll it so it's visible
    // Your application might not need or want this behavior.
    CGRect aRect = self.contentView.frame;
    aRect.size.height -= kbSize.height;
    if (!CGRectContainsPoint(aRect, [self.contentView convertPoint:self.activeField.frame.origin fromView:self.activeField]) ) {
        CGPoint scrollPoint = CGPointMake(0.0, [self.contentView convertPoint:self.activeField.frame.origin fromView:self.activeField].y-(kbSize.height)+55);
        [self.scrollView setContentOffset:scrollPoint animated:YES];
    }
}

- (IBAction)dismissKeyboard:(id)sender
{
    [self.activeField resignFirstResponder];
}

// Called when the UIKeyboardWillHideNotification is sent
- (void)keyboardWillBeHidden:(NSNotification*)aNotification
{
    UIEdgeInsets contentInsets = UIEdgeInsetsZero;
    [self.scrollView setContentInset:contentInsets];
    [self.scrollView setScrollIndicatorInsets:contentInsets];
}

- (void)textFieldDidBeginEditing:(UITextField *)textField
{
    self.activeField = textField;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
	self.SSN = textField.text;

    self.activeField = nil;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	self.SSN = textField.text;
	[textField resignFirstResponder];
	return YES;
}

- (void)addButtonToKeyboard {
	// create custom button
	UIButton *doneButton = [UIButton buttonWithType:UIButtonTypeCustom];
	doneButton.frame = CGRectMake(0, 163, 106, 53);
	doneButton.adjustsImageWhenHighlighted = NO;
	if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 3.0) {
		[doneButton setImage:[UIImage imageNamed:@"DoneUp3.png"] forState:UIControlStateNormal];
		[doneButton setImage:[UIImage imageNamed:@"DoneDown3.png"] forState:UIControlStateHighlighted];
	} else {
		[doneButton setImage:[UIImage imageNamed:@"DoneUp.png"] forState:UIControlStateNormal];
		[doneButton setImage:[UIImage imageNamed:@"DoneDown.png"] forState:UIControlStateHighlighted];
	}
	[doneButton addTarget:self action:@selector(doneButton:) forControlEvents:UIControlEventTouchUpInside];
	// locate keyboard view
	UIWindow* tempWindow = [[[UIApplication sharedApplication] windows] objectAtIndex:1];
	UIView* keyboard;
	for(int i=0; i<[tempWindow.subviews count]; i++) {
		keyboard = [tempWindow.subviews objectAtIndex:i];
		// keyboard found, add the button
		if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 3.2) {
			if([[keyboard description] hasPrefix:@"<UIPeripheralHost"] == YES)
				[keyboard addSubview:doneButton];
		}
	}
}

- (void)doneButton:(id)sender {
	NSLog(@"doneButton");
	self.SSN = self.activeField.text;
    [self.activeField resignFirstResponder];
}


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return tableSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch (section) {
        case userRegistrationDataSSNSection:
            return userRegistrationDataSSNSectionRows;
            break;
		case userRegistrationDataDOBSection:
            return userRegistrationDataDOBSectionRows;
            break;
        default:
            return 0;
            break;
    }
    return 0;
}


#define TEXTFIELD_TAG 1
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *CellIdentifier = @"FCCUserRegistrationCell";
	SideBySideTextInputTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:SideBySideTextInputTableViewCell.cellIdentifier];
	if (cell == nil) {
		cell = [[SideBySideTextInputTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
	}
	
    switch (indexPath.section) {
        case userRegistrationDataSSNSection:
            switch (indexPath.row) {
                case SSNRow:
					cell.selectionStyle = UITableViewCellSelectionStyleNone;
					
					cell.textLabel.text = Localize(@"fcc.prompt.lastfour");
					
					cell.textField.keyboardType = UIKeyboardTypeNumberPad;
					cell.textField.returnKeyType = UIReturnKeyDone;
					cell.textField.delegate = self;
					cell.textField.clearButtonMode = UITextFieldViewModeNever;
					cell.textField.autocorrectionType = UITextAutocorrectionTypeNo;
					cell.textField.attributedPlaceholder = [[NSAttributedString alloc] initWithString:@"XXXX" attributes:@{NSForegroundColorAttributeName: [Theme.tableCellTextColor colorWithAlphaComponent:0.5]}];
					cell.textField.secureTextEntry = YES;
					cell.textField.tag = TEXTFIELD_TAG;
					cell.textField.enabled = self.hasSSN;
					cell.textField.text = self.SSN;
					
					cell.accessoryType = UITableViewCellAccessoryNone;
                    break;
                case hasSSNRow:
                    cell.textLabel.text = Localize(@"I do not have a SSN");
					cell.accessoryType = self.hasSSN ? UITableViewCellAccessoryNone : UITableViewCellAccessoryCheckmark;
					
					cell.textField.hidden = YES;
					cell.textField.enabled = NO;
					cell.textField.text = @"";
                    break;
                default:
                    break;
            }
            break;
        case userRegistrationDataDOBSection:
            switch (indexPath.row) {
                case DOBRow:
                    cell.textLabel.text = Localize(@"Birthday");
					cell.textField.hidden = NO;
					cell.textField.enabled = NO;
					cell.textField.text = self.DOB ? [self.dateFormatter stringFromDate:self.DOB] : Localize(@"MM/DD/YYYY");
					cell.textField.secureTextEntry = NO;
					
					cell.accessoryType = UITableViewCellAccessoryNone;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    
    return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
    switch (indexPath.section) {
        case userRegistrationDataSSNSection:
			switch (indexPath.row) {
				case SSNRow:
					self.hasSSN = YES;
					break;
				case hasSSNRow:
					self.hasSSN = !self.hasSSN;
					break;
				default:
					break;
			}
            break;
		case userRegistrationDataDOBSection:
            if(indexPath.row == DOBRow){
				if(iPadB) {
					UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
					self.datePickerPopoverController.modalPresentationStyle = UIModalPresentationPopover;
					self.datePickerPopoverController.popoverPresentationController.sourceView = cell;
					self.datePickerPopoverController.popoverPresentationController.sourceRect = cell.detailTextLabel.frame;
					self.datePickerPopoverController.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionRight;
					[self presentViewController:self.datePickerPopoverController animated:YES completion:nil];
					self.datePickerView.hidden = NO;

				} else {
					
					if (self.datePickerView.hidden) {
						CGSize contentSize = self.datePickerView.frame.size;
						contentSize.height += self.scrollView.contentSize.height;
						self.scrollView.contentSize = contentSize;
						self.datePickerView.hidden = NO;
					}
					if (self.scrollView.contentSize.height > self.view.window.frame.size.height) {
						CGPoint scrollPoint = CGPointMake(0.0, (self.datePickerView.bounds.size.height-self.scrollView.contentInset.top));
						[self.scrollView setContentOffset:scrollPoint animated:YES];
					}
				}
            }
            break;
        default:
            break;
    }
    
    [self.tableView reloadData];
}

#pragma mark - UIPickerView - Date/Time

- (void)createDatePicker
{
	_datePickerView = [[UIDatePicker alloc] initWithFrame:CGRectZero];
	
	self.datePickerView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    
    self.datePickerView.datePickerMode = UIDatePickerModeDate;
	[self.datePickerView addTarget:self action:@selector(dateChanged:) forControlEvents:UIControlEventValueChanged];
	
	// note we are using CGRectZero for the dimensions of our picker view,
	// this is because picker views have a built in optimum size,
	// you just need to set the correct origin in your view.
	//
    [self.datePickerView sizeToFit];
	CGRect tmpFrame = self.datePickerView.frame;
	tmpFrame.origin.y = self.scrollView.contentSize.height;
	tmpFrame.origin.x = (self.parentViewController.view.frame.size.width - tmpFrame.size.width) / 2;
	self.datePickerView.frame = tmpFrame;

	// add this picker to our view controller, initially hidden
	self.datePickerView.hidden = YES;
	[self.datePickerView setValue:Theme.textColor forKey:@"textColor"];
	
	if(iPadB) {
		self.datePickerPopoverController = [[UIViewController alloc] init]; //ViewController

		UIView *popoverView = [[UIView alloc] initWithFrame:self.datePickerView.bounds];   //view
		popoverView.backgroundColor = [UIColor clearColor];

		[popoverView addSubview:self.datePickerView];
		[popoverView sizeToFit];
		self.datePickerPopoverController.preferredContentSize = self.datePickerView.bounds.size;
		self.datePickerPopoverController.view = popoverView;
	}
	else {
		[self.scrollView addSubview:self.datePickerView];
	}
}

- (void)dateChanged:(id)sender {
	self.DOB = self.datePickerView.date;
	[self.tableView reloadData];
}

@end
