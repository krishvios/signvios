//
//  SCIVideophoneEngine+UserInterfaceProperties.h
//  ntouchMac
//
//  Created by Adam Preble on 8/1/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine.h"
#import "SCISignMailGreetingInfo.h"
#import "SCIProfileImagePrivacy.h"

typedef enum {
	SCICallerIDNumberTypeLocal = 0,
	SCICallerIDNumberTypeTollFree = 1
} SCICallerIDNumberType;

typedef enum : NSUInteger
{
	SCIRingGroupDisplayModeHidden = 0,		//eRING_GROUP_DISPLAY_MODE_HIDDEN,
	SCIRingGroupDisplayModeReadOnly = 1,	//eRING_GROUP_DISPLAY_MODE_READ_ONLY,
	SCIRingGroupDisplayModeReadWrite = 2,	//eRING_GROUP_DISPLAY_MODE_READ_WRITE,
	SCIRingGroupDisplayModeUnknown = NSUIntegerMax,
} SCIRingGroupDisplayMode;

typedef NS_ENUM(NSUInteger, SCISecureCallMode)
{
	SCISecureCallModeNotPreferred = 0, // estiSECURE_CALL_NOT_PREFERRED
	SCISecureCallModePreferred = 1,    // estiSECURE_CALL_PREFERRED
	SCISecureCallModeRequired = 2      // estiSECURE_CALL_REQUIRED
};

@interface SCIVideophoneEngine (UserInterfaceProperties)

- (void)registerStandardPropertyManagerChangeNotifications;

@property (nonatomic) BOOL callWaitingEnabled;
@property (nonatomic) BOOL doNotDisturbEnabled;
@property (nonatomic) BOOL audioEnabled;
@property (nonatomic) BOOL tunnelingEnabled;
@property (nonatomic) NSString *personalGreetingText;
@property (nonatomic) BOOL blockCallerID;
@property (nonatomic) BOOL blockAnonymousCallers;
@property (nonatomic) SCISecureCallMode secureCallMode;

//Readwrite properties.  Set using primitive methods to avoid
//	KVO infinite recursion.
@property (nonatomic) NSInteger maxCalls;
@property (nonatomic, readonly) NSDate *lastChannelMessageViewTime;
- (void)setLastChannelMessageViewTimePrimitive:(NSDate *)lastChannelMessageViewTime;
@property (nonatomic, readonly) NSDate *lastMessageViewTime;
- (void)setLastMessageViewTimePrimitive:(NSDate *)lastMessageViewTime;
@property (nonatomic, readonly) NSDate *lastMissedViewTime;
- (void)setLastMissedViewTimePrimitive:(NSDate *)lastMissedViewTime;
@property (nonatomic, readonly) SCICallerIDNumberType callerIDNumberType;
- (void)setCallerIDNumberTypePrimitive:(SCICallerIDNumberType)callerIDNumberType;
@property (nonatomic, readonly) BOOL directSignMailEnabled;
- (void)setDirectSignMailEnabledPrimitive:(BOOL)directSignMailEnabled;
@property (nonatomic, readonly) BOOL displayContactImages;
- (void)setDisplayContactImagesPrimitive:(BOOL)displayContactImages;
@property (nonatomic, readonly) NSInteger ringsBeforeGreeting;
- (void)setRingsBeforeGreetingPrimitive:(NSInteger)numberOfRings;
@property (nonatomic) BOOL vrclEnabled;
- (void)setVrclEnabledPrimitive:(BOOL)vrclEnabled;
@property (nonatomic, readonly) SCIProfileImagePrivacyLevel profileImagePrivacyLevel;
- (void)setProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)profileImagePrivacyLevel;
@property (nonatomic, readonly) SCISignMailGreetingType signMailGreetingPreference;
- (void)setSignMailGreetingPreferencePrimitive:(SCISignMailGreetingType)signMailGreetingPreference;
- (void)setSignMailGreetingPreferencePrimitive:(SCISignMailGreetingType)signMailGreetingPreference send:(BOOL)send;
@property (nonatomic, readonly) SCISignMailGreetingType signMailPersonalGreetingType;
@property (nonatomic, readonly) NSDate *lastCIRCallTime;
- (void)setLastCIRCallTimePrimitive:(NSDate *)lastCIRCallTime;
@property (nonatomic, readonly) BOOL PulseEnabled;
@property (nonatomic, readonly) BOOL VRIFeatures;
@property (nonatomic, readonly) BOOL SpanishFeatures;
- (void)setSpanishFeaturesPrimitive:(BOOL)SpanishFeatures;
- (void)setSavedTextsCountPrimitive:(NSInteger)count;
@property (nonatomic, readonly) BOOL H265Enabled;
- (void)setH265EnabledPrimitive:(BOOL)H265Enabled;
@property (nonatomic, readonly) BOOL captureWidescreenEnabled;
- (void)setCaptureWidescreenEnabledPrimitive:(BOOL)captureWidescreenEnabled;
@property (nonatomic, readonly) BOOL internetSearchAllowed;
- (void)setInternetSearchAllowedPrimitive:(BOOL)internetSearchAllowed;
@property (nonatomic, readonly) BOOL showInternetSearchNag;
- (void)setShowInternetSearchNagPrimitive:(BOOL)showInternetSearchNag;

