//
//  YelpCategory.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YelpCategory : NSObject
@property (nonatomic) NSString *name;
@property (nonatomic) NSString *alias;

- (id)initWithJSONCategories:(NSDictionary *)categories;

@end
