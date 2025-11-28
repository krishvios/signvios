//
//  SCIContactManager.m
//  ntouchMac
//
//  Created by Adam Preble on 2/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIContactManager.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCIContactCpp.h"
#import "IstiContactManager.h"
#import "ContactPhone.h"
#import "SCIContact.h"
#import "SCICallListManager.h"
#import "SCIContact+PhotoConvenience.h"
#import "PhotoManager.h"
#import "SCIItemId.h"
#import "SCIItemIdCpp.h"
#import "ContactListItem.h"
#import "IstiLDAPDirectoryContactManager.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"


#ifdef stiDEBUG
#define kTechnicalSupportByVideophoneUrl @"sip:18012879403@vrs-eng.qa.sip.svrs.net"
#define kCustomerInformationRepresentativeByVideophoneUrl @"sip:18667566729@vrs-eng.qa.sip.svrs.net"
#define kFieldHelpDeskByVideophoneUrl @"sip:vrs-eng.qa.sip.svrs.net"
#define kEmergencyByVideophoneUrl @"sip:911@vrs-eng.qa.sip.svrs.net"
#else
#define kTechnicalSupportByVideophoneUrl @"sip:18012879403@tech.svrs.tv"
#define kCustomerInformationRepresentativeByVideophoneUrl @"sip:18667566729@cir.svrs.tv"
#define kFieldHelpDeskByVideophoneUrl @"sip:fhd.svrs.tv"
#define kEmergencyByVideophoneUrl @"sip:911@hold.sorensonvrs.com"
#endif


NSNotificationName const SCINotificationContactsChanged = @"SCINotificationContactsChanged";
NSNotificationName const SCINotificationContactRemoved = @"SCINotificationContactRemoved";
NSNotificationName const SCINotificationFavoritesChanged = @"SCINotificationFavoritesChanged";
NSNotificationName const SCINotificationLDAPDirectoryContactsChanged = @"SCINotificationLDAPDirectoryContactsChanged";
NSString * const SCINotificationKeyAddedContact = @"SCINotificationKeyAddedContact";

@interface SCIContactManager () {
	NSDictionary *mContactsChangedUserInfo;
	NSDictionary *mContactsAndPhonesForNumbers;
}
- (void)refreshWithoutNotification;

@property (nonatomic, strong, readwrite) NSArray *contacts;
@property (nonatomic, strong, readwrite) NSArray *favorites;
@property (nonatomic, strong, readwrite) NSArray *LDAPDirectoryContacts;

@end

@implementation SCIContactManager

@synthesize contacts = mContacts;
@synthesize favorites = mFavorites;
@synthesize LDAPDirectoryContacts = mLDAPDirectoryContacts;

static SCIContactManager *sharedManager = nil;

+ (SCIContactManager *)sharedManager
{
	if (!sharedManager)
	{
		sharedManager = [[SCIContactManager alloc] init];
	}
	return sharedManager;
}

- (id)init
{
	if ((self = [super init]))
	{
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyAuthenticatedDidChange:)
													 name:SCINotificationAuthenticatedDidChange
												   object:[SCIVideophoneEngine sharedEngine]];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyInterfaceModeChanged:)
													 name:SCINotificationInterfaceModeChanged
												   object:[SCIVideophoneEngine sharedEngine]];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (SCIContact *)contactForIstiContact:(const IstiContactSharedPtr &)istiContact
{
	for (SCIContact *contact in self.contacts)
	{
		if (contact.istiContact == istiContact)
			return contact;
	}
	return nil;
}

- (SCIContact *)contactForLDAPIstiContact:(const IstiContactSharedPtr &)istiContact
{
	for (SCIContact *contact in self.LDAPDirectoryContacts)
	{
		if (contact.istiContact == istiContact)
			return contact;
	}
	return nil;
}


- (IstiContactManager *)istiContactManager
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();
	return manager;
}

