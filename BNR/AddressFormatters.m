//
//  AddressFormatters.m
//  RegexKitLiteFormatterTest
//
//  Created by Nate Chandler on 3/27/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import "AddressFormatters.h"

@implementation AddressFormatter
- (NSString *)regex
{
	return @"^[A-Za-z0-9 #@&*()_+,.:;\"'/-]{0,50}$";
}
@end

@implementation CityFormatter
- (NSString *)regex 
{
	return @"^[A-Za-z #@&*()_+,.:;\"'/-]{0,32}$";
}
@end

@implementation ZipCodeFormatter
- (NSString *)partialStringRegex
{
	return @"(^[0-9]{0,5}$)|(^[0-9]{5,5}-[0-9]{0,4}$)";
}
- (NSString *)fullStringRegex
{
	return @"(^[0-9]{5,5}$)|(^[0-9]{5,5}-[0-9]{4,4}$)";
}
@end

@implementation EmailFormatter
- (NSString *)partialStringRegex
{
	return @"^[A-Za-z0-9_.-]*[@]{0,1}([A-Za-z0-9_-]+\\.)*[A-Za-z]{0,10}$"; // allow .travel and .museum
}
- (NSString *)fullStringRegex
{
	return @"^[A-Za-z0-9_.-]+@([A-Za-z0-9_-]+\\.)+[A-Za-z]{2,10}$";
}
@end

@implementation AIMFormatter
- (NSString *)partialStringRegex
{
	return @"(^[A-Za-z]{0,1}[A-Za-z0-9]{0,15}$)";
}
- (NSString *)fullStringRegex
{
	return @"(^[A-Za-z]{1,1}[A-Za-z0-9]{2,15}$)";
}
@end