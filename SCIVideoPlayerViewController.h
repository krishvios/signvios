//
//  SCIVideoPlayerViewController.h
//  ntouch
//
//  Created by Kevin Selman on 12/29/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>
#import "SCIMessageInfo.h"

typedef NS_ENUM(NSUInteger, SCIVideoPlayerEvent) {
	SCIVideoPlayerEventInitialized = 0,
	SCIVideoPlayerEventFinished,
	SCIVideoPlayerEventError
};

@interface SCIVideoPlayerViewController : UIViewController <AVPlayerViewControllerDelegate>

@property (nonatomic, strong) SCIMessageInfo *message;

- (id)initWithURL:(NSURL *)url atTime:(NSTimeInterval)time message:(SCIMessageInfo *)message;
- (NSTimeInterval)currentTime;

@end

extern NSNotificationName const SCINotificationPlayerFinished;
