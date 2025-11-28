//
//  ContactUtilities.m
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "ContactUtilities.h"
#import "Utilities.h"
#import "AppDelegate.h"
#import "SCIContactManager.h"
#import "SCIVideophoneEngine.h"
#import "SCICallListItem.h"
#import "SCIMessageInfo.h"

@implementation ContactUtilities

+ (void)AddressBookAccessAllowedWithCallbackBlock:(void (^)(BOOL granted))completionBlock
{
	switch ([CNContactStore authorizationStatusForEntityType:CNEntityTypeContacts])
	{
		case CNAuthorizationStatusNotDetermined:
		{
			[[[CNContactStore alloc] init] requestAccessForEntityType:CNEntityTypeContacts completionHandler:^(BOOL granted, NSError * _Nullable error) {
				if (granted == YES && completionBlock) {
					completionBlock(YES);
				}
			}];
			break;
		}
		case CNAuthorizationStatusRestricted:
		case CNAuthorizationStatusDenied:
		{
			Alert(Localize(@"Unable to Access Contacts"), Localize(@"This app does not have access to your contacts.  You can enable access in Privacy Settings."));
			completionBlock(NO);
			break;
		}
		case CNAuthorizationStatusAuthorized:
		{
			if (completionBlock) {
				completionBlock(YES);
			}
			break;
		}
	}
}

+ (NSArray<CNContact *> *)findContactsByPhoneNumber:(NSString *)searchString
{
	NSMutableArray *foundContacts = [[NSMutableArray alloc] init];
	NSArray *contacts = [ContactUtilities findContacts];
	
	for (CNContact *contact in contacts) {
		for (CNLabeledValue *phoneLabels in contact.phoneNumbers) {
			CNPhoneNumber *phone = phoneLabels.value;
			NSString *tempPhone = [UnformatPhoneNumber(phone.stringValue) normalizedDialString];
			
			if (phone.stringValue && [tempPhone isEqualToString:[searchString normalizedDialString]]) {
				[foundContacts addObject:contact];
			}
		}
	}
	return foundContacts;
}

+ (NSArray<CNContact *> *)findContacts
{
	CNContactStore *store = [[CNContactStore alloc] init];
	NSArray *keys = @[CNContactFamilyNameKey, CNContactGivenNameKey, CNContactPhoneNumbersKey, CNContactThumbnailImageDataKey];
	CNContactFetchRequest *request = [[CNContactFetchRequest alloc] initWithKeysToFetch:keys];
	NSError *error;
	NSMutableArray *outContacts = [[NSMutableArray alloc] init];
	[store enumerateContactsWithFetchRequest:request error:&error usingBlock:^(CNContact * __nonnull contact, BOOL * __nonnull stop) {
		if (error) {
			NSLog(@"error fetching contacts %@", error);
		} else {
			[outContacts addObject:contact];
		}
	}];
	
	if (outContacts.count) {
		return outContacts;
	}
	else {
		return nil;
	}
}

+ (NSString *)displayNameForDialString:(NSString*)dialString {
	if (!ValidDomain(dialString))
		dialString = [[SCIVideophoneEngine sharedEngine] phoneNumberReformat:dialString];
	if ([dialString isEqualToString:k911])
		return Localize(@"Emergency");
	else if ([dialString isEqualToString:k411])
		return Localize(@"Information");
	else if ([dialString isEqualToString:kTechnicalSupportByVideophoneUrl])
		return Localize(@"Technical Support");
	else if ([dialString isEqualToString:kTechnicalSupportByVideophone])
		return Localize(@"Technical Support");
	else if ([dialString isEqualToString:kTechnicalSupportByPhone])
		return Localize(@"Technical Support");
	else if ([dialString isEqualToString:kCustomerInformationRepresentativeByVideophoneUrl])
		return Localize(@"Customer Care");
	else if ([dialString isEqualToString:kCustomerInformationRepresentativeByVideophone])
		return Localize(@"Customer Care");
	else if ([dialString isEqualToString:kCustomerInformationRepresentativeByPhone])
		return Localize(@"Customer Care");
	else if ([dialString isEqualToString:kCustomerInformationRepresentativeByN11])
		return Localize(@"Customer Care");
	else if ([dialString isEqualToString:@"call.svrs.tv"])
		return Localize(@"SVRS");
	else if ([dialString isEqualToString:@"sip:hold.sorensonvrs.com"])
		return Localize(@"SVRS");
	else if ([dialString isEqualToString:kSVRSByVideophoneUrl])
		return Localize(@"SVRS");
	else if ([dialString isEqualToString:kSVRSByPhone])
		return Localize(@"SVRS");
	else if ([dialString isEqualToString:kSVRSEspanolByPhone])
		return Localize(@"SVRS Español");
	else
		return nil;
}

