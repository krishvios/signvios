//
//  YelpManager.m
//  YelpDemo
//
//  Created by Nate Chandler on 6/20/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "YelpManager.h"
#import "BNRBackgroundURLConnectionMakingRunLoopThread.h"
#import "YelpRegion.h"
#import "YelpRegion_Internal.h"
#import "YelpBusiness.h"
#import <CoreLocation/CoreLocation.h>
#import "SCIVideophoneEngine.h"
#import "SCIEmergencyAddress.h"
#import "BNRUUID.h"
#import "NSObject+BNRDebug.h"

@interface YelpManager () <CLLocationManagerDelegate>
@property (nonatomic) BNRBackgroundURLConnectionMakingRunLoopThread *thread;

@property (nonatomic) NSString *baseURL;
@end

static NSString * const kYelpManagerHTTPVerbGet = @"GET";
static NSString * const kYelpManagerHTTPVerbHead = @"HEAD";
static NSString * const kYelpManagerHTTPVerbPost = @"POST";
static NSString * const kYelpManagerHTTPVerbPut = @"PATCH";
static NSString * const kYelpManagerHTTPVerbDelete = @"DELETE";
static NSString * const kYelpManagerHTTPVerbTrace = @"TRACE";
static NSString * const kYelpManagerHTTPVerbOptions = @"OPTIONS";
static NSString * const kYelpManagerHTTPVerbConnect = @"CONNECT";
static NSString * const kYelpManagerHTTPVerbPatch = @"PATCH";

static NSString * const kYelpManagerScheme = @"https";
static NSString * const kYelpManagerV3APIHost = @"api.yelp.com";
static NSString * const kYelpManagerV3URLPath = @"/v3/businesses/search";

static NSString * const kYelpManagerPhoneSearchQueryKeyYWSID = @"ywsid";

static NSString * const kYelpManagerSearchResultKeyError = @"error";
static NSString * const kYelpManagerSearchResultKeyRegion = @"region";
static NSString * const kYelpManagerSearchResultKeyTotal = @"total";
static NSString * const kYelpManagerSearchResultKeyBusinesses = @"businesses";

static NSString * const kYelpManagerAPIKey = @"KuchZ9eE64nwuJHXuETVEHdiWqXupOAUXnipfUCVafuUQhlj6ijBRyOG1O8EcoZrW2V_uYlCqmXGt6elXGr_SHMH4bapQDsnSMxpxpB-I4RG6wOfawHwdPyVPrX3WXYx";

NSString * const kYelpManagerSearchQueryKeyTerm = @"term";
NSString * const kYelpManagerSearchQueryKeyLimit = @"limit";
NSString * const kYelpManagerSearchQueryKeyOffset = @"offset";
NSString * const kYelpManagerSearchQueryKeySort = @"sort";
NSString * const kYelpManagerSearchQueryKeyCategoryFilter = @"category_filter";
NSString * const kYelpManagerSearchQueryKeyRadiusFilter = @"radius_filter";
NSString * const kYelpManagerSearchQueryKeyDealsFilter = @"deals_filter";

NSString * const kYelpManagerSearchQueryKeyCountryCode = @"cc";
NSString * const kYelpManagerSearchQueryKeyLanguageCode = @"lang";
NSString * const kYelpManagerSearchQueryKeyLanguageFilter = @"lang_filter";

NSString * const kYelpManagerSearchQueryKeyLocation = @"location";

NSString * const kYelpManagerPhoneSearchQueryKeyPhone = @"phone";
NSString * const kYelpManagerPhoneSearchQueryKeyCountryCode = @"cc";

@implementation YelpManager

#pragma mark - Singleton

+ (YelpManager *)sharedManager
{
	static YelpManager *sharedManager = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[self alloc] init];
	});
	
	return sharedManager;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.locationManager = [[CLLocationManager alloc] init];
		self.locationManager.delegate = self;
		self.locationManagerAuthorizationStatus = kCLAuthorizationStatusNotDetermined;
		
		[self setupThread];
	}
	return self;
}

- (void)dealloc
{
	[self teardownThread];
}

#pragma mark - Thread Lifecycle

- (void)setupThread
{
	self.thread = [[BNRBackgroundURLConnectionMakingRunLoopThread alloc] init];
}

- (void)teardownThread
{
	[self.thread exit:self waitUntilDone:NO];
}

#pragma mark - Accessors

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

#pragma mark - Helpers: Location

- (NSDictionary *)locationDictionary
{
	NSDictionary *res = nil;

	CLLocation *location = self.locationManager.location;
	if ([CLLocationManager locationServicesEnabled] &&
		location) {
			
		CLLocationCoordinate2D coordinate = location.coordinate;
		res = @{@"latitude" : [NSString stringWithFormat:@"%f", coordinate.latitude], @"longitude" : [NSString stringWithFormat:@"%f", coordinate.longitude]};
	} else {
		//Fall-back to E911 address if it's available
//		SCIEmergencyAddress *address = self.engine.emergencyAddress;
//		if (address && address.zipCode) {
//			res = @{kYelpManagerSearchQueryKeyLocation: address.zipCode};
//		} else {
			res = [[NSDictionary alloc] init];
//		}
	}
	
	return res;
}

