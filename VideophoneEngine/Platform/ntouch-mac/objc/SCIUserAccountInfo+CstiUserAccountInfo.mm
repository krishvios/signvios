//
//  SCIUserAccountInfo+CstiUserAccountInfo.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserAccountInfo+CstiUserAccountInfo.h"
#import "SCIUserAccountInfo+Convenience.h"
#import "SCIUserAccountInfo_Internal.h"

@implementation SCIUserAccountInfo (CstiUserAccountInfo)
+ (SCIUserAccountInfo *)userAccountInfoWithCstiUserAccountInfo:(CstiUserAccountInfo *)cstiUserAccountInfo
{
	SCIUserAccountInfo *userAccountInfo = [[SCIUserAccountInfo alloc] init];
	
	userAccountInfo.name = [NSString stringWithUTF8String:cstiUserAccountInfo->m_Name.c_str()];
	userAccountInfo.phoneNumber = [NSString stringWithUTF8String:cstiUserAccountInfo->m_TollFreeNumber.c_str()];
	userAccountInfo.tollFreeNumber = [NSString stringWithUTF8String:cstiUserAccountInfo->m_TollFreeNumber.c_str()];
	userAccountInfo.localNumber = [NSString stringWithUTF8String:cstiUserAccountInfo->m_LocalNumber.c_str()];
	userAccountInfo.associatedPhone = [NSString stringWithUTF8String:cstiUserAccountInfo->m_AssociatedPhone.c_str()];
	userAccountInfo.currentPhone = [NSString stringWithUTF8String:cstiUserAccountInfo->m_CurrentPhone.c_str()];
	userAccountInfo.address1 =  [NSString stringWithUTF8String:cstiUserAccountInfo->m_Address1.c_str()];
	userAccountInfo.address2 = [NSString stringWithUTF8String:cstiUserAccountInfo->m_Address2.c_str()];
	userAccountInfo.city = [NSString stringWithUTF8String:cstiUserAccountInfo->m_City.c_str()];
	userAccountInfo.state = [NSString stringWithUTF8String:cstiUserAccountInfo->m_State.c_str()];
	userAccountInfo.zipCode = [NSString stringWithUTF8String:cstiUserAccountInfo->m_ZipCode.c_str()];
	userAccountInfo.email = [NSString stringWithUTF8String:cstiUserAccountInfo->m_Email.c_str()];
	userAccountInfo.mainEmail= [NSString stringWithUTF8String:cstiUserAccountInfo->m_EmailMain.c_str()];
	userAccountInfo.pagerEmail = [NSString stringWithUTF8String:cstiUserAccountInfo->m_EmailPager.c_str()];
	[userAccountInfo setSignMailEnabledWithString:cstiUserAccountInfo->m_SignMailEnabled.c_str() ? [NSString stringWithUTF8String:cstiUserAccountInfo->m_SignMailEnabled.c_str()] : kSCIUserAccountInfoFalse];
	userAccountInfo.ringGroupLocalNumber = [NSString stringWithUTF8String:cstiUserAccountInfo->m_RingGroupLocalNumber.c_str()];
	userAccountInfo.ringGroupTollFreeNumber = [NSString stringWithUTF8String:cstiUserAccountInfo->m_RingGroupTollFreeNumber.c_str()];
	userAccountInfo.ringGroupName = [NSString stringWithUTF8String:cstiUserAccountInfo->m_RingGroupName.c_str()];
	[userAccountInfo setMustCallCIRWithString:cstiUserAccountInfo->m_MustCallCIR.c_str() ? [NSString stringWithUTF8String:cstiUserAccountInfo->m_MustCallCIR.c_str()] : kSCIUserAccountInfoZero];
	[userAccountInfo setUserRegistrationDataRequiredWithString:cstiUserAccountInfo->m_UserRegistrationDataRequired.c_str() ? [NSString stringWithUTF8String:cstiUserAccountInfo->m_UserRegistrationDataRequired.c_str()] : kSCIUserAccountInfoZero];

	return userAccountInfo;
}

- (CstiUserAccountInfo *)cstiUserAccountInfo
{
	CstiUserAccountInfo *cstiUserAccountInfo = new CstiUserAccountInfo();
	NSString *phoneNumber = [self phoneNumber];
	if (phoneNumber) cstiUserAccountInfo->m_PhoneNumber = [phoneNumber UTF8String];
	NSString *tollFreeNumber = [self tollFreeNumber];
	if (tollFreeNumber) cstiUserAccountInfo->m_TollFreeNumber = [tollFreeNumber UTF8String];
	NSString *localNumber = [self localNumber];
	if (localNumber) cstiUserAccountInfo->m_LocalNumber = [localNumber UTF8String];
	NSString *associatedPhone = [self associatedPhone];
	if (associatedPhone) cstiUserAccountInfo->m_AssociatedPhone = [associatedPhone UTF8String];
	NSString *currentPhone = [self currentPhone];
	if (currentPhone) cstiUserAccountInfo->m_CurrentPhone = [currentPhone UTF8String];
	
	NSString *address1 = [self address1];
	if (address1) cstiUserAccountInfo->m_Address1 = [address1 UTF8String];
	NSString *address2 = [self address2];
	if (address2) cstiUserAccountInfo->m_Address2 = [address2 UTF8String];
	NSString *city = [self city];
	if (city) cstiUserAccountInfo->m_City = [city UTF8String];
	NSString *state = [self state];
	if (state) cstiUserAccountInfo->m_State = [state UTF8String];
	NSString *zipCode = [self zipCode];
	if (zipCode) cstiUserAccountInfo->m_ZipCode = [zipCode UTF8String];
	NSString *email = [self email];
	if (email) cstiUserAccountInfo->m_Email = [email UTF8String];
	NSString *mainEmail = [self mainEmail];
	if (mainEmail) cstiUserAccountInfo->m_EmailMain = [mainEmail UTF8String];
	NSString *pagerEmail = [self pagerEmail];
	if (pagerEmail) cstiUserAccountInfo->m_EmailPager = [pagerEmail UTF8String];
	NSString *signMailEnabledString = [self signMailEnabledString];
	if (signMailEnabledString) cstiUserAccountInfo->m_SignMailEnabled = [signMailEnabledString UTF8String];
	NSString *ringGroupLocalNumber = [self ringGroupLocalNumber];
	if (ringGroupLocalNumber) cstiUserAccountInfo->m_RingGroupLocalNumber = [ringGroupLocalNumber UTF8String];
	NSString *ringGroupTollFreeNumber = [self ringGroupTollFreeNumber];
	if (ringGroupTollFreeNumber) cstiUserAccountInfo->m_RingGroupTollFreeNumber = [ringGroupTollFreeNumber UTF8String];
	NSString *ringGroupName = [self ringGroupName];
	if (ringGroupName) cstiUserAccountInfo->m_RingGroupName = [ringGroupName UTF8String];
	NSString *mustCallCIRString = [self mustCallCIRString];
	if (mustCallCIRString) cstiUserAccountInfo->m_MustCallCIR = [mustCallCIRString UTF8String];
	NSString *userRegistrationDataRequiredString = [self userRegistrationDataRequiredString];
	if (userRegistrationDataRequiredString) cstiUserAccountInfo->m_UserRegistrationDataRequired = [userRegistrationDataRequiredString UTF8String];
	return cstiUserAccountInfo;
}
@end
