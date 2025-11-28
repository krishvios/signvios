//
//  SCIEmergencyAddress.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum : NSUInteger {
	// **IMPORTANT!!** DO NOT change the values of these enumerations!! If more
	// are needed, they must be added at the end of the list! If one value is no
	// longer used, just put on a comment on the line indicating that it is
	// unused.
	SCIEmergencyAddressProvisioningStatusNotSubmitted,			//ePROVISION_STATUS_NOT_SUBMITTED
	SCIEmergencyAddressProvisioningStatusSubmitted,				//ePROVISION_STATUS_SUBMITTED
	SCIEmergencyAddressProvisioningStatusUpdatedUnverified,		//ePROVISION_STATUS_VPUSER_VERIFY	//need to verify an update made on back end
	SCIEmergencyAddressProvisioningStatusUnknown				//ePROVISION_STATUS_UNKNOWN
} SCIEmergencyAddressProvisioningStatus;

@interface SCIEmergencyAddress : NSObject
@property (nonatomic, strong, readwrite) NSString *address1;
@property (nonatomic, strong, readwrite) NSString *address2;
@property (nonatomic, strong, readwrite) NSString *city;
@property (nonatomic, strong, readwrite) NSString *state;
@property (nonatomic, strong, readwrite) NSString *zipCode;
@property (nonatomic, readonly) BOOL empty;
@property (nonatomic, assign, readwrite) SCIEmergencyAddressProvisioningStatus status;
@end

NSString *NSStringFromSCIEmergencyAddressProvisioningStatusWithPrefix(SCIEmergencyAddressProvisioningStatus status, NSString *prefix);
NSString *NSStringFromSCIEmergencyAddressProvisioningStatus(SCIEmergencyAddressProvisioningStatus status);