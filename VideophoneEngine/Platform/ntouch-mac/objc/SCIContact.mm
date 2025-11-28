//
//  SCIContact.m
//  ntouchMac
//
//  Created by Adam Preble on 2/8/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIContactCpp.h"
#import "IstiContactManager.h"
#import "IstiContact.h"
#import "ContactListItem.h"
#import "SCIVideophoneEngineCpp.h"
#import "NSString+SCIAdditions.h"
#import "SCIItemIdCpp.h"
#import "SCIContactManager.h"
#import "CstiLDAPDirectoryContactManager.h"

static NSString *nameForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *companyNameForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *workPhoneForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *homePhoneForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *cellPhoneForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *phoneForIstiContactOfType(const IstiContactSharedPtr &istiContact, CstiContactListItem::EPhoneNumberType type);
static NSString *relayLanguageForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *photoIdentifierForIstiContact(const IstiContactSharedPtr &istiContact);
static NSString *photoTimestampForIstiContact(const IstiContactSharedPtr &istiContact);
static BOOL vcoForIstiContact(const IstiContactSharedPtr &istiContact);
static int ringPatternForIstiContact(const IstiContactSharedPtr &istiContact);

static void setNameForIstiContact(const IstiContactSharedPtr &istiContact, NSString *name);
static void setCompanyNameForIstiContact(const IstiContactSharedPtr &istiContact, NSString *companyName);
static void setPhoneForIstiContactOfType(const IstiContactSharedPtr &istiContact, CstiContactListItem::EPhoneNumberType type, NSString *phone);
static void setRelayLanguageForIstiContact(const IstiContactSharedPtr &istiContact, NSString *relayLanguage);
static void setPhotoIdentifierForIstiContact(const IstiContactSharedPtr &istiContact, NSString *photoIdentifier);
static void setPhotoTimestampForIstiContact(const IstiContactSharedPtr &istiContact, NSString *photoTimestamp);
static void setVCOForIstiContact(const IstiContactSharedPtr &istiContact, BOOL vco);
static void setRingPatternForIstiContact(const IstiContactSharedPtr &istiContact, int ringPattern);
static void	setFavoritesSaveFlags(const IstiContactSharedPtr &istiContact, BOOL home, BOOL work, BOOL mobile);

static CstiItemId idForIstiContact(const IstiContactSharedPtr &istiContact);
static IstiContactSharedPtr istiContactForID(CstiItemId ID);
static CstiFavoriteSharedPtr CstiFavoriteForID(CstiItemId ID);

NSString * const SCIRelayLanguageEnglish = @"English";
NSString * const SCIRelayLanguageSpanish = @"Spanish";

@interface SCIContact () {
	NSString *mSearchString;
	
	NSString *mName;
	NSString *mCompanyName;
	NSString *mWorkPhone;
	NSString *mHomePhone;
	NSString *mCellPhone;
	NSString *mRelayLanguage;
	NSString *mPhotoIdentifier;
	NSString *mPhotoTimestamp;
	BOOL mVco;
	int mRingPattern;
	BOOL mHomeIsFavorite;
	BOOL mWorkIsFavorite;
	BOOL mCellIsFavorite;
	BOOL mIsFavorite;
	BOOL mIsFixed;
	NSNumber *mFavoriteOrder;
	BOOL mIsLDAPContact;
	
	CstiItemId mIstiContactID;
}
@property (nonatomic, assign, readwrite) CstiItemId istiContactID;
@property (nonatomic, assign, readwrite) BOOL isFavorite;
@property (nonatomic, assign, readwrite) BOOL isFixed;
@property (nonatomic, assign, readwrite) BOOL isLDAPContact;
- (void)invalidateSearchString;
- (NSString *)keyForPhoneType:(SCIContactPhone)type;
- (void)applyFieldsToIstiContact:(const IstiContactSharedPtr &)istiContact;
@end

@implementation SCIContact

