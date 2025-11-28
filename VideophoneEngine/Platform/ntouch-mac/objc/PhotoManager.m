//
//  PhotoManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/18/12.
//  Copyright (c) 2012-2014 Sorenson Communications. All rights reserved.
//

#import "PhotoManager.h"
#import "NSRunLoop+PerformBlock.h"
#import "SCIContact.h"
#import "SCICallListItem.h"
#import "SCIMessageInfo.h"
#import "SCICall.h"
#import "SCIContactManager.h"
#import "SCIItemId.h"
#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#if TARGET_OS_IPHONE
#import "ContactUtilities.h"
#import "UIImage+ProportionalFill.h"
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
#import "NSImage+BNRRepresentationAdditions.h"
#import "ABAddressBook+SCIContactSearch.h"
#import "NSImage+MGCropExtensions.h"
#endif
	
NSUInteger const PhotoManagerAvatarLowIndex = 10;
NSUInteger const PhotoManagerAvatarHighIndex = 65;
NSUInteger const PhotoManagerFacebookIndex = 1000;
NSUInteger const PhotoManagerCustomImageIndex = 1001;
NSUInteger const PhotoManagerSorensonVRSImageIndex = 1002;
NSUInteger const PhotoManagerRapidoVRSImageIndex = 1003;
NSUInteger const PhotoManagerCIRImageIndex = 1004;
NSUInteger const PhotoManagerTechSupportImageIndex = 1005;
NSUInteger const PhotoManagerVRIImageIndex = 1006;

static SCIImage *PhotoManagerPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut);
static SCIImage *PhotoManagerCachedPhotoForContact(SCIContact *contact);
static PhotoManagerFetchCompletionBlock PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(PhotoManagerFetchOrPlaceholderCompletionBlock block);

static NSString *PhotoManagerSorensonPhotoFileNameForContact(SCIContact *contact);
static NSString *PhotoManagerSorensonPhotoFileNameForIdentifierAndTimestamp(NSString *identifier, NSString *timestamp);
static NSString *PhotoManagerCustomImageFileNameForContact(SCIContact *contact);

static BOOL PhotoManagerStorePhotoWithFileName(SCIImage *photo, NSString *identifier, NSError * __autoreleasing *errorOut);
static void PhotoManagerDeletePhotoWithFileName(NSString *fileName, NSError * __autoreleasing *errorOut);

static BOOL PhotoManagerStoreCustomImageForContact(SCIImage *photo, SCIContact *contact, NSError * __autoreleasing *errorOut);
static BOOL PhotoManagerUpdateCustomImageFileNameForContact(SCIContact *contact, NSString *originalFileName, NSError * __autoreleasing *errorOut);

@interface PhotoManager () <NSCacheDelegate>

//Photo Fetching Block Maintenance
@property (nonatomic) NSObject *latestTagLock;
@property (nonatomic, readonly) NSUInteger latestTag;
@property (nonatomic) NSMutableDictionary *blocksForTags;
- (BOOL)hasBlockForTag:(NSUInteger)tag;
- (void)setBlock:(PhotoManagerFetchCompletionBlock)block forTag:(NSUInteger)tag;
- (PhotoManagerFetchCompletionBlock)blockForTag:(NSUInteger)tag;
- (void)removeBlockForTag:(NSUInteger)tag;

//Block Performance
@property (nonatomic) dispatch_queue_t photoLoadingQueue;
- (void)performBlockOnPhotoLoadingQueue:(void (^)(void))block;

@property (nonatomic) NSCache<NSString *, SCIImage *> *imageCache;

@end

@implementation PhotoManager

@synthesize latestTag = mLatestTag;

#pragma mark - Singleton

+ (PhotoManager *)sharedManager
{
	static PhotoManager *sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[PhotoManager alloc] init];
	});
	return sharedManager;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.latestTagLock = [[NSObject alloc] init];
		self.blocksForTags = [NSMutableDictionary dictionary];
		self.photoLoadingQueue = dispatch_queue_create("com.sorensonvrs.ntouch.photoloader", DISPATCH_QUEUE_SERIAL);
		
		self.imageCache = [[NSCache alloc] init];
	}
	return self;
}

- (void)dealloc
{
	self.photoLoadingQueue = nil;
}

- (SCIImage *)cachedPhotoForContact:(SCIContact *)contact {
	return PhotoManagerCachedPhotoForContact(contact);
}

#pragma mark - Helpers: Fetching Indirection

