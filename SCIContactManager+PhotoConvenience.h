//
//  SCIContactManager+PhotoConvenience.h
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIContactManager.h"

@interface SCIContactManager (PhotoConvenience)

- (NSUInteger)setPhotoIdentifierToFacebookForAllContactsIfFacebookPhotoFoundWithContactBlock:(void (^)(SCIContact *contact, BOOL *bail))block;
- (void)setPhotoIdentifierToSorensonForAllContactsIfSorensonPhotoFoundWithContactBlock:(void (^)(SCIContact *contact, BOOL *bail))block completion:(void (^)(NSUInteger count))handler;

@end
