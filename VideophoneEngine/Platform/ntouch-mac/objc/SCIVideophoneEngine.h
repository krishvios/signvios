//
//  SCIVideophoneEngine.h
//  ntouchMac
//
//  Created by Adam Preble on 2/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#pragma once

#import <Foundation/Foundation.h>

@class SCICall;
@class SCICallListItem;
@class SCIUserAccountInfo;
@class SCIEmergencyAddress;
@class SCIUserPhoneNumbers;
@class SCIItemId;
@class SCIMessageInfo;
@class SCIMessageCategory;
@class SCIContact;
@class SCIPersonalGreetingPreferences;
@class SCIDynamicProviderAgreementStatus;
@class SCIDynamicProviderAgreement;
@class SCICoreVersion;

typedef NS_ENUM(NSInteger, SCIVoiceCarryOverType) {
	SCIVoiceCarryOverTypeNone = 0,
	SCIVoiceCarryOverTypeOneLine = 1,
	SCIVoiceCarryOverTypeTwoLine = 2,
	SCIVoiceCarryOverTypeUnknown = INT_MAX,
};

typedef NS_ENUM(NSUInteger, SCIInterfaceMode) {
	SCIInterfaceModeStandard,
	SCIInterfaceModePublic,
	SCIInterfaceModeKiosk,
	SCIInterfaceModeInterpreter,
	SCIInterfaceModeTechSupport,
	SCIInterfaceModeVRI,
	SCIInterfaceModeAbusiveCaller,
	SCIInterfaceModePorted,
	SCIInterfaceModeHearing,
};

typedef NS_ENUM(NSUInteger, SCIRestartReason) {				//!< IstiPlatform::EstiRestartReason
	SCIRestartReasonUnknown = 0,		//!< estiRESTART_REASON_UNKNOWN
	SCIRestartReasonMediaLoss,			//!< estiRESTART_REASON_MEDIA_LOSS
	SCIRestartReasonVRCL,				//!< estiRESTART_REASON_VRCL
	SCIRestartReasonUpdate,				//!< estiRESTART_REASON_UPDATE
	SCIRestartReasonCrash,				//!< estiRESTART_REASON_CRASH
	SCIRestartReasonAccountTransfer,	//!< estiRESTART_REASON_ACCOUNT_TRANSFER
	SCIRestartReasonDNSChange,			//!< estiRESTART_REASON_DNS_CHANGE
	SCIRestartReasonStateNotifyEvent,	//!< estiRESTART_REASON_STATE_NOTIFY_EVENT
	SCIRestartReasonTerminated			//!< estiRESTART_REASON_TERMINATED
};

typedef NS_ENUM(NSInteger, SCIAutoSpeedMode) {
	SCIAutoSpeedModeLegacy = 0,
	SCIAutoSpeedModeLimited,
	SCIAutoSpeedModeAuto,
};

typedef NS_ENUM(NSUInteger, SCIDialSource)
{
	SCIDialSourceUnknown = 0,
	SCIDialSourceContact = 1,
	SCIDialSourceCallHistory = 2,
	SCIDialSourceSignMail = 3,
	SCIDialSourceFavorite = 4,
	SCIDialSourceAdHoc = 5,
	SCIDialSourceVRCL = 6,
	SCIDialSourceSVRSButton = 7,
	SCIDialSourceRecentCalls = 8,
	SCIDialSourceTransferred = 9,
	SCIDialSourceInternetSearch = 10,
	SCIDialSourceWebDial = 11,
	SCIDialSourcePushNotificationMissedCall = 12,
	SCIDialSourcePushNotificationSignMailCall = 13,
	SCIDialSourceLDAP = 14,
	SCIDialSourceFastSearch = 15,  //This is specific to mac and pc it searches your contacts and recent calls with the dial pad
	SCIDialSourceMyPhone = 16, //This is when performing Add Call or Call Transfer to a MyPhone Group number (intercom)
	SCIDialSourceUIButton = 17, // This is making a call from any UI Button like a Call CIR Dialog or Techsupport in settings.
	SCIDialSourceSVRSEnglishContact = 18,
	SCIDialSourceSVRSSpanishContact = 19,
	SCIDialSourceDeviceContact = 20,
	SCIDialSourceDirectSignMail = 21,
	SCIDialSourceDirectSignMailFailure = 22,
	SCIDialSourceCallHistoryDisconnected = 23,
	SCIDialSourceRecentCallsDisconnected = 24,
	SCIDialSourceNativeDialer = 25,
	SCIDialSourceBlockList = 26,
	SCIDialSourceCallWithEncryption = 27,
	SCIDialSourceDirectory = 28,
	SCIDialSourceSpanishAdHoc = 29,
	SCIDialSourceHelpUI = 30,
	SCIDialSourceLoginUI = 31,
	SCIDialSourceErrorUI = 32,
	SCIDialSourceCallCustService = 33
};

typedef NS_ENUM(NSUInteger, SCIDialMethod)
{
	SCIDialMethodByDialString = 0, /*!< User supplied dial string. */
	SCIDialMethodByDSPhoneNumber = 1, /*!< Phone number supplied, used for look up in a Directory Service like LDAP. */
	SCIDialMethodByVRSPhoneNumber = 3, /*!< Phone number supplied, used for dialing via a Video Relay Service.	*/
	SCIDialMethodByVRSWithVCO = 4, /*!< Phone number supplied, used for dialing via a Video Relay Service using Voice Carry Over. */
	SCIDialMethodUnknown = 8, /*!< Unknown dial method.  The number may be a hearing number or a deaf number. */
	//	SCIDialMethodByVideoServerNumber = 9, /*!< Number supplied is used in the called party field when calling a video server */ -- This dial type has been deprecated.
	SCIDialMethodByOtherVRSProvider = 10, /*!<Call is placed through another VRS provider */
	SCIDialMethodUnknownWithVCO = 11, /*!<Unknown dial method. The number may be a hearing number or a deaf number.  If the number is determined to be a hearing number then use VCO. */
	SCIDialMethodVRSDisconnected = 12 /*!<VRS call disconnected before dialing the customer. IVR, HoldServer, Hand-off, Interpreter. */

};

