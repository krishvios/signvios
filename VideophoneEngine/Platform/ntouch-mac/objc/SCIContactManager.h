//
//  SCIContactManager.h
//  ntouchMac
//
//  Created by Adam Preble on 2/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIContact.h"

extern NSNotificationName const SCINotificationContactsChanged;
extern NSNotificationName const SCINotificationContactRemoved;
extern NSNotificationName const SCINotificationFavoritesChanged;
extern NSNotificationName const SCINotificationLDAPDirectoryContactsChanged;
extern NSString * const SCINotificationKeyAddedContact;

@class SCIContact;

@interface SCIContactManager : NSObject {
	BOOL mWasAuthenticated;
}

@property (class, readonly) SCIContactManager *sharedManager;

@property (nonatomic, strong, readonly) NSArray *contacts;
@property (nonatomic, strong, readonly) NSArray *favorites;
@property (nonatomic, strong, readonly) NSArray *LDAPDirectoryContacts;

- (void)addContact:(SCIContact *)contact;
- (void)updateContact:(SCIContact *)contact;
- (void)removeContact:(SCIContact *)contact;

- (void)clearContacts;
- (void)clearThumbnailImages;

- (void)refresh;
- (void)reload;

- (void)refreshLDAPDirectoryContacts;
- (void)reloadLDAPDirectoryContacts;

- (NSArray *)fixedContacts;
- (NSArray *)compositeContacts;
- (NSArray *)compositeContactsWithServiceNumbers;

- (NSArray *)compositeFavorites;

- (NSArray *)compositeLDAPDirectoryContacts;

- (SCIContact *)contactForFavorite:(SCIContact *)favorite;
- (BOOL)favoritesFull;
- (int)favoritesCount;
- (int)favoritesMaxCount;
- (void)favoritesFavoriteMoveFromOldIndex:(int)oldIndex toNewIndex:(int)newIndex;
- (void)favoritesListSet;
- (void)clearFavorites;
- (BOOL)removeFavorite:(SCIContact *)favorite;
- (void)addLDAPContactToFavorites:(SCIContact *)LDAPFavorite phoneType:(SCIContactPhone)phoneType;
- (void)removeLDAPContactFromFavorites:(SCIContact *)LDAPFavorite phoneType:(SCIContactPhone)phoneType;
- (void)removeLDAPContactsFromFavorites;
- (SCIContact *)LDAPcontactForFavorite:(SCIContact *)favorite;

- (BOOL)companyNameEnabled;

- (void)getContact:(SCIContact **)contact phone:(SCIContactPhone *)phone forNumber:(NSString *)number;
- (NSString *)contactNameForNumber:(NSString *)number;
- (BOOL)contactExistsLikeContact:(SCIContact *)contact;
- (void)invalidateCachedNumbers;
- (void)importContactsForNumber:(NSString*)phoneNumber password:(NSString*)password;

+ (NSString *)SorensonVRSPhone;
+ (NSString *)SVRSEspañolPhone;
+ (NSString *)SorensonVRIPhone;
+ (NSString *)customerServicePhone;
+ (NSString *)customerServicePhoneFull;
+ (NSString *)techSupportPhone;
+ (NSString *)emergencyPhone;

+ (NSString *)customerServiceIP;
+ (NSString *)techSupportIP;
+ (NSString *)emergencyPhoneIP;

@end
