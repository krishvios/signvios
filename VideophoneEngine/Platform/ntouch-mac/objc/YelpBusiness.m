//
//  YelpBusiness.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/2/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "YelpBusiness.h"
#import "YelpCategory.h"

static NSString * const kYelpBusinessKeyCategories = @"categories";
static NSString * const kYelpBusinessKeyDisplayPhone = @"display_phone";
static NSString * const kYelpBusinessKeyIdentifier = @"id";
static NSString * const kYelpBusinessKeyImageURL = @"image_url";
static NSString * const kYelpBusinessKeyIsClaimed = @"is_claimed";
static NSString * const kYelpBusinessKeyIsClosed = @"is_closed";
static NSString * const kYelpBusinessKeyLocation = @"location";
static NSString * const kYelpBusinessKeyDistance = @"distance";
static NSString * const kYelpBusinessKeyLocationAddress1 = @"address1";
static NSString * const kYelpBusinessKeyLocationAddress2 = @"address2";
static NSString * const kYelpBusinessKeyLocationAddress3 = @"address3";
static NSString * const kYelpBusinessKeyLocationCity = @"city";
static NSString * const kYelpBusinessKeyLocationCountryCode = @"country";
static NSString * const kYelpBusinessKeyLocationDisplayAddress = @"display_address";
static NSString * const kYelpBusinessKeyLocationPostalCode = @"zip_code";
static NSString * const kYelpBusinessKeyLocationStateCode = @"state";
static NSString * const kYelpBusinessKeyURL = @"url";
static NSString * const kYelpBusinessKeyName = @"name";
static NSString * const kYelpBusinessKeyPhone = @"phone";
static NSString * const kYelpBusinessKeyRating = @"rating";
static NSString * const kYelpBusinessKeyReviewCount = @"review_count";

@implementation YelpBusiness

+ (instancetype)businessWithJSONDictionary:(NSDictionary *)dictionary
{
	return [[self alloc] initWithJSONDictionary:dictionary];
}

- (id)initWithJSONDictionary:(NSDictionary *)dictionary
{
	self = [super init];
	if (self) {
		self.categories = ^{
			NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
			
			NSArray *categoryArrays = dictionary[kYelpBusinessKeyCategories];
			for (NSDictionary *categories in categoryArrays) {
				YelpCategory *category = [[YelpCategory alloc] initWithJSONCategories:categories];
				[mutableRes addObject:category];
			}
			
			return [mutableRes copy];
		}();
		self.displayPhone = dictionary[kYelpBusinessKeyDisplayPhone];
		self.identifier = dictionary[kYelpBusinessKeyIdentifier];
		self.imageURL = dictionary[kYelpBusinessKeyImageURL];
		self.isClaimed = [(NSString *)dictionary[kYelpBusinessKeyIsClaimed] boolValue];
		self.isClosed = [(NSString *)dictionary[kYelpBusinessKeyIsClosed] boolValue];
		self.location = dictionary[kYelpBusinessKeyLocation];
		if(dictionary[kYelpBusinessKeyDistance])
		{
			self.distance = [dictionary[kYelpBusinessKeyDistance] doubleValue];
		}
		else
		{
			self.distance = -1;
		}
		self.URL = dictionary[kYelpBusinessKeyURL];
		self.name = dictionary[kYelpBusinessKeyName];
		self.phone = dictionary[kYelpBusinessKeyPhone];
		self.rating = YelpRatingForStringValue([dictionary[kYelpBusinessKeyRating] stringValue]);
		self.reviewCount = [(NSString *)dictionary[kYelpBusinessKeyReviewCount] integerValue];
	}
	return self;
}

-(NSString*)getCategoryString
{
	NSString *categoryString = @"";
	for(int i = 0; i < [self.categories count]; i++)
	{
		YelpCategory *category = [self.categories objectAtIndex:i];
		if([categoryString isEqualToString:@""])
			categoryString = category.name;
		else
			categoryString = [NSString stringWithFormat:@"%@, %@", categoryString, category.name];
	}
	return categoryString;
}

static YelpRating YelpRatingForStringValue(NSString *value)
{
	YelpRating res = YelpRatingUnknown;
	if ([value isEqualToString:@"1"]) {
		res = YelpRatingOne;
	} else if ([value isEqualToString:@"1.5"]) {
		res = YelpRatingOneHalf;
	} else if ([value isEqualToString:@"2"]) {
		res = YelpRatingTwo;
	} else if ([value isEqualToString:@"2.5"]) {
		res = YelpRatingTwoHalf;
	} else if ([value isEqualToString:@"3"]) {
		res = YelpRatingThree;
	} else if ([value isEqualToString:@"3.5"]) {
		res = YelpRatingThreeHalf;
	} else if ([value isEqualToString:@"4"]) {
		res = YelpRatingFour;
	} else if ([value isEqualToString:@"4.5"]) {
		res = YelpRatingFourHalf;
	} else if ([value isEqualToString:@"5"]) {
		res = YelpRatingFive;
	} else {
		res = YelpRatingUnknown;
	}
	return res;
}

@end
