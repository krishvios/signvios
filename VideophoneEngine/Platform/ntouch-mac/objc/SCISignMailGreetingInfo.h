//
//  SCISignMailGreetingInfo.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCISignMailGreetingType.h"

@interface SCISignMailTypedGreetingInfo : NSObject
@property (nonatomic) NSArray *URLs;
@property (nonatomic) SCISignMailGreetingType type;
@end

@interface SCISignMailGreetingInfo : NSObject
@property (nonatomic) NSArray *typedInfos;
@property (nonatomic) int maxRecordingLength; //in seconds

- (SCISignMailTypedGreetingInfo *)typedInfoOfType:(SCISignMailGreetingType)type;
- (NSArray *)URLsOfType:(SCISignMailGreetingType)type;
@end

#ifdef __cplusplus
extern "C" {
#endif
	BOOL SCISignMailGreetingTypeIsPersonal(SCISignMailGreetingType type);
	BOOL SCISignMailGreetingTypeHasText(SCISignMailGreetingType type);
#ifdef __cplusplus
}
#endif
