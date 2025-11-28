//
//  SCITableViewController.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "SCITableViewController.h"
#import "SCIVideophoneEngine.h"
#import "AppDelegate.h"
#import "SCIAccountManager.h"
#import "Alert.h"
#import "Utilities.h"
#import "ntouch-Swift.h"

@implementation SCITableViewController


- (void)viewDidLoad {
	[super viewDidLoad];

	self.refreshControl = [[UIRefreshControl alloc] init];
	[self.refreshControl addTarget:self action:@selector(refreshControlActivated) forControlEvents:UIControlEventValueChanged];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyTableViewTheme)
												 name:Theme.themeDidChangeNotification
											   object:nil];
	[self applyTableViewTheme];
}

- (void)applyTableViewTheme {
	self.view.backgroundColor = Theme.backgroundColor;
}

- (void)refreshControlActivated {
	[[SCIVideophoneEngine sharedEngine] heartbeat];
	[self.refreshControl endRefreshing];
}

- (UIView *)viewController:(UITableViewController *)controller viewForHeaderInSection:(NSInteger)section
{
	NSString *text = [controller tableView:controller.tableView titleForHeaderInSection:section];
	if (!text)
		return nil;
	
	CGFloat width = controller.view.bounds.size.width-10; //build in a slight buffer around the edges so some text doesn't get cut off on smaller devices
	CGFloat labelX = 16.0;
	
	NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:text attributes:@{ NSFontAttributeName: Theme.tableHeaderFont }];
	CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize size = rect.size;
	CGFloat height = size.height;
	
	// create the parent view that will hold the Label
	UIView* view = [[UIView alloc] initWithFrame:CGRectMake(0.0, 0.0, width, height + 10.0)];
	view.backgroundColor = Theme.tableHeaderBackgroundColor;
	
	// create the label object
	UILabel * label = [[UILabel alloc] initWithFrame:CGRectZero];
	label.tag = 20052420;
	label.textColor = Theme.tableHeaderTextColor;
	label.frame = CGRectMake(labelX, 5.0, width, height);
	label.numberOfLines = 0;
	label.font = Theme.tableHeaderFont;
	label.text = text;
	[view addSubview:label];
	
	return view;
}

- (UIView *)viewController:(UITableViewController *)controller viewForHeaderInSection:(NSInteger)section withSubTitle:(NSString *)subTitle
{

	NSString *text = [controller tableView:controller.tableView titleForHeaderInSection:section];
	if (!text)
		return nil;
    else if (!subTitle)
        return [self viewController:controller viewForHeaderInSection:section];
    
	CGFloat width = controller.view.bounds.size.width - 10;
	CGFloat labelX = 10.0;
	
	NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName: Theme.tableHeaderFont }];
	CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize headerSize = rect.size;
	attributedText = [[NSAttributedString alloc] initWithString:subTitle attributes:@{NSFontAttributeName: Theme.tableHeaderFont }];
	rect = [attributedText boundingRectWithSize:CGSizeMake(width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize subTextSize = rect.size;
    
	CGFloat height = headerSize.height + subTextSize.height + 2; // room for shadow
	
	// create the parent view that will hold the Labels
	UIView* view = [[UIView alloc] initWithFrame:CGRectMake(10.0, 0.0, width, height + 20.0)];
    view.backgroundColor = [UIColor clearColor];
	view.opaque = NO;
	
	// create the header label object
	UILabel *headerlabel = [[UILabel alloc] initWithFrame:CGRectZero];
	headerlabel.textColor = Theme.tableHeaderTextColor;
	headerlabel.numberOfLines = 0;
	headerlabel.text = text;
	headerlabel.frame = CGRectMake(labelX, 10.0, width, headerSize.height);
	headerlabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    
    UILabel *subTextLabel = [[UILabel alloc]initWithFrame:CGRectZero];
	subTextLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth;
	subTextLabel.textColor = Theme.tableHeaderTextColor;
	subTextLabel.frame = CGRectMake(labelX, headerlabel.frame.origin.y + headerlabel.frame.size.height, width- 20, subTextSize.height);
	subTextLabel.numberOfLines = 0;
	subTextLabel.lineBreakMode = NSLineBreakByWordWrapping;
	subTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
	subTextLabel.text = subTitle;
	
	[view addSubview:headerlabel];
    [view addSubview:subTextLabel];
	
	return view;
}

- (UIView *)viewController:(UITableViewController *)controller viewForFooterInSection:(NSInteger)section
{
	// determine the text height
	NSString *text = [controller tableView:controller.tableView titleForFooterInSection:section];
	if (nil == text) {
		return nil;
	}
	UIFont *labelFont = Theme.tableFooterFont;
	
	CGFloat width = controller.view.bounds.size.width - 20.0;
	NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName: labelFont}];
	CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize size = rect.size;
	CGFloat height = size.height + 2;
	
	// create the parent view that will hold the Label
	UIView* view = [[UIView alloc] initWithFrame:CGRectMake(10.0, 0.0, width, height + 20.0)];
	
	// create the label object
	UILabel * label = [[UILabel alloc] initWithFrame:CGRectZero];
	label.textColor = Theme.tableFooterTextColor;
	label.frame = CGRectMake(10.0, 10.0, width, height);
	label.numberOfLines = 0;
	label.textAlignment = NSTextAlignmentCenter;
	label.font = labelFont;
	label.backgroundColor = [UIColor clearColor];
	label.opaque = NO;
	label.shadowOffset = CGSizeMake(0, 1);
	label.shadowColor = [UIColor grayColor];
	
	label.text = text;
	[view addSubview:label];
	
	return view;
}

- (UIView *)viewController:(UITableViewController *)controller noRecordsViewForFooterInSection:(NSInteger)section {
	NSString *text = [controller tableView:controller.tableView titleForFooterInSection:section];
	if (!text)
		return nil;
	
	CGFloat width = controller.view.bounds.size.width;
	NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:text attributes:@{ NSFontAttributeName: Theme.tableFooterFont }];
	CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize size = rect.size;
	CGFloat height = size.height + 2; // room for shadow
	
	// create the parent view that will hold the Label
	UIView* view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, height)];
	view.autoresizingMask = UIViewAutoresizingFlexibleWidth;
	view.opaque = NO;
	
	// create the label object
	UILabel * label = [[UILabel alloc] initWithFrame:CGRectZero];
	label.textColor = Theme.tableFooterTextColor;
	label.frame = CGRectMake(0, 0, width, height);
	label.numberOfLines = 0;
	label.font = Theme.tableFooterFont;
	label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	label.textAlignment = NSTextAlignmentCenter;
	label.text = text;
	[view addSubview:label];
	
	return view;
}

- (BOOL)canPlayVideoWithAlerts {
	if (appDelegate.networkMonitor.bestInterfaceType == SCINetworkTypeNone) {
		AlertWithTitleAndMessage(Localize(@"Can’t Play Video"), Localize(@"Network not available."));
		return NO;
	}
	else {
		return YES;
	}
}

@end
