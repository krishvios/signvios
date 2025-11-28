//
//  SCIContactManager+PhotoConvenience.m
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIContactManager+PhotoConvenience.h"
#import "SCIContact+PhotoConvenience.h"
#import "NSArray+BNREnumeration.h"
#import "PhotoManager.h"

@implementation SCIContactManager (PhotoConvenience)

- (NSUInteger)setPhotoIdentifierToFacebookForAllContactsIfFacebookPhotoFoundWithContactBlock:(void (^)(SCIContact *contact, BOOL *bail))block
{
	NSUInteger count = 0;
	
	for (SCIContact *contact in self.contacts)
	{
		BOOL bail = NO;
		block(contact, &bail);
		if (bail) break;
		
		//Only try to set the photoIdentifier to Facebook if the photoIdentifier's type is not presently Sorenson or Facebook.
		BOOL set = NO;
		PhotoType photoType = PhotoManagerPhotoTypeForContact(contact);
		if (photoType != PhotoTypeSorenson &&
			photoType != PhotoTypeFacebook) {
			set = [contact setPhotoIdentifierToFacebookIfFacebookPhotoFound];
		}
		
		if (set)
		{
			[self updateContact:contact];
			count++;
		}
	}
	
	return count;
}

- (void)setPhotoIdentifierToSorensonForAllContactsIfSorensonPhotoFoundWithContactBlock:(void (^)(SCIContact *contact, BOOL *bail))block completion:(void (^)(NSUInteger count))handler
{
	__block NSUInteger count = 0;
	[self.contacts bnr_enumerateOnQueue:NULL continuation:^(id object, NSUInteger index, void (^continuation)(void)) {
		SCIContact *contact = (SCIContact *)object;
		
		BOOL bail = NO;
		block(contact, &bail);
		
		if (!bail) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-retain-cycles"
			[contact setPhotoIdentifierToSorensonIfSorensonPhotoFoundWithCompletion:^(BOOL success) {
				if (success) {
					[self updateContact:contact];
					count++;
				}
				continuation();
			}];
#pragma clang diagnostic pop
		} else {
			handler(count);
		}
	}
							 completion:^{
								 handler(count);
							 }];
}

@end
