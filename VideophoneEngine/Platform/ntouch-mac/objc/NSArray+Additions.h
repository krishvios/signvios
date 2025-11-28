//
//  NSArray+Additions.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSArray (Additions)
- (NSArray *)filteredArrayUsingBlock:(BOOL (^)(id obj))block;
- (NSArray *)tailArray;
@end
