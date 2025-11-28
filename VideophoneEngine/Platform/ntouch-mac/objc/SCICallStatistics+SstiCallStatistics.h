//
//  SCICallStatistics+SstiCallStatistics.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallStatistics.h"
#import "stiDefs.h"
#import "stiSVX.h"
#import "IstiCall.h"

@interface SCITransmissionStatistics (SstiStatistics)
+ (SCITransmissionStatistics *)transmissionStatisticsWithSstiStatistics:(SstiStatistics)sstiStatistics;
@end

@interface SCICallStatistics (SstiCallStatistics)
+ (SCICallStatistics *)callStatisticsWithSstiCallStatistics:(SstiCallStatistics)sstiCallStatistics;
@end