#ifdef stiLDAP_CONTACT_LIST
- (IstiLDAPDirectoryContactManager *)istiLDAPDirectoryContactManager
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiLDAPDirectoryContactManager *manager = engine->LDAPDirectoryContactManagerGet();
	return manager;
}
#endif

- (void)refreshWithoutNotification
{
	IstiContactManager *manager = [self istiContactManager];
	manager->Lock();
	
	int numContacts = manager->getNumContacts();
	NSMutableArray *contacts = [[NSMutableArray alloc] initWithCapacity:numContacts];
	
	for (int i = 0; i < numContacts; i++)
	{
		IstiContactSharedPtr cppContact = manager->getContact(i);
		SCIContact *contact = [[SCIContact alloc] initWithIstiContact:cppContact];
		[contacts addObject:contact];
	}
	[contacts sortUsingSelector:@selector(compare:)];
	
	self.contacts = contacts;
	
	int numFavorites = manager->FavoritesCountGet();
	NSMutableArray *favorites = [[NSMutableArray alloc] initWithCapacity:numFavorites];
	
	for (int i = 0; i < numFavorites; i++)
	{
		CstiFavoriteSharedPtr cppFavorite = manager->FavoriteByIndexGet(i);
		if (cppFavorite) {
			SCIContact *favorite = [[SCIContact alloc] initWithCstiFavorite:cppFavorite];
			if (favorite) {
				[favorites addObject:favorite];
			}
		}
	}
	[favorites sortUsingSelector:@selector(compare:)];
	
	self.favorites = favorites;
	manager->Unlock();
	
	[self invalidateCachedNumbers];
	[[SCICallListManager sharedManager] updateNamesAndNotify:YES];
	
#if APPLICATION == APP_NTOUCH_IOS
	[self refreshDataSharing];
#endif
}

- (void)refresh
{
	[self refreshWithoutNotification];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactsChanged
														object:self];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationFavoritesChanged
														object:self];
}

- (void)reload
{
	self.istiContactManager->ReloadContactsFromCore();
}


- (void)refreshDataSharing API_UNAVAILABLE(macos) {
	NSMutableArray *contactsArray = [NSMutableArray new];
	
	for (SCIContact *contact in self.contacts) {
		NSMutableDictionary *numbers = [NSMutableDictionary new];
		numbers[@"home"] = contact.homePhone;
		numbers[@"work"] = contact.workPhone;
		numbers[@"cell"] = contact.cellPhone;
		NSMutableDictionary *contactDict = [NSMutableDictionary new];
		contactDict[@"name"] = contact.name;
		contactDict[@"companyName"] = contact.companyName;
		contactDict[@"numbers"] = [numbers copy];
		[contactsArray addObject:[contactDict copy]];
	}
	
	NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.com.sorenson.ntouch.contacts"];
	[defaults setObject:contactsArray forKey:@"contacts"];
}


- (void)refreshLDAPDirectoryContactsWithoutNotification
{
#ifdef stiLDAP_CONTACT_LIST

	IstiLDAPDirectoryContactManager *manager = [self istiLDAPDirectoryContactManager];
	
	int numContacts = manager->getNumContacts();
	NSMutableArray *LDAPDirectoryContacts = [[NSMutableArray alloc] initWithCapacity:numContacts];
	
	for (int i = 0; i < numContacts; i++)
	{
		IstiContactSharedPtr cppContact = manager->getContact(i);
		SCIContact *contact = [[SCIContact alloc] initWithIstiContactFromLDAPDirectory:cppContact];
		[LDAPDirectoryContacts addObject:contact];
	}
	[LDAPDirectoryContacts sortUsingSelector:@selector(compare:)];
	
	self.LDAPDirectoryContacts = LDAPDirectoryContacts;
	
	[self invalidateCachedNumbers];
	[[SCICallListManager sharedManager] updateNamesAndNotify:YES];
	
#endif
}


