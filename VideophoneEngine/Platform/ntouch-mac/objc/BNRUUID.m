//
//  BNRUUID.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/25/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "BNRUUID.h"

@interface BNRUUID ()
{
	CFUUIDRef mCFUUIDRef;
}
@property (nonatomic, assign) CFUUIDRef CFUUIDRef;
@end

@implementation BNRUUID

@synthesize CFUUIDRef = mCFUUIDRef;

+ (BNRUUID *)UUID
{
	return [[BNRUUID alloc] init];
}

+ (NSString *)string
{
	return [[[BNRUUID alloc] init] stringValue];
}

- (id)init
{
	self = [super init];
	if (self) {
		CFAllocatorRef defaultAllocator = CFAllocatorGetDefault();
		mCFUUIDRef = CFUUIDCreate(defaultAllocator);
	}
	return self;
}

- (void)dealloc
{
	CFRelease(self.CFUUIDRef);
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	if ([object isMemberOfClass:[BNRUUID class]]) {
		BNRUUID *comparand = (BNRUUID *)object;
		res = CFEqual(self.CFUUIDRef, comparand.CFUUIDRef);
	}
	return res;
}

- (NSUInteger)hash
{
	return self.stringValue.hash;
}

- (void)setCFUUIDRef:(CFUUIDRef)CFUUIDRef
{
	CFRelease(self.CFUUIDRef);
	mCFUUIDRef = CFUUIDRef;
	CFRetain(self.CFUUIDRef);
}

#pragma mark - public API

- (NSString *)stringValue
{
	CFAllocatorRef defaultAllocator = CFAllocatorGetDefault();
	CFStringRef cfStringUUID = CFUUIDCreateString(defaultAllocator, self.CFUUIDRef);
    return (__bridge_transfer NSString *)cfStringUUID;
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone *)zone
{
	BNRUUID *copy = [BNRUUID UUID];
	[copy setCFUUIDRef:self.CFUUIDRef];
	return copy;
}

@end
