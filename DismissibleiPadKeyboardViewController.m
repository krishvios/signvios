//
//  DismissibleiPadKeyboardViewController.m
//  ntouch
//
//  Created by J.R. Blackham on 9/9/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "DismissibleiPadKeyboardViewController.h"

@interface DismissibleiPadKeyboardViewController ()

@end

@implementation DismissibleiPadKeyboardViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (BOOL)disablesAutomaticKeyboardDismissal
{
    return NO;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
