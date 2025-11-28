//
//  ContactPhone.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/16/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "ContactPhone.h"

@interface ContactPhone ()
{
	SCIContact *mContact;
	SCIContactPhone mPhone;
}
@end

@implementation ContactPhone

@synthesize contact = mContact;
@synthesize phone = mPhone;

- (id)initWithContact:(SCIContact *)contact phone:(SCIContactPhone)phone
{
	self = [super init];
	if (self) {
		mContact = contact;
		mPhone = phone;
	}
	return self;
}

- (id)copyWithZone:(NSZone *)zone
{
	return [[ContactPhone alloc] initWithContact:self.contact phone:self.phone];
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	if ([object isKindOfClass:self.class]) {
		ContactPhone *comparand = (ContactPhone *)object;
		res = ([self.contact isEqual:comparand.contact]
			   && self.phone == comparand.phone);
	}
	return res;
}


@end
