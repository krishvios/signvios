//
//  NSString+SCIPhoneNumber.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (SCIPhoneNumber)

- (NSString *)sci_dialableStringFromString;
- (NSString *)sci_undecoratedNumberFromDialableNumber;
- (NSString *)sci_dialableNumberFromPhoneNumber;
- (BOOL)sci_phoneNumberIsDialable;
- (NSString *)sci_stringByReplacingLettersWithPhoneNumberEquivalents;
@end

#ifdef __cplusplus
extern "C" {
#endif
unichar SCIPhoneNumberCharacterForCharacter(unichar character);
#ifdef __cplusplus
}
#endif
