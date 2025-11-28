//
//  FavoritesViewController.m
//  ntouch
//
//  Created by Todd Ouzts on 5/17/10.
//  Copyright 2010 Sorenson Communications. All rights reserved.
//

#import "FavoritesViewController.h"
#import "AppDelegate.h"
#import "SService.h"
#import "SAccount.h"
#import "SFavorite.h"
#import "Utilities.h"
#import "Alert.h"

@implementation FavoritesViewController

/*
 - (void)viewDidLoad {
	[super viewDidLoad];
	if (editable)
		self.navigationItem.leftBarButtonItem = self.editButtonItem;
}
*/

- (void)setObject {
	self.managedObjectContext = appDelegate.service.managedObjectContext;
	self.parentObject = appDelegate.service.account;
	self.entityName = @"SFavorite";
	self.editable = YES;
	if (editable)
		self.navigationItem.leftBarButtonItem = self.editButtonItem;
}

- (NSPredicate *)predicate {
	return [NSPredicate predicateWithFormat: @"account == %@", parentObject];
}

- (NSArray *)sortDescriptors {
	NSSortDescriptor *sortDescriptor1 = [[NSSortDescriptor alloc] initWithKey:@"name" ascending:YES];
	NSArray *array = [NSArray arrayWithObjects:sortDescriptor1, nil];
	[sortDescriptor1 release];
	return array;
}

- (NSString *)sectionNameKeyPath {
	return nil;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	// Updates the appearance of the Edit|Done button as necessary.
	[super setEditing:editing animated:animated];
	
	// replace the add button while editing.
	if (self.editing) {
		UIBarButtonItem *addButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(addAContact:)];
		[self.navigationItem setRightBarButtonItem:addButton animated:YES];
		[addButton release];
	} else
		self.navigationItem.rightBarButtonItem = nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	return nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
	return nil;
}

- (void)configureCell:(UITableViewCell *)cell forObject:(id)object {
	SFavorite *favorite = object;
	cell.textLabel.text = favorite.name;
	if (YES)
		cell.detailTextLabel.text = Localize(@"mobile"); // TODO
}

// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    static NSString *CellIdentifier = @"Cell";
    	
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier] autorelease];
	}
    
	[self configureCell:cell forObject:[fetchedResultsController objectAtIndexPath:indexPath]];
	
    return cell;
}

#pragma mark ABPeoplePickerNavigationControllerDelegate methods

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[self dismissModalViewControllerAnimated:YES];
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	NSString *name = (NSString *)ABRecordCopyCompositeName(person);
	NSUInteger recordID = ABRecordGetRecordID(person);
	BOOL new = YES;
	for (SFavorite *favorite in appDelegate.service.account.favorites)
		if (recordID == favorite.contactID.integerValue) {
			new = NO;
			break;
		}
	if (new) {
		SFavorite *favorite = [NSEntityDescription insertNewObjectForEntityForName:@"SFavorite" inManagedObjectContext:appDelegate.managedObjectContext];
		[appDelegate.service.account addFavoritesObject:favorite];
		favorite.name = name;
		favorite.contactID = [NSNumber numberWithInteger:recordID];
		[self dismissModalViewControllerAnimated:YES];
	}
	else
		AlertWithTitleAndMessage(Localize(@"Can’t Add Contact"), Localize(@"The selected contact is already in your Favorites list."));

	return NO;
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
      shouldContinueAfterSelectingPerson:(ABRecordRef)person
                                property:(ABPropertyID)property
                              identifier:(ABMultiValueIdentifier)identifier {
/*	
	// did the user select a phone number?
	if (property == kABPersonPhoneProperty) {
		// get a list of the contact's phone numbers
		ABMultiValueRef multiPhones = ABRecordCopyValue(person, kABPersonPhoneProperty);
		// loop through each one
		for (CFIndex i = 0; i < ABMultiValueGetCount(multiPhones); i++) {
			// find the one identified as selected
			if (identifier == ABMultiValueGetIdentifierAtIndex(multiPhones, i)) {
				CFStringRef phoneNumberRef = ABMultiValueCopyValueAtIndex(multiPhones, i);
				CFRelease(multiPhones);
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
				if (actionSheet.numberOfButtons > 1) {
					[actionSheet showFromTabBar:appDelegate.tabBarController.tabBar];
					[actionSheet release];
				}
				else
					AlertWithTitleAndMessage(Localize(@"No IM Apps Found"), Localize(@"If you install a supported Instant Messaging app (AIM or BeejiveIM) from the App Store, you can use it to call a Contact via SIPRelay."));
				break;
			}
		}
	}
*/	
	// tell the picker we're done selecting things
	return NO;
}

- (IBAction)addAContact:(id)sender {
	ABPeoplePickerNavigationController *peoplePicker = [[ABPeoplePickerNavigationController alloc] init];
	peoplePicker.peoplePickerDelegate = self;
	peoplePicker.modalInPopover = YES;
	peoplePicker.modalPresentationStyle = UIModalPresentationFormSheet;
	[self presentModalViewController:peoplePicker animated:YES];
	[peoplePicker release];
}

@end
