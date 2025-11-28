//
//  VideoCenterEpisodeViewController.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>
#import "SCITableViewController.h"
#import "SCIMessageCategory.h"
#import "SCIMessageInfo.h"

@interface VideoCenterEpisodeViewController : SCITableViewController
{
	
	NSArray *programsArray;
	NSMutableArray *episodesArray;
}

@property (nonatomic, strong) NSString *channelImage;
@property (nonatomic, strong) SCIMessageCategory *selectedCategory;

@end
