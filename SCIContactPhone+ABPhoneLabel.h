//
//  SCIContactPhone+ABPhoneLabel.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/22/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIContact.h"

SCIContactPhone SCIContactPhoneFromABPhoneLabel(NSString *label);
NSArray *ABPhoneLabelsFromSCIContactPhone(SCIContactPhone phoneType);