typedef NS_ENUM(NSInteger, SCIDeviceLocationType)
{
	SCIDeviceLocationTypeNotSet = -1,
	SCIDeviceLocationTypeUpdate = 0,
	SCIDeviceLocationTypeSharedWorkspace = 1,
	SCIDeviceLocationTypePrivateWorkspace = 2,
	SCIDeviceLocationTypeSharedLivingSpace = 3,
	SCIDeviceLocationTypePrivateLivingSpace = 4,
	SCIDeviceLocationTypeWorkIssuedMobileDevice = 5,
	SCIDeviceLocationTypeOtherSharedSpace = 6,
	SCIDeviceLocationTypeOtherPrivateSpace = 7,
	SCIDeviceLocationTypePrison = 8,
	SCIDeviceLocationTypeRestricted = 9,
};

@interface SCIVideophoneEngine : NSObject

@property (nonatomic) NSString *username;
@property (nonatomic) NSString *loginName;
@property (nonatomic) NSString *password;
@property (nonatomic, readonly) NSString *phoneUserId;
@property (nonatomic, readonly) NSString *ringGroupUserId;
@property (nonatomic, readonly) SCICoreVersion *coreVersion;
@property (nonatomic, strong) NSString *deviceToken;

@property (nonatomic) NSUInteger maximumSendSpeed;
@property (nonatomic) NSUInteger maximumRecvSpeed;
@property (nonatomic, readonly) NSString *publicIPAddress;
@property (nonatomic, readonly) NSString *localIPAddress;
@property (nonatomic) SCIInterfaceMode interfaceMode;
@property (nonatomic) BOOL allowIncomingCallsMode;

@property (nonatomic, strong, readonly) SCIUserAccountInfo *userAccountInfo;
@property (nonatomic, strong, readonly) SCIEmergencyAddress *emergencyAddress;

@property (nonatomic, readonly) BOOL isStarted;
@property (nonatomic, readonly) BOOL isConnecting;
@property (nonatomic, readonly) BOOL isConnected;
@property (nonatomic, readonly) BOOL isAuthenticated;
@property (nonatomic, readonly) BOOL isFetchingAuthorization;
@property (nonatomic, readonly) BOOL isAuthorizing;
@property (nonatomic, readonly) BOOL isAuthorized;
@property (nonatomic, assign) BOOL isSipRegistered;
@property (nonatomic, readonly) BOOL isLoggingOut;
@property (atomic) BOOL isLoggingIn;
#ifdef stiDEBUG
@property (nonatomic, readonly) NSString *debugStateDescription;
#endif

@property (copy) void (^loginBlock)(NSDictionary *accountStatusDictionary, NSDictionary *errors, void (^proceedBlock)(void), void (^terminateBlock)(void));

@property (nonatomic, readonly) BOOL remoteVideoPrivacyEnabled; // see also: SCINotificationRemoteVideoPrivacyChanged

@property (nonatomic, readwrite) NSString *softwareVersion;
@property (nonatomic) SCIDialSource callDialSource;
@property (nonatomic) SCIDialMethod callDialMethod;

@property (nonatomic, readonly) BOOL isIPv6Enabled;
@property (nonatomic) SCIDeviceLocationType deviceLocationType;
@property (class, readonly) SCIVideophoneEngine *sharedEngine;
- (void)launchWithOptions:(NSDictionary *)options;

- (void)answerOnVp:(NSString*)ipAddress;
- (void)callWithVP:(NSString*)ipAddress dialString:(NSString*)dialString;

- (void)startupNetwork;
- (void)startupNetworkAndUpdateNetworkSettings:(BOOL)updateNetworkSettings;
- (void)startupNetworkAndUpdateNetworkSettings:(BOOL)updateNetworkSettings completion:(void (^)(NSError *))block;
- (void)shutdownNetwork;
- (void)startup;
- (void)startupAndUpdateNetworkSettings:(BOOL)updateNetworkSettings;
- (void)startupAndUpdateNetworkSettings:(BOOL)updateNetworkSettings completion:(void (^)(NSError *))block;
- (void)updateNetworkSettingsWithCompletion:(void (^)(NSError *))block;
- (void)shutdown;
- (void)forceServiceOutageCheck;
- (void)applicationHasNetwork;
- (void)applicationHasNoNetwork;
- (void)applicationWillEnterForeground;
- (void)applicationDidEnterBackground;
- (void)setRestartReason:(SCIRestartReason)reason;

- (void)associateWithBlock:(void (^)(NSError *error))block;
- (BOOL)loginUsingCache;
- (void)loginUsingCacheWithBlock:(void (^)(NSDictionary *accountStatusDictionary, NSDictionary *errors, void (^proceedBlock)(void), void (^terminateBlock)(void)))block;
- (void)loginWithBlock:(void (^)(NSDictionary *accountStatusDictionary, NSDictionary *errors, void (^proceedBlock)(void), void (^terminateBlock)(void)))block;
- (void)logout;
- (void)logoutWithBlock:(void (^)(void))block;

