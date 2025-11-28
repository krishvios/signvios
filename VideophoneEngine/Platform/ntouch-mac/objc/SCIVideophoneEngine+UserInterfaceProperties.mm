//
//  SCIVideophoneEngine+UserInterfaceProperties.m
//  ntouchMac
//
//  Created by Adam Preble on 8/1/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCIVideophoneEngine+UserInterfaceProperties_ProviderAgreementStatus.h"
#import "SCIVideophoneEngineCpp.h"
#import "stiDefs.h"
#import "stiOS.h"
#import "stiError.h"
#import "stiTools.h"
#import "IstiVideophoneEngine.h"
#import "IstiMessageManager.h"
#import "IstiMessageViewer.h"
#import "IstiAudioOutput.h"
#import "CstiCoreRequest.h"
#import "CstiStateNotifyResponse.h"
#import "PropertyManager.h"
#import "propertydef.h"
#import "CommonConst.h"
#import "NSArray+CstiSettingList.h"
#import "SCISettingItem.h"
#import <objc/runtime.h>
#import "NSRunLoop+PerformBlock.h"
#import "cmPropertyNameDef.h"
#import "SCIPropertyNames.h"
#import "SCIPropertyManager.h"
#import "NSObject+BNRAssociatedDictionary.h"
#import "SCISettingItem+Convenience.h"
#import "SCIStaticProviderAgreementStatus.h"
#import "ntouch-Swift.h"

extern NSError *NSErrorFromCstiCoreResponse(CstiCoreResponse *resp);
extern NSString * const SCIKeyProviderAgreementStatusString;

@interface SCIVideophoneEngine (UserInterfacePropertiesPrivate)
- (SCIPropertyManager *)propertyManager;
- (void)expungeCachedValueForPropertyNamed:(NSString *)propertyName;
- (BOOL)isExpungingCachedValueForPropertyNamed:(NSString *)propertyName;
- (void)setShouldNotifyPropertyChanged:(BOOL)shouldNotifyPropertyChanged forPropertyNamed:(NSString *)propertyName;
- (BOOL)shouldNotifyPropertyChangedForPropertyNamed:(NSString *)propertyName;
- (void)setValueIsCurrent:(BOOL)current forPropertyNamed:(NSString *)propertyName;
- (BOOL)valueForPropertyNamedIsCurrent:(NSString *)propertyName;
- (void)willChangeValueForPropertyName:(NSString *)propertyName;
- (void)didChangeValueForPropertyName:(NSString *)propertyName;
@end

@implementation SCIVideophoneEngine (UserInterfaceProperties)

#pragma mark - Provider Agreement

- (BOOL)callWaitingEnabled
{
	int autoRejectIncomingInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameAutoRejectIncoming
														  inStorageLocation:SCIPropertyStorageLocationPersistent
																withDefault:0];
	BOOL callWaitingEnabled = (autoRejectIncomingInt == 0);
	return callWaitingEnabled;
}
- (void)setCallWaitingEnabled:(BOOL)callWaitingEnabled
{
	int autoRejectIncomingInt = callWaitingEnabled ? 0 : 1;
	[self.propertyManager setIntValue:autoRejectIncomingInt
					 forPropertyNamed:SCIPropertyNameAutoRejectIncoming
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self setMaxCalls:callWaitingEnabled ? 2 : 1];
}

- (void)setMaxCalls:(NSInteger)callCount
{
	if (self.videophoneEngine) {
		self.videophoneEngine->MaxCallsSet(callCount);
	}
}

- (NSInteger)maxCalls {
	if (self.videophoneEngine) {
		return self.videophoneEngine->MaxCallsGet();
	}
	
	return 0;
}

- (BOOL)doNotDisturbEnabled
{
	int doNotDisturbInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDoNotDisturbEnabled
															 inStorageLocation:SCIPropertyStorageLocationPersistent
																   withDefault:0];
	BOOL dndEnabled = (doNotDisturbInt == 1);
	return dndEnabled;
}

- (void)setDoNotDisturbEnabled:(BOOL)doNotDisturbEnabled
{
	[[SCIVideophoneEngine sharedEngine] setRejectIncomingCalls:doNotDisturbEnabled];
	
	int doNotDisturbInt = doNotDisturbEnabled ? 1 : 0;
	[self.propertyManager setIntValue:doNotDisturbInt
					 forPropertyNamed:SCIPropertyNameDoNotDisturbEnabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameDoNotDisturbEnabled
										  withScope:SCIPropertyScopeUser];
}

- (BOOL)audioEnabled
{
	int audioDisabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDisableAudio
													 inStorageLocation:SCIPropertyStorageLocationPersistent
														   withDefault:1];
	BOOL audioEnabled = (audioDisabledInt == 0);
	return audioEnabled;
}
- (void)setAudioEnabled:(BOOL)audioEnabled
{
	int audioDisabledInt = audioEnabled ? 0 : 1;
	[self.propertyManager setIntValue:audioDisabledInt
					 forPropertyNamed:SCIPropertyNameDisableAudio
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameDisableAudio
										  withScope:SCIPropertyScopePhone];
	
	EstiBool audioEnabledBool = audioEnabled ? estiTRUE : estiFALSE;
	// No longer setting audio enabled or disabled. Instead we will set wether or not to play audio received
	// the UI is now responsible for setting audioPrivacy when audio is disabled.
//	self.videophoneEngine->AudioEnabledSet(audioEnabledBool);
	if(IstiAudioOutput::InstanceGet())
		IstiAudioOutput::InstanceGet()->AudioOut(audioEnabledBool ? estiON : estiOFF);
}

- (BOOL)blockCallerID
{
	EstiSwitch blockCallerIDSwitch = self.videophoneEngine->BlockCallerIDGet();
	BOOL callerIDBlocked = (blockCallerIDSwitch == estiON && self.isAuthenticated) ? YES : NO;
	return callerIDBlocked;
}

- (void)setBlockCallerID:(BOOL)callerIDBLocked
{
	EstiSwitch callerIDBlockedSwitch = callerIDBLocked ? estiON : estiOFF;
	self.videophoneEngine->BlockCallerIDSet(callerIDBlockedSwitch);
}

- (BOOL)blockAnonymousCallers
{
	EstiSwitch blockAnonymousCallersSwitch = self.videophoneEngine->BlockAnonymousCallersGet();
	BOOL anonymousCallersBlocked = (blockAnonymousCallersSwitch == estiON) ? YES : NO;
	return anonymousCallersBlocked;
}

- (void)setBlockAnonymousCallers:(BOOL)anonymousCallersBLocked
{
	EstiSwitch anonymousCallersBlockedSwitch = anonymousCallersBLocked ? estiON : estiOFF;
	self.videophoneEngine->BlockAnonymousCallersSet(anonymousCallersBlockedSwitch);
}

- (SCISecureCallMode)secureCallMode
{
	return (SCISecureCallMode)self.videophoneEngine->SecureCallModeGet();
}

- (void)setSecureCallMode:(SCISecureCallMode)mode
{
	self.videophoneEngine->SecureCallModeSet((EstiSecureCallMode)mode);
}

#ifdef stiTUNNEL_MANAGER
- (BOOL)tunnelingEnabled
{
	return (BOOL)self.videophoneEngine->TunnelingEnabledGet();
}
- (void)setTunnelingEnabled:(BOOL)tunnelingEnabled
{
	self.videophoneEngine->TunnelingEnabledSet((bool)tunnelingEnabled);
}
#endif

