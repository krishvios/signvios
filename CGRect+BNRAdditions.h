//
//  CGRect+BNRAdditions.h
//  ntouchMac
//
//  Created by Nate Chandler on 4/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

#define CGRectMedXEdge (4)
#define CGRectMedYEdge (5)

CGRect CGRectScaledCenteredInsideRect(CGRect sourceRect, CGRect targetRect);
CGRect CGRectScaledFavoringXSideAndYSideInRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xSide, CGRectEdge ySide);
CGRect CGRectAlignedToXSideAndYSideInRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xSide, CGRectEdge ySide);
CGRect CGRectAlignedToXSideAndYSideInsideRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xSide, CGRectEdge ySide);
CGRect CGRectInsetFromRect(CGRect outerRect, CGFloat leftInset, CGFloat topInset, CGFloat rightInset, CGFloat bottomInset);
CGRect CenteredFramePreservingAspectRatio(CGSize containerSize, CGSize movieSize);
CGRect CGRectAroundPoint(CGPoint point, CGFloat radius);
CGRect CGRectOfSizeCenteredAroundRect(CGRect rect, CGSize size);