@synthesize name = mName;
@synthesize companyName = mCompanyName;
@synthesize workPhone = mWorkPhone;
@synthesize homePhone = mHomePhone;
@synthesize cellPhone = mCellPhone;
@synthesize relayLanguage = mRelayLanguage;
@synthesize photoIdentifier = mPhotoIdentifier;
@synthesize photoTimestamp = mPhotoTimestamp;
@synthesize vco = mVco;
@synthesize ringPattern = mRingPattern;
@synthesize homeIsFavorite = mHomeIsFavorite;
@synthesize workIsFavorite = mWorkIsFavorite;
@synthesize cellIsFavorite = mCellIsFavorite;
@synthesize isFavorite = mIsFavorite;
@synthesize isFixed = mIsFixed;
@synthesize istiContactID = mIstiContactID;
@synthesize favoriteOrder = mFavoriteOrder;
@synthesize isLDAPContact = mIsLDAPContact;


- (id)init
{
	return [self initWithIstiContact:NULL];
}

- (id)initWithIstiContact:(const IstiContactSharedPtr &)contact
{
	if ((self = [super init]))
	{
		if (contact) {
			mName = nameForIstiContact(contact);
			mCompanyName = companyNameForIstiContact(contact);
			mWorkPhone = workPhoneForIstiContact(contact);
			mHomePhone = homePhoneForIstiContact(contact);
			mCellPhone = cellPhoneForIstiContact(contact);
			mRelayLanguage = relayLanguageForIstiContact(contact);
			mPhotoIdentifier = photoIdentifierForIstiContact(contact);
			mPhotoTimestamp = photoTimestampForIstiContact(contact);
			mVco = vcoForIstiContact(contact);
			mRingPattern = ringPatternForIstiContact(contact);
			mIstiContactID = idForIstiContact(contact);
			mHomeIsFavorite = [self isFavoriteForPhoneType:SCIContactPhoneHome contact:contact];
			mWorkIsFavorite = [self isFavoriteForPhoneType:SCIContactPhoneWork contact:contact];
			mCellIsFavorite = [self isFavoriteForPhoneType:SCIContactPhoneCell contact:contact];
		} else {
			mName = @"";
			mCompanyName = @"";
			mPhotoIdentifier = @"00000000-0000-0000-0000-000000000001";
			mPhotoTimestamp = @"000000";
		}
	}
	return self;
}

#ifdef stiLDAP_CONTACT_LIST
- (id)initWithIstiContactFromLDAPDirectory:(const IstiContactSharedPtr &)contact
{
	if ((self = [super init]))
	{
		if (contact) {
			mName = nameForIstiContact(contact);
//			mCompanyName = companyNameForIstiContact(contact);
			mWorkPhone = workPhoneForIstiContact(contact);
			mHomePhone = homePhoneForIstiContact(contact);
			mCellPhone = cellPhoneForIstiContact(contact);
			mRelayLanguage = relayLanguageForIstiContact(contact);
			mPhotoIdentifier = photoIdentifierForIstiContact(contact);
			mPhotoTimestamp = photoTimestampForIstiContact(contact);
			mVco = vcoForIstiContact(contact);
			mRingPattern = ringPatternForIstiContact(contact);
			mIstiContactID = idForIstiContact(contact);
			mHomeIsFavorite = [self isLDAPFavoriteForPhoneType:SCIContactPhoneHome contact:contact];
			mWorkIsFavorite = [self isLDAPFavoriteForPhoneType:SCIContactPhoneWork contact:contact];
			mCellIsFavorite = [self isLDAPFavoriteForPhoneType:SCIContactPhoneCell contact:contact];
			mIsLDAPContact = YES;
			
		} else {
			mName = @"";
			mCompanyName = @"";
			mPhotoIdentifier = @"00000000-0000-0000-0000-000000000001";
			mPhotoTimestamp = @"000000";
		}
	}
	return self;
}
#endif

