//
//  NSThread+PerformBlock.h
//  YelpDemo
//
//  Created by Nate Chandler on 6/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSThread (PerformBlock)
- (void)performBlock:(void (^)())block;
- (void)performBlock:(void (^)())block waitUntilDone:(BOOL)wait;
@end
