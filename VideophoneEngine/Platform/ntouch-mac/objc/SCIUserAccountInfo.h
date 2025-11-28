//
//  SCIUserAccountInfo.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/22/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCIUserAccountInfo : NSObject
@property (nonatomic, strong, readwrite) NSString *name;
@property (nonatomic, strong, readwrite) NSString *preferredNumber;
@property (nonatomic, strong, readwrite) NSString *phoneNumber;
@property (nonatomic, strong, readwrite) NSString *tollFreeNumber;
@property (nonatomic, strong, readwrite) NSString *localNumber;
@property (nonatomic, strong, readwrite) NSString *associatedPhone;
@property (nonatomic, strong, readwrite) NSString *currentPhone;
@property (nonatomic, strong, readwrite) NSString *address1;
@property (nonatomic, strong, readwrite) NSString *address2;
@property (nonatomic, strong, readwrite) NSString *city;
@property (nonatomic, strong, readwrite) NSString *state;
@property (nonatomic, strong, readwrite) NSString *zipCode;
@property (nonatomic, strong, readwrite) NSString *email;
@property (nonatomic, strong, readwrite) NSString *mainEmail;
@property (nonatomic, strong, readwrite) NSString *pagerEmail;
@property (nonatomic, assign, readwrite) NSNumber *signMailEnabled;
@property (nonatomic, strong, readwrite) NSString *ringGroupLocalNumber;
@property (nonatomic, strong, readwrite) NSString *ringGroupTollFreeNumber;
@property (nonatomic, strong, readwrite) NSString *ringGroupName;
@property (nonatomic, strong, readwrite) NSNumber *mustCallCIR;
@property (nonatomic, strong, readwrite) NSNumber *userRegistrationDataRequired;

@end
