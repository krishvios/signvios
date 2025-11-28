//
//  NSArray+BNREnumeration.h
//  ntouchMac
//
//  Created by Nate Chandler on 1/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSArray (BNREnumeration)
- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock completion:(void (^)())completionBlock;
- (void)bnr_enumerateOnQueue:(dispatch_queue_t)queue continuation:(void (^)(id, NSUInteger, void (^)(void)))continuationBlock;
@end