- (id)initWithCstiFavorite:(const CstiFavoriteSharedPtr &)favorite
{
	if ((self = [super init]))
	{
		if (favorite) {
			IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
			IstiContactManager *manager = engine->ContactManagerGet();
			IstiContactSharedPtr cppContact = manager->ContactByFavoriteGet(favorite);
#ifdef stiLDAP_CONTACT_LIST
			if (cppContact == NULL) {
				IstiLDAPDirectoryContactManager *LDAPManager = engine->LDAPDirectoryContactManagerGet();
				cppContact = LDAPManager->ContactByFavoriteGet(favorite);
				
			}
#endif
			if (cppContact == NULL) {
				mName = @"Unavailable";
				CstiItemId faveNumberId = favorite->PhoneNumberIdGet();

				mIstiContactID = faveNumberId;//idForIstiContact(cppContact);

				mIsFavorite = YES;

			} else {
				std::string favoriteNumber;
				CstiItemId faveNumberId = favorite->PhoneNumberIdGet();
				
				CstiContactListItem::EPhoneNumberType favoriteNumberType = cppContact->PhoneNumberPublicIdFind(faveNumberId);
				cppContact->DialStringGet(favoriteNumberType, &favoriteNumber);
				
				mName = nameForIstiContact(cppContact);
				mCompanyName = companyNameForIstiContact(cppContact);
				
				mHomeIsFavorite = NO;
				mWorkIsFavorite = NO;
				mCellIsFavorite = NO;
				
				
				switch (favoriteNumberType) {
					case CstiContactListItem::ePHONE_HOME:
						mHomePhone = [NSString stringWithUTF8String:favoriteNumber.c_str()];
						mHomeIsFavorite = YES;
						break;
					case CstiContactListItem::ePHONE_WORK:
						mWorkPhone = [NSString stringWithUTF8String:favoriteNumber.c_str()];
						mWorkIsFavorite = YES;
						break;
					case CstiContactListItem::ePHONE_CELL:
						mCellPhone = [NSString stringWithUTF8String:favoriteNumber.c_str()];
						mCellIsFavorite = YES;
					default:
						break;
				}
				
				mRelayLanguage = relayLanguageForIstiContact(cppContact);
				mPhotoIdentifier = photoIdentifierForIstiContact(cppContact);
				mPhotoTimestamp = photoTimestampForIstiContact(cppContact);
				mVco = vcoForIstiContact(cppContact);
				mRingPattern = ringPatternForIstiContact(cppContact);
				mIstiContactID = faveNumberId;//idForIstiContact(cppContact);
				
				mIsFavorite = YES;
				
				//			mHomeIsFavorite = ((ContactListItem *)cppContact)->HomeFavoriteOnSaveGet();//[self isFavoriteForPhoneType:SCIContactPhoneHome];
				//			mWorkIsFavorite = ((ContactListItem *)cppContact)->WorkFavoriteOnSaveGet();//[self isFavoriteForPhoneType:SCIContactPhoneWork];
				//			mCellIsFavorite = ((ContactListItem *)cppContact)->MobileFavoriteOnSaveGet();//[self isFavoriteForPhoneType:SCIContactPhoneCell];
			}
		} else {
			mPhotoIdentifier = @"00000000-0000-0000-0000-000000000001";
			mPhotoTimestamp = @"000000";
		}
	}
	return self;
}

- (id)copyWithZone:(NSZone *)zone
{
	__typeof__(self) copy = [[self.class alloc] init];
	copy.name = self.name;
	copy.companyName = self.companyName;
	copy.workPhone = self.workPhone;
	copy.homePhone = self.homePhone;
	copy.cellPhone = self.cellPhone;
	copy.relayLanguage = self.relayLanguage;
	copy.photoIdentifier = self.photoIdentifier;
	copy.photoTimestamp = self.photoTimestamp;
	copy.vco = self.vco;
	copy.ringPattern = self.ringPattern;
	copy.istiContactID = self.istiContactID;
	copy.homeIsFavorite = self.homeIsFavorite;
	copy.workIsFavorite = self.workIsFavorite;
	copy.cellIsFavorite = self.cellIsFavorite;
    copy.isFavorite = self.isFavorite;
    copy.isFixed = self.isFixed;
    copy.isLDAPContact = self.isLDAPContact;

	return copy;
}

- (NSUInteger)hash
{
	return self.contactId.hash;
}

+ (SCIContact *)fixedContactWithName:(NSString *)name phone:(NSString *)phone photoIdentifier:(NSString *)photoIdentifier relayLanguage:(NSString *)relayLanguage
{
	SCIContact *contact = [[SCIContact alloc] init];
	contact->mIsFixed = YES;
	contact.name = name;
	contact.workPhone = phone;
	contact.photoIdentifier = photoIdentifier;
	contact.relayLanguage = relayLanguage;
	return contact;
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p %@ %@ Home:%@ Work:%@ Cell:%@>", NSStringFromClass([self class]), self, self.name, self.companyName, self.homePhone, self.workPhone, self.cellPhone];
}

- (IstiContactSharedPtr)istiContact
{
	return istiContactForID(mIstiContactID);
}

