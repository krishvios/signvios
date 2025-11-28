//
//  SCISignMailGreetingInfo.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCISignMailGreetingInfo.h"
#import "SCISignMailGreetingInfo_Cpp.h"
#import "CstiMessageResponse.h"

@interface SCISignMailTypedGreetingInfo ()
+ (SCISignMailTypedGreetingInfo *)typedInfoWithSGreetingInfo:(const CstiMessageResponse::SGreetingInfo &)greetingInfo maxRecordSecondsOut:(int *)maxRecordSecondsOut;
@end

@implementation SCISignMailGreetingInfo

+ (SCISignMailGreetingInfo *)greetingInfoFromSGreetingInfoVector:(const std::vector<CstiMessageResponse::SGreetingInfo> &)greetingInfoList
{
	SCISignMailGreetingInfo *res = [[SCISignMailGreetingInfo alloc] init];
	int maxRecordingLength = 0;
	NSArray *typedInfos = nil;
	
	if (!greetingInfoList.empty()) {
		NSMutableArray *mutableTypedInfos = [[NSMutableArray alloc] init];
		for (std::vector<CstiMessageResponse::SGreetingInfo>::const_iterator iterator = greetingInfoList.begin();
			 iterator != greetingInfoList.end();
			 iterator++) {
			CstiMessageResponse::SGreetingInfo greetingInfo = *iterator;
			
			int nMaxRecordSeconds = 0;
			SCISignMailTypedGreetingInfo *typedInfo = [SCISignMailTypedGreetingInfo typedInfoWithSGreetingInfo:greetingInfo maxRecordSecondsOut:&nMaxRecordSeconds];
			[mutableTypedInfos addObject:typedInfo];
			maxRecordingLength = nMaxRecordSeconds;
		}
		typedInfos = [mutableTypedInfos copy];
	}
	
	res.typedInfos = typedInfos;
	res.maxRecordingLength = maxRecordingLength;
	
	return res;
}

- (SCISignMailTypedGreetingInfo *)typedInfoOfType:(SCISignMailGreetingType)type
{
	SCISignMailTypedGreetingInfo *res = nil;
	
	for (SCISignMailTypedGreetingInfo *typedInfo in self.typedInfos) {
		if (typedInfo.type == type) {
			res = typedInfo;
			break;
		}
	}
		
	return res;
}

- (NSArray *)URLsOfType:(SCISignMailGreetingType)type
{
	SCISignMailTypedGreetingInfo *typedInfo = [self typedInfoOfType:type];
	return typedInfo.URLs;
}

@end

@implementation SCISignMailTypedGreetingInfo

+ (SCISignMailTypedGreetingInfo *)typedInfoWithSGreetingInfo:(const CstiMessageResponse::SGreetingInfo &)greetingInfo maxRecordSecondsOut:(int *)maxRecordSecondsOut;
{
	SCISignMailTypedGreetingInfo *typedInfo = [[SCISignMailTypedGreetingInfo alloc] init];
	
	CstiMessageResponse::EGreetingType type = greetingInfo.eGreetingType;
	typedInfo.type = SCISignMailGreetingTypeFromEGreetingType(type);
	
	const char *pszGreetingViewURL1 = greetingInfo.Url1.c_str();
	const char *pszGreetingViewURL2 = greetingInfo.Url2.c_str();
	int nMaxRecordSeconds = greetingInfo.nMaxSeconds;
	NSString *greetingViewURL1 = (pszGreetingViewURL1) ? [NSString stringWithUTF8String:pszGreetingViewURL1] : nil;
	NSString *greetingViewURL2 = (pszGreetingViewURL2) ? [NSString stringWithUTF8String:pszGreetingViewURL2] : nil;
	NSMutableArray *greetingViewURLs = [[NSMutableArray alloc] init];
	if (greetingViewURL1) [greetingViewURLs addObject:greetingViewURL1];
	if (greetingViewURL2) [greetingViewURLs addObject:greetingViewURL2];
	typedInfo.URLs = [greetingViewURLs copy];
	
	if (maxRecordSecondsOut) {
		*maxRecordSecondsOut = nMaxRecordSeconds;
	}
	return typedInfo;
}

