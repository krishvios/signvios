//
//  MyPhoneSetupViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "SCITableViewController.h"

@interface MyPhoneSetupViewController : SCITableViewController <UITextFieldDelegate>

@property (nonatomic, strong) UITextField *phoneNumberTextField;
@property (nonatomic, strong) UITextField *descriptionTextField;
@property (nonatomic, strong) UITextField *passwordTextField;
@property (nonatomic, strong) UITextField *confirmPasswordTextField;
@property (nonatomic, strong) NSString *phoneDescription;
@property (nonatomic, strong) NSString *password;
@property (nonatomic, strong) NSString *confirmPassword;

- (IBAction)nextButtonClicked:(id)sender;
- (IBAction)cancelButtonClicked:(id)sender;

@end
