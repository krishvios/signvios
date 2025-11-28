//
//  WatchSignMailItem.h
//  ntouch
//
//  Created by Kevin Selman on 6/26/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface WatchSignMailItem : NSObject

@property (nonatomic, strong) NSString *callerName;
@property (nonatomic, strong) NSDate *callTime;
@property (nonatomic, strong) NSURL *videoURL;
@property (nonatomic, strong) NSString *viewId;
@property (nonatomic, strong) NSString *messageId;
@property (nonatomic) NSTimeInterval pausePoint;


- (instancetype)initWithDictionary:(NSDictionary *)dictionary;

@end
