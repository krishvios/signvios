//
//  YelpManager.h
//  YelpDemo
//
//  Created by Nate Chandler on 6/20/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#if TARGET_OS_IPHONE
//#import <UIKit/UIKit.h>
#define SCIImage UIImage
#define SCISize CGSize
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
#define SCIImage NSImage
#define SCISize NSSize
#endif

@class YelpRegion;
@class YelpBusiness;

@interface YelpManager : NSObject

@property (nonatomic) CLAuthorizationStatus locationManagerAuthorizationStatus;
@property (nonatomic) CLLocationManager *locationManager;


@property (class, readonly) YelpManager *sharedManager;

- (id)fetchJSONResultForSearchWithDictionary:(NSDictionary *)dictionary
									   delay:(NSTimeInterval)delay
									completion:(void (^)(id JSONObject, NSError *error))block;
- (id)fetchJSONResultForSearchWithDictionary:(NSDictionary *)dictionary
								  completion:(void (^)(id JSONObject, NSError *error))block;
- (id)JSONResultForSynchronousSearchWithDictionary:(NSDictionary *)dictionary error:(NSError * __autoreleasing *)errorOut;
- (id)JSONResultForSynchronousPhoneSearchWithDictionary:(NSDictionary *)dictionary error:(NSError * __autoreleasing *)errorOut;


- (id)fetchResultForSearchWithDictionary:(NSDictionary *)dictionary
							  completion:(void (^)(YelpRegion *region, NSUInteger resultCount, NSArray<YelpBusiness *> *businesses, NSError *error))block;
- (id)fetchResultForSearchWithDictionary:(NSDictionary *)dictionary
								   delay:(NSTimeInterval)delay
							  completion:(void (^)(YelpRegion *region, NSUInteger resultCount, NSArray<YelpBusiness *> *businesses, NSError *error))block;

- (BOOL)cancelFetchWithID:(id)theID;

- (void)startUpdatingLocation;
- (void)stopUpdatingLocation;
- (void)requestWhenInUseAuthorization API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos);

@end

extern NSString * const kYelpManagerSearchQueryKeyTerm;
extern NSString * const kYelpManagerSearchQueryKeyLimit;
extern NSString * const kYelpManagerSearchQueryKeyOffset;
extern NSString * const kYelpManagerSearchQueryKeySort;
extern NSString * const kYelpManagerSearchQueryKeyCategoryFilter;
extern NSString * const kYelpManagerSearchQueryKeyRadiusFilter;
extern NSString * const kYelpManagerSearchQueryKeyDealsFilter;

extern NSString * const kYelpManagerSearchQueryKeyCountryCode;
extern NSString * const kYelpManagerSearchQueryKeyLanguageCode;
extern NSString * const kYelpManagerSearchQueryKeyLanguageFilter;

extern NSString * const kYelpManagerSearchQueryKeyLocation;
extern NSString * const kYelpManagerSearchQueryKeyLocationCoordinates;

extern NSString * const kYelpManagerPhoneSearchQueryKeyPhone;
extern NSString * const kYelpManagerPhoneSearchQueryKeyCountryCode;
