//
//  MyPhoneEditDetailViewController.h
//  ntouch
//
//  Created by Kevin Selman on 6/15/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//


#import "SCIRingGroupInfo.h"
#import "SCITableViewController.h"

@interface MyPhoneEditDetailViewController : SCITableViewController <UITextFieldDelegate>

@property (nonatomic, strong) SCIRingGroupInfo *infoItem;
@property (nonatomic, strong) UITextField *phoneNumberTextField;
@property (nonatomic, strong) UITextField *descriptionTextField;
@property (nonatomic, assign) BOOL isEditMode;
@property (nonatomic, assign) BOOL allowsRemove;
@property (nonatomic, assign) NSInteger displayMode;

- (IBAction)saveButtonClicked:(id)sender;
- (IBAction)cancelButtonClicked:(id)sender;

@end
