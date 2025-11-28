//
//  SCITextView.m
//  ntouch
//
//  Created by Kevin Selman on 2/21/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "SCITextView.h"

@implementation SCITextView

- (void)setText:(NSString *)text
{
    [super setText:text];
	
    if (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) {
        CGRect rect = [self.textContainer.layoutManager usedRectForTextContainer:self.textContainer];
        UIEdgeInsets inset = self.textContainerInset;
        self.contentSize = CGSizeMake(ceil(rect.size.width)  + inset.left + inset.right,
                                      ceil(rect.size.height) + inset.top  + inset.bottom);
    }
}

@end
