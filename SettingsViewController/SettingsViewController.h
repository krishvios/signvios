//
//  SettingsViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import "SCITableViewController.h"
#import "AppDelegate.h"

@interface SettingsViewController : UITableViewController <UITextFieldDelegate, MFMailComposeViewControllerDelegate>

@property (nonatomic, strong) UISwitch *audioEnabledSwitch;
@property (nonatomic, strong) UISwitch *vcoActiveSwitch;
@property (nonatomic, strong) UILabel *vcoActiveLabel;
@property (nonatomic, strong) UISwitch *autoRejectCallsSwitch;
@property (nonatomic, strong) UISwitch *callKitEnabledSwitch;
@property (nonatomic, strong) UISwitch *callWaitingSwitch;
@property (nonatomic, strong) UISwitch *secureCallModeSwitch;
@property (nonatomic, strong) UISwitch *tunnelingSwitch;
@property (nonatomic, strong) UITextField *nvpIpAddressTextField;
@property (nonatomic, strong) UISwitch *blockCallerIDSwitch;
@property (nonatomic, strong) UISwitch *blockAnonymousCallersSwitch;
@property (nonatomic, strong) UISwitch *enableSpanishFeaturesSwitch;
@property (nonatomic, strong) UISwitch *pulseEnableSwitch;

@end