- (void)refreshLDAPDirectoryContacts
{
	[self refreshLDAPDirectoryContactsWithoutNotification];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationLDAPDirectoryContactsChanged
														object:self];
}

- (void)reloadLDAPDirectoryContacts
{
#ifdef stiLDAP_CONTACT_LIST
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		IstiLDAPDirectoryContactManager *manager = [self istiLDAPDirectoryContactManager];
		manager->ReloadContactsFromLDAPDirectory();
	});
#endif
}


- (void)refreshWithAddedIstiContact:(const IstiContactSharedPtr &)istiContact
{
	[self refreshWithoutNotification];
	
	auto tempIsticontact = istiContact;
	__weak SCIContact *addedContact = [self contactForIstiContact:istiContact];
	
	// If no photo is assigned to the contact, try to set the profile photo for the contact.
	if ([addedContact.photoIdentifier isEqualToString:@"00000000-0000-0000-0000-000000000001"]) {

		[addedContact setPhotoIdentifierToSorensonIfSorensonPhotoFoundWithCompletion:^(BOOL success) {
			if (success) {
				SCIContact *temp = [self contactForIstiContact:tempIsticontact];
				addedContact.homeIsFavorite = temp.homeIsFavorite;
				addedContact.workIsFavorite = temp.workIsFavorite;
				addedContact.cellIsFavorite = temp.cellIsFavorite;
				[[SCIContactManager sharedManager] updateContact:addedContact];
			} else {
			}
			
			NSDictionary *userInfo = nil;
			if (addedContact)
			{
				userInfo = [NSDictionary dictionaryWithObjectsAndKeys:addedContact, SCINotificationKeyAddedContact, nil];
			}
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactsChanged
																object:self
															  userInfo:userInfo];
		}];
	} else {
		// Set the timestamp for the photo because it isn't saved when the contact is added to core.
		[[SCIVideophoneEngine sharedEngine] setPhotoTimestampForContact:addedContact completion:^{
			NSDictionary *userInfo = nil;
			if (addedContact)
			{
				userInfo = [NSDictionary dictionaryWithObjectsAndKeys:addedContact, SCINotificationKeyAddedContact, nil];
			}
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactsChanged
																object:self
															  userInfo:userInfo];
		}];
	}
}

- (void)refreshWithEditedIstiContact:(const IstiContactSharedPtr &)istiContact
{
	[self refreshWithoutNotification];
	auto tempIsticontact = istiContact;
	__weak SCIContact *editedContact = [self contactForIstiContact:istiContact];
	
	
	// If no photo is assigned to the contact, try to set the profile photo for the contact.
	if ([editedContact.photoIdentifier isEqualToString:@"00000000-0000-0000-0000-000000000001"]) {

		[editedContact setPhotoIdentifierToSorensonIfSorensonPhotoFoundWithCompletion:^(BOOL success) {
			if (success) {
				SCIContact *temp = [self contactForIstiContact:tempIsticontact];
				editedContact.homeIsFavorite = temp.homeIsFavorite;
				editedContact.workIsFavorite = temp.workIsFavorite;
				editedContact.cellIsFavorite = temp.cellIsFavorite;

				[[SCIContactManager sharedManager] updateContact:editedContact];
			} else {
			}
			
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactsChanged
																object:self];
		}];
	} else {
		// Set the timestamp for the photo because it isn't saved when the contact is added to core.
		[[SCIVideophoneEngine sharedEngine] setPhotoTimestampForContact:editedContact completion:^{
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactsChanged
																object:self];
		}];
	}
}

- (void)refreshWithRemoveItemId:(const CstiItemId &)cstiItemId
{
	SCIItemId *itemId = [SCIItemId itemIdWithCstiItemId:cstiItemId];
	SCIContact *removedContact = nil;
	for (SCIContact *contact in self.contacts)
	{
		if ([contact.contactId isEqual:itemId]) {
			removedContact = contact;
			break;
		}
	}
	
	if (removedContact != nil) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationContactRemoved
															object:removedContact];
	}
	
	[self refresh];
}

