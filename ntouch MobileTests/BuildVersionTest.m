//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2011 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "BuildVersionTest.h"
#import "ntouch-Swift.h"


@implementation BuildVersionTest

- (void)setUp
{
    [super setUp];
}

- (void)tearDown
{
    [super tearDown];
}

- (void) testInitWithString {
	BuildVersion *buildVersion = [[BuildVersion alloc] initWithString:@"1.0.500"];
	XCTAssertTrue((buildVersion.major == 1), @"testInitWithString Test A");
	XCTAssertTrue((buildVersion.minor == 0), @"testInitWithString Test B");
	XCTAssertTrue((buildVersion.buildNumber == 500), @"testInitWithString Test C");
}

- (void) testIsVersionNewerCurrentVersion {
	NSString* current = @"1.0.600";
	NSString* newer = @"1.0.601";
	
	// Test build number
	XCTAssertTrue([BuildVersion isVersionNewerCurrentVersion:current updateVersion:newer], @"isVersionNewerCurrentVersion Test A");
	XCTAssertFalse([BuildVersion isVersionNewerCurrentVersion:newer updateVersion:current], @"isVersionNewerCurrentVersion Test B");
	
	newer = @"1.3.700";
	current = @"1.0.700";
	
	// Test minor number
	XCTAssertTrue([BuildVersion isVersionNewerCurrentVersion:current updateVersion:newer], @"isVersionNewerCurrentVersion Test A");
	XCTAssertFalse([BuildVersion isVersionNewerCurrentVersion:newer updateVersion:current], @"isVersionNewerCurrentVersion Test B");
}

- (void) testIsGreaterThanBuildVersion {
	BuildVersion *buildVersion1 = [[BuildVersion alloc] initWithString:@"1.0.500"];
	BuildVersion *buildVersion2 = [[BuildVersion alloc] initWithString:@"1.1.499"];
	
	XCTAssertFalse([buildVersion1 isGreaterThanBuildVersion:buildVersion2], @"IsGreaterThanBuildVersion Test A");
	XCTAssertTrue([buildVersion2 isGreaterThanBuildVersion:buildVersion1], @"IsGreaterThanBuildVersion Test B");
}

- (void) testIsGreaterThanRevisionVersionMisMatch {
	BuildVersion *buildVersion1 = [[BuildVersion alloc] initWithString:@"4.1.165"];
	BuildVersion *buildVersion2 = [[BuildVersion alloc] initWithString:@"4.2.0.1"];
	
	XCTAssertFalse([buildVersion1 isGreaterThanBuildVersion:buildVersion2], @"IsGreaterThanBuildVersion Test A");
	XCTAssertTrue([buildVersion2 isGreaterThanBuildVersion:buildVersion1], @"IsGreaterThanBuildVersion Test B");
}


- (void) testIsGreaterThanRevisionVersion {
	BuildVersion *buildVersion1 = [[BuildVersion alloc] initWithString:@"4.2.0.1"];
	BuildVersion *buildVersion2 = [[BuildVersion alloc] initWithString:@"4.2.0.2"];
	
	XCTAssertFalse([buildVersion1 isGreaterThanBuildVersion:buildVersion2], @"IsGreaterThanBuildVersion Test A");
	XCTAssertTrue([buildVersion2 isGreaterThanBuildVersion:buildVersion1], @"IsGreaterThanBuildVersion Test B");
}


- (void) testIsGreaterThanBuildVersionShort {
	BuildVersion *buildVersion1 = [[BuildVersion alloc] initWithString:@"1.1"];
	BuildVersion *buildVersion2 = [[BuildVersion alloc] initWithString:@"2.1"];
	
	XCTAssertFalse([buildVersion1 isGreaterThanBuildVersion:buildVersion2], @"IsGreaterThanBuildVersion Test A");
	XCTAssertTrue([buildVersion2 isGreaterThanBuildVersion:buildVersion1], @"IsGreaterThanBuildVersion Test B");
}

- (void) testIsVersionNewerCurrentVersionWithBlank {
	NSString* current = @"";
	NSString* newer = @"1.0.601";
	
	// Test build number by passing blank value.
	XCTAssertTrue([BuildVersion isVersionNewerCurrentVersion:current updateVersion:newer], @"testIsVersionNewerCurrentVersionWithBlank Test A");
	XCTAssertFalse([BuildVersion isVersionNewerCurrentVersion:newer updateVersion:current], @"testIsVersionNewerCurrentVersionWithBlank Test B");
}

- (void) testInitWithBlankString {
	BuildVersion *buildVersion = [[BuildVersion alloc] initWithString:@""];
	XCTAssertFalse((buildVersion.major == 1), @"testInitWithString Test A");
	XCTAssertFalse((buildVersion.minor == 0), @"testInitWithString Test B");
	XCTAssertFalse((buildVersion.buildNumber == 500), @"testInitWithString Test C");
}

- (void) testIsGreaterThanBuildVersionWithBlank {
	BuildVersion *buildVersion1 = [[BuildVersion alloc] initWithString:@""];
	BuildVersion *buildVersion2 = [[BuildVersion alloc] initWithString:@"1.1.499"];
	
	XCTAssertFalse([buildVersion1 isGreaterThanBuildVersion:buildVersion2], @"IsGreaterThanBuildVersionWithBlank Test A");
	XCTAssertTrue([buildVersion2 isGreaterThanBuildVersion:buildVersion1], @"IsGreaterThanBuildVersionWithBlank Test B");
}


@end
