//
//  SCIContactViewControllerDisplayEntity.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/24/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol SCIContactDisplayable <NSObject>

@required
- (NSString *)title;
- (BOOL)isFixed;
- (BOOL)isHeader;
- (BOOL)isEqual:(id)obj;
- (NSString *)number;
- (NSString *)numberSearchString;
- (NSComparisonResult)compare:(id)other;

@property (nonatomic, readonly) NSString *displayedDetail;
@property (nonatomic, readonly) NSArray *recentCalls;

@end
