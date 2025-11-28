//
//  SCIUserAccountInfo+Convenience.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/22/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserAccountInfo+Convenience.h"
#import "SCIUserAccountInfo_Internal.h"

@implementation SCIUserAccountInfo (Convenience)

- (NSString *)signMailEnabledString
{
	NSString *res = nil;
	
	NSNumber *signMailEnabled = [self signMailEnabled];
	if (signMailEnabled) {
		res = ([signMailEnabled boolValue]) ? kSCIUserAccountInfoTrue : kSCIUserAccountInfoFalse;
	} else {
		res = nil;
	}
	
	return res;
}

- (void)setSignMailEnabledWithString:(NSString *)signMailEnabledString
{
	[self setSignMailEnabledWithBool:^{
		BOOL res = NO;
	
		if (signMailEnabledString) {
			if ([signMailEnabledString isEqualToString:kSCIUserAccountInfoTrue]) {
				res = YES;
			} else {
				res = NO;
			}
		} else {
			res = NO;
		}
		
		return res;
	}()];
}

- (BOOL)signMailEnabledBool
{
	return [[self signMailEnabled] boolValue];
}

- (void)setSignMailEnabledWithBool:(BOOL)signMailEnabledBool;
{
	[self setSignMailEnabled:[NSNumber numberWithBool:signMailEnabledBool]];
}

- (NSString *)mustCallCIRString
{
	NSString *res = nil;
	
	NSNumber *mustCallCIR = [self mustCallCIR];
	if (mustCallCIR) {
		res = ([mustCallCIR boolValue]) ? kSCIUserAccountInfoOne : kSCIUserAccountInfoZero;
	} else {
		res = nil;
	}
	
	return res;
}

- (void)setMustCallCIRWithString:(NSString *)mustCallCIRString
{
	[self setMustCallCIRWithBool:^{
		BOOL res = NO;
		
		if (mustCallCIRString) {
			if ([mustCallCIRString isEqualToString:kSCIUserAccountInfoTrue]) {
				res = YES;
			} else {
				res = NO;
			}
		} else {
			res = NO;
		}
		
		return res;
	}()];
}

- (BOOL)mustCallCIRBool
{
	return [[self mustCallCIR] boolValue];
}

- (void)setMustCallCIRWithBool:(BOOL)mustCallCIRBool
{
	[self setMustCallCIR:[NSNumber numberWithBool:mustCallCIRBool]];
}

- (NSString *)userRegistrationDataRequiredString
{
	NSString *res = nil;
	
	NSNumber *userRegistrationDataRequired = [self userRegistrationDataRequired];
	if (userRegistrationDataRequired) {
		res = ([userRegistrationDataRequired boolValue]) ? kSCIUserAccountInfoTrue : kSCIUserAccountInfoFalse;
	} else {
		res = nil;
	}
	
	return res;
}

- (void)setUserRegistrationDataRequiredWithString:(NSString *)userRegistrationDataRequiredString
{
	[self setUserRegistrationDataRequiredWithBool:^{
		BOOL res = NO;
		
		if (userRegistrationDataRequiredString) {
			if ([userRegistrationDataRequiredString isEqualToString:kSCIUserAccountInfoTrue]) {
				res = YES;
			} else {
				res = NO;
			}
		} else {
			res = NO;
		}
		
		return res;
	}()];
}

- (BOOL)userRegistrationDataRequiredBool
{
	return [[self userRegistrationDataRequired] boolValue];
}

- (void)setUserRegistrationDataRequiredWithBool:(BOOL)userRegistrationDataRequiredBool
{
	[self setUserRegistrationDataRequired:[NSNumber numberWithBool:userRegistrationDataRequiredBool]];
}

@end
