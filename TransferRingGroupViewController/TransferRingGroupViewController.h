//
//  TransferRingGroupViewController.h
//  ntouch
//
//  Created by John Gallagher on 8/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SCITableViewController.h"

@class TransferRingGroupViewController;
@class SCIRingGroupInfo;

@protocol TransferRingGroupViewControllerDelegate <NSObject>

- (void)transferRingGroupViewController:(nonnull TransferRingGroupViewController *)ringGroupViewController didSelectRingGroupInfo:(nonnull SCIRingGroupInfo *)ringGroupInfo;

@end

@interface TransferRingGroupViewController : SCITableViewController

@property (nonatomic, weak, nullable) id<TransferRingGroupViewControllerDelegate> delegate;

@end
