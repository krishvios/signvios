//
//  SCIPhoneNumberFormatter_SubclassInterface.h
//  ntouchMac
//
//  Created by Nate Chandler on 11/30/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIPhoneNumberFormatter.h"

@interface SCIPhoneNumberFormatter ()
+ (NSString *)format:(NSString *)phoneNumber withLocale:(NSString *)locale bailedOut:(BOOL *)bailed;
@end