- (void)heartbeat;
- (void)clientReRegister;
- (void)restartConnection;
- (void)sipStackRestart;
- (void)resetNetworkMACAddress;
#ifdef stiENABLE_H46017
- (BOOL)getH46017Enabled;
- (void)setH46017Enabled:(BOOL)enabled;
#endif
- (void)setCodecCapabilities:(BOOL)mainSupported h264Level:(NSUInteger)h264Level;
- (void)setDialSource:(SCIDialSource)source;
- (void)setDialMethod:(SCIDialMethod)method;
/// If a call is not created (i.e. the VRS interpreter is told to dial a new phone number without disconnecting), this method returns nil with an SCIErrorDomainVideophoneEngine, code 0 error.
- (SCICall *)dialCallWithDialString:(NSString *)dialString callListName:(NSString *)callListName relayLanguage:(NSString *)language vco:(BOOL)vco forceEncryption:(BOOL)forceEncryption error:(NSError **)errorOut;
/// If a call is not created (i.e. the VRS interpreter is told to dial a new phone number without disconnecting), this method returns nil with an SCIErrorDomainVideophoneEngine, code 0 error.
- (SCICall *)dialCallWithDialString:(NSString *)dialString callListName:(NSString *)callListName relayLanguage:(NSString *)language vcoType:(SCIVoiceCarryOverType)vco forceEncryption:(BOOL)forceEncryption error:(NSError **)errorOut;
- (void)waitForHangUp;
- (BOOL)removeCall:(SCICall *)call;

- (void)checkForUpdateFromVersion:(NSString *)version block:(void (^)(NSString *versionAvailable, NSString *updateURL, NSError *err))block;

- (void)loadUserAccountInfo;
- (void)loadUserAccountInfoWithCompletionHandler:(void (^)(NSError *error))block;
- (void)setSignMailEmailPreferencesEnabled:(BOOL)enabled withPrimaryEmail:(NSString *)primaryAddress secondaryEmail:(NSString *)secondaryAddress completionHandler:(void (^)(NSError *error))block;
- (void)changePassword:(NSString *)password completionHandler:(void (^)(NSError *error))block;
- (void)saveUserSSN:(NSString *)SSN DOB:(NSString *)DOB andHasSSN:(BOOL)hasSSN completionHandler:(void (^)(NSError *error))block;

- (void)loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:(void (^)(NSError *))block;
- (void)provisionEmergencyAddress:(SCIEmergencyAddress *)address reloadEmergencyAddressAndEmergencyAddressProvisioningStatus:(BOOL)reload completionHandler:(void (^)(NSError *error))block;
- (void)provisionAndReloadEmergencyAddressAndEmergencyAddressProvisioningStatus:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block;
- (void)provisionEmergencyAddress:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block;
- (void)deprovisionEmergencyAddressAndProvisionEmergencyAddress:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block;
- (void)deprovisionEmergencyAddressWithCompletionHandler:(void (^)(NSError *error))block;
- (void)acceptEmergencyAddressUpdate;
- (void)rejectEmergencyAddressUpdate;

- (void)setDynamicProviderAgreementStatus:(SCIDynamicProviderAgreementStatus *)agreementStatus completionHandler:(void (^)(BOOL success, NSError *error))block;
- (void)setDynamicProviderAgreementStatus:(SCIDynamicProviderAgreementStatus *)agreementStatus timeout:(NSTimeInterval)timeout completionHandler:(void (^)(BOOL, NSError *))block;
- (void)fetchDynamicProviderAgreementsWithCompletionHandler:(void (^)(NSArray *agreements, NSError *error))block;

- (SCIUserPhoneNumbers *)userPhoneNumbers;
- (void)setUserPhoneNumbers:(SCIUserPhoneNumbers *)userPhoneNumbers;

- (NSString *)phoneNumberReformat:(NSString *)phoneNumber;
- (BOOL)phoneNumberIsValid:(NSString *)phoneNumber;
- (BOOL)dialStringIsValid:(NSString *)dialString;
- (void)fetchDialStringIsSorenson:(NSString *)dialString completionHandler:(void (^)(BOOL isSorenson, NSError *error))block;
- (void)fetchPhoneNumberIsSorenson:(NSString *)phoneNumber completionHandler:(void (^)(BOOL isSorenson, NSError *error))block;

