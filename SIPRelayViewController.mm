//
//  SIPRelayViewController.m
//  SorensonVRS
//
//  Created by Todd Ouzts on 10/8/09.
//  Copyright 2009 Sorenson Communications. All rights reserved.
//

#import "SIPRelayViewController.h"
#import "AppDelegate.h"
#import "Utilities.h"
#import "Alert.h"
#import "SService.h"
#import "SAccount.h"
#import "SMessage.h"

/*
@interface ABPeoplePickerNavigationController (Expose)
@property (nonatomic, assign) BOOL allowsCancel;
@property (nonatomic, assign) BOOL allowsCardEditing;
@end
*/

@implementation SIPRelayViewController

@synthesize contactName;
@synthesize contactPhone;

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
	navigationController.navigationBar.barStyle = UIBarStyleBlack;
	
	UIView *customView = [[UIView alloc] initWithFrame:CGRectMake(0,0,0,0)];
	UIBarButtonItem *barButtonItem = [[UIBarButtonItem alloc] initWithCustomView:customView];
	[viewController.navigationItem setRightBarButtonItem:barButtonItem animated:NO];
	[barButtonItem release];
	[customView release];
	
	// this only partially works
	//	viewController.view.backgroundColor = [UIColor clearColor];
}

- (void)awakeFromNib {
	// must retain this view controller because it will be released by setViewControllers below
	[self retain];
	
//	endCallButton.tintColor = [UIColor redColor];
	
	peoplePicker = [[ABPeoplePickerNavigationController alloc] init];
	NSMutableArray *newControllers = [NSMutableArray arrayWithArray:[self.tabBarController viewControllers]];
	int index = [newControllers indexOfObject:self.navigationController];
	[newControllers replaceObjectAtIndex:index withObject:peoplePicker];
	// the tabBarItem has to be reset because the previous line causes it to change to "Groups" with no icon
//	[peoplePicker.tabBarItem initWithTabBarSystemItem:UITabBarSystemItemContacts tag:0];
	[peoplePicker.tabBarItem initWithTitle:@"SIPRelay" image:[UIImage imageNamed:@"SIPRelayIcon.png"] tag:0];
//	peoplePicker.allowsCancel = NO;
//	peoplePicker.allowsCardEditing = YES;
	[self.tabBarController setViewControllers:newControllers animated:NO];
	if (kBranding) {
		peoplePicker.navigationBar.barStyle = UIBarStyleBlack;
		peoplePicker.topViewController.view.backgroundColor = [UIColor clearColor];
	}
	peoplePicker.peoplePickerDelegate = self;
	peoplePicker.delegate = self;
	[peoplePicker release];
}

										
- (IBAction)showPicker:(id)sender {
    ABPeoplePickerNavigationController *picker = [[ABPeoplePickerNavigationController alloc] init];
    picker.peoplePickerDelegate = self;
	if (NO)
		[self.navigationController pushViewController:picker animated:NO];
	else
		[self presentModalViewController:picker animated:NO];
    [picker release];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	if (kBranding)
		self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	self.tabBarItem.title = Localize(@"SIPRelay");
	self.tabBarItem.image = [UIImage imageNamed:@"SIPRelayIcon.png"];
	self.navigationController.title = Localize(@"SIPRelay");
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return YES;
}

- (void)dealloc {
	[contactName release];
	[contactPhone release];
    [super dealloc];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex != alertView.cancelButtonIndex) {
		NSString *protocol = [NSString stringWithFormat:@"beejiveim://chat?screenname=SIPRelay&message=%@", contactPhone];
		protocol = [protocol stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:protocol]];
	}
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == actionSheet.cancelButtonIndex) // -1 on iPad, since Cancel button is a tap outside
		return;
	NSString *action = [actionSheet buttonTitleAtIndex:buttonIndex];
	if ([action isEqualToString:Localize(@"Call with AIM")]) {
		//[appDelegate trackGANEvent:@"Video" action:@"Finish Reply by SIPRelay"];
		if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"aim:"]]) {
			[[UIPasteboard generalPasteboard] setString:contactPhone];
			NSString *protocol = [NSString stringWithFormat:@"aim:goim?screenname=SIPRelay&message=%@", contactPhone];
			protocol = [protocol stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:protocol]];
		}
		else
			AlertWithTitleAndMessage(Localize(@"AIM Not Found"), Localize(@"If you install AIM (AOL Instant Messenger) from the App Store, you can use it to call a Contact via SIPRelay. You may need to delete and then reinstall the appropriate version of AIM for it to register properly on this device."));
	}
	else if ([action isEqualToString:Localize(@"Call with BeejiveIM")]) {
		if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"beejiveim:"]]) {
			[[UIPasteboard generalPasteboard] setString:contactPhone];
			AlertWithTitleAndMessageAndDelegate(Localize(@"Phone Number Copied"), [NSString stringWithFormat:Localize(@"To call this Contact with BeejiveIM, you can paste “%@” into a message for the “SIPRelay” buddy."), contactPhone], self);
		}
		else
			AlertWithTitleAndMessage(Localize(@"BeejiveIM Not Found"), Localize(@"If you install BeejiveIM from the App Store, you can use it to call a Contact via SIPRelay."));
	}
}

