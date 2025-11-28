//
//  SCICallListManager+UserInterfaceProperties.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/8/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallListManager+UserInterfaceProperties.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import <objc/runtime.h>

@interface SCICallListManagerKeyValueObserver : NSObject
- (void)startObservingVideophoneEngineUserInterfaceProperties;
- (void)stopObservingVideophoneEngineUserInterfaceProperties;
@end

@interface SCICallListManager (UserInterfacePropertiesPrivate)
@property (nonatomic, strong, readwrite) SCICallListManagerKeyValueObserver *keyValueObserver;
- (void)videophoneEngineDidChangeValueForLastMissedViewTime;
@end

@implementation SCICallListManager (UserInterfaceProperties)

#pragma mark - Public API: Observing

- (void)startObservingUserInterfaceProperties
{
	self.keyValueObserver = [[SCICallListManagerKeyValueObserver alloc] init];
	[self.keyValueObserver startObservingVideophoneEngineUserInterfaceProperties];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyCallListItemsChanged:)
												 name:SCINotificationCallListItemsChanged
											   object:self];
}

- (void)stopObservingUserInterfaceProperties
{
	[self.keyValueObserver stopObservingVideophoneEngineUserInterfaceProperties];
	self.keyValueObserver = nil;
	
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationCallListItemsChanged
												  object:self];
}

#pragma mark - Public API: User Interface Properties

+ (NSSet *)keyPathsForValuesAffectingHasUnviewedMissedCalls
{
	return [NSSet setWithObject:SCICallListManagerKeyUnviewedMissedCallCount];
}

- (BOOL)hasUnviewedMissedCalls
{
	return (self.unviewedMissedCallCount > 0);
}

- (void)setHasUnviewedMissedCalls:(BOOL)hasUnviewedMissedCalls
{
	[self setHasUnviewedMissedCallsPrimitive:hasUnviewedMissedCalls];
}

- (void)setHasUnviewedMissedCallsPrimitive:(BOOL)hasUnviewedMissedCalls
{
	NSDate *lastMissedCallViewTime = nil;
	if (hasUnviewedMissedCalls) {
		lastMissedCallViewTime = [NSDate dateWithTimeIntervalSince1970:0.0f];
	} else {
		lastMissedCallViewTime = [[SCICallListManager sharedManager] latestMissedCallTime];
	}
	// Use the primitive setter to avoid infinite recursion via KVO:
	[[SCIVideophoneEngine sharedEngine] setLastMissedViewTimePrimitive:lastMissedCallViewTime];
}

- (NSUInteger)unviewedMissedCallCount
{
	NSUInteger res = 0;
	NSDate *missedCallsReference = [[SCIVideophoneEngine sharedEngine] lastMissedViewTime];
	if (missedCallsReference) {
		res = [self missedCallsAfter:missedCallsReference];
	}
	return res;
}

#pragma mark - Notifications

- (void)notifyCallListItemsChanged:(NSNotification *)note // SCINotificationCallListItemsChanged
{
	[self willChangeValueForKey:SCICallListManagerKeyUnviewedMissedCallCount];
	[self didChangeValueForKey:SCICallListManagerKeyUnviewedMissedCallCount];
}

@end

NSString * const SCICallListManagerKeyHasUnviewedMissedCalls = @"hasUnviewedMissedCalls";
NSString * const SCICallListManagerKeyUnviewedMissedCallCount = @"unviewedMissedCallCount";

@implementation SCICallListManager (UserInterfacePropertiesPrivate)
static int keyValueObserverAssociatedObjectKey = 0;
- (SCICallListManagerKeyValueObserver *)keyValueObserver
{
	return objc_getAssociatedObject(self, &keyValueObserverAssociatedObjectKey);
}
- (void)setKeyValueObserver:(SCICallListManagerKeyValueObserver *)keyValueObserver
{
	objc_setAssociatedObject(self, &keyValueObserverAssociatedObjectKey, nil, OBJC_ASSOCIATION_ASSIGN);
	if (keyValueObserver)
		objc_setAssociatedObject(self, &keyValueObserverAssociatedObjectKey, keyValueObserver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
- (void)videophoneEngineDidChangeValueForLastMissedViewTime
{
	[self willChangeValueForKey:SCICallListManagerKeyUnviewedMissedCallCount];
	[self didChangeValueForKey:SCICallListManagerKeyUnviewedMissedCallCount];
}
@end

static int SCICallListManagerKeyValueObserverKVOContext = 0;

@implementation SCICallListManagerKeyValueObserver

- (void)startObservingVideophoneEngineUserInterfaceProperties
{
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine addObserver:self
			 forKeyPath:SCIKeyLastMissedViewTime
				options:0
				context:&SCICallListManagerKeyValueObserverKVOContext];
}

- (void)stopObservingVideophoneEngineUserInterfaceProperties
{
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine removeObserver:SCIKeyLastMissedViewTime
				forKeyPath:SCIKeyLastMissedViewTime
				   context:&SCICallListManagerKeyValueObserverKVOContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if (context != &SCICallListManagerKeyValueObserverKVOContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
		
	if (object == [SCIVideophoneEngine sharedEngine]) {
		if ([keyPath isEqualToString:SCIKeyLastMissedViewTime]) {
			[[SCICallListManager sharedManager] videophoneEngineDidChangeValueForLastMissedViewTime];
		}
	}
}

- (void)dealloc
{
	// Remove observers
	SCIVideophoneEngine *engine = [SCIVideophoneEngine sharedEngine];
	[engine removeObserver:SCIKeyLastMissedViewTime
				forKeyPath:SCIKeyLastMissedViewTime
				   context:&SCICallListManagerKeyValueObserverKVOContext];
	
}


@end
