//
//  ContactUtilities.h
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2018 Sorenson Communications, Inc. -- All rights reserved
//

#import <Foundation/Foundation.h>
#import <Contacts/Contacts.h>

@class SCICall;
@class SCICallListItem;
@class SCIMessageInfo;

@interface ContactUtilities : NSObject

+ (void)AddressBookAccessAllowedWithCallbackBlock:(void (^)(BOOL granted))completionBlock;
+ (NSArray<CNContact *> *)findContactsByPhoneNumber:(NSString *)searchString;
+ (NSArray<CNContact *> *)findContacts;
+ (NSString *)displayNameForDialString:(NSString*)dialString;
+ (NSString *)displayNameForSCICall:(SCICall *)call;
+ (NSString *)displayNameForSCICallListItem:(SCICallListItem *)callListItem;
+ (NSString *)displayNameForSCIMessageInfo:(SCIMessageInfo *)msgInfoItem;
+ (UIImage *)imageForDeviceContact:(CNContact *)contact toFitSize:(CGSize)size;
+ (NSString *)phoneLabelForPhoneNumber:(NSString *)phoneNumber;
+ (void)addContact:(NSString*)phoneNumber
		 firstName:(NSString*)firstName
		  lastName:(NSString*)lastName
	  organization:(NSString*)org;

@end
