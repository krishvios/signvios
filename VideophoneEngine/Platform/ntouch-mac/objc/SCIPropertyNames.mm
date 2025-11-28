//
//  SCIPropertyNames.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/5/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIPropertyNames.h"
#import "propertydef.h"
#import "CommonConst.h"
#import "cmPropertyNameDef.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIVideophoneEngine+UserInterfaceProperties_ProviderAgreementStatus.h"
#import "NSArray+Additions.h"
#import "stiSVX.h"
#import "NSDate+BNRAdditions.h"
#import "smiConfig.h"

NSString * const SCIPropertyNameAgreementsEnabled = @(AGREEMENTS_ENABLED);
NSString * const SCIPropertyNameAutoRejectIncoming = @(PropertyDef::AutoRejectIncoming);
NSString * const SCIPropertyNameBlockAnonymousCallers = @(BLOCK_ANONYMOUS_CALLERS);
NSString * const SCIPropertyNameBlockCallerID = @(BLOCK_CALLER_ID);
NSString * const SCIPropertyNameBlockCallerIDEnabled = @(BLOCK_CALLER_ID_ENABLED);
NSString * const SCIPropertyNameCallerIDNumber = @(PropertyDef::CallerIdNumber);
NSString * const SCIPropertyNameCallTransferEnabled = @(CALL_TRANSFER_ENABLED);
NSString * const SCIPropertyNameContactImagesEnabled = @(CONTACT_IMAGES_ENABLED);
NSString * const SCIPropertyNameExportContactsEnabled = @(EXPORT_CONTACTS_ENABLED);
NSString * const SCIPropertyNameCoreServiceURL1 = @(cmCORE_SERVICE_URL1);
NSString * const SCIPropertyNameDirectSignMailEnabled = @(DIRECT_SIGNMAIL_ENABLED);
NSString * const SCIPropertyNameDisableAudio = @(PropertyDef::DisableAudio);
NSString * const SCIPropertyNameDisplayContactImages = @(DISPLAY_CONTACT_IMAGES);
NSString * const SCIPropertyNameDoNotDisturbEnabled = @(PropertyDef::cmAUTO_REJECT);
NSString * const SCIPropertyNameDTMFEnabled = @(DTMF_ENABLED);
NSString * const SCIPropertyNameDeafHearingVideoEnabled = @(DEAF_HEARING_VIDEO_ENABLED);
NSString * const SCIPropertyNameFavoritesEnabled = @(FAVORITES_ENABLED);
NSString * const SCIPropertyNameGreetingPreference = @(GREETING_PREFERENCE);
NSString * const SCIPropertyNameGreetingText = @(GREETING_TEXT);
NSString * const SCIPropertyNameGroupVideoChatEnabled = @(GROUP_VIDEO_CHAT_ENABLED);
NSString * const SCIPropertyNameInternetSearchAllowed = @(INTERNET_SEARCH_ALLOWED);
NSString * const SCIPropertyNameInternetSearchEnabled = @(INTERNET_SEARCH_ENABLED);
NSString * const SCIPropertyNamePulseEnabled = @(PULSE_ENABLED);
NSString * const SCIPropertyNameShowInternetSearchNag = @"ShowInternetSearchNag";
NSString * const SCIPropertyNameSpanishFeatures = @(PropertyDef::SpanishFeatures);
NSString * const SCIPropertyNameLastChannelMessageViewTime = @(PropertyDef::LastChannelMessageViewTime);
NSString * const SCIPropertyNameLastCIRCallTime = @(LAST_CIR_CALL_TIME);
NSString * const SCIPropertyNameLastMessageViewTime = @(PropertyDef::LastMessageViewTime);
NSString * const SCIPropertyNameLastMissedTime = @(PropertyDef::LastMissedTime);
NSString * const SCIPropertyNameLDAPDirectoryEnabled = @(LDAP_ENABLED);
NSString * const SCIPropertyNameLDAPDirectoryDisplayName = @(LDAP_DIRECTORY_DISPLAY_NAME);
NSString * const SCIPropertyNameLDAPServerRequiresAuthentication = @(LDAP_SERVER_REQUIRES_AUTHENTICATION);
NSString * const SCIPropertyNameMacAddress = @(MAC_ADDRESS);
NSString * const SCIPropertyNamePersonalGreetingEnabled = @(PERSONAL_GREETING_ENABLED);
NSString * const SCIPropertyNamePersonalGreetingType = @(PERSONAL_GREETING_TYPE);
NSString * const SCIPropertyNameProfileImagePrivacyLevel = @(PROFILE_IMAGE_PRIVACY_LEVEL);
NSString * const SCIPropertyNameProviderAgreementStatus = @(PropertyDef::ProviderAgreementStatus);
NSString * const SCIPropertyNameRealTimeTextEnabled = @(REAL_TIME_TEXT_ENABLED);
NSString * const SCIPropertyNameRingGroupDisplayMode = @(RING_GROUP_DISPLAY_MODE);
NSString * const SCIPropertyNameRingGroupEnabled = @(RING_GROUP_ENABLED);
NSString * const SCIPropertyNameRingsBeforeGreeting = @(cmRINGS_BEFORE_GREETING);
NSString * const SCIPropertyNameSecureCallMode = @(SECURE_CALL_MODE);
NSString * const SCIPropertyNameVCOPreference = @(PropertyDef::VCOPreference);
NSString * const SCIPropertyNameVCOActive = @(PropertyDef::VCOActive);
NSString * const SCIPropertyNameVideoMessageEnabled = @(PropertyDef::VideoMessageEnabled);
NSString * const SCIPropertyNameVoiceCallbackNumber = @(PropertyDef::VoiceCallbackNumber);
NSString * const SCIPropertyNameVRCLEnabled = @(PropertyDef::VRCLPortOverride);
NSString * const SCIPropertyNameWebDialEnabled = @(WEB_DIAL_ENABLED);
NSString * const SCIPropertyNameSavedTextsCount = @(PropertyDef::SavedTextsCount);
NSString * const SCIPropertyNameCaptureWidescreenEnabled = @("CaptureWidescreen");
NSString * const SCIPropertyNameH265Enabled = @("EnableH265");
NSString * const SCIPropertyNameDebugEnabled = @("DebugEnabled");
NSString * const SCIPropertyNameVRIFeatures = @(PropertyDef::VRIFeatures);

