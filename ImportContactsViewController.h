//
//  ImportContactsViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import "SCITableViewController.h"
#import "SCIContact.h"
#import <ContactsUI/ContactsUI.h>

@class LoadingView;

@interface ImportContactsViewController : SCITableViewController <UITextFieldDelegate, CNContactPickerDelegate> {
	NSOperationQueue *opQ;
	LoadingView *loadingView;
	UITextField *phoneNumberTextField;
	UITextField *PINTextField;
}

@property (nonatomic, retain) LoadingView *loadingView;
@property (nonatomic, assign) BOOL isImporting;
@property (nonatomic, assign) BOOL shouldContinue;
@property (nonatomic, assign) SCIContact *importPhotoContact;
@property (nonatomic, assign) BOOL contactPhotosEnabled;

@end