- (void)setLastChannelMessageViewTime:(NSDate*)date
{
	[self setLastChannelMessageViewTimePrimitive:date];
}
- (void)setLastChannelMessageViewTimePrimitive:(NSDate*)date
{
	int lastChannelMessageViewTimeInt = (int)[date timeIntervalSince1970];
	[self.propertyManager setIntValue:lastChannelMessageViewTimeInt
					 forPropertyNamed:SCIPropertyNameLastChannelMessageViewTime
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameLastChannelMessageViewTime
										  withScope:SCIPropertyScopeUser];
}
- (NSDate *)lastChannelMessageViewTime
{
	int lastChannelMessageViewTimeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLastChannelMessageViewTime
																	 inStorageLocation:SCIPropertyStorageLocationPersistent
																		   withDefault:(int)[[NSDate date] timeIntervalSince1970]];
	NSDate *lastChannelMessageViewTime = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)lastChannelMessageViewTimeInt];
	return lastChannelMessageViewTime;
}

- (void)setLastMessageViewTime:(NSDate*)date
{
	[self setLastMessageViewTimePrimitive:date];
}
- (void)setLastMessageViewTimePrimitive:(NSDate*)date
{
	int lastMessageViewTimeInt = (int)[date timeIntervalSince1970];
	[self.propertyManager setIntValue:lastMessageViewTimeInt
					 forPropertyNamed:SCIPropertyNameLastMessageViewTime
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameLastMessageViewTime
										  withScope:SCIPropertyScopeUser];
}
- (NSDate *)lastMessageViewTime
{
	int lastMessageViewTimeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLastMessageViewTime
															  inStorageLocation:SCIPropertyStorageLocationPersistent
																	withDefault:(int)[[NSDate date] timeIntervalSince1970]];
	NSDate *lastMessageViewTime = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)lastMessageViewTimeInt];
	return lastMessageViewTime;
}

- (void)setLastMissedViewTime:(NSDate*)date
{
	[self setLastMissedViewTimePrimitive:date];
}
- (void)setLastMissedViewTimePrimitive:(NSDate*)date
{
	int lastMissedTimeInt = (int)[date timeIntervalSince1970];
	[self.propertyManager setIntValue:lastMissedTimeInt
					 forPropertyNamed:SCIPropertyNameLastMissedTime
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameLastMissedTime
										  withScope:SCIPropertyScopeUser];
}
- (NSDate *)lastMissedViewTime
{
	int lastMissedTimeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLastMissedTime
														 inStorageLocation:SCIPropertyStorageLocationPersistent
															   withDefault:(int)[[NSDate date] timeIntervalSince1970]];
	return lastMissedTimeInt > 0 ? [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)lastMissedTimeInt] : nil;
}

- (void)setRingGroupEnabledPrimitiveLocal:(BOOL)ringGroupEnabled
{
	int ringGroupEnabledInt = ringGroupEnabled ? 1 : 0;
	[self.propertyManager setIntValue:ringGroupEnabledInt
					 forPropertyNamed:SCIPropertyNameRingGroupEnabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
}
- (NSNumber *)ringGroupEnabled
{
	NSNumber *res = nil;
    res = [NSNumber numberWithBool:self.ringGroupEnabledPrimitive];
    return res;
}
- (BOOL)ringGroupEnabledPrimitive
{
	int ringGroupEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameRingGroupEnabled
														   inStorageLocation:SCIPropertyStorageLocationPersistent
																 withDefault:1];
	BOOL ringGroupEnabled = ringGroupEnabledInt ? YES : NO;
	return ringGroupEnabled;
}

- (void)setRingGroupDisplayModePrimitiveLocal:(SCIRingGroupDisplayMode)ringGroupDisplayMode
{
	int ringGroupDisplayModeInt = (int)ringGroupDisplayMode;
	[self.propertyManager setIntValue:ringGroupDisplayModeInt
					 forPropertyNamed:SCIPropertyNameRingGroupDisplayMode
					inStorageLocation:SCIPropertyStorageLocationPersistent];
}
- (NSNumber *)ringGroupDisplayModeOrNil
{
	NSNumber *res = nil;
    res = [NSNumber numberWithUnsignedInteger:self.ringGroupDisplayModePrimitive];
	return res;
}
- (SCIRingGroupDisplayMode)ringGroupDisplayModePrimitive
{
	int ringGroupDisplayModeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameRingGroupDisplayMode
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:(int)SCIRingGroupDisplayModeReadOnly];
	SCIRingGroupDisplayMode ringGroupDisplayMode = (SCIRingGroupDisplayMode)ringGroupDisplayModeInt;
	return ringGroupDisplayMode;
}

- (void)setCallerIDNumberType:(SCICallerIDNumberType)callerIDNumberType
{
	[self setCallerIDNumberTypePrimitive:callerIDNumberType];
}
- (void)setCallerIDNumberTypePrimitive:(SCICallerIDNumberType)callerIDNumberType
{
	int callerIDNumberInt = (int)callerIDNumberType;
	[self.propertyManager setIntValue:callerIDNumberInt
					 forPropertyNamed:SCIPropertyNameCallerIDNumber
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameCallerIDNumber
										  withScope:SCIPropertyScopeUser];
}
- (SCICallerIDNumberType)callerIDNumberType
{
	int callerIDNumberInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameCallerIDNumber
														 inStorageLocation:SCIPropertyStorageLocationPersistent
															   withDefault:0];
	SCICallerIDNumberType callerIDNumberType = (SCICallerIDNumberType)callerIDNumberInt;
	return callerIDNumberType;
}

- (void)setPersonalGreetingText:(NSString *)personalGreetingText
{
	[self.propertyManager setStringValue:personalGreetingText
						forPropertyNamed:SCIPropertyNameGreetingText
					   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameGreetingText
										  withScope:SCIPropertyScopeUser];
}
- (NSString *)personalGreetingText
{
	NSString *personalGreetingText = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameGreetingText
																	 inStorageLocation:SCIPropertyStorageLocationPersistent
																		   withDefault:@""];
	return personalGreetingText;
}

+ (NSSet *)keyPathsForValuesAffectingProviderAgreementAccepted
{
	return [NSSet setWithObject:SCIKeyProviderAgreementStatusString];
}
- (void)setProviderAgreementAcceptedPrimitive:(BOOL)providerAgreementAccepted
{
	[self setProviderAgreementStatusStringPrimitive:[self providerAgreementStatusString:providerAgreementAccepted]];
}
- (NSNumber *)providerAgreementAccepted
{
	NSNumber *res = nil;
	if ([self valueForPropertyNamedIsCurrent:SCIPropertyNameProviderAgreementStatus]) {
		res = [NSNumber numberWithBool:self.providerAgreementAcceptedPrimitive];
	}
	return res;
}
- (BOOL)providerAgreementAcceptedPrimitive
{
	return [self providerAgreementStatusStringIsAcceptance:self.providerAgreementStatusString];
}

