//
//  SCIPropertyNames.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/5/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIPropertyManager.h"

SCIPropertyType SCIPropertyTypeForPropertyName(NSString *propertyName);
id SCIDefaultValueForPropertyName(NSString *propertyName);
SCIPropertyScope SCIPropertyScopeForPropertyName(NSString *propertyName);

NSArray *SCIVideophoneEngineKeysForPropertyName(NSString *propertyName);
NSArray *SCIPropertyNamesForVideophoneEngineKey(NSString *videophoneEngineKey);

extern NSString * const SCIPropertyNameAgreementsEnabled;
extern NSString * const SCIPropertyNameAutoRejectIncoming;
extern NSString * const SCIPropertyNameBlockAnonymousCallers;
extern NSString * const SCIPropertyNameBlockCallerIDEnabled;
extern NSString * const SCIPropertyNameBlockCallerID;
extern NSString * const SCIPropertyNameCallerIDNumber;
extern NSString * const SCIPropertyNameCallTransferEnabled;
extern NSString * const SCIPropertyNameContactImagesEnabled;
extern NSString * const SCIPropertyNameExportContactsEnabled;
extern NSString * const SCIPropertyNameCoreServiceURL1;
extern NSString * const SCIPropertyNameDirectSignMailEnabled;
extern NSString * const SCIPropertyNameDisableAudio;
extern NSString * const SCIPropertyNameDisplayContactImages;
extern NSString * const SCIPropertyNameDoNotDisturbEnabled;
extern NSString * const SCIPropertyNameDTMFEnabled;
extern NSString * const SCIPropertyNameDeafHearingVideoEnabled;
extern NSString * const SCIPropertyNameFavoritesEnabled;
extern NSString * const SCIPropertyNameGreetingPreference;
extern NSString * const SCIPropertyNameGreetingText;
extern NSString * const SCIPropertyNameGroupVideoChatEnabled;
extern NSString * const SCIPropertyNameInternetSearchAllowed;
extern NSString * const SCIPropertyNameInternetSearchEnabled;
extern NSString * const SCIPropertyNamePulseEnabled;
extern NSString * const SCIPropertyNameShowInternetSearchNag;
extern NSString * const SCIPropertyNameSpanishFeatures;
extern NSString * const SCIPropertyNameLastChannelMessageViewTime;
extern NSString * const SCIPropertyNameLastCIRCallTime;
extern NSString * const SCIPropertyNameLastMessageViewTime;
extern NSString * const SCIPropertyNameLastMissedTime;
extern NSString * const SCIPropertyNameLDAPDirectoryEnabled;
extern NSString * const SCIPropertyNameLDAPDirectoryDisplayName;
extern NSString * const SCIPropertyNameLDAPServerRequiresAuthentication;
extern NSString * const SCIPropertyNameMacAddress;
extern NSString * const SCIPropertyNamePersonalGreetingEnabled;
extern NSString * const SCIPropertyNamePersonalGreetingType;
extern NSString * const SCIPropertyNameProfileImagePrivacyLevel;
extern NSString * const SCIPropertyNameProviderAgreementStatus;
extern NSString * const SCIPropertyNameRealTimeTextEnabled;
extern NSString * const SCIPropertyNameRingGroupDisplayMode;
extern NSString * const SCIPropertyNameRingGroupEnabled;
extern NSString * const SCIPropertyNameRingsBeforeGreeting;
extern NSString * const SCIPropertyNameSecureCallMode;
extern NSString * const SCIPropertyNameVCOPreference;
extern NSString * const SCIPropertyNameVCOActive;
extern NSString * const SCIPropertyNameVideoMessageEnabled;
extern NSString * const SCIPropertyNameVoiceCallbackNumber;
extern NSString * const SCIPropertyNameVRCLEnabled;
extern NSString * const SCIPropertyNameWebDialEnabled;
extern NSString * const SCIPropertyNameSavedTextsCount;
extern NSString * const SCIPropertyNameCaptureWidescreenEnabled;
extern NSString * const SCIPropertyNameH265Enabled;
extern NSString * const SCIPropertyNameDebugEnabled;
extern NSString * const SCIPropertyNameVRIFeatures;