static NSString * const SCIVideophoneEngineKey = @"key";
static NSString * const SCIPropertyName = @"name";

static NSDictionary *SCIPropertyTypesForPropertyNames();
static NSArray *SCIVideophoneEngineKeysAndPropertyNames();
static NSDictionary *SCIDefaultValuesForPropertyNames();
static NSDictionary *SCIPropertyScopeForPropertyNames();

SCIPropertyType SCIPropertyTypeForPropertyName(NSString *propertyName)
{
	NSNumber *typeNumber = SCIPropertyTypesForPropertyNames()[propertyName];
	SCIPropertyType type = (SCIPropertyType)[typeNumber unsignedIntegerValue];
	return type;
}

NSArray *SCIVideophoneEngineKeysForPropertyName(NSString *propertyName)
{
	NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
	
	for (NSDictionary *pair in SCIVideophoneEngineKeysAndPropertyNames()) {
		if ([pair[SCIPropertyName] isEqualToString:propertyName]) {
			[mutableRes addObject:pair[SCIVideophoneEngineKey]];
		}
	}
	
	return [mutableRes copy];
}

NSArray *SCIPropertyNamesForVideophoneEngineKey(NSString *videophoneEngineKey)
{
	NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
	
	for (NSDictionary *pair in SCIVideophoneEngineKeysAndPropertyNames()) {
		if ([pair[SCIVideophoneEngineKey] isEqualToString:videophoneEngineKey]) {
			[mutableRes addObject:pair[SCIPropertyName]];
		}
	}
	
	return [mutableRes copy];
}

id SCIDefaultValueForPropertyName(NSString *propertyName)
{
	id res = nil;
	
	res = SCIDefaultValuesForPropertyNames()[propertyName];
	if (!res) {
		res = SCIDefaultValueForPropertyType(SCIPropertyTypeForPropertyName(propertyName));
	}
	
	return res;
}