- (void)addContact:(SCIContact *)contact
{
	IstiContactManager *manager = [self istiContactManager];
	IstiContactSharedPtr istiContact = [contact generateIstiContact];
	manager->addContact(istiContact);
}

- (void)updateContact:(SCIContact *)contact
{
	IstiContactSharedPtr istiContact = contact.istiContact;
	
	if (istiContact) {
		istiContact->HomeFavoriteOnSaveSet(contact.homeIsFavorite);
		istiContact->WorkFavoriteOnSaveSet(contact.workIsFavorite);
		istiContact->MobileFavoriteOnSaveSet(contact.cellIsFavorite);
		self.istiContactManager->updateContact(istiContact);
	}
	
	[self refresh];
}

- (void)removeContact:(SCIContact *)contact
{
	IstiContactSharedPtr istiContact = contact.istiContact;
	if (istiContact)
		self.istiContactManager->removeContact(istiContact);
}

- (void)clearContacts
{
	IstiContactManager *manager = [self istiContactManager];
	manager->clearContacts();
}

- (void)clearThumbnailImages
{
#if APPLICATION == APP_NTOUCH_IOS
	for (SCIContact *contact in self.contacts)
	{
		if (contact.contactPhotoThumbnail)
			contact.contactPhotoThumbnail = nil;
	}
#endif
}

- (NSArray *)fixedContacts
{
	NSMutableArray *_fixedContacts = [[NSMutableArray alloc] init];
	
	if ([SCIVideophoneEngine sharedEngine].VRIFeatures) {
		[_fixedContacts addObject:[SCIContact fixedContactWithName:@"SCIS VRI"
															 phone:[SCIContactManager SorensonVRIPhone]
												   photoIdentifier:PhotoManagerVRIImageIdentifier()
													 relayLanguage:SCIRelayLanguageEnglish]];
	}
	
	if ([[SCIVideophoneEngine sharedEngine] interfaceMode] != SCIInterfaceModeHearing) {
		[_fixedContacts addObject: [SCIContact fixedContactWithName:@"SVRS"
															  phone:[SCIContactManager SorensonVRSPhone]
													photoIdentifier:PhotoManagerSorensonVRSImageIdentifier()
													  relayLanguage:SCIRelayLanguageEnglish]];
	}
	
	if ([[SCIVideophoneEngine sharedEngine] SpanishFeatures] &&
		[[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModeStandard) {
		[_fixedContacts addObject:[SCIContact fixedContactWithName:@"SVRS Español"
															 phone:[SCIContactManager SVRSEspañolPhone]
												   photoIdentifier:PhotoManagerRapidoVRSImageIdentifier()
													 relayLanguage:SCIRelayLanguageSpanish]];
	}
	
	[_fixedContacts addObject:[SCIContact fixedContactWithName:@"Customer Care"
														 phone:[SCIContactManager customerServicePhoneFull]
											   photoIdentifier:PhotoManagerCIRImageIdentifier()
												 relayLanguage:SCIRelayLanguageEnglish]];
	
	return [_fixedContacts copy];
}

- (NSArray *)contacts
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		return nil;
	} else {
		return mContacts;
	}
}

- (NSArray *)compositeContacts
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		return [self fixedContacts];
	} else {
		return [[self fixedContacts] arrayByAddingObjectsFromArray:[self contacts]];
	}
}

- (NSArray *)compositeContactsWithServiceNumbers
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	NSArray *n11Contacts = [NSArray arrayWithObjects:
							[SCIContact fixedContactWithName:@"Emergency" phone:[SCIContactManager emergencyPhone] photoIdentifier:nil relayLanguage:SCIRelayLanguageEnglish],
							[SCIContact fixedContactWithName:@"Information" phone:@"411" photoIdentifier:nil relayLanguage:SCIRelayLanguageEnglish], nil];
	NSArray *contacts = [[self fixedContacts] arrayByAddingObjectsFromArray:n11Contacts];
	if (publicMode) {
		return contacts;
	} else {
		return [contacts arrayByAddingObjectsFromArray:[self contacts]];
	}
}

