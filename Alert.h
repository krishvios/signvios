//
//  Alert.h
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

void AlertWithError(NSError *error);
void AlertWithMessage(NSString *message);
void AlertWithTitleAndMessage(NSString *title, NSString *message);

#ifdef __cplusplus
}
#endif
