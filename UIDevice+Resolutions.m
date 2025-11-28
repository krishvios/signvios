//
//  UIDevice+Resolutions.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "UIDevice+Resolutions.h"

@implementation UIDevice (Resolutions)

- (UIDeviceResolution)resolution
{
	UIDeviceResolution resolution = UIDeviceResolution_Unknown;
	UIScreen *mainScreen = [UIScreen mainScreen];
	CGFloat scale = ([mainScreen respondsToSelector:@selector(scale)] ? mainScreen.scale : 1.0f);
	CGFloat pixelHeight = (CGRectGetHeight(mainScreen.bounds) * scale);
	
	if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone){
		if (scale == 3.0) {
			if (pixelHeight == 2208.0f || pixelHeight == 1242.0f)
				resolution = UIDeviceResolution_iPhoneRetina6Plus;
			else if (pixelHeight == 2436 || pixelHeight == 1125)
				resolution = UIDeviceResolution_iPhoneX;
			else if (pixelHeight == 2688.0f || pixelHeight == 1242.0f)
				resolution = UIDeviceResolution_iPhoneXSMax;
		}
		
		else if (scale == 2.0f) {
            if (pixelHeight == 960.0f || pixelHeight == 640.0f)
                resolution = UIDeviceResolution_iPhoneRetina35;
            else if (pixelHeight == 1136.0f || pixelHeight == 640.0f)
                resolution = UIDeviceResolution_iPhoneRetina4;
			else if (pixelHeight == 1334.0f)
				resolution = UIDeviceResolution_iPhoneRetina6;
			else if (pixelHeight == 1792.0f || pixelHeight == 828.0f)
				resolution = UIDeviceResolution_iPhoneXR;
		
        }
		
		else if (scale == 1.0f && pixelHeight == 480.0f)
            resolution = UIDeviceResolution_iPhoneStandard;
		
	} else {
        if (scale == 2.0f && pixelHeight == 2048.0f)
            resolution = UIDeviceResolution_iPadRetina;
		else if (scale = 2.0f && pixelHeight == 1536.0f)
			resolution = UIDeviceResolution_iPadRetina;
		else if (pixelHeight == 1024.0f)
            resolution = UIDeviceResolution_iPadStandard;
	}
	
	return resolution;
}

@end
