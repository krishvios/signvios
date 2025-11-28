//
//  SCIMessageManager+UserInterfaceProperties.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageManager+UserInterfaceProperties.h"
#import "SCIMessageManagerInternal.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIMessageCategory+Additions.h"
#import "SCIMessageInfo.h"
#import <objc/runtime.h>

@interface SCIMessageManagerKeyValueObserver : NSObject
- (void)startObservingVideophoneEngineUserInterfaceProperties;
- (void)stopObservingVideophoneEngineUserInterfaceProperties;
@end

@interface SCIMessageManager (UserInterfacePropertiesPrivate)
@property (nonatomic, strong, readwrite) SCIMessageManagerKeyValueObserver *keyValueObserver;
- (void)videophoneEngineDidChangeValueForLastChannelMessageViewTime;
- (void)videophoneEngineDidChangeValueForLastMessageViewTime;
- (void)videophoneEngineDidChangeValueForVideoMessageEnabled;
@end

@implementation SCIMessageManager (UserInterfaceProperties)

#pragma mark - Public API: Observing

- (void)startObservingUserInterfaceProperties
{
	self.keyValueObserver = [[SCIMessageManagerKeyValueObserver alloc] init];
	[self.keyValueObserver startObservingVideophoneEngineUserInterfaceProperties];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifySignMailChanged:)
												 name:SCIMessageManagerNotificationSignMailChanged
											   object:[SCIMessageManager sharedManager]];
}

- (void)stopObservingUserInterfaceProperties
{
	[self.keyValueObserver stopObservingVideophoneEngineUserInterfaceProperties];
	self.keyValueObserver = nil;
	
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCIMessageManagerNotificationSignMailChanged
												  object:[SCIMessageManager sharedManager]];
}

#pragma mark - Public API: User Interface Properties: General

- (BOOL)enabled
{
	return [[SCIVideophoneEngine sharedEngine] videoMessageEnabled];
}

#pragma mark - Public API: User Interface Properties: Video Center

- (BOOL)hasNewVideos
{
	BOOL res = NO;
	NSDate *missedVideosReference = [[SCIVideophoneEngine sharedEngine] lastChannelMessageViewTime];
	if (missedVideosReference) {
		res = ([self.latestVideoTime compare:missedVideosReference] == NSOrderedDescending);
	}
	return res;
}

- (void)setHasNewVideos:(BOOL)hasNewVideos
{
	[self setHasNewVideosPrimitive:hasNewVideos];
}

- (void)setHasNewVideosPrimitive:(BOOL)hasNewVideos
{
	NSDate *lastChannelMessageViewTime = nil;
	if (hasNewVideos) {
		lastChannelMessageViewTime = [NSDate dateWithTimeIntervalSince1970:0.0f];
	} else {
		lastChannelMessageViewTime = self.latestVideoTime;
	}
	// Use the primitive setter to avoid infinite recursion via KVO:
	[[SCIVideophoneEngine sharedEngine] setLastChannelMessageViewTimePrimitive:lastChannelMessageViewTime];
}

#pragma mark - Helpers: User Interface Properties: Video Center

- (NSDate *)latestVideoTime
{
	NSDate *latest = [NSDate dateWithTimeIntervalSince1970:0];
	NSArray *channels = self.channels;
	for (SCIMessageCategory *category in channels) {
		for (SCIMessageInfo *message in category.descendantMessages) {
			if ([message.date compare:latest] == NSOrderedDescending) latest = message.date;
		}
	}
	return latest;
}

#pragma mark - Public API: User Interface Properties: SignMail

- (BOOL)hasNewSignMail
{
	BOOL res = NO;
	NSDate *missedSignMailReference = [[SCIVideophoneEngine sharedEngine] lastMessageViewTime];
	if (missedSignMailReference) {
		res = ([self.latestSignMailTime compare:missedSignMailReference] == NSOrderedDescending);
	}
	return res;
}
- (void)setHasNewSignMail:(BOOL)hasNewSignMail
{
	[self setHasNewSignMailPrimitive:hasNewSignMail];
}

- (void)setHasNewSignMailPrimitive:(BOOL)hasNewSignMail
{
	NSDate *lastMessageViewTime = nil;
	if (hasNewSignMail) {
		lastMessageViewTime = [NSDate dateWithTimeIntervalSince1970:0.0f];
	} else {
		lastMessageViewTime = self.latestSignMailTime;
	}
	//Use the primitive setter to avoid infinite recursion via KVO:
	[[SCIVideophoneEngine sharedEngine] setLastMessageViewTimePrimitive:lastMessageViewTime];
}

+ (NSSet *)keyPathsForValuesAffectingHasUnviewedSignMail
{
	return [NSSet setWithObject:SCIMessageManagerKeyUnviewedSignMailCount];
}

- (BOOL)hasUnviewedSignMail
{
	return (self.signMailCategory.unviewedChildMessages.count > 0);
}

+ (NSSet *)keyPathsForValuesAffectingHasSignMail
{
	return [NSSet setWithObject:SCIMessageManagerKeySignMailCount];
}

- (BOOL)hasSignMail
{
	return (self.signMailCategory.childMessages.count > 0);
}

- (NSUInteger)unviewedSignMailCount
{
	return self.signMailCategory.unviewedChildMessages.count;
}