- (NSArray *)favorites
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		return nil;
	} else {
		return mFavorites;
	}
}

- (NSArray *)compositeFavorites
{
	return [self favorites];
}

- (NSArray *)LDAPDirectoryContacts
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		return nil;
	} else {
		return mLDAPDirectoryContacts;
	}
}

- (NSArray *)compositeLDAPDirectoryContacts
{
	return [self LDAPDirectoryContacts];
}


- (SCIContact *)contactForFavorite:(SCIContact *)favorite
{
  	CstiFavoriteSharedPtr pFavorite = [self istiContactManager]->FavoriteByPhoneNumberIdGet(favorite.contactId.cstiItemId);
	IstiContactSharedPtr pContact = [self istiContactManager]->ContactByFavoriteGet(pFavorite);
	return [self contactForIstiContact:pContact];
}

- (BOOL)favoritesFull
{
	return ([self istiContactManager]->FavoritesFull() == true) ? YES : NO;
}

- (int)favoritesCount
{
	return ([self istiContactManager]->FavoritesCountGet());
}

- (int)favoritesMaxCount
{
	return ([self istiContactManager]->FavoritesMaxCountGet());
}

- (void)favoritesFavoriteMoveFromOldIndex:(int)oldIndex toNewIndex:(int)newIndex
{
	NSLog(@"old %d, new %d",oldIndex, newIndex);
	[self istiContactManager]->FavoriteMove(oldIndex, newIndex);
}

- (void)favoritesListSet
{
	// Create a Favorite List Object
	std::vector<CstiFavorite> favoriteList;
	for (SCIContact *favorite in self.favorites) {
		CstiFavoriteSharedPtr pFavorite = [self istiContactManager]->FavoriteByPhoneNumberIdGet(favorite.contactId.cstiItemId);
		favoriteList.push_back(*pFavorite);

	}

	[self istiContactManager]->FavoriteListSet(favoriteList);
}

- (void)clearFavorites
{
    // Create an empty Favorite List Object
    std::vector<CstiFavorite> favoriteList;
    
    [self istiContactManager]->FavoriteListSet(favoriteList);
}

#ifdef stiLDAP_CONTACT_LIST
- (SCIContact *)LDAPcontactForFavorite:(SCIContact *)favorite
{
	IstiLDAPDirectoryContactManager *manager = [self istiLDAPDirectoryContactManager];
	CstiFavoriteSharedPtr pFavorite = [self istiContactManager]->FavoriteByPhoneNumberIdGet(favorite.contactId.cstiItemId);
	IstiContactSharedPtr pContact = manager->ContactByFavoriteGet(pFavorite);

	return [self contactForLDAPIstiContact:pContact];
}
#endif

- (void)addLDAPContactToFavorites:(SCIContact *)LDAPFavorite phoneType:(SCIContactPhone)phoneType
{
	// Convert SCIContactPhone type to EPhoneNumberType
	CstiContactListItem::EPhoneNumberType cstiType = CstiContactListItem::ePHONE_HOME;
	switch (phoneType) {
		case SCIContactPhoneHome: cstiType = CstiContactListItem::ePHONE_HOME; break;
		case SCIContactPhoneWork: cstiType = CstiContactListItem::ePHONE_WORK; break;
		case SCIContactPhoneCell: cstiType = CstiContactListItem::ePHONE_CELL; break;
		default: cstiType = CstiContactListItem::ePHONE_HOME; break;
	}

	IstiContactSharedPtr istiContact = LDAPFavorite.istiContact;
	
	const CstiItemId itemId = istiContact->PhoneNumberPublicIdGet(cstiType);

	[self istiContactManager]->FavoriteAdd(itemId,false);
}

