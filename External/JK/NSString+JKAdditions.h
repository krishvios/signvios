//
//  NSString+JKAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>


//Source: https://github.com/jerrykrinock/CategoriesObjC/blob/master/NSString%2BTruncate.m
@interface NSString (JKAdditions)
- (NSAttributedString *)jk_attributedStringWithTruncationStyle:(NSLineBreakMode)truncationStyle;
- (NSString *)jk_stringByTruncatingMiddleToLength:(NSUInteger)limit
								   wholeWords:(BOOL)wholeWords;
- (NSString *)jk_stringByTruncatingEndToLength:(NSUInteger)limit
								wholeWords:(BOOL)wholeWords;
@end
