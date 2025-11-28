//
//  SCIMessageCategory.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/14/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIItemId;

@interface SCIMessageCategory : NSObject
+ (SCIMessageCategory *)messageCategoryWithId:(SCIItemId *)itemId 
							  supercategoryId:(SCIItemId *)parentItemId 
										 type:(NSUInteger)type
									 longName:(NSString *)longName 
									shortName:(NSString *)shortName;
@property (nonatomic, strong, readwrite) SCIItemId *categoryId;
@property (nonatomic, strong, readwrite) SCIItemId *supercategoryId;
@property (nonatomic, assign, readwrite) NSUInteger type;
@property (nonatomic, strong, readwrite) NSString *longName;
@property (nonatomic, strong, readwrite) NSString *shortName;
@end

extern const NSUInteger SCIMessageTypeNone;					/*!< No message type specified. */
extern const NSUInteger SCIMessageTypeSignMail;				/*!< Sign mail message type. */
extern const NSUInteger SCIMessageTypeFromSorenson;			/*!< Message from Sorenson. */
extern const NSUInteger SCIMessageTypeFromOther;			/*!< Message from another entity. */
extern const NSUInteger SCIMessageTypeNews;					/*!< News item. */
extern const NSUInteger SCIMessageTypeFromTechSupport;		/*!< Message from Sorenson Tech support. */
extern const NSUInteger SCIMessageTypeP2PSignMail;			/*!< Sign mail message type for point-to-point. */
extern const NSUInteger SCIMessageTypeThirdPartyVideoMail;	/*!< UNUSED */
extern const NSUInteger SCIMessageTypeSorensonProductTips;	/*!< UNUSED */
extern const NSUInteger SCIMessageTypeDirectSignMail;		/*!< Direct SignMail message type. */
extern const NSUInteger SCIMessageTypeInteractiveCare;		/*!< Message sent by an Interactive Care Client. */