- (NSInteger)fetchPhotoForContact:(SCIContact *)contact withCompletion:(PhotoManagerFetchCompletionBlock)block errorIn:(NSError *)errorIn
{
	NSInteger tag = [self popLatestTag];
	
	[self setBlock:block forTag:tag];
	[self performBlockOnPhotoLoadingQueue:^{
		SCIImage *photo = nil;
		NSError *error = nil;
		BOOL shouldContinue = NO;
		
		//Check whether contact photos are enabled
		if (![[SCIVideophoneEngine sharedEngine] displayContactImages] ||
			![[SCIVideophoneEngine sharedEngine] contactPhotosFeatureEnabled])
		{
			photo = PhotoManagerPhotoPlaceholder();
			shouldContinue = YES;
		}
		
		//Check whether we have a contact.
		if (!shouldContinue && !contact && [self hasBlockForTag:tag])
		{
			if (errorIn)
			{
				error = errorIn;
			}
			shouldContinue = YES;
		}
		
		//Get photo according to GUID.
		if (!shouldContinue && [self hasBlockForTag:tag])
		{
			photo = PhotoManagerPhotoForContact(contact, &error);
		}
		
		[self performWithAndDeleteBlockForTag:tag action:^(PhotoManagerFetchCompletionBlock completion) {
			if (completion)
			{
				// Why NSRunLoopCommonModes? It ends up being necessary in order to post events while a modal window
				// is up, specifically via runModalForWindow: which Mac uses while doing the logout/login.  (NSAlerts
				// don't appear to interfere with this?)  Otherwise the blocks posted to the main thread using this
				// method don't get run.
				[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode, NSRunLoopCommonModes ] block:^{
					completion(photo, error);
				}];
			}
		}];
	}];
	
	return tag;
}


- (NSInteger)fetchPhotoForMessageInfo:(SCIMessageInfo *)messageInfo withCompletion:(PhotoManagerFetchCompletionBlock)block errorIn:(NSError *)errorIn
{
	NSInteger tag = [self popLatestTag];
	
	[self setBlock:block forTag:tag];
	[self performBlockOnPhotoLoadingQueue:^{
		__block SCIImage *photo = nil;
		NSError *error = nil;
		BOOL shouldContinue = NO;
		
		//Check whether we have a messageInfo.
		if (!messageInfo && [self hasBlockForTag:tag])
		{
			if (errorIn)
			{
				error = errorIn;
			}
			shouldContinue = YES;
		}
		
		//Get photo according to messageInfo.previewImageURL.
		if (!shouldContinue && [self hasBlockForTag:tag])
		{
			[[SCIVideophoneEngine sharedEngine] loadPreviewImageForMessageInfo:messageInfo completion:^(NSData *data, NSError *error) {
				if (data) {
					photo = [[SCIImage alloc] initWithData:data];
				} else {
					photo = PhotoManagerPhotoPlaceholder();
				}
				
				[self performWithAndDeleteBlockForTag:tag action:^(PhotoManagerFetchCompletionBlock completion) {
					if (completion)
					{
						// Why NSRunLoopCommonModes? It ends up being necessary in order to post events while a modal window
						// is up, specifically via runModalForWindow: which Mac uses while doing the logout/login.  (NSAlerts
						// don't appear to interfere with this?)  Otherwise the blocks posted to the main thread using this
						// method don't get run.
						[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode, NSRunLoopCommonModes ] block:^{
							completion(photo, error);
						}];
					}
				}];
				
			}];
		} else {
			[self performWithAndDeleteBlockForTag:tag action:^(PhotoManagerFetchCompletionBlock completion) {
				if (completion)
				{
					// Why NSRunLoopCommonModes? It ends up being necessary in order to post events while a modal window
					// is up, specifically via runModalForWindow: which Mac uses while doing the logout/login.  (NSAlerts
					// don't appear to interfere with this?)  Otherwise the blocks posted to the main thread using this
					// method don't get run.
					[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode, NSRunLoopCommonModes ] block:^{
						completion(photo, error);
					}];
				}
			}];
		}
	}];
	
	return tag;
}

- (NSInteger)fetchPhotoForContactWithNumber:(NSString *)number withCompletion:(PhotoManagerFetchCompletionBlock)block errorInUserInfo:(NSDictionary *)userInfo
{
	SCIContact *contact = nil;
	SCIContactPhone phone = SCIContactPhoneNone;
	[[SCIContactManager sharedManager] getContact:&contact
											phone:&phone
										forNumber:number];
	NSError *errorIn = nil;
	if (!contact) {
		errorIn = [NSError errorWithDomain:PhotoManagerErrorDomain code:PhotoManagerErrorCodeNoMatchingContact userInfo:userInfo];
	}
	return [self fetchPhotoForContact:contact withCompletion:block errorIn:errorIn];
}

#pragma mark - Public API: Storing

- (void)storeCustomImage:(SCIImage *)image forContact:(SCIContact *)contact
{
	NSError *error = nil;
	PhotoManagerStoreCustomImageForContact(image, contact, &error);
}

#pragma mark - Public API: Updating

- (void)updateCustomImageFileNameForContact:(SCIContact *)contact withOriginalFileName:(NSString *)originalFilename
{
    NSError *error = nil;

    __unused BOOL storeImageSuccess = PhotoManagerUpdateCustomImageFileNameForContact(contact, originalFilename, &error);
}


#pragma mark - Public API: Downloading

- (void)downloadSorensonPhotoForContact:(SCIContact *)contact withCompletion:(void (^)(BOOL success, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block
{
	[[SCIVideophoneEngine sharedEngine] loadPhotoForContact:contact completion:^(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
		if (data) {
			BOOL storePhotoSuccess = NO;
			
			NSString *fileName = PhotoManagerSorensonPhotoFileNameForIdentifierAndTimestamp(imageId, imageDate);
			NSError *storePhotoError = nil;

			if (fileName != nil) {
				storePhotoSuccess = PhotoManagerStorePhotoWithFileName([[SCIImage alloc] initWithData:data], fileName, &storePhotoError);
			}
			
			block(storePhotoSuccess, phoneNumber, imageId, imageDate, storePhotoError);
		} else {
			block(NO, phoneNumber, imageId, imageDate, error);
		}
	}];
}

- (void)downloadSorensonPhotoForContact:(SCIContact *)contact withCompletionWithImageData:(void (^)(BOOL success, NSData *imageData, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block
{
	[[SCIVideophoneEngine sharedEngine] loadPhotoForContact:contact completion:^(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
		if (data) {
			BOOL storePhotoSuccess = NO;
			
			NSString *fileName = PhotoManagerSorensonPhotoFileNameForIdentifierAndTimestamp(imageId, imageDate);
			NSError *storePhotoError = nil;

			if (fileName != nil) {
				storePhotoSuccess = PhotoManagerStorePhotoWithFileName([[SCIImage alloc] initWithData:data], fileName, &storePhotoError);
			}
			
			block(storePhotoSuccess, data, phoneNumber, imageId, imageDate, storePhotoError);
		} else {
			block(NO, data, phoneNumber, imageId, imageDate, error);
		}
	}];
}

#pragma mark - Helpers: Downloading

- (void)downloadSorensonPhotoAndNotifyForContact:(SCIContact *)contact
{
	[self downloadSorensonPhotoForContact:contact withCompletion:^(BOOL success, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
		if (success) {
			// Why NSRunLoopCommonModes? It ends up being necessary in order to post events while a modal window
			// is up, specifically via runModalForWindow: which Mac uses while doing the logout/login.  (NSAlerts
			// don't appear to interfere with this?)  Otherwise the blocks posted to the main thread using this
			// method don't get run.
			[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode, NSRunLoopCommonModes ] block:^{
				
				// Update contact's TimeStamp if it's nil.  There has been a case where the photoTimeStamp
				// was nil and the UI + Photomanager caused an infinite loop downloading same image.
				// Also another infinite loop condition seems to appear when you have two contacts with the same
				// number, but different photoIDs saved in the contact.photoIdentifier.
				// Reset back to Empty ID to be looked up again.
				BOOL updateContact = NO;
				if (!contact.photoTimestamp || !contact.photoTimestamp.length) {
					contact.photoTimestamp = imageDate;
					updateContact = YES;
				}
				
				if (![contact.photoIdentifier isEqualToString:imageId]) {
					contact.photoIdentifier = PhotoManagerEmptyImageIdentifier();
					updateContact = YES;
				}
				
				if (updateContact) {
					[[SCIContactManager sharedManager] updateContact:contact];
				}
				
				NSMutableDictionary *mutableUserInfo = [NSMutableDictionary dictionary];
				if (phoneNumber) [mutableUserInfo setObject:phoneNumber forKey:PhotoManagerKeyPhoneNumber];
				if (imageId) [mutableUserInfo setObject:imageId forKey:PhotoManagerKeyImageId];
				if (imageDate) [mutableUserInfo setObject:imageDate forKey:PhotoManagerKeyImageDate];
				NSDictionary *userInfo = [mutableUserInfo copy];
				[[NSNotificationCenter defaultCenter] postNotificationName:PhotoManagerNotificationLoadedSorensonPhoto object:self userInfo:userInfo];
			}];
		}
	}];
}

#pragma mark - Public API: Fetch Convenience

- (NSInteger)fetchPhotoOrPlaceholderForContact:(SCIContact *)contact withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block
{
	return [self fetchPhotoForContact:contact withCompletion:PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(block)];
}

- (NSInteger)fetchPhotoOrPlaceholderForContactWithNumber:(NSString *)number withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block
{
	return [self fetchPhotoForContactWithNumber:number withCompletion:PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(block)];
}

- (NSInteger)fetchPhotoOrPlaceholderForCallListItem:(SCICallListItem *)callListItem withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block
{
	return [self fetchPhotoForCallListItem:callListItem withCompletion:PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(block)];
}

- (NSInteger)fetchPhotoOrPlaceholderForMessageInfo:(SCIMessageInfo *)messageInfo withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block
{
	return [self fetchPhotoForMessageInfo:messageInfo withCompletion:PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(block)];
}

- (NSInteger)fetchPhotoOrPlaceholderForCall:(SCICall *)call withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block
{
	return [self fetchPhotoForCall:call withCompletion:PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(block)];
}

#pragma mark - Public API: Fetching

- (NSInteger)fetchPhotoForContact:(SCIContact *)contact withCompletion:(PhotoManagerFetchCompletionBlock)block
{
	return [self fetchPhotoForContact:contact withCompletion:block errorIn:nil];
}

- (NSInteger)fetchPhotoForContactWithNumber:(NSString *)number withCompletion:(PhotoManagerFetchCompletionBlock)block
{
	return [self fetchPhotoForContactWithNumber:number withCompletion:block errorInUserInfo:@{PhotoManagerKeyNumber : number}];
}

- (NSInteger)fetchPhotoForCallListItem:(SCICallListItem *)callListItem withCompletion:(PhotoManagerFetchCompletionBlock)block
{
	return [self fetchPhotoForContactWithNumber:callListItem.phone withCompletion:block errorInUserInfo:@{PhotoManagerKeyCallListItem : callListItem ?: [NSNull null]}];
}

- (NSInteger)fetchPhotoForMessageInfo:(SCIMessageInfo *)messageInfo withCompletion:(PhotoManagerFetchCompletionBlock)block
{
	if (messageInfo.previewImageURL && (messageInfo.previewImageURL.length > 0)) {
		return [self fetchPhotoForMessageInfo:messageInfo withCompletion:block errorIn:nil];
	} else {
		return [self fetchPhotoForContactWithNumber:messageInfo.dialString withCompletion:block errorInUserInfo:@{PhotoManagerKeyMessageInfo : messageInfo ?: [NSNull null]}];
	}
}

- (NSInteger)fetchPhotoForCall:(SCICall *)call withCompletion:(PhotoManagerFetchCompletionBlock)block
{
	return [self fetchPhotoForContactWithNumber:call.dialString withCompletion:block errorInUserInfo:@{PhotoManagerKeyCall : call ?: [NSNull null]}];
}

- (void)stopFetchingPhotoForTag:(NSInteger)tag
{
	[self removeBlockForTag:tag];
}

- (void)deletePhotoWithFileName:(NSString *)fileName {
	PhotoManagerDeletePhotoWithFileName(fileName, nil);
}

#pragma mark - Photo Fetching Block Maintenance

- (BOOL)hasBlockForTag:(NSUInteger)tag
{
	BOOL res = NO;
	@synchronized(self.blocksForTags)
	{
		res = !!self.blocksForTags[@(tag)];
	}
	return res;
}

- (void)setBlock:(PhotoManagerFetchCompletionBlock)block forTag:(NSUInteger)tag
{
	@synchronized(self.blocksForTags)
	{
		[self.blocksForTags setObject:block forKey:@(tag)];
	}
}

- (PhotoManagerFetchCompletionBlock)blockForTag:(NSUInteger)tag
{
	PhotoManagerFetchCompletionBlock res = nil;
	@synchronized(self.blocksForTags)
	{
		return self.blocksForTags[@(tag)];
	}
	return res;
}

- (void)performWithAndDeleteBlockForTag:(NSUInteger)tag action:(void (^)(PhotoManagerFetchCompletionBlock))action
{
	@synchronized(self.blocksForTags)
	{
		id key = @(tag);
		PhotoManagerFetchCompletionBlock block = self.blocksForTags[key];
		if(action) action(block);
		[self.blocksForTags removeObjectForKey:key];
	}
}

- (void)removeBlockForTag:(NSUInteger)tag
{
	@synchronized(self.blocksForTags)
	{
		PhotoManagerFetchCompletionBlock block = nil;
		if ((block = self.blocksForTags[@(tag)])) NSLog(@"%s:removing %@", __PRETTY_FUNCTION__, block);
		[self.blocksForTags removeObjectForKey:@(tag)];
	}
}

#pragma mark - Helpers: Performing Blocks

- (void)performBlockOnPhotoLoadingQueue:(void (^)(void))block
{
	dispatch_async(self.photoLoadingQueue, block);
}

#pragma mark - Helpers: Latest Tag

- (NSInteger)popLatestTag
{
	NSInteger res = 0;
	@synchronized(self.latestTagLock)
	{
		mLatestTag++;
		res = mLatestTag;
	}
	return res;
}

@end

#pragma mark - Static Functions
//Declarations For Convenience Functions Used By Helper Functions
static SCIImage *PhotoManagerAvatarForContact(SCIContact *contact, NSError * __autoreleasing *errorOut);
static SCIImage *PhotoManagerFacebookPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut);
static SCIImage *PhotoManagerSorensonPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut);
static SCIImage *PhotoManagerCustomImageForContact(SCIContact *contact, NSError * __autoreleasing *errorOut);
static PhotoType PhotoManagerPhotoTypeForIdentifier(NSString *identifier);
static SCIImage *PhotoManagerStoredImageWithFileName(NSString *fileName, NSError *__autoreleasing *errorOut);
static NSString *PhotoManagerPathWithFileName(NSString *fileName, NSError * __autoreleasing *errorOut);
static void PhotoManagerDeletePhotoWithFileName(NSString *fileName, NSError * __autoreleasing *errorOut);
static BOOL PhotoManagerStorePhotoWithFileName(SCIImage *photoData, NSString *fileName, NSError * __autoreleasing *errorOut);
static BOOL PhotoManagerUpdatePhotoWithFileName(NSString *fileName, NSString *originalFileName, NSError * __autoreleasing *errorOut);

//Helper Function Definitions
static SCIImage *PhotoManagerPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	SCIImage *photo = nil;
	NSError *error = nil;
	
	NSString *identifier = contact.photoIdentifier;
	PhotoType type = PhotoManagerPhotoTypeForIdentifier(identifier);
	
	switch (type) {
		case PhotoTypeUnknown: {
			error = [NSError errorWithDomain:PhotoManagerErrorDomain code:PhotoManagerErrorCodePhotoTypeUnknown userInfo:nil];
		} break;
		case PhotoTypePlaceholder: {
			photo = PhotoManagerPhotoPlaceholder();
		} break;
		case PhotoTypeFacebook: {
			photo = PhotoManagerFacebookPhotoForContact(contact, &error);
		} break;
		case PhotoTypeCustom: {
			photo = PhotoManagerCustomImageForContact(contact, &error);
		} break;
		case PhotoTypeAvatar: {
			photo = PhotoManagerAvatarForContact(contact, &error);
		} break;
		case PhotoTypeSorenson: {
			photo = PhotoManagerSorensonPhotoForContact(contact, &error);
		} break;
		case PhotoTypeSorensonVRS: {
			photo = PhotoManagerPhotoSorensonVRS();
		} break;
		case PhotoTypeRapidoVRS: {
			photo = PhotoManagerPhotoRapidoVRS();
		} break;
		case PhotoTypeCIR: {
			photo = PhotoManagerPhotoCIR();
		} break;
		case PhotoTypeTechSupport: {
			photo = PhotoManagerPhotoTechSupport();
		} break;
		case PhotoTypeVRI: {
			photo = PhotoManagerPhotoVRI();
		} break;
	}
	
	if (errorOut)
	{
		*errorOut = error;
	}
	return photo;
}

static SCIImage *PhotoManagerCachedPhotoForContact(SCIContact *contact)
{
	SCIImage *photo = nil;
	
	NSString *identifier = contact.photoIdentifier;
	PhotoType type = PhotoManagerPhotoTypeForIdentifier(identifier);
	
	switch (type) {
		case PhotoTypeUnknown: {
			photo = nil;
		} break;
		case PhotoTypePlaceholder: {
			photo = PhotoManagerPhotoPlaceholder();
		} break;
		case PhotoTypeFacebook: {
			// We don't currently cache device contact photos...
			photo = nil;
		} break;
		case PhotoTypeCustom: {
			NSString *fileName = PhotoManagerCustomImageFileNameForContact(contact);
			if (fileName != nil) {
				photo = [PhotoManager.sharedManager.imageCache objectForKey:fileName];
			} else {
				photo = nil;
			}
		} break;
		case PhotoTypeAvatar: {
			photo = PhotoManagerAvatarForContact(contact, nil);
		} break;
		case PhotoTypeSorenson: {
			NSString *fileName = PhotoManagerSorensonPhotoFileNameForContact(contact);
			if (fileName != nil) {
				photo = [PhotoManager.sharedManager.imageCache objectForKey:fileName];
			} else {
				photo = nil;
			}
		} break;
		case PhotoTypeSorensonVRS: {
			photo = PhotoManagerPhotoSorensonVRS();
		} break;
		case PhotoTypeRapidoVRS: {
			photo = PhotoManagerPhotoRapidoVRS();
		} break;
		case PhotoTypeCIR: {
			photo = PhotoManagerPhotoCIR();
		} break;
		case PhotoTypeTechSupport: {
			photo = PhotoManagerPhotoTechSupport();
		} break;
		case PhotoTypeVRI: {
			photo = PhotoManagerPhotoVRI();
		} break;
	}
	
	return photo;
}

static PhotoManagerFetchCompletionBlock PhotoManagerFetchCompletionBlockFromFetchOrPlaceholderCompletionBlock(PhotoManagerFetchOrPlaceholderCompletionBlock block)
{
	return ^(SCIImage *imageIn, NSError *error) {
		SCIImage *image = nil;
		if (imageIn) {
			image = imageIn;
		} else {
			image = PhotoManagerPhotoPlaceholder();
		}
		block(image);
	};
}

//Convenience Function Definitions
static SCIImage *PhotoManagerAvatarForContact(SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	SCIImage *photo = nil;
	NSError *error = nil;
	
	//expecting identifier of the form @"00000000-0000-0000-0000-000000000000"
	NSString *identifier = contact.photoIdentifier;
	NSArray *identifierComponents = [identifier componentsSeparatedByString:@"-"];
	if (identifierComponents.count == 5)
	{
		NSString *finalComponent = identifierComponents[4];
		NSInteger finalComponentValue = finalComponent.integerValue;
		NSString *avatarImageName = [NSString stringWithFormat:@"%zd-1.jpg", finalComponentValue];
		photo = [SCIImage imageNamed:avatarImageName];
	}
	
	if (errorOut)
	{
		*errorOut = error;
	}
	return photo;
}

static SCIImage *PhotoManagerFacebookPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	SCIImage *photo = nil;
	NSError *error = nil;
	
#if TARGET_OS_IPHONE
	NSArray *phones = contact.phones;
	NSUInteger count = phones.count;
	NSData *imageData;
	
	// Loop through all contact phones.
	for (NSUInteger i = 0; i < count; i++) {
		NSArray *contactsArray = [ContactUtilities findContactsByPhoneNumber:phones[i]];
		if(contactsArray.count) {
			// Loop through all found contacts.
			for (int j = 0; j < contactsArray.count; j++) {
				CNContact *contact = [contactsArray objectAtIndex:j];
				if(contact.thumbnailImageData) {
					imageData = contact.thumbnailImageData;
					photo = [SCIImage imageWithData:imageData];
					break;
				}
			}
		}
	}
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	ABPerson *match = [[ABAddressBook sharedAddressBook] personBestMatchingSCIContact:contact];
	if (match)
	{
		NSData *photoData = match.imageData;
		photo = [[SCIImage alloc] initWithData:photoData];
	}
#endif
	if (!photo)
		error = [NSError errorWithDomain:PhotoManagerErrorDomain code:PhotoManagerErrorCodeNoFacebookPhotoFound userInfo:nil];
	
	if (errorOut)
	{
		*errorOut = error;
	}

	return photo;
}


static NSString *PhotoManagerSorensonPhotoFileNameForIdentifierAndTimestamp(NSString *identifier, NSString *timestamp)
{
	if (identifier != nil && timestamp != nil) {
		NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
		[dateFormatter setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"]];
		[dateFormatter setDateFormat:@"MM/dd/yyyy HH:mm:ss"];
		NSDate *date = [dateFormatter dateFromString:timestamp];
		
		return date != nil ? [NSString stringWithFormat:@"%@-%.f", identifier, date.timeIntervalSince1970] : nil;
	}
	
	return nil;
}


static NSString *PhotoManagerSorensonPhotoFileNameForContact(SCIContact *contact)
{
	return PhotoManagerSorensonPhotoFileNameForIdentifierAndTimestamp(contact.photoIdentifier, contact.photoTimestamp);
}


static SCIImage *PhotoManagerSorensonPhotoForContact(SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	SCIImage *photo = nil;
	NSError *error = nil;
	
	NSString *fileName = PhotoManagerSorensonPhotoFileNameForContact(contact);
	
	if (fileName != nil)
	{
		photo = PhotoManagerStoredImageWithFileName(fileName, &error);
	}
	
	if (fileName == nil || photo == nil)
	{
		NSFileManager  *manager = [NSFileManager defaultManager];
		
#if TARGET_OS_IPHONE
		// the photo-cache directory
		NSString *path = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"photo-cache"];
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		// the preferred way to get the ntouch-mac directory
		NSString *path = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"ntouch-mac"];
#endif
		// grab all the files in the dir
		NSArray *allFiles = [manager contentsOfDirectoryAtPath:path error:nil];
		
		// filter the array for only contact.photoIdentifier files
		NSPredicate *fltr = [NSPredicate predicateWithFormat:@"self BEGINSWITH %@", contact.photoIdentifier];
		NSArray *contactPhotoIdentifierFiles = [allFiles filteredArrayUsingPredicate:fltr];
		
		// use fast enumeration to iterate the array and delete the files
		for (NSString *contactPhotoIdentifierFile in contactPhotoIdentifierFiles)
		{
			NSError *error = nil;
			[manager removeItemAtPath:[path stringByAppendingPathComponent:contactPhotoIdentifierFile] error:&error];
			[PhotoManager.sharedManager.imageCache removeObjectForKey:contactPhotoIdentifierFile];
		}
		
		[[PhotoManager sharedManager] downloadSorensonPhotoAndNotifyForContact:contact];
	}
	
	if (errorOut)
	{
		*errorOut = error;
	}
	return photo;
}

static NSString *PhotoManagerCustomImageFileNameForContact(SCIContact *contact)
{
	NSString *fileName = nil;
	
	if (contact.phones.count > 0) {
        if (contact.isFavorite) {
            fileName = [[SCIContactManager sharedManager] contactForFavorite:contact].phones[0];
        } else {
            fileName = contact.phones[0];
        }
	}
	
	
	return fileName;
}

static SCIImage *PhotoManagerCustomImageForContact(SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	SCIImage *image = nil;
	NSError *error = nil;
	
	NSString *fileName = PhotoManagerCustomImageFileNameForContact(contact);
	if (fileName != nil) {
		image = PhotoManagerStoredImageWithFileName(fileName, &error);
	}
	
	if (errorOut)
	{
		*errorOut = error;
	}
	return image;
}

static BOOL PhotoManagerUpdateCustomImageFileNameForContact(SCIContact *contact, NSString *originalFileName, NSError * __autoreleasing *errorOut)
{
    BOOL res = NO;
	
	NSString *fileName = PhotoManagerCustomImageFileNameForContact(contact);
    
    if (fileName) {
        res = PhotoManagerUpdatePhotoWithFileName(fileName, originalFileName, errorOut);
	} else {
		PhotoManagerDeletePhotoWithFileName(originalFileName, errorOut);
	}
    
    return res;
}

static BOOL PhotoManagerUpdatePhotoWithFileName(NSString *fileName, NSString *originalFileName, NSError * __autoreleasing *errorOut)
{
    BOOL res = NO;
    NSError *error = nil;
    
    NSString *originalPath = PhotoManagerPathWithFileName(originalFileName, &error);
    NSString *path = PhotoManagerPathWithFileName(fileName, &error);
    res = [[NSFileManager defaultManager] moveItemAtPath:originalPath toPath:path error:nil];
	
	SCIImage *photo = [PhotoManager.sharedManager.imageCache objectForKey:originalFileName];
	if (photo != nil) {
		[PhotoManager.sharedManager.imageCache removeObjectForKey:originalFileName];
		[PhotoManager.sharedManager.imageCache setObject:photo forKey:fileName cost:photo.size.width * photo.size.height];
	}

    if (errorOut)
    {
        *errorOut = error;
    }
    return res;
}

static BOOL PhotoManagerStoreCustomImageForContact(SCIImage *photo, SCIContact *contact, NSError * __autoreleasing *errorOut)
{
	BOOL res = NO;
	
	NSString *fileName = PhotoManagerCustomImageFileNameForContact(contact);
	if (fileName != nil) {
		res = PhotoManagerStorePhotoWithFileName(photo, fileName, errorOut);
	}
	
	return res;
}

static BOOL PhotoManagerStorePhotoWithFileName(SCIImage *photo, NSString *fileName, NSError * __autoreleasing *errorOut)
{
	BOOL res = NO;
	NSError *error = nil;
	SCIImage *photoToSave = photo != nil ? photo : PhotoManagerErrorImage();

	[PhotoManager.sharedManager.imageCache setObject:photoToSave forKey:fileName cost:photoToSave.size.width * photoToSave.size.height];
	
	#if TARGET_OS_IPHONE
	NSData *photoData = UIImageJPEGRepresentation(photoToSave, 1.0);
	#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	NSData *photoData = [photoToSave bnr_representationUsingType:NSJPEGFileType properties:nil];
	#endif
	NSString *path = PhotoManagerPathWithFileName(fileName, &error);
	res = [photoData writeToFile:path atomically:NO];
	
	if (errorOut)
	{
		*errorOut = error;
	}
	return res;
}

static PhotoType PhotoManagerPhotoTypeForIdentifier(NSString *identifier)
{
	PhotoType res = PhotoTypeUnknown;
	
	//expecting identifier of the form @"00000000-0000-0000-0000-000000000000"
	NSArray *identifierComponents = [identifier componentsSeparatedByString:@"-"];
	if (identifierComponents.count == 5) {
		//could the identifier be a "special" (placeholder, avatar, or "facebook") ?
		BOOL special = [identifier hasPrefix:@"00000000-0000-0000-0000-00000000"];
		if (special)
		{
			NSString *finalComponent = identifierComponents[4];
			NSInteger finalComponentValue = finalComponent.integerValue;
			if (finalComponentValue == 1 || finalComponentValue == 0)
			{
				res = PhotoTypePlaceholder;
			}
			else if (finalComponentValue >= PhotoManagerAvatarLowIndex && finalComponentValue <= PhotoManagerAvatarHighIndex)
			{
				res = PhotoTypeAvatar;
			}
			else if (finalComponentValue == PhotoManagerFacebookIndex)
			{
				res = PhotoTypeFacebook;
			}
			else if (finalComponentValue == PhotoManagerCustomImageIndex)
			{
				res = PhotoTypeCustom;
			}
			else if (finalComponentValue == PhotoManagerSorensonVRSImageIndex)
			{
				res = PhotoTypeSorensonVRS;
			}
			else if (finalComponentValue == PhotoManagerRapidoVRSImageIndex)
			{
				res = PhotoTypeRapidoVRS;
			}
			else if (finalComponentValue == PhotoManagerCIRImageIndex)
			{
				res = PhotoTypeCIR;
			}
			else if (finalComponentValue == PhotoManagerTechSupportImageIndex)
			{
				res = PhotoTypeTechSupport;
			}
			else if (finalComponentValue == PhotoManagerVRIImageIndex)
			{
				res = PhotoTypeVRI;
			}
			else
			{
				special = NO;
			}
		}
		if (!special)
		{
			res = PhotoTypeSorenson;
		}
	}
	
	return res;
}

