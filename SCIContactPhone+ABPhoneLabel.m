//
//  SCIContactPhone+ABPhoneLabel.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/22/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIContactPhone+ABPhoneLabel.h"
#import <ContactsUI/ContactsUI.h>

SCIContactPhone SCIContactPhoneFromABPhoneLabel(NSString *label)
{
	if ([label isEqualToString:(NSString*)CNLabelPhoneNumberMobile] || [label isEqualToString:(NSString*)CNLabelPhoneNumberiPhone])
		return SCIContactPhoneCell;
	else if ([label isEqualToString:(NSString*)CNLabelWork])
		return SCIContactPhoneWork;
	else if ([label isEqualToString:(NSString*)CNLabelPhoneNumberMain] || [label isEqualToString:(NSString*)CNLabelHome])
		return SCIContactPhoneHome;
	else
		return SCIContactPhoneNone;
}

NSArray *ABPhoneLabelsFromSCIContactPhone(SCIContactPhone phoneType)
{
	NSArray *res = nil;
	switch (phoneType) {
		case SCIContactPhoneCell: {
			res = @[(NSString*)CNLabelPhoneNumberMain, (NSString*)CNLabelPhoneNumberiPhone, (NSString*)CNLabelPhoneNumberMobile];
		} break;
		case SCIContactPhoneHome: {
			res = @[(NSString*)CNLabelPhoneNumberMain, (NSString*)CNLabelHome];
		} break;
		case SCIContactPhoneWork: {
			res = @[(NSString*)CNLabelWork];
		} break;
		case SCIContactPhoneNone:
		default: {
			res = @[(NSString*)CNLabelPhoneNumberHomeFax, (NSString*)CNLabelPhoneNumberPager, (NSString*)CNLabelPhoneNumberWorkFax];
		} break;
	}
	return res;
}
