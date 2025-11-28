//
//  SCIRingGroupInfo+SRingGroupInfo.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/21/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIRingGroupInfo+SRingGroupInfo.h"
#import "SCIRingGroupInfo+Cpp.h"

@implementation SCIRingGroupInfo (SRingGroupInfo)

+ (SCIRingGroupInfo *)ringGroupInfoWithSRingGroupInfo:(CstiCoreResponse::SRingGroupInfo *)sRingGroupInfo
{
	SCIRingGroupInfo *ringGroupInfo = nil;
	if (sRingGroupInfo) {
		return [self ringGroupInfoWithLocalNumber:&sRingGroupInfo->LocalPhoneNumber
								   tollFreeNumber:&sRingGroupInfo->TollFreePhoneNumber
									  description:&sRingGroupInfo->Description
											state:SCIRingGroupInfoStateFromERingGroupMemberState(sRingGroupInfo->eState)];
	}
	return ringGroupInfo;
}

@end

SCIRingGroupInfoState SCIRingGroupInfoStateFromERingGroupMemberState(CstiCoreResponse::ERingGroupMemberState eRingGroupMemberState)
{
	SCIRingGroupInfoState res = SCIRingGroupInfoStateUnknown;
	switch (eRingGroupMemberState) {
		case CstiCoreResponse::eRING_GROUP_STATE_ACTIVE: res = SCIRingGroupInfoStateActive; break;
		case CstiCoreResponse::eRING_GROUP_STATE_PENDING: res = SCIRingGroupInfoStatePending; break;
		default: res = SCIRingGroupInfoStateUnknown;
	}
	return res;
}