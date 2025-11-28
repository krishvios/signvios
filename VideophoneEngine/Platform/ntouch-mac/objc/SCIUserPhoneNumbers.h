//
//  SCIUserPhoneNumbers.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/3/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
	SCIUserPhoneNumberTypeUnknown,
	SCIUserPhoneNumberTypeSorenson,
	SCIUserPhoneNumberTypeTollFree,
	SCIUserPhoneNumberTypeLocal,
	SCIUserPhoneNumberTypeHearing,
	SCIUserPhoneNumberTypeRingGroupLocal,
	SCIUserPhoneNumberTypeRingGroupTollFree
} SCIUserPhoneNumberType;

@interface SCIUserPhoneNumbers : NSObject

@property (nonatomic, assign, readwrite) SCIUserPhoneNumberType preferredPhoneNumberType;
@property (nonatomic, strong, readwrite) NSString *sorensonPhoneNumber;
@property (nonatomic, strong, readwrite) NSString *tollFreePhoneNumber;
@property (nonatomic, strong, readwrite) NSString *localPhoneNumber;
@property (nonatomic, strong, readwrite) NSString *hearingPhoneNumber;
@property (nonatomic, strong, readwrite) NSString *ringGroupLocalPhoneNumber;
@property (nonatomic, strong, readwrite) NSString *ringGroupTollFreePhoneNumber;

- (NSString *)preferredPhoneNumber;
- (NSString *)phoneNumberOfType:(SCIUserPhoneNumberType)phoneNumberType;
- (SCIUserPhoneNumberType)typeOfPhoneNumber:(NSString *)phoneNumber;

@end

#ifdef __cplusplus
extern "C" {
#endif
extern NSString *NSStringFromSCIUserPhoneNumberType(SCIUserPhoneNumberType phoneNumberType);
#ifdef __cplusplus
}
#endif