- (NSUInteger)signMailCount
{
	return self.signMailCategory.childMessages.count;
}

#pragma mark - Helpers: User Interface Properties: SignMail

- (NSDate *)latestSignMailTime
{
	NSDate *latest = [NSDate dateWithTimeIntervalSince1970:0];
	NSArray *signMail = self.signMail;
	for (SCIMessageInfo *message in signMail) {
		if ([message.date compare:latest] == NSOrderedDescending) latest = message.date;
	}
	return latest;
}

#pragma mark - Notifications

- (void)notifySignMailChanged:(NSNotification *)note // SCIMessageManagerNotificationSignMailChanged
{
	[self willChangeValueForKey:SCIMessageManagerKeySignMailCount];
	[self willChangeValueForKey:SCIMessageManagerKeyUnviewedSignMailCount];
	[self didChangeValueForKey:SCIMessageManagerKeySignMailCount];
	[self didChangeValueForKey:SCIMessageManagerKeyUnviewedSignMailCount];
}

@end

NSString * const SCIMessageManagerKeyEnabled = @"enabled";
NSString * const SCIMessageManagerKeyHasNewVideos = @"hasNewVideos";
NSString * const SCIMessageManagerKeyHasNewSignMail = @"hasNewSignMail";
NSString * const SCIMessageManagerKeyHasSignMail = @"hasSignMail";
NSString * const SCIMessageManagerKeySignMailCount = @"signMailCount";
NSString * const SCIMessageManagerKeyHasUnviewedSignMail = @"hasUnviewedSignMail";
NSString * const SCIMessageManagerKeyUnviewedSignMailCount = @"unviewedSignMailCount";

@implementation SCIMessageManager (UserInterfacePropertiesPrivate)
static int keyValueObserverAssociatedObjectKey = 0;
- (SCIMessageManagerKeyValueObserver *)keyValueObserver
{
	return objc_getAssociatedObject(self, &keyValueObserverAssociatedObjectKey);
}
- (void)setKeyValueObserver:(SCIMessageManagerKeyValueObserver *)keyValueObserver
{
	objc_setAssociatedObject(self, &keyValueObserverAssociatedObjectKey, nil, OBJC_ASSOCIATION_ASSIGN);
	if (keyValueObserver)
		objc_setAssociatedObject(self, &keyValueObserverAssociatedObjectKey, keyValueObserver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (void)videophoneEngineDidChangeValueForLastChannelMessageViewTime
{
	[self willChangeValueForKey:SCIMessageManagerKeyHasNewVideos];
	[self didChangeValueForKey:SCIMessageManagerKeyHasNewVideos];
}
- (void)videophoneEngineDidChangeValueForLastMessageViewTime
{
	[self willChangeValueForKey:SCIMessageManagerKeyHasNewSignMail];
	[self didChangeValueForKey:SCIMessageManagerKeyHasNewSignMail];
}
- (void)videophoneEngineDidChangeValueForVideoMessageEnabled
{
	[self willChangeValueForKey:SCIMessageManagerKeyEnabled];
	[self didChangeValueForKey:SCIMessageManagerKeyEnabled];
}
@end

static int SCIMessageManagerUserInterfacePropertiesKVOContext = 0;

@implementation SCIMessageManagerKeyValueObserver

- (void)startObservingVideophoneEngineUserInterfaceProperties
{
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine addObserver:self
			 forKeyPath:SCIKeyLastChannelMessageViewTime
				options:0
				context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine addObserver:self
			 forKeyPath:SCIKeyLastMessageViewTime
				options:0
				context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine addObserver:self
			 forKeyPath:SCIKeyVideoMessageEnabled
				options:0
				context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
}

- (void)stopObservingVideophoneEngineUserInterfaceProperties
{
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine removeObserver:self
				forKeyPath:SCIKeyLastChannelMessageViewTime
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine removeObserver:self
				forKeyPath:SCIKeyLastMessageViewTime
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine removeObserver:self
				forKeyPath:SCIKeyVideoMessageEnabled
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if (context != &SCIMessageManagerUserInterfacePropertiesKVOContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
	
	if (object == [SCIVideophoneEngine sharedEngine]) {
		if ([keyPath isEqualToString:SCIKeyLastChannelMessageViewTime]) {
			[[SCIMessageManager sharedManager] videophoneEngineDidChangeValueForLastChannelMessageViewTime];
		}
		else if ([keyPath isEqualToString:SCIKeyLastMessageViewTime]) {
			[[SCIMessageManager sharedManager] videophoneEngineDidChangeValueForLastMessageViewTime];
		}
		else if ([keyPath isEqualToString:SCIKeyVideoMessageEnabled]) {
			[[SCIMessageManager sharedManager] videophoneEngineDidChangeValueForVideoMessageEnabled];
		}
	}
}

- (void)dealloc
{
	// Remove observers
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine removeObserver:self
				forKeyPath:SCIKeyLastChannelMessageViewTime
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine removeObserver:self
				forKeyPath:SCIKeyLastMessageViewTime
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];
	[engine removeObserver:self
				forKeyPath:SCIKeyVideoMessageEnabled
				   context:&SCIMessageManagerUserInterfacePropertiesKVOContext];

}

@end








