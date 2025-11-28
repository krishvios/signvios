//
//  NSString+BNRAdditions.h
//  SideBarDemo
//
//  Created by Nate Chandler on 2/15/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (BNRAdditions)
- (void)drawCenteredInRect:(CGRect)rect withAttributes:(NSDictionary *)attrs;
- (void)drawCenteredHorizontallyInsideRect:(CGRect)rect withAttributes:(NSDictionary *)attrs;
- (void)drawCenteredAboutPoint:(CGPoint)point withAttributes:(NSDictionary *)attrs;
- (void)drawLeftJustifiedAndVerticallyCenteredInRect:(CGRect)rect withInset:(CGFloat)leftInset attributes:(NSDictionary *)attrs;
- (BOOL)containsString:(NSString *)string;

- (BOOL)bnr_containsCharacters:(NSCharacterSet *)characters options:(NSStringCompareOptions)options range:(NSRange)range;
- (NSString *)bnr_stringByTruncatingToLength:(NSUInteger)length;
- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet;
- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet options:(NSStringCompareOptions)options;
- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet options:(NSStringCompareOptions)options range:(NSRange)range;
- (NSString *)bnr_stringByTruncatingToWithinLength:(NSUInteger)length keepingTogetherSubstringsSeparatedByCharacters:(NSCharacterSet *)characters unlessOmittedLengthExceeds:(NSUInteger)maxOmission;
- (NSString *)bnr_stringByTruncatingToFirstCharacterInSet:(NSCharacterSet *)characterSet;
- (NSString *)bnr_stringByTruncatingToFirstNewline;
- (NSString *)bnr_stringByTruncatingToWithinWidth:(CGFloat)width lineBreakMode:(NSLineBreakMode)mode forAttributes:(NSDictionary *)attrs;
- (NSString *)bnr_substringAfterRange:(NSRange)range;
- (NSString *)bnr_substringBeforeRange:(NSRange)range;

// Returns nil if the parameter is NULL, unlike +stringWithUTF8String:, which throws an exception.
+ (instancetype)bnr_stringWithUTF8String:(const char *)utf8;

@end
