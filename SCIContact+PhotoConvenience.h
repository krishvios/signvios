//
//  SCIContact+PhotoConvenience.h
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIContact.h"

@interface SCIContact (PhotoConvenience)

- (BOOL)setPhotoIdentifierToFacebookIfFacebookPhotoFound;
- (void)setPhotoIdentifierToSorensonIfSorensonPhotoFoundWithCompletion:(void (^)(BOOL success))block;

@end

