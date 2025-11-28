//
//  Utilities.h
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <UIKit/UIKit.h>

@interface Utilities : NSObject {

}

@end

#ifdef __cplusplus
extern "C" {
#endif

#pragma	mark Randomization

NSInteger RandomInteger(NSInteger max);
void RandomizeList(NSMutableArray *list);

#pragma	mark Alerts
void Alert(NSString *sTitle, NSString *sMessage);
void AlertNotYetImplemented(void);
Boolean Confirm(NSString *sMessage);
Boolean ConfirmSheet(UIView *view, NSString *sMessage);
void ConfirmAction(NSString *title, NSString *sMessage, id delegate);

#define Localize(key) [[NSBundle mainBundle] localizedStringForKey:(key) value:@"" table:nil]

#define kDelay 0.5
#define addressRegEx		@"^[A-Za-z0-9 #@&*()_+,.:;\"'/-]{0,50}$"		//!< Regular expression for mailing address validation.
#define addressLine2RegEx	@"^[A-Za-z0-9 #@&*()_+,.:;\"'/-]{0,20}$"		//!< Regular expression for mailing address line 2 validation.
#define poBoxRegEx			@"^[p]([o0]st)?\\.*\\s*((([o0](ffice)?)\\.*\\s*)|(b([o0]?x)?\\s*)|(([o0](ffice)?)\\.*\\s*)(b([o0]?x)?\\s*))\\#?\\s*[0-9]*"		//!< Regular expression for PO Boxes
#define cityRegEx			@"^[A-Za-z #@&*()_+,.:;\"'/-]{0,32}$"			//!< Regular exprssion for city name validation.
#define zipRegEx			@"(^[0-9]{5,5}$)|(^[0-9]{5,5}-[0-9]{4,4}$)"		//!< Regular expression for US zip code validation.
#define passwordRegEx		@"^[A-Za-z0-9 #@&*!()_+,.:;\"'/-]{0,50}$"		//!< Regular expression for password validation.

NSString *IntegerWithCommas(NSInteger i);
NSString *IntegerWithoutCommas(NSInteger i);
NSString *FloatWithCommas(float f);
NSString *FloatWithoutCommas(float f);
NSString *FloatToString(float f);
NSString *FloatToChips(float f);
NSString *SecondsToTime(NSUInteger time);

UIImage *MakeIcon(UIImage *picture, float size);

NSString *DateToJustDate(NSDate *date);
NSString *DateToString(NSDate *date, NSDateFormatterStyle dateStyle, NSDateFormatterStyle timeStyle);
NSString *DateToDateString(NSDate *date);
NSString *DateToTimeString(NSDate *date);
NSString *DateToTimeStamp(NSDate *date);
NSString *DateToJustTimeStamp(NSDate *date);
NSString *FractionDigits(float f, NSInteger digits);
NSString *PercentDigits(float f, NSInteger digits);
NSString *FloatToCurrency(float f);
NSString *FloatToCurrencyRounded(float f);
NSString *FloatToPercent(float f);
NSString *IntegerToNth(NSInteger i);
NSString *UnformatPhoneNumber(NSString *input);
NSString *FormatAsPhoneNumber(NSString *input);
NSString *AddPhoneNumberTrunkCode(NSString *phoneNumber);
NSDate *DateWithGMTString(NSString *string);
BOOL ValidDomain(NSString *dialString);
BOOL ValidIPAddressOrDomain(NSString *string);
BOOL ValidateInputField(NSString *inputString, NSString *regexp);
NSString *relativeDate(NSDate *date);
float cpu_usage();

#ifdef __cplusplus
}
#endif