- (NSDictionary *)enhancedDictionaryWithLocation:(NSDictionary *)dictionary
{
	NSMutableDictionary *mutableRes = [[NSMutableDictionary alloc] init];
	
	[mutableRes addEntriesFromDictionary:dictionary];
	[mutableRes addEntriesFromDictionary:self.locationDictionary];
	
	return [mutableRes copy];
}

#pragma mark - Helpers: Request Generation

- (NSMutableURLRequest *)requestForSearchWithDictionary:(NSDictionary *)dictionary
{	
	NSURLComponents *urlComponents = [[NSURLComponents alloc] init];
	urlComponents.scheme = kYelpManagerScheme;
	urlComponents.host = kYelpManagerV3APIHost;
	urlComponents.path = kYelpManagerV3URLPath;
	NSString *latitude = [dictionary objectForKey:@"latitude"];
	NSString *longitude = [dictionary objectForKey:@"longitude"];
	NSString *searchTerm = [dictionary objectForKey:@"term"];
	if (latitude && longitude) {
		urlComponents.query = [NSString stringWithFormat:@"latitude=%@&longitude=%@&term=%@", latitude, longitude, searchTerm];
	} else {
		urlComponents.query = [NSString stringWithFormat:@"term=%@", searchTerm];
	}
	NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:urlComponents.URL];
	request.HTTPMethod = kYelpManagerHTTPVerbGet;
	request.timeoutInterval = 10.0f;
	request.cachePolicy = NSURLRequestReloadIgnoringLocalAndRemoteCacheData;
	NSString *authHeader = [NSString stringWithFormat:@"Bearer %@", kYelpManagerAPIKey];
	[request setValue:authHeader forHTTPHeaderField:@"Authorization"];
	return request;
}

#pragma mark - Helpers: Fetching

- (id)fetchDataResultForRequest:(NSURLRequest *)request
						  delay:(NSTimeInterval)delay
					 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block
{
	return [self.thread performURLRequest:request completionQueue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0) delay:delay completion:block];
}

- (id)fetchDataResultForRequest:(NSURLRequest *)request
					 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block
{
	return [self fetchDataResultForRequest:request delay:0.0 completion:block];
}

- (id)fetchJSONResultForRequest:(NSURLRequest *)request
						  delay:(NSTimeInterval)delay
					   completion:(void (^)(id JSONObject, NSError *error))block
{
	return [self fetchDataResultForRequest:request
									 delay:delay
								completion:^(NSURLResponse *response, NSData *data, NSError *error) {
		id JSONObjectOut = nil;
		NSError *errorOut = nil;
									
		if (data) {
			NSError *deserializationError = nil;
			id JSONObject = ^(NSError * __autoreleasing *errorOut){
				id res = nil;
				NSError *error = nil;
				
				res = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
				
				if (errorOut) {
					*errorOut = error;
				}
				return res;
			}(&deserializationError);
			if (JSONObject) {
				JSONObjectOut = JSONObject;
			} else {
				errorOut = deserializationError;
			}
		} else {
			JSONObjectOut = nil;
			errorOut = error;
		}
		
		if (block) {
			dispatch_async(dispatch_get_main_queue(), ^{
				block(JSONObjectOut, errorOut);
			});
		}
	}];
}

- (id)JSONResultForSynchronousRequest:(NSURLRequest *)req error:(NSError * __autoreleasing *)errorOut
{
	NSError *error = nil;
	id res = nil;

	NSURLResponse *response = nil;
	NSError *sendError = nil;
	NSData *data = [NSURLConnection sendSynchronousRequest:req returningResponse:&response error:&error];
	if (data) {
		NSError *deserializationError = nil;
		id JSONObject = [NSJSONSerialization JSONObjectWithData:data options:0 error:&deserializationError];
		if (JSONObject) {
			res = JSONObject;
		} else {
			error = deserializationError;
		}
	} else {
		error = sendError;
	}
	
	if (errorOut) {
		*errorOut = error;
	}
	return res;
}

#pragma mark - Public API

- (void)requestWhenInUseAuthorization {
	self.locationManager = [[CLLocationManager alloc] init];
	[self.locationManager requestWhenInUseAuthorization];
}

- (id)fetchJSONResultForSearchWithDictionary:(NSDictionary *)dictionary
									   delay:(NSTimeInterval)delay
									completion:(void (^)(id JSONObject, NSError *error))block
{
	NSURLRequest *request = [self requestForSearchWithDictionary:dictionary];
	return [self fetchJSONResultForRequest:request
									 delay:delay
								completion:block];
}

- (id)fetchJSONResultForSearchWithDictionary:(NSDictionary *)dictionary
								  completion:(void (^)(id JSONObject, NSError *error))block
{
	return [self fetchJSONResultForSearchWithDictionary:dictionary delay:0.0 completion:block];
}

