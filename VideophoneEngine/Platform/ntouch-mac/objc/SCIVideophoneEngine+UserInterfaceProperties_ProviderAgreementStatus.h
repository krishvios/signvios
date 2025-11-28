//
//  SCIVideophoneEngine+UserInterfaceProperties_ProviderAgreementStatus.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine+UserInterfaceProperties.h"

@class SCIStaticProviderAgreementStatus;

@interface SCIVideophoneEngine (UserInterfaceProperties_ProviderAgreementStatus)
@property (nonatomic, readonly) NSString *providerAgreementStatusString;
- (void)setProviderAgreementStatusStringPrimitive:(NSString *)providerAgreementStatusString;
@property (nonatomic, readonly) NSNumber *providerAgreementAccepted; // BOOL NSNumber: nil indicates that the value has not yet been loaded from Core
- (void)setProviderAgreementAcceptedPrimitive:(BOOL)providerAgreementAccepted;
- (void)setProviderAgreementAcceptedPrimitive:(BOOL)accepted completionHandler:(void (^)(NSError *))block;
- (void)setProviderAgreementAcceptedPrimitive:(BOOL)accepted timeout:(NSTimeInterval)timeout completionHandler:(void (^)(NSError *))block;
@property (nonatomic, readonly) SCIStaticProviderAgreementStatus *providerAgreementStatus;
- (void)setProviderAgreementStatusPrimitive:(SCIStaticProviderAgreementStatus *)providerAgreementStatus;
- (BOOL)providerAgreementStatusStringIsAcceptance:(NSString *)string;
- (NSString *)providerAgreementStatusString:(BOOL)accepted;
@end

extern NSString * const SCIKeyProviderAgreementAccepted;
extern NSString * const SCIKeyProviderAgreementStatusString;
extern NSString * const SCIKeyProviderAgreementStatus;
