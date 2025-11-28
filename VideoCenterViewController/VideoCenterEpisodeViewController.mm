//
//  VideoCenterEpisodeViewController.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "VideoCenterEpisodeViewController.h"
#import "Utilities.h"
#import "AppDelegate.h"
#import "SCIVideophoneEngine.h"
#import "SCIItemId.h"
#import "SCIVideoPlayer.h"
#import "Alert.h"
#import "ntouch-Swift.h"

@implementation VideoCenterEpisodeViewController

@synthesize channelImage;
@synthesize selectedCategory;

#pragma mark - Table view delegate

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	return [self viewController:self viewForHeaderInSection:section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
	return nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	SCIMessageCategory *category = [programsArray objectAtIndex:section];
	if(category)
		return category.longName;
	else
		return @"";
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *CellIdentifier = @"EpisodeCell";
	EpisodeCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[[NSBundle mainBundle] loadNibNamed:@"EpisodeCell" owner:self options:nil] lastObject];
	}
	
	SCIMessageInfo *message = [[episodesArray objectAtIndex:indexPath.section] objectAtIndex:indexPath.row];
    cell.episodeTitle.text = message.name;
	
	// Set release date as sub title
	NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
    [dateFormat setDateFormat:@"EEEE MMMM d, YYYY"];
	cell.episodeSubTitle.text = [dateFormat stringFromDate:message.date];

	// Set viewed status image
	cell.statusImage.hidden = message.viewed;
	cell.isViewed = message.viewed;
	
	// Set progress bar and label
	SCILog(@"%f %f %@", message.pausePoint, message.duration, message.name);
	if(message.duration > 0) {
		if(message.viewed && message.pausePoint > 0) {
			cell.progressLabel.hidden = NO;
			cell.progressView.hidden = NO;
			[cell.progressView setProgress: message.pausePoint / message.duration];
			cell.progressLabel.text = [NSString stringWithFormat:@"%.0f of %.0f min.", message.pausePoint / 60, message.duration / 60];
		}
		else {
			cell.progressLabel.hidden = YES;
			cell.progressView.hidden = YES;
			cell.progressLabel.text = [NSString stringWithFormat:@"0 of %.0f min.", message.duration / 60];
		}
	}
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
	SCIMessageInfo *message = [[episodesArray objectAtIndex:indexPath.section] objectAtIndex:indexPath.row];
	
	if (message && [self canPlayVideoWithAlerts])
	{
		self.tableView.allowsSelection = NO;
		[AnalyticsManager.shared trackUsage:AnalyticsEventVideoCenter properties:@{ @"description" : @"play" }];
		[[SCIVideoPlayer sharedManager] playMessage:message withCompletion:^(NSError *error) {
			self.tableView.allowsSelection = YES;
			[self didUpdate];
			if (error) {
				AlertWithTitleAndMessage(Localize(@"Video Unavailable"), Localize(@"videocenter.video.failed"));
			}
		}];
	}
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [programsArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [[episodesArray objectAtIndex:section] count];
}

- (void)didUpdate {
	// Get episode data
	programsArray = [[SCIVideophoneEngine sharedEngine] messageSubcategoriesForCategoryId:selectedCategory.categoryId];
	
	// if this channel doesnt have any subcategories, use parent category.
	if(programsArray.count == 0)
		programsArray = [[NSArray alloc] initWithObjects:[selectedCategory copy], nil];
	
	// Get episodes for this program (category) and sort them.
	episodesArray = [[NSMutableArray alloc] init];
	for (SCIMessageCategory *subcategory in programsArray) {
		NSArray *unSortedEpisodes = [[SCIVideophoneEngine sharedEngine] messagesForCategoryId:subcategory.categoryId];
		[episodesArray addObject: [unSortedEpisodes sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
			SCIMessageInfo *msg1 = (SCIMessageInfo *)obj1;
			SCIMessageInfo *msg2 = (SCIMessageInfo *)obj2;
			return [msg2.date compare:msg1.date];
		}]];
	}
	[self.tableView reloadData];
}

#pragma mark - Notifications

- (void)notifyCategoryChanged:(NSNotification *)note // SCINotificationMessageCategoryChanged
{
	SCIItemId *categoryId = [[note userInfo] objectForKey:SCINotificationKeyItemId];
	if([selectedCategory.categoryId isEqualToItemId:categoryId])
		[self didUpdate];
}

- (void)notifyMessageChanged:(NSNotification *)note // SCINotificationMessageChanged
{
	SCIItemId *messageId = [[note userInfo] objectForKey:SCINotificationKeyItemId];
	BOOL found = NO;
	for (SCIMessageInfo *info in episodesArray)
	{
		if ([info isKindOfClass:[SCIMessageInfo class]] && [info.messageId isEqualToItemId:messageId])
		{
			found = YES;
			break;
		}
	}
	if (found)
		[self didUpdate];
}

#pragma mark - View lifecycle

- (id)initWithStyle:(UITableViewStyle)style {
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[self setTitle:Localize(selectedCategory.longName)];
	
	[AnalyticsManager.shared trackUsage:AnalyticsEventVideoCenter properties:@{ @"description" : @"channel_viewed",
																				@"channel_name" : selectedCategory.longName.lowercaseString }];
	
	[self didUpdate];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.clearsSelectionOnViewWillAppear = NO;
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyMessageChanged:)
												 name:SCINotificationMessageChanged object:[SCIVideophoneEngine sharedEngine]];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyCategoryChanged:)
												 name:SCINotificationMessageCategoryChanged object:[SCIVideophoneEngine sharedEngine]];
	
	self.tableView.rowHeight = UITableViewAutomaticDimension;
	self.tableView.estimatedRowHeight = 55.0f;
}

- (void)viewWillDisappear:(BOOL)animated {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super viewWillDisappear:animated];
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

@end
