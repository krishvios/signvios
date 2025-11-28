//
//  NSBundle+SCIAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 9/13/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSBundle+SCIAdditions.h"

@implementation NSBundle (SCIAdditions)

- (NSString *)sci_shortVersionString
{
	NSString *shortVersion = [[self infoDictionary] objectForKey:@"CFBundleShortVersionString"];
	return shortVersion;
}

- (NSString *)sci_bundleVersion
{
	return [[self infoDictionary] objectForKey:@"CFBundleVersion"];;

}

@end
