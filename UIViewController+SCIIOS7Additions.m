//
//  UIViewController+SCIIOS7Additions.m
//  ntouch
//
//  Created by Nate Chandler on 10/22/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "UIViewController+SCIIOS7Additions.h"
#import "NSObject+ELLAdditions.h"
#import "UIGeometry+SCIAdditions.h"
#import "IOS7Utilities.h"

@implementation UIViewController (SCIIOS7Additions)

- (void)sci_setNoEdgesForExtendedLayout
{
	if ([self respondsToSelector:@selector(setEdgesForExtendedLayout:)]) {
#if SCI_IOS_SDK_IS_AT_LEAST_IPHONE_7_0
		self.edgesForExtendedLayout = UIRectEdgeNone;
#else
		SCIUIRectEdge rectEdge = SCIUIRectEdgeNone;
		[self ell_performSelector:@selector(setEdgesForExtendedLayout:) withValue:&rectEdge];
#endif
	}
}

@end
