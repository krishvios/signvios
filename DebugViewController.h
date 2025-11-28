//
//  DebugViewController.h
//  ntouch
//
//  Created by DBaker on 1/21/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCITableViewController.h"
#import "AppDelegate.h"
#import "TypeListController.h"
#import "SCIVideophoneEngine.h"

@interface DebugViewController : SCITableViewController <UINavigationControllerDelegate, UISearchBarDelegate>
{
	
}

@property (nonatomic,readwrite) NSMutableDictionary *editingItem;
@property (nonatomic,readwrite) NSMutableArray *debugArry;
@property (nonatomic,readwrite) BOOL searchingArry;
@property (nonatomic, strong) UISearchBar *searchBar;

@end