static NSString *PhotoManagerPathWithFileName(NSString *string, NSError * __autoreleasing *errorOut)
{
	NSString *res = nil;
	NSError *error = nil;
	
#if TARGET_OS_IPHONE
	NSString *path = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"photo-cache"];
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	NSString *path = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"ntouch-mac"];
#endif
	
	NSString *name = string;
	NSString *extension = @"jpg";
	res = path;
	res = [res stringByAppendingPathComponent:name];
	res = [res stringByAppendingPathExtension:extension];
	
	BOOL isDir;
	if(![[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir])
		if(![[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error])
			NSLog(@"Error: Create folder failed");

	if (errorOut)
	{
		*errorOut = error;
	}
	return res;
}

static SCIImage *PhotoManagerStoredImageWithFileName(NSString *fileName, NSError * __autoreleasing *errorOut)
{
	SCIImage *photo = nil;
	NSError *error = nil;
	
	photo = [PhotoManager.sharedManager.imageCache objectForKey:fileName];
	if (photo == nil) {
		NSString *path = PhotoManagerPathWithFileName(fileName, &error);
		photo = [[SCIImage alloc] initWithContentsOfFile:path];
		if (photo != nil) {
			[PhotoManager.sharedManager.imageCache setObject:photo forKey:fileName cost:photo.size.width * photo.size.height];
		}
	}
	
	if (errorOut) {
		*errorOut = error;
	}
	return photo;
}

static void PhotoManagerDeletePhotoWithFileName(NSString *fileName, NSError * __autoreleasing *errorOut)
{
	NSError *error = nil;
	NSString *filePath = PhotoManagerPathWithFileName(fileName, errorOut);
	
	[PhotoManager.sharedManager.imageCache removeObjectForKey:fileName];
	if ([[NSFileManager defaultManager] removeItemAtPath:filePath error:&error] != YES)
		NSLog(@"Unable to delete file: %@", [error localizedDescription]);
	
	if (errorOut)
	{
		*errorOut = error;
	}
}

#pragma mark - Global Functions
NSString *PhotoManagerRemoveImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-000000000000"];
}

NSString *PhotoManagerEmptyImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-000000000001"];
}

NSString *PhotoManagerFacebookPhotoIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerFacebookIndex];
}

NSString *PhotoManagerCustomImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerCustomImageIndex];
}

NSString *PhotoManagerSorensonVRSImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerSorensonVRSImageIndex];
}

NSString *PhotoManagerRapidoVRSImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerRapidoVRSImageIndex];
}

NSString *PhotoManagerCIRImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerCIRImageIndex];
}

NSString *PhotoManagerTechSupportImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerTechSupportImageIndex];
}

NSString *PhotoManagerVRIImageIdentifier()
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)PhotoManagerVRIImageIndex];
}

NSString *PhotoManagerAvatarPhotoIdentifierForIndex(NSUInteger index)
{
	return [NSString stringWithFormat:@"00000000-0000-0000-0000-%012lu", (unsigned long)index];
}

SCIImage *PhotoManagerPhotoPlaceholder()
{
#if TARGET_OS_IPHONE
	return nil;
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"PersonPlaceholder"];
#endif
}

SCIImage *PhotoManagerPhotoPeoplePlaceholder()
{
#if TARGET_OS_IPHONE
	return nil;
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"PeoplePlaceholder"];
#endif
}

SCIImage *PhotoManagerPhotoBlocked()
{
#if TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"Blocked_Grey48"];
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"Blocked_Grey125"];
#endif
}

SCIImage *PhotoManagerPhotoSorensonVRS()
{
	return [SCIImage imageNamed:@"SorensonVRS"];
}

SCIImage *PhotoManagerPhotoRapidoVRS()
{
	return [SCIImage imageNamed:@"SorensonVRS"];
}

SCIImage *PhotoManagerPhotoCIR()
{
	return [SCIImage imageNamed:@"CIR"];
}

SCIImage *PhotoManagerPhotoTechSupport()
{
	return [SCIImage imageNamed:@"TechSupport"];
}

SCIImage *PhotoManagerPhotoVRI()
{
	return [SCIImage imageNamed:@"SCIS_hug-bug-320"];
}

SCIImage *PhotoManagerPhotoURL()
{
	return [SCIImage imageNamed:@"URL.pdf"];
}

SCIImage *PhotoManagerPhotoVideo()
{
	return [SCIImage imageNamed:@"Video.pdf"];
}

SCIImage *PhotoManagerErrorImage()
{
#if TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"ContactPhotoErrorImage"];
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	return [SCIImage imageNamed:@"PersonPlaceholder"];
#endif
}

PhotoType PhotoManagerPhotoTypeForContact(SCIContact *contact)
{
	return PhotoManagerPhotoTypeForIdentifier(contact.photoIdentifier);
}

inline SCISize PhotoManagerDefaultPhotoSize()
{
#if TARGET_OS_IPHONE
	return CGSizeMake(320, 320);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	return NSMakeSize(320, 320);
#endif
}

NSNotificationName const PhotoManagerNotificationLoadedSorensonPhoto = @"PhotoManagerNotificationLoadedSorensonPhoto";
NSString * const PhotoManagerKeyPhoneNumber = @"phoneNumber";
NSString * const PhotoManagerKeyImageDate = @"imageDate";
NSString * const PhotoManagerKeyImageId = @"imageId";

NSString * const PhotoManagerErrorDomain = @"PhotoManagerErrorDomain";

const NSInteger PhotoManagerErrorCodeNoMatchingContact = 1;
const NSInteger PhotoManagerErrorCodePhotoIdentifierOutOfAvatarRange = 2;
const NSInteger PhotoManagerErrorCodePhotoTypeUnknown = 3;
const NSInteger PhotoManagerErrorCodeNoFacebookPhotoFound = 4;

NSString * const PhotoManagerKeyCallListItem = @"callListItem";
NSString * const PhotoManagerKeyMessageInfo = @"messageInfo";
NSString * const PhotoManagerKeyCall = @"call";
NSString * const PhotoManagerKeyNumber = @"number";

