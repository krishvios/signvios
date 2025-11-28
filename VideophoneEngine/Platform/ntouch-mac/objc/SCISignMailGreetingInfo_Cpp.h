//
//  SCISignMailGreetingInfo_Cpp.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCISignMailGreetingInfo.h"
#import "CstiMessageResponse.h"
#import <vector>

@interface SCISignMailGreetingInfo ()
+ (SCISignMailGreetingInfo *)greetingInfoFromSGreetingInfoVector:(const std::vector<CstiMessageResponse::SGreetingInfo> &)greetingInfoList;
@end

extern SCISignMailGreetingType SCISignMailGreetingTypeFromEGreetingType(CstiMessageResponse::EGreetingType greetingType);
extern SCISignMailGreetingType SCISignMailGreetingTypeFromESignMailGreetingType(eSignMailGreetingType greetingType);
extern eSignMailGreetingType ESignMailGreetingTypeFromSCISignMailGreetingType(SCISignMailGreetingType type);