- (void)setProviderAgreementAcceptedPrimitive:(BOOL)accepted timeout:(NSTimeInterval)timeout completionHandler:(void (^)(NSError *))block
{
	NSString *providerAgreementStatusString = [self providerAgreementStatusString:accepted];
	SCISettingItem *item = [SCISettingItem settingItemWithName:SCIPropertyNameProviderAgreementStatus
												   stringValue:providerAgreementStatusString];
	
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-retain-cycles"
	[self setUserSetting:item
		   preStoreBlock:^(id oldValue, id newValue) {
			   [self setShouldNotifyPropertyChanged:NO forPropertyNamed:SCIPropertyNameProviderAgreementStatus];
		   }
		  postStoreBlock:^(id oldValue, id newValue) {
			  [self setShouldNotifyPropertyChanged:YES forPropertyNamed:SCIPropertyNameProviderAgreementStatus];
		  }
				 timeout:timeout
			  completion:block];
#pragma clang diagnostic pop
}
- (void)setProviderAgreementAcceptedPrimitive:(BOOL)accepted completionHandler:(void (^)(NSError *))block
{
	[self setProviderAgreementAcceptedPrimitive:accepted timeout:SCIVideophoneEngineDefaultTimeout completionHandler:block];
}

- (BOOL)videoMessageEnabled
{
	int videoMessageEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameVideoMessageEnabled
															  inStorageLocation:SCIPropertyStorageLocationPersistent
																	withDefault:1];
	BOOL videoMessageEnabled = videoMessageEnabledInt ? 1 : 0;
	return videoMessageEnabled;
}

- (BOOL)DTMFEnabled
{
	int dtmfEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDTMFEnabled
													  inStorageLocation:SCIPropertyStorageLocationPersistent
															withDefault:stiDTMF_ENABLED_DEFAULT];
	BOOL dtmfEnabled = dtmfEnabledInt ? YES : NO;
	return dtmfEnabled;
}

- (BOOL)DeafHearingVideoEnabled
{
	int dhvEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDeafHearingVideoEnabled
													  inStorageLocation:SCIPropertyStorageLocationPersistent
															withDefault:stiDEAF_HEARING_VIDEO_ENABLED_DEFAULT];
	BOOL dhvEnabled = dhvEnabledInt ? YES : NO;
	return dhvEnabled;
}

-(void)setDirectSignMailEnabled:(BOOL)directSignMailEnabled
{
	[self setDirectSignMailEnabledPrimitive:directSignMailEnabled];
}
-(void)setDirectSignMailEnabledPrimitive:(BOOL)directSignMailEnabled
{
	int directSignMailEnabledInt = directSignMailEnabled ? 1 : 0;
	[self.propertyManager setIntValue:directSignMailEnabledInt
					 forPropertyNamed:SCIPropertyNameDirectSignMailEnabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameDirectSignMailEnabled
										  withScope:SCIPropertyScopeUser];
}

- (BOOL)directSignMailEnabled
{
	int directSignMailEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDirectSignMailEnabled
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:stiDIRECT_SIGNMAIL_ENABLED_DEFAULT];
	BOOL directSignMailEnabled = directSignMailEnabledInt ? YES : NO;
	return directSignMailEnabled;
}

-(void)setDisplayContactImages:(BOOL)displayContactImages
{
	[self setDisplayContactImagesPrimitive:displayContactImages];
}
- (void)setDisplayContactImagesPrimitive:(BOOL)displayContactImages
{
	int displayContactImagesInt = displayContactImages ? 1 : 0;
	[self.propertyManager setIntValue:displayContactImagesInt
					 forPropertyNamed:SCIPropertyNameDisplayContactImages
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameDisplayContactImages
										  withScope:SCIPropertyScopeUser];
}
- (BOOL)displayContactImages
{
	int displayContactImagesInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDisplayContactImages
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:1];
	BOOL displayContactImages = displayContactImagesInt ? YES : NO;
	return displayContactImages;
}

- (NSInteger)ringsBeforeGreeting
{
	uint32_t currentRings = 8;
	uint32_t maxRings;
	
	if (self.videophoneEngine) {
		self.videophoneEngine->RingsBeforeGreetingGet(&currentRings, &maxRings);
		if (currentRings > maxRings)
		{
			[self setRingsBeforeGreeting:maxRings];
			currentRings = maxRings;
		
		}
	}

	return currentRings;
}
- (void)setRingsBeforeGreeting:(NSUInteger)ringsBeforeGreeting
{
	if (self.videophoneEngine) {
		self.videophoneEngine->RingsBeforeGreetingSet((uint32_t)ringsBeforeGreeting);
	}
}

- (void)setRingsBeforeGreetingPrimitive:(NSInteger)numberOfRings {

	[self setRingsBeforeGreeting:numberOfRings];
}

- (BOOL)contactPhotosFeatureEnabled
{
	int contactPhotosFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameContactImagesEnabled
																	  inStorageLocation:SCIPropertyStorageLocationPersistent
																			withDefault:stiCONTACT_PHOTOS_ENABLED_DEFAULT];
	BOOL contactPhotosFeatureEnabled = (contactPhotosFeatureEnabledInt) ? YES : NO;
	return contactPhotosFeatureEnabled;
}

- (BOOL)isExportContactsFeatureEnabled
{
	return [self.propertyManager intValueForPropertyNamed:SCIPropertyNameExportContactsEnabled
										inStorageLocation:SCIPropertyStorageLocationPersistent
											  withDefault:1];
}

- (BOOL)personalSignMailGreetingFeatureEnabled
{
	int personalSignMailGreetingFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNamePersonalGreetingEnabled
																		  inStorageLocation:SCIPropertyStorageLocationPersistent
																				withDefault:stiPSMG_ENABLED_DEFAULT];
	BOOL personalSignMailGreetingFeatureEnabled = (personalSignMailGreetingFeatureEnabledInt) ? YES : NO;
	return personalSignMailGreetingFeatureEnabled;
}

- (SCIProfileImagePrivacyLevel)profileImagePrivacyLevel
{
	int profileImagePrivacyLevelInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameProfileImagePrivacyLevel
																		 inStorageLocation:SCIPropertyStorageLocationPersistent
																		 withDefault:0];
	
	SCIProfileImagePrivacyLevel profileImagePrivacyLevel = (SCIProfileImagePrivacyLevel)profileImagePrivacyLevelInt;
	
	return profileImagePrivacyLevel;
}

- (void)setProfileImagePrivacyLevel:(SCIProfileImagePrivacyLevel)profileImagePrivacyLevel
{
	int profileImagePrivacyLevelInt = (int)profileImagePrivacyLevel;
	[self.propertyManager setIntValue:profileImagePrivacyLevelInt
					 forPropertyNamed:SCIPropertyNameProfileImagePrivacyLevel
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameProfileImagePrivacyLevel
											  withScope:SCIPropertyScopeUser];
}

- (void)setShowInternetSearchNagPrimitive:(BOOL)showInternetSearchNag {
	int showInternetSearchNagInt = showInternetSearchNag ? 1 : 0;
	[self.propertyManager setIntValue:showInternetSearchNagInt
					 forPropertyNamed:SCIPropertyNameShowInternetSearchNag
					inStorageLocation:SCIPropertyStorageLocationPersistent];

}

- (BOOL)showInternetSearchNag {
	int showInternetSearchNagInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameShowInternetSearchNag
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:1];
	BOOL showInternetSearchNag = (showInternetSearchNagInt) ? YES : NO;
	return showInternetSearchNag;
}

- (void)setInternetSearchAllowedPrimitive:(BOOL)allowed {
	int internetSearchAllowed = allowed ? 1 : 0;
	[self.propertyManager setIntValue:internetSearchAllowed
					 forPropertyNamed:SCIPropertyNameInternetSearchAllowed
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameInternetSearchAllowed
										  withScope:SCIPropertyScopeUser];
}