SCIPropertyScope SCIPropertyScopeForPropertyName(NSString *propertyName)
{
	SCIPropertyScope res = SCIPropertyScopeUnknown;
	
	NSNumber *resNumber = SCIPropertyScopeForPropertyNames()[propertyName];
	if (resNumber) {
		res = (SCIPropertyScope)[resNumber unsignedIntegerValue];
	} else {
		//Default to user settings.
		res = SCIPropertyScopeUser;
	}
	
	return res;
}

static NSDictionary *SCIPropertyTypesForPropertyNames()
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddTypeForName(type, name) [mutableDictionary setObject:[NSNumber numberWithUnsignedInteger:type] forKey:name]

		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameAgreementsEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameFavoritesEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameAutoRejectIncoming);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameBlockAnonymousCallers);
		AddTypeForName(SCIPropertyTypeInt,      SCIPropertyNameBlockCallerID);
		AddTypeForName(SCIPropertyTypeInt,      SCIPropertyNameBlockCallerIDEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDoNotDisturbEnabled);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameCallerIDNumber);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameCallTransferEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDirectSignMailEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDisplayContactImages);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameContactImagesEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameExportContactsEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDTMFEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDeafHearingVideoEnabled);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameCoreServiceURL1);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameGreetingText);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameGroupVideoChatEnabled);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameSpanishFeatures);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDisableAudio);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameGreetingPreference);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameInternetSearchAllowed);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameInternetSearchEnabled);
		AddTypeForName(SCIPropertyTypeInt, 		SCIPropertyNamePulseEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameShowInternetSearchNag);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNamePersonalGreetingType);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLastChannelMessageViewTime);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLastCIRCallTime);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLastMessageViewTime);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLastMissedTime);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLDAPDirectoryEnabled);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameLDAPDirectoryDisplayName);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameLDAPServerRequiresAuthentication);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNamePersonalGreetingEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameProfileImagePrivacyLevel);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameProviderAgreementStatus);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameRealTimeTextEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameRingGroupDisplayMode);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameRingGroupEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameRingsBeforeGreeting);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameSecureCallMode);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameVCOPreference);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameVCOActive);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameVideoMessageEnabled);
		AddTypeForName(SCIPropertyTypeString,	SCIPropertyNameVoiceCallbackNumber);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameVRCLEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameWebDialEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameSavedTextsCount);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameCaptureWidescreenEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameH265Enabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameDebugEnabled);
		AddTypeForName(SCIPropertyTypeInt,		SCIPropertyNameVRIFeatures);
		
#undef AddTypeForName
		
		dictionary = [mutableDictionary copy];
	});
	return dictionary;
}