- (void)removeLDAPContactFromFavorites:(SCIContact *)LDAPFavorite phoneType:(SCIContactPhone)phoneType
{
	// Convert SCIContactPhone type to EPhoneNumberType
	CstiContactListItem::EPhoneNumberType cstiType = CstiContactListItem::ePHONE_HOME;
	switch (phoneType) {
		case SCIContactPhoneHome: cstiType = CstiContactListItem::ePHONE_HOME; break;
		case SCIContactPhoneWork: cstiType = CstiContactListItem::ePHONE_WORK; break;
		case SCIContactPhoneCell: cstiType = CstiContactListItem::ePHONE_CELL; break;
		default: cstiType = CstiContactListItem::ePHONE_HOME; break;
	}
	
	IstiContactSharedPtr istiContact = LDAPFavorite.istiContact;
	
	const CstiItemId itemId = istiContact->PhoneNumberPublicIdGet(cstiType);
	
	[self istiContactManager]->FavoriteRemove(itemId);
}

- (void)removeLDAPContactsFromFavorites
{
	for (SCIContact *favorite in self.favorites) {
		CstiFavoriteSharedPtr cstiFavorite =  favorite.cstiFavorite;
		if (!cstiFavorite->IsContact()) {
			[self istiContactManager]->FavoriteRemove(favorite.contactId.cstiItemId);
		}
	}
}

- (BOOL)removeFavorite:(SCIContact *)favorite
{
	stiHResult result = [self istiContactManager]->FavoriteRemove(favorite.contactId.cstiItemId);
	return (result == stiRESULT_SUCCESS);
}

- (BOOL)companyNameEnabled
{
	return ([self istiContactManager]->CompanyNameEnabled() == true) ? YES : NO;
}

+ (NSString *)SorensonVRSPhone
{
	return @"18663278877";
}
+ (NSString *)SVRSEspañolPhone
{
	return @"18669877528";
}
+ (NSString *)SorensonVRIPhone
{
	return @"18012077353";
}
+ (NSString *)customerServicePhone
{
	return @"611";
}
+ (NSString *)customerServicePhoneFull
{
	return @"18667566729";
}
+ (NSString *)techSupportPhone
{
	return @"18012879403";
}
+ (NSString *)emergencyPhone
{
	return @"911";
}
+ (NSString *)customerServiceIP
{
	return kCustomerInformationRepresentativeByVideophoneUrl;
}
+ (NSString *)techSupportIP
{
	return kTechnicalSupportByVideophoneUrl;
}
+ (NSString *)emergencyPhoneIP
{
	return kEmergencyByVideophoneUrl;
}


#pragma mark - looking up caching contact/phone pairs for numbers

- (void)getContact:(SCIContact **)contactOut phone:(SCIContactPhone *)phoneOut forNumber:(NSString *)number
{
	SCIContact *contact = nil;
	SCIContactPhone phone = SCIContactPhoneNone;

	ContactPhone *pair = [mContactsAndPhonesForNumbers objectForKey:[number normalizedDialString]];
	if (pair) {
		contact = pair.contact;
		phone = pair.phone;
	}
	else {
		[self getFixedContact:&contact phone:&phone forIPAddress:number];
	}
	
	if (contactOut) {
		*contactOut = contact;
	}
	if (phoneOut) {
		*phoneOut = phone;
	}
}

- (BOOL)getFixedContact:(SCIContact **)contact phone:(SCIContactPhone *)phone forIPAddress:(NSString *)address
{
	BOOL res = NO;
	if ([address isEqualToString:[SCIContactManager customerServiceIP]]) {
		[self getContact:contact phone:phone forNumber:[SCIContactManager customerServicePhoneFull]];
		res = YES;
	} else if ([address isEqualToString:[SCIContactManager techSupportIP]]) {
		[self getContact:contact phone:phone forNumber:[SCIContactManager techSupportPhone]];
		res = YES;
	}
	else if ([address isEqualToString:[SCIContactManager emergencyPhoneIP]]) {
		[self getContact:contact phone:phone forNumber:[SCIContactManager emergencyPhone]];
		res = YES;
	}
	return res;
}

