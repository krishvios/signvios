//
//  SCIVPEPhoneNumberFormatter.m
//  ntouchMac
//
//  Created by Nate Chandler on 11/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVPEPhoneNumberFormatter.h"
#import "SCIPhoneNumberFormatter_SubclassInterface.h"
#import "SCIVideophoneEngine.h"

@implementation SCIVPEPhoneNumberFormatter

+ (BOOL)shouldFormatString:(NSString *)string
{
	if ([[SCIVideophoneEngine sharedEngine] isNameRingGroupMember:string]) {
		return NO;
	} else {
		return [super shouldFormatString:string];
	}
}

@end
