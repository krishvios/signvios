//
//  SCICall+Display.m
//  ntouchMac
//
//  Created by Adam Preble on 5/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICall+Display.h"
#import "SCIContactManager.h"
#import "Utilities.h"

@implementation SCICall (Display)

- (NSString *)displayDialString
{
	NSString *output = self.newDialString;
	if (output == nil || output.length == 0)
		output = self.dialString;
	return output;
}

- (NSString *)displayName
{
	NSString *dialString = [self displayDialString];
	NSString *name = [[SCIContactManager sharedManager] contactNameForNumber:dialString];
	if (name)
	{
		if([name isEqualToString:@"SVRS Español"])
		{
			name = @"SVRS";
		}
		
		return name;
	}
	else
		return self.remoteName;
}

- (NSString *)displayNumber
{
	NSString *dialString = [self displayDialString];
	return FormatAsPhoneNumber(dialString);
}

- (NSString *)displayID
{
	NSString *res;
	
	if ([self isInterpreter] || [self isTechSupport])
		res = [self remoteAlternateName];
	
	return res;
}

- (BOOL)displayIsHoldable
{
	return [self isHoldable];
}

- (void)displayNameDidChange
{
	[self willChangeValueForKey:@"displayName"];
	[self didChangeValueForKey:@"displayName"];
}

- (void)displayNumberDidChange
{
	[self willChangeValueForKey:@"displayNumber"];
	[self didChangeValueForKey:@"displayNumber"];
}

- (void)displayIDDidChange
{
	[self willChangeValueForKey:@"displayID"];
	[self didChangeValueForKey:@"displayID"];
}

- (void)displayIsHoldableDidChange
{
	[self willChangeValueForKey:@"displayIsHoldable"];
	[self didChangeValueForKey:@"displayIsHoldable"];
}

@end