- (BOOL)internetSearchEnabled
{
	int internetSearchEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameInternetSearchEnabled
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:stiINTERNET_SEARCH_ENABLED_DEFAULT];
	BOOL internetSearchEnabled = (internetSearchEnabledInt) ? YES : NO;
	return internetSearchEnabled;
}

- (BOOL)internetSearchAllowed {
	int internetSearchAllowedInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameInternetSearchAllowed
																				 inStorageLocation:SCIPropertyStorageLocationPersistent
																					   withDefault:0];
	BOOL internetSearchAllowed = (internetSearchAllowedInt) ? YES : NO;
	return internetSearchAllowed;
}

- (BOOL)realTimeTextFeatureEnabled
{
	int realTimeTextFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameRealTimeTextEnabled
																	 inStorageLocation:SCIPropertyStorageLocationPersistent
																		   withDefault:stiREAL_TIME_TEXT_ENABLED_DEFAULT];
	BOOL realTimeTextFeatureEnabled = (realTimeTextFeatureEnabledInt) ? YES : NO;
	return realTimeTextFeatureEnabled;
}

- (BOOL)webDialFeatureEnabled
{
	int webDialFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameWebDialEnabled
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:0];
	BOOL webDialFeatureEnabled = (webDialFeatureEnabledInt) ? YES : NO;
	return webDialFeatureEnabled;
}

- (BOOL)callTransferFeatureEnabled
{
	int callTransferFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameCallTransferEnabled
																	 inStorageLocation:SCIPropertyStorageLocationPersistent
																		   withDefault:stiCALL_TRANSFER_ENABLED_DEFAULT];
	BOOL callTransferFeatureEnabled = (callTransferFeatureEnabledInt) ? YES : NO;
	return callTransferFeatureEnabled;
}

- (BOOL)dynamicAgreementsFeatureEnabled
{
	int dynamicAgreementsFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameAgreementsEnabled
																		  inStorageLocation:SCIPropertyStorageLocationPersistent
																				withDefault:stiAGREEMENTS_ENABLED_DEFAULT];
	BOOL dynamicAgreementsFeatureEnabled = (dynamicAgreementsFeatureEnabledInt) ? YES : NO;
	return dynamicAgreementsFeatureEnabled;
}

- (BOOL)favoritesFeatureEnabled
{
	int favoritesFeatureEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameFavoritesEnabled
																		  inStorageLocation:SCIPropertyStorageLocationPersistent
																				withDefault:stiFAVORITES_ENABLED_DEFAULT];
	BOOL favoritesFeatureEnabled = (favoritesFeatureEnabledInt) ? YES : NO;
	return favoritesFeatureEnabled;
}

- (BOOL)groupVideoChatEnabled
{
	int groupVideoChatEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameGroupVideoChatEnabled
																  inStorageLocation:SCIPropertyStorageLocationPersistent
																		withDefault:stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT];
	BOOL groupVideoChatEnabled = (groupVideoChatEnabledInt) ? YES : NO;
	return groupVideoChatEnabled;
}

- (BOOL)LDAPDirectoryEnabled
{
	int LDAPDirectoryEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLDAPDirectoryEnabled
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:stiLDAP_ENABLED_DEFAULT];
	BOOL LDAPDirectoryEnabled = (LDAPDirectoryEnabledInt) ? YES : NO;
	return LDAPDirectoryEnabled;
}

- (NSString *)LDAPDirectoryDisplayName
{
	NSString *LDAPDirectoryDisplayName = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameLDAPDirectoryDisplayName
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:@"Directory"];
	return LDAPDirectoryDisplayName;
}


- (NSNumber *)LDAPServerRequiresAuthentication
{
	NSNumber *res = nil;
    res = [NSNumber numberWithUnsignedInteger:self.LDAPServerRequiresAuthenticationPrimitive];
    return res;
}
- (int)LDAPServerRequiresAuthenticationPrimitive
{
	int LDAPServerRequiresAuthenticationPrimitiveInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLDAPServerRequiresAuthentication
																	  inStorageLocation:SCIPropertyStorageLocationPersistent
																			withDefault:0];
	return LDAPServerRequiresAuthenticationPrimitiveInt;
}


- (BOOL)blockCallerIDEnabled
{
	int blockCallerIDEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameBlockCallerIDEnabled
																inStorageLocation:SCIPropertyStorageLocationPersistent
																	  withDefault:stiBLOCK_CALLER_ID_ENABLED_DEFAULT];
	BOOL blockCallerIDEnabled = (blockCallerIDEnabledInt) ? YES : NO;
	return blockCallerIDEnabled;
}

- (NSString *)macAddress
{
	NSString *address = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameMacAddress inStorageLocation:SCIPropertyStorageLocationPersistent withDefault:@""];
	return address;
}

- (void)setSignMailGreetingPreferencePrimitive:(SCISignMailGreetingType)signMailGreetingPreference
{
	[self setSignMailGreetingPreferencePrimitive:signMailGreetingPreference send:YES];
}
- (void)setSignMailGreetingPreferencePrimitive:(SCISignMailGreetingType)signMailGreetingPreference send:(BOOL)send
{
	int signMailGreetingPreferenceInt = (int)signMailGreetingPreference;
	[self.propertyManager setIntValue:signMailGreetingPreferenceInt
					 forPropertyNamed:SCIPropertyNameGreetingPreference
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	// Set the SCIPropertyNamePersonalGreetingType property if the signMailGreetingPreference is a PSMG
	if (SCISignMailGreetingTypeIsPersonal(signMailGreetingPreference)) {
		[self.propertyManager setIntValue:signMailGreetingPreferenceInt
						 forPropertyNamed:SCIPropertyNamePersonalGreetingType
						inStorageLocation:SCIPropertyStorageLocationPersistent];
	}

	if (send) {
		[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameGreetingPreference
											  withScope:SCIPropertyScopeUser];
		
		// Set the SCIPropertyNamePersonalGreetingType property if the signMailGreetingPreference is a PSMG
		if (SCISignMailGreetingTypeIsPersonal(signMailGreetingPreference)) {
			[self.propertyManager sendValueForPropertyNamed:SCIPropertyNamePersonalGreetingType
												  withScope:SCIPropertyScopeUser];
		}

	}
}
- (SCISignMailGreetingType)signMailGreetingPreference
{
	int greetingPreferenceInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameGreetingPreference
															 inStorageLocation:SCIPropertyStorageLocationPersistent
																   withDefault:(int)SCISignMailGreetingTypeDefault];
	SCISignMailGreetingType signMailGreetingPreference = (SCISignMailGreetingType)greetingPreferenceInt;
	
	int personalGreetingTypeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNamePersonalGreetingType
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:(int)SCISignMailGreetingTypeDefault];
	SCISignMailGreetingType personalGreetingType = (SCISignMailGreetingType)personalGreetingTypeInt;
	
	return (SCISignMailGreetingTypeIsPersonal(signMailGreetingPreference)) ? personalGreetingType : signMailGreetingPreference;
}

- (SCISignMailGreetingType)signMailPersonalGreetingType
{
	int personalGreetingTypeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNamePersonalGreetingType
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:(int)SCISignMailGreetingTypePersonalVideoOnly];
	SCISignMailGreetingType personalGreetingType = (SCISignMailGreetingType)personalGreetingTypeInt;
	return personalGreetingType;
}

