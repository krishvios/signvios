//
//  SCIVideophoneEngine+RingGroupProperties.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/12/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine.h"

@class SCIRingGroupInfo;

@interface SCIVideophoneEngine (RingGroupProperties)

//Observing
- (void)startObservingRingGroupProperties;
- (void)stopObservingRingGroupProperties;

//getting
@property (nonatomic, readonly) BOOL inRingGroup;	//KVO-compliant
@property (nonatomic, readonly) NSArray *ringGroupInfos;	//KVO-compliant
@property (nonatomic, readonly) int ringGroupCount;
@property (nonatomic, readonly) NSArray *otherRingGroupInfos;
@property (nonatomic, readonly) NSString *preferredRingGroupNumber;
@property (nonatomic, readonly) NSArray *ringGroupNumbers;
- (BOOL)ringGroupInfoIsThisEndpoint:(SCIRingGroupInfo *)ringGroupInfo;
- (BOOL)ringGroupInfoIsCreator:(SCIRingGroupInfo *)ringGroupMember;

@end

extern NSString * const SCIKeyRingGroupInfos;
extern NSString * const SCIKeyInRingGroup;