static NSArray *SCIVideophoneEngineKeysAndPropertyNames()
{
	static NSArray *array = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableArray *mutableArray = [[NSMutableArray alloc] init];
#define AddKeyForName(key, name) [mutableArray addObject:@{ SCIVideophoneEngineKey : key, SCIPropertyName : name }]
		AddKeyForName(SCIKeyDynamicAgreementsFeatureEnabled,	SCIPropertyNameAgreementsEnabled);
		AddKeyForName(SCIKeyBlockAnonymousCallers,				SCIPropertyNameBlockAnonymousCallers);
		AddKeyForName(SCIKeyBlockCallerID,						SCIPropertyNameBlockCallerID);
		AddKeyForName(SCIKeyBlockCallerIDEnabled,               SCIPropertyNameBlockCallerIDEnabled);
		AddKeyForName(SCIKeyFavoritesFeatureEnabled,			SCIPropertyNameFavoritesEnabled);
		AddKeyForName(SCIKeyAudioEnabled,						SCIPropertyNameDisableAudio);
		AddKeyForName(SCIKeyCallerIDNumberType,					SCIPropertyNameCallerIDNumber);
		AddKeyForName(SCIKeyCallTransferFeatureEnabled,			SCIPropertyNameCallTransferEnabled);
		AddKeyForName(SCIKeyCallWaitingEnabled,					SCIPropertyNameAutoRejectIncoming);
		AddKeyForName(SCIKeyDirectSignMailEnabled,				SCIPropertyNameDirectSignMailEnabled);
		AddKeyForName(SCIKeyDoNotDisturbEnabled,				SCIPropertyNameDoNotDisturbEnabled);
		AddKeyForName(SCIKeyContactPhotosFeatureEnabled,		SCIPropertyNameContactImagesEnabled);
		AddKeyForName(SCIKeyExportContactsFeatureEnabled,		SCIPropertyNameExportContactsEnabled);
		AddKeyForName(SCIKeyCoreServerAddress,					SCIPropertyNameCoreServiceURL1);
		AddKeyForName(SCIKeyDisplayContactImages,				SCIPropertyNameDisplayContactImages);
		AddKeyForName(SCIKeyDTMFEnabled,						SCIPropertyNameDTMFEnabled);
		AddKeyForName(SCIKeyLastChannelMessageViewTime,			SCIPropertyNameLastChannelMessageViewTime);
		AddKeyForName(SCIKeyLastCIRCallTime,					SCIPropertyNameLastCIRCallTime);
		AddKeyForName(SCIKeyLastMessageViewTime,				SCIPropertyNameLastMessageViewTime);
		AddKeyForName(SCIKeyLastMissedViewTime,					SCIPropertyNameLastMissedTime);
		AddKeyForName(SCIKeyLDAPDirectoryEnabled,				SCIPropertyNameLDAPDirectoryEnabled);
		AddKeyForName(SCIKeyLDAPDirectoryDisplayName,			SCIPropertyNameLDAPDirectoryDisplayName);
		AddKeyForName(SCIKeyLDAPServerRequiresAuthentication,	SCIPropertyNameLDAPServerRequiresAuthentication);
		AddKeyForName(SCIKeyPersonalGreetingText,				SCIPropertyNameGreetingText);
		AddKeyForName(SCIKeyGroupVideoChatEnabled,				SCIPropertyNameGroupVideoChatEnabled);
		AddKeyForName(SCIKeyInternetSearchAllowed,				SCIPropertyNameInternetSearchAllowed);
		AddKeyForName(SCIKeyInternetSearchEnabled,				SCIPropertyNameInternetSearchEnabled);
		AddKeyForName(SCIKeyShowInternetSearchNag,				SCIPropertyNameShowInternetSearchNag);
		AddKeyForName(SCIKeySpanishFeatures,					SCIPropertyNameSpanishFeatures);
		AddKeyForName(SCIKeyPersonalSignMailGreetingFeatureEnabled,	SCIPropertyNamePersonalGreetingEnabled);
		AddKeyForName(SCIKeyProfileImagePrivacyLevel,			SCIPropertyNameProfileImagePrivacyLevel);
		AddKeyForName(SCIKeyProviderAgreementStatusString,		SCIPropertyNameProviderAgreementStatus);
		AddKeyForName(SCIKeyRealTimeTextFeatureEnabled,			SCIPropertyNameRealTimeTextEnabled);
		AddKeyForName(SCIKeyRingGroupDisplayModeOrNil,			SCIPropertyNameRingGroupDisplayMode);
		AddKeyForName(SCIKeyRingGroupEnabled,					SCIPropertyNameRingGroupEnabled);
		AddKeyForName(SCIKeyRingsBeforeGreeting,				SCIPropertyNameRingsBeforeGreeting);
		AddKeyForName(SCIKeySecureCallMode,						SCIPropertyNameSecureCallMode);
		AddKeyForName(SCIKeySignMailGreetingPreference,			SCIPropertyNameGreetingPreference);
		AddKeyForName(SCIKeySignMailPersonalGreetingType,		SCIPropertyNamePersonalGreetingType);
		AddKeyForName(SCIKeyVideoMessageEnabled,				SCIPropertyNameVideoMessageEnabled);
		AddKeyForName(SCIKeyVoiceCarryOverNumber,				SCIPropertyNameVoiceCallbackNumber);
		AddKeyForName(SCIKeyVoiceCarryOverType,					SCIPropertyNameVCOPreference);
		AddKeyForName(SCIKeyVCOActive,							SCIPropertyNameVCOActive);
		AddKeyForName(SCIKeyVrclEnabled,						SCIPropertyNameVRCLEnabled);
		AddKeyForName(SCIKeyWebDialFeatureEnabled,				SCIPropertyNameWebDialEnabled);
		AddKeyForName(SCIKeySavedTextsCount,					SCIPropertyNameSavedTextsCount);
		AddKeyForName(SCIKeyCaptureWidescreenEnabled,			SCIPropertyNameCaptureWidescreenEnabled);
		AddKeyForName(SCIKeyH265Enabled,						SCIPropertyNameH265Enabled);
		AddKeyForName(SCIKeyDebugEnabled,						SCIPropertyNameDebugEnabled);
		AddKeyForName(SCIKeyVRIFeatures,						SCIPropertyNameVRIFeatures);
		
#undef AddKeyForName
		
		array = [mutableArray copy];
	});
	return array;
}

