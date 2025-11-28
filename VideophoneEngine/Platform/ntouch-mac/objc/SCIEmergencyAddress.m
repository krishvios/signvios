//
//  SCIEmergencyAddress.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIEmergencyAddress.h"

@implementation SCIEmergencyAddress

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.status = SCIEmergencyAddressProvisioningStatusUnknown;
	}
	return self;
}

- (BOOL)empty
{
	return !self.address1 && !self.address2 && !self.city && !self.state && !self.zipCode;
}

@end

NSString *NSStringFromSCIEmergencyAddressProvisioningStatusWithPrefix(SCIEmergencyAddressProvisioningStatus status, NSString *prefix)
{
	NSString *res;
	
	NSString *statusString;
	switch (status) {
		case SCIEmergencyAddressProvisioningStatusNotSubmitted: {
			statusString = @"Not Submitted";
		} break;
		case SCIEmergencyAddressProvisioningStatusSubmitted: {
			statusString = @"Submitted";
		} break;
		case SCIEmergencyAddressProvisioningStatusUpdatedUnverified: {
			statusString = @"User Verification Required";
		} break;
		case SCIEmergencyAddressProvisioningStatusUnknown: {
			statusString = nil;
		} break;
	}
	
	if (statusString) {
		res = [NSString stringWithFormat:@"%@%@", prefix, statusString];
	} else {
		res = @"";
	}
	
	return res;
}

NSString *NSStringFromSCIEmergencyAddressProvisioningStatus(SCIEmergencyAddressProvisioningStatus status)
{
	return NSStringFromSCIEmergencyAddressProvisioningStatusWithPrefix(status, @"Address Status: ");
}
