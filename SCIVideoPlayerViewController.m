//
//  SCIVideoPlayerViewController.m
//  ntouch
//
//  Created by Kevin Selman on 12/29/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

#import "SCIVideoPlayerViewController.h"
#import "SCIVideophoneEngine.h"
#import "ntouch-Swift.h"

@interface SCIVideoPlayerViewController ()

@property (nonatomic, strong) AVPlayerViewController *playerVC;
@property (nonatomic, strong) VideoPlayer *avPlayer;
@property (nonatomic, strong) AVPlayerItem *avPlayerItem;
@property (nonatomic, strong) IBOutlet UIView *videoView;
@property (nonatomic) AnalyticsEvent analyticsEvent;
@property (nonatomic) BOOL playedUntilEnd;

@end

static void *SCIVideoPlayerViewControllerPlayerRateContext = &SCIVideoPlayerViewControllerPlayerRateContext;
static void *SCIVideoPlayerViewControllerVideoBoundsContext = &SCIVideoPlayerViewControllerVideoBoundsContext;

NSNotificationName const SCINotificationPlayerFinished = @"SCINotificationPlayerFinished";

@implementation SCIVideoPlayerViewController

#pragma mark - View Lifecycle

- (id)initWithURL:(NSURL *)url atTime:(NSTimeInterval)time message:(SCIMessageInfo *)message
{
	self = [super init];
	if (self) {
		self.message = message;
		self.analyticsEvent = ([[SCIVideophoneEngine sharedEngine] messageCategoryForMessage:message].type == SCIMessageTypeSignMail) ? AnalyticsEventSignMailPlayer : AnalyticsEventVideoPlayer;
		if (self.analyticsEvent != AnalyticsEventSignMailPlayer) {
			[[AnalyticsManager shared] trackUsage:self.analyticsEvent properties:@{ @"description" : [NSString stringWithFormat:@"selected_video_%@", message.messageId.string] }];
		}
		self.avPlayerItem = [AVPlayerItem playerItemWithURL:url];
		self.avPlayer = [[VideoPlayer alloc] initWithPlayerItem:self.avPlayerItem];
		self.avPlayer.analyticsEvent = self.analyticsEvent;
		[self addObserver:self forKeyPath:@"avPlayer.rate" options:NSKeyValueObservingOptionNew context:SCIVideoPlayerViewControllerPlayerRateContext];
		[self addObserver:self forKeyPath:@"playerVC.videoBounds" options:NSKeyValueObservingOptionNew context:SCIVideoPlayerViewControllerVideoBoundsContext];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(playerItemDidReachEnd:)
													 name:AVPlayerItemDidPlayToEndTimeNotification
												   object:self.avPlayer.currentItem];
		if (time) {
			[self.avPlayer seekToTime:CMTimeMakeWithSeconds(time, 30)];
		}
	}
	return self;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	
	self.view.backgroundColor = Theme.backgroundColor;
	UIBarButtonItem *doneButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Done") style:UIBarButtonItemStylePlain target:self action:@selector(doneButtonTapped:)];
	
	self.navigationItem.leftBarButtonItem = doneButton;
	self.edgesForExtendedLayout = UIRectEdgeNone;
	
	// Add Call and Reply icons to navigation bar
	UIImage *callImage = [[UIImage imageNamed:@"SolidPhoneIcon"] imageWithRenderingMode:UIImageRenderingModeAutomatic];
	UIBarButtonItem *callButton = [[UIBarButtonItem alloc] initWithImage:callImage style:UIBarButtonItemStylePlain target:self action:@selector(callButtonTapped:)];
	callButton.accessibilityLabel = @"Call";
	
	UIImage *replyImage = [[UIImage imageNamed:@"SolidDirectSignMailNavBar"] imageWithRenderingMode:UIImageRenderingModeAutomatic];
	UIBarButtonItem *replyButton = [[UIBarButtonItem alloc] initWithImage:replyImage style:UIBarButtonItemStylePlain target:self action:@selector(replyButtonTapped:)];
	replyButton.accessibilityLabel = Localize(@"Send SignMail");
	
	UIImage *deleteImage = [[UIImage imageNamed:@"TrashCan"] imageWithRenderingMode:UIImageRenderingModeAutomatic];
	UIBarButtonItem *deleteButton = [[UIBarButtonItem alloc] initWithImage:deleteImage style:UIBarButtonItemStylePlain target:self action:@selector(deleteButtonTapped:)];
	deleteButton.accessibilityLabel = Localize(@"Delete");
	
	NSMutableArray<UIBarButtonItem *> *rightBarButtonItems = [[NSMutableArray alloc] init];
	if ([[SCIVideophoneEngine sharedEngine] phoneNumberIsValid:self.message.dialString] && self.message ) {
		[rightBarButtonItems addObject:deleteButton];
		
		if ([[SCIVideophoneEngine sharedEngine] phoneNumberIsValid:self.message.dialString] && !self.message.interpreterId.length ) {
			[rightBarButtonItems addObject:replyButton];
		}
	}
	if ([[SCIVideophoneEngine sharedEngine] phoneNumberIsValid:self.message.dialString]) {
		[rightBarButtonItems addObject:callButton];
	}
	
	[self.navigationItem setRightBarButtonItems:rightBarButtonItems];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	NSError *error = nil;
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback
									 withOptions:AVAudioSessionCategoryOptionAllowBluetooth
										   error:&error];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	
	self.playerVC = [[AVPlayerViewController alloc] init];
	self.playerVC.player = self.avPlayer;
    self.playerVC.allowsPictureInPicturePlayback = YES;
	self.playerVC.delegate = self;
	UIView *playerView = self.playerVC.view;
	[self addChildViewController:self.playerVC];
	[self.videoView addSubview:playerView];
	[self.videoView sendSubviewToBack:playerView];
	[self.playerVC didMoveToParentViewController:self];
	
	CGFloat padding = 0.0;
	playerView.translatesAutoresizingMaskIntoConstraints = NO;
	[[playerView.leftAnchor constraintEqualToAnchor:self.videoView.leftAnchor constant:padding] setActive:YES];
	[[playerView.topAnchor constraintEqualToAnchor:self.videoView.topAnchor constant:padding] setActive:YES];
	[[playerView.rightAnchor constraintEqualToAnchor:self.videoView.rightAnchor constant:padding] setActive:YES];
	[[playerView.heightAnchor constraintEqualToAnchor:self.videoView.heightAnchor constant:0] setActive:YES];

	[self.avPlayer play];
}

