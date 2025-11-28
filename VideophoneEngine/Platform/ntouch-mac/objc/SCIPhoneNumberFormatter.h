//
//  PhoneNumberFormatter.h
//  ntouchMac
//
//  Created by Adam Preble on 2/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCIPhoneNumberFormatter : NSFormatter

+ (NSString *)stringForPhoneNumber:(NSString *)phonenumber;
+ (NSString *)phoneNumberForString:(NSString *)string;
+ (BOOL)shouldFormatString:(NSString *)string;
+ (NSString *)strip:(NSString *)phoneNumber;

@end

#ifdef __cplusplus
extern "C" {
#endif
	extern BOOL SCIPhoneNumberFormatterStringIsDomainDialString(NSString *string);
	extern BOOL SCIPhoneNumberFormatterStringIsIPDialString(NSString *string);
#ifdef __cplusplus
}
#endif
