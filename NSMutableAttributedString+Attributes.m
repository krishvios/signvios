//
//  NSMutableAttributedString+Helper.m
//  ntouch
//
//  Created by Kevin Selman on 1/6/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import "NSMutableAttributedString+Attributes.h"

@interface NSString(MASAttributes)

-(NSRange)rangeOfStringNoCase:(NSString*)s;

@end

@implementation NSString(MASAttributes)

-(NSRange)rangeOfStringNoCase:(NSString*)s
{
	return  [self rangeOfString:s options:NSCaseInsensitiveSearch];
}

@end

@implementation NSMutableAttributedString (Attributes)

- (void)addColor:(UIColor *)color substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && color != nil) {
		[self addAttribute:NSForegroundColorAttributeName
					 value:color
					 range:range];
	}
}

- (void)addBackgroundColor:(UIColor *)color substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && color != nil) {
		[self addAttribute:NSBackgroundColorAttributeName
					 value:color
					 range:range];
	}
}

- (void)addUnderlineForSubstring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound) {
		[self addAttribute: NSUnderlineStyleAttributeName
					 value:@(NSUnderlineStyleSingle)
					 range:range];
	}
}

- (void)addStrikeThrough:(int)thickness substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound) {
		[self addAttribute: NSStrikethroughStyleAttributeName
					 value:@(thickness)
					 range:range];
	}
}

- (void)addShadowColor:(UIColor *)color width:(int)width height:(int)height radius:(int)radius substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && color != nil) {
		NSShadow *shadow = [[NSShadow alloc] init];
		[shadow setShadowColor:color];
		[shadow setShadowOffset:CGSizeMake (width, height)];
		[shadow setShadowBlurRadius:radius];
		
		[self addAttribute: NSShadowAttributeName
					 value:shadow
					 range:range];
	}
}

- (void)addFontWithName:(NSString *)fontName size:(int)fontSize substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && fontName != nil) {
		UIFont * font = [UIFont fontWithName:fontName size:fontSize];
		[self addAttribute: NSFontAttributeName
					 value:font
					 range:range];
	}
}

- (void)addFont:(UIFont *)font substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && font != nil) {
		[self addAttribute: NSFontAttributeName
					 value:font
					 range:range];
	}
}

- (void)addAlignment:(NSTextAlignment)alignment substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound) {
		NSMutableParagraphStyle* style=[[NSMutableParagraphStyle alloc]init];
		style.alignment = alignment;
		[self addAttribute: NSParagraphStyleAttributeName
					 value:style
					 range:range];
	}
}

- (void)addStrokeColor:(UIColor *)color thickness:(int)thickness substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound && color != nil) {
		[self addAttribute:NSStrokeColorAttributeName
					 value:color
					 range:range];
		[self addAttribute:NSStrokeWidthAttributeName
					 value:@(thickness)
					 range:range];
	}
}

- (void)addVerticalGlyph:(BOOL)glyph substring:(NSString *)substring {
	NSRange range = [self.string rangeOfStringNoCase:substring];
	if (range.location != NSNotFound) {
		[self addAttribute:NSForegroundColorAttributeName
					 value:@(glyph)
					 range:range];
	}
}

@end
