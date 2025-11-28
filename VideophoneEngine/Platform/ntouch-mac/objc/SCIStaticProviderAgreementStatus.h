//
//  SCIStaticProviderAgreementStatus.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIStaticProviderAgreementState.h"

@interface SCIStaticProviderAgreementStatus : NSObject

+ (instancetype)providerAgreementStatusFromString:(NSString *)providerAgreementStatusString;
+ (instancetype)emptyProviderAgreementStatus;

@property (nonatomic) NSDate *date;
@property (nonatomic, readonly) NSString *softwareVersionString;
@property (nonatomic) NSUInteger softwareMajorVersion;
@property (nonatomic) NSUInteger softwareMediumVersion;
@property (nonatomic) NSUInteger softwareMinorVersion;

- (SCIStaticProviderAgreementState)stateForCurrentSoftwareVersionString:(NSString *)softwareVersionString;
- (NSString *)createProviderAgreementStatusStringForCurrentSoftwareVersionString:(NSString *)softwareVersionString;

@end

#ifdef __cplusplus
extern "C" {
#endif
	extern SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusString(NSString *providerAgreementString, NSString *currentSoftwareVersionString);
	extern NSString *SCIStaticProviderAgreementStatusStringFromProviderAgreementState(SCIStaticProviderAgreementState state, NSDate *date, NSString *softwareVersionString);
#ifdef __cplusplus
}
#endif

