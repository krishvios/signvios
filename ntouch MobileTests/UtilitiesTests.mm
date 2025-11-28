//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2011 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "UtilitiesTests.h"
#import "Utilities.h"
#import "SCIDebug.h"


@implementation UtilitiesTests

- (void)setUp
{
    [super setUp];
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testIntegerWithCommas {
	NSString *temp = IntegerWithCommas(123456789);
	XCTAssertTrue([temp isEqualToString:@"123,456,789"], @"IntegerWithCommas Test");
}

- (void)testIntegerWithoutCommas {
	NSString *temp = IntegerWithoutCommas(123456789);
	XCTAssertTrue([temp isEqualToString:@"123456789"], @"IntegerWithoutCommas Test");
}

- (void)testFloatWithoutCommas {
	NSString *number = FloatWithoutCommas(123456.54f);
	XCTAssertTrue([number isEqualToString:@"123456.54"], @"FloatWithoutCommas Test");
}

- (void)testFloatWithCommas {
	NSString *number = FloatWithCommas(123456.54f);
	XCTAssertTrue([number isEqualToString:@"123,456.54"], @"FloatWithCommas Test");
}

- (void)testFloatToChips {
	NSString *bigNumber = FloatToChips(123456.99f);
	XCTAssertTrue([bigNumber isEqualToString:@"123,457"], @"FloatToChips Test");
	
	// Implementation doesnt handle 5 digit floats well.  Testing existing impl.
	NSString *mediumNumber = FloatToChips(12345.55f);
	XCTAssertTrue([mediumNumber isEqualToString:@"12345,.55"], @"FloatToChips Test");
	
	NSString *tinyNumber = FloatToChips(12345.55f);
	XCTAssertTrue([tinyNumber isEqualToString:@"12345,.55"], @"FloatToChips Test");
}

- (void)testSecondsToTime {
	NSUInteger seconds = 4140;
	XCTAssertTrue([SecondsToTime(seconds) isEqualToString:@"1:09:00"], @"SecondsToTime Test");
	seconds = 656;
	XCTAssertTrue([SecondsToTime(seconds) isEqualToString:@"10:56"], @"SecondsToTime Test");
}

- (void)testMakeIcon {
	UIImage *iconImage = MakeIcon([UIImage imageNamed:@"SorensonLogo"], 100.0f);
	XCTAssertTrue((iconImage.size.width == 100), @"MakeIcon Test");
}

- (void)testUITextAccentColor {
	UIColor *textColor = UITextAccentColor();
	XCTAssertTrue((textColor != nil), @"UITextAccentColor Test");
	// Test color values.  requires CoreGraphics.  Do we want?
	//const float* colors = CGColorGetComponents(textColor);
}

- (void)testSorensonGoldColor {
	UIColor *goldColor = SorensonGoldColor();
	XCTAssertTrue((goldColor != nil), @"SorensonGoldColor Test");
}

- (void)testDateToJustDate {
	NSDate *testDate = [[NSDate alloc] initWithTimeIntervalSince1970:162286200];
	NSString *dateString = DateToJustDate(testDate);
	XCTAssertTrue([dateString isEqualToString:@"1975-02-22"], @"DateToJustDate Test");
}

- (void)testDateToString {
	NSDate *testDate = [[NSDate alloc] initWithTimeIntervalSince1970:162286200];
	NSString *result = DateToString(testDate, NSDateFormatterMediumStyle, NSDateFormatterMediumStyle);
	XCTAssertTrue([result isEqualToString:@"Feb 22, 1975, 12:30:00 AM"], @"DateToString");
}

- (void)testDateToTimeStamp {
	NSDate *testDate = [[NSDate alloc] initWithTimeIntervalSince1970:162286200];
	NSString *result = DateToTimeStamp(testDate);
	XCTAssertTrue([result isEqualToString:@"1975-02-22 00:30:00"], @"DateToTimeStamp Test");
}
				  
- (void)testDateToJustTimeStamp {
	NSDate *testDate = [[NSDate alloc] initWithTimeIntervalSince1970:162286200];
	NSString *result = DateToJustTimeStamp(testDate);
	XCTAssertTrue([result isEqualToString:@"00:30:00"], @"DateToTimeStamp Test");
}

- (void)testFractionDigits {
	NSString *result = FractionDigits(1234.1234f, 3);
	XCTAssertTrue([result isEqualToString:@"1,234.123"], @"FractionDigits Test");
}

- (void)testPercentDigits {
	NSString *result = PercentDigits(0.336f, 2);
	XCTAssertTrue([result isEqualToString:@"33.60%"], @"PercentDigits Test");
}

- (void)testFloatToCurrency {
	NSString *result = FloatToCurrency(1234.1234f);
	XCTAssertTrue([result isEqualToString:@"$1,234.12"], @"FloatToCurrency Test");
	
}

// TODO
//FloatToCurrencyRounded
//FloatToPercent
//IntegerToNth
//NewUnformatPhoneNumber
//UnformatPhoneNumber
//FormatAsPhoneNumber
//DateWithGMTString
//ValidDomain
//ValidIPAddressOrDomain
//ValidateInputField

@end
