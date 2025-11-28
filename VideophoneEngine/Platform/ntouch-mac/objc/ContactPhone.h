//
//  ContactPhone.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/16/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIContact.h"

@interface ContactPhone : NSObject <NSCopying>
@property (nonatomic, strong, readwrite) SCIContact *contact;
@property (nonatomic, assign, readwrite) SCIContactPhone phone;
- (id)initWithContact:(SCIContact *)contact phone:(SCIContactPhone)phone;
@end