- (NSDate *)lastCIRCallTime
{
	int lastCIRCallTimeInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameLastCIRCallTime
														  inStorageLocation:SCIPropertyStorageLocationPersistent
																withDefault:0];
	NSDate *lastCIRCallTime = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)lastCIRCallTimeInt];
	return lastCIRCallTime;
}
- (void)setLastCIRCallTimePrimitive:(NSDate *)lastCIRCallTime
{
	int lastCIRCallTimeInt = (int)[lastCIRCallTime timeIntervalSince1970];
	[self.propertyManager setIntValue:lastCIRCallTimeInt
					 forPropertyNamed:SCIPropertyNameLastCIRCallTime
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameLastCIRCallTime
										  withScope:SCIPropertyScopeUser];
}

- (void)setVoiceCarryOverType:(SCIVoiceCarryOverType)voiceCarryOverType
{
	EstiVcoType vcoType = EstiVcoTypeFromSCIVoiceCarryOverType(voiceCarryOverType);
	int vcoPreferenceInt = (int)vcoType;
	[self.propertyManager setIntValue:vcoPreferenceInt
					 forPropertyNamed:SCIPropertyNameVCOPreference
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameVCOPreference
										  withScope:SCIPropertyScopePhone];
	
	[self updateEngineForVCOType:voiceCarryOverType];
}
- (SCIVoiceCarryOverType)voiceCarryOverType
{
	int vcoPreferenceInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameVCOPreference
														inStorageLocation:SCIPropertyStorageLocationPersistent
															  withDefault:(int)estiVCO_NONE];
	SCIVoiceCarryOverType voiceCarryOverType = SCIVoiceCarryOverTypeFromEstiVcoType((EstiVcoType)vcoPreferenceInt);
	return voiceCarryOverType;
}

- (void)updateEngineForVCOType:(SCIVoiceCarryOverType)voiceCarryOverType
{
	EstiVcoType vcoType = EstiVcoTypeFromSCIVoiceCarryOverType(voiceCarryOverType);
	self.videophoneEngine->VCOTypeSet(vcoType);
}

- (void)setVcoActive:(BOOL)vcoActive
{
	[self setVcoActivePrimitive:vcoActive];
}

- (void)setVcoActivePrimitive:(BOOL)vcoActive
{
	int vcoActiveInt = vcoActive ? 1 : 0;
	[self.propertyManager setIntValue:vcoActiveInt
					 forPropertyNamed:SCIPropertyNameVCOActive
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameVCOActive
										  withScope:SCIPropertyScopePhone];
	
	self.videophoneEngine->VCOUseSet(vcoActive ?  estiTRUE : estiFALSE);
}

- (BOOL)vcoActive {
	int vcoActive = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameVCOActive
												 inStorageLocation:SCIPropertyStorageLocationPersistent
													   withDefault:stiVCO_ACTIVE_DEFAULT];
	return (vcoActive == 1);
}

- (void)setVoiceCarryOverNumber:(NSString *)voiceCarryOverNumber
{
	[self.propertyManager setStringValue:voiceCarryOverNumber
						forPropertyNamed:SCIPropertyNameVoiceCallbackNumber
					   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameVoiceCallbackNumber
										  withScope:SCIPropertyScopePhone];
	
	
	const char *pszVoiceCarryOverNumber = voiceCarryOverNumber.UTF8String;
	self.videophoneEngine->VCOCallbackNumberSet(pszVoiceCarryOverNumber);
}
- (NSString *)voiceCarryOverNumber
{
	NSString *voiceCarryOverNumber = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameVoiceCallbackNumber
																	 inStorageLocation:SCIPropertyStorageLocationPersistent
																		   withDefault:@""];
	return voiceCarryOverNumber;
}

- (NSString *)coreServerAddress
{
	NSString *coreServerAddress = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameCoreServiceURL1
																  inStorageLocation:SCIPropertyStorageLocationTemporary
																		withDefault:@""];
	return coreServerAddress;
}

- (BOOL)vrclEnabled
{
	int vrclEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameVRCLEnabled
															   inStorageLocation:SCIPropertyStorageLocationPersistent
																	 withDefault:0];
	BOOL vrclEnabled = vrclEnabledInt ? YES : NO;
	return vrclEnabled;
}

-(void)setVrclEnabled:(BOOL)vrclEnabled
{
	[self setVrclEnabledPrimitive:vrclEnabled];
}

- (void)setVrclEnabledPrimitive:(BOOL)vrclEnabled
{
	int vrclEnabledInt = vrclEnabled ? 15327 : 0;  // This value is the VRCL port number.  Setting to 0 to disable.
	[self.propertyManager setIntValue:vrclEnabledInt
					 forPropertyNamed:SCIPropertyNameVRCLEnabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	self.videophoneEngine->VRCLPortSet(vrclEnabledInt);
}

- (BOOL)H265Enabled
{
	int H265EnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameH265Enabled
													  inStorageLocation:SCIPropertyStorageLocationPersistent
															withDefault:0];
	BOOL H265Enabled = H265EnabledInt ? YES : NO;
	return H265Enabled;
}

- (void)setH265Enabled:(BOOL)H265Enabled
{
	[self setH265EnabledPrimitive:H265Enabled];
}

- (void)setH265EnabledPrimitive:(BOOL)H265Enabled
{
	int H265EnabledInt = H265Enabled ? 1 : 0;
	[self.propertyManager setIntValue:H265EnabledInt
					 forPropertyNamed:SCIPropertyNameH265Enabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	SCICaptureController.shared.enableHEVC = H265Enabled;
	SCIDisplayController.shared.enableHEVC = H265Enabled;
}

- (BOOL)captureWidescreenEnabled
{
	int captureWidescreenEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameCaptureWidescreenEnabled
													  inStorageLocation:SCIPropertyStorageLocationPersistent
															withDefault:0];
	BOOL captureWidescreenEnabled = captureWidescreenEnabledInt ? YES : NO;
	return captureWidescreenEnabled;
}

- (void)setCaptureWidescreenEnabled:(BOOL)captureWidescreenEnabled
{
	[self setCaptureWidescreenEnabledPrimitive:captureWidescreenEnabled];
}

- (void)setCaptureWidescreenEnabledPrimitive:(BOOL)captureWidescreenEnabled
{
	int captureWidescreenEnabledInt = captureWidescreenEnabled ? 1 : 0;
	[self.propertyManager setIntValue:captureWidescreenEnabledInt
					 forPropertyNamed:SCIPropertyNameCaptureWidescreenEnabled
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	SCICaptureController.shared.preferWidescreen = captureWidescreenEnabled;
}

- (BOOL)debugEnabled
{
	int debugEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNameDebugEnabled
													  inStorageLocation:SCIPropertyStorageLocationPersistent
															withDefault:0];
	BOOL debugEnabled = debugEnabledInt ? YES : NO;
	return debugEnabled;
}

- (BOOL)SpanishFeatures
{
	NSString * spanishFeaturesString = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameSpanishFeatures
																		inStorageLocation:SCIPropertyStorageLocationPersistent
																			  withDefault:@"EN"];
	BOOL languageFeatures = NO;
	if ([spanishFeaturesString isEqualToString:@"EN"]) {
		languageFeatures = NO;
	} else if ([spanishFeaturesString isEqualToString:@"ES"]) {
		languageFeatures = YES;
	}
	
	return languageFeatures;
}

-(void)setSpanishFeatures:(BOOL)SpanishFeatures
{
	[self setSpanishFeaturesPrimitive:SpanishFeatures];
}

