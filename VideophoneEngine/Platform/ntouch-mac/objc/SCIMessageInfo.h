//
//  SCIMessageInfo.h
//  ntouchMac
//
//  Created by Adam Preble on 5/9/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SCIItemId;

@interface SCIMessageInfo : NSObject <NSCopying>

@property (nonatomic) SCIItemId *messageId;
@property (nonatomic) SCIItemId *categoryId;
@property (nonatomic) NSString *viewId;
@property (nonatomic) NSString *name;
@property (nonatomic) NSDate *date;
@property (nonatomic) NSTimeInterval duration;
@property (nonatomic) BOOL viewed;
@property (nonatomic) NSString *dialString;
@property (nonatomic) NSString *interpreterId;
@property (nonatomic) NSString *previewImageURL;
@property (nonatomic) NSTimeInterval pausePoint;
@property (nonatomic) NSUInteger typeId;

@end