- (id)JSONResultForSynchronousSearchWithDictionary:(NSDictionary *)dictionary error:(NSError * __autoreleasing *)errorOut
{
	NSError *error = nil;
	id res = nil;
	
	NSURLRequest *req = [self requestForSearchWithDictionary:dictionary];
	res = [self JSONResultForSynchronousRequest:req error:&error];
	
	if (errorOut) {
		*errorOut = error;
	}
	return res;
}

- (id)JSONResultForSynchronousPhoneSearchWithDictionary:(NSDictionary *)dictionary error:(NSError * __autoreleasing *)errorOut
{
	NSError *error = nil;
	id res = nil;
	
	NSURLRequest *req = [self requestForSearchWithDictionary:dictionary];
	res = [self JSONResultForSynchronousRequest:req error:&error];
	
	if (errorOut) {
		*errorOut = error;
	}
	return res;
}

- (BOOL)cancelFetchWithID:(id)theID
{
	return [self.thread cancelURLRequestWithIdentifier:theID];
}

- (id)fetchResultForSearchWithDictionary:(NSDictionary *)dictionary
								   delay:(NSTimeInterval)delay
								completion:(void (^)(YelpRegion *region, NSUInteger resultCount, NSArray<YelpBusiness *> *businesses, NSError *error))block
{
	NSDictionary *enhancedDictionary = [self enhancedDictionaryWithLocation:dictionary];
	return [self fetchJSONResultForSearchWithDictionary:enhancedDictionary
												  delay:delay
											 completion:^(id JSONObject, NSError *error) {
												 YelpRegion *regionOut = nil;
												 NSUInteger resultCountOut = 0;
												 NSArray<YelpBusiness *> *businessesOut = nil;
												 NSError *errorOut = nil;
												 
												 if (JSONObject) {
													 NSDictionary *JSONDictionary = (NSDictionary *)JSONObject;
													 
													 NSDictionary *errorDictionary = JSONDictionary[kYelpManagerSearchResultKeyError];
													 if (!errorDictionary) {													 
														 regionOut = [YelpRegion regionWithJSONDictionary:JSONDictionary[kYelpManagerSearchResultKeyRegion]];
														 resultCountOut = [JSONDictionary[kYelpManagerSearchResultKeyTotal] integerValue];
														 businessesOut = ^{
															 NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
															 
															 NSArray<YelpBusiness *> *businessDictionaries = JSONDictionary[kYelpManagerSearchResultKeyBusinesses];
															 for (NSDictionary *businessDictionary in businessDictionaries) {
																 YelpBusiness *business = [YelpBusiness businessWithJSONDictionary:businessDictionary];
																 [mutableRes addObject:business];
															 }
															 
															 return [mutableRes copy];
														 }();
													 } else {
														 NSLog(@"Yelp result error: %@", errorDictionary.debugDescription);
														 NSError *error = [NSError
																		   errorWithDomain:@"YelpFusion"
																		   code:-1
																		   userInfo:@{ NSLocalizedDescriptionKey: errorDictionary[@"description"] ?: @"An error occurred",
																					   @"response": errorDictionary }];
														 errorOut = error;
													 }
												 }
												 
												 if (block) {
													 block(regionOut, resultCountOut, businessesOut, errorOut);
												 }
													 
											 }];
}

- (id)fetchResultForSearchWithDictionary:(NSDictionary *)dictionary
							  completion:(void (^)(YelpRegion *region, NSUInteger resultCount, NSArray<YelpBusiness *> *businesses, NSError *error))block
{
	return [self fetchResultForSearchWithDictionary:dictionary delay:0.0 completion:block];
}

- (void)startUpdatingLocation
{
#if APPLICATION == APP_NTOUCH_IOS // method not implemented on Mac OSX
	if([self.locationManager respondsToSelector:@selector(requestWhenInUseAuthorization)])
	{
		[self.locationManager requestWhenInUseAuthorization];
	}
#endif
	[self.locationManager startUpdatingLocation];
}

- (void)stopUpdatingLocation
{
	[self.locationManager stopUpdatingLocation];
}

#pragma mark - CLLocationManagerDelegater

- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
	switch (status) {
		case kCLAuthorizationStatusAuthorized:
			NSLog(@"Location Services are now Authorized");
			self.locationManagerAuthorizationStatus = kCLAuthorizationStatusAuthorized;
			break;
			
		case kCLAuthorizationStatusDenied:
			NSLog(@"Location Services are now Denied");
			self.locationManagerAuthorizationStatus = kCLAuthorizationStatusDenied;
			break;
			
		case kCLAuthorizationStatusNotDetermined:
			NSLog(@"Location Services are now Not Determined");
			self.locationManagerAuthorizationStatus = kCLAuthorizationStatusNotDetermined;
			break;
			
		case kCLAuthorizationStatusRestricted:
			NSLog(@"Location Services are now Restricted");
			self.locationManagerAuthorizationStatus = kCLAuthorizationStatusRestricted;
			break;
			
		default:
			NSLog(@"Location Services are %d", status);
			self.locationManagerAuthorizationStatus = status;
			break;
	}
}

- (void)locationManager:(CLLocationManager*)manager didFailWithError:(NSError*)error
{
	NSLog(@"locationManager::didFailWithError %@", [error localizedDescription]);
}

@end
