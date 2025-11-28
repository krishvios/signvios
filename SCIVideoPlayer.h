//
//  SCIVideoPlayer.h
//  ntouch
//
//  Created by Kevin Selman on 4/7/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "SCIMessageInfo.h"


@interface SCIVideoPlayer : NSObject

@property (assign) BOOL isPlaying;

@property (class, readonly) SCIVideoPlayer *sharedManager;
- (void)playMessage:(SCIMessageInfo *)message withCompletion:(void (^)(NSError *error))block;
- (void)playMovieAtURLString:(NSString *)urlString;
- (void)playMovieAtURL:(NSURL *)theURL;
- (void)playMovieAtURL:(NSURL *)theURL atPoint:(NSTimeInterval)pausePoint message:(SCIMessageInfo *)message;


@end
