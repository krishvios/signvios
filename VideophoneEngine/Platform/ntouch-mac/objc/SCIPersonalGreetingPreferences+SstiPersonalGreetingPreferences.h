//
//  SCIPersonalGreetingPreferences+SstiPersonalGreetingPreferences.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPersonalGreetingPreferences.h"
#import "stiSVX.h"

@interface SCIPersonalGreetingPreferences (SstiPersonalGreetingPreferences)
+ (instancetype)personalGreetingPreferencesWithSstiPersonalGreetingPreferences:(SstiPersonalGreetingPreferences *)sstiPersonalGreetingPreferences;
- (SstiPersonalGreetingPreferences)createSstiPersonalGreetingPreferences;
@end
