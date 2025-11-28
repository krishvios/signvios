//
//  WatchCallHistoryItem.h
//  ntouch
//
//  Created by Kevin Selman on 6/26/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface WatchCallHistoryItem : NSObject

@property (nonatomic, strong) NSString *callerName;
@property (nonatomic, strong) NSString *callTime;

- (instancetype)initWithDictionary:(NSDictionary *)dictionary;

@end
