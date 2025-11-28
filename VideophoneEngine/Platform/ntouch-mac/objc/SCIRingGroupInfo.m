//
//  SCIRingGroupInfo.m
//  ntouchMac
//
//  Created by Adam Preble on 7/19/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIRingGroupInfo.h"
#import "SCIContact.h"

@implementation SCIRingGroupInfo

@synthesize state, phone, name;
@synthesize localPhone, tollfreePhone;

- (NSString *)stateString
{
	return NSStringFromSCIRingGroupInfoState(self.state);
}

- (id)copyWithZone:(NSZone *)zone
{
	SCIRingGroupInfo *copy = [[SCIRingGroupInfo alloc] init];
	copy.state = self.state;
	copy.phone = [self.phone copyWithZone:zone];
	copy.name = [self.name copyWithZone:zone];
	return copy;
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	if ([object isMemberOfClass:[SCIRingGroupInfo class]]) {
		SCIRingGroupInfo *comparand = (SCIRingGroupInfo *)object;
		res = ([comparand.name isEqualToString:self.name] &&
			   SCIContactPhonesAreEqual(comparand.phone, self.phone));
	}
	return res;
}

- (NSUInteger)hash
{
	return (self.name.hash | self.phone.hash);
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p name: %@ phone: %@ state: %@>", [self class], self, self.name, self.phone, NSStringFromSCIRingGroupInfoState(self.state)];
}

@end


NSString *NSStringFromSCIRingGroupInfoState(SCIRingGroupInfoState state)
{
	switch (state) {
		case SCIRingGroupInfoStateActive: return @"Active";
		case SCIRingGroupInfoStatePending: return @"Pending";
		default: return @"Unknown";
	}
}