- (IstiContactSharedPtr)generateIstiContact
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();
	IstiContactSharedPtr res = manager->ContactCreate();
	mIstiContactID = idForIstiContact(res);
	[self applyFieldsToIstiContact:res];
	return res;
}

- (void)applyFieldsToIstiContact:(const IstiContactSharedPtr &)istiContact
{
	setNameForIstiContact(istiContact, mName);
	setCompanyNameForIstiContact(istiContact, mCompanyName);
	setPhoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_WORK, mWorkPhone);
	setPhoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_HOME, mHomePhone);
	setPhoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_CELL, mCellPhone);
	setRelayLanguageForIstiContact(istiContact, mRelayLanguage);
	setPhotoIdentifierForIstiContact(istiContact, mPhotoIdentifier);
	setPhotoTimestampForIstiContact(istiContact, mPhotoTimestamp);
	setVCOForIstiContact(istiContact, mVco);
	setRingPatternForIstiContact(istiContact, mRingPattern);
	setFavoritesSaveFlags(istiContact, self.homeIsFavorite, self.workIsFavorite, self.cellIsFavorite);
}

- (CstiFavoriteSharedPtr)cstiFavorite
{
	return CstiFavoriteForID(mIstiContactID);
}

#pragma mark - Accessors

- (NSString *)nameOrCompanyName
{
	if ([[SCIContactManager sharedManager] companyNameEnabled]) {
		if (self.name.length > 0)
			return self.name;
		else
			return self.companyName ? self.companyName : @"";
	} else {
		return self.name ? self.name : @"";
	}
}

- (NSString *)nameAndOrCompanyName
{
    if ([[SCIContactManager sharedManager] companyNameEnabled]) {
        if (self.name.length > 0 && self.companyName.length > 0)
            return [NSString stringWithFormat:@"%@ | %@", self.name, self.companyName];
        else if (self.name.length > 0)
            return self.name;
        else
			return self.companyName ? self.companyName : @"";
    } else {
		return self.name ? self.name : @"";
    }
}

- (void)setName:(NSString *)name
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setNameForIstiContact(istiContact, name);
	mName = name;
	[self invalidateSearchString];
}

- (void)setCompanyName:(NSString *)companyName
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setCompanyNameForIstiContact(istiContact, companyName);
	mCompanyName = companyName;
	[self invalidateSearchString];
}

- (void)setDialString:(NSString *)str forType:(CstiContactListItem::EPhoneNumberType)type
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setPhoneForIstiContactOfType(istiContact, type, str);
	[self invalidateSearchString];
}

- (void)setWorkPhone:(NSString *)workPhone
{
	[self setDialString:workPhone forType:CstiContactListItem::ePHONE_WORK];
	mWorkPhone = workPhone;
}

- (void)setHomePhone:(NSString *)homePhone
{
	[self setDialString:homePhone forType:CstiContactListItem::ePHONE_HOME];
	mHomePhone = homePhone;
}

- (void)setCellPhone:(NSString *)cellPhone
{
	[self setDialString:cellPhone forType:CstiContactListItem::ePHONE_CELL];
	mCellPhone = cellPhone;
}

- (SCIItemId *)contactId
{
	return [SCIItemId itemIdWithCstiItemId:mIstiContactID];
}

- (NSString *)keyForPhoneType:(SCIContactPhone)type
{
    NSString *key;
    switch (type) {
        case SCIContactPhoneWork: {
            key = @"workPhone";
        } break;
        case SCIContactPhoneHome: {
            key = @"homePhone";
        } break;
        case SCIContactPhoneCell: {
            key = @"cellPhone";
        } break;
		case SCIContactPhoneNone:
			break;
    }
    return key;
}

- (NSArray *)phonesOfTypes:(NSArray *)types;
{
	NSMutableArray *mutablePhones = [NSMutableArray array];
	
	for (NSNumber *typeNumber in types) {
		SCIContactPhone type = (SCIContactPhone)[typeNumber intValue];
		NSString *phone = [self phoneOfType:type];
		if (phone &&
			phone.length > 0) {
			[mutablePhones addObject:phone];
		}
	}
	
	return [mutablePhones copy];
}

- (NSArray *)phones
{
	NSArray *types = @[@(SCIContactPhoneHome), @(SCIContactPhoneWork), @(SCIContactPhoneCell)];
	return [self phonesOfTypes:types];
}

