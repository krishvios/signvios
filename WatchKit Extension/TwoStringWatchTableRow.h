//
//  HistoryRow.h
//  ntouch
//
//  Created by Kevin Selman on 6/26/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>

@interface TwoStringWatchTableRow : NSObject

@property (nonatomic, weak) IBOutlet WKInterfaceLabel *primaryLabel;
@property (nonatomic, weak) IBOutlet WKInterfaceLabel *secondaryLabel;

@end