#pragma mark ABPeoplePickerNavigationControllerDelegate methods

- (void)syncWithContacts {
	for (SMessage *aMessage in appDelegate.service.account.messages)
		[aMessage syncWithContacts];
	[appDelegate didUpdate];
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[self syncWithContacts];
}


- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	// continue on because we want the user to select a phone number from the selected person's record
	return YES;
}

// Dismisses the email composition interface when users tap Cancel or Send. Proceeds to update the message field with the result of the operation.
- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error {    
	switch (result) {
		case MFMailComposeResultSaved:
		case MFMailComposeResultSent:
//			[appDelegate trackGANEvent:@"Video" action:@"Finish Reply by Mail"];
			break;
	}
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
      shouldContinueAfterSelectingPerson:(ABRecordRef)person
                                property:(ABPropertyID)property
                              identifier:(ABMultiValueIdentifier)identifier {
	
	// did the user select a phone number?
	if (property == kABPersonPhoneProperty) {
		// get a list of the contact's phone numbers
		ABMultiValueRef multiPhones = ABRecordCopyValue(person, kABPersonPhoneProperty);
		// loop through each one
		for (CFIndex i = 0; i < ABMultiValueGetCount(multiPhones); i++) {
			// find the one identified as selected
			if (identifier == ABMultiValueGetIdentifierAtIndex(multiPhones, i)) {
				CFStringRef phoneNumberRef = (CFStringRef)ABMultiValueCopyValueAtIndex(multiPhones, i);
				contactPhone = [(NSString *)phoneNumberRef copy];
				CFRelease(phoneNumberRef);
				
				contactName = (NSString *)ABRecordCopyCompositeName(person);
				NSLog(@"user selected %@ at %@", contactName, contactPhone);

//				UIActionSheet *actionSheet = [[UIActionSheet alloc] initWithTitle:[NSString stringWithFormat:Localize(@"Call %@ at %@ using SIPRelay?"), contactName, contactPhone] delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:nil];
				UIActionSheet *actionSheet = [[UIActionSheet alloc] init];
				actionSheet.delegate = self;
				actionSheet.title = [NSString stringWithFormat:Localize(@"Call %@ at %@ using SIPRelay?"), contactName, contactPhone];
				if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"aim:"]])
					[actionSheet addButtonWithTitle:Localize(@"Call with AIM")];
				if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"beejiveim:"]])
					[actionSheet addButtonWithTitle:Localize(@"Call with BeejiveIM")];
				[actionSheet addButtonWithTitle:Localize(@"Cancel")];
				actionSheet.cancelButtonIndex = actionSheet.numberOfButtons - 1;
				if (actionSheet.numberOfButtons > 1)
					[actionSheet showFromTabBar:appDelegate.tabBarController.tabBar];
				else
					AlertWithTitleAndMessage(Localize(@"No IM Apps Found"), Localize(@"If you install a supported Instant Messaging app (AIM or BeejiveIM) from the App Store, you can use it to call a Contact via SIPRelay."));
				[actionSheet release];
				break;
			}
		}
		CFRelease(multiPhones);
	}
	else if (NO && property == kABPersonEmailProperty) {
		if ([MFMailComposeViewController canSendMail]) {
			// get a list of the contact's email addresses
			ABMultiValueRef multiEmails = ABRecordCopyValue(person, kABPersonEmailProperty);
			// loop through each one
			for (CFIndex i = 0; i < ABMultiValueGetCount(multiEmails); i++) {
				// find the one identified as selected
				if (identifier == ABMultiValueGetIdentifierAtIndex(multiEmails, i)) {
					CFStringRef emailRef = (CFStringRef)ABMultiValueCopyValueAtIndex(multiEmails, i);
					CFRelease(multiEmails);
					
					contactName = (NSString *)ABRecordCopyCompositeName(person);
					NSLog(@"user selected %@ at %@", contactName, (NSString *)emailRef);
					
					MFMailComposeViewController *picker = [[MFMailComposeViewController alloc] init];
					picker.mailComposeDelegate = self;
									
					[picker setToRecipients:[NSArray arrayWithObject:(NSString *)emailRef]];
					
	//				[picker setSubject:[NSString stringWithFormat:Localize(@"Re: %@"), selectedMessage.name]];
					// this won't work because this view is already embedded as a modal view controller
					[self presentModalViewController:picker animated:YES];
					[picker release];
					break;
				}
			}
		}
		else
			AlertWithTitleAndMessage(Localize(@"Can’t Send Mail"), Localize(@"Your device is not yet configured to send email."));
	}
	else
		NSLog(@"user didn't select something we handle");
	
	[self syncWithContacts];
	// tell the picker we're done selecting things
	return NO;
}

@end
