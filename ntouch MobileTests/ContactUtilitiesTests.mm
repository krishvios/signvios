//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2011 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "ContactUtilitiesTests.h"
#import "ContactUtilities.h"

@implementation ContactUtilitiesTests

#define contactFirst @"Unit"
#define contactLast1 @"Test 1"
#define contactPhone1 @"801-287-9814"
#define contactLast2 @"Test 2"
#define contactPhone2 @"801-287-9815"
#define contactOrg @"Sorenson Communications"

- (void)setUp
{
    [super setUp];
}

- (void)tearDown
{
    [super tearDown];
}

- (void) testFindRecordByFullName {
	
	if(![ContactUtilities findRecordByFullName:@"Unit Test 1"]) 
		[ContactUtilities addContact:contactPhone1 
						   firstName:contactFirst 
							lastName:contactLast1
						organization:contactOrg];
	
	ABRecordRef record = [ContactUtilities findRecordByFullName:@"Unit Test 1"];
	XCTAssertTrue((record == nil), @"findRecordByFullName Test"); // Impl requires contact has a picture
}

- (void) testFindRecordByPhoneNumber {
	
	if(![ContactUtilities findRecordByFullName:@"Unit Test 2"]) 
		[ContactUtilities addContact:contactPhone2
						   firstName:contactFirst
							lastName:contactLast2
						organization:contactOrg];

	// Look for contactPhone2
	ABRecordRef record1 = [ContactUtilities findRecordByPhoneNumber:@"18012879815"];
	XCTAssertTrue((record1 != nil), @"findRecordByPhoneNumber Test B");
	
	// Look for contactPhone2, formatted
	ABRecordRef record2 = [ContactUtilities findRecordByPhoneNumber:@"1(801) 287-9815"];
	XCTAssertTrue((record2 != nil), @"findRecordByPhoneNumber Test B");
	
	// Look for a number we know is not there
	ABRecordRef record3 = [ContactUtilities findRecordByPhoneNumber:@"1-123-999-1239"];
	XCTAssertTrue((record3 == nil), @"findRecordByPhoneNumber Test C");
}

- (void) testDisplayNameForRecord {
	
	ABRecordRef record = [ContactUtilities findRecordByPhoneNumber:contactPhone2];
	NSString *result = [ContactUtilities displayNameForRecord:record];
	XCTAssertTrue((result != nil), @"displayNameForRecord Test");
}

- (void) testDisplayNameForDialString {
	// Test not found string
	XCTAssertFalse([[ContactUtilities displayNameForDialString:@"9999"] isEqualToString:@"Test"], @"");
	
	// Test known strings
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"911"] isEqualToString:@"Emergency"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"411"] isEqualToString:@"Information"], @"");
	//XCTAssertTrue([[ContactUtilities displayNameForDialString:@"tech.svrs.tv"] isEqualToString:@"Sorenson Tech Support"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18012879403"] isEqualToString:@"Sorenson Tech Support"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18664966111"] isEqualToString:@"Sorenson Tech Support"], @"");
	//XCTAssertTrue([[ContactUtilities displayNameForDialString:@"cir.svrs.tv"] isEqualToString:@"Sorenson CIR"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18013868500"] isEqualToString:@"Sorenson CIR"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18667566729"] isEqualToString:@"Sorenson CIR"], @"");
	//XCTAssertTrue([[ContactUtilities displayNameForDialString:@"call.svrs.tv"] isEqualToString:@"Sorenson VRS"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18663278877"] isEqualToString:@"SVRS"], @"");
	XCTAssertTrue([[ContactUtilities displayNameForDialString:@"18669877528"] isEqualToString:@"SVRS Español"], @"");
}

- (void) testRecordIDForRecord {
	ABRecordRef record = [ContactUtilities findRecordByPhoneNumber:contactPhone2];
	if(!record)
		[ContactUtilities addContact:contactPhone2
						   firstName:contactFirst
							lastName:contactLast2
						organization:contactOrg];
	
	ABRecordID recordID = [ContactUtilities recordIDForRecord:record];
	XCTAssertTrue((recordID > 0), @"testRecordIDForRecord Test");
}

- (void) testRecordForRecordID {
	ABRecordRef record = [ContactUtilities findRecordByPhoneNumber:contactPhone2];
	if(!record)
		[ContactUtilities addContact:contactPhone2
						   firstName:contactFirst
							lastName:contactLast2
						organization:contactOrg];
	
//	ABRecordID recordID = [ContactUtilities recordIDForRecord:record];
//	ABRecordRef record2 = [ContactUtilities recordForRecordID:recordID];
	XCTAssertTrue((record2 != nil), @"testRecordForRecordID Test");
}

- (void) testCopyGroup {
	ABRecordRef record = [ContactUtilities copyGroup];
	XCTAssertTrue((record != NULL), @"testFindGroup Test");
    if (record) {
        CFRelease(record);
    }
}

- (void) testPhoneLabelForPhoneNumber {
	NSString *label = [ContactUtilities phoneLabelForPhoneNumber:contactPhone2];
	XCTAssertTrue([label isEqualToString:@"ntouch"], @"testPhoneLabelForPhoneNumber Test");
}

@end
