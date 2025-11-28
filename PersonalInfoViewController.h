//
//  PersonalInfoViewController
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//


#import <UIKit/UIKit.h>
#import "SCITableViewController.h"
#import "SCIProfileImagePrivacy.h"

@interface PersonalInfoViewController : SCITableViewController <UITextFieldDelegate, UIImagePickerControllerDelegate, UINavigationControllerDelegate>

@property (nonatomic, strong) UIAlertController *profileImageActionSheet;
@property (nonatomic, strong) UITableViewCell *profileImageCell;
@property (nonatomic, strong) UIImage *profileImage;
@property (nonatomic, strong) UIImagePickerController *imagePicker;
@property (nonatomic, strong) IBOutlet UIView *headerView;
@property (nonatomic, strong) IBOutlet UIView *footerView;

@property (nonatomic, strong) IBOutlet UILabel *nameLabel;
@property (nonatomic, strong) IBOutlet UILabel *localNumberTitleLabel;
@property (nonatomic, strong) IBOutlet UILabel *localNumberLabel;
@property (nonatomic, strong) IBOutlet UILabel *tollFreeNumberLabel;

@property (nonatomic, strong) IBOutlet UILabel *devNameLabel;
@property (nonatomic, strong) IBOutlet UILabel *devLocalNumberLabel;
@property (nonatomic, strong) IBOutlet UILabel *devTollFreeNumberLabel;

@property (nonatomic, assign) BOOL signMailNotify;
@property (nonatomic, strong) UISwitch *signMailNotifySwitch;
@property (nonatomic, strong) NSString *email1;
@property (nonatomic, strong) UITextField *email1TextField;
@property (nonatomic, strong) NSString *email2;
@property (nonatomic, strong) UITextField *email2TextField;

@property (nonatomic, assign) BOOL useTollFreeCallerID;
@property (nonatomic, strong) UISwitch *callerIDSwitch;

@property (nonatomic, strong) UISwitch *contactPhotosEnabledSwitch;

@property (nonatomic, assign) BOOL savingInProgress;
@property (nonatomic, assign) BOOL hasChangedFields;
@property (nonatomic, assign) BOOL isRingGroup;
@property (nonatomic, assign) BOOL hasTollFreeNumber;
@property (nonatomic, assign) BOOL showDeviceInfo;

@property (nonatomic, assign) BOOL updateProfilePhotoPrivacyOnExit;
@property (nonatomic, assign) SCIProfileImagePrivacyLevel newPrivacyLevel;



- (IBAction)saveButtonClicked:(id)sender;
- (IBAction)cancelButtonClicked:(id)sender;

@end