- (void)setSpanishFeaturesPrimitive:(BOOL)SpanishFeatures
{
	NSString * spanishFeaturesString = SpanishFeatures ? @"ES" : @"EN";
	[self.propertyManager setStringValue:spanishFeaturesString
					 forPropertyNamed:SCIPropertyNameSpanishFeatures
					   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameSpanishFeatures withScope:SCIPropertyScopeUser];
}

- (void)setSavedTextsCountPrimitive:(NSInteger)count
{
	[self.propertyManager setIntValue:(int)count
					 forPropertyNamed:SCIPropertyNameSavedTextsCount
					inStorageLocation:SCIPropertyStorageLocationPersistent];
	
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameSavedTextsCount
										  withScope:SCIPropertyScopeUser];
}

- (BOOL)PulseEnabled {
	int pulseEnabledInt = [self.propertyManager intValueForPropertyNamed:SCIPropertyNamePulseEnabled
													   inStorageLocation:SCIPropertyStorageLocationPersistent
															 withDefault:stiPULSE_ENABLED_DEFAULT];
	return (pulseEnabledInt == 1);
}

- (BOOL)VRIFeatures
{
	NSString * vriFeaturesString = [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameVRIFeatures
																   inStorageLocation:SCIPropertyStorageLocationPersistent
																		 withDefault:@"0"];
	return [vriFeaturesString isEqualToString:@"1"];
}

#pragma mark - Property Change Notifications

- (NSArray *)namesOfPropertiesToObserve
{
	static NSArray *namesOfPropertiesToObserve =
  @[SCIPropertyNameLastChannelMessageViewTime,
	SCIPropertyNameLastMessageViewTime,
	SCIPropertyNameLastMissedTime,
	SCIPropertyNameRingGroupEnabled,
	SCIPropertyNameCallerIDNumber,
	SCIPropertyNameProviderAgreementStatus,
	SCIPropertyNameVideoMessageEnabled,
	SCIPropertyNameRingGroupDisplayMode,
	SCIPropertyNameDirectSignMailEnabled,
	SCIPropertyNameDisplayContactImages,
	SCIPropertyNameContactImagesEnabled,
	SCIPropertyNameExportContactsEnabled,
	SCIPropertyNameProfileImagePrivacyLevel,
	SCIPropertyNameGreetingPreference,
	SCIPropertyNamePersonalGreetingType,
	SCIPropertyNameLastCIRCallTime,
	SCIPropertyNameGreetingText,
	SCIPropertyNamePersonalGreetingEnabled,
	SCIPropertyNameDTMFEnabled,
	SCIPropertyNameRealTimeTextEnabled,
	SCIPropertyNameWebDialEnabled,
	SCIPropertyNameCallTransferEnabled,
	SCIPropertyNameAgreementsEnabled,
	SCIPropertyNameDoNotDisturbEnabled,
	SCIPropertyNameFavoritesEnabled,
	SCIPropertyNameGroupVideoChatEnabled,
	SCIPropertyNameSpanishFeatures,
	SCIPropertyNameLDAPDirectoryEnabled,
	SCIPropertyNameLDAPDirectoryDisplayName,
	SCIPropertyNameLDAPServerRequiresAuthentication,
	SCIPropertyNameBlockAnonymousCallers,
	SCIPropertyNameBlockCallerIDEnabled,
	SCIPropertyNameBlockCallerID,
	SCIPropertyNameVCOPreference,
	SCIPropertyNameVCOActive,
	SCIPropertyNameInternetSearchEnabled,
	SCIPropertyNameCaptureWidescreenEnabled,
	SCIPropertyNameSecureCallMode];
	return namesOfPropertiesToObserve;
}

- (void)registerStandardPropertyManagerChangeNotifications
{
	for (NSString *propertyName in self.namesOfPropertiesToObserve) {
		[self requestPropertyChangedNotificationsForProperty:propertyName.UTF8String];
	}
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyPropertyChanged:) name:SCINotificationPropertyChanged object:self];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyLoggedIn:) name:SCINotificationLoggedIn object:self];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyAuthenticatedDidChange:) name:SCINotificationAuthenticatedDidChange object:self];
	// Because we are in a category we don't have a good way of removing this observer, but that should be okay since we are the only one that sends this notification.
}

- (void)notifyPropertyChanged:(NSNotification *)note // SCINotificationPropertyChanged
{
	NSString *propertyName = note.userInfo[SCINotificationKeyName];
	SCILog(@"PropertyManager property %@ changed.", propertyName);
	if ([self shouldNotifyPropertyChangedForPropertyNamed:propertyName]) {
		[self willChangeValueForPropertyName:propertyName];
		[self setValueIsCurrent:YES forPropertyNamed:propertyName];
		[self didChangeValueForPropertyName:propertyName];
	}
}

- (NSArray *)namesOfPropertiesToFetch
{
	static NSArray *namesOfPropertiesToFetch =
	@[
	  SCIPropertyNameProviderAgreementStatus,
	  SCIPropertyNameLastChannelMessageViewTime,
	  SCIPropertyNameLastMessageViewTime,
	  SCIPropertyNameLastMissedTime,
	  SCIPropertyNameDirectSignMailEnabled,
	  SCIPropertyNameDisplayContactImages,
	  SCIPropertyNameProfileImagePrivacyLevel,
	  SCIPropertyNameGreetingText,
	  SCIPropertyNamePersonalGreetingType,
	  SCIPropertyNameVideoMessageEnabled,
	  SCIPropertyNamePulseEnabled,
	  SCIPropertyNameWebDialEnabled,
	  SCIPropertyNameDoNotDisturbEnabled,
	  SCIPropertyNameSpanishFeatures,
	  SCIPropertyNameLDAPDirectoryDisplayName,
	  SCIPropertyNameVCOPreference,
	  SCIPropertyNameVCOActive,
	  SCIPropertyNameVRIFeatures,
	  SCIPropertyNameDeafHearingVideoEnabled,
	  SCIPropertyNameSecureCallMode
	  ];
	return namesOfPropertiesToFetch;
}

- (NSArray *)namesOfPropertiesNotToSend
{
	static NSArray *namesOfPropertiesNotToSend = @[SCIPropertyNameVRIFeatures, SCIPropertyNameLDAPDirectoryDisplayName];
	return namesOfPropertiesNotToSend;
}

- (NSSet *)namesOfPropertiesNotToExpunge
{
	static NSSet *namesOfPropertiesNotToExpunge = [NSSet setWithArray:@[SCIPropertyNameDoNotDisturbEnabled]];
	return namesOfPropertiesNotToExpunge;
}

