//
//  SCIRingGroupInfo+Cpp.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIRingGroupInfo+Cpp.h"
#import <string>

@implementation SCIRingGroupInfo (Cpp)

+ (SCIRingGroupInfo *)ringGroupInfoWithLocalNumber:(std::string *)localNumber
									tollFreeNumber:(std::string *)tollFreeNumber
									   description:(std::string *)description
											 state:(SCIRingGroupInfoState)state
{
	SCIRingGroupInfo *ringGroupInfo = [[SCIRingGroupInfo alloc] init];
	ringGroupInfo.name = [NSString stringWithUTF8String:description->c_str()];
	const char *localPhone = localNumber->c_str();
	ringGroupInfo.localPhone = localPhone && strlen(localPhone) > 0 ? [NSString stringWithUTF8String:localPhone] : nil;
	const char *tollfreePhone = tollFreeNumber->c_str();
	ringGroupInfo.tollfreePhone = tollfreePhone && strlen(tollfreePhone) > 0 ? [NSString stringWithUTF8String:tollfreePhone] : nil;
	ringGroupInfo.phone = ringGroupInfo.localPhone ?: ringGroupInfo.tollfreePhone;
	ringGroupInfo.state = state;
	return ringGroupInfo;
}

@end

SCIRingGroupInfoState SCIRingGroupInfoStateFromInt(int state)
{
	SCIRingGroupInfoState res = SCIRingGroupInfoStateUnknown;
	switch (state) {
		case 1: res = SCIRingGroupInfoStateActive; break;
		case 2: res = SCIRingGroupInfoStatePending; break;
		default: res = SCIRingGroupInfoStateUnknown;
	}
	return res;
}

SCIRingGroupInfoState SCIRingGroupInfoStateFromERingGroupMemberState(IstiRingGroupManager::ERingGroupMemberState state)
{
	SCIRingGroupInfoState res = SCIRingGroupInfoStateUnknown;
	switch (state) {
		case IstiRingGroupManager::eRING_GROUP_STATE_ACTIVE: res = SCIRingGroupInfoStateActive; break;
		case IstiRingGroupManager::eRING_GROUP_STATE_PENDING: res = SCIRingGroupInfoStatePending; break;
		default:
		case IstiRingGroupManager::eRING_GROUP_STATE_UNKNOWN: res = SCIRingGroupInfoStateUnknown; break;
	}
	return res;
}