//
//  NSMutableString+SCIAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 7/26/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSMutableString (SCIAdditions)

- (void)sci_appendDeletingBackspaces:(NSString *)string;

@end