- (NSString *)contactNameForNumber:(NSString *)number
{
	SCIContact *contact;
	SCIContactPhone phone;
	[self getContact:&contact phone:&phone forNumber:number];
	if (contact) {
		return [contact nameOrCompanyName];
	} else {
		return nil;
	}
}

- (BOOL)contactExistsLikeContact:(SCIContact *)contact
{
	BOOL duplicate = NO;
	for (SCIContact *savedContact in self.compositeContacts) {
		if ([savedContact hasSamePhoneNumbersAsContact:contact]) {
			duplicate = YES;
			break;
		}
	}
	
	return duplicate;
}

- (void)invalidateCachedNumbers
{
	NSArray *contacts = [self compositeContacts];
	NSArray *otherFixedContacts = [NSArray arrayWithObjects:[SCIContact fixedContactWithName:@"SVRS Español"
																					   phone:[SCIContactManager SVRSEspañolPhone]
																			 photoIdentifier:PhotoManagerRapidoVRSImageIdentifier()
																			   relayLanguage:SCIRelayLanguageSpanish],
								   							[SCIContact fixedContactWithName:@"SCIS VRI"
																					   phone:[SCIContactManager SorensonVRIPhone]
																			 photoIdentifier:PhotoManagerVRIImageIdentifier()
																			   relayLanguage:SCIRelayLanguageEnglish],nil];
	contacts = [contacts arrayByAddingObjectsFromArray:otherFixedContacts];
	contacts = [contacts arrayByAddingObjectsFromArray:[self compositeLDAPDirectoryContacts]];
	
	NSMutableArray *keys = [NSMutableArray arrayWithCapacity:3 * contacts.count];
	NSMutableArray *values = [NSMutableArray arrayWithCapacity:3 * contacts.count];

	for (SCIContact *contact in contacts) {
		NSArray *numbers = [contact phones];
		for (int index = 0; index < numbers.count; ++index) {
			SCIContactPhone phone = [contact phoneAtIndex:index];
			NSString *number = [[numbers objectAtIndex:index] normalizedDialString];

			ContactPhone *pair = [[ContactPhone alloc] initWithContact:contact
																 phone:phone];
			[keys addObject:[number normalizedDialString]];
			[values addObject:pair];
		}
	}

	mContactsAndPhonesForNumbers = [NSDictionary dictionaryWithObjects:values
															   forKeys:keys];
}

- (void)importContactsForNumber:(NSString*)phoneNumber password:(NSString*)password
{
	IstiContactManager *manager = [self istiContactManager];
	if (phoneNumber.length == 10 && [phoneNumber characterAtIndex:0] != '1')
		phoneNumber = [@"1" stringByAppendingString:phoneNumber];
	manager->ImportContacts([phoneNumber UTF8String], [password UTF8String]);
//	manager->ImportContacts("13113113112", "1234");
}

#pragma mark - Notifications

- (void)notifyAuthenticatedDidChange:(NSNotification *)note // SCINotificationAuthenticatedDidChange
{
	BOOL authenticated = [SCIVideophoneEngine sharedEngine].isAuthenticated;
	if (!authenticated && mWasAuthenticated) {
		self.contacts = nil;
		
		// Purge contacts when logging out.
		IstiContactManager *manager = [self istiContactManager];
		manager->purgeContacts();
	}
	mWasAuthenticated = authenticated;
}

- (void)notifyInterfaceModeChanged:(NSNotification *)note // SCINotificationInterfaceModeChanged
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		[self invalidateCachedNumbers];
	}
}

@end
