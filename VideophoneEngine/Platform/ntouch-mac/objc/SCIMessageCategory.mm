//
//  SCIMessageCategory.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/14/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageCategory.h"
#import "SCIVideophoneEngine.h"
#import "SCIItemId.h"
#import "stiSVX.h"

@interface SCIMessageCategory () <NSCopying>
{
	SCIItemId *mCategoryId;
	SCIItemId *mSupercategoryId;
	NSUInteger mType;
	NSString *mLongName;
	NSString *mShortName;
}
@end

@implementation SCIMessageCategory

@synthesize categoryId = mCategoryId;
@synthesize supercategoryId = mSupercategoryId;
@synthesize type = mType;
@synthesize longName = mLongName;
@synthesize shortName = mShortName;

+ (SCIMessageCategory *)messageCategoryWithId:(SCIItemId *)categoryId 
							  supercategoryId:(SCIItemId *)supercategoryId 
										 type:(NSUInteger)type
									 longName:(NSString *)longName 
									shortName:(NSString *)shortName
{
	SCIMessageCategory *res = [[SCIMessageCategory alloc] init];
	res.categoryId = categoryId;
	res.supercategoryId = supercategoryId;
	res.type = type;
	res.longName = longName;
	res.shortName = shortName;
	return res;
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone *)zone
{
	SCIMessageCategory *copy = [[SCIMessageCategory allocWithZone:zone] init];
	NSArray *keys = [NSArray arrayWithObjects:@"categoryId", @"supercategoryId", @"type", @"longName", @"shortName", nil];
	for (NSString *key in keys)
	{
		[copy setValue:[self valueForKey:key] forKey:key];
	}
	return copy;
}

#pragma mark - overridden NSObject instance methods

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@ %p type=%lu shortName=%@ longName=%@>", NSStringFromClass([self class]), self, (unsigned long)self.type, self.shortName, self.longName];
}

- (BOOL)isEqual:(id)object
{
	BOOL res = NO;
	
	if ([object isKindOfClass:[SCIMessageCategory class]]) {
		SCIMessageCategory *comparand = (SCIMessageCategory *)object;
		res = [self.categoryId isEqualToItemId:comparand.categoryId];
	}
	
	return res;
}

@end

const NSUInteger SCIMessageTypeNone = estiMT_NONE;									/*!< No message type specified. */
const NSUInteger SCIMessageTypeSignMail = estiMT_SIGN_MAIL;							/*!< Sign mail message type. */
const NSUInteger SCIMessageTypeFromSorenson = estiMT_MESSAGE_FROM_SORENSON;			/*!< Message from Sorenson. */
const NSUInteger SCIMessageTypeFromOther = estiMT_MESSAGE_FROM_OTHER;				/*!< Message from another entity. */
const NSUInteger SCIMessageTypeNews = estiMT_NEWS;									/*!< News item. */
const NSUInteger SCIMessageTypeFromTechSupport = estiMT_FROM_TECH_SUPPORT;			/*!< Message from Sorenson Tech support. */
const NSUInteger SCIMessageTypeP2PSignMail = estiMT_P2P_SIGNMAIL;					/*!< Sign mail message type for point-to-point. */
const NSUInteger SCIMessageTypeThirdPartyVideoMail = estiMT_THIRD_PARTY_VIDEO_MAIL;	/*!< UNUSED */
const NSUInteger SCIMessageTypeSorensonProductTips = estiMT_SORENSON_PRODUCT_TIPS;	/*!< UNUSED */
const NSUInteger SCIMessageTypeDirectSignMail = estiMT_DIRECT_SIGNMAIL;				/*!< Direct SignMail message type. */
const NSUInteger SCIMessageTypeInteractiveCare = estiMT_INTERACTIVE_CARE;			/*!< Message sent by an Interactive Care Client. */