@end

SCISignMailGreetingType SCISignMailGreetingTypeFromEGreetingType(CstiMessageResponse::EGreetingType greetingType)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addPreferenceForType( preference , type ) \
do { \
[mutableDictionary setObject:@((preference)) forKey:@((type))]; \
} while (0)
		//TODO: CstiMessageResponse::EGreetingType may need an extra entry eGREETING_TYPE_NONE.  If and when
		//		that's added, this mapping needs to be updated to map
		//		CstiMessageResponse::eGREETING_TYPE_UNKNOWN to SCISignMailGreetingTypeUnknown and to
		//		map CstiMessageResponse::eGREETING_TYPE_NONE to SCISignMailGreetingPrefenceNone.
		addPreferenceForType( SCISignMailGreetingTypeNone, CstiMessageResponse::eGREETING_TYPE_UNKNOWN );
		addPreferenceForType( SCISignMailGreetingTypeDefault, CstiMessageResponse::eGREETING_TYPE_DEFAULT );
		addPreferenceForType( SCISignMailGreetingTypePersonalVideoOnly, CstiMessageResponse::eGREETING_TYPE_PERSONAL );
#undef addPreferenceForType
		
		dictionary = [mutableDictionary copy];
	});
	return (SCISignMailGreetingType)[dictionary[@(greetingType)] unsignedIntegerValue];
}

SCISignMailGreetingType SCISignMailGreetingTypeFromESignMailGreetingType(eSignMailGreetingType greetingType)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addPreferenceForType( preference, type ) \
do { \
mutableDictionary[@((type))] = @((preference)); \
} while(0)
		
		addPreferenceForType(SCISignMailGreetingTypeNone,					eGREETING_NONE);
		addPreferenceForType(SCISignMailGreetingTypeDefault,				eGREETING_DEFAULT);
		addPreferenceForType(SCISignMailGreetingTypePersonalVideoOnly,		eGREETING_PERSONAL);
		addPreferenceForType(SCISignMailGreetingTypePersonalVideoAndText,	eGREETING_PERSONAL_TEXT);
		addPreferenceForType(SCISignMailGreetingTypePersonalTextOnly,		eGREETING_TEXT_ONLY);
		
#undef addPreferenceForType
		
		dictionary = [mutableDictionary copy];
	});
	return (SCISignMailGreetingType)[dictionary[@(greetingType)] unsignedIntegerValue];
}

eSignMailGreetingType ESignMailGreetingTypeFromSCISignMailGreetingType(SCISignMailGreetingType type)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addTypeForPreference( type, preference ) \
do { \
mutableDictionary[@((preference))] = @((type)); \
} while(0)
		
		addTypeForPreference(eGREETING_NONE,			SCISignMailGreetingTypeNone);
		addTypeForPreference(eGREETING_DEFAULT,			SCISignMailGreetingTypeDefault);
		addTypeForPreference(eGREETING_PERSONAL,		SCISignMailGreetingTypePersonalVideoOnly);
		addTypeForPreference(eGREETING_PERSONAL_TEXT,	SCISignMailGreetingTypePersonalVideoAndText);
		addTypeForPreference(eGREETING_TEXT_ONLY,		SCISignMailGreetingTypePersonalTextOnly);
		
#undef addTypeForPreference
		
		dictionary = [mutableDictionary copy];
	});
	return (eSignMailGreetingType)[dictionary[@(type)] intValue];
}

BOOL SCISignMailGreetingTypeIsPersonal(SCISignMailGreetingType type)
{
	return (type == SCISignMailGreetingTypePersonalVideoOnly ||
			type == SCISignMailGreetingTypePersonalVideoAndText ||
			type == SCISignMailGreetingTypePersonalTextOnly);
}

BOOL SCISignMailGreetingTypeHasText(SCISignMailGreetingType type)
{
	return (type == SCISignMailGreetingTypePersonalVideoAndText ||
			type == SCISignMailGreetingTypePersonalTextOnly);
}



