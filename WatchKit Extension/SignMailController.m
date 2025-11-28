//
//  InterfaceController.m
//   WatchKit Extension
//
//  Created by Kevin Selman on 5/15/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "SignMailController.h"
#import "ntouchWatchConstantsAndKeys.h"
#import "WatchSignMailItem.h"
#import "TwoStringWatchTableRow.h"


@interface SignMailController()

@property (nonatomic, weak) IBOutlet WKInterfaceTable *signMailTable;
@property (nonatomic, weak) IBOutlet WKInterfaceLabel *footerLabel;
@property (nonatomic, strong) NSMutableArray *signMailsArray;

@end


@implementation SignMailController

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

- (void)setupTable
{
	if ([[WCSession defaultSession] isReachable]) {
		NSDictionary *requestDict = @{ WatchKitSignMailListRequestKey : @"refreshData" };
		
		// Fetch Call history from parent application.  Expect an array od Dictionary Objects
		[[WCSession defaultSession] sendMessage:requestDict
								   replyHandler:^(NSDictionary *replyHandler) {
									   NSLog(@"sendMessage complete.");
									  
									   if (replyHandler[WatchKitSignMailListDataKey]) {
										   // Create custom call history objects from dictionaries
										   self.signMailsArray = [[NSMutableArray alloc] init];
										   NSArray *signMailDicts = replyHandler[WatchKitSignMailListDataKey];
										   for (NSDictionary *dict in signMailDicts) {
											   WatchSignMailItem *newItem = [[WatchSignMailItem alloc] initWithDictionary:dict];
											   [self.signMailsArray addObject:newItem];
										   }
										   
										   if (!self.signMailsArray.count) {
											   [self.footerLabel setHidden:NO];
											   [self.footerLabel setText:@"No SignMails"];
										   }
										   else {
											   [self.footerLabel setHidden:YES];
										   }
										   
										   // Update Table
										   [self.signMailTable setNumberOfRows:self.signMailsArray.count withRowType:@"TwoStringWatchTableRow"];
										   
										   NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
										   formatter.dateFormat = @"MM-dd HH:mm";
										   for (NSInteger i = 0; i < self.signMailTable.numberOfRows; i++)
										   {
											   NSObject *row = [self.signMailTable rowControllerAtIndex:i];
											   WatchSignMailItem *item = self->_signMailsArray[i];
											   TwoStringWatchTableRow *signMailRow = (TwoStringWatchTableRow *) row;
											   [signMailRow.primaryLabel setText:item.callerName];
											   [signMailRow.secondaryLabel setText:[formatter stringFromDate:item.callTime]];
										   }
									   }
								   }
								   errorHandler:^(NSError *error) {
									NSLog(@"sendMessage error %@.", error.localizedDescription);
								   }
		 ];
	}
	else {
		[self.footerLabel setText:@"ntouch app not reachable."];
	}
}

- (void)table:(WKInterfaceTable *)table didSelectRowAtIndex:(NSInteger)rowIndex {
	
	if (rowIndex < self.signMailsArray.count) {
		WatchSignMailItem *item = self.signMailsArray[rowIndex];

		NSDictionary *context = @{ @"url": item.videoURL,
								   @"id": item.messageId };
		[self presentControllerWithName:@"SignMailPlayerController" context:context];
	}
}

@end



