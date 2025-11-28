//
//  Alert.m
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2016 Sorenson Communications, Inc. -- All rights reserved
//

#import "Alert.h"
#import "Utilities.h"
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>
#import "AppDelegate.h"

void AlertWithError(NSError *error)
{
	[[FIRCrashlytics crashlytics] recordError:error];
	AlertWithTitleAndMessage(Localize(@"Error"), Localize([error localizedDescription]));
}


void AlertWithMessage(NSString *message)
{
	/* open an alert with an OK button */
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@""
																   message:message
															preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")  style:UIAlertActionStyleCancel handler:nil]];
	
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}

void AlertWithTitleAndMessage(NSString *title, NSString *message)
{
	/* open an alert with OK and Cancel buttons */
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:title
																   message:message
															preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:Localize(@"OK")  style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
		[alert dismissViewControllerAnimated:YES completion:nil];
	}]];
	
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}
