//
//  YelpRegion.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/2/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "YelpRegion.h"
#import "YelpRegion_Internal.h"

static NSString * const kYelpRegionKeyCenter;
static NSString * const kYelpRegionKeySpan;
static NSString * const kYelpRegionKeyCenterLatitude;
static NSString * const kYelpRegionKeyCenterLongitude;
static NSString * const kYelpRegionKeySpanLatitudeDelta;
static NSString * const kYelpRegionKeySpanLongitudeDelta;

@implementation YelpRegion

+ (instancetype)regionWithJSONDictionary:(NSDictionary *)dictionary
{
	return [[YelpRegion alloc] initWithJSONDictionary:dictionary];
}

- (id)initWithJSONDictionary:(NSDictionary *)dictionary
{
	self = [super init];
	if (self) {
		NSDictionary *centerDictionary = dictionary[kYelpRegionKeyCenter];
		NSString *latitudeString = centerDictionary[kYelpRegionKeyCenterLatitude];
		NSString *longitudeString = centerDictionary[kYelpRegionKeyCenterLongitude];

#if TARGET_OS_IPHONE
		self.center = CGPointMake(longitudeString.doubleValue, latitudeString.doubleValue);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		self.center = NSMakePoint(longitudeString.doubleValue, latitudeString.doubleValue);
#endif
		NSDictionary *spanDictionary = [dictionary objectForKey:kYelpRegionKeySpan];
		NSString *latitudeDeltaString = spanDictionary[kYelpRegionKeySpanLatitudeDelta];
		NSString *longitudeDeltaString = spanDictionary[kYelpRegionKeySpanLongitudeDelta];
		
#if TARGET_OS_IPHONE
		self.span = CGSizeMake(longitudeDeltaString.doubleValue, latitudeDeltaString.doubleValue);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		self.span = NSMakeSize(longitudeDeltaString.doubleValue, latitudeDeltaString.doubleValue);
#endif

	}
	return self;
}

@end

static NSString * const kYelpRegionKeyCenter = @"center";
static NSString * const kYelpRegionKeySpan = @"span";
static NSString * const kYelpRegionKeyCenterLatitude = @"latitude";
static NSString * const kYelpRegionKeyCenterLongitude = @"longitude";
static NSString * const kYelpRegionKeySpanLatitudeDelta = @"latitude_delta";
static NSString * const kYelpRegionKeySpanLongitudeDelta = @"longitude_delta";
