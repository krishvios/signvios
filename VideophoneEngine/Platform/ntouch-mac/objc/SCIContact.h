//
//  SCIContact.h
//  ntouchMac
//
//  Created by Adam Preble on 2/8/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, SCIContactPhone) {
    SCIContactPhoneNone = -1,
    SCIContactPhoneHome = 0,
    SCIContactPhoneWork = 1,
    SCIContactPhoneCell = 2,
};
#define SCIContactPhoneCount (3)

#ifdef __cplusplus
extern "C" {
	BOOL SCIContactPhonesAreEqual( NSString * _Null_unspecified firstContactPhone, NSString * _Null_unspecified secondContactPhone);
}
#else
extern BOOL SCIContactPhonesAreEqual(NSString * _Null_unspecified firstContactPhone, NSString * _Null_unspecified secondContactPhone);
#endif

#if APPLICATION == APP_NTOUCH_IOS
@class UIImage;
#endif

extern NSString * _Nonnull const SCIRelayLanguageEnglish;
extern NSString * _Nonnull const SCIRelayLanguageSpanish;

extern NSString * _Nonnull const SCIKeyContactPhotoIdentifier;

@class SCIItemId;

@interface SCIContact : NSObject <NSCopying>

@property (nonatomic, strong, null_unspecified) NSString *name;
@property (nonatomic, strong, null_unspecified) NSString *companyName;
@property (nonatomic, strong, null_unspecified) NSString *workPhone;
@property (nonatomic, strong, null_unspecified) NSString *homePhone;
@property (nonatomic, strong, null_unspecified) NSString *cellPhone;
@property (nonatomic, strong, null_unspecified) NSString *relayLanguage;
@property (nonatomic, strong, null_unspecified) NSString *photoIdentifier;
@property (nonatomic, strong, null_unspecified) NSString *photoTimestamp;
@property (nonatomic, assign) BOOL vco;
@property (nonatomic, assign) int ringPattern;
@property (nonatomic, assign) BOOL homeIsFavorite;  // Use -isFavoriteForPhoneType method to check if favorited.
@property (nonatomic, assign) BOOL workIsFavorite;  // These variables are only used to flag that favorite needs to be
@property (nonatomic, assign) BOOL cellIsFavorite;  // created/removed on Save/Edit
@property (nonatomic, readonly, null_unspecified) SCIItemId *contactId;
@property (nonatomic, readonly, null_unspecified) NSString *searchString;
@property (nonatomic, readonly) BOOL isFavorite;
@property (nonatomic, readonly) BOOL isFixed;
@property (nonatomic, readonly) BOOL isLDAPContact;
@property (nonatomic, null_unspecified) NSNumber *favoriteOrder;

#if APPLICATION == APP_NTOUCH_IOS
@property (nonatomic, strong, nullable) UIImage * contactPhotoThumbnail;
#endif

+ (null_unspecified SCIContact *)fixedContactWithName:(null_unspecified NSString *)name phone:(null_unspecified NSString *)phone photoIdentifier:(null_unspecified NSString *)photoIdentifier relayLanguage:(null_unspecified NSString *)relayLanguage;

@property (nonatomic, readonly, nonnull) NSString *nameOrCompanyName;
@property (nonatomic, readonly, nonnull) NSString *nameAndOrCompanyName;

- (null_unspecified NSArray *)phonesOfTypes:(null_unspecified NSArray *)types;
@property (nonatomic, readonly, nullable) NSArray *phones;
@property (nonatomic, readonly) NSUInteger phonesCount;
- (SCIContactPhone)phoneAtIndex:(NSUInteger)index;
- (null_unspecified NSString *)phoneOfType:(SCIContactPhone)type;
- (void)setPhone:(null_unspecified NSString *)phone ofType:(SCIContactPhone)type;

- (NSComparisonResult)compare:(null_unspecified SCIContact *)other;
- (BOOL)hasSamePhoneNumbersAsContact:(null_unspecified SCIContact *)other;
@property (nonatomic, readonly) BOOL isResolved;
@property (nonatomic, readonly) BOOL isResolvedFavorite;
- (BOOL)isFavoriteForPhoneType:(SCIContactPhone)phoneType;

@end


@interface NSString (SCIContactAdditions)
@property (nonatomic, readonly, null_unspecified) NSString *normalizedSearchString;
@property (nonatomic, readonly, null_unspecified) NSString *normalizedDialString;
+ (null_unspecified NSString *)stringFromContactPhoneType:(SCIContactPhone)type;
+ (null_unspecified NSString *)lowercaseStringFromContactPhoneType:(SCIContactPhone)type;
@end
