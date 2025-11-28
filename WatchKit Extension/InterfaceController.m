//
//  InterfaceController.m
//   WatchKit Extension
//
//  Created by Kevin Selman on 5/15/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "InterfaceController.h"
#import "ntouchWatchConstantsAndKeys.h"
#import "WatchCallHistoryItem.h"
#import "TwoStringWatchTableRow.h"


@interface InterfaceController()

@property (nonatomic, weak) IBOutlet WKInterfaceTable *historyTable;
@property (nonatomic, weak) IBOutlet WKInterfaceLabel *footerLabel;
@property (nonatomic, strong) NSMutableArray *recentCallArray;

@end


@implementation InterfaceController

- (void)awakeWithContext:(id)context {
    [super awakeWithContext:context];

    // Configure interface objects here.
}

- (void)willActivate {
    // This method is called when watch view controller is about to be visible to user
    [super willActivate];	
	[self setupTable];
}

- (void)didDeactivate {
    // This method is called when watch view controller is no longer visible
    [super didDeactivate];
}

- (void)setupTable {
	if ([[WCSession defaultSession] isReachable]) {
		NSDictionary *requestDict = @{ WatchKitCallHistoryRequestKey : @"refreshData" };
		// Fetch Call history from parent application.  Expect an array od Dictionary Objects
		[[WCSession defaultSession] sendMessage:requestDict
								   replyHandler:^(NSDictionary *replyHandler) {
									   NSLog(@"sendMessage complete.");
									   
									   if (replyHandler[WatchKitCallHistoryDataKey]) {
										   // Create custom call history objects from dictionaries
										   self.recentCallArray = [[NSMutableArray alloc] init];
										   NSArray *callHistoryDicts = replyHandler[WatchKitCallHistoryDataKey];
										   for (NSDictionary *dict in callHistoryDicts) {
											   WatchCallHistoryItem *newItem = [[WatchCallHistoryItem alloc] initWithDictionary:dict];
											   [self.recentCallArray addObject:newItem];
										   }
										   
										   if (!self.recentCallArray.count) {
											   [self.footerLabel setHidden:NO];
											   [self.footerLabel setText:@"No Recent Calls"];
										   }
										   else {
											   [self.footerLabel setHidden:YES];
										   }
										   
										   // Update Table
										   [self.historyTable setNumberOfRows:self.recentCallArray.count withRowType:@"TwoStringWatchTableRow"];
										   for (NSInteger i = 0; i < self.historyTable.numberOfRows; i++)
										   {
											   NSObject *row = [self.historyTable rowControllerAtIndex:i];
											   WatchCallHistoryItem *item = self->_recentCallArray[i];
											   TwoStringWatchTableRow *historyRow = (TwoStringWatchTableRow *) row;
											   [historyRow.primaryLabel setText:item.callerName];
											   [historyRow.secondaryLabel setText:item.callTime];
										   }
									   }
								   }
								   errorHandler:^(NSError *error) {
									   NSLog(@"sendMessage error %@.", error.localizedDescription);
								   }
		 ];
		
		
		
	}
	else {
		[self.footerLabel setText:@"ntouch app is not reachable."];
	}
}
@end



