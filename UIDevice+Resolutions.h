//
//  UIDevice+Resolutions.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

enum {
    UIDeviceResolution_Unknown				= 0,
    UIDeviceResolution_iPhoneStandard		= 1,    // iPhone 1,3,3GS Standard Display		(320x480px)
    UIDeviceResolution_iPhoneRetina35		= 2,    // iPhone 4,4S Retina Display 3.5"		(640x960px)
    UIDeviceResolution_iPhoneRetina4		= 3,    // iPhone 5 Retina Display 4"			(640x1136px)
	UIDeviceResolution_iPhoneRetina6		= 4,	// iPhone 6 Retina Display 4.6"			(750x1334px)
	UIDeviceResolution_iPhoneRetina6Plus	= 5,	// iPhone 6 Plus Retina Display 5.7"	(1080x1920px)
    UIDeviceResolution_iPadStandard			= 6,    // iPad 1,2 Standard Display			(1024x768px)
    UIDeviceResolution_iPadRetina			= 7,    // iPad 3 Retina Display				(2048x1536px)
	UIDeviceResolution_iPhoneX				= 8,	// iPhone X								(2436x1125px)
	UIDeviceResolution_iPhoneXR				= 9,	// iPhone XR							(1792x828px)
	UIDeviceResolution_iPhoneXSMax			= 10	// iPhone XS Max						(2688x1242px)
	
}; typedef NSUInteger UIDeviceResolution;

@interface UIDevice (Resolutions)

- (UIDeviceResolution)resolution;

NSString *NSStringFromResolution(UIDeviceResolution resolution);

@end
