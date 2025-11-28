//
//  YelpBusiness.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/2/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "YelpCategory.h"

typedef enum {
	YelpRatingUnknown,
	YelpRatingOne,
	YelpRatingOneHalf,
	YelpRatingTwo,
	YelpRatingTwoHalf,
	YelpRatingThree,
	YelpRatingThreeHalf,
	YelpRatingFour,
	YelpRatingFourHalf,
	YelpRatingFive
} YelpRating;

@interface YelpBusiness : NSObject
@property (nonatomic) NSArray<YelpCategory *> *categories;
@property (nonatomic) NSString *displayPhone;
@property (nonatomic) NSString *identifier;
@property (nonatomic) NSString *imageURL;
@property (nonatomic) BOOL isClaimed;
@property (nonatomic) BOOL isClosed;
//location
@property (nonatomic) NSDictionary<NSString *, id> *location;
@property (nonatomic) double distance;
@property (nonatomic) NSString *URL;
@property (nonatomic) NSString *name;
@property (nonatomic) NSString *phone;
@property (nonatomic) YelpRating rating;
@property (nonatomic) NSUInteger reviewCount;

-(NSString*)getCategoryString;
//deals
//gift certificates

+ (instancetype)businessWithJSONDictionary:(NSDictionary *)dictionary;
- (id)initWithJSONDictionary:(NSDictionary *)dictionary;
@end
