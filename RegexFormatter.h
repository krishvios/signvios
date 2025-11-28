//
//  RegexKitLiteFormatter.h
//  RegexKitLiteFormatterTest
//
//  Created by Nate Chandler on 3/27/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RegexFormatter : NSFormatter
@property (nonatomic, strong, readwrite) NSString *regex;
@property (nonatomic, strong, readwrite) NSString *fullStringRegex;
@property (nonatomic, strong, readwrite) NSString *partialStringRegex;
@end
