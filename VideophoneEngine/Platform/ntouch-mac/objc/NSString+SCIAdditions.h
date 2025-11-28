//
//  NSString+SCIAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 9/11/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (SCIAdditions)

+ (instancetype)sci_stringWithWideCharacter:(unichar)character ofLength:(NSUInteger)length;
+ (instancetype)sci_stringWithCharacter:(unichar)character ofLength:(NSUInteger)length;

- (NSComparisonResult)sci_compareAlphaFirst:(NSString *)other;

- (NSArray *)sci_componentsOfLengthAtMost:(NSUInteger)maxLength;

@end