- (void)notifyLoggedIn:(NSNotification *)note // SCINotificationLoggedIn
{
	[self setCallWaitingEnabled:[self callWaitingEnabled]];
	if (self.interfaceMode == SCIInterfaceModePublic) {
		[self setAudioEnabled:NO];
		
		// Disable VCO in Public mode
		self.voiceCarryOverType = SCIVoiceCarryOverTypeNone;
	} else {
		
		BOOL audioEnabled = [self audioEnabled] || (self.voiceCarryOverType == SCIVoiceCarryOverTypeOneLine);
		[self setAudioEnabled:audioEnabled];
		if (audioEnabled && (self.voiceCarryOverType == SCIVoiceCarryOverTypeNone)) {
			[self setVoiceCarryOverType:SCIVoiceCarryOverTypeOneLine];
			[self.propertyManager sendProperties];
		}
	}
	
	NSMutableDictionary *oldValues = [NSMutableDictionary dictionaryWithCapacity:self.namesOfPropertiesToFetch.count];
	NSSet *namesOfPropertiesNotToExpunge = self.namesOfPropertiesNotToExpunge;
	
	for (NSString *propertyName in self.namesOfPropertiesToFetch) {
		[self willChangeValueForPropertyName:propertyName];
		[self setValueIsCurrent:NO forPropertyNamed:propertyName];
		[self didChangeValueForPropertyName:propertyName];
		
		BOOL shouldExpunge = ![namesOfPropertiesNotToExpunge containsObject:propertyName];
		if (shouldExpunge)
		{
			[self expungeCachedValueForPropertyNamed:propertyName];
		}
		
		id oldValue = [[SCIPropertyManager sharedManager] valueOfType:SCIPropertyTypeForPropertyName(propertyName)
													 forPropertyNamed:propertyName
													inStorageLocation:SCIPropertyStorageLocationPersistent
														  withDefault:SCIDefaultValueForPropertyName(propertyName)];
		[oldValues setObject:oldValue forKey:propertyName];		
	}

	[self fetchSettings:self.namesOfPropertiesToFetch completion:^(NSArray *settingList, NSError *error) {
		if (!error) {
			NSMutableArray *propertiesInCore = [NSMutableArray array];
			for (SCISettingItem *setting in settingList) {
				[propertiesInCore addObject:setting.name];
				
				[self setValueIsCurrent:YES forPropertyNamed:setting.name];
				id newValue = setting.value;
				
				// If the new value is different from the old value, we will receive SCINotificationPropertyChanged.  However, if
				// the new value is equal to the old value, we will not receive the notification.  In that case, we need to make
				// the KVO announcement ourselves.
				if ([newValue isEqual:[oldValues objectForKey:setting.name]]) {
					[self willChangeValueForPropertyName:setting.name];
					[self didChangeValueForPropertyName:setting.name];
				}
			}
			
			NSPredicate *relativeComplementPredicate = [NSPredicate predicateWithFormat:@"NOT SELF IN %@ AND NOT SELF IN %@", propertiesInCore, self.namesOfPropertiesNotToSend];
			NSArray *propertiesNotInCore = [self.namesOfPropertiesToFetch filteredArrayUsingPredicate:relativeComplementPredicate];
			for (NSString *propertyName in propertiesNotInCore) {
				[self setValueIsCurrent:YES forPropertyNamed:propertyName];
				[[SCIPropertyManager sharedManager] sendValueForPropertyNamed:propertyName withScope:SCIPropertyScopeForPropertyName(propertyName)];
			}
		}
	}];

}

- (void)notifyAuthenticatedDidChange:(NSNotification *)note // SCINotificationAuthenticatedDidChange
{
	if (!self.isAuthenticated) {
		for (NSString *propertyName in self.namesOfPropertiesToFetch) {
			if (SCIPropertyScopeForPropertyName(propertyName) == SCIPropertyScopeUser) { // Only expunge User properties.
				[self expungeCachedValueForPropertyNamed:propertyName];
			}
		}
	}
}

@end

NSString * const SCIKeyAudioEnabled = @"audioEnabled";
NSString * const SCIKeyBlockAnonymousCallers = @"blockAnonymousCallers";
NSString * const SCIKeyBlockCallerIDEnabled = @"blockCallerIDEnabled";
NSString * const SCIKeyBlockCallerID = @"blockCallerID";
NSString * const SCIKeyCallerIDNumberType = @"callerIDNumberType";
NSString * const SCIKeyCallTransferFeatureEnabled = @"callTransferFeatureEnabled";
NSString * const SCIKeyCallWaitingEnabled = @"callWaitingEnabled";
NSString * const SCIKeyContactPhotosFeatureEnabled = @"contactPhotosFeatureEnabled";
NSString * const SCIKeyExportContactsFeatureEnabled = @"exportContactsFeatureEnabled";
NSString * const SCIKeyCoreServerAddress = @"coreServerAddress";
NSString * const SCIKeyDirectSignMailEnabled = @"directSignMailEnabled";
NSString * const SCIKeyDisplayContactImages = @"displayContactImages";
NSString * const SCIKeyDoNotDisturbEnabled = @"doNotDisturbEnabled";
NSString * const SCIKeyDTMFEnabled = @"DTMFEnabled";
NSString * const SCIKeyDynamicAgreementsFeatureEnabled = @"dynamicAgreementsFeatureEnabled";
NSString * const SCIKeyFavoritesFeatureEnabled = @"favoritesFeatureEnabled";
NSString * const SCIKeyGroupVideoChatEnabled = @"groupVideoChatEnabled";
NSString * const SCIKeyInternetSearchAllowed = @"internetSearchAllowed";
NSString * const SCIKeyInternetSearchEnabled = @"internetSearchEnabled";
NSString * const SCIKeyShowInternetSearchNag = @"showInternetSearchNag";
NSString * const SCIKeySpanishFeatures = @"SpanishFeatures";
NSString * const SCIKeyLastChannelMessageViewTime = @"lastChannelMessageViewTime";
NSString * const SCIKeyLastCIRCallTime = @"lastCIRCallTime";
NSString * const SCIKeyLastMessageViewTime = @"lastMessageViewTime";
NSString * const SCIKeyLastMissedViewTime = @"lastMissedViewTime";
NSString * const SCIKeyLDAPDirectoryEnabled = @"LDAPDirectoryEnabled";
NSString * const SCIKeyLDAPDirectoryDisplayName = @"LDAPDirectoryDisplayName";
NSString * const SCIKeyLDAPServerRequiresAuthentication = @"LDAPServerRequiresAuthentication";
NSString * const SCIKeyPersonalGreetingText = @"personalGreetingText";
NSString * const SCIKeyPersonalSignMailGreetingFeatureEnabled = @"personalSignMailGreetingFeatureEnabled";
NSString * const SCIKeyProfileImagePrivacyLevel = @"profileImagePrivacyLevel";
NSString * const SCIKeyProviderAgreementAccepted = @"providerAgreementAccepted";
NSString * const SCIKeyRealTimeTextFeatureEnabled = @"realTimeTextFeatureEnabled";
NSString * const SCIKeyRingGroupDisplayModeOrNil = @"ringGroupDisplayModeOrNil";
NSString * const SCIKeyRingGroupEnabled = @"ringGroupEnabled";
NSString * const SCIKeyRingsBeforeGreeting = @"ringsBeforeGreeting";
NSString * const SCIKeySecureCallMode = @"secureCallMode";
NSString * const SCIKeySignMailGreetingPreference = @"signMailGreetingPreference";
NSString * const SCIKeySignMailPersonalGreetingType = @"signMailPersonalGreetingType";
NSString * const SCIKeyTunnelingEnabled = @"tunnelingEnabled";
NSString * const SCIKeyVideoMessageEnabled = @"videoMessageEnabled";
NSString * const SCIKeyVoiceCarryOverNumber = @"voiceCarryOverNumber";
NSString * const SCIKeyVoiceCarryOverType = @"voiceCarryOverType";
NSString * const SCIKeyVCOActive = @"vcoActive";
NSString * const SCIKeyVrclEnabled = @"vrclEnabled";
NSString * const SCIKeyWebDialFeatureEnabled = @"webDialFeatureEnabled";
NSString * const SCIKeySavedTextsCount = @"savedTextsCount";
NSString * const SCIKeyCaptureWidescreenEnabled = @"CaptureWidescreenEnabled";
NSString * const SCIKeyH265Enabled = @"H265Enabled";
NSString * const SCIKeyDebugEnabled = @"debugEnabled";
NSString * const SCIKeyVRIFeatures = @"VRIFeatures";

