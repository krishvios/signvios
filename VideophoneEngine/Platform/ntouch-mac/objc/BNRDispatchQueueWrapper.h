//
//  BNRDispatchQueueWrapper.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/10/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface BNRDispatchQueueWrapper : NSObject

+ (instancetype)wrapperForDispatchQueue:(dispatch_queue_t)queue;
- (id)initWithDispatchQueue:(dispatch_queue_t)queue;

@property (nonatomic, assign) dispatch_queue_t queue;

@end
