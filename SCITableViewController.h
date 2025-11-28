//
//  SCITableViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2014 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>

@interface SCITableViewController : UITableViewController

- (UIView *)viewController:(UITableViewController *)controller viewForHeaderInSection:(NSInteger)section;
- (UIView *)viewController:(UITableViewController *)controller viewForHeaderInSection:(NSInteger)section withSubTitle:(NSString *)subTitle;
- (UIView *)viewController:(UITableViewController *)controller viewForFooterInSection:(NSInteger)section;
- (UIView *)viewController:(UITableViewController *)controller noRecordsViewForFooterInSection:(NSInteger)section;

- (BOOL)canPlayVideoWithAlerts;

@end
