//
//  SCIUserPhoneNumbers+SstiUserPhoneNumbers.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/3/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserPhoneNumbers.h"
#import "stiSVX.h"

@interface SCIUserPhoneNumbers (SstiUserPhoneNumbers)

+ (SCIUserPhoneNumbers *)userPhoneNumbersWithSstiUserPhoneNumbers:(SstiUserPhoneNumbers)sstiUserPhonenumbers;
- (SstiUserPhoneNumbers)sstiUserPhoneNumbers;

@end
