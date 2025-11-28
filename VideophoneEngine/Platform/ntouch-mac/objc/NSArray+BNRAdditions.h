//
//  NSArray+BNRAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSArray (BNRAdditions)
- (NSArray *)bnr_arrayByRemovingFirstInstanceOfObject:(id)object;
- (NSArray *)bnr_arrayByRemovingObject:(id)object;
- (NSArray *)bnr_arrayByRemovingObjectAtIndex:(NSUInteger)index;
- (NSArray *)bnr_arrayWithDifferenceInArray:(NSArray *)subtractand;
- (NSUInteger)bnr_indexBeforeIndex:(NSUInteger)index wrapping:(BOOL)wrapping;
- (NSUInteger)bnr_indexAfterIndex:(NSUInteger)index wrapping:(BOOL)wrapping;
- (NSArray *)bnr_arrayByReplacingObjectAtIndex:(NSUInteger)index withObject:(id)object;
- (NSArray *)bnr_trucatedArrayWithAtMost:(NSInteger)maxNumberOfItems;
- (NSArray *)bnr_arrayByPerformingBlock:(id (^)(id object, NSUInteger index))block;
- (id)bnr_firstObject;
@end
