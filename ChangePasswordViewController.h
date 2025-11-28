//
//  ChangePasswordViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import "SCITableViewController.h"

@interface ChangePasswordViewController : UITableViewController <UITextFieldDelegate>

@property (nonatomic, assign) BOOL isMyPhonePassword;
@property (nonatomic, strong) UITextField *oldPasswordTextField;
@property (nonatomic, strong) UITextField *changePasswordTextField;
@property (nonatomic, strong) UITextField *confirmPasswordTextField;
@property (nonatomic, strong) NSString *enteredNewPassword;

@end
