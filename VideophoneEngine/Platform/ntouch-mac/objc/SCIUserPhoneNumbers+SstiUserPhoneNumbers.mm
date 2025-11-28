//
//  SCIUserPhoneNumbers+SstiUserPhoneNumbers.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/3/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserPhoneNumbers+SstiUserPhoneNumbers.h"

@implementation SCIUserPhoneNumbers (SstiUserPhoneNumbers)

+ (SCIUserPhoneNumbers *)userPhoneNumbersWithSstiUserPhoneNumbers:(SstiUserPhoneNumbers)sstiUserPhoneNumbers
{
	SCIUserPhoneNumbers *res = [[SCIUserPhoneNumbers alloc] init];
	
	[res setSorensonPhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szSorensonPhoneNumber]];
	[res setTollFreePhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szTollFreePhoneNumber]];
	[res setLocalPhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szLocalPhoneNumber]];
	[res setHearingPhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szHearingPhoneNumber]];
	[res setRingGroupLocalPhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szRingGroupLocalPhoneNumber]];
	[res setRingGroupTollFreePhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szRingGroupTollFreePhoneNumber]];
	[res setPreferredPhoneNumberType:[res typeOfPhoneNumber:[NSString stringWithUTF8String:sstiUserPhoneNumbers.szPreferredPhoneNumber]]];
	return res;
}

- (SstiUserPhoneNumbers)sstiUserPhoneNumbers
{
	SstiUserPhoneNumbers res;
	sprintf(res.szSorensonPhoneNumber, "%s", (const char *)[[self sorensonPhoneNumber] UTF8String]);
	sprintf(res.szTollFreePhoneNumber, "%s", (const char *)[[self tollFreePhoneNumber] UTF8String]);
	sprintf(res.szLocalPhoneNumber, "%s", (const char *)[[self localPhoneNumber] UTF8String]);
	sprintf(res.szHearingPhoneNumber, "%s", (const char *)[[self hearingPhoneNumber] UTF8String]);
	sprintf(res.szRingGroupLocalPhoneNumber, "%s", (const char *)[[self ringGroupLocalPhoneNumber] UTF8String]);
	sprintf(res.szRingGroupTollFreePhoneNumber, "%s", (const char *)[[self ringGroupTollFreePhoneNumber] UTF8String]);
	sprintf(res.szPreferredPhoneNumber, "%s", (const char *)[[self preferredPhoneNumber] UTF8String]);
	return res;
}

@end
