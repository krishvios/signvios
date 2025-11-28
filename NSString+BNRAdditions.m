//
//  NSString+BNRAdditions.m
//  SideBarDemo
//
//  Created by Nate Chandler on 2/15/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import "NSString+BNRAdditions.h"
#import "NSString+JKAdditions.h"

@implementation NSString (BNRAdditions)
- (void)drawCenteredInRect:(CGRect)rect withAttributes:(NSDictionary *)attrs
{
    CGSize size = [self sizeWithAttributes:attrs];
    CGFloat x, y, w, h;
    x = rect.origin.x + (rect.size.width - size.width) / 2;
    y = rect.origin.y + (rect.size.height - size.height) / 2;
    w = size.width;
    h = size.height;
    
    CGRect rectToDrawIn = CGRectMake(x, y, w, h);
    [self drawInRect:rectToDrawIn withAttributes:attrs];
}

- (void)drawCenteredHorizontallyInsideRect:(CGRect)rect withAttributes:(NSDictionary *)attrs
{
	CGSize size = [self sizeWithAttributes:attrs];
	if (size.width > rect.size.width) {
		CGFloat x, y, w, h;
		x = rect.origin.x;
		y = rect.origin.y + (rect.size.height - size.height) / 2;
		w = rect.size.width;
		h = size.height;
		CGRect rectToDrawIn = CGRectMake(x, y, w, h);
		NSStringDrawingContext* con = [[NSStringDrawingContext alloc] init];
		[self drawWithRect:rectToDrawIn options:NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesLineFragmentOrigin attributes:attrs context:con];
	} else {
		[self drawCenteredInRect:rect withAttributes:attrs];
	}
}

- (void)drawCenteredAboutPoint:(CGPoint)point withAttributes:(NSDictionary *)attrs
{
	CGSize size = [self sizeWithAttributes:attrs];
	CGFloat x, y;
	x = point.x - size.width/2;
	y = point.y - size.height/2;
	CGPoint targetPoint = CGPointMake(x, y);
	[self drawAtPoint:targetPoint withAttributes:attrs];
}

- (void)drawLeftJustifiedAndVerticallyCenteredInRect:(CGRect)rect withInset:(CGFloat)leftInset attributes:(NSDictionary *)attrs
{
    CGSize size = [self sizeWithAttributes:attrs];
    CGFloat x, y, w, h;
    x = floorf(rect.origin.x + leftInset);
    y = floorf(rect.origin.y + (rect.size.height - size.height) / 2);
    w = rect.size.width;
    h = size.height;
    
    CGRect rectToDrawIn = CGRectMake(x, y, w, h);
	NSStringDrawingContext* con = [[NSStringDrawingContext alloc] init];
	[self drawWithRect:rectToDrawIn options:NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesLineFragmentOrigin attributes:attrs context:con];
}

- (BOOL)containsString:(NSString *)string
{
	BOOL res = YES;
	if (string && string.length > 0) {
		res = ([self rangeOfString:string].location != NSNotFound);
	}
	return res;
}

- (BOOL)bnr_containsCharacters:(NSCharacterSet *)characters options:(NSStringCompareOptions)options range:(NSRange)range
{
	return [self rangeOfCharacterFromSet:characters options:options range:range].location != NSNotFound;
}

- (NSRange)bnr_totalRange
{
	return NSMakeRange(0, self.length);
}

- (NSString *)bnr_stringByTruncatingToLength:(NSUInteger)length
{
	NSString *res = nil;
	
	if (length < self.length) {
		res = [self substringWithRange:NSMakeRange(0, length)];
	} else {
		res = self;
	}
	
	return res;
}

- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet
{
	return [self bnr_rangesOfCharactersFromSet:characterSet options:0];
}

- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet options:(NSStringCompareOptions)options
{
	return [self bnr_rangesOfCharactersFromSet:characterSet options:options range:self.bnr_totalRange];
}

- (NSArray *)bnr_rangesOfCharactersFromSet:(NSCharacterSet *)characterSet options:(NSStringCompareOptions)options range:(NSRange)range
{
	NSMutableArray *mutableRes = [NSMutableArray array];
	
	NSUInteger totalLength = range.length;
	NSString *current = self;
	NSRange currentRange = range;
	
	NSRange characterRange;
	while ((characterRange = [current rangeOfCharacterFromSet:characterSet options:options range:currentRange]).location != NSNotFound) {
		[mutableRes addObject:[NSValue valueWithRange:characterRange]];
		currentRange = NSMakeRange(characterRange.location + characterRange.length, totalLength - (characterRange.location + characterRange.length));
	}
	
	return [mutableRes copy];
}

- (NSString *)bnr_stringByTruncatingToWithinLength:(NSUInteger)length keepingTogetherSubstringsSeparatedByCharacters:(NSCharacterSet *)characters unlessOmittedLengthExceeds:(NSUInteger)maxOmission
{
	NSString *res = nil;
	
	NSString *totalTruncation = [self bnr_stringByTruncatingToLength:length];
	
	NSArray *ranges = [totalTruncation bnr_rangesOfCharactersFromSet:characters];
	if (ranges.count > 0) {
		NSRange range = [(NSValue *)ranges.lastObject rangeValue];
		
		NSString *subsequent = [self bnr_substringAfterRange:range];
		if (subsequent.length > maxOmission) {
			res = totalTruncation;
		} else {
			res = [totalTruncation bnr_substringBeforeRange:range];
		}
	} else {
		res = totalTruncation;
	}
	
	return res;
}

- (NSString *)bnr_stringByTruncatingToFirstCharacterInSet:(NSCharacterSet *)characterSet
{
	NSRange range = [self rangeOfCharacterFromSet:characterSet];
	return [self substringToIndex:range.location];
}

- (NSString *)bnr_stringByTruncatingToFirstNewline
{
	return [self bnr_stringByTruncatingToFirstCharacterInSet:[NSCharacterSet newlineCharacterSet]];
}

- (NSString *)bnr_stringByTruncatingToWithinWidth:(CGFloat)width lineBreakMode:(NSLineBreakMode)mode forAttributes:(NSDictionary *)attrs
{
	NSString *res = nil;
	
	NSUInteger length = self.length;
	while ([[self substringToIndex:length] sizeWithAttributes:attrs].width > width) {
		length--;
	}
	switch (mode) {
		case NSLineBreakByTruncatingHead: {
			[NSException raise:@"Not implemented." format:@"%s has not been implemented for NSLineBreakByTruncatingHead", __PRETTY_FUNCTION__];
		} break;
		case NSLineBreakByTruncatingMiddle: {
			res = [self jk_stringByTruncatingMiddleToLength:length wholeWords:NO];
		} break;
		case NSLineBreakByTruncatingTail: {
			res = [self jk_stringByTruncatingEndToLength:length wholeWords:NO];
		} break;
		default:
			break;
	}
	
	return res;
}

- (NSString *)bnr_substringAfterRange:(NSRange)range
{
	NSUInteger length = self.length;
	NSRange targetRange = NSMakeRange(range.location + range.length, length - (range.location + range.length));
	return [self substringWithRange:targetRange];
}

- (NSString *)bnr_substringBeforeRange:(NSRange)range
{
	NSRange targetRange = NSMakeRange(0, range.location);
	return [self substringWithRange:targetRange];
}

+ (instancetype)bnr_stringWithUTF8String:(const char *)utf8
{
	NSString *res = nil;
	
	if (utf8 == NULL) {
		res = nil;
	} else {
		res = [[self class] stringWithUTF8String:utf8];
	}
	
	return res;
}

@end