- (void)setUserAccountPhoto:(NSData *)data completion:(void (^)(BOOL, NSError *))block;
- (void)loadUserAccountPhotoWithCompletion:(void (^)(NSData *, NSError *))block;
- (void)loadPreviewImageForMessageInfo:(SCIMessageInfo *)messageInfo completion:(void (^)(NSData *, NSError *))block;
- (void)loadPhotoForContact:(SCIContact *)contact completion:(void (^)(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block;
- (void)setPhotoTimestampForContact:(SCIContact *)contact completion:(void (^)(void))block;
- (void)loadContactPhotoForNumber:(NSString *)number completion:(void (^)(NSData *, NSError *))block;

- (SCIPersonalGreetingPreferences *)personalGreetingPreferences;
- (void)setPersonalGreetingPreferences:(SCIPersonalGreetingPreferences *)personalGreetingPreferences;

- (void)sendRemoteLogEvent:(NSString *)eventString;

- (void)setMaxAutoSpeeds:(NSUInteger)recvSpeed sendSpeed:(NSUInteger)sendSpeed;
- (SCIAutoSpeedMode)networkAutoSpeedMode;

- (void)sendDeviceToken:(NSString*)deviceToken;
- (void)sendNotificationTypes:(NSUInteger)types;

- (void)setLDAPCredentials:(NSDictionary *)LDAPCredentials completion:(void (^)(void))block;
- (void)clearLDAPCredentialsAndContacts;

#pragma mark SignMail, Video Center

@property (readonly) SCIItemId *signMailMessageCategoryId;
- (SCIMessageCategory *)messageCategoryForCategoryId:(SCIItemId *)categoryId;
- (SCIMessageInfo *)messageInfoForMessageId:(SCIItemId *)messageId;
@property (readonly) NSArray<SCIMessageCategory *> *messageCategories;
- (NSArray<SCIMessageCategory *> *)messageSubcategoriesForCategoryId:(SCIItemId *)categoryId;
- (NSArray<SCIMessageInfo *> *)messagesForCategoryId:(SCIItemId *)categoryId; // array of SCIMessageInfo objects
- (SCIMessageCategory *)messageCategoryForMessage:(SCIMessageInfo *)info;
- (SCIMessageCategory *)messageCategoryForMessageCategory:(SCIMessageCategory *)category;
- (void)updateLastMessageViewTime;
@property (readonly) NSUInteger numberOfSignMailMessages;
- (NSUInteger)numberOfNewMessagesInCategory:(SCIItemId *)categoryId;
@property (readonly) NSUInteger signMailCapacity; // number of allowed signmail messages
- (void)downloadURLforMessage:(SCIMessageInfo *)message completion:(void (^)(NSString *downloadURL, NSString *viewId, NSError *sendError))block;
- (void)requestDownloadURLsForMessage:(SCIMessageInfo *)message completion:(void (^)(NSArray *downloadURLs, NSString *viewId, NSError *sendError))block;
- (void)downloadURLListforMessage:(SCIMessageInfo *)message completion:(void (^)(NSString *downloadURL, NSString *viewId, NSError *sendError))block;
- (void)markMessage:(SCIMessageInfo *)message viewed:(BOOL)viewed;
- (void)markMessage:(SCIMessageInfo *)message pausePoint:(NSTimeInterval)seconds;
- (void)deleteMessage:(SCIMessageInfo *)message;
- (void)deleteMessagesInCategory:(SCIItemId *)categoryId;
- (void)registerSignMail;
- (void)registerSignMailWithCompletion:(void (^)(NSError *error))block;
- (SCICall *)signMailSend:(NSString *)dialString error:(NSError **)errorOut;

#pragma mark Ring Groups
- (void)setSipProxyAddress:(NSString *)ProxyIPAddress;
- (void)createRingGroupWithAlias:(NSString *)alias password:(NSString *)password completion:(void (^)(NSError *))block;
- (void)validateRingGroupCredentialsWithPhone:(NSString *)phone password:(NSString *)password completion:(void (^)(NSError *))block;
- (void)fetchRingGroupInfoWithCompletion:(void (^)(NSArray *ringGroupInfos, NSError *error))block; // array of SCIRingGroupInfo
- (void)setRingGroupInfoWithPhone:(NSString *)phone description:(NSString *)description completion:(void (^)(NSError *error))block;
- (void)fetchRingGroupInviteInfoWithCompletion:(void (^)(NSString *name, NSString *localPhone, NSString *tollFreePhone, NSError *error))block;
- (void)acceptRingGroupInvitationWithPhone:(NSString *)phone completion:(void (^)(NSError *error))block;
- (void)rejectRingGroupInvitationWithPhone:(NSString *)phone completion:(void (^)(NSError *error))block;
- (void)setRingGroupPassword:(NSString *)password completion:(void (^)(NSError *error))block;
- (void)inviteRingGroupPhone:(NSString *)phone description:(NSString *)desc completion:(void (^)(NSError *error))block;
- (void)removeRingGroupPhone:(NSString *)phone completion:(void (^)(NSError *error))block;
- (BOOL)isNameRingGroupMember:(NSString*)name;
- (BOOL)isNumberRingGroupMember:(NSString*)number;
- (NSInteger)ringGroupDisplayMode;
- (BOOL)isInRingGroup;

- (void)setRejectIncomingCalls:(BOOL)reject;
- (BOOL)rejectIncomingCalls;
- (NSUInteger)contactsMaxCount;
- (void)updatePassword:(NSString *)password;

#pragma mark Pulse
- (void)pulseEnable:(BOOL)enabled API_UNAVAILABLE(macos);
- (void)pulseMissedCall:(BOOL)missedCalls API_UNAVAILABLE(macos);
- (void)pulseSignMail:(BOOL)newSignMails API_UNAVAILABLE(macos);
- (BOOL)isPulsePowered;

#pragma mark Group Calls
- (BOOL)joinCall:(SCICall *)callOne withCall:(SCICall *)callTwo;
- (BOOL)joinCallAllowedForCall:(SCICall *)callOne withCall:(SCICall *)callTwo;
- (BOOL)createDHVIConference:(SCICall *)call;

#pragma mark Debug Tools
- (void)saveFile;
- (void)setTraceFlagWithName:(NSString *)name value:(NSInteger)value;

@end

extern NSString * const SCIKeyInterfaceMode;
extern NSString * const SCIKeyRejectIncomingCalls;

extern NSNotificationName const SCINotificationLoggedIn;
extern NSNotificationName const SCINotificationAuthorizedDidChange;
extern NSNotificationName const SCINotificationAuthorizingDidChange;
extern NSNotificationName const SCINotificationFetchingAuthorizationDidChange;
extern NSNotificationName const SCINotificationAuthenticatedDidChange;
extern NSNotificationName const SCINotificationConnectedDidChange;
extern NSNotificationName const SCINotificationConnecting;
extern NSNotificationName const SCINotificationConnected;
extern NSNotificationName const SCINotificationCoreOfflineGenericError;
extern NSNotificationName const SCINotificationCallDialing;
extern NSNotificationName const SCINotificationCallPreIncoming;
extern NSNotificationName const SCINotificationCallIncoming; // userInfo[SCINotificationKeyCall] = SCICall
extern NSNotificationName const SCINotificationCallAnswering;
extern NSNotificationName const SCINotificationCallDisconnecting;
extern NSNotificationName const SCINotificationCallDisconnected;
extern NSNotificationName const SCINotificationCallLeaveMessage;
extern NSNotificationName const SCINotificationCallRedirect;
extern NSNotificationName const SCINotificationCallSelfCertificationRequired;
extern NSNotificationName const SCINotificationCallPromptUserForVRS;
extern NSNotificationName const SCINotificationCallRemoteRemovedTextOverlay;
extern NSNotificationName const SCINotificationEstablishingConference;
extern NSNotificationName const SCINotificationConferencing;
extern NSNotificationName const SCINotificationConferencingReadyStateChanged;
extern NSNotificationName const SCINotificationCallInformationChanged;
extern NSNotificationName const SCINotificationTransferring;
extern NSNotificationName const SCINotificationTransferFailed;
extern NSNotificationName const SCINotificationUserAccountInfoUpdated;
extern NSNotificationName const SCINotificationServiceOutageMessageUpdated;
extern NSNotificationName const SCINotificationCallListChanged;
extern NSNotificationName const SCINotificationEmergencyAddressUpdated;
extern NSNotificationName const SCINotificationRemoteRingCountChanged; // @"rings"->NSNumber
extern NSNotificationName const SCINotificationLocalRingCountChanged;
extern NSNotificationName const SCINotificationRemoteVideoPrivacyChanged;
extern NSNotificationName const SCINotificationNudgeReceived;
extern NSNotificationName const SCINotificationUserLoggedIntoAnotherDevice;
extern NSNotificationName const SCINotificationDisplayProviderAgreement;
extern NSNotificationName const SCINotificationMessageCategoryChanged; // SCINotificationKeyItemId->SCIItemId of category
extern NSNotificationName const SCINotificationMessageChanged;	//SCINotificationKeyItemId->SCIItemId of message
extern NSNotificationName const SCINotificationSignMailUploadURLGetFailed;
extern NSNotificationName const SCINotificationMessageSendFailed;
extern NSNotificationName const SCINotificationMailboxFull; // Remote user's mailbox is full.
extern NSNotificationName const SCINotificationHeldCallLocal;
extern NSNotificationName const SCINotificationResumedCallLocal;
extern NSNotificationName const SCINotificationHeldCallRemote;
extern NSNotificationName const SCINotificationResumedCallRemote;
extern NSNotificationName const SCINotificationContactListImportComplete;
extern NSNotificationName const SCINotificationContactListImportError;
extern NSNotificationName const SCINotificationContactListItemAddError;
extern NSNotificationName const SCINotificationContactListItemEditError;
extern NSNotificationName const SCINotificationContactListItemDeleteError;
extern NSNotificationName const SCINotificationBlockedListItemEditError;
extern NSNotificationName const SCINotificationBlockedListItemAddError;
extern NSNotificationName const SCINotificationRingGroupCreated;
extern NSNotificationName const SCINotificationRingGroupChanged;
extern NSNotificationName const SCINotificationRingGroupInvitationReceived;
extern NSNotificationName const SCINotificationRingGroupRemoved;
extern NSNotificationName const SCINotificationRingGroupModeChanged;
extern NSNotificationName const SCINotificationPasswordChanged;
extern NSNotificationName const SCINotificationRasRegistrationConfirmed;
extern NSNotificationName const SCINotificationRasRegistrationRejected;
extern NSNotificationName const SCINotificationUpdateVersionCheckRequired;
extern NSNotificationName const SCINotificationFailedToEstablishTunnel;
extern NSNotificationName const SCINotificationUserAccountPhotoUpdated;
extern NSNotificationName const SCINotificationSuggestionCallCIR;
extern NSNotificationName const SCINotificationRequiredCallCIR;
extern NSNotificationName const SCINotificationPromoteNewFeaturePersonalSignMailGreeting;
extern NSNotificationName const SCINotificationPromoteNewFeatureWebDial;
extern NSNotificationName const SCINotificationPromoteNewFeatureContactPhotos;
extern NSNotificationName const SCINotificationPromoteNewFeatureSharedText;
extern NSNotificationName const SCINotificationPersonalSignMailGreetingAvailable;
extern NSNotificationName const SCINotificationRemoteTextReceived;
extern NSNotificationName const SCINotificationBlockedNumberWhitelisted;
extern NSNotificationName const SCINotificationAutoSpeedSettingChanged;
extern NSNotificationName const SCINotificationMaxSpeedsChanged;
extern NSNotificationName const SCINotificationPropertyChanged; // passes SCINotificationKeyName
extern NSNotificationName const SCINotificationInterfaceModeChanged;
extern NSNotificationName const SCINotificationUserRegistrationDataRequired;
extern NSNotificationName const SCINotificationAgreementChanged;
extern NSNotificationName const SCINotificationSIPRegistrationConfirmed;
extern NSNotificationName const SCINotificationFavoriteListChanged;
extern NSNotificationName const SCINotificationFavoriteListError;
extern NSNotificationName const SCINotificationHearingStatusChanged;
extern NSNotificationName const SCINotificationConferenceEncryptionStateChanged;
extern NSNotificationName const SCINotificationConferenceServiceError;
extern NSNotificationName const SCINotificationConferenceRoomAddAllowedChanged;
extern NSNotificationName const SCINotificationContactReceived;
extern NSNotificationName const SCINotificationLDAPCredentialsInvalid;
extern NSNotificationName const SCINotificationLDAPCredentialsValid;
extern NSNotificationName const SCINotificationLDAPServerUnavailable;
extern NSNotificationName const SCINotificationLDAPError;
extern NSNotificationName const SCINotificationNATTraversalSIPChanged;
extern NSNotificationName const SCINotificationRingsBeforeGreetingChanged;
extern NSNotificationName const SCINotificationDisplayPleaseWait;
extern NSNotificationName const SCINotificationRequestParentAction;
extern NSNotificationName const SCINotificationDeviceLocationTypeChanged;
extern NSNotificationName const SCINotificationPropertyReceived;
extern NSNotificationName const SCINotificationTeaserVideoDetailsChanged;
extern NSNotificationName const SCINotificationSignVideoUpdateRequired;

extern NSString * const SCINotificationKeyCall;
extern NSString * const SCINotificationKeyItemId;
extern NSString * const SCINotificationKeyCount;
extern NSString * const SCINotificationKeyName;
extern NSString * const SCINotificationKeyRingGroupInfos;
extern NSString * const SCINotificationKeyLocalPhone;
extern NSString * const SCINotificationKeyTollFreePhone;
extern NSString * const SCINotificationKeyEmergencyAddress;
extern NSString * const SCINotificationKeyAgreements;
extern NSString * const SCINotificationKeyMessage;
extern NSString * const SCINotificationKeyProviderAgreementStatus;
extern NSString * const SCINotificationKeyProviderAgreementType;
extern NSString * const SCINotificationKeyConferencingReadyState;
extern NSString * const SCINotificationKeySIPServerIP;
extern NSString * const SCINotificationKeySIPServerIPv6;

extern NSString * const SCIErrorDomainVideophoneEngine;	//errorCode to be interpreted as an SCIVideophoneEngineErrorCode
extern NSString * const SCIErrorDomainGeneral;			//errorCode to be interpreted as an stiHResult
extern NSString * const SCIErrorDomainCoreResponse;		//errorCode to be interpreted as an SCICoreResponseErrorCode
extern NSString * const SCIErrorDomainRingGroupManagerResponse;
extern NSString * const SCIErrorDomainMessageResponse;
extern NSString * const SCIErrorDomainCoreRequestSend;
extern NSString * const SCIErrorDomainRingGroupManagerRequestSend;
extern NSString * const SCIErrorDomainMessageRequestSend;
extern NSString * const SCIErrorDomainHTTPResponse;

typedef NS_ERROR_ENUM(SCIErrorDomainVideophoneEngine, SCIVideophoneEngineErrorCode) {
	SCIVideophoneEngineErrorCodeNoError = 0,
	SCIVideophoneEngineErrorCodeEngineNotStarted,
	SCIVideophoneEngineErrorCodeEmptyDialString,
	SCIVideophoneEngineErrorCodeEmptyCallList,
	SCIVideophoneEngineErrorCodeNullDefaultUserSettingsList,
	SCIVideophoneEngineErrorCodeFailedToLoadUserAccountInfo,
	SCIVideophoneEngineErrorCodeNullUserAccountInfo,
	SCIVideophoneEngineErrorCodeEmptyUserSettingsList,
	SCIVideophoneEngineErrorCodeNullUserSettingsList,
	SCIVideophoneEngineErrorCodeEmptyPhoneSettingsList,
	SCIVideophoneEngineErrorCodeNullPhoneSettingsList,
	SCIVideophoneEngineErrorCodeFaultyProviderAgreementType,
	SCIVideophoneEngineErrorCodeUserAccountPhotoFeatureNotSupportedByCoreVersion,
};

typedef NS_ERROR_ENUM(SCIErrorDomainCoreRequestSend, SCICoreRequestSendErrorCode) {
	SCICoreRequestSendErrorCodeNoError = 0,
	SCICoreRequestSendErrorCodeRequestRemoved,
	SCICoreRequestSendErrorCodeRequestTimedOut,
};

typedef NS_ERROR_ENUM(SCIErrorDomainCoreResponse, SCICoreResponseErrorCode) {
	SCICoreResponseErrorCodeConnectionError = -1,

	// Errors occurring during message processing
	SCICoreResponseErrorCodeNoError = 0,						// No Error
	SCICoreResponseErrorCodeNotLoggedIn = 1,					// Not Logged In
	SCICoreResponseErrorCodeUsernameOrPasswordInvalid = 2,		// Username and Password not valid
	SCICoreResponseErrorCodePhoneNumberNotFound = 3,			// Phone Number does not exist
	SCICoreResponseErrorCodeDuplicatePhoneNumber = 4,			// Phone Number already exists
	SCICoreResponseErrorCodeErrorCreatingUser = 5,				// Could not create user
	SCICoreResponseErrorCodeErrorAssociatingPhoneNumber = 6,	// Phone Number could not be associated
	SCICoreResponseErrorCodeCannotAssociatePhoneNumber = 7,		// Phone Number could not be associated - not controlled by user or available
	SCICoreResponseErrorCodeErrorDisassociatingPhoneNumber = 8,	// Phone Number could not be disassociated
	SCICoreResponseErrorCodeCannotDisassociatePhoneNumber = 9,	// Phone Number could not be disassociated - not controlled by user
	SCICoreResponseErrorCodeErrorUpdatingUser = 10,				// Unable to update user or phone
	SCICoreResponseErrorCodeUniqueIdUnrecognized = 11,			// UniqueId not recognized
	SCICoreResponseErrorCodePhoneNumberNotRegistered = 12,		// Phone Number is not registered with Directory
	SCICoreResponseErrorCodeDuplicateUniqueId = 13,				// UniqueId Already Exists
	SCICoreResponseErrorCodeConnectionToSignmailDown = 14,		// Unable to connect to SignMail server
	SCICoreResponseErrorCodeNoInformationSent = 15,				// No information sent
	SCICoreResponseErrorCodeUniqueIdEmpty = 17,				// Unique ID cannot be empty
	SCICoreResponseErrorCodeRealNumberListReleaseFailed = 22,	// Real Number service failed to release the list of numbers
	SCICoreResponseErrorCodeReleaseNumbersNotSpecified = 23,	// Request did not specify phone numbers to release
	SCICoreResponseErrorCodeCountAttributeInvalid = 25,			// Count attribute has an invalid value
	SCICoreResponseErrorCodeRealNumberListGetFailed = 26,		// Real Number service failed to get a list of numbers
	SCICoreResponseErrorCodeRealNumberServiceUnavailable = 27,	// Unable to connect to NANP (real number) server
	SCICoreResponseErrorCodeNotPrimaryUserOnPhone = 28,		// User is not the Primary User of this phone
	SCICoreResponseErrorCodePrimaryUserOnAnotherPhone = 29,	// User is a Primary User on another Phone
	SCICoreResponseErrorCodeProvisionRequestFailed = 30,
	SCICoreResponseErrorCodeNoCurrentEmergencyStatusFound = 31,
	SCICoreResponseErrorCodeNoCurrentEmergencyAddressFound = 32,
	SCICoreResponseErrorCodeEmergencyAddressDataBlank = 33,
	SCICoreResponseErrorCodeDeprovisionRequestFailed = 34,
	SCICoreResponseErrorCodeUnableToContactDatabase = 35,
	SCICoreResponseErrorCodeNoRealNumberAvailable = 36,
	SCICoreResponseErrorCodeUnableToAcquireNumber = 37,
	SCICoreResponseErrorCodeNoPendingNumberAvailable = 38,
	SCICoreResponseErrorCodeRouterInfoNotCleared = 39,
	SCICoreResponseErrorCodeNoPhoneInfoFound = 40,
	SCICoreResponseErrorCodeQicDoesNotExist = 41,
	SCICoreResponseErrorCodeQicCannotBeReached = 42,
	SCICoreResponseErrorCodeQicNotDeclared = 43,
	SCICoreResponseErrorCodeImageProblem = 44,
	SCICoreResponseErrorCodeImageProblemSavingData = 45,
	SCICoreResponseErrorCodeImageProblemDeleting = 46,
	SCICoreResponseErrorCodeImageListGetProblem = 47,
	SCICoreResponseErrorCodeEndpointMatchError = 48,
	SCICoreResponseErrorCodeBlockListItemDenied = 49,
	SCICoreResponseErrorCodeCallListPlanNotFound = 50,
	SCICoreResponseErrorCodeEndpointTypeNotFound = 51,
	SCICoreResponseErrorCodePhoneTypeNotRecognized = 52,
	SCICoreResponseErrorCodeCannotGetServiceContactOverrides = 53,
	SCICoreResponseErrorCodeCannotContactItrs = 54,
	SCICoreResponseErrorCodeUriListSetMismatch = 55,
	SCICoreResponseErrorCodeUriListSetDeleteError = 56,
	SCICoreResponseErrorCodeUriListSetAddError = 57,
	SCICoreResponseErrorCodeSignmailRegisterError = 58,
	SCICoreResponseErrorCodeAddedMoreContactsThanAllowed = 59,
	SCICoreResponseErrorCodeCannotDetermineWhichLogin = 60,
	SCICoreResponseErrorCodeLoginValueInvalid = 61,
	SCICoreResponseErrorCodeErrorUpdatingAlias = 62,
	SCICoreResponseErrorCodeErrorCreatingGroup = 63,
	SCICoreResponseErrorCodeInvalidPin = 64,
	SCICoreResponseErrorCodeUserCannotCreateRinggroup = 65,
	SCICoreResponseErrorCodeEndpointNotRinggroupCapable = 66,
	SCICoreResponseErrorCodeCallListItemNotOwned = 67,
	SCICoreResponseErrorCodeVideoServerUnreachable = 68,
	SCICoreResponseErrorCodeCannotUpdateEnumServer = 69,
	SCICoreResponseErrorCodeCallListItemNotFound = 70,
	SCICoreResponseErrorCodeErrorRollingBackRingGroup = 71,
	SCICoreResponseErrorCodeNoInviteFound = 72,
	SCICoreResponseErrorCodeNotMemberOfRingGroup = 73,
	SCICoreResponseErrorCodeCannotInvitePhoneNumberToGroup = 74,
	SCICoreResponseErrorCodeCannotInviteUser = 75,
	SCICoreResponseErrorCodeRingGroupDoesntExist = 76,
	SCICoreResponseErrorCodeRingGroupNumberNotFound = 77,
	SCICoreResponseErrorCodeRingGroupInviteAcceptNotCompleted = 78,
	SCICoreResponseErrorCodeRingGroupInviteRejectNotCompleted = 79,
	SCICoreResponseErrorCodeRingGroupNumberCannotBeEmpty = 80,
	SCICoreResponseErrorCodeInvalidDescriptionLength = 81,
	SCICoreResponseErrorCodeUserNotFound = 82,
	SCICoreResponseErrorCodeInvalidUser = 83,
	SCICoreResponseErrorCodeGroupUserNotFound = 84,
	SCICoreResponseErrorCodeUserBeingRemovedNotInGroup = 85,
	SCICoreResponseErrorCodeUserCannotBeRemovedFromGroup = 86,
	SCICoreResponseErrorCodeUserNotMemberOfGroup = 87,
	SCICoreResponseErrorCodeElementHasInvalidPhoneNumber = 88,
	SCICoreResponseErrorCodePasswordAndConfirmDoNotMatch = 89,
	SCICoreResponseErrorCodeCannotBlockRingGroupMember = 90,
	SCICoreResponseErrorCodeCannotRemoveLastUserFromGroup = 91,
	SCICoreResponseErrorCodeUriListMustContainItrsUris = 92,
	SCICoreResponseErrorCodeInvalidURI = 93,
	SCICoreResponseErrorCodeRedirectToAndFromUrisCannotBeSame = 94,
	SCICoreResponseErrorCodeRemoveOtherMembersBeforeRemovingRingGroup = 95,
	SCICoreResponseErrorCodeNeedOneMemberInRingGroup = 96,
	SCICoreResponseErrorCodeCouldNotLocateActiveLocalNumber = 97,
	SCICoreResponseErrorCodeUnknownError2 = 98,
	SCICoreResponseErrorCodeInvalidGroupCredentials = 99,
	SCICoreResponseErrorCodeNotMemberOfThisGroup = 100,
	SCICoreResponseErrorCodeErrorSettingGroupPassword = 101,
	SCICoreResponseErrorCodePhoneNumberCannotBeEmpty = 102,
	SCICoreResponseErrorCodeErrorWhileSettingRingGroupInfo = 103,
	SCICoreResponseErrorCodeDialStringRequiredElement = 104,
	SCICoreResponseErrorCodeCannotRemoveRingGroupCreator = 105,
	SCICoreResponseErrorCodeImageNotFound = 106,
	SCICoreResponseErrorCodeNoAgreementFound = 107,
	//eERROR 108,
	SCICoreResponseErrorCodeErrorUpdatingAgreement = 109,
	// eERROR 110,
	SCICoreResponseErrorCodeGuidIncorrectFormat = 111,
	SCICoreResponseErrorCodeCoreConfiguration = 112,
	SCICoreResponseErrorCodeErrorRemovingImage = 113,
	SCICoreResponseErrorCodeUnableSetSetting = 114,
	SCICoreResponseErrorCodeOfflineAuthentication = 115,


	// High-Level Server Errors (message not processed)
	SCICoreResponseErrorCodeDatabaseConnectionError = (NSInteger)0xFFFFFFFB,// Database connection error
	SCICoreResponseErrorCodeXMLValidationError = (NSInteger)0xFFFFFFFC,		// XML Validation Error
	SCICoreResponseErrorCodeXMLFormatError = (NSInteger)0xFFFFFFFD,			// XML Format Error
	SCICoreResponseErrorCodeGenericServerError = (NSInteger)0xFFFFFFFE,		// General Server Error (exception, etc.)
	SCICoreResponseErrorCodeUnknownError = (NSInteger)0xFFFFFFFF
};

typedef NS_ERROR_ENUM(SCIErrorDomainHTTPResponse, SCIHTTPResponseErrorCode) {
	SCIHTTPResponseErrorCodeNoError = 0,
	SCIHTTPResponseErrorCodeErrorResponse,
	SCIHTTPResponseErrorCodeUnsupportedContentType,
	SCIHTTPResponseErrorCodeNoContentType,
};

extern NSString * const SCIErrorKeyLogin;
extern NSString * const SCIErrorKeyFetchEmergencyAddressAndEmergencyAddressProvisioningStatus;
extern NSString * const SCIErrorKeyFetchAgreements;
extern NSString * const SCIErrorKeyFetchProviderAgreementStatus;
extern NSString * const SCIErrorKeyFetchProviderAgreementType;

extern NSNotificationName const SCIUserNotificationPromoDisplayPSMG;
extern NSNotificationName const SCIUserNotificationPromoWebDial;
extern NSNotificationName const SCIUserNotificationPromoPhotosOnContacts;
extern NSNotificationName const SCIUserNotificationPromoShareText;
extern NSNotificationName const SCIUserNotificationPSMGAvailable;
extern NSNotificationName const SCIUserNotificationDismissibleCallCIR;

typedef NSInteger SCIUserInput;
extern const NSInteger SCIUserInputPowerToggle;
extern const NSInteger SCIUserInputShowContacts;
extern const NSInteger SCIUserInputShowMissed;
extern const NSInteger SCIUserInputShowMail;
extern const NSInteger SCIUserInputFarFlash;
extern const NSInteger SCIUserInputMenu;
extern const NSInteger SCIUserInputHome;         // ntouch uses "Home" instead of "Menu"
extern const NSInteger SCIUserInputBackCancel;
extern const NSInteger SCIUserInputUp;
extern const NSInteger SCIUserInputLeft;
extern const NSInteger SCIUserInputEnter;
extern const NSInteger SCIUserInputRight;
extern const NSInteger SCIUserInputDown;
extern const NSInteger SCIUserInputDisplay;
extern const NSInteger SCIUserInputStatus;       // ntouch uses "Status" instead of "Display"
extern const NSInteger SCIUserInputKeyboard;
extern const NSInteger SCIUserInputZero;
extern const NSInteger SCIUserInputOne;
extern const NSInteger SCIUserInputTwo;
extern const NSInteger SCIUserInputThree;
extern const NSInteger SCIUserInputFour;
extern const NSInteger SCIUserInputFive;
extern const NSInteger SCIUserInputSize;
extern const NSInteger SCIUserInputSeven;
extern const NSInteger SCIUserInputEight;
extern const NSInteger SCIUserInputNine;
extern const NSInteger SCIUserInputStar;
extern const NSInteger SCIUserInputPound;
extern const NSInteger SCIUserInputDot;
extern const NSInteger SCIUserInputTiltUp;
extern const NSInteger SCIUserInputPanLeft;
extern const NSInteger SCIUserInputPanRight;
extern const NSInteger SCIUserInputTiltDown;
extern const NSInteger SCIUserInputNear;
extern const NSInteger SCIUserInputFar;
extern const NSInteger SCIUserInputSpareA;
extern const NSInteger SCIUserInputWide;
extern const NSInteger SCIUserInputTele;
extern const NSInteger SCIUserInputSpareB;
extern const NSInteger SCIUserInputSpareC;
extern const NSInteger SCIUserInputSpareD;
extern const NSInteger SCIUserInputSpeaker;
extern const NSInteger SCIUserInputClear;        // ntouch uses "Clear" instead of "Speaker"
extern const NSInteger SCIUserInputViewSelf;
extern const NSInteger SCIUserInputViewMode;
extern const NSInteger SCIUserInputViewPos;
extern const NSInteger SCIUserInputAudioPriv;
extern const NSInteger SCIUserInputDND;
extern const NSInteger SCIUserInputHelp;         // ntouch uses "Help" instead of "No Calls"
extern const NSInteger SCIUserInputVideoPrivacy;
extern const NSInteger SCIUserInputSVRS;
extern const NSInteger SCIUserInputExitUI;

extern NSString *NSStringFromSCICoreResponseErrorCode(SCICoreResponseErrorCode error);
extern NSString *NSStringFromSCIVideophoneEngineErrorCode(SCIVideophoneEngineErrorCode error);

extern const NSTimeInterval SCIVideophoneEngineTimeoutNone;
extern const NSTimeInterval SCIVideophoneEngineDefaultTimeout;

