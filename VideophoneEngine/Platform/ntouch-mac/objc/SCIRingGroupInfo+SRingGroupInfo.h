//
//  SCIRingGroupInfo+SRingGroupInfo.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/21/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIRingGroupInfo.h"
#import "CstiCoreResponse.h"

extern SCIRingGroupInfoState SCIRingGroupInfoStateFromERingGroupMemberState(CstiCoreResponse::ERingGroupMemberState eRingGroupMemberState);

@interface SCIRingGroupInfo (SRingGroupInfo)

+ (SCIRingGroupInfo *)ringGroupInfoWithSRingGroupInfo:(CstiCoreResponse::SRingGroupInfo *)sRingGroupInfo;

@end
