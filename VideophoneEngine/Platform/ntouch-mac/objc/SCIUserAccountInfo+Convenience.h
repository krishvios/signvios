//
//  SCIUserAccountInfo+Convenience.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/22/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserAccountInfo.h"

@interface SCIUserAccountInfo (Convenience)
- (NSString *)signMailEnabledString;
- (void)setSignMailEnabledWithString:(NSString *)signMailEnabledString;
- (BOOL)signMailEnabledBool;
- (void)setSignMailEnabledWithBool:(BOOL)signMailEnabledBool;
- (NSString *)mustCallCIRString;
- (void)setMustCallCIRWithString:(NSString *)mustCallCIRString;
- (BOOL)mustCallCIRBool;
- (void)setMustCallCIRWithBool:(BOOL)mustCallCIRBool;
- (NSString *)userRegistrationDataRequiredString;
- (void)setUserRegistrationDataRequiredWithString:(NSString *)userRegistrationDataRequiredString;
- (BOOL)userRegistrationDataRequiredBool;
- (void)setUserRegistrationDataRequiredWithBool:(BOOL)userRegistrationDataRequiredBool;
@end