+ (NSString *)displayNameForSCICall:(SCICall *)call {
	
	if (!call)
		return nil;
	
	NSString *result = [self displayNameForDialString:call.dialString];
	
	if (!result)
		result = [[SCIContactManager sharedManager] contactNameForNumber:call.dialString];
	
	if (!result && call.remoteName.length)
		result = call.remoteName;
	
	if (!result)
		result = FormatAsPhoneNumber(call.dialString);
	
	return result;
}

+ (NSString *)displayNameForSCICallListItem:(SCICallListItem *)callListItem {
	
	if (!callListItem)
		return nil;
	
	NSString *result = [self displayNameForDialString:callListItem.phone];
	
	if (!result)
		result = [[SCIContactManager sharedManager] contactNameForNumber:callListItem.phone];
	
	if (!result && callListItem.name.length)
		result = callListItem.name;
	
	if (!result)
		result = FormatAsPhoneNumber(callListItem.phone);
	
	return result;
}

+ (NSString *)displayNameForSCIMessageInfo:(SCIMessageInfo *)msgInfoItem {
	
	if (!msgInfoItem)
		return nil;
	
	NSString *result = [self displayNameForDialString:msgInfoItem.dialString];
	
	if (!result)
		result = [[SCIContactManager sharedManager] contactNameForNumber:msgInfoItem.dialString];
	
	if (!result && msgInfoItem.name.length)
		result = msgInfoItem.name;
	
	if (!result)
		result = FormatAsPhoneNumber(msgInfoItem.dialString);
	
	return result;
}

+ (UIImage *)imageForDeviceContact:(CNContact *)contact toFitSize:(CGSize)size
{
	UIImage *image = nil;
	if (contact.thumbnailImageData) {
		UIImage *fullImage = [UIImage imageWithData:contact.thumbnailImageData];
		image = MakeIcon(fullImage, size.width);
	}
	return image;
}

+ (NSString *)phoneLabelForPhoneNumber:(NSString *)phoneNumber {
	NSArray *contactsArray = [self findContactsByPhoneNumber:phoneNumber];
	NSString *label = nil;
	if (contactsArray.count) {
		CNContact *contact = contactsArray[0];
		for (CNLabeledValue *phoneLabels in contact.phoneNumbers) {
			CNPhoneNumber *phone = phoneLabels.value;
			NSString *tempPhone = UnformatPhoneNumber(phone.stringValue);
			
			// Add "1" to beginning of device contact
			if (tempPhone.length == 10 && ![[tempPhone substringToIndex:1] isEqualToString:@"1"]) {
				tempPhone = [@"1" stringByAppendingString:tempPhone];
			}
			
			if (phone.stringValue && [tempPhone isEqualToString:phoneNumber]) {
				label = phoneLabels.value;
			}
		}
	}
	return label;
}

+ (void)addContact:(NSString*)phoneNumber 
		 firstName:(NSString*)firstName 
		  lastName:(NSString*)lastName 
	  organization:(NSString*)org {
	
	[ContactUtilities AddressBookAccessAllowedWithCallbackBlock:^(BOOL granted) {
		CNContactStore *store = [[CNContactStore alloc] init];
		CNMutableContact *contact = [[CNMutableContact alloc] init];
		contact.familyName = lastName;
		contact.givenName = firstName;
		contact.organizationName = org;
		
		CNLabeledValue *homePhone = [CNLabeledValue labeledValueWithLabel:CNLabelHome value:[CNPhoneNumber phoneNumberWithStringValue:phoneNumber]];
		contact.phoneNumbers = @[homePhone];
		
		CNSaveRequest *request = [[CNSaveRequest alloc] init];
		[request addContact:contact toContainerWithIdentifier:nil];
		
		NSError *saveError;
		if (![store executeSaveRequest:request error:&saveError]) {
			NSLog(@"error = %@", saveError);
		}
	}];
}

@end