static NSDictionary *SCIDefaultValuesForPropertyNames()
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddDefaultValueForKey(defaultValue, name) [mutableDictionary setObject:defaultValue forKey:name]
		
		AddDefaultValueForKey(@(stiAGREEMENTS_ENABLED_DEFAULT),						SCIPropertyNameAgreementsEnabled);
		AddDefaultValueForKey(@(stiFAVORITES_ENABLED_DEFAULT),						SCIPropertyNameFavoritesEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameAutoRejectIncoming);
		AddDefaultValueForKey(@(0),													SCIPropertyNameBlockAnonymousCallers);
		AddDefaultValueForKey(@(0),													SCIPropertyNameBlockCallerID);
		AddDefaultValueForKey(@(stiBLOCK_CALLER_ID_ENABLED_DEFAULT),				SCIPropertyNameBlockCallerIDEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameDoNotDisturbEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameCallerIDNumber);
		AddDefaultValueForKey(@(stiCALL_TRANSFER_ENABLED_DEFAULT),					SCIPropertyNameCallTransferEnabled);
		AddDefaultValueForKey(@(stiDIRECT_SIGNMAIL_ENABLED_DEFAULT),				SCIPropertyNameDirectSignMailEnabled);
		AddDefaultValueForKey(@(1),													SCIPropertyNameDisplayContactImages);
		AddDefaultValueForKey(@(1),													SCIPropertyNameDisableAudio);
		AddDefaultValueForKey(@(stiDTMF_ENABLED_DEFAULT),							SCIPropertyNameDTMFEnabled);
		AddDefaultValueForKey(@(stiDEAF_HEARING_VIDEO_ENABLED_DEFAULT),				SCIPropertyNameDeafHearingVideoEnabled);
		AddDefaultValueForKey(@(SCISignMailGreetingTypeDefault),					SCIPropertyNameGreetingPreference);
		AddDefaultValueForKey(@(stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT),				SCIPropertyNameGroupVideoChatEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameInternetSearchAllowed);
		AddDefaultValueForKey(@(1),													SCIPropertyNameShowInternetSearchNag);
		AddDefaultValueForKey(@(stiINTERNET_SEARCH_ENABLED_DEFAULT),				SCIPropertyNameInternetSearchEnabled);
		AddDefaultValueForKey(@"EN",												SCIPropertyNameSpanishFeatures);
		AddDefaultValueForKey(@(SCISignMailGreetingTypePersonalVideoOnly),			SCIPropertyNamePersonalGreetingType);
		//TODO: These really should always be @([[NSDate date] timeIntervalSince1970])...
		AddDefaultValueForKey(@(0),													SCIPropertyNameLastChannelMessageViewTime);
		AddDefaultValueForKey(@(0),													SCIPropertyNameLastMessageViewTime);
		AddDefaultValueForKey(@(0),													SCIPropertyNameLastMissedTime);
		AddDefaultValueForKey(@(stiLDAP_ENABLED_DEFAULT),							SCIPropertyNameLDAPDirectoryEnabled);
		AddDefaultValueForKey(@"Directory",											SCIPropertyNameLDAPDirectoryDisplayName);
		AddDefaultValueForKey(@(1),													SCIPropertyNameLDAPServerRequiresAuthentication);
		AddDefaultValueForKey(@(stiREAL_TIME_TEXT_ENABLED_DEFAULT),					SCIPropertyNameRealTimeTextEnabled);
		AddDefaultValueForKey(@(SCIRingGroupDisplayModeReadOnly),					SCIPropertyNameRingGroupDisplayMode);
		AddDefaultValueForKey(@(1),													SCIPropertyNameRingGroupEnabled);
		AddDefaultValueForKey(@(8),													SCIPropertyNameRingsBeforeGreeting);
		AddDefaultValueForKey(@(stiSECURE_CALL_MODE_DEFAULT),						SCIPropertyNameSecureCallMode);
		AddDefaultValueForKey(@"",													SCIPropertyNameGreetingText);
		AddDefaultValueForKey(@(estiVCO_NONE),										SCIPropertyNameVCOPreference);
		AddDefaultValueForKey(@(stiVCO_ACTIVE_DEFAULT),								SCIPropertyNameVCOActive);
		AddDefaultValueForKey(@(stiPSMG_ENABLED_DEFAULT),                           SCIPropertyNamePersonalGreetingEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameProfileImagePrivacyLevel);
		AddDefaultValueForKey(@(stiCONTACT_PHOTOS_ENABLED_DEFAULT),					SCIPropertyNameContactImagesEnabled);
		AddDefaultValueForKey(@(1),													SCIPropertyNameExportContactsEnabled);
		AddDefaultValueForKey(@(1),													SCIPropertyNameVideoMessageEnabled);
		AddDefaultValueForKey(@"",													SCIPropertyNameVoiceCallbackNumber);
		AddDefaultValueForKey(@(0),													SCIPropertyNameVRCLEnabled);
		AddDefaultValueForKey(@(1),													SCIPropertyNameWebDialEnabled);
		AddDefaultValueForKey(@(9),													SCIPropertyNameSavedTextsCount);
		AddDefaultValueForKey(@(0),													SCIPropertyNameCaptureWidescreenEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameH265Enabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameDebugEnabled);
		AddDefaultValueForKey(@(0),													SCIPropertyNameVRIFeatures);
		
