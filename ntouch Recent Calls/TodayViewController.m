//
//  TodayViewController.m
//  ntouch Recent Calls
//
//  Created by Joseph Laser on 10/26/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>

@interface TodayViewController () <NCWidgetProviding>

@property (weak, nonatomic) IBOutlet UITableView *recentCallsTableView;

@end

@implementation TodayViewController

@synthesize recentNames;
@synthesize recentUnformatedNumbers;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
	
	[self getRecentCalls];
	
	if([self.extensionContext respondsToSelector:@selector(setWidgetLargestAvailableDisplayMode:)])
	{
		[self.extensionContext setWidgetLargestAvailableDisplayMode:NCWidgetDisplayModeExpanded];
	}
	else
	{
		self.preferredContentSize = CGSizeMake(320, 220);
	}
	
	if (@available(iOS 13.0, *)) {
		self.recentCallsTableView.backgroundColor = UIColor.clearColor;
	}
	
	dispatch_async(dispatch_get_main_queue(), ^() {
		[self.recentCallsTableView reloadData];
	});
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)widgetPerformUpdateWithCompletionHandler:(void (^)(NCUpdateResult))completionHandler {
    // Perform any setup necessary in order to update the view.
    
    // If an error is encountered, use NCUpdateResultFailed
    // If there's no update required, use NCUpdateResultNoData
    // If there's an update, use NCUpdateResultNewData
	
	[self getRecentCalls];
	
	dispatch_async(dispatch_get_main_queue(), ^() {
		[self.recentCallsTableView reloadData];
	});

    completionHandler(NCUpdateResultNewData);
}

- (void)getRecentCalls
{
	NSUserDefaults *sharedDefaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.sorenson.ntouch.recent-calls-extension-data-sharing"];
	NSDictionary *recentCallSummary = [sharedDefaults objectForKey:@"recentSummary"];
	
	recentNames = [recentCallSummary objectForKey:@"name"];
	recentUnformatedNumbers = [recentCallSummary objectForKey:@"unformatedNumber"];
}

#pragma mark - Table view delegate

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	if([self.extensionContext respondsToSelector:@selector(setWidgetLargestAvailableDisplayMode:)] && self.extensionContext.widgetActiveDisplayMode == NCWidgetDisplayModeCompact)
	{
		if([recentNames count] < 2)
		{
			return [recentNames count];
		}
		return 2;
	}
	if([recentNames count] < 5)
	{
		return [recentNames count];
	}
	return 5;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *CellIdentifier = @"TopRecentCell";
	UITableViewCell *cell = (UITableViewCell *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
	}
	
	cell.textLabel.text = [recentNames objectAtIndex:indexPath.row];
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	[self.recentCallsTableView deselectRowAtIndexPath:indexPath animated:YES];
	
	NSString *urlString = [NSString stringWithFormat:@"ntouch://?numberToDial=%@", [recentUnformatedNumbers objectAtIndex:indexPath.row]];
	NSURL *callURL = [NSURL URLWithString:urlString];
	[self.extensionContext openURL:callURL completionHandler:nil];
}

- (void)widgetActiveDisplayModeDidChange:(NCWidgetDisplayMode)activeDisplayMode withMaximumSize:(CGSize)maxSize{
	if (activeDisplayMode == NCWidgetDisplayModeCompact) {
		self.preferredContentSize = CGSizeMake(320, 88);
	}
	else
	{
		self.preferredContentSize = CGSizeMake(320, 220);
	}
	dispatch_async(dispatch_get_main_queue(), ^() {
		[self.recentCallsTableView reloadData];
	});
}

@end