- (BOOL)playerViewControllerShouldAutomaticallyDismissAtPictureInPictureStart:(AVPlayerViewController *)playerViewController {
	return false;
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
	if (self.isBeingDismissed || self.parentViewController.isBeingDismissed) {
		NSTimeInterval time = CMTimeGetSeconds(self.avPlayer.currentTime);
		NSDictionary *userInfo = @{ @"currentTime" : [NSNumber numberWithFloat:time] };
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationPlayerFinished object:self userInfo:userInfo];

		NSError *error = nil;
		[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord
										 withOptions:AVAudioSessionCategoryOptionAllowBluetooth
											   error:&error];
		[self logDuration];
	}
}

- (void)dealloc {
	[self removeObserver:self forKeyPath:@"avPlayer.rate"];
	[self removeObserver:self forKeyPath:@"playerVC.videoBounds"];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)doneButtonTapped:(id)sender
{
	[self dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - Helpers

- (void)logDuration
{
	if (self.analyticsEvent == AnalyticsEventVideoPlayer) {
		NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
		[formatter setNumberStyle:NSNumberFormatterPercentStyle];
		[formatter setRoundingMode:NSNumberFormatterRoundHalfUp];
		[formatter setMaximumFractionDigits:0];
		double percentagePlayed = CMTimeGetSeconds(self.avPlayer.currentTime) / [self duration];
		NSNumber *number = [NSNumber numberWithDouble:percentagePlayed];
		[AnalyticsManager.shared trackUsage:self.analyticsEvent properties:@{ @"description" : [NSString stringWithFormat:@"duration_%@", self.playedUntilEnd ? @"100%" : [formatter stringFromNumber:number]] }];
		self.playedUntilEnd = NO;
	}
}

#pragma mark - Accessors

- (NSTimeInterval)currentTime {
	return (NSTimeInterval)self.avPlayer.currentTime.value;
}

- (double)duration
{
	if (self.avPlayer.currentItem.status == AVPlayerItemStatusReadyToPlay) {
		return CMTimeGetSeconds(self.avPlayer.currentItem.asset.duration);
	} else {
		return 0.f;
	}
}

#pragma mark - Calling Methods

- (IBAction)callButtonTapped:(id)sender {
	[self dismissViewControllerAnimated:YES completion:^{
		[AnalyticsManager.shared trackUsage:AnalyticsEventSignMailPlayer properties: @{ @"description" : @"call" }];
		SCIDialSource dialSource = (self.message.typeId == SCIMessageTypeDirectSignMail ? SCIDialSourceDirectSignMail : SCIDialSourceSignMail);
		[SCICallController.shared makeOutgoingCallTo:self.message.dialString dialSource:dialSource];
	}];
}

- (IBAction)replyButtonTapped:(id)sender {
	[self dismissViewControllerAnimated:YES completion:^{
		[AnalyticsManager.shared trackUsage:AnalyticsEventSignMailPlayer properties:@{ @"description": @"reply" }];
		SCIDialSource dialSource = (self.message.typeId == SCIMessageTypeDirectSignMail ? SCIDialSourceDirectSignMail : SCIDialSourceSignMail);
		
		NSString *initiationSource = (self.message.typeId == SCIMessageTypeDirectSignMail ? @"reply_direct_signmail_player" : @"reply_signmail_player");
		
		[AnalyticsManager.shared trackUsage:AnalyticsEventSignMailInitiationSource properties: @{ @"description": initiationSource }];
		
		[SCICallController.shared makeOutgoingCallTo:self.message.dialString dialSource:dialSource skipToSignMail:true];
	}];
}

- (IBAction)deleteButtonTapped:(id)sender {
	[AnalyticsManager.shared trackUsage:AnalyticsEventSignMailPlayer properties: @{ @"description" : @"delete" }];
	NSString *alertTitle = Localize(@"Delete SignMail");
	NSString *alertMessage = Localize(@"signmail.delete.confirm");
	
	UIAlertController *confirmDeleteAlert = [UIAlertController alertControllerWithTitle:alertTitle message:alertMessage preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:nil];
	
	UIAlertAction *deleteAction = [UIAlertAction actionWithTitle:Localize(@"Delete") style:UIAlertActionStyleDefault handler:^(UIAlertAction *action)
								   {
									   [AnalyticsManager.shared trackUsage:AnalyticsEventSignMailPlayer properties: @{ @"description" : @"signmail_deleted" }];
									   [[SCIVideophoneEngine sharedEngine] deleteMessage:self.message];
									   
									   [self dismissViewControllerAnimated:YES completion:nil];
								   }];
	
	[confirmDeleteAlert addAction:cancelAction];
	[confirmDeleteAlert addAction:deleteAction];
	
	[self presentViewController:confirmDeleteAlert animated:YES completion:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if (context == SCIVideoPlayerViewControllerPlayerRateContext) {
		NSString *description;
		if (self.avPlayer.rate == 1) {
			description = @"playing";
		} else if (self.avPlayer.rate == 0) {
			description = @"paused";
		}
		
		if (description) {
			[AnalyticsManager.shared trackUsage:self.analyticsEvent properties:@{ @"description" : description }];
		}
	}
	else if (context == SCIVideoPlayerViewControllerVideoBoundsContext) {
		if (CGSizeEqualToSize(self.playerVC.contentOverlayView.bounds.size, [UIScreen mainScreen].bounds.size)) {
			[AnalyticsManager.shared trackUsage:self.analyticsEvent properties:@{ @"description" : @"full_screen" }];
		}
	}
	else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

#pragma mark - Notifications

- (void)playerItemDidReachEnd:(NSNotification *)notification
{
	self.playedUntilEnd = YES;
}

@end
