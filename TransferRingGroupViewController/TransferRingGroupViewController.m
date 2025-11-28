//
//  TransferRingGroupViewController.m
//  ntouch
//
//  Created by John Gallagher on 8/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "AppDelegate.h"
#import "TransferRingGroupViewController.h"
#import "Utilities.h"
#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "ntouch-Swift.h"

@interface TransferRingGroupViewController ()
@end

@implementation TransferRingGroupViewController

#pragma mark - Lifecycle

- (id)init
{
    self = [super initWithStyle:UITableViewStyleGrouped];
    if (self) {
		
    }
    return self;
}

- (void)loadView
{
	[super loadView];
    self.navigationItem.title = @"myPhone Group";
	[self.tableView registerNib:TransferRingGroupInfoCell.nib forCellReuseIdentifier:TransferRingGroupInfoCell.cellIdentifier];
}

#pragma mark - Accessors: Convenience

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return self.engine.otherRingGroupInfos.count;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
	if (self.engine.otherRingGroupInfos.count == 0) {
		return @"There are no other phones in your myPhone group.";
	}
	else
		return nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	TransferRingGroupInfoCell *cell = [tableView dequeueReusableCellWithIdentifier:TransferRingGroupInfoCell.cellIdentifier forIndexPath:indexPath];
	[self configureCell:cell atIndexPath:indexPath];
    
    return cell;
}

- (void)configureCell:(TransferRingGroupInfoCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
	cell.ringGroupInfo = [self ringGroupInfoAtIndexPath:indexPath];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	[self didSelectRingGroupInfo:[self ringGroupInfoAtIndexPath:indexPath]];
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
	return [self viewController:self noRecordsViewForFooterInSection:section].frame.size.height;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
	return [self viewController:self noRecordsViewForFooterInSection:section];
}

#pragma mark - Helpers: UITableView

- (SCIRingGroupInfo *)ringGroupInfoAtIndexPath:(NSIndexPath *)indexPath
{
	return self.engine.otherRingGroupInfos[indexPath.row];
}

#pragma mark - Delegate

- (void)didSelectRingGroupInfo:(SCIRingGroupInfo *)ringGroupInfo
{
	if ([self.delegate respondsToSelector:@selector(transferRingGroupViewController:didSelectRingGroupInfo:)]) {
		[[SCIVideophoneEngine sharedEngine] setDialSource:SCIDialSourceMyPhone];
		[self.delegate transferRingGroupViewController:self didSelectRingGroupInfo:ringGroupInfo];
	}
}

@end
