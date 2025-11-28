//
//  NotificationController.m
//   WatchKit Extension
//
//  Created by Kevin Selman on 5/15/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "NotificationController.h"
#import <os/log.h>
#import <WatchKit_Extension-Swift.h>

@interface NotificationController() <SignMailDownloadDelegate>

@property (nonatomic, strong) IBOutlet WKInterfaceImage *contactImageView;
@property (nonatomic, strong) IBOutlet WKInterfaceLabel *alertLabel;
@property (nonatomic, strong) IBOutlet WKInterfaceMovie *alertMovie;
@property (nonatomic, strong) IBOutlet WKInterfaceLabel *alertProgressLabel;

@property (nonatomic, strong) NSURL *contactImageURL;
@property (nonatomic, strong) NSURL *signMailVideoURL;
@property (nonatomic, strong) NSURL *signMailPosterImageURL;
@property (nonatomic, strong) NSString *alertMessage;
@property (nonatomic, strong) NSURLSession *urlSession;

@end

@implementation NotificationController

#pragma mark - View lifecycle

- (instancetype)init {
	self = [super init];
	if (self){
		self.urlSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration] delegate:self delegateQueue:NSOperationQueue.mainQueue];
	}
	return self;
}

- (void)willActivate {
	// This method is called when watch view controller is about to be visible to user
	[super willActivate];
	
	if (self.contactImageURL) {
		NSURLSessionDownloadTask *downloadTask = [self.urlSession
												  downloadTaskWithURL:self.contactImageURL
												  completionHandler:^(NSURL *location,
																	  NSURLResponse *response,
																	  NSError *error) {
			
			UIImage *image = [UIImage imageWithData: [NSData dataWithContentsOfURL:location]];
			UIGraphicsBeginImageContextWithOptions(image.size, NO, 1.0);
			CGRect bounds = { CGPointZero, image.size };
			[[UIBezierPath bezierPathWithRoundedRect:bounds cornerRadius:image.size.width / 2] addClip];
			[image drawInRect:bounds];
			UIImage *roundedImage = UIGraphicsGetImageFromCurrentImageContext();
			UIGraphicsEndImageContext();
			
			dispatch_async(dispatch_get_main_queue(), ^{
				[self.contactImageView setImage:roundedImage];
			});
		}];
		[downloadTask resume];
	}
	else {
		[self.contactImageView setHidden:YES];
	}
	
	if (self.signMailVideoURL) {
		[self.alertMovie setHidden:YES];
		
		[SignMailDownloadManager.shared downloadSignMailFrom:self.signMailVideoURL
												   messageId:[NSUUID UUID].UUIDString
										allowsCellularAccess:YES
													delegate:self];
		[self.alertProgressLabel setHidden:NO];
	}
	
	if (self.signMailPosterImageURL) {
		NSURLSessionDownloadTask *downloadTask = [self.urlSession
												  downloadTaskWithURL:self.signMailPosterImageURL
												  completionHandler:^(NSURL *location,
																	  NSURLResponse *response,
																	  NSError *error) {
			WKImage *image = [WKImage imageWithImageData:[NSData dataWithContentsOfURL:location]];

			dispatch_async(dispatch_get_main_queue(), ^{
				[self.alertMovie setPosterImage:image];
			});
		}];
		[downloadTask resume];
	}
	
	if (self.alertMessage) {
		[self.alertLabel setText:self.alertMessage];
	}
}

- (void)didDeactivate {
	[super didDeactivate];
}

- (void)didAppear {
	[super didAppear];
}

- (void)didReceiveNotification:(UNNotification *)notification {
	[super didReceiveNotification:notification];
//	os_log_t customLog = os_log_create("com.sorenson.ntouch", "apns");
	// Set Alert label
	self.alertMessage = notification.request.content.body;
	
	// Remove Return Call and View actions
	NSMutableArray *modifiedActions = [[NSMutableArray alloc] init];
	for (UNNotificationAction *action in self.notificationActions) {
		if ([action.identifier isEqualToString:@"RETURN_CALL_IDENTIFIER"] ||
			[action.identifier isEqualToString:@"RETURN_SIGNMAIL_CALL_IDENTIFIER"] ||
			[action.identifier isEqualToString:@"VIEW_SIGNMAIL_IDENTIFIER"]) {
			continue;
		}
		else {
			[modifiedActions addObject:action];
		}
	}
	
	self.notificationActions = modifiedActions;
	
	NSDictionary *requestDict = notification.request.content.userInfo[@"ntouch"];
	NSString *categoryID = notification.request.content.categoryIdentifier;
	
	if (requestDict && notification.request.content.categoryIdentifier.length) {
		if ([categoryID isEqualToString: @"SIGNMAIL_CATEGORY"]) {
			NSURL *videoUrl = [NSURL URLWithString:requestDict[@"video-url"]];
			if (videoUrl) {
				self.signMailVideoURL = videoUrl;
//				os_log(customLog, "video url: %{public}@", videoUrl.absoluteURL);
			}
			NSURL *imageUrl = [NSURL URLWithString:requestDict[@"image-url"]];
			if (imageUrl) {
				self.signMailPosterImageURL = imageUrl;
//				os_log(customLog, "image url: %{public}@", imageUrl.absoluteURL);
			}
		}
			
		else if ([categoryID isEqualToString: @"MISSED_CALL_CATEGORY"]) {
			NSURL *imageUrl = [NSURL URLWithString:requestDict[@"image-url"]];
			if (imageUrl) {
				self.contactImageURL = imageUrl;
//				os_log(customLog, "image url: %{public}@", imageUrl.absoluteURL);
			}
		}
	}
}

- (void)didReceiveNotification:(UNNotification *)notification withCompletion:(void(^)(WKUserNotificationInterfaceType interface)) completionHandler {
}

#pragma mark - NSURLSession delegate

- (void)URLSession:(NSURLSession *)session didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
 completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential * _Nullable credential))completionHandler {
	NSURLCredential *credential = [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
	completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
}

#pragma mark - SignMailDownloadManager delegate

- (void)downloadDidFinishDownloadingToURL:(NSURL * _Nonnull)url {
	[self.alertMovie setHidden:NO];
	[self.alertProgressLabel setHidden:YES];
	[self.alertMovie setMovieURL:url];
}

- (void)downloadFailedWithError:(NSError * _Nullable)withError {
	[self.alertProgressLabel setText: @"Error downloading."];
}

- (void)downloadProgressedWithReceivedBytes:(int64_t)receivedBytes ofTotalBytes:(int64_t)ofTotalBytes {
	NSNumber *percentComplete = @(100 * receivedBytes / ofTotalBytes);
	[self.alertProgressLabel setText:[NSString stringWithFormat:@"%@%%", percentComplete.stringValue]];
}

@end
