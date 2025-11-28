//
//  CGRect+BNRAdditions.m
//  ntouchMac
//
//  Created by Nate Chandler on 4/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "CGRect+BNRAdditions.h"

CGRect CGRectScaledCenteredInsideRect(CGRect sourceRect, CGRect targetRect) 
{
	return CGRectScaledFavoringXSideAndYSideInRect(sourceRect, targetRect, CGRectMedXEdge, CGRectMedYEdge);
}

CGRect CGRectScaledFavoringXSideAndYSideInRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xEdge, CGRectEdge yEdge)
{
	if (sourceRect.size.width == 0 || sourceRect.size.height == 0) return targetRect;
	if (CGRectIsEmpty(targetRect))
		return CGRectZero;
	
	CGFloat width_source = sourceRect.size.width;
	CGFloat height_source = sourceRect.size.height;
	
	CGFloat width_target = targetRect.size.width;
	CGFloat height_target = targetRect.size.height;
	CGFloat x_target = targetRect.origin.x;
	CGFloat y_target = targetRect.origin.y;
	
	CGFloat width_embedded = 0.0;
	CGFloat height_embedded = 0.0;
	CGFloat x_embedded = x_target;
	CGFloat y_embedded = y_target;
	
	CGFloat ratio_source = height_source / width_source;
	CGFloat ratio_target = height_target / width_target;
	
	if (ratio_target > ratio_source) {
		width_embedded = width_target;
		height_embedded = (width_target * height_source) / width_source;
		x_embedded += 0;
		switch (yEdge) {
			case CGRectMinYEdge: {
				y_embedded += 0;
			} break;
			case CGRectMaxYEdge: {
				y_embedded += ceilf(height_target - height_embedded);
			} break;
			case CGRectMaxXEdge:
			case CGRectMinXEdge: {
			} break;
		}
	} else if (ratio_target == ratio_source) {
		width_embedded = width_target;
		height_embedded = height_target;
		x_embedded += 0;
		y_embedded += 0;
	} else if (ratio_target < ratio_source) {
		width_embedded = (height_target * width_source) / height_source;
		height_embedded = height_target;
		switch (xEdge) {
			case CGRectMinXEdge: {
				x_embedded += 0;
			} break;
			case CGRectMaxXEdge: {
				x_embedded += ceilf(width_target - width_embedded);
			} break;
			case CGRectMaxYEdge:
			case CGRectMinYEdge: {
			} break;
		}
	}
	
	width_embedded = floorf(width_embedded);
	height_embedded = floorf(height_embedded);
	x_embedded = floorf(x_embedded);
	y_embedded = floorf(y_embedded);
	
	CGRect embeddedRect = CGRectMake(x_embedded, y_embedded, width_embedded, height_embedded);
	
	return embeddedRect;
}

CGRect CGRectAlignedToXSideAndYSideInRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xEdge, CGRectEdge yEdge)
{
	CGFloat width_source = sourceRect.size.width;
	CGFloat height_source = sourceRect.size.height;
	
	CGFloat width_target = targetRect.size.width;
	CGFloat height_target = targetRect.size.height;
	CGFloat x_target = targetRect.origin.x;
	CGFloat y_target = targetRect.origin.y;
	
	CGFloat width_aligned = 0.0f;
	CGFloat height_aligned = 0.0f;
	CGFloat x_aligned = 0.0f;
	CGFloat y_aligned = 0.0f;
	
	width_aligned = sourceRect.size.width;
	height_aligned = sourceRect.size.height;
	
	switch (xEdge) {
		case CGRectMinXEdge: {
			x_aligned = x_target;
		} break;
		case CGRectMaxXEdge: {
			x_aligned = x_target + width_target - width_source;
		} break;
		case CGRectMaxYEdge:
		case CGRectMinYEdge: {
		} break;
	}
	
	switch (yEdge) {
		case CGRectMinYEdge: {
			y_aligned = y_target;
		} break;
		case CGRectMaxYEdge: {
			y_aligned = y_target + height_target - height_source;
		} break;
		case CGRectMaxXEdge:
		case CGRectMinXEdge: {
		} break;
	}
	
	CGRect alignedRect = CGRectMake(x_aligned, y_aligned, width_aligned, height_aligned);
	
	return alignedRect;
}

CGRect CGRectAlignedToXSideAndYSideInsideRect(CGRect sourceRect, CGRect targetRect, CGRectEdge xEdge, CGRectEdge yEdge)
{
	CGFloat width_source = sourceRect.size.width;
	CGFloat height_source = sourceRect.size.height;
	
	CGFloat width_target = targetRect.size.width;
	CGFloat height_target = targetRect.size.height;
	CGFloat x_target = targetRect.origin.x;
	CGFloat y_target = targetRect.origin.y;
	
	CGFloat width_aligned = 0.0f;
	CGFloat height_aligned = 0.0f;
	CGFloat x_aligned = 0.0f;
	CGFloat y_aligned = 0.0f;
	
	width_aligned = (sourceRect.size.width > targetRect.size.width) ? width_target : width_source;
	height_aligned = (sourceRect.size.height > targetRect.size.height) ? height_target : height_source;
	
	switch (xEdge) {
		case CGRectMinXEdge: {
			x_aligned = x_target;
		} break;
		case CGRectMaxXEdge: {
			x_aligned = x_target + width_target - width_aligned;
		} break;
		case CGRectMaxYEdge:
		case CGRectMinYEdge: {
		} break;
	}
	
	switch (yEdge) {
		case CGRectMinYEdge: {
			y_aligned = y_target;
		} break;
		case CGRectMaxYEdge: {
			y_aligned = y_target + height_target - height_aligned;
		} break;
		case CGRectMaxXEdge:
		case CGRectMinXEdge: {
		} break;
	}
	
	CGRect alignedRect = CGRectMake(x_aligned, y_aligned, width_aligned, height_aligned);
	
	return alignedRect;
}

CGRect CGRectInsetFromRect(CGRect outerRect, CGFloat leftInset, CGFloat topInset, CGFloat rightInset, CGFloat bottomInset)
{
	return CGRectMake(outerRect.origin.x + leftInset,
					  outerRect.origin.y + bottomInset, 
					  outerRect.size.width - (leftInset + rightInset), 
					  outerRect.size.height - (topInset + bottomInset));
}

CGRect CenteredFramePreservingAspectRatio(CGSize containerSize, CGSize movieSize)
{
	CGSize movieViewSize;
	
	if (movieSize.width/movieSize.height < containerSize.width/containerSize.height)
	{
		movieViewSize = CGSizeMake(containerSize.height * movieSize.width / movieSize.height, containerSize.height);
	}
	else
	{
		movieViewSize = CGSizeMake(containerSize.width, containerSize.width * movieSize.height / movieSize.width);
	}
	
	return CGRectMake(containerSize.width/2.0 - movieViewSize.width/2.0,
					  containerSize.height/2.0 - movieViewSize.height/2.0,
					  movieViewSize.width,
					  movieViewSize.height);
}

CGRect CGRectAroundPoint(CGPoint point, CGFloat radius)
{
	return CGRectMake(point.x - radius, point.y - radius, radius * 2, radius * 2);
}

CGRect CGRectOfSizeCenteredAroundRect(CGRect rect, CGSize size)
{
	CGRect res;
	res.size = size;
	res.origin = CGPointMake(rect.origin.x + (rect.size.width - size.width) / 2.0f,
							 rect.origin.y + (rect.size.height - size.height) / 2.0f);
	return res;
}

