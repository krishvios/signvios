//
//  SCIRingGroupInfo.h
//  ntouchMac
//
//  Created by Adam Preble on 7/19/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
	SCIRingGroupInfoStateUnknown,
	SCIRingGroupInfoStateActive,
	SCIRingGroupInfoStatePending,
} SCIRingGroupInfoState;

@interface SCIRingGroupInfo : NSObject <NSCopying>

@property (nonatomic) NSString *name;
@property (nonatomic) NSString *phone;
@property (nonatomic) NSString *localPhone;
@property (nonatomic) NSString *tollfreePhone;
@property (nonatomic) SCIRingGroupInfoState state;
@property (nonatomic, readonly) NSString *stateString;

@end


NSString *NSStringFromSCIRingGroupInfoState(SCIRingGroupInfoState state);
