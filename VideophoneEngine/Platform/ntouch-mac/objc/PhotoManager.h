//
//  PhotoManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 10/18/12.
//  Copyright (c) 2012-2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#define SCIImage UIImage
#define SCISize CGSize
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
#define SCIImage NSImage
#define SCISize NSSize
#endif

typedef enum  {
	PhotoTypeUnknown,
	PhotoTypePlaceholder,
	PhotoTypeAvatar,
	PhotoTypeSorenson,
	PhotoTypeFacebook,
	PhotoTypeCustom,
	PhotoTypeSorensonVRS,
	PhotoTypeRapidoVRS,
	PhotoTypeCIR,
	PhotoTypeTechSupport,
	PhotoTypeVRI
} PhotoType;

@class SCIContact;
@class SCICallListItem;
@class SCIMessageInfo;
@class SCICall;

typedef void (^PhotoManagerFetchCompletionBlock)(SCIImage *photo, NSError *error);
typedef void (^PhotoManagerFetchOrPlaceholderCompletionBlock)(SCIImage *photo);

@interface PhotoManager : NSObject

@property (class, readonly) PhotoManager *sharedManager;

//Store
- (void)storeCustomImage:(SCIImage *)image forContact:(SCIContact *)contact;

//Update filename
- (void)updateCustomImageFileNameForContact:(SCIContact *)contact withOriginalFileName:(NSString *)originalFilename;

//Download
- (void)downloadSorensonPhotoForContact:(SCIContact *)contact withCompletion:(void (^)(BOOL success, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block;
- (void)downloadSorensonPhotoForContact:(SCIContact *)contact withCompletionWithImageData:(void (^)(BOOL success, NSData *imageData, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block;

// Returns a cached image if it is available in the cache. Otherwise, returns nil.
- (SCIImage *)cachedPhotoForContact:(SCIContact *)contact;

//Fetch
- (NSInteger)fetchPhotoForContact:(SCIContact *)contact withCompletion:(PhotoManagerFetchCompletionBlock)block;
- (NSInteger)fetchPhotoForContactWithNumber:(NSString *)number withCompletion:(PhotoManagerFetchCompletionBlock)block;
- (NSInteger)fetchPhotoForCallListItem:(SCICallListItem *)callListItem withCompletion:(PhotoManagerFetchCompletionBlock)block;
- (NSInteger)fetchPhotoForMessageInfo:(SCIMessageInfo *)messageInfo withCompletion:(PhotoManagerFetchCompletionBlock)block;
- (NSInteger)fetchPhotoForCall:(SCICall *)call withCompletion:(PhotoManagerFetchCompletionBlock)block;

//Fetch Convenience
- (NSInteger)fetchPhotoOrPlaceholderForContact:(SCIContact *)contact withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block;
- (NSInteger)fetchPhotoOrPlaceholderForContactWithNumber:(NSString *)number withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block;
- (NSInteger)fetchPhotoOrPlaceholderForCallListItem:(SCICallListItem *)callListItem withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block;
- (NSInteger)fetchPhotoOrPlaceholderForMessageInfo:(SCIMessageInfo *)messageInfo withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block;
- (NSInteger)fetchPhotoOrPlaceholderForCall:(SCICall *)call withCompletion:(PhotoManagerFetchOrPlaceholderCompletionBlock)block;

//Stop Fetch
- (void)stopFetchingPhotoForTag:(NSInteger)tag;
- (void)deletePhotoWithFileName:(NSString *)fileName;

@end

extern NSNotificationName const PhotoManagerNotificationLoadedSorensonPhoto;

extern NSString * const PhotoManagerKeyPhoneNumber;
extern NSString * const PhotoManagerKeyImageDate;
extern NSString * const PhotoManagerKeyImageId;

extern NSUInteger const PhotoManagerAvatarLowIndex;
extern NSUInteger const PhotoManagerAvatarHighIndex;
extern NSUInteger const PhotoManagerFacebookIndex;
extern NSUInteger const PhotoManagerCustomImageIndex;
extern NSUInteger const PhotoManagerSorensonVRSImageIndex;
extern NSUInteger const PhotoManagerRapidoVRSImageIndex;
extern NSUInteger const PhotoManagerCIRImageIndex;
extern NSUInteger const PhotoManagerTechSupportImageIndex;
#ifdef __cplusplus
extern "C" {
#endif
extern NSString *PhotoManagerRemoveImageIdentifier();
extern NSString *PhotoManagerEmptyImageIdentifier();
extern NSString *PhotoManagerFacebookPhotoIdentifier();
extern NSString *PhotoManagerCustomImageIdentifier();
extern NSString *PhotoManagerSorensonVRSImageIdentifier();
extern NSString *PhotoManagerRapidoVRSImageIdentifier();
extern NSString *PhotoManagerCIRImageIdentifier();
extern NSString *PhotoManagerTechSupportImageIdentifier();
extern NSString *PhotoManagerVRIImageIdentifier();
extern inline NSString *PhotoManagerAvatarPhotoIdentifierForIndex(NSUInteger index);
extern SCIImage *PhotoManagerPhotoPlaceholder();
extern SCIImage *PhotoManagerPhotoPeoplePlaceholder();
extern SCIImage *PhotoManagerPhotoBlocked();
extern SCIImage *PhotoManagerPhotoSorensonVRS();
extern SCIImage *PhotoManagerPhotoRapidoVRS();
extern SCIImage *PhotoManagerPhotoCIR();
extern SCIImage *PhotoManagerPhotoTechSupport();
extern SCIImage *PhotoManagerPhotoVRI();
extern SCIImage *PhotoManagerPhotoURL();
extern SCIImage *PhotoManagerPhotoVideo();
extern SCIImage *PhotoManagerErrorImage();
extern PhotoType PhotoManagerPhotoTypeForContact(SCIContact *contact);
extern SCISize PhotoManagerDefaultPhotoSize();
#ifdef __cplusplus
}
#endif

extern NSString * const PhotoManagerErrorDomain;
extern const NSInteger PhotoManagerErrorCodeNoMatchingContact;
extern const NSInteger PhotoManagerErrorCodePhotoIdentifierOutOfAvatarRange;
extern const NSInteger PhotoManagerErrorCodePhotoTypeUnknown;
extern const NSInteger PhotoManagerErrorCodeNoFacebookPhotoFound;
extern NSString * const PhotoManagerKeyCallListItem;
extern NSString * const PhotoManagerKeyMessageInfo;
extern NSString * const PhotoManagerKeyCall;
extern NSString * const PhotoManagerKeyNumber;

