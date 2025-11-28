//
//  NSMutableString+SCIAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/26/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "NSMutableString+SCIAdditions.h"

@implementation NSMutableString (SCIAdditions)

- (void)sci_appendDeletingBackspaces:(NSString *)string
{
	NSUInteger length = string.length;
	for (NSUInteger index = 0; index < length; index++) {
		unichar character = [string characterAtIndex:index];
		switch (character) {
			case 8: {//backspace
				if (self.length > 0) {
					[self deleteCharactersInRange:NSMakeRange(self.length - 1, 1)];
				}
			} break;
			default: {
				[self appendFormat:@"%C", character];
			} break;
		}
	}
}

@end
