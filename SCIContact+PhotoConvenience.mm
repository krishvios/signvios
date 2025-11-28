//
//  SCIContact+PhotoConvenience.m
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIContact+PhotoConvenience.h"
#import "PhotoManager.h"
#import "ContactUtilities.h"

@implementation SCIContact (PhotoConvenience)

- (BOOL)setPhotoIdentifierToFacebookIfFacebookPhotoFound
{
	BOOL res = NO;
	NSUInteger count = self.phones.count;
	// Loop through all contact phones.
	for (NSUInteger i = 0; i < count; i++) {
		NSArray *contactsArray = [ContactUtilities findContactsByPhoneNumber:self.phones[i]];
		if(contactsArray.count) {
			// Loop through all found records.
			for (CNContact *contact in contactsArray) {
				if (contact.thumbnailImageData) {
					self.photoIdentifier = PhotoManagerFacebookPhotoIdentifier();
					res = YES;
				}
			}
		}
	}
	return res;
}

- (void)setPhotoIdentifierToSorensonIfSorensonPhotoFoundWithCompletion:(void (^)(BOOL success))block
{
	[[PhotoManager sharedManager] downloadSorensonPhotoForContact:self withCompletion:^(BOOL success, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
		if (success) {
			self.photoIdentifier = imageId;
			self.photoTimestamp = imageDate;
		}
		block(success);
	}];
}

@end
