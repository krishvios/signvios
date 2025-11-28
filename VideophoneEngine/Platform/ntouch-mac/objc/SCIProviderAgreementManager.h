//
//  SCIProviderAgreementManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/23/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SCIProviderAgreementManagerInterface) {
	SCIProviderAgreementManagerInterfaceNone,
	SCIProviderAgreementManagerInterfaceDisplaying,
};

@class SCIStaticProviderAgreementStatus;

@protocol SCIProviderAgreementInterfaceManager;

@interface SCIProviderAgreementManager : NSObject

@property (class, readonly) SCIProviderAgreementManager *sharedManager;

- (void)showInterfaceAsPartOfLogInProcessForDynamicProviderAgreements:(NSArray *)agreements
									   agreementsAcceptedBlock:(void (^)(BOOL))agreementsAcceptedBlock;
- (void)showInterfaceAsPartOfLogInProcessForStaticProviderAgreement:(SCIStaticProviderAgreementStatus *)status
											 agreementAcceptedBlock:(void (^)(BOOL providerAgreementAccepted))agreementAcceptedBlock;

@property (nonatomic, readonly) SCIProviderAgreementManagerInterface interface;

@property (nonatomic, readwrite) id<SCIProviderAgreementInterfaceManager> interfaceManager;

@end

extern NSString * const SCIProviderAgreementManagerKeyInterface;
