//
//  SCIRingGroupInfo+Cpp.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIRingGroupInfo.h"
#import <string>
#import "IstiRingGroupManager.h"

@interface SCIRingGroupInfo (Cpp)

+ (SCIRingGroupInfo *)ringGroupInfoWithLocalNumber:(std::string *)localNumber
									tollFreeNumber:(std::string *)tollFreeNumber
									   description:(std::string *)description
											 state:(SCIRingGroupInfoState)state;

@end

SCIRingGroupInfoState SCIRingGroupInfoStateFromInt(int state);
SCIRingGroupInfoState SCIRingGroupInfoStateFromERingGroupMemberState(IstiRingGroupManager::ERingGroupMemberState state);
