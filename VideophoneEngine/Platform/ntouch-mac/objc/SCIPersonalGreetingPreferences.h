//
//  SCIPersonalGreetingPreferences.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCISignMailGreetingType.h"

@interface SCIPersonalGreetingPreferences : NSObject

@property (nonatomic) NSString *greetingText;
@property (nonatomic) BOOL greetingEnabled;
@property (nonatomic) SCISignMailGreetingType greetingType;
@property (nonatomic) SCISignMailGreetingType personalType;

@end