- (NSUInteger)phonesCount
{
    return ((([self homePhone]) && [[self homePhone] length] > 0) ? 1 : 0) + 
	((([self workPhone]) && [[self workPhone] length] > 0) ? 1 : 0) + 
	((([self cellPhone]) && [[self cellPhone] length] > 0) ? 1 : 0);
}

- (SCIContactPhone)phoneAtIndex:(NSUInteger)index
{
    SCIContactPhone phone;
	//perform index mutation to skip over the first phone(s) if absent
    if ((![self homePhone] || [[self homePhone] length] == 0)/*&& index >= 0*/) index++;
    if ((![self workPhone] || [[self workPhone] length] == 0) && index >= 1) index++;
	//the now mutated index is equal to 0, 1, or 2 according to the input and which phone numbers the phone has
	//the SCIContactPhone enum is {SCIContactPhoneHome = 0, SCIContactPhoneWork = 1, SCIContactPhoneCell = 2}
	//the mutated index value is now equal to the appropriate enum value.
	phone = (SCIContactPhone)index;
    return phone;
}

- (NSString *)phoneOfType:(SCIContactPhone)type
{
    return [self valueForKey:[self keyForPhoneType:type]];
}

- (void)setPhone:(NSString *)phone ofType:(SCIContactPhone)type
{
    [self setValue:phone forKey:[self keyForPhoneType:type]];
}

- (void)setRelayLanguage:(NSString *)relayLanguage
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setRelayLanguageForIstiContact(istiContact, relayLanguage);
	mRelayLanguage = relayLanguage;
}

- (void)setPhotoIdentifier:(NSString *)photoIdentifier
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setPhotoIdentifierForIstiContact(istiContact, photoIdentifier);
	mPhotoIdentifier = photoIdentifier;
}

- (void)setPhotoTimestamp:(NSString *)photoTimestamp
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setPhotoTimestampForIstiContact(istiContact, photoTimestamp);
	mPhotoTimestamp = photoTimestamp;
}

- (void)setVco:(BOOL)vco
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setVCOForIstiContact(istiContact, vco);
	mVco = vco;
}

-(void)setRingPattern:(int)ringPattern
{
	IstiContactSharedPtr istiContact = self.istiContact;
	if (istiContact) setRingPatternForIstiContact(istiContact, ringPattern);
	mRingPattern = ringPattern;
}

- (NSComparisonResult)compare:(SCIContact *)other
{
	if (self.isFixed && other.isFixed)
	{
		// Both are fixed. Compare names.
		return [self.name compare:other.name];
	}
	else if (!self.isFixed && !other.isFixed)
	{
		if (self.isFavorite && other.isFavorite) {
			return [self.favoriteOrder compare:other.favoriteOrder];
		} else {
			// Neither are fixed. Compare names with alpha coming before numeric/symbols.
			return [self.nameOrCompanyName sci_compareAlphaFirst:other.nameOrCompanyName];
		}
	}
	else if (self.isFixed)
	{
		return NSOrderedAscending;
	}
	else // other is fixed
	{
		return NSOrderedDescending;
	}
}

- (BOOL)isEqual:(id)object {
	if (self == object) {
		return YES;
	}
	
	if (![object isKindOfClass:[SCIContact class]]) {
		return NO;
	}
	
	return [self compare:(SCIContact *)object] == NSOrderedSame;
}

- (BOOL)hasSamePhoneNumbersAsContact:(SCIContact *)other
{
	// Check that both contacts have the same set of phone numbers.
	NSArray *selfPhones = [self.phones mutableCopy];
	NSMutableArray *otherPhones = [other.phones mutableCopy];
	
	for (NSString *selfPhone in selfPhones)
	{
		BOOL found = NO;
		for (NSString *otherPhone in [otherPhones copy])
		{
			if (SCIContactPhonesAreEqual(selfPhone, otherPhone))
			{
				[otherPhones removeObject:otherPhone];
				found = YES;
				break;
			}
		}
		if (!found)
			return NO;
	}
	return otherPhones.count == 0;
}

- (BOOL)isResolved
{
	return (istiContactForID(mIstiContactID) != NULL);
}

- (BOOL)isResolvedFavorite
{
	return (CstiFavoriteForID(mIstiContactID) != NULL);
}

