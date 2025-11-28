//
//  SCIMessageManager+UserInterfaceProperties.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageManager.h"

@interface SCIMessageManager (UserInterfaceProperties)

//Observing
- (void)startObservingUserInterfaceProperties;
- (void)stopObservingUserInterfaceProperties;

//User Interface Properties: General
@property (nonatomic, assign, readonly) BOOL enabled;

//User Interface Properties: Video Center
@property (nonatomic, assign, readwrite) BOOL hasNewVideos;
- (void)setHasNewVideosPrimitive:(BOOL)hasNewVideos;

//User Interface Properties: Sign Mail
@property (nonatomic, assign, readwrite) BOOL hasNewSignMail;
- (void)setHasNewSignMailPrimitive:(BOOL)hasNewSignMail;
@property (nonatomic, assign, readonly) BOOL hasUnviewedSignMail;
@property (nonatomic, assign, readonly) BOOL hasSignMail;
- (NSUInteger)unviewedSignMailCount;
- (NSUInteger)signMailCount;

@end

extern NSString * const SCIMessageManagerKeyEnabled;
extern NSString * const SCIMessageManagerKeyHasNewVideos;
extern NSString * const SCIMessageManagerKeyHasNewSignMail;
extern NSString * const SCIMessageManagerKeyHasSignMail;
extern NSString * const SCIMessageManagerKeyHasUnviewedSignMail;
extern NSString * const SCIMessageManagerKeyUnviewedSignMailCount;
extern NSString * const SCIMessageManagerKeySignMailCount;
