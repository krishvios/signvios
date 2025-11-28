//
//  SCIPersonalGreetingPreferences+SstiPersonalGreetingPreferences.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/6/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPersonalGreetingPreferences+SstiPersonalGreetingPreferences.h"
#import "SCISignMailGreetingInfo_Cpp.h"

@implementation SCIPersonalGreetingPreferences (SstiPersonalGreetingPreferences)

+ (instancetype)personalGreetingPreferencesWithSstiPersonalGreetingPreferences:(SstiPersonalGreetingPreferences *)sstiPersonalGreetingPreferences
{
	SCIPersonalGreetingPreferences *res = nil;
	
	if (sstiPersonalGreetingPreferences) {
		res = [[self alloc] init];
		res.greetingText = [NSString stringWithUTF8String:sstiPersonalGreetingPreferences->szGreetingText.c_str()];
		res.greetingEnabled = (sstiPersonalGreetingPreferences->bGreetingEnabled == estiTRUE) ? YES : NO;
		res.greetingType = SCISignMailGreetingTypeFromESignMailGreetingType(sstiPersonalGreetingPreferences->eGreetingType);
		res.personalType = SCISignMailGreetingTypeFromESignMailGreetingType(sstiPersonalGreetingPreferences->ePersonalType);
	}
	
	return res;
}

- (SstiPersonalGreetingPreferences)createSstiPersonalGreetingPreferences
{
	SstiPersonalGreetingPreferences res;
	
	const char *pszGreetingText = self.greetingText.UTF8String;
	res.szGreetingText = (pszGreetingText) ? std::string(pszGreetingText) : std::string();
	res.bGreetingEnabled = (self.greetingEnabled) ? estiTRUE : estiFALSE;
	res.eGreetingType = ESignMailGreetingTypeFromSCISignMailGreetingType(self.greetingType);
	res.ePersonalType = ESignMailGreetingTypeFromSCISignMailGreetingType(self.personalType);
	
	return res;
}

@end

