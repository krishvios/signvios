//
//  SIPRelayViewController.h
//  SorensonVRS
//
//  Created by Todd Ouzts on 10/8/09.
//  Copyright 2009 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AddressBookUI/AddressBookUI.h>
#import <MessageUI/MessageUI.h>

@interface SIPRelayViewController : UIViewController <UINavigationControllerDelegate, ABPeoplePickerNavigationControllerDelegate, UIActionSheetDelegate, MFMailComposeViewControllerDelegate> {
	ABPeoplePickerNavigationController *peoplePicker;
	UIImagePickerController *imagePicker;
	NSString *contactName;
	NSString *contactPhone;
}

@property (nonatomic, retain) NSString *contactName;
@property (nonatomic, retain) NSString *contactPhone;

@end
