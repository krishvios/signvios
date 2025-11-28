//
//  SignMailGreetingRecordViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import "SCISignMailGreetingType.h"

@class SignMailGreetingManager;

@interface SignMailGreetingRecordViewController : UIViewController <UITextViewDelegate>

@property (nonatomic, assign) SCISignMailGreetingType selectedGreetingType;

@end
