//
//  NSString+MACAddress.m
//  ntouch
//
//  Created by Kevin Selman on 12/7/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

#import "NSString+MACAddress.h"
#import "ntouch-Swift.h"

static NSString * const macAddressRegEx = @"^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$";

@implementation NSString (MACAddress)

- (BOOL)sci_isValidMACAddress
{
	NSString *macAddress = self;
	return ([macAddress isMatchedByRegex:macAddressRegEx]);
}

@end
