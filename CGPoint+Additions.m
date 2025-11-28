//
//  NSPoint+Additions.m
//  GrayscaleLightRingDemo
//
//  Created by Nate Chandler on 7/11/12.
//  Copyright (c) 2012 Big Nerd Ranch. All rights reserved.
//

#import "CGPoint+Additions.h"

CGPoint CGOffsetPoint(CGPoint point, CGSize offset)
{
	return CGPointMake(point.x + offset.width, point.y + offset.height);
}

CGPoint CGSubtractPoints(CGPoint arg1, CGPoint arg2)
{
	return CGPointMake(arg1.x - arg2.x, arg1.y - arg2.y);
}

CGPoint CGAddPoints(CGPoint arg1, CGPoint arg2)
{
	return CGPointMake(arg1.x + arg2.x, arg1.y + arg2.y);
}

CGPoint CGScalePointUniformly(CGPoint point, CGFloat scale)
{
	return CGPointMake(point.x * scale, point.y * scale);
}

CGPoint CGScalePoint(CGPoint point, CGPoint scale)
{
	return CGPointMake(point.x * scale.x, point.y * scale.y);
}
