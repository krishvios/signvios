//
//  IOS7Utilities.h
//  ntouch
//
//  Created by Nate Chandler on 10/22/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

#ifdef __IPHONE_7_0
#define SCI_IPHONE_7_0 __IPHONE_7_0
#else
#define SCI_IPHONE_7_0 70000
#endif

#define SCI_IOS_SDK_IS_AT_LEAST_IPHONE_7_0 (__IPHONE_OS_VERSION_MAX_ALLOWED >= SCI_IPHONE_7_0)