- (NSString *)searchString
{
	if (!mSearchString)
	{
		NSMutableString *str =[[NSMutableString alloc] init];
		[str appendString:self.name ? self.name.normalizedSearchString : @""];
		if ([[SCIContactManager sharedManager] companyNameEnabled]) {
			if (self.companyName.length > 0) {
				[str appendString:@"|"];
				[str appendString:[self.companyName normalizedSearchString]];
			}
		}
		for (NSString *key in [NSArray arrayWithObjects:@"homePhone", @"cellPhone", @"workPhone", nil])
		{
			NSString *value = [self valueForKey:key];
			if (value.length > 0)
			{
				[str appendString:@"|"];
				[str appendString:value];
			}
		}
		mSearchString = str;
	}
	return mSearchString;
}

- (NSNumber *)favoriteOrder
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();

	// Find the favorite
	CstiFavoriteSharedPtr poFavorite = NULL;
	poFavorite = manager->FavoriteByPhoneNumberIdGet(mIstiContactID);
	
	if (poFavorite != NULL) {
		mFavoriteOrder = [NSNumber numberWithInt:poFavorite->PositionGet()];
	} else {
		mFavoriteOrder = [NSNumber numberWithLong:NSNotFound];
	}
	return  mFavoriteOrder;
}

- (void)setFavoriteOrder:(NSNumber *)favoriteOrder
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();
	
	// Find the favorite
	CstiFavoriteSharedPtr poFavorite = NULL;
	poFavorite = manager->FavoriteByPhoneNumberIdGet(mIstiContactID);
	
	poFavorite->PositionSet(favoriteOrder.intValue);
	mFavoriteOrder = favoriteOrder;
}

- (void)invalidateSearchString
{
	mSearchString = nil;
}

- (BOOL)isFavoriteForPhoneType:(SCIContactPhone)phoneType {
	
	if (self.isLDAPContact)
		return [self isLDAPFavoriteForPhoneType:phoneType];

	if (![self phoneOfType:phoneType])
		return NO;

	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();
	IstiContactSharedPtr istiContact = NULL;
	manager->ContactByIDGet(mIstiContactID, &istiContact);
	
	return [self isFavoriteForPhoneType:phoneType contact:istiContact];
}

- (BOOL)isFavoriteForPhoneType:(SCIContactPhone)phoneType contact:(const IstiContactSharedPtr &)istiContact {
	if (self.isLDAPContact)
		return [self isLDAPFavoriteForPhoneType:phoneType contact:istiContact];
	if (self.isFixed || ![self phoneOfType:phoneType] || !istiContact) {
		return NO;
	}

	// Convert SCIContactPhone type to EPhoneNumberType
	CstiContactListItem::EPhoneNumberType cstiType = CstiContactListItem::ePHONE_HOME;
	switch (phoneType) {
		case SCIContactPhoneHome: cstiType = CstiContactListItem::ePHONE_HOME; break;
		case SCIContactPhoneWork: cstiType = CstiContactListItem::ePHONE_WORK; break;
		case SCIContactPhoneCell: cstiType = CstiContactListItem::ePHONE_CELL; break;
		default: cstiType = CstiContactListItem::ePHONE_HOME; break;
	}

	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();

	const CstiItemId itemId = istiContact->PhoneNumberPublicIdGet(cstiType);
	BOOL result = manager->IsFavorite(itemId);
	return result;
}

#ifdef stiLDAP_CONTACT_LIST
- (BOOL)isLDAPFavoriteForPhoneType:(SCIContactPhone)phoneType {
	if (self.isFixed){
		return NO;
	}
	if (![self phoneOfType:phoneType])
		return NO;
	
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiLDAPDirectoryContactManager *LDAPManager = engine->LDAPDirectoryContactManagerGet();
	IstiContactSharedPtr istiContact = NULL;
	LDAPManager->ContactByIDGet(mIstiContactID, &istiContact);

	return [self isLDAPFavoriteForPhoneType:phoneType contact:istiContact];
}
#endif

