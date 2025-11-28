//
//  NSPoint+Additions.h
//  GrayscaleLightRingDemo
//
//  Created by Nate Chandler on 7/11/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import <Foundation/Foundation.h>

extern CGPoint CGOffsetPoint(CGPoint point, CGSize offset);
extern CGPoint CGSubtractPoints(CGPoint arg1, CGPoint arg2);
CGPoint CGAddPoints(CGPoint arg1, CGPoint arg2);
CGPoint CGScalePointUniformly(CGPoint point, CGFloat scale);
CGPoint CGScalePoint(CGPoint point, CGPoint scale);

