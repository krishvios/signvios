//
//  SCISignMailGreetingType.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

typedef NS_ENUM(NSUInteger, SCISignMailGreetingType)
{
	SCISignMailGreetingTypeNone = 0,
	SCISignMailGreetingTypeDefault = 1,
	SCISignMailGreetingTypePersonalVideoOnly = 2,
	SCISignMailGreetingTypePersonalVideoAndText = 3,
	SCISignMailGreetingTypePersonalTextOnly = 4,
	SCISignMailGreetingTypeUnknown = NSUIntegerMax,
};