- (BOOL)isLDAPFavoriteForPhoneType:(SCIContactPhone)phoneType contact:(const IstiContactSharedPtr &)istiContact {

	if (![self phoneOfType:phoneType])
		return NO;

	// Convert SCIContactPhone type to EPhoneNumberType
	CstiContactListItem::EPhoneNumberType cstiType = CstiContactListItem::ePHONE_HOME;
	switch (phoneType) {
		case SCIContactPhoneHome: cstiType = CstiContactListItem::ePHONE_HOME; break;
		case SCIContactPhoneWork: cstiType = CstiContactListItem::ePHONE_WORK; break;
		case SCIContactPhoneCell: cstiType = CstiContactListItem::ePHONE_CELL; break;
		default: cstiType = CstiContactListItem::ePHONE_HOME; break;
	}

	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();

	const CstiItemId itemId = istiContact->PhoneNumberPublicIdGet(cstiType);
	BOOL result = manager->IsFavorite(itemId);
	return result;
}


@end


@implementation NSString (SCIContactAdditions)

- (NSString *)normalizedSearchString
{
	// Strip out whitespace characters and all ASCII characters with index > 127.
	static NSCharacterSet *stripped = nil;
	if (!stripped)
	{
		NSMutableCharacterSet *cs = [NSMutableCharacterSet characterSetWithRange:NSMakeRange(127, 128)];
		[cs formUnionWithCharacterSet:[NSCharacterSet whitespaceCharacterSet]];
		[cs formUnionWithCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@"-()"]];
		stripped = cs;
	}
	return [[[self componentsSeparatedByCharactersInSet:stripped] componentsJoinedByString:@""] lowercaseString];
}

- (NSString *)normalizedDialString {
	if (self.length == 10) {
		return [@"1" stringByAppendingString:self];
	}
	else {
		return self;
	}
}

+ (NSString *)stringFromContactPhoneType:(SCIContactPhone)type
{
	NSString *res = nil;
	
	switch (type) {
		case SCIContactPhoneHome: {
			res = @"Home";
			break;
		}
		case SCIContactPhoneCell: {
			res = @"Cell";
			break;
		}
		case SCIContactPhoneWork: {
			res = @"Work";
			break;
		}
		default: {
			res = @"Unknown";
			break;
		}
	}
	
	return res;
}

+ (NSString *)lowercaseStringFromContactPhoneType:(SCIContactPhone)type
{
	return [[self stringFromContactPhoneType:type] lowercaseString];
}

@end

#pragma mark - global functions
BOOL SCIContactPhonesAreEqual(NSString *firstContactPhone, NSString *secondContactPhone)
{
	if (firstContactPhone == secondContactPhone)
		return YES;
	if (firstContactPhone == nil || secondContactPhone == nil)
		return NO;
	
	return [[firstContactPhone normalizedDialString]
			isEqualToString:[secondContactPhone normalizedDialString]];
}

#pragma mark - local funcations
static NSString *nameForIstiContact(const IstiContactSharedPtr &istiContact)
{
	std::string name = istiContact->NameGet();
	if (!name.empty()) {
		NSString *output = [NSString stringWithUTF8String:name.c_str()];
		return output;
	}
	else {
		return nil;
	}
}

static NSString *companyNameForIstiContact(const IstiContactSharedPtr &istiContact)
{
	std::string companyName = istiContact->CompanyNameGet();
	if (!companyName.empty()) {
		NSString *output = [NSString stringWithUTF8String:companyName.c_str()];
		return output;
	}
	else {
		return nil;
	}
}

static NSString *workPhoneForIstiContact(const IstiContactSharedPtr &istiContact)
{
	return phoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_WORK);
}

static NSString *homePhoneForIstiContact(const IstiContactSharedPtr &istiContact)
{
	return phoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_HOME);
}

static NSString *cellPhoneForIstiContact(const IstiContactSharedPtr &istiContact)
{
	return phoneForIstiContactOfType(istiContact, CstiContactListItem::ePHONE_CELL);
}

static NSString *phoneForIstiContactOfType(const IstiContactSharedPtr &istiContact, CstiContactListItem::EPhoneNumberType type)
{
	std::string cstr;
	if (istiContact->DialStringGet(type, &cstr))
		return nil;
	NSString *str;
	if (!cstr.empty())
		str = [NSString stringWithUTF8String:cstr.c_str()];
	return str;
}

static NSString *relayLanguageForIstiContact(const IstiContactSharedPtr &istiContact)
{
	std::string cstr;
	if (istiContact->LanguageGet(&cstr))
		return nil;
	NSString *str = [NSString stringWithUTF8String:cstr.c_str()];
	return str;
}

