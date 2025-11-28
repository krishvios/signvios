//
//  SCIVideoPlayer.m
//  ntouch
//
//  Created by Kevin Selman on 4/7/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCIVideoPlayer.h"
#import "SCIVideophoneEngine.h"
#import "AppDelegate.h"
#import "Alert.h"
#import "Utilities.h"
#import "SCIVideoPlayerViewController.h"
#import "SCIMessageCategory.h"
#import "SCICoreVersion.h"
#import "ntouch-Swift.h"
#import "SCINotifications.h"


@interface SCIVideoPlayer ()

@property (nonatomic, strong) SCIVideoPlayerViewController *playerViewController;
@property (nonatomic, strong) SCIMessageInfo *message;

@property (copy) void (^VideoPlayerDidFinishCompletionBlock)(NSError *error);

@end

@implementation SCIVideoPlayer

#pragma mark - Singleton

+ (SCIVideoPlayer *)sharedManager
{
	static SCIVideoPlayer *sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[SCIVideoPlayer alloc] init];
	});
	return sharedManager;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {

	}
	return self;
}

- (void)dealloc
{
	self.playerViewController = nil;
}

#pragma mark - MPMoviePlayer

- (void)playMessage:(SCIMessageInfo *)messageInfo withCompletion:(void (^)(NSError *error))block {
	self.message = messageInfo;
	
	[[SCIVideophoneEngine sharedEngine] downloadURLListforMessage:self.message completion:^(NSString *downloadURL, NSString *viewId, NSError *sendError) {
	
		if (sendError) {
			block(sendError);
		}
		else {
			// Update ViewId in SCIMessageInfo object.  This is required for setting Pause Point.
			self.message.viewId = viewId;
			
			NSTimeInterval pausePoint = 0;
			if (self.message.pausePoint && self.message.pausePoint < self.message.duration) {
				pausePoint = self.message.pausePoint;
			}
			
			NSURL *fileURL = [NSURL URLWithString:downloadURL];
			NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: fileURL];
			[request setHTTPMethod: @"HEAD"];
			NSURLSession *session = [NSURLSession sharedSession];
			
			[[session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
				
				NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
				dispatch_async( dispatch_get_main_queue(), ^{
					
					if (!error && httpResponse.statusCode == 200) {
						self.VideoPlayerDidFinishCompletionBlock = block;
						[self playMovieAtURL:fileURL atPoint:pausePoint message:messageInfo];
					}
					else if (block) {
						// Error recieved from remote server.
						block([NSError errorWithDomain:@"HTTP error" code:httpResponse.statusCode userInfo:nil]);
					}
				});
				
			}] resume];
		}
	}];
}

- (void)playMovieAtURLString:(NSString *)urlString
{
	NSURL *fileURL = [NSURL URLWithString:urlString];
	[self playMovieAtURL:fileURL];
}

- (void)playMovieAtURL:(NSURL *)theURL
{
	[self playMovieAtURL:theURL atPoint:0 message:nil];
}

- (void)playMovieAtURL:(NSURL *)theURL atPoint:(NSTimeInterval)pausePoint message:(SCIMessageInfo *)message
{
	SCILog(@"playMovieAtURL %@", theURL);
	
	self.playerViewController = [[SCIVideoPlayerViewController alloc] initWithURL:theURL atTime:pausePoint message:message];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyVideoFinished:)
												 name:SCINotificationPlayerFinished
											   object:self.playerViewController];
	
	[[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(reachabilityChanged:)
												 name: SCINetworkReachabilityDidChange object: nil];
	
	UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:self.playerViewController];
	navController.modalPresentationStyle = UIModalPresentationOverFullScreen;
	[appDelegate.topViewController presentViewController:navController animated:YES completion:^{
		self.isPlaying = YES;
	}];
}

#pragma mark - Notifications

- (void)notifyVideoFinished:(NSNotification *)notification
{
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationPlayerFinished
												  object:[notification object]];
	
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINetworkReachabilityDidChange
												  object:nil];
	if (self.message)
	{
		[[SCIVideophoneEngine sharedEngine] markMessage:self.message viewed:YES];

		NSNumber *seconds = notification.userInfo[@"currentTime"];
		if (seconds) {
			[[SCIVideophoneEngine sharedEngine] markMessage:self.message pausePoint:[seconds floatValue]];
		}
	}
	
	if (self.VideoPlayerDidFinishCompletionBlock) {
		self.VideoPlayerDidFinishCompletionBlock(NULL);
	}
	
	self.isPlaying = NO;
	self.playerViewController = nil;
	self.message = nil;
}

- (void)reachabilityChanged: (NSNotification *)note {
	if (note.userInfo) {
		NSNumber *netType = [note.userInfo objectForKey:SCINetworkReachabilityKey];
		if (netType.intValue == SCINetworkTypeNone && self.isPlaying == YES) {
			AlertWithTitleAndMessage(Localize(@"Can’t Play Video"), Localize(@"Network not available."));
		}
	}
}

@end
