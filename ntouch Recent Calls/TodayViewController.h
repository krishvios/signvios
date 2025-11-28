//
//  TodayViewController.h
//  ntouch Recent Calls
//
//  Created by Joseph Laser on 10/26/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>

API_AVAILABLE(ios(10.0))
@interface TodayViewController : UIViewController <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic, strong) NSArray *recentNames;
@property (nonatomic, strong) NSArray *recentUnformatedNumbers;

@end