#undef AddDefaultValueForKey
		
		dictionary = [mutableDictionary copy];
	});
	return dictionary;
}

static NSDictionary *SCIPropertyScopeForPropertyNames()
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddPropertyScopeForKey( propertyScope, name ) \
do { \
	mutableDictionary[name] = @((propertyScope)); \
} while(0)
		
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameAgreementsEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameFavoritesEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameAutoRejectIncoming);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameBlockAnonymousCallers);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameBlockCallerID);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameBlockCallerIDEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameDoNotDisturbEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameCallerIDNumber);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameCallTransferEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameDirectSignMailEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameDisplayContactImages);
		AddPropertyScopeForKey(SCIPropertyScopePhone, 		SCIPropertyNameDisableAudio);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameDTMFEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameDeafHearingVideoEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameGreetingPreference);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameGroupVideoChatEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameInternetSearchAllowed);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameInternetSearchEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNamePulseEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameSpanishFeatures);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNamePersonalGreetingType);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLastChannelMessageViewTime);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLastMessageViewTime);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLastMissedTime);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLDAPDirectoryEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLDAPDirectoryDisplayName);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameLDAPServerRequiresAuthentication);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameRealTimeTextEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameRingGroupDisplayMode);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameRingGroupEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameRingsBeforeGreeting);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameSecureCallMode);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameGreetingText);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameVCOPreference);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameVCOActive);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNamePersonalGreetingEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameProfileImagePrivacyLevel);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameContactImagesEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameExportContactsEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameVideoMessageEnabled);
		AddPropertyScopeForKey(SCIPropertyScopePhone, 	SCIPropertyNameVoiceCallbackNumber);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameVRCLEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameWebDialEnabled);
		AddPropertyScopeForKey(SCIPropertyScopeUser,		SCIPropertyNameSavedTextsCount);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameCaptureWidescreenEnabled);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameH265Enabled);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameDebugEnabled);
		AddPropertyScopeForKey(SCIPropertyScopePhone,		SCIPropertyNameShowInternetSearchNag);
		AddPropertyScopeForKey(SCIPropertyScopeUser, 		SCIPropertyNameVRIFeatures);
		
#undef AddPropertyScopeForKey
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary;
}


