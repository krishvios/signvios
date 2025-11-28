//
//  NSObject+BNRAssociatedDictionary.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSObject (BNRAssociatedDictionary)
- (void)bnr_setAssociatedObject:(id)object forKey:(id<NSCopying>)key;
- (id)bnr_associatedObjectForKey:(id<NSCopying>)key;
- (id)bnr_associatedObjectGeneratedBy:(id (^)(void))generatorBlock forKey:(id<NSCopying>)key;
@end
