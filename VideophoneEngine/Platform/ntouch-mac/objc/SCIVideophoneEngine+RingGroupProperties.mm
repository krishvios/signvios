//
//  SCIVideophoneEngine+RingGroupProperties.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine+RingGroupProperties.h"
#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "IstiRingGroupManager.h"
#import "SCIRingGroupInfo.h"
#import "SCIRingGroupInfo+Cpp.h"
#import "SCIUserAccountInfo.h"
#import "SCIUserAccountInfo.h"

@interface SCIVideophoneEngine (RingGroupPropertiesPrivate)
- (IstiRingGroupManager *)ringGroupManager;
@end

@implementation SCIVideophoneEngine (RingGroupProperties)

#pragma mark - Public API - Observing

- (void)startObservingRingGroupProperties
{
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyChanged:)
												 name:SCINotificationRingGroupChanged
											   object:self];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyCreated:)
												 name:SCINotificationRingGroupCreated
											   object:self];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyRemoved:)
												 name:SCINotificationRingGroupRemoved
											   object:self];
}

- (void)stopObservingRingGroupProperties
{
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationRingGroupChanged
												  object:self];
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationRingGroupCreated
												  object:self];
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationRingGroupRemoved
												  object:self];
}

#pragma mark - Public API - 

+ (NSSet *)keyPathsForValuesAffectingInRingGroup
{
	return [NSSet setWithObject:SCIKeyRingGroupInfos];
}
- (BOOL)inRingGroup
{
	return (self.ringGroupManager->ItemCountGet() > 0);
}

- (int)ringGroupCount
{
    return self.ringGroupManager->ItemCountGet();
}

- (NSArray *)ringGroupInfos
{
	NSMutableArray *mutableRes = nil;
	
	int count = self.ringGroupManager->ItemCountGet();
	mutableRes = [NSMutableArray arrayWithCapacity:(NSUInteger)count];
	for (int i = 0; i < count; i++) {
		std::string localNumber;
		std::string tollFreeNumber;
		std::string description;
		IstiRingGroupManager::ERingGroupMemberState state;
		self.ringGroupManager->ItemGetByIndex(i, &localNumber, &tollFreeNumber, &description, &state);
		SCIRingGroupInfo *ringGroupInfo = [SCIRingGroupInfo ringGroupInfoWithLocalNumber:&localNumber
																		  tollFreeNumber:&tollFreeNumber
																			 description:&description
																				   state:SCIRingGroupInfoStateFromERingGroupMemberState(state)];
		[mutableRes addObject:ringGroupInfo];
	}
	
	return [mutableRes copy];
}
- (void)postValueChangeRingGroupInfos
{
	[self willChangeValueForKey:SCIKeyRingGroupInfos];
	[self didChangeValueForKey:SCIKeyRingGroupInfos];
}

+ (NSSet *)keyPathsForValuesAffectingOtherRingGroupInfos
{
	return [NSSet setWithObjects:SCIKeyRingGroupInfos, nil];
}
- (NSArray *)otherRingGroupInfos
{
	NSArray *res = nil;
	
	
	NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
	NSArray *ringGroupInfos = self.ringGroupInfos;
	for (SCIRingGroupInfo *ringGroupInfo in ringGroupInfos) {
		if (![self ringGroupInfoIsThisEndpoint:ringGroupInfo]) {
			[mutableRes addObject:ringGroupInfo];
		}
	}
	res = [mutableRes copy];
	
	return res;
}

- (NSString *)preferredRingGroupNumber
{
	NSString *res = nil;
	SCICallerIDNumberType numberType = self.callerIDNumberType;
	switch (numberType) {
		case SCICallerIDNumberTypeTollFree: {
			res = self.userAccountInfo.ringGroupTollFreeNumber;
		} break;
		default:
		case SCICallerIDNumberTypeLocal: {
			res = self.userAccountInfo.ringGroupLocalNumber;
		} break;
	}
	return res;
}

- (NSArray *)ringGroupNumbers
{
	NSMutableArray *mutableRes = [NSMutableArray arrayWithCapacity:2];
	NSString *ringGroupLocalNumber = self.userAccountInfo.ringGroupLocalNumber;
	NSString *ringGroupTollFreeNumber = self.userAccountInfo.ringGroupTollFreeNumber;
	if (ringGroupLocalNumber.length) [mutableRes addObject:ringGroupLocalNumber];
	if (ringGroupTollFreeNumber.length) [mutableRes addObject:ringGroupTollFreeNumber];
	
	// if the userAccountInfo has not yet been reloaded, we need to use the local numbers for now.
	if (ringGroupLocalNumber.length == 0 &&
		ringGroupTollFreeNumber.length == 0) {
		NSString *endpointLocalNumber = self.userAccountInfo.localNumber;
		NSString *endpointTollfreeNumber = self.userAccountInfo.tollFreeNumber;
		if (endpointLocalNumber.length) [mutableRes addObject:endpointLocalNumber];
		if (endpointTollfreeNumber.length) [mutableRes addObject:endpointTollfreeNumber];
	}
	
	return [mutableRes copy];
}

- (BOOL)ringGroupInfoIsThisEndpoint:(SCIRingGroupInfo *)ringGroupInfo
{
	NSString *localNumber = self.userAccountInfo.localNumber;
	NSString *tollFreeNumber = self.userAccountInfo.tollFreeNumber;
	NSString *number = ringGroupInfo.phone;
	return ([number isEqualToString:localNumber] || [number isEqualToString:tollFreeNumber]);
}

- (BOOL)ringGroupInfoIsCreator:(SCIRingGroupInfo *)ringGroupMember
{
	BOOL res = NO;
	
	if (self.videophoneEngine) {
		IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
		if (ringGroupManager) {
			bool bRes = ringGroupManager->IsGroupCreator(std::string(ringGroupMember.phone.UTF8String, [ringGroupMember.phone lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
			res = bRes ? YES : NO;
		}
	}
	
	return res;
}

#pragma mark - Notifications

- (void)notifyChanged:(NSNotification *)note // SCINotificationRingGroupChanged
{
	[self postValueChangeRingGroupInfos];
}

- (void)notifyCreated:(NSNotification *)note // SCINotificationRingGroupCreated
{
	[self postValueChangeRingGroupInfos];
}

- (void)notifyRemoved:(NSNotification *)note // SCINotificationRingGroupRemoved
{
	[self postValueChangeRingGroupInfos];
}

@end

NSString * const SCIKeyInRingGroup = @"inRingGroup";
NSString * const SCIKeyRingGroupInfos = @"ringGroupInfos";

@implementation SCIVideophoneEngine (RingGroupPropertiesPrivate)
- (IstiRingGroupManager *)ringGroupManager
{
	return self.videophoneEngine->RingGroupManagerGet();
}
@end

