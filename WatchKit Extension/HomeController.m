//
//   HomeController.m
//   WatchKit Extension
//
//  Created by Kevin Selman on 5/15/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "HomeController.h"
#import "ntouchWatchConstantsAndKeys.h"


@interface HomeController()

@property (nonatomic, weak) IBOutlet WKInterfaceButton *historyButton;
@property (nonatomic, weak) IBOutlet WKInterfaceButton *signMailButton;

@end


@implementation HomeController

- (void)awakeWithContext:(id)context {
    [super awakeWithContext:context];

    // Configure interface objects here.
}

- (void)willActivate {
    // This method is called when watch view controller is about to be visible to user
    [super willActivate];
}

- (void)didDeactivate {
    // This method is called when watch view controller is no longer visible
    [super didDeactivate];
}

//- (IBAction)historyButtonTapped:(id)sender
//{
//
//}
//
//- (IBAction)signMailButtonTapped:(id)sender
//{
//	
//}


@end



