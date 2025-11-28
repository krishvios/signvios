//
//  NSString+LanguageCode.m
//  ntouch Mac
//
//  Created by Nathan Gerhardt on 10/17/22.
//  Copyright © 2022 Sorenson Communications. All rights reserved.
//

#import "NSString+LanguageCode.h"

@implementation NSString (LanguageCode)
-(NSString *)LanguageCode{
	if ([self containsString:@"en"])
		return @"en-us";
	else if ([self  isEqual: @"es"])
		return @"es-mx";	
	return @"Unknown";
}
@end
