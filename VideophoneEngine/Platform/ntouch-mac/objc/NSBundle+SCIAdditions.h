//
//  NSBundle+SCIAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 9/13/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSBundle (SCIAdditions)
@property (nonatomic, readonly) NSString *sci_shortVersionString;
@property (nonatomic, readonly) NSString *sci_bundleVersion;

@end