//Readonly properties
@property (nonatomic, readonly) NSNumber *ringGroupEnabled; // BOOL NSNumber: nil indicates that the value has not yet been loaded from Core
@property (nonatomic, readonly) NSNumber *ringGroupDisplayModeOrNil; //SCIRingGroupDisplayMode (NSUInteger) NSNumber: nil indicates that the value has not yet been loaded from Core
@property (nonatomic, readonly) BOOL videoMessageEnabled;
@property (nonatomic, readonly) BOOL DTMFEnabled;
@property (nonatomic, readonly) BOOL DeafHearingVideoEnabled;
@property (nonatomic, readonly) NSString *coreServerAddress;
@property (nonatomic, readonly) BOOL contactPhotosFeatureEnabled;
@property (nonatomic, readonly, getter=isExportContactsFeatureEnabled) BOOL exportContactsFeatureEnabled;
@property (nonatomic, readonly) BOOL personalSignMailGreetingFeatureEnabled;
@property (nonatomic, readonly) BOOL realTimeTextFeatureEnabled;
@property (nonatomic, readonly) BOOL webDialFeatureEnabled;
@property (nonatomic, readonly) BOOL callTransferFeatureEnabled;
@property (nonatomic, readonly) BOOL dynamicAgreementsFeatureEnabled;
@property (nonatomic, readonly) BOOL favoritesFeatureEnabled;
@property (nonatomic, readonly) BOOL groupVideoChatEnabled;
@property (nonatomic, readonly) BOOL internetSearchEnabled;
@property (nonatomic, readonly) BOOL LDAPDirectoryEnabled;
@property (nonatomic, readonly) NSString *LDAPDirectoryDisplayName;
@property (nonatomic, readonly) NSNumber *LDAPServerRequiresAuthentication; // BOOL NSNumber: nil indicates that the value has not yet been loaded from Core
@property (nonatomic, readonly) BOOL blockCallerIDEnabled;
@property (nonatomic, readonly) NSString *macAddress;

@property (nonatomic, readonly) BOOL debugEnabled;


#pragma mark VCO

@property (nonatomic) SCIVoiceCarryOverType voiceCarryOverType;
@property (nonatomic) BOOL vcoActive;
- (void)updateEngineForVCOType:(SCIVoiceCarryOverType)voiceCarryOverType;
- (void)setVcoActivePrimitive:(BOOL)vcoActive;
@property (nonatomic) NSString *voiceCarryOverNumber;

@end

extern NSString * const SCIKeyAudioEnabled;
extern NSString * const SCIKeyBlockAnonymousCallers;
extern NSString * const SCIKeyBlockCallerID;
extern NSString * const SCIKeyBlockCallerIDEnabled;
extern NSString * const SCIKeyCallerIDNumberType;
extern NSString * const SCIKeyCallTransferFeatureEnabled;
extern NSString * const SCIKeyCallWaitingEnabled;
extern NSString * const SCIKeyContactPhotosFeatureEnabled;
extern NSString * const SCIKeyExportContactsFeatureEnabled;
extern NSString * const SCIKeyCoreServerAddress;
extern NSString * const SCIKeyDirectSignMailEnabled;
extern NSString * const SCIKeyDisplayContactImages;
extern NSString * const SCIKeyDoNotDisturbEnabled;
extern NSString * const SCIKeyDTMFEnabled;
extern NSString * const SCIKeyDynamicAgreementsFeatureEnabled;
extern NSString * const SCIKeyFavoritesFeatureEnabled;
extern NSString * const SCIKeyGroupVideoChatEnabled;
extern NSString * const SCIKeyInternetSearchAllowed;
extern NSString * const SCIKeyInternetSearchEnabled;
extern NSString * const SCIKeyShowInternetSearchNag;
extern NSString * const SCIKeySpanishFeatures;
extern NSString * const SCIKeyLastChannelMessageViewTime;
extern NSString * const SCIKeyLastCIRCallTime;
extern NSString * const SCIKeyLastMessageViewTime;
extern NSString * const SCIKeyLastMissedViewTime;
extern NSString * const SCIKeyLDAPDirectoryEnabled;
extern NSString * const SCIKeyLDAPDirectoryDisplayName;
extern NSString * const SCIKeyLDAPServerRequiresAuthentication;
extern NSString * const SCIKeyPersonalGreetingText;
extern NSString * const SCIKeyPersonalSignMailGreetingFeatureEnabled;
extern NSString * const SCIKeyProfileImagePrivacyLevel;
extern NSString * const SCIKeyRealTimeTextFeatureEnabled;
extern NSString * const SCIKeyRingGroupDisplayModeOrNil;
extern NSString * const SCIKeyRingGroupEnabled;
extern NSString * const SCIKeyRingsBeforeGreeting;
extern NSString * const SCIKeySecureCallMode;
extern NSString * const SCIKeySignMailGreetingPreference;
extern NSString * const SCIKeySignMailPersonalGreetingType;
extern NSString * const SCIKeyTunnelingEnabled;
extern NSString * const SCIKeyVCOPreference;
extern NSString * const SCIKeyVideoMessageEnabled;
extern NSString * const SCIKeyVoiceCarryOverNumber;
extern NSString * const SCIKeyVoiceCarryOverType;
extern NSString * const SCIKeyVCOActive;
extern NSString * const SCIKeyVrclEnabled;
extern NSString * const SCIKeyWebDialFeatureEnabled;
extern NSString * const SCIKeySavedTextsCount;
extern NSString * const SCIKeyCaptureWidescreenEnabled;
extern NSString * const SCIKeyH265Enabled;
extern NSString * const SCIKeyDebugEnabled;
extern NSString * const SCIKeyVRIFeatures;
