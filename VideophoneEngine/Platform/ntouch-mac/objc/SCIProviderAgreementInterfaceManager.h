//
//  SCIProviderAgreementInterfaceManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 9/13/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SCIProviderAgreementInterfaceManagerResult) {
	SCIProviderAgreementInterfaceManagerResultAccept,
	SCIProviderAgreementInterfaceManagerResultReject,
	SCIProviderAgreementInterfaceManagerResultAbort,
	SCIProviderAgreementInterfaceManagerResultUnnecessary,
};

typedef void(^SCIProviderAgreementInterfaceManagerResultBlock)(SCIProviderAgreementInterfaceManagerResult);

@class SCIDynamicProviderAgreement;
@class SCIStaticProviderAgreementStatus;
@class SCIProviderAgreementManager;

@protocol SCIProviderAgreementInterfaceManager <NSObject>

- (void)logout;

- (void)showDynamicProviderAgreementInterfaceForAgreement:(SCIDynamicProviderAgreement *)agreement
												loggingIn:(BOOL)loggingIn
											   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))block;
- (void)hideDynamicProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result;

- (void)showStaticProviderAgreementInterfaceForStatus:(SCIStaticProviderAgreementStatus *)status
											loggingIn:(BOOL)loggingIn
										   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))completionBlock;
- (void)hideStaticProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result;

- (void)providerAgreementManagerDidStartLoadingAgreementsWhileShowingDynamicProviderAgreementInterface:(SCIProviderAgreementManager *)manager;
- (void)providerAgreementManager:(SCIProviderAgreementManager *)manager didFinishLoadingAgreementsWhileShowingProviderAgreementInterfaceWithFirstUnacceptedAgreement:(SCIDynamicProviderAgreement *)agreement;

- (void)showAlertWithTitle:(NSString *)title
				   message:(NSString *)message
		defaultButtonTitle:(NSString *)defaultButtonTitle
	  alternateButtonTitle:(NSString *)alternateButtonTitle
		  otherButtonTitle:(NSString *)otherButtonTitle
		 defaultReturnCode:(NSInteger)defaultReturnCode
				completion:(void (^)(NSInteger returnCode))block;

@end