@implementation SCIVideophoneEngine (UserInterfacePropertiesInternal)

- (NSString *)providerAgreementStatusString
{
	return [self.propertyManager stringValueForPropertyNamed:SCIPropertyNameProviderAgreementStatus
										   inStorageLocation:SCIPropertyStorageLocationPersistent
												 withDefault:SCIDefaultValueForPropertyName(SCIPropertyNameProviderAgreementStatus)];
}
- (void)setProviderAgreementStatusStringPrimitive:(NSString *)providerAgreementStatusString
{
	[self.propertyManager setStringValue:providerAgreementStatusString
						forPropertyNamed:SCIPropertyNameProviderAgreementStatus
					   inStorageLocation:SCIPropertyStorageLocationPersistent];
	[self.propertyManager sendValueForPropertyNamed:SCIPropertyNameProviderAgreementStatus withScope:SCIPropertyScopeUser];
}

+ (NSSet *)keyPathsForValuesAffectingProviderAgreementStatus
{
	return [NSSet setWithObjects:SCIKeyProviderAgreementStatusString, nil];
}
- (SCIStaticProviderAgreementStatus *)providerAgreementStatus
{
	return [SCIStaticProviderAgreementStatus providerAgreementStatusFromString:self.providerAgreementStatusString];
}
- (void)setProviderAgreementStatusPrimitive:(SCIStaticProviderAgreementStatus *)providerAgreementStatus
{
	[self setProviderAgreementStatusStringPrimitive:[providerAgreementStatus createProviderAgreementStatusStringForCurrentSoftwareVersionString:self.softwareVersion]];
}

- (BOOL)providerAgreementStatusStringIsAcceptance:(NSString *)string
{
	return (SCIStaticProviderAgreementStateFromProviderAgreementStatusString(string, self.softwareVersion) == SCIStaticProviderAgreementStateAccepted);
}

- (NSString *)providerAgreementStatusString:(BOOL)accepted
{
	return SCIStaticProviderAgreementStatusStringFromProviderAgreementState(accepted ? SCIStaticProviderAgreementStateAccepted : SCIStaticProviderAgreementStateCancelled, nil, self.softwareVersion);
}

@end

NSString * const SCIKeyProviderAgreementStatusString = @"providerAgreementStatusString";
NSString * const SCIKeyProviderAgreementStatus = @"providerAgreementStatus";

@implementation SCIVideophoneEngine (UserInterfacePropertiesPrivate)

static NSString * const SCIVideophoneEngineUserInterfacePropertiesCurrentForValuesOfNamedProperties = @"SCIVideophoneEngineUserInterfacePropertiesCurrentForValuesOfNamedProperties";
- (NSMutableDictionary *)currentForValuesOfNamedProperties
{
	return [self bnr_associatedObjectGeneratedBy:^id{return [[NSMutableDictionary alloc] init];} forKey:SCIVideophoneEngineUserInterfacePropertiesCurrentForValuesOfNamedProperties];
}
- (BOOL)valueForPropertyNamedIsCurrent:(NSString *)propertyName
{
	BOOL res = NO;
	
	if (propertyName) {
		NSNumber *number = self.currentForValuesOfNamedProperties[propertyName];
		res = [number boolValue];
	}
	
	return res;
}
- (void)setValueIsCurrent:(BOOL)current forPropertyNamed:(NSString *)propertyName
{
	NSNumber *number = [NSNumber numberWithBool:current];
	self.currentForValuesOfNamedProperties[propertyName] = number;
}

static NSString * const SCIVideophoneEngineUserInterfacePropertiesExpungingCachedValuesOfNamedProperties = @"SCIVideophoneEngineUserInterfacePropertiesExpungingCachedValuesOfNamedProperties";
- (NSMutableDictionary *)expungingCachedValuesOfNamedProperties
{
	return [self bnr_associatedObjectGeneratedBy:^id{return [[NSMutableDictionary alloc] init];} forKey:SCIVideophoneEngineUserInterfacePropertiesExpungingCachedValuesOfNamedProperties];
}
- (BOOL)isExpungingCachedValueForPropertyNamed:(NSString *)propertyName
{
	BOOL res = NO;
	
	if (propertyName) {
		NSNumber *number = self.expungingCachedValuesOfNamedProperties[propertyName];
		res = [number boolValue];
	}
	
	return res;
}
- (void)setExpungingCachedValue:(BOOL)expunging forPropertyNamed:(NSString *)propertyName
{
	if (propertyName) {
		NSNumber *number = [NSNumber numberWithBool:expunging];
		self.expungingCachedValuesOfNamedProperties[propertyName] = number;
	}
}

static NSString * const SCIVideophoneEngineUserInterfacePropertiesShouldNotifyPropertyChangedForNamedProperties = @"SCIVideophoneEngineUserInterfacePropertiesShouldNotifyPropertyChangedForNamedProperties";
- (NSMutableDictionary *)shouldNotifyPropertyChangedForNamedProperties
{
	return [self bnr_associatedObjectGeneratedBy:^id{return [[NSMutableDictionary alloc] init];} forKey:SCIVideophoneEngineUserInterfacePropertiesShouldNotifyPropertyChangedForNamedProperties];
}
- (BOOL)shouldNotifyPropertyChangedForPropertyNamed:(NSString *)propertyName
{
	BOOL res = NO;
	
	if (propertyName) {
		if (![self isExpungingCachedValueForPropertyNamed:propertyName]) {
			NSNumber *number = self.shouldNotifyPropertyChangedForNamedProperties[propertyName];
			if (number) {
				res = [number boolValue];
			} else {
				//Default to YES.
				res = YES;
			}
		} else {
			res = NO;
		}
	}
	
	return res;
}
- (void)setShouldNotifyPropertyChanged:(BOOL)shouldNotifyPropertyChanged forPropertyNamed:(NSString *)propertyName
{
	if (propertyName) {
		NSNumber *number = [NSNumber numberWithBool:shouldNotifyPropertyChanged];
		self.shouldNotifyPropertyChangedForNamedProperties[propertyName] = number;
	}
}

- (void)expungeCachedValueForPropertyNamed:(NSString *)propertyName
{
	[[SCIPropertyManager sharedManager] setValue:SCIDefaultValueForPropertyName(propertyName)
										  ofType:SCIPropertyTypeForPropertyName(propertyName)
								forPropertyNamed:propertyName
							   inStorageLocation:SCIPropertyStorageLocationPersistent
								   preStoreBlock:^(id oldValue, id newValue) {
									   [self setExpungingCachedValue:YES forPropertyNamed:propertyName];
								   }
								  postStoreBlock:^(id oldValue, id newValue) {
									  [self setExpungingCachedValue:NO forPropertyNamed:propertyName];
								  }];
}

- (SCIPropertyManager *)propertyManager
{
	return [SCIPropertyManager sharedManager];
}

- (void)willChangeValueForPropertyName:(NSString *)propertyName
{
	NSArray *keys = SCIVideophoneEngineKeysForPropertyName(propertyName);
	for (NSString *key in keys) {
		[self willChangeValueForKey:key];
	}
}

- (void)didChangeValueForPropertyName:(NSString *)propertyName
{
	NSArray *keys = SCIVideophoneEngineKeysForPropertyName(propertyName);
	for (NSString *key in keys) {
		[self didChangeValueForKey:key];
	}
}


@end