static NSString *photoIdentifierForIstiContact(const IstiContactSharedPtr &istiContact)
{
	std::string cstr;
	if (istiContact->PhotoGet(&cstr))
		return nil;
	NSString *str = [NSString stringWithUTF8String:cstr.c_str()];
	return str;
}

static NSString *photoTimestampForIstiContact(const IstiContactSharedPtr &istiContact)
{
	std::string cstr;
	if (istiContact->PhotoTimestampGet(&cstr))
		return nil;
	NSString *str = [NSString stringWithUTF8String:cstr.c_str()];
	return str;
}

static BOOL vcoForIstiContact(const IstiContactSharedPtr &istiContact)
{
	bool vco;
	istiContact->VCOGet(&vco);
	return (vco == true);
}

static int ringPatternForIstiContact(const IstiContactSharedPtr &istiContact)
{
	int pattern;
	istiContact->RingPatternGet(&pattern);
	return pattern;
}

static void setNameForIstiContact(const IstiContactSharedPtr &istiContact, NSString *name)
{
	istiContact->NameSet(std::string([name UTF8String], [name lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
}

static void setCompanyNameForIstiContact(const IstiContactSharedPtr &istiContact, NSString *companyName)
{
	istiContact->CompanyNameSet(std::string([companyName UTF8String], [companyName lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
}

static void setPhoneForIstiContactOfType(const IstiContactSharedPtr &istiContact, CstiContactListItem::EPhoneNumberType type, NSString *phone)
{
	std::string dialString;
	istiContact->DialStringGet(type, &dialString);
	
	// If phone is not empty, set the number. If not, then only set the number with the empty string if the existing number for the phone type is not empty (deleting a number from a contact).
	if (phone.length || !dialString.empty()) {
		istiContact->PhoneNumberSet(type, std::string([phone UTF8String], [phone lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
	}
}

static void setRelayLanguageForIstiContact(const IstiContactSharedPtr &istiContact, NSString *relayLanguage)
{
	istiContact->LanguageSet(std::string([relayLanguage UTF8String], [relayLanguage lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
}

static void setPhotoIdentifierForIstiContact(const IstiContactSharedPtr &istiContact, NSString *photoIdentifier)
{
	istiContact->PhotoSet(std::string([photoIdentifier UTF8String], [photoIdentifier lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
}

static void setPhotoTimestampForIstiContact(const IstiContactSharedPtr &istiContact, NSString *photoTimestamp)
{
	istiContact->PhotoTimestampSet(std::string([photoTimestamp UTF8String], [photoTimestamp lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
}

static void setVCOForIstiContact(const IstiContactSharedPtr &istiContact, BOOL vco)
{
	bool val = (vco == YES);
	istiContact->VCOSet(val);
}

static void setRingPatternForIstiContact(const IstiContactSharedPtr &istiContact, int ringPattern)
{
	istiContact->RingPatternSet(ringPattern);
}

static void	setFavoritesSaveFlags(const IstiContactSharedPtr &istiContact, BOOL home, BOOL work, BOOL mobile)
{
	istiContact->HomeFavoriteOnSaveSet(home);
	istiContact->WorkFavoriteOnSaveSet(work);
	istiContact->MobileFavoriteOnSaveSet(mobile);
}

static CstiItemId idForIstiContact(const IstiContactSharedPtr &istiContact)
{
	return istiContact->ItemIdGet();
}

static IstiContactSharedPtr istiContactForID(CstiItemId ID)
{
	IstiContactSharedPtr res = NULL;
	
	if (ID.IsValid()) {
		IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
		IstiContactManager *manager = engine->ContactManagerGet();
		manager->ContactByIDGet(ID, &res);
		
#ifdef stiLDAP_CONTACT_LIST
		if (res == NULL) {
			IstiLDAPDirectoryContactManager *LDAPManager = engine->LDAPDirectoryContactManagerGet();
			LDAPManager->ContactByIDGet(ID, &res);
		}
#endif
	}
	return res;
}

static CstiFavoriteSharedPtr CstiFavoriteForID(CstiItemId ID)
{
	CstiFavoriteSharedPtr res = NULL;
	
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiContactManager *manager = engine->ContactManagerGet();
	res = manager->FavoriteByPhoneNumberIdGet(ID);

	
	return res;
}

NSString * const SCIKeyContactPhotoIdentifier = @"photoIdentifier";
