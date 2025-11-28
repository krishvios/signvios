//
//  FavoritesViewController.h
//  ntouch
//
//  Created by Todd Ouzts on 5/17/10.
//  Copyright 2010 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "BaseListViewController.h"
#import <AddressBookUI/AddressBookUI.h>

@interface FavoritesViewController : BaseListViewController <ABPeoplePickerNavigationControllerDelegate> {

}

- (IBAction)addAContact:(id)sender;

@end
