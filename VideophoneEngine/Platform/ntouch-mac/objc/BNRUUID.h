//
//  BNRUUID.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/25/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface BNRUUID : NSObject <NSCopying>
+ (BNRUUID *)UUID;
+ (NSString *)string;
- (NSString *)stringValue;
@end
