//
//  STIVideophoneEngine.m
//  ntouchMac
//
//  Created by Adam Preble on 2/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngineCpp.h"
#import "smiConfig.h"
#import "stiDebugTools.h"
#import "stiDefs.h"
#import "stiOS.h"
#import "stiError.h"
#import "stiTools.h"
#import "IstiVideophoneEngine.h"
#import "IstiMessageManager.h"
#import "IstiMessageViewer.h"
#import "IstiPlatform.h"
#import "CstiCoreRequest.h"
#import "CstiStateNotifyResponse.h"
#import "PropertyManager.h"
#import "cmPropertyNameDef.h"
#import "propertydef.h"
#import "CommonConst.h"
#import <sys/sysctl.h>
#import "SCIContactCpp.h"
#import "IstiContact.h"
#import "IstiContactManager.h"
#import "SCIContactManagerCpp.h"
#import "SCICallListItemCpp.h"
#import "SCICallCpp.h"
#import "SCICall+Display.h"
#import "SCIUserAccountInfo.h"
#import "SCIUserAccountInfo+Convenience.h"
#import "SCIUserAccountInfo+CstiUserAccountInfo.h"
#import "SCIEmergencyAddress.h"
#import "SCICallListManager.h"
#import "NSArray+CstiSettingList.h"
#import "SCISettingItem.h"
#import "SCISettingItem+Convenience.h"
#import "SCIUserPhoneNumbers.h"
#import "SCIUserPhoneNumbers+SstiUserPhoneNumbers.h"
#import "SCIMessageInfoCpp.h"
#import "SCIItemIdCpp.h"
#import "SCIMessageCategory.h"
#import "stiSVX.h"
#import "SCIBlockedManager.h"
#import "IstiCall.h"
#import "SCIMessageViewer.h"
#import "SCIMessageViewerCpp.h"
#import "SCIRingGroupInfo.h"
#import "SCIRingGroupInfo+SRingGroupInfo.h"
#import "SCIRingGroupInfo+Cpp.h"
#if TARGET_OS_IPHONE
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#endif
#import "IstiBlockListManager.h"
#import "IstiRingGroupManager.h"
#import "IstiTunnelManager.h"
#import "IstiRingGroupMgrResponse.h"
#import "SCIDebug.h"
#import "NSRunLoop+PerformBlock.h"
#if APPLICATION == APP_NTOUCH_MAC
#import "NSApplication+SystemVersion.h"
#endif
#import "IstiVideoOutput.h"
#import "SCIHTTPImageTransferrer.h"
#import "NSArray+BNREnumeration.h"
#import "NSBundle+SCIAdditions.h"
#import "CstiMessageRequest.h"
#import "CstiMessageResponse.h"
#import "SCIPropertyManager.h"
#import "SCIPropertyManager+SettingItem.h"
#if APPLICATION == APP_NTOUCH_IOS
#import "CstiVRCLClient.h"
#endif
#import "SCIDynamicProviderAgreementStatus.h"
#import "SCIDynamicProviderAgreementContent.h"
#import "SCIPersonalGreetingPreferences.h"
#import "SCIPersonalGreetingPreferences+SstiPersonalGreetingPreferences.h"
#import "SCIDynamicProviderAgreementStatus_Cpp.h"
#import "unichar+Additions.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCICoreVersion.h"
#import "SCIStaticProviderAgreementStatus.h"
#import "SCIStaticProviderAgreementStatus+SCISettingItem.h"
#import "SCIProviderAgreementType.h"
#import "SCISignMailGreetingInfo_Cpp.h"
#import <ImageIO/ImageIO.h>
#if APPLICATION == APP_NTOUCH_IOS
#import "UICKeyChainStore.h"
#import "NSString+MACAddress.h"
#import <sys/utsname.h>
#endif
#import "stiSVX.h"
#import "PhotoManager.h"
#ifdef stiLDAP_CONTACT_LIST
#import "IstiLDAPDirectoryContactManager.h"
#endif
#import "SCIMessageDownloadURLItem.h"
#include "SCIPropertyNames.h"
#import "IstiStatusLED.h"
#import "IstiBluetooth.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#include <memory>
#include "IUserAccountManager.h"
#import "stiOSMacOSXNet.h"
#include "NSString+LanguageCode.h"

extern std::string g_strMACAddress;

NSString * const SCIKeyInterfaceMode = @"interfaceMode";
NSString * const SCIKeyRejectIncomingCalls = @"rejectIncomingCalls";

NSNotificationName const SCINotificationLoggedIn = @"SCINotificationLoggedIn";
NSNotificationName const SCINotificationAuthorizedDidChange = @"SCINotificationAuthorizedDidChange";
NSNotificationName const SCINotificationAuthorizingDidChange = @"SCINotificationAuthorizingDidChange";
NSNotificationName const SCINotificationFetchingAuthorizationDidChange = @"SCINotificationFetchingAuthorizationDidChange";
NSNotificationName const SCINotificationAuthenticatedDidChange = @"SCINotificationAuthenticatedDidChange";
NSNotificationName const SCINotificationConnectedDidChange = @"SCINotificationConnectedDidChange";
NSNotificationName const SCINotificationConnected = @"SCINotificationConnected";
NSNotificationName const SCINotificationConnecting = @"SCINotificationConnecting";
NSNotificationName const SCINotificationCoreOfflineGenericError = @"SCINotificationCoreOfflineGenericError";
NSNotificationName const SCINotificationCallDialing = @"SCINotificationCallDialing";
NSNotificationName const SCINotificationCallPreIncoming = @"SCINotificationCallPreIncoming";
NSNotificationName const SCINotificationCallIncoming = @"SCINotificationCallIncoming"; // userInfo[SCINotificationKeyCall] = SCICall
NSNotificationName const SCINotificationCallAnswering = @"SCINotificationCallAnswering";
NSNotificationName const SCINotificationCallDisconnecting = @"SCINotificationCallDisconnecting";
NSNotificationName const SCINotificationCallDisconnected = @"SCINotificationCallDisconnected";
NSNotificationName const SCINotificationCallLeaveMessage = @"SCINotificationCallLeaveMessage";
NSNotificationName const SCINotificationCallRedirect = @"SCINotificationCallRedirect";
NSNotificationName const SCINotificationCallSelfCertificationRequired = @"SCINotificationCallSelfCertificationRequired";
NSNotificationName const SCINotificationCallPromptUserForVRS = @"SCINotificationCallPromptUserForVRS";
NSNotificationName const SCINotificationCallRemoteRemovedTextOverlay = @"SCINotificationCallRemoteRemovedTextOverlay";
NSNotificationName const SCINotificationEstablishingConference = @"SCINotificationEstablishingConference";
NSNotificationName const SCINotificationConferencing = @"SCINotificationConferencing";
NSNotificationName const SCINotificationConferencingReadyStateChanged = @"SCINotificationConferencingReadyStateChanged";
NSNotificationName const SCINotificationCallInformationChanged = @"SCINotificationCallInformationChanged";
NSNotificationName const SCINotificationTransferring = @"SCINotificationTransferring";
NSNotificationName const SCINotificationTransferFailed = @"SCINotificationTransferFailed";
NSNotificationName const SCINotificationUserAccountInfoUpdated = @"SCINotificationUserAccountInfoUpdated";
NSNotificationName const SCINotificationServiceOutageMessageUpdated = @"SCINotificationServiceOutageMessageUpdated";
NSNotificationName const SCINotificationCallListChanged = @"SCINotificationCallListChanged";
NSNotificationName const SCINotificationEmergencyAddressUpdated = @"SCINotificationEmergencyAddressUpdated";
NSNotificationName const SCINotificationRemoteRingCountChanged = @"SCINotificationRemoteRingCountChanged";
NSNotificationName const SCINotificationLocalRingCountChanged = @"SCINotificationLocalRingCountChanged";
NSNotificationName const SCINotificationRemoteVideoPrivacyChanged = @"SCINotificationRemoteVideoPrivacyChanged";
NSNotificationName const SCINotificationNudgeReceived = @"SCINotificationNudgeReceived";
NSNotificationName const SCINotificationUserLoggedIntoAnotherDevice = @"SCINotificationUserLoggedIntoAnotherDevice";
NSNotificationName const SCINotificationDisplayProviderAgreement = @"SCINotificationDisplayProviderAgreement";
NSNotificationName const SCINotificationMessageCategoryChanged = @"SCINotificationMessageCategoryChanged";
NSNotificationName const SCINotificationMessageChanged = @"SCINotificationMessageChanged";
NSNotificationName const SCINotificationSignMailUploadURLGetFailed = @"SCINotificationSignMailUploadURLGetFailed";
NSNotificationName const SCINotificationMessageSendFailed = @"SCINotificationMessageSendFailed";
NSNotificationName const SCINotificationMailboxFull = @"SCINotificationMailboxFull";
NSNotificationName const SCINotificationHeldCallLocal = @"SCINotificationHeldCallLocal";
NSNotificationName const SCINotificationResumedCallLocal = @"SCINotificationResumedCallLocal";
NSNotificationName const SCINotificationHeldCallRemote = @"SCINotificationHeldCallRemote";
NSNotificationName const SCINotificationResumedCallRemote = @"SCINotificationResumedCallRemote";
NSNotificationName const SCINotificationAutoSpeedSettingChanged = @"SCINotificationAutoSpeedSettingChanged";
NSNotificationName const SCINotificationMaxSpeedsChanged = @"SCINotificationMaxSpeedsChanged";
NSNotificationName const SCINotificationBlockedNumberWhitelisted = @"SCINotificationBlockedNumberWhitelisted";
NSNotificationName const SCINotificationContactListImportComplete = @"SCINotificationContactListImportComplete";
NSNotificationName const SCINotificationContactListImportError = @"SCINotificationContactListImportError";
NSNotificationName const SCINotificationContactListItemAddError = @"SCINotificationContactListItemAddError";
NSNotificationName const SCINotificationContactListItemEditError = @"SCINotificationContactListItemEditError";
NSNotificationName const SCINotificationContactListItemDeleteError = @"SCINotificationContactListItemDeleteError";
NSNotificationName const SCINotificationBlockedListItemEditError = @"SCINotificationBlockedListItemEditError";
NSNotificationName const SCINotificationBlockedListItemAddError = @"SCINotificationBlockedListItemAddError";
NSNotificationName const SCINotificationRingGroupCreated = @"SCINotificationRingGroupCreated";
NSNotificationName const SCINotificationRingGroupChanged = @"SCINotificationRingGroupChanged";
NSNotificationName const SCINotificationRingGroupInvitationReceived = @"SCINotificationRingGroupInvitationReceived";
NSNotificationName const SCINotificationRingGroupRemoved = @"SCINotificationRingGroupRemoved";
NSNotificationName const SCINotificationRingGroupModeChanged = @"SCINotificationRingGroupModeChanged";
NSNotificationName const SCINotificationPasswordChanged = @"SCINotificationPasswordChanged";
NSNotificationName const SCINotificationRasRegistrationConfirmed = @"SCINotificationRasRegistrationConfirmed";
NSNotificationName const SCINotificationRasRegistrationRejected = @"SCINotificationRasRegistrationRejected";
NSNotificationName const SCINotificationSIPRegistrationConfirmed = @"SCINotificationSIPRegistrationConfirmed";
NSNotificationName const SCINotificationUpdateVersionCheckRequired = @"SCINotificationUpdateVersionCheckRequired";
NSNotificationName const SCINotificationFailedToEstablishTunnel = @"SCINotificationFailedToEstablishTunnel";
NSNotificationName const SCINotificationUserAccountPhotoUpdated = @"SCINotificationUserAccountPhotoUpdated";
NSNotificationName const SCINotificationSuggestionCallCIR = @"SCINotificationSuggestionCallCIR";
NSNotificationName const SCINotificationRequiredCallCIR = @"SCINotificationRequiredCallCIR";
NSNotificationName const SCINotificationPromoteNewFeaturePersonalSignMailGreeting = @"SCINotificationPromoteNewFeaturePersonalSignMailGreeting";
NSNotificationName const SCINotificationPromoteNewFeatureWebDial = @"SCINotificationPromoteNewFeatureWebDial";
NSNotificationName const SCINotificationPromoteNewFeatureContactPhotos = @"SCINotificationPromoteNewFeatureContactPhotos";
NSNotificationName const SCINotificationPromoteNewFeatureSharedText = @"SCINotificationPromoteNewFeatureSharedText";
NSNotificationName const SCINotificationPersonalSignMailGreetingAvailable = @"SCINotificationPersonalSignMailGreetingAvailable";
NSNotificationName const SCINotificationRemoteTextReceived = @"SCINotificationRemoteTextReceived";
NSNotificationName const SCINotificationInterfaceModeChanged = @"SCINotificationInterfaceModeChanged";
NSNotificationName const SCINotificationUserRegistrationDataRequired = @"SCINotificationUserRegistrationDataRequired";
NSNotificationName const SCINotificationAgreementChanged = @"SCINotificationAgreementChanged";
NSNotificationName const SCINotificationFavoriteListChanged = @"SCINotificationFavoriteListChanged";
NSNotificationName const SCINotificationFavoriteListError = @"SCINotificationFavoriteListError";
NSNotificationName const SCINotificationHearingStatusChanged = @"SCINotificationHearingStatusChanged";
NSNotificationName const SCINotificationConferenceEncryptionStateChanged = @"SCINotificationConferenceEncryptionStateChanged";
NSNotificationName const SCINotificationConferenceServiceError = @"SCINotificationConferenceServiceError";
NSNotificationName const SCINotificationConferenceRoomAddAllowedChanged = @"SCINotificationConferenceRoomAddAllowedChanged";
NSNotificationName const SCINotificationContactReceived = @"SCINotificationContactReceived";
NSNotificationName const SCINotificationLDAPCredentialsInvalid = @"SCINotificationLDAPCredentialsInvalid";
NSNotificationName const SCINotificationLDAPCredentialsValid = @"SCINotificationLDAPCredentialsValid";
NSNotificationName const SCINotificationLDAPServerUnavailable = @"SCINotificationLDAPServerUnavailable";
NSNotificationName const SCINotificationLDAPError = @"SCINotificationLDAPError";
NSNotificationName const SCINotificationNATTraversalSIPChanged = @"SCINotificationNATTraversalSIPChanged";
NSNotificationName const SCINotificationRingsBeforeGreetingChanged = @"SCINotificationRingsBeforeGreetingChanged";
NSNotificationName const SCINotificationDisplayPleaseWait = @"SCINotificationDisplayPleaseWait";
NSNotificationName const SCINotificationRequestParentAction = @"SCINotificationRequestParentAction";
NSNotificationName const SCINotificationDeviceLocationTypeChanged = @"SCINotificationDeviceLocationTypeChanged";
NSNotificationName const SCINotificationTeaserVideoDetailsChanged = @"SCINotificationTeaserVideoDetailsChanged";
NSNotificationName const SCINotificationSignVideoUpdateRequired = @"SCINotificationSignVideoUpdateRequired";

NSString * const SCINotificationKeyCall = @"call";
NSString * const SCINotificationKeyItemId = @"itemId";
NSString * const SCINotificationKeyCount = @"count";
NSString * const SCINotificationKeyRingGroupInfos = @"infos";
NSString * const SCINotificationKeyLocalPhone = @"localPhone";
NSString * const SCINotificationKeyTollFreePhone = @"tollFreePhone";
NSString * const SCINotificationKeyEmergencyAddress = @"emergencyAddress";
NSString * const SCINotificationKeyAgreements = @"agreements";
NSString * const SCINotificationKeyMessage = @"message";
NSString * const SCINotificationKeyProviderAgreementStatus = @"agreementStatus";
NSString * const SCINotificationKeyProviderAgreementType = @"providerAgreementType";
NSString * const SCINotificationKeyConferencingReadyState = @"conferencingReadyState";
NSString * const SCINotificationKeySIPServerIP = @"SIPServerIP";
NSString * const SCINotificationKeySIPServerIPv6 = @"SIPServerIPv6";

NSString * const SCIErrorDomainVideophoneEngine = @"SCIErrorDomainVideophoneEngine";
NSString * const SCIErrorDomainGeneral = @"SCIErrorDomainGeneral";
NSString * const SCIErrorDomainCoreResponse = @"SCIErrorDomainCoreResponse";
NSString * const SCIErrorDomainRingGroupManagerResponse = @"SCIErrorDomainRingGroupManagerResponse";
NSString * const SCIErrorDomainMessageResponse = @"SCIErrorDomainMessageResponse";
NSString * const SCIErrorDomainCoreRequestSend = @"SCIErrorDomainCoreRequestSend";
NSString * const SCIErrorDomainRingGroupManagerRequestSend = @"SCIErrorDomainRingGroupManagerRequestSend";
NSString * const SCIErrorDomainMessageRequestSend = @"SCIErrorDomainMessageRequestSend";
NSString * const SCIErrorDomainHTTPResponse = @"SCIErrorDomainHTTPResponse";

NSString * const SCIErrorKeyLogin = @"loginError";
NSString * const SCIErrorKeyFetchEmergencyAddressAndEmergencyAddressProvisioningStatus = @"fetchEmergencyAddressAndProvisioningError";
NSString * const SCIErrorKeyFetchAgreements = @"agreements";
NSString * const SCIErrorKeyFetchProviderAgreementStatus = @"providerAgreementStatus";
NSString * const SCIErrorKeyFetchProviderAgreementType = @"providerAgreementType";

NSNotificationName const SCIUserNotificationPromoDisplayPSMG = @"PromoDisplayPSMG";
NSNotificationName const SCIUserNotificationPromoWebDial = @"PromoWebDial";
NSNotificationName const SCIUserNotificationPromoPhotosOnContacts = @"PromoPhotosOnContacts";
NSNotificationName const SCIUserNotificationPromoShareText = @"PromoShareText";
NSNotificationName const SCIUserNotificationPSMGAvailable = @"PSMGAvailable";
NSNotificationName const SCIUserNotificationDismissibleCallCIR = @"DismissibleCallCIR";

// const strings
#if APPLICATION == APP_NTOUCH_IOS
static const ProductType product = ProductType::iOS;
#elif APPLICATION == APP_NTOUCH_MAC
static const ProductType product = ProductType::Mac;
#endif

const NSTimeInterval SCIVideophoneEngineTimeoutNone = 0.0;
const NSTimeInterval SCIVideophoneEngineDefaultTimeout = SCIVideophoneEngineTimeoutNone;
static const NSTimeInterval SCIVideophoneEngineTimeoutTimerTickTime = 1.0;

NSString *NSStringFromEstiCmMessage(int32_t m);
NSString *NSStringFromCstiCoreResponseCode(CstiCoreResponse::EResponse response);
NSString *NSStringFromECoreError(CstiCoreResponse::ECoreError error);
NSString *NSStringFromCstiMessageResponseCode(CstiMessageResponse::EResponse responseCode);
NSString *NSStringFromFilePlayState(uint32_t param);

#if APPLICATION == APP_NTOUCH_IOS
CstiVRCLClient *m_oRemoteClient;
stiHResult RemoteVRCLCallBack(
														int32_t n32Message,
														size_t MessageParam,
														size_t CallbackParam,
														size_t FromId)
{
//	SCIVideophoneEngine *pThis = (IstiVideophoneEngineTest *)CallbackParam;
	
	return stiRESULT_SUCCESS;
}
#endif

@interface SCIVideophoneEngine () {
	IstiVideophoneEngine *mVideophoneEngine;
	IstiStatusLED *mStatusLEDInterface;
	IstiBluetooth *mBluetoothInterface;
	
	NSMutableDictionary *mCoreResponseHandlers; // keys are NSNumbers for requestIds; values are blocks.
	NSMutableDictionary *mMessageResponseHandlers; //keys are NSNumbers for requestIds; values are blocks.
	NSMutableDictionary *mRingGroupManagerResponseHandlers; // keys are NSNumbers for requestIds; values are blocks.
	
	NSMutableDictionary *mCoreRequestTimeouts;
	
	NSMutableArray *mActiveCalls; // SCICall objects that are thought to be 'active'; managed by -callForIstiCall:, -forgetCall:.
	
	BOOL mCallInProgress;
	NSCondition *mCallInProgressCondition;
	
	SCIUserPhoneNumbers *mUserPhoneNumbers;
	
	SCIHTTPImageTransferrer *mImageUploader;
	SCIHTTPImageTransferrer *mImageDownloader;
	
	NSTimer *mTimeoutTimer;
	CstiSignalConnection::Collection mSignalConnections;
}
@property (nonatomic) SCIUserAccountInfo *userAccountInfo;
@property (nonatomic) SCIEmergencyAddress *emergencyAddress;
@property (nonatomic, getter = isStarted) BOOL started;
@property (nonatomic, getter = isConnecting) BOOL connecting;
@property (nonatomic, getter = isConnected) BOOL connected;
@property (nonatomic, getter = isAuthenticated) BOOL authenticated;
@property (nonatomic, getter = isFetchingAuthorization) BOOL fetchingAuthorization;
@property (nonatomic, getter = isAuthorizing) BOOL authorizing;
@property (nonatomic, getter = isAuthorized) BOOL authorized;
@property (nonatomic, getter = isSipRegistered) BOOL sipRegistered;
@property (nonatomic, getter = isProcessingLogin) BOOL processingLogin;
@property (nonatomic, getter = isLoggingOut) BOOL loggingOut;
@property (nonatomic) NSString *phoneUserId;
@property (nonatomic) NSString *ringGroupUserId;
@property (nonatomic) SCICoreVersion *coreVersion;
@property (nonatomic, readonly) NSSet *logoutSuppressedNotifications;
@property (nonatomic, readonly) NSSet *interfaceModeSuppressedNotifications;
- (stiHResult)engineCallbackWithMessage:(int32_t)message param:(size_t)param;
- (void)handlePropertyManagerChange:(const char *)propertyName;
@end

#if APPLICATION != APP_NTOUCH_IOS //Temporary change to enable iOS to send callbacks to SCIVideophoneEngine
static
#endif
stiHResult EngineCallback(int32_t n32Message, size_t MessageParam, size_t param) {
	SCIVideophoneEngine *engine = (__bridge SCIVideophoneEngine*)(void *)param;
	@autoreleasepool {
		return [engine engineCallbackWithMessage:n32Message param:MessageParam];
	}
}

void PropertyChangeCallbackStub(const char *pszProperty, void *pCallbackParam)
{
	SCIVideophoneEngine *vpe = (__bridge SCIVideophoneEngine *)pCallbackParam;
	@autoreleasepool {
		[vpe handlePropertyManagerChange:pszProperty];
	}
}

@implementation SCIVideophoneEngine

@synthesize userAccountInfo = mUserAccountInfo;
@synthesize emergencyAddress = mEmergencyAddress;
@synthesize username = mUsername;
@synthesize password = mPassword;
@synthesize loginName = mLoginName;
@synthesize started = mStarted;
@synthesize connected = mConnected;
@synthesize connecting = mConnecting;
@synthesize authenticated = mAuthenticated;
@synthesize fetchingAuthorization = mFetchingAuthorization;
@synthesize authorizing = mAuthorizing;
@synthesize authorized = mAuthorized;
@synthesize sipRegistered = mSipRegistered;
@synthesize processingLogin = mProcessingLogin;
@synthesize loggingOut = mLoggingOut;
@synthesize remoteVideoPrivacyEnabled = mRemoteVideoPrivacyEnabled;
@synthesize softwareVersion;
@synthesize logoutSuppressedNotifications = mLogoutSuppressedNotifications;
@synthesize callDialSource = mCallDialSource;
@synthesize callDialMethod = mCallDialMethod;

EstiResult stiGenerateUniqueID (char *pszUniqueID);

#pragma mark - Class Lifecycle

+ (void)initialize
{
	extern DebugTool g_stiHTTPTaskDebug;
	extern DebugTool g_stiTraceDebug;
#ifdef stiDEBUG
	g_stiHTTPTaskDebug.valueSet(0); // enable core request / response logging
	g_stiTraceDebug.valueSet(1);
#else
	g_stiTraceDebug = 0;
#endif
}

#pragma mark - Shared Instance

static SCIVideophoneEngine *sharedEngine = nil;

+ (SCIVideophoneEngine *)sharedEngine
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedEngine = [[SCIVideophoneEngine alloc] init];
	});
	return sharedEngine;
}

#pragma mark - Lifecycle

- (id)init
{
	if ((self = [super init]))
	{
		stiOSSetAdapterName("en0");
		
		mCoreResponseHandlers = [[NSMutableDictionary alloc] init];
		mActiveCalls = [[NSMutableArray alloc] init];
		mRingGroupManagerResponseHandlers = [[NSMutableDictionary alloc] init];
		mMessageResponseHandlers = [[NSMutableDictionary alloc] init];
		mCoreRequestTimeouts = [[NSMutableDictionary alloc] init];
		
		mStarted = NO;
		mConnected = NO;
		mConnecting = NO;
		mAuthenticated = NO;
		mFetchingAuthorization = NO;
		mAuthorizing = NO;
		mAuthorized = NO;
		mProcessingLogin = NO;
		mSipRegistered = NO;
		
		mCallInProgress = NO;
		mCallInProgressCondition = [[NSCondition alloc] init];
		
		mCallDialSource = SCIDialSourceUnknown;
		mCallDialMethod = SCIDialMethodUnknown;
        
        // Update UI_LANGUAGE if the language is changed on the device
        [[NSNotificationCenter defaultCenter] addObserverForName: NSCurrentLocaleDidChangeNotification object:self queue:nil usingBlock:^(NSNotification *notification)
         {
            WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
			NSString * uiLanguage = [[[[[NSBundle mainBundle] preferredLocalizations] objectAtIndex:0] LanguageCode] stringByReplacingOccurrencesOfString:@"-" withString:@"_"];
            pPM->setPropertyString(UI_LANGUAGE, [uiLanguage UTF8String], WillowPM::PropertyManager::Persistent);
         }];
	}
	return self;
}

- (void)dealloc
{

}

#pragma mark - Public API: Managing Engine State

- (void)launchWithOptions:(NSDictionary *)options
{
	SCILog(@"%@", options);
	NSString *marketingVersion = [NSBundle.mainBundle sci_shortVersionString];
	NSString *bundleVersion = [NSBundle.mainBundle sci_bundleVersion];
	NSString *fullVersion = [marketingVersion stringByAppendingString:[@"." stringByAppendingString:bundleVersion]];

	stiErrorLogSystemInit(fullVersion.UTF8String);
	// Initialize Watchdog. (MUST set up Watchdog before creating PropertyManager.)
	stiOSWdInitialize();
	// Initialize PropertyManager. (MUST set up PropertyManager before creating VideophoneEngine.)
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	char staticDirectory[1024];
#if APPLICATION == APP_NTOUCH_MAC
	sprintf(staticDirectory, "%s/Contents/Resources/", [[[NSBundle mainBundle] bundlePath] UTF8String]);
	NSString *appSupportPath = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"ntouch-mac"];
	NSString *propertiesPath = [appSupportPath stringByAppendingPathComponent:@"PropertyTable.xml"];

	if (![[NSFileManager defaultManager] fileExistsAtPath:appSupportPath])
	{
		NSError* error = nil;
		[[NSFileManager defaultManager] createDirectoryAtPath:appSupportPath withIntermediateDirectories:YES attributes:nil error:&error];
	}
	
	static char staticPropertyFilePath[512];
	strcpy(staticPropertyFilePath, [propertiesPath UTF8String]);
	pPM->SetFilename(staticPropertyFilePath); // SetFilename just saves a pointer; doesn't actually copy the string.
#elif APPLICATION == APP_NTOUCH_IOS
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES);
	NSString *preferenceFolder = [paths objectAtIndex:0];
	sprintf(staticDirectory, "%s/", [[[NSBundle mainBundle] bundlePath] UTF8String]);

	NSString *preferenceFile = [preferenceFolder stringByAppendingPathComponent:@"com.sorenson.sorensoncomm.ntouch.xml"];
	
	static char staticPropertyFilePath[512];
	strcpy(staticPropertyFilePath, [preferenceFile UTF8String]);
	pPM->SetFilename(staticPropertyFilePath);
#endif
	pPM->SetBackup1Filename("");
	pPM->SetDefaultFilename("");
	pPM->SetUpdateFilename("");
	
	pPM->init("M.m.R.B");
	
	if (options)
	{
		for (NSString *key in options)
		{
			if ([key isEqualToString:@"ssl"])
			{
				int optionValue = [[options objectForKey:key] intValue];
				int sslFailover = (1==optionValue)?eSSL_ON_WITH_FAILOVER:eSSL_OFF;
				
				pPM->setPropertyInt(CoreServiceSSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
				pPM->setPropertyInt(StateNotifySSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
				pPM->setPropertyInt(MessageServiceSSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
			}
			else {
				pPM->setPropertyString([key UTF8String], [[options objectForKey:key] UTF8String], WillowPM::PropertyManager::Persistent);
			}
		}
	}

	// Set SIP properties
//	pPM->setPropertyInt(NAT_TRAVERSAL_SIP, estiSIPNATEnabledMediaRelayAllowed, WillowPM::PropertyManager::Persistent);
//	pPM->PropertySend(NAT_TRAVERSAL_SIP, estiPTypePhone);
	
// Set H323 properties
//	pPM->setPropertyInt(NAT_TRAVERSAL_H323, estiH323NATEnabled, WillowPM::PropertyManager::Persistent);
//	pPM->PropertySend(NAT_TRAVERSAL_H323, estiPTypePhone);
//	pPM->setPropertyString(cmGATEKEEPER_IP_ADDRESS, "65.37.249.202", WillowPM::PropertyManager::Persistent);
	
	// Set Dialing  and registration properties
//	pPM->setPropertyInt(PROTOCOL_DIAL, estiDialFirstListed, WillowPM::PropertyManager::Persistent);
//	pPM->setPropertyInt(PROTOCOL_REGISTRATION, estiRegisterSipFirst, WillowPM::PropertyManager::Persistent);
//	pPM->PropertySend(PROTOCOL_DIAL, estiPTypePhone);
//	pPM->PropertySend(PROTOCOL_REGISTRATION, estiPTypePhone);
//	int nProtocolUpdated = 0;
//	pPM->getPropertyInt("VRSProtocolUpdated", &nProtocolUpdated, 0);
//	if (nProtocolUpdated == 0)
//	{
//		pPM->setPropertyInt(PROTOCOL_VRS, estiSIP, WillowPM::PropertyManager::Persistent);
//		pPM->PropertySend(PROTOCOL_VRS, estiPTypePhone);
//		pPM->setPropertyInt("VRSProtocolUpdated", 1, WillowPM::PropertyManager::Persistent);
//	}
//#if APPLICATION == APP_NTOUCH_IOS
//	pPM->setPropertyString(H323_PRIVATE_DOMAIN_OVERRIDE, "mgk.tsvrs.in", WillowPM::PropertyManager::Persistent);
//#endif
//	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "s.lsmxb.svrs.net", WillowPM::PropertyManager::Persistent);
//	pPM->removeProperty(SIP_PRIVATE_DOMAIN_OVERRIDE);
//	pPM->setPropertyString(SIP_PUBLIC_DOMAIN_OVERRIDE, "proxytest.tsvrs.in", WillowPM::PropertyManager::Persistent);
//	pPM->removeProperty(SIP_PUBLIC_DOMAIN_OVERRIDE);

//	System B (Core 3.0/MultiRing)
//	pPM->setPropertyInt (TUNNELING_ENABLED, 1, WillowPM::PropertyManager::Persistent);
#ifdef USE_QA_SYSTEM
	
#define DEBUG_SERVER_PRODUCTION	1
#define DEBUG_SERVER_SYSTEM_A	2
#define DEBUG_SERVER_SYSTEM_B	3
#define DEBUG_SERVER_SYSTEM_C	4
#define DEBUG_SERVER_SYSTEM_C_INTERCEPTOR	5
#define DEBUG_SERVER_SYSTEM_F	6
#define DEBUG_SERVER_SYSTEM_UAT 7
	
//#define DEBUG_SERVER DEBUG_SERVER_PRODUCTION
//#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_A
//#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_B
//#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_C
//#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_C_INTERCEPTOR
//#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_F
#define DEBUG_SERVER DEBUG_SERVER_SYSTEM_UAT

#if DEBUG_SERVER == DEBUG_SERVER_PRODUCTION
	{
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://mvrscore.sorenson.com/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://mvrsstatenotify.sorenson.com/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://mvrsmessage.sorenson.com/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://mvrscore.sorenson2.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://mvrsstatenotify.sorenson2.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://mvrsmessage.sorenson2.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	
	//Use the Production SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "s.ci.svrs.net", WillowPM::PropertyManager::Persistent);
	
	// Disable SSL
	pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	
	//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);
	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_A
	{
//	Use the System A Core, State Notify, and Message Services
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://core1.qa-a.svrs.net/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://core1.qa-a.sorenson2.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://statenotify1.qa-a.svrs.net/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://statenotify1.qa-a.sorenson2.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://message1.qa-a.svrs.net/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://message1.qa-a.sorenson2.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
		
	//	Use the System A SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.dev.sip.svrs.net", WillowPM::PropertyManager::Persistent);
		
	//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);

	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_B
	{
//	Use the System B Core, State Notify, and Message Services
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://core1.qa-b.svrs.net/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://core1.qa-b.sorenson2.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://statenotify1.qa-b.svrs.net/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://statenotify1.qa-b.sorenson2.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://message1.qa-b.svrs.net/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://message1.qa-b.sorenson2.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	
//	Use the System B SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.dev.sip.svrs.net", WillowPM::PropertyManager::Persistent);

// Disable SSL
	pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	
//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);
	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_C
	{
//	Use the System C Core, State Notify, and Message Services
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://core1.qa-c.svrs.net/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://core1.qa-c.sorenson2.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://statenotify1.qa-c.svrs.net/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://statenotify1.qa-c.sorenson2.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://message1.qa-c.svrs.net/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://message1.qa-c.sorenson2.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
		
//	Use the System C SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(SIP_PUBLIC_DOMAIN_OVERRIDE, "vrs.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	
// Disable SSL
	pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	
//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);

//Enable EndpointMonitor
	pPM->setPropertyString(ENDPOINT_MONITOR_SERVER, stiENDPOINT_MONITOR_SERVER_DEFAULT, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(ENDPOINT_MONITOR_ENABLED, 1, WillowPM::PropertyManager::Persistent);
	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_C_INTERCEPTOR
	{
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://10.20.164.11/Interceptor/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://10.20.164.11/Interceptor/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://10.20.164.11/Interceptor/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://10.20.164.11/Interceptor/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://10.20.164.11/Interceptor/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://10.20.164.11/Interceptor/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	
//	Use the System C SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	
// Disable SSL
	pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	
//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);
	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_F
	{
//	Use the System F Core, State Notify, and Message Services
	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://systemf.tsvrs.in/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://systemf.tsvrs.in/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://systemf.tsvrs.in/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
	
//	Use the System B SIP Server Address
	pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.dev.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	
// Disable SSL
	pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
	
//Disable TLS
	pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);
	}
#elif DEBUG_SERVER == DEBUG_SERVER_SYSTEM_UAT
	{
		// Use the System UAT Core, State Notify, and Message Services
		pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://core1-UAT.sorensonqa.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(cmCORE_SERVICE_ALT_URL1, "http://core2-UAT.sorensonqa.biz/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://statenotify1-UAT.sorensonqa.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_ALT_URL1, "http://statenotify2-UAT.sorensonqa.biz/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://message1-UAT.sorensonqa.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(cmMESSAGE_SERVICE_ALT_URL1, "http://message2-UAT.sorensonqa.biz/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
		
		// Use the System C SIP Server Address
		pPM->setPropertyString(SIP_PRIVATE_DOMAIN_OVERRIDE, "vrs.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
		pPM->setPropertyString(SIP_PUBLIC_DOMAIN_OVERRIDE, "vrs.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
		
		// Disable SSL
		pPM->setPropertyInt(CoreServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
		pPM->setPropertyInt(StateNotifySSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
		pPM->setPropertyInt(MessageServiceSSLFailOver, eSSL_OFF, WillowPM::PropertyManager::Persistent);
		
		// Disable TLS
		pPM->setPropertyInt(SIP_NAT_TRANSPORT, 1, WillowPM::PropertyManager::Persistent);
		
		// Enable EndpointMonitor
		pPM->setPropertyString(ENDPOINT_MONITOR_SERVER, stiENDPOINT_MONITOR_SERVER_DEFAULT, WillowPM::PropertyManager::Persistent);
		pPM->setPropertyInt(ENDPOINT_MONITOR_ENABLED, 1, WillowPM::PropertyManager::Persistent);
	}
#endif

#undef DEBUG_SERVER
#undef DEBUG_SERVER_PRODUCTION
#undef DEBUG_SERVER_SYSTEM_A
#undef DEBUG_SERVER_SYSTEM_B
#undef DEBUG_SERVER_SYSTEM_C
#undef DEBUG_SERVER_SYSTEM_C_INTERCEPTOR
#undef DEBUG_SERVER_SYSTEM_F
#undef DEBUG_SERVER_SYSTEM_UAT
	
	pPM->setPropertyString(cmVRS_SERVER, "vrs-eng.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(cmVRS_ALT_SERVER, "vrs-eng.qa.sip.svrs.net", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(TURN_SERVER, "coturn.qasvrs.sorensonaws.com", WillowPM::PropertyManager::Persistent);
	pPM->setPropertyString(TURN_SERVER_ALT, "istun.qa-sorenson.com", WillowPM::PropertyManager::Persistent);
	
#endif
	
//	Production
//	pPM->setPropertyString(cmCORE_SERVICE_URL1, "http://mvrscore.sorenson.com/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
//	pPM->setPropertyString(cmSTATE_NOTIFY_SERVICE_URL1, "http://mvrsstatenotify.sorenson.com/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
//	pPM->setPropertyString(cmMESSAGE_SERVICE_URL1, "http://mvrsmessage.sorenson.com/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
//	pPM->setPropertyString(cmVRS_SERVER, "call.svrs.tv", WillowPM::PropertyManager::Persistent);
	
	pPM->setPropertyInt(RING_GROUP_ENABLED, 1, WillowPM::PropertyManager::Persistent);
	
	
#if APPLICATION == APP_NTOUCH_IOS

	// Save and Restore MAC Address from KeyChain and Property Table
	NSString *keyChainMACAddress = [UICKeyChainStore stringForKey:[NSString stringWithUTF8String:MAC_ADDRESS]];
	
	// Reset MAC Address if the device model doesn't match the stored device model
	NSString *storedDevice = [[SCIPropertyManager sharedManager] stringValueForPropertyNamed:@"DeviceModel" inStorageLocation:SCIPropertyStorageLocationPersistent withDefault:@""];
	struct utsname systemInfo;
	uname(&systemInfo);
	NSString *currentDevice = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
	if (storedDevice != nil && ![storedDevice isEqualToString:currentDevice]) {
		keyChainMACAddress = @"";
	}
	
	// Check if a valid MACAddress is in the KeyChain
	if ([keyChainMACAddress sci_isValidMACAddress])
	{
		g_strMACAddress = [keyChainMACAddress UTF8String];
		pPM->setPropertyString(MAC_ADDRESS, [keyChainMACAddress UTF8String], WillowPM::PropertyManager::Persistent);
	}
	else
	{
		// Check if a MACAddress is in the PropertyTable
		std::string macAddress;
		pPM->getPropertyString(MAC_ADDRESS, &macAddress, "");
		keyChainMACAddress = [NSString stringWithUTF8String:macAddress.c_str()];

		if (![storedDevice isEqualToString:currentDevice]) {
			keyChainMACAddress = @"";
		}
		
		if (![keyChainMACAddress sci_isValidMACAddress])
		{
			char szMacAddress[nMAX_MAC_ADDRESS_WITH_COLONS_STRING_LENGTH +1] = "";
			stiGenerateUniqueID(szMacAddress);
			keyChainMACAddress = [NSString stringWithUTF8String:szMacAddress];
		}
		
		[UICKeyChainStore setString:keyChainMACAddress forKey:[NSString stringWithUTF8String:MAC_ADDRESS]];
		g_strMACAddress = [keyChainMACAddress UTF8String];
	}
#endif
	
	pPM->saveToPersistentStorage();
#if APPLICATION == APP_NTOUCH_MAC
	NSString *dynamicDataFolder = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"ntouch-mac"];
#elif APPLICATION == APP_NTOUCH_IOS
	NSString *dynamicDataFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
#endif
	dynamicDataFolder = [dynamicDataFolder stringByAppendingString:@"/"];
	mVideophoneEngine = videophoneEngineCreate(product, fullVersion.UTF8String, true, true, dynamicDataFolder.UTF8String, staticDirectory, EngineCallback, (size_t)self);
	
	mVideophoneEngine->ServiceOutageMessageConnect([self](std::shared_ptr<vpe::ServiceOutageMessage> message) {
		NSLog(@"Service Outage Signal Received: %s", message->MessageText.c_str());
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInteger:static_cast<int>(message->MessageType)], @"type", [NSString stringWithUTF8String:message->MessageText.c_str()], @"message", nil];
		
		[self postOnMainThreadNotificationName:SCINotificationServiceOutageMessageUpdated userInfo:userInfo];
	});
	
	IstiContactManager *contactManager = mVideophoneEngine->ContactManagerGet();
	mSignalConnections.push_back(contactManager->contactErrorSignalGet ().Connect ([self](IstiContactManager::Response response, stiHResult hResult) {
		if (hResult == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
			[self postOnMainThreadNotificationName:SCINotificationCoreOfflineGenericError userInfo:nil];
		} else {
			switch (response) {
				case IstiContactManager::EditError:
					[self postOnMainThreadNotificationName:SCINotificationContactListItemEditError userInfo:nil];
					[self performBlockOnMainThread:^{
						// Just refresh if it couldn't be found:
						[[SCIContactManager sharedManager] refresh];
					}];
					break;
				case IstiContactManager::AddError:
					[self postOnMainThreadNotificationName:SCINotificationContactListItemAddError userInfo:nil];
					[self performBlockOnMainThread:^{
						// Just refresh if it couldn't be found:
						[[SCIContactManager sharedManager] refresh];
					}];
					break;
				case IstiContactManager::DeleteError:
					[self postOnMainThreadNotificationName:SCINotificationContactListItemDeleteError userInfo:nil];
					[self performBlockOnMainThread:^{
						// Just refresh if it couldn't be found:
						[[SCIContactManager sharedManager] refresh];
					}];
					break;
				case IstiContactManager::ImportError:
					NSLog(@"Message 0x%02x: Contact list import complete", response);
					[self postOnMainThreadNotificationName:SCINotificationContactListImportError userInfo:nil];
					break;
				default:
					break;
			}
		}
	}));
	
	mSignalConnections.push_back(contactManager->favoriteErrorSignalGet ().Connect ([self](IstiContactManager::Response response, stiHResult hResult) {
		if (hResult == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
			[self postOnMainThreadNotificationName:SCINotificationCoreOfflineGenericError userInfo:nil];
		} else {
			switch (response) {
				case IstiContactManager::AddError:
				case IstiContactManager::EditError:
				case IstiContactManager::DeleteError:
				case IstiContactManager::GetListError:
				case IstiContactManager::SetListError:
				case IstiContactManager::Unknown:
					NSLog(@"Message 0x%02x: Favorite List Error ***", response);
					[self postOnMainThreadNotificationName:SCINotificationFavoriteListError userInfo:nil];
					break;
				default:
					break;
			}
		}
	}));
	
	mSignalConnections.push_back(mVideophoneEngine->contactReceivedSignalGet ().Connect ([self](const SstiSharedContact &contact) {
		NSLog(@"Received shared contact: %s %s %s", contact.Name.c_str(), contact.CompanyName.c_str(), contact.DialString.c_str());
		NSString *name = [NSString stringWithCString:contact.Name.c_str() encoding:[NSString defaultCStringEncoding]];
		NSString *companyName = [NSString stringWithCString:contact.CompanyName.c_str() encoding:[NSString defaultCStringEncoding]];
		NSString *dialString = [NSString stringWithCString:contact.DialString.c_str() encoding:[NSString defaultCStringEncoding]];
		NSNumber *numberType = [NSNumber numberWithInt:contact.eNumberType];
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:name, @"name", companyName, @"companyName", dialString, @"dialString", numberType, @"numberType", nil];

		[self postOnMainThreadNotificationName:SCINotificationContactReceived userInfo:userInfo];
	}));
	
	IstiBlockListManager *blockListManager = mVideophoneEngine->BlockListManagerGet();
	mSignalConnections.push_back(blockListManager->errorSignalGet ().Connect ([self](IstiBlockListManager::Response response, stiHResult stiHResult, unsigned int requestID, const std::string itemID) {
		if (stiHResult == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
			[self postOnMainThreadNotificationName:SCINotificationCoreOfflineGenericError userInfo:nil];
		} else {
			switch (response) {
				case IstiBlockListManager::Denied:
					NSLog(@"Message 0x%02x: Blocked list item is whitelisted", response);
					// Only post this notification if we are authenticated.  We may get this event if we request the blocked list after we have been logged out, so don't post this
					// notification in that case.
					if (self.isAuthenticated) {
						[self postOnMainThreadNotificationName:SCINotificationBlockedNumberWhitelisted userInfo:nil];
					}
					break;
				case IstiBlockListManager::EditError:
					[self postOnMainThreadNotificationName:SCINotificationBlockedListItemEditError userInfo:nil];
					[self performBlockOnMainThread:^{
						// Just refresh if it couldn't be found:
						[[SCIBlockedManager sharedManager] refresh];
					}];
					break;
				case IstiBlockListManager::AddError:
					[self postOnMainThreadNotificationName:SCINotificationBlockedListItemAddError userInfo:nil];
					break;
				default:
					break;
			}
		}
	}));
	
	mSignalConnections.push_back(mVideophoneEngine->clientAuthenticateSignalGet().Connect([self](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		if (result->coreErrorNumber == CstiCoreResponse::eNO_ERROR) {
			self.authenticated = YES;
		}
		
		ServiceResponseSP<ClientAuthenticateResult> resultCopy = result;
		
		[self performBlockOnMainThread:^{
			[self handleClientAuthenticateResultWithResponse:resultCopy];
		}];
	}));
	
	mSignalConnections.push_back(mVideophoneEngine->userAccountInfoReceivedSignalGet().Connect([self](const ServiceResponseSP<CstiUserAccountInfo>& response) {
		if (response->coreErrorNumber == CstiCoreResponse::eNO_ERROR) {
			ServiceResponseSP<CstiUserAccountInfo> responseCopy = response;
			[self performBlockOnMainThread:^{
				[self handleUserAccountInfoGetResultWithResponse:responseCopy];
			}];
		}
	}));
	
	mSignalConnections.push_back(mVideophoneEngine->deviceLocationTypeChangedSignalGet().Connect([self](const DeviceLocationType& result) {
		[self postOnMainThreadNotificationName:SCINotificationDeviceLocationTypeChanged userInfo:nil];
	}));
	
	mSignalConnections.push_back(IstiMessageViewer::InstanceGet()->stateSetSignalGet().Connect([self](IstiMessageViewer::EState state) {
#if DEBUG
			NSString *str = NSStringFromFilePlayState(state);
			NSLog(@"File Play State: %05x: %@", state, str);
#endif
			[self performBlockOnMainThread:^{
				[[SCIMessageViewer sharedViewer] updateStateWithEstiState:state];
			}];
	}));
	
	mVideophoneEngine->Startup();
#if APPLICATION == APP_NTOUCH_IOS
	m_oRemoteClient = new CstiVRCLClient(RemoteVRCLCallBack, (size_t)mVideophoneEngine, false);
	m_oRemoteClient->Startup();
	

	
#endif
	
	// Set VCO Type to stored VCO type
	EstiVcoType vcoType = estiVCO_NONE;
	pPM->getPropertyInt(PropertyDef::VCOPreference, (int *)&vcoType, (int)estiVCO_NONE);
	mVideophoneEngine->VCOTypeSet(vcoType);
	
	// Set VCOUseSet based on stored VCOActive property
	EstiBool vcoActive = estiFALSE;
	pPM->getPropertyInt(PropertyDef::VCOActive, (int *)&vcoActive, stiVCO_ACTIVE_DEFAULT);
	mVideophoneEngine->VCOUseSet(vcoActive);
	
	// Set VCO callback number to number stored in the property manager so that the correct number will
	// be sent to Terpnet when an incoming call arrives before any outgoing calls have been placed.
	std::string voiceCarryOverNumber;
	pPM->getPropertyString(PropertyDef::VoiceCallbackNumber, &voiceCarryOverNumber, "", WillowPM::PropertyManager::Persistent);
	mVideophoneEngine->VCOCallbackNumberSet(voiceCarryOverNumber.c_str());
	
	// user isn't allowed to use app without accepting so set this to true
	mVideophoneEngine->DefaultProviderAgreementSet(true);

	
	[self startupNetwork];
	[self sendDeviceInfo];
	
	//TODO: Start checking for updates.
//	[self startUpdateCheckTimer];
	
	[self setConnecting:YES postConnectedDidChange:YES];
	
	// Determine if HEVC SignMail playback is supported for the device
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	if (@available(iOS 11.0, macOS 10.13, *)) {
		msgMgr->HEVCSignMailPlaybackSupportedSet(true);
	} else {
		msgMgr->HEVCSignMailPlaybackSupportedSet(false);
	}
	
	if (SCIVideophoneEngine.sharedEngine.debugEnabled) {
		SCIVideophoneEngine.sharedEngine.vrclEnabled = YES;
	}
    
	NSString * uiLanguage = [[[[[NSBundle mainBundle] preferredLocalizations] objectAtIndex:0] LanguageCode] stringByReplacingOccurrencesOfString:@"-" withString:@"_"];
    pPM->setPropertyString(UI_LANGUAGE, [uiLanguage UTF8String], WillowPM::PropertyManager::Persistent);
}

- (void)startupAndUpdateNetworkSettings:(BOOL)updateNetworkSettings completion:(void (^)(NSError *))block
{
	mVideophoneEngine->Startup();
	if (updateNetworkSettings) {
		[self updateNetworkSettingsWithCompletion:block];
	}
}

- (void)startupAndUpdateNetworkSettings:(BOOL)updateNetworkSettings
{
	[self startupAndUpdateNetworkSettings:updateNetworkSettings completion:nil];
}

- (void)startup
{
	mVideophoneEngine->Startup();
	[self updateNetworkSettings];
}

- (void)shutdown
{
	if (mVideophoneEngine) {
		mVideophoneEngine->Shutdown();
	}
}

- (void)forceServiceOutageCheck {
	
	if (mVideophoneEngine) {
		mVideophoneEngine->ServiceOutageForceCheck();
	}
}

#pragma mark - Public API: Network State

- (void)startupNetworkAndUpdateNetworkSettings:(BOOL)updateNetworkSettings completion:(void (^)(NSError *))block
{
	mVideophoneEngine->NetworkStartup();
	if (updateNetworkSettings) {
		[self updateNetworkSettingsWithCompletion:block];
	}
}

- (void)startupNetworkAndUpdateNetworkSettings:(BOOL)updateNetworkSettings
{
	[self startupNetworkAndUpdateNetworkSettings:updateNetworkSettings completion:nil];
}

- (void)startupNetwork
{
	[self startupNetworkAndUpdateNetworkSettings:YES];
}

- (void)shutdownNetwork
{
	mVideophoneEngine->NetworkShutdown();
}

- (void)clientReRegister {
	if(mVideophoneEngine) {
		self.isSipRegistered = NO;
		mVideophoneEngine->ClientReRegister();
	}
}

- (void)restartConnection
{
#if APPLICATION == APP_NTOUCH_IOS
	[self updateNetworkSettingsWithCompletion:nil];
#endif
	self.isSipRegistered = NO;

	if(mVideophoneEngine) {
		mVideophoneEngine->NetworkChangeNotify();
	}
	SCILog(@"Network has changed.");
}

-(void)sipStackRestart
{
	if (mVideophoneEngine) {
		mVideophoneEngine->SipStackRestart();
	}
}

- (void)resetNetworkMACAddress
{
#if APPLICATION == APP_NTOUCH_IOS
	// This will reset the saved MAC address to a new install state, the app must now be restarted
	[self sendRemoteLogEvent:@"AppEvent=ResetMACAddress"];
	g_strMACAddress.clear();
	[UICKeyChainStore setString:@"" forKey:[NSString stringWithUTF8String:MAC_ADDRESS]];
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	pPM->removeProperty(MAC_ADDRESS, true);
	pPM->saveToPersistentStorage();
	
	[self performSelector:@selector(exitApplication) withObject:nil afterDelay:2.0];
#endif
}

- (void)exitApplication
{
	NSLog(@"Exiting");
	exit(1);
}

#pragma mark - Helpers: Network State

- (void)sendDeviceInfo
{
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	// send product identifier to Core (ie iPad2,2)
	size_t size = 256;
	char name[256] = "Unknown";
	NSString *systemVersion = @"Unknown";
#if APPLICATION == APP_NTOUCH_MAC
	sysctlbyname("hw.model", &name, &size, NULL, 0);
	unsigned major, minor, bug;
	[NSApp getSystemVersionMajor:&major minor:&minor bugFix:&bug];
	systemVersion = [NSString stringWithFormat:@"%u.%u.%u", major, minor, bug];
#elif APPLICATION == APP_NTOUCH_IOS
	sysctlbyname("hw.machine", &name, &size, NULL, 0);
	systemVersion = [[UIDevice currentDevice] systemVersion];
	NSString *udid = [UIDevice currentDevice].identifierForVendor.UUIDString;
	if (udid != nil)
	{
		pPM->setPropertyString("DeviceVendorIdentifier", [udid UTF8String], WillowPM::PropertyManager::Persistent);
		pPM->PropertySend("DeviceVendorIdentifier", estiPTypePhone);
	}
#endif
	pPM->setPropertyString("DeviceModel", name, WillowPM::PropertyManager::Persistent);
	pPM->PropertySend("DeviceModel", estiPTypePhone);
	
	// Check if device is managed by MDM
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *managedConfig = [userDefaults dictionaryForKey:@"com.apple.configuration.managed"];
	
	// We check for the key in the managed configuration for iOS, and user defaults for macOS (which also requires a custom configuration profile installed on the device).
	if ((managedConfig && [managedConfig objectForKey:@"MDMEnabled"]) || (userDefaults && [userDefaults objectForKey:@"MDMEnabled"]))
	{
		pPM->setPropertyInt(MANAGED_DEVICE, 1);
		pPM->PropertySend(MANAGED_DEVICE, estiPTypePhone);
	}

	pPM->setPropertyString("SystemVersion", [systemVersion UTF8String], WillowPM::PropertyManager::Persistent);
	pPM->PropertySend("SystemVersion", estiPTypePhone);
	
#if APPLICATION == APP_NTOUCH_IOS
	// send telecom carrier to Core (ie Verizon)
	CTTelephonyNetworkInfo *netinfo = [[CTTelephonyNetworkInfo alloc] init];
	CTCarrier *carrier = [netinfo subscriberCellularProvider];
	if (carrier && carrier.carrierName)
	{
		SCILog(@"Carrier Name: %s", [carrier.carrierName UTF8String]);
		pPM->setPropertyString("TelecomCarrier", [carrier.carrierName UTF8String], WillowPM::PropertyManager::Persistent);
		pPM->PropertySend("TelecomCarrier", estiPTypePhone);
	}
#endif
}

- (void)sendDeviceToken:(NSString*)deviceToken
{
	NSString *token = deviceToken ? deviceToken : @"";
	
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	pPM->setPropertyString("DeviceToken", [token UTF8String], WillowPM::PropertyManager::Persistent);
	pPM->PropertySend("DeviceToken", estiPTypePhone);
}

- (void)sendNotificationTypes:(NSUInteger)types
{
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	pPM->setPropertyInt("PushNotificationEnabledTypes", (int)types, WillowPM::PropertyManager::Persistent);
	pPM->PropertySend("PushNotificationEnabledTypes", estiPTypePhone);
}

- (void)setLDAPCredentials:(NSDictionary *)LDAPCredentials completion:(void (^)(void))block
{
	#ifdef stiLDAP_CONTACT_LIST
	NSString *LDAPUsername = [LDAPCredentials objectForKey:@"LDAPUsername"];
	NSString *LDAPPassword = [LDAPCredentials objectForKey:@"LDAPPassword"];
	auto username = std::string([LDAPUsername UTF8String], [LDAPUsername lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
	auto password = std::string([LDAPPassword UTF8String], [LDAPPassword lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
	IstiLDAPDirectoryContactManager *LDAPManager = [[SCIVideophoneEngine sharedEngine] videophoneEngine]->LDAPDirectoryContactManagerGet();
	LDAPManager->LDAPDistinguishedNameSet(username);
	LDAPManager->LDAPPasswordSet(password);
	LDAPManager->LDAPSearchFilterSet("(objectClass=organizationalPerson)");
	
	LDAPManager->LDAPCredentialsValidate(username, password);
	#endif
	
	if(block)
		block();
}

- (void)clearLDAPCredentialsAndContacts
{
	#ifdef stiLDAP_CONTACT_LIST
	IstiLDAPDirectoryContactManager *LDAPManager = [[SCIVideophoneEngine sharedEngine] videophoneEngine]->LDAPDirectoryContactManagerGet();
	LDAPManager->clearContacts();
	LDAPManager->LDAPDistinguishedNameSet("");
	LDAPManager->LDAPPasswordSet("");
	#endif
}


- (void)updateNetworkSettings
{
	[self updateNetworkSettingsWithCompletion:nil];
}

- (void)updateNetworkSettingsWithCompletion:(void (^)(NSError *))block
{
	block = [block copy];
	bool ipv4Enabled = false;
	std::string szBuf;

	if (stiRESULT_SUCCESS == stiGetLocalIp(&szBuf, estiTYPE_IPV4)) {
		
		WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance ();
		CstiCoreRequest *coreRequest = new CstiCoreRequest();
		
		pPm->setPropertyString(IPv4IpAddress, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
		coreRequest->phoneSettingsSet(IPv4IpAddress, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		
		szBuf.clear ();
		
		if (stiRESULT_SUCCESS == stiGetNetSubnetMaskAddress(&szBuf, estiTYPE_IPV4)) {
			pPm->setPropertyString(IPv4SubnetMask, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv4SubnetMask, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
		
		szBuf.clear ();
		
		if (stiRESULT_SUCCESS == stiGetDNSAddress(0, &szBuf, estiTYPE_IPV4)) {
			pPm->setPropertyString(IPv4Dns1, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv4Dns1, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
		
		szBuf.clear ();
		
		if (stiRESULT_SUCCESS == stiGetDNSAddress(1, &szBuf, estiTYPE_IPV4)) {
			pPm->setPropertyString(IPv4Dns2, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv4Dns2, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
		
		szBuf.clear ();
		
		//DHCP and default gateway methods not implemented on iOS
		//pPm->setPropertyInt(NetDhcp, stiOSGetDHCPEnabled() ? 1 : 0, WillowPM::PropertyManager::Persistent);
		//coreRequest->phoneSettingsSet(NetDhcp, stiOSGetDHCPEnabled() ? 1 : 0);
		
#if APPLICATION == APP_NTOUCH_MAC
		if (stiRESULT_SUCCESS == stiGetDefaultGatewayAddress(&szBuf, estiTYPE_IPV4)) {
			pPm->setPropertyString(IPv4GatewayIp, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv4GatewayIp, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
#endif
		
		[self sendBasicCoreRequest:coreRequest completion:block];
		
		ipv4Enabled = true;
	}
	
	if (stiRESULT_SUCCESS == stiGetLocalIp(&szBuf, estiTYPE_IPV6)) {
		
		WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance ();
		CstiCoreRequest *coreRequest = new CstiCoreRequest();
		
		pPm->setPropertyString(IPv6IpAddress, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
		coreRequest->phoneSettingsSet(IPv6IpAddress, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		
		szBuf.clear ();
		
		if (stiRESULT_SUCCESS == stiGetDNSAddress(0, &szBuf, estiTYPE_IPV6)) {
			pPm->setPropertyString(IPv6Dns1, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv6Dns1, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
		
		szBuf.clear ();
		
		if (stiRESULT_SUCCESS == stiGetDNSAddress(1, &szBuf, estiTYPE_IPV6)) {
			pPm->setPropertyString(IPv6Dns2, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
			coreRequest->phoneSettingsSet(IPv6Dns2, szBuf.c_str(), CstiCoreRequest::eSETTING_TYPE_STRING);
		}
		
		//DHCP and default gateway methods not implemented on iOS
		//pPm->setPropertyInt(NetDhcp, stiOSGetDHCPEnabled() ? 1 : 0, WillowPM::PropertyManager::Persistent);
		//coreRequest->phoneSettingsSet(NetDhcp, stiOSGetDHCPEnabled() ? 1 : 0);

		[self sendBasicCoreRequest:coreRequest completion:block];
	}
	
	if(mVideophoneEngine) {
		NSLog(@"Network: Using IPv6: %@", ipv4Enabled ? @"NO" : @"YES");
		mVideophoneEngine->IPv6EnabledSet(!ipv4Enabled);
	}

}

#pragma mark - Public API: Setting Application Events (Tunneling)

- (void)applicationHasNetwork
{
	if(mVideophoneEngine) {
		mVideophoneEngine->TunnelManagerGet()->ApplicationEventSet (IstiTunnelManager::eApplicationIOSHasNetwork);
	}
}

- (void)applicationHasNoNetwork
{
	if(mVideophoneEngine) {
		mVideophoneEngine->TunnelManagerGet()->ApplicationEventSet (IstiTunnelManager::eApplicationIOSHasNoNetwork);
	}
}

- (void)applicationWillEnterForeground
{
	if(mVideophoneEngine) {
		mVideophoneEngine->TunnelManagerGet()->ApplicationEventSet (IstiTunnelManager::eApplicationIOSWillEnterForeground);
		mVideophoneEngine->useProxyKeepAliveSet (true);
	}
}

- (void)applicationDidEnterBackground
{
	if(mVideophoneEngine) {
		mVideophoneEngine->TunnelManagerGet()->ApplicationEventSet (IstiTunnelManager::eApplicationIOSDidEnterBackground);
		mVideophoneEngine->useProxyKeepAliveSet (false);
		
		[SCIPropertyManager.sharedManager sendProperties];
	}
}

- (void)setRestartReason:(SCIRestartReason)reason
{
	IstiPlatform::EstiRestartReason eRestartReason = IstiPlatformEstiRestartReasonFromSCIRestartReason(reason);
	IstiPlatform *platform = IstiPlatform::InstanceGet();
	platform->RestartSystem(eRestartReason);
}

#pragma mark - Public API: Account State

- (void)associateWithBlock:(void (^)(NSError *error))block
{
	NSAssert(self.username.length > 0, @"expected username to be set");
	NSAssert(self.password.length > 0, @"expected password to be set");
	
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	
	coreRequest->userAccountAssociate(NULL, // no need to specify name
									  [self.username UTF8String],
									  [self.password UTF8String],
									  estiFALSE, // not used anymore
									  estiFALSE, // bMatchingPrimaryUserRequired
									  estiFALSE   // bMultipleEndpointEnabled
									  );
	
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (BOOL)loginUsingCache
{
	return mVideophoneEngine->logInUsingCache(std::string([self.username UTF8String], [self.username lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), std::string([self.password UTF8String], [self.password lengthOfBytesUsingEncoding:NSUTF8StringEncoding])) == stiRESULT_SUCCESS;
}

- (void)loginUsingCacheWithBlock:(void (^)(NSDictionary *accountStatusDictionary, NSDictionary *errors, void (^proceedBlock)(void), void (^terminateBlock)(void)))block
{
	stiHResult res = mVideophoneEngine->logInUsingCache(std::string(self.username.UTF8String, [self.username lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), std::string(self.password.UTF8String, [self.password lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
	
	if (res == stiRESULT_SUCCESS)
	{
		self.isLoggingIn = YES; //Gets set to NO in the UI when the login window closes
		self.loginBlock = block; //The block will used in the eCLIENT_AUTHENTICATE_RESULT and eUSER_ACCOUNT_INFO_GET_RESULT handlers
	}
	else
	{
		// If logInUsingCache fails perform normal login
		// instead of reporting error to UI
		[self loginWithBlock: block];
	}
}

- (void)loginWithBlock:(void (^)(NSDictionary *accountStatusDictionary, NSDictionary *errors, void (^proceedBlock)(void), void (^terminateBlock)(void)))block
{
	unsigned int requestId = 0;
	stiHResult res = mVideophoneEngine->CoreLogin(std::string(self.username.UTF8String, [self.username lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), std::string(self.password.UTF8String, [self.password lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), NULL, &requestId, self.loginName.UTF8String);
	
	if (res == stiRESULT_SUCCESS)
	{
		if (self.password)
		{
			self.isLoggingIn = YES; //Gets set to NO in the UI when the login window closes
		}
		self.loginBlock = block; //The block will used in the eCLIENT_AUTHENTICATE_RESULT and eUSER_ACCOUNT_INFO_GET_RESULT handlers
	}
	else
	{
		[self performBlockOnMainThread:^{
			NSError *error = [NSError errorWithDomain:SCIErrorDomainCoreRequestSend code:res userInfo:nil];
			if (block)
				block(nil, @{ SCIErrorKeyLogin : error }, nil, nil);
		}];
	}
}

- (void)authorizeAccountWithCompletion:(void (^)(NSDictionary *statusDictionary, NSDictionary *errorDictionary))block
{
	self.fetchingAuthorization = YES;
	__block NSMutableDictionary *mutableStatusDictionary = [[NSMutableDictionary alloc] init];
	__block NSMutableDictionary *mutableErrorDictionary = [[NSMutableDictionary alloc] init];
	__block NSUInteger operationCount = 2;
	__block NSObject *operationCountLock = [[NSObject alloc] init];
	
	void (^joinOperationCompletionBlock)(NSArray *, NSArray *, NSArray *, NSArray *) = ^(NSArray *objects, NSArray *keys, NSArray *errors, NSArray *errorKeys){
		if (objects && keys) {
			@synchronized(mutableStatusDictionary) {
				NSUInteger index = 0;
				for (index = 0; index < objects.count; index++) {
					id object = objects[index];
					id<NSCopying> key = keys[index];
					[mutableStatusDictionary setObject:object forKey:key];
				}
			}
		}
		
		if (errors) {
			@synchronized(mutableErrorDictionary) {
				NSUInteger index = 0;
				for (index = 0; index < errors.count; index++) {
					id error = errors[index];
					id<NSCopying> errorKey = errorKeys[index];
					[mutableErrorDictionary setObject:error forKey:errorKey];
				}
			}
		}
		
		NSUInteger currentCount = NSUIntegerMax;
		@synchronized(operationCountLock) {
			operationCount = operationCount - 1;
			currentCount = operationCount;
		}
		
		if (currentCount == 0) {
			self.fetchingAuthorization = NO;
			
			NSDictionary *statusDictionaryOut = [mutableStatusDictionary copy];
			NSDictionary *errorDictionaryOut = [mutableErrorDictionary copy];
			if (block)
				block(statusDictionaryOut, errorDictionaryOut);
		}
		else {
			self.processingLogin = NO;
		}
	};
	
	[self fetchDynamicProviderAgreementsWithCompletionHandler:^(NSArray *agreements, NSError *error) {
		NSArray *objectsOut = nil;
		NSArray *keysOut = nil;
		NSArray *errorsOut = nil;
		NSArray *errorKeysOut = nil;
		
		if (agreements) {
			objectsOut = @[@(SCIProviderAgreementTypeDynamic), agreements];
			keysOut = @[SCINotificationKeyProviderAgreementType, SCINotificationKeyAgreements];
			errorsOut = nil;
			errorKeysOut = nil;
		} else if (error) {
			objectsOut = nil;
			keysOut = nil;
			errorsOut = @[error];
			errorKeysOut = @[SCIErrorKeyFetchAgreements];
		}
		
		joinOperationCompletionBlock(objectsOut, keysOut, errorsOut, errorKeysOut);
	}];
	
	if (self.interfaceMode != SCIInterfaceModeHearing) {
		[self fetchEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:^(SCIEmergencyAddress *address, NSError *error) {
			NSArray *objectsOut = nil;
			NSArray *keysOut = nil;
			NSArray *errorsOut = nil;
			NSArray *errorKeysOut = nil;
			
			
			id addressObject = nil;
			id<NSCopying> addressKey = nil;
			
			NSError *fetchEmergencyInfoError = nil;
			NSString *fetchEmergencyInfoErrorKey = nil;
			
			if (!error) {
				addressObject = address;
				addressKey = SCINotificationKeyEmergencyAddress;
			} else {
				fetchEmergencyInfoError = error;
				fetchEmergencyInfoErrorKey = SCIErrorKeyFetchEmergencyAddressAndEmergencyAddressProvisioningStatus;
			}
			
			objectsOut = ^{
				NSMutableArray *mutableObjects = [[NSMutableArray alloc] init];
				
				if (addressObject) [mutableObjects addObject:addressObject];
				
				return [mutableObjects copy];
			}();
			
			keysOut = ^{
				NSMutableArray *mutableKeys = [[NSMutableArray alloc] init];
				
				if (addressKey) [mutableKeys addObject:addressKey];
				
				return [mutableKeys copy];
			}();
			
			errorsOut = ^{
				NSMutableArray *mutableErrors = [[NSMutableArray alloc] init];
				
				if (fetchEmergencyInfoError) [mutableErrors addObject:fetchEmergencyInfoError];
				
				return [mutableErrors copy];
			}();
			
			errorKeysOut = ^{
				NSMutableArray *mutableErrorKeys = [[NSMutableArray alloc] init];
				
				if (fetchEmergencyInfoErrorKey) [mutableErrorKeys addObject:fetchEmergencyInfoErrorKey];
				
				return [mutableErrorKeys copy];
			}();
			
			joinOperationCompletionBlock(objectsOut, keysOut, errorsOut, errorKeysOut);
		}];
	} else {
		joinOperationCompletionBlock(nil, nil, nil, nil);
	}
}

- (void)fetchLocalInterfaceGroupWithCompletion:(void (^)(NSNumber *interfaceModeNumber, NSError *error))block
{
	CstiCoreRequest *request = new CstiCoreRequest();
	request->userInterfaceGroupGet();
	
	[self sendCoreRequest:request completion:^(CstiCoreResponse *response, NSError *sendError) {
		NSNumber *interfaceModeNumberOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto pszTemp = response->ResponseStringGet(0);
			int interfaceModeInt = atoi(pszTemp.c_str());
			
			interfaceModeNumberOut = [NSNumber numberWithInt:interfaceModeInt];
			errorOut = nil;
		} else {
			interfaceModeNumberOut = nil;
			errorOut = sendError;
		}
		
		if (block)
			block(interfaceModeNumberOut, errorOut);
	}];
}

- (void)logout
{
	[self logoutWithBlock:nil];
}

- (void)logoutWithBlock:(void (^)())block
{
	if (self.isAuthenticated) {
		// Send Core a blank Push Token to prevent core sending
		// pushes when logged out.  (Bugs 30611, 31609)
		if (self.deviceToken.length) {
			CstiCoreRequest *coreRequest = new CstiCoreRequest();
			CstiCoreRequest::ESettingType settingType = CstiCoreRequest::eSETTING_TYPE_STRING;
			std::string propertyName = "DeviceToken";
			std::string propertyValue = "";
			coreRequest->phoneSettingsSet (propertyName.c_str(), propertyValue.c_str(), settingType);
			unsigned int requestId = 0;
			mVideophoneEngine->CoreRequestSendEx(coreRequest, &requestId);
		}
		
		// Save all changes the user has made before logging out
		[SCIPropertyManager.sharedManager sendProperties];
		
		// Make sure recent calls are cleared from the device
#if APPLICATION == APP_NTOUCH_IOS
		if (@available(iOS 12.0, *)) {
			[NSUserActivity deleteAllSavedUserActivitiesWithCompletionHandler:^{}];
		}
#endif
		
		self.loggingOut = YES;
		unsigned int unRequestId = 0;
		mVideophoneEngine->CoreLogout(NULL, &unRequestId);
		
		// clear registration with gatekeeper
		SstiUserPhoneNumbers sUserPhoneNumbers;
		memset(&sUserPhoneNumbers, 0, sizeof(SstiUserPhoneNumbers));
		mVideophoneEngine->UserPhoneNumbersSet(&sUserPhoneNumbers);
		self.phoneUserId = nil;
		self.ringGroupUserId = nil;
		
#ifdef stiLDAP_CONTACT_LIST
		[self clearLDAPCredentialsAndContacts];
#endif
		[self handleCoreRequestId:unRequestId withBlock:^(CstiCoreResponse *response, NSError *sendError) {
			self.loggingOut = NO;
			self.authenticated = NO;
			self.processingLogin = NO;
			[self performBlockOnMainThread:block];
		}];
	} else {
		[self performBlockOnMainThread:block];
	}
}

- (void)heartbeat
{
	if (mVideophoneEngine) {
		mVideophoneEngine->StateNotifyHeartbeatRequest();
	}
}


#ifdef stiENABLE_H46017
//extern bool g_bH46017Enabled;

//- (BOOL) getH46017Enabled {
//	return g_bH46017Enabled?YES:NO;
//}
//
//- (void) setH46017Enabled: (BOOL)enabled {
//	SCILog(@"Start: H46017: %d", g_bH46017Enabled);
//	if (g_bH46017Enabled != enabled) {
//		g_bH46017Enabled = enabled;
//		mVideophoneEngine->NetworkChangeNotify();
//	}
//}
#endif

#pragma mark - Public API: Dialing

- (void)setDialSource:(SCIDialSource)source
{
	self.callDialSource = source;
}

- (void)setDialMethod:(SCIDialMethod)method
{
	self.callDialMethod = method;
}


- (SCICall *)dialCallWithDialString:(NSString *)dialString callListName:(NSString *)callListName relayLanguage:(NSString *)language vco:(BOOL)vco forceEncryption:(BOOL)forceEncryption error:(NSError **)errorOut
{
	//This method is preserved to avoid breaking compatibility.  Only estiVCO_ONE_LINE or estiVCO_NONE calls
	//can be placed using this method.
	SCIVoiceCarryOverType vcoType = (vco) ? SCIVoiceCarryOverTypeOneLine : SCIVoiceCarryOverTypeNone;
	return [self dialCallWithDialString:dialString callListName:callListName relayLanguage:language vcoType:vcoType forceEncryption:forceEncryption error:errorOut];
}

- (SCICall *)dialCallWithDialString:(NSString *)dialString callListName:(NSString *)callListName relayLanguage:(NSString *)language vcoType:(SCIVoiceCarryOverType)vco forceEncryption:(BOOL)forceEncryption error:(NSError **)errorOut;
{
	if (self.isStarted == NO)
	{
		NSString *localizedDesc = @"Connecting to server. Please try again later.";
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
		if (errorOut) {
			*errorOut = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEngineNotStarted userInfo:userInfo];
		}
		return nil;
	}
	if (dialString == nil || dialString.length == 0)
	{
		NSString *localizedDesc = @"No phone number supplied.";
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
		if (errorOut) {
			*errorOut = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEmptyDialString userInfo:userInfo];
		}
		return nil;
	}
	
	//set voice callback number into VPE defaulting to NULL
	std::string voiceCarryOverNumber;
	if (vco == SCIVoiceCarryOverTypeTwoLine) {
		WillowPM::PropertyManager *pPM = WillowPM::PropertyManager::getInstance();
		pPM->getPropertyString(PropertyDef::VoiceCallbackNumber, &voiceCarryOverNumber, "", WillowPM::PropertyManager::Persistent);
	}
	mVideophoneEngine->VCOCallbackNumberSet(voiceCarryOverNumber.c_str());
	
	if (self.callDialMethod == SCIDialMethodUnknown && vco != SCIVoiceCarryOverTypeNone) {
		self.callDialMethod = SCIDialMethodUnknownWithVCO;
	}
	
	IstiCall *call = NULL;
	stiHResult result = stiRESULT_SUCCESS;
	
	// If this is a call from a Push Notification then also log a user event stating this.
	if (self.callDialSource == SCIDialSourcePushNotificationMissedCall)
	{
		[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=MissedCallPushNotificationSelected"];

	}
	else if (self.callDialSource == SCIDialSourcePushNotificationMissedCall)
	{
		[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"UserEvent=SignMailPushNotificationSelected"];
	}

	result = mVideophoneEngine->CallDial(EstiDialMethodForSCIDialMethod(self.callDialMethod), [dialString UTF8String], [callListName UTF8String], [language UTF8String], EstiDialSourceForSCIDialSource(self.callDialSource), forceEncryption, &call);
	
	// Ensure dial source is set back to unknown to ensure we do not incorrectly log additional calls with this dial source.
	[self setDialSource:SCIDialSourceUnknown];
	
	[self setDialMethod:SCIDialMethodUnknown];
	
	if (stiIS_ERROR(result))
	{
		stiHResult resultCode = stiRESULT_CODE(result);
		if (errorOut)
		{
			NSString *localizedDesc = @"An error occurred while placing the call.";
			if (resultCode == stiRESULT_UNABLE_TO_DIAL_SELF) {
				localizedDesc = @"You cannot dial your own phone number.";
			} else if (resultCode == stiRESULT_SVRS_CALLS_NOT_ALLOWED) {
				localizedDesc = @"This account is not allowed to place VRS calls.";
			}
			NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
			*errorOut = [NSError errorWithDomain:SCIErrorDomainGeneral code:resultCode userInfo:userInfo];
		}
		return nil;
	}
	else {
		[mCallInProgressCondition lock];
		mCallInProgress = YES;
		[mCallInProgressCondition signal];
		[mCallInProgressCondition unlock];
		
		if (call)
		{
			SCICall *output = [self callForIstiCall:call];
			return output;
		}
		else if (errorOut)
		{
			// The swift/objc interface requires that if we return nil, we set the errorOut to an error. Otherwise, we get a
			// Foundation._GenericObjCError.nilError instead, which is ugly to catch (the name is not ABI stable). Better to throw a generic error
			// and have anything that uses this handle it gracefully instead.
			*errorOut = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNoError userInfo:@{ NSLocalizedDescriptionKey: @"A call was not created" }];
		}
		return nil;
	}
}

- (void)waitForHangUp
{
	[mCallInProgressCondition lock];
	if (!mCallInProgress) {
		[mCallInProgressCondition unlock];
		return;
	}
	[mCallInProgressCondition wait];
	[mCallInProgressCondition unlock];
}

- (BOOL)removeCall:(SCICall *)call
{
	if (call.istiCall)
	{
		stiHResult res = mVideophoneEngine->CallObjectRemove(call.istiCall);
		return res == stiRESULT_SUCCESS;
	}
	return YES;
}

#pragma mark - Public API: Calls via VP

- (void)answerOnVp:(NSString*)ipAddress
{
#if APPLICATION == APP_NTOUCH_IOS
	m_oRemoteClient->Connect([ipAddress UTF8String]);
	m_oRemoteClient->Answer();
#endif
}

- (void)callWithVP:(NSString*)ipAddress dialString:(NSString*)dialString
{
#if APPLICATION == APP_NTOUCH_IOS
	if (!ipAddress)
	{
		ipAddress = [[NSUserDefaults standardUserDefaults] stringForKey:@"AnswerOnVPKey"];
	}
	if (ipAddress)
	{
		m_oRemoteClient->Connect([ipAddress UTF8String]);
		m_oRemoteClient->Dial([dialString UTF8String]);
	}
#endif
}

#pragma mark - C++ API: Call List

- (void)clearCallListWithType:(CstiCallList::EType)callListType completionHandler:(void (^)(NSError *error))handler
{
	handler = [handler copy];
	
	CstiCoreRequest *request = new CstiCoreRequest();
	CstiCallList *callList = new CstiCallList();
	callList->TypeSet(callListType);
	request->callListSet(callList);
	
	[self sendBasicCoreRequest:request completion:handler];
}

#pragma mark - Helpers: User Defaults

- (void)loadUserDefaultsIfAccountIsNew
{
	return [self loadUserDefaultsIfAccountIsNewWithCompletionHandler:nil];
}

- (void)loadUserDefaultsIfAccountIsNewWithCompletionHandler:(void (^)(BOOL loaded, NSError *error))block
{
	SCILog(@"Start");
	[self fetchAccountIsNewWithCompletionHandler:^(NSNumber *newAccountNumber, NSError *error) {
		BOOL loadedOut = NO;
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		
		if (newAccountNumber) {
			if (newAccountNumber.intValue == 1) {
				loadedOut = NO;
				errorOut = nil;
				shouldPerformBlock = NO;
				//This is our first time running the app.  Download the default settings from Core
				//and save them into the property manager.
				[self loadUserDefaultsWithCompletionHandler:^(NSError *error) {
					BOOL loadedOut = NO;
					NSError *errorOut = nil;
					BOOL shouldPerformBlock = NO;
					
					if (!error) {
						//After we've _successfully_ loaded the defaults, store that the account is
						//no longer new to Core.
						[self setUserSetting:[SCISettingItem settingItemWithUTF8StringName:PropertyDef::NewAccount intValue:0]
								  completion:^(NSError *error) {
									  BOOL loadedOut = NO;
									  NSError *errorOut = nil;
									  
									  if (!error) {
										  loadedOut = YES;
										  errorOut = nil;
									  } else {
										  loadedOut = NO;
										  errorOut = error;
									  }
									  
									  if (block)
										  block(loadedOut, errorOut);
								  }];
					} else {
						loadedOut = NO;
						errorOut = error;
						shouldPerformBlock = YES;
					}
					
					if (shouldPerformBlock && block)
						block(loadedOut, errorOut);
				}];
			} else {
				loadedOut = NO;
				errorOut = nil;
				shouldPerformBlock = YES;
			}
		} else {
			loadedOut = NO;
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (shouldPerformBlock && block)
			block(loadedOut, errorOut);
	}];
}

- (void)fetchAccountIsNewWithCompletionHandler:(void (^)(NSNumber *newAccountNumber, NSError *error))block
{
	SCILog(@"Start");
	[self fetchUserSetting:PropertyDef::NewAccount completion:^(SCISettingItem *setting, NSError *error) {
		NSNumber *newAccountNumberOut = nil;
		NSError *errorOut = nil;
		
		if (setting) {
			newAccountNumberOut = setting.value;
			errorOut = nil;
		} else {
			newAccountNumberOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(newAccountNumberOut, errorOut);
	}];
}

- (void)loadUserDefaultsWithCompletionHandler:(void (^)(NSError *error))block
{
	[self fetchUserDefaultsWithCompletionHandler:^(NSArray *defaults, NSError *error) {
		NSError *errorOut = nil;
		
		if (defaults) {
			for (SCISettingItem *item in defaults) {
				[[SCIPropertyManager sharedManager] setValueFromSettingItem:item inStorageLocation:SCIPropertyStorageLocationPersistent];
			}
		} else {
			errorOut = error;
		}
		
		if (block)
			block(errorOut);
	}];
}

- (void)fetchUserDefaultsWithCompletionHandler:(void (^)(NSArray *defaults, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->getUserDefaults();
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSArray *defaultsOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto settingList = response->SettingListGet();
			if (!settingList.empty()) {
				defaultsOut = [NSArray arrayWithCstiSettingList:settingList];
				errorOut = nil;
			} else {
				defaultsOut = nil;
				errorOut = ^ {
					NSString *localizedDesc = @"Received non-error response for the DefaultUserSettingsGet request, but the response did not contain a list of user settings.";
					NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
					return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNullDefaultUserSettingsList userInfo:userInfo];
				}();
			}
		} else {
			errorOut = error;
		}
		
		if (block)
			block(defaultsOut, errorOut);
	}];
}

#pragma mark - Public API: User Account Info

- (void)loadUserAccountInfo
{
	[self loadUserAccountInfoWithCompletionHandler:nil];
}

- (void)loadUserAccountInfoWithCompletionHandler:(void (^)(NSError *error))block
{
	[self fetchUserAccountInfoWithCompletionHandler:^(CstiUserAccountInfo *userAccountInfo, NSError *error) {
		NSError *errorOut = nil;
		
		if (userAccountInfo) {
			BOOL configuredSuccessfully = [self configureWithUserAccountInfo:userAccountInfo];
			if (configuredSuccessfully) {
				errorOut = nil;
			} else {
				errorOut = ^ {
					NSString *localizedDesc = @"Loading user account info failed.";
					NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
					return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeFailedToLoadUserAccountInfo userInfo:userInfo];
				}();
			}
		} else {
			errorOut = error;
		}
		
		if (block)
			block(errorOut);
	}];
}

- (void)fetchUserAccountInfoWithCompletionHandler:(void (^)(CstiUserAccountInfo *, NSError *))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->userAccountInfoGet();
	coreRequest->callbackAdd<vpe::ServiceResponse<CstiUserAccountInfo>>([self, block] (ServiceResponseSP<CstiUserAccountInfo> successResponse) {
		dispatch_async(dispatch_get_main_queue(), ^() {
			if (block) {
				block(&successResponse->content, nil);
			}
		});
	}, [self, block] (ServiceResponseSP<CstiUserAccountInfo> errorResponse) {
		dispatch_async(dispatch_get_main_queue(), ^() {
			NSError *error = ^ {
				NSString *localizedDesc = @"Fetched NULL user account info.";
				NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
				return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNullUserAccountInfo userInfo:userInfo];
			}();
			if (block) {
				block(nil, error);
			}
		});
	});
	mVideophoneEngine->CoreRequestSend(coreRequest);
}

- (void)setSignMailEmailPreferencesEnabled:(BOOL)enabled withPrimaryEmail:(NSString *)primaryAddress secondaryEmail:(NSString *)secondaryAddress completionHandler:(void (^)(NSError *error))block
{
	mVideophoneEngine->userAccountManagerGet()->signMailEmailPreferencesSet(enabled, std::string(primaryAddress.UTF8String, [primaryAddress lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), std::string(secondaryAddress.UTF8String, [secondaryAddress lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), [self, block] (std::shared_ptr<CstiCoreResponse> response) {
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		NSError *error = NSErrorFromCstiCoreResponse(response.get());
		if (error) {
			errorOut = error;
			shouldPerformBlock = YES;
		} else {
			[self loadUserAccountInfoWithCompletionHandler:^(NSError *error) {
				if (block) {
					block(error);
				}
			}];
		}
		
		if (shouldPerformBlock && block) {
			block(errorOut);
		}
	});
}

#pragma mark - Helpers: User Account Info

- (BOOL)configureWithUserAccountInfo:(CstiUserAccountInfo *)userAccountInfo
{
	if(!userAccountInfo) {
		SCILog(@"UserAccountInfo is NULL");
		return NO;
	}
	
	if (userAccountInfo->m_LocalNumber.empty() &&
		userAccountInfo->m_PhoneNumber.empty()) {
		// indicates user account has been ported
		NSLog(@"Number has been ported; logging out."); // do this because MstiVideophoneEngine does it.
		[self postOnMainThreadNotificationName:SCINotificationUserLoggedIntoAnotherDevice userInfo:nil];  //This notification will call logout.
		return NO;
	}
	
	self.userAccountInfo = [SCIUserAccountInfo userAccountInfoWithCstiUserAccountInfo:userAccountInfo];
	
	if (!userAccountInfo->m_RingGroupLocalNumber.empty())
	{
		mVideophoneEngine->LocalNameSet(std::string(userAccountInfo->m_RingGroupName));
	}
	else
	{
		mVideophoneEngine->LocalNameSet(std::string(userAccountInfo->m_Name));
	}
	
	//
	// Set the Contact list quota
	//
	{
		int count = std::stoi(userAccountInfo->m_CLContactQuota);
		mVideophoneEngine->ContactManagerGet()->setMaxCount(count);
	}
	
	//
	// Set the blocked list quota
	//
	if (mVideophoneEngine->BlockListManagerGet()->EnabledGet())
	{
		if (!userAccountInfo->m_CLBlockedQuota.empty())
		{
			int count = std::stoi(userAccountInfo->m_CLBlockedQuota);
			mVideophoneEngine->BlockListManagerGet()->MaximumLengthSet(count);
		}
	}
	
	//
	// Configure toll-free preference based on availability
	//
	{
		SCIUserAccountInfo *userAccountInfo = self.userAccountInfo;
		BOOL isRingGroup = ((userAccountInfo.ringGroupLocalNumber.length > 0) ||
							(userAccountInfo.ringGroupTollFreeNumber.length > 0));
		
		BOOL hasEndpointTollFree = (userAccountInfo.tollFreeNumber.length > 0);
		BOOL hasGroupTollFree = (userAccountInfo.ringGroupTollFreeNumber.length > 0);
		
		
		int iUseTollFree = 0;
		WillowPM::PropertyManager::getInstance ()->getPropertyInt (PropertyDef::CallerIdNumber, &iUseTollFree, 0, WillowPM::PropertyManager::Persistent);
		
		// We may have just joined or been removed from a ring group.  (If leaving a ring group, the ring group had a toll-free number but
		// the endpoint leaving the ring group did not.  If joining a ring-group, the endpoint had a toll-free number but the ring group
		// did not.)  Alternatively, the toll-free number may have been removed from the account in VDM.  In any of these cases, we will be
		// in a situation where our CallerIdNumber is toll-free even though there is no toll-free number associated with the account.  So,
		// if we presently have 1 (toll-free) as the CallerIdNumber, set the CallerIdNumber to be 0 (local).
		if ((iUseTollFree == 1) && ((isRingGroup && !hasGroupTollFree) || (!isRingGroup && !hasEndpointTollFree))) {
			WillowPM::PropertyManager::getInstance()->setPropertyInt(PropertyDef::CallerIdNumber, 0, WillowPM::PropertyManager::Persistent);
			WillowPM::PropertyManager::getInstance()->PropertySend(PropertyDef::CallerIdNumber, estiPTypeUser);
		}
	}
	
	//
	// Set phone number with videophone engine for use with gatekeeper
	//
	self.userPhoneNumbers = ^{
		SstiUserPhoneNumbers sUserPhoneNumbers;
		
		memset(&sUserPhoneNumbers, 0, sizeof(SstiUserPhoneNumbers));
		if (!userAccountInfo->m_PhoneNumber.empty()) {
			sprintf(sUserPhoneNumbers.szSorensonPhoneNumber, "%s", userAccountInfo->m_PhoneNumber.c_str());
		}
		if (!userAccountInfo->m_TollFreeNumber.empty()) {
			sprintf(sUserPhoneNumbers.szTollFreePhoneNumber, "%s", userAccountInfo->m_TollFreeNumber.c_str());
		}
		if (!userAccountInfo->m_LocalNumber.empty()) {
			sprintf(sUserPhoneNumbers.szLocalPhoneNumber, "%s", userAccountInfo->m_LocalNumber.c_str());
		}
//		if (0 < strlen(userAccountInfo->m_HearingNumber)) {
//			sprintf(sUserPhoneNumbers.szHearingPhoneNumber, "%s", (const char *)userAccountInfo->m_HearingNumber);
//		}
		if (!userAccountInfo->m_RingGroupTollFreeNumber.empty()) {
			sprintf(sUserPhoneNumbers.szRingGroupTollFreePhoneNumber, "%s", userAccountInfo->m_RingGroupTollFreeNumber.c_str());
		}
		if (!userAccountInfo->m_RingGroupLocalNumber.empty()) {
			sprintf(sUserPhoneNumbers.szRingGroupLocalPhoneNumber, "%s", userAccountInfo->m_RingGroupLocalNumber.c_str());
		}
		
		SCIUserPhoneNumbers *userPhoneNumbers = [SCIUserPhoneNumbers userPhoneNumbersWithSstiUserPhoneNumbers:sUserPhoneNumbers];
		
		int iUseTollFree = 0;
		WillowPM::PropertyManager::getInstance ()->getPropertyInt (PropertyDef::CallerIdNumber, &iUseTollFree, 0, WillowPM::PropertyManager::Persistent);
		
		// preferred phone number is used for gatekeeper
		userPhoneNumbers.preferredPhoneNumberType = ^{
			SCIUserPhoneNumberType type = SCIUserPhoneNumberTypeUnknown;
			// ring-group-toll-free >  ring-group-local > single-toll-free > single-local > Sorenson
			
			if (iUseTollFree &&
				(userPhoneNumbers.ringGroupTollFreePhoneNumber.length > 0))
			{
				type = SCIUserPhoneNumberTypeRingGroupTollFree;
			}
			else
			if (userPhoneNumbers.ringGroupLocalPhoneNumber.length > 0) {
				type = SCIUserPhoneNumberTypeRingGroupLocal;
			}
			else
			if (iUseTollFree &&
				(userPhoneNumbers.tollFreePhoneNumber.length > 0))
			{
				type = SCIUserPhoneNumberTypeTollFree;
			}
			else
			if (userPhoneNumbers.localPhoneNumber.length > 0)
			{
				type = SCIUserPhoneNumberTypeLocal;
			}
			else
			if (userPhoneNumbers.sorensonPhoneNumber.length > 0)
			{
				type = SCIUserPhoneNumberTypeSorenson;
			}
			else
			{
				type = SCIUserPhoneNumberTypeUnknown;
			}
			
			NSAssert1([[userPhoneNumbers phoneNumberOfType:type] length] > 0, @"Preferred phone number type was determined to be %@ but the user does not have a phone number of that type.", NSStringFromSCIUserPhoneNumberType(type));
			
			return type;
		}();
		
		return userPhoneNumbers;
	}();

	[self postOnMainThreadNotificationName:SCINotificationUserAccountInfoUpdated userInfo:nil];
	if (self.authorized &&
		self.userAccountInfo.mustCallCIRBool) {
		[self postOnMainThreadNotificationName:SCINotificationRequiredCallCIR userInfo:nil];
	}
	if (self.authorized &&
		self.userAccountInfo.userRegistrationDataRequiredBool) {
		[self postOnMainThreadNotificationName:SCINotificationUserRegistrationDataRequired userInfo:nil];
	}
	return YES;
}

- (void)handleUserAccountInfoGetResultWithResponse:(const ServiceResponseSP<CstiUserAccountInfo> &)response
{
	CstiUserAccountInfo *userAccountInfo = &response->content;
	
	NSError *error = nil;
	
	if (userAccountInfo) {
		BOOL configuredSuccessfully = [self configureWithUserAccountInfo:userAccountInfo];
		if (configuredSuccessfully) {
			error = nil;
		} else {
			error = ^ {
				NSString *localizedDesc = @"Loading user account info failed.";
				NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
				return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeFailedToLoadUserAccountInfo userInfo:userInfo];
			}();
		}
		if (self.loginBlock && !self.processingLogin) {
			if (!error) {
				self.processingLogin = YES;
				[self authorizeAccountWithCompletion:^(NSDictionary *statusDictionary, NSDictionary *errorDictionary) {
					self.authorizing = YES;
					if (self.loginBlock) {
						self.loginBlock(statusDictionary, errorDictionary,
										^{
											self.authorized = YES;
											[[SCICallListManager sharedManager] refresh];
											[self loadUserDefaultsIfAccountIsNew];
											[self updateNetworkSettings];
											[self postOnMainThreadNotificationName:SCINotificationLoggedIn userInfo:nil];
											if (self.userAccountInfo.mustCallCIRBool) {
												[self postOnMainThreadNotificationName:SCINotificationRequiredCallCIR userInfo:nil];
											}
											if (self.userAccountInfo.userRegistrationDataRequiredBool) {
												[self postOnMainThreadNotificationName:SCINotificationUserRegistrationDataRequired userInfo:nil];
											}
											if (self.deviceLocationType == SCIDeviceLocationTypeUpdate) {
												[self postOnMainThreadNotificationName:SCINotificationDeviceLocationTypeChanged userInfo:nil];
											}
										},
										^{
											[self logout];
										});
						self.loginBlock = nil;
						self.processingLogin = NO;
					}
				}];
			} else {
				SCILog(@"Failed to load valid user account info.  Aborting login.");
				if (self.loginBlock && !self.processingLogin){
					self.processingLogin = YES;
					self.loginBlock(nil, @{ SCIErrorKeyLogin : error }, nil, nil);
					self.loginBlock = nil;
					self.processingLogin = NO;
				}
			}
		}
	}
}

- (void)handleClientAuthenticateResultWithResponse:(const ServiceResponseSP<ClientAuthenticateResult>&)result
{
	auto phoneUserId = result->content.userId;
	auto ringGroupUserId = result->content.groupUserId;
	self.phoneUserId = (!phoneUserId.empty()) ? [NSString stringWithUTF8String:phoneUserId.c_str()] : nil;
	self.ringGroupUserId = (!ringGroupUserId.empty()) ? [NSString stringWithUTF8String:ringGroupUserId.c_str()] : nil;
	
	NSError *error = NSErrorFromServiceResponse(result);
	if (result->coreErrorNumber == CstiCoreResponse::eCANNOT_DETERMINE_WHICH_LOGIN) {
		std::vector<CstiCoreResponse::SRingGroupInfo> sRingGroupInfos = result->content.ringGroupList;
		
		NSMutableArray *mutableRingGroupInfos = [NSMutableArray array];
		
		std::vector<CstiCoreResponse::SRingGroupInfo>::const_iterator i;
		std::vector<CstiCoreResponse::SRingGroupInfo>::const_iterator end = sRingGroupInfos.end();
		for (i = sRingGroupInfos.begin (); i != end; i++) {
			CstiCoreResponse::SRingGroupInfo sRingGroupInfo = *i;
			SCIRingGroupInfo *info = [SCIRingGroupInfo ringGroupInfoWithSRingGroupInfo:&sRingGroupInfo];
			[mutableRingGroupInfos addObject:info];
		}
		
		NSArray *ringGroupInfos = [mutableRingGroupInfos copy];
		if (self.loginBlock && !self.processingLogin) {
			self.processingLogin = YES;
			self.loginBlock (@{ SCINotificationKeyRingGroupInfos : ringGroupInfos },
							 nil,
							 //This action doesn't make sense--there is no way to proceed.  We already know at
							 //this point without waiting for user feedback that the login attempt failed.  Once
							 //the user selects the group member which they want to log in with, loginWithBlock:
							 //will be called again.
							 nil, // proceedBlock
							 //There is no need to terminate the login process because the login process already
							 //failed.
							 nil // terminateBlock
							 );
			self.loginBlock = nil;
			self.processingLogin = NO;
		}
	} else if (result->coreErrorNumber == CstiCoreResponse::eUSERNAME_OR_PASSWORD_INVALID) {
		if (self.loginBlock && !self.processingLogin) {
			self.processingLogin = YES;
			self.loginBlock(nil, @{ SCIErrorKeyLogin : error }, nil, nil);
			self.loginBlock = nil;
			self.processingLogin = NO;
		} else {
			[self logoutWithBlock:^{
				self.userAccountInfo = nil;
			}];
		}
	}
	else if (error) {
		if (self.loginBlock && !self.processingLogin) {
			self.processingLogin = YES;
			self.loginBlock(nil, @{ SCIErrorKeyLogin : error }, nil, nil);
			self.loginBlock = nil;
			self.processingLogin = NO;
		}
	}
}

#pragma mark - Public API: Credentials

- (void)changePassword:(NSString *)password completionHandler:(void (^)(NSError *error))block
{
	mVideophoneEngine->userAccountManagerGet()->userPinSet(std::string(password.UTF8String, [password lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), [self, password, block] (std::shared_ptr<CstiCoreResponse> response) {
		NSError *errorOut = nil;
		NSError *error = NSErrorFromCstiCoreResponse(response.get());
		if (error) {
			errorOut = error;
		} else {
			[self setPassword:password];
		}
		
		if (block) {
			dispatch_async(dispatch_get_main_queue(), ^(void) {
				block(errorOut);
			});
		}
	});
}

- (void)saveUserSSN:(NSString *)SSN DOB:(NSString *)DOB andHasSSN:(BOOL)hasSSN completionHandler:(void (^)(NSError *error))block
{
	mVideophoneEngine->userAccountManagerGet()->userRegistrationInfoSet(std::string(DOB.UTF8String, [DOB lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), std::string(SSN.UTF8String, [SSN lengthOfBytesUsingEncoding:NSUTF8StringEncoding]), hasSSN, [self, block] (std::shared_ptr<CstiCoreResponse> response) {
		NSError *errorOut = nil;
		NSError *error = NSErrorFromCstiCoreResponse(response.get());
		if (error) {
			errorOut = error;
		}
		
		if (block) {
			block(errorOut);
		}
	});
}

#pragma mark - Public API: E911 Address/Provisioning

- (void)loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:(void (^)(NSError *))block
{
	[self fetchEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:^(SCIEmergencyAddress *emergencyAddress, NSError *error) {
		NSError *errorOut = nil;
		BOOL shouldPostNotification = NO;
		
		if (!error) {
			self.emergencyAddress = emergencyAddress;
			errorOut = nil;
			shouldPostNotification = YES;
		} else {
			errorOut = error;
			shouldPostNotification = NO;
		}
		
		if (block)
			block(errorOut);
		
		if (shouldPostNotification)
			[self postOnMainThreadNotificationName:SCINotificationEmergencyAddressUpdated userInfo:nil];
	}];
}

- (void)fetchEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:(void (^)(SCIEmergencyAddress *address, NSError *error))block
{
	[self fetchEmergencyAddressWithCompletionHandler:^(SCIEmergencyAddress *addressFetched, NSError *error) {
		NSInteger code = error.code;
		SCIEmergencyAddress *addressOut = nil;
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		
		if (!error
			|| code == SCICoreResponseErrorCodeNoCurrentEmergencyAddressFound
			|| code == SCICoreResponseErrorCodeEmergencyAddressDataBlank) {
			//TODO: This is more intelligible than what it is replacing: we had been ignoring
			//      error responses from the emergency address fetch and proceeding with the
			//      status fetch regardless rather than, as now, only allowing specific errors.
			//		However, it hasn't been verified that these are the only errors that should
			//		be allowed.
			if (error.code == SCICoreResponseErrorCodeNoCurrentEmergencyAddressFound)
				addressFetched = [[SCIEmergencyAddress alloc] init];
			[self fetchEmergencyAddressProvisioningStatusWithCompletionHandler:^(SCIEmergencyAddressProvisioningStatus statusFetched, NSError *error) {
				SCIEmergencyAddress *addressOut = nil;
				NSError *errorOut = nil;
				
				if (!error) {
					self.emergencyAddress = addressFetched;
					addressOut = addressFetched;
					addressOut.status = statusFetched;
					errorOut = nil;
				} else {
					addressOut = nil;
					errorOut = error;
				}
				
				if (block)
					block(addressOut, errorOut);
			}];
			shouldPerformBlock = NO;
		} else {
			addressOut = nil;
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (block && shouldPerformBlock)
			block(addressOut, errorOut);
	}];
}

- (void)fetchEmergencyAddressWithCompletionHandler:(void (^)(SCIEmergencyAddress *, NSError *))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressGet();
	coreRequest->callbackAdd<vpe::ServiceResponse<vpe::Address>>([self, block] (ServiceResponseSP<vpe::Address> successResponse) {
		SCIEmergencyAddress *emergencyAddress = [[SCIEmergencyAddress alloc] init];
		emergencyAddress.address1 = [NSString stringWithUTF8String:successResponse->content.street.c_str()];
		emergencyAddress.address2 = [NSString stringWithUTF8String:successResponse->content.apartmentNumber.c_str()];
		emergencyAddress.city = [NSString stringWithUTF8String:successResponse->content.city.c_str()];
		emergencyAddress.state = [NSString stringWithUTF8String:successResponse->content.state.c_str()];
		emergencyAddress.zipCode = [NSString stringWithUTF8String:successResponse->content.zipCode.c_str()];
		
		if (block) {
			block(emergencyAddress, nil);
		}
		
	}, [self, block] (ServiceResponseSP<vpe::Address> errorResponse) {
		NSError *error = NSErrorFromServiceResponse(errorResponse);
		if (block) {
			block(nil, error);
		}
	});
	mVideophoneEngine->CoreRequestSend(coreRequest);
}

- (void)fetchEmergencyAddressProvisioningStatusWithCompletionHandler:(void (^)(SCIEmergencyAddressProvisioningStatus status, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressProvisionStatusGet();
	coreRequest->callbackAdd<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>>([self, block] (ServiceResponseSP<EstiEmergencyAddressProvisionStatus> successResponse) {
		if (block) {
			dispatch_async(dispatch_get_main_queue(), ^(void) {
				block((SCIEmergencyAddressProvisioningStatus)successResponse->content, nil);
			});
		}
	}, [self, block] (ServiceResponseSP<EstiEmergencyAddressProvisionStatus> errorResponse) {
		NSError *error = NSErrorFromServiceResponse(errorResponse);
		if (block) {
			block(SCIEmergencyAddressProvisioningStatusNotSubmitted, error);
		}
	});
	mVideophoneEngine->CoreRequestSend(coreRequest);
}

- (void)provisionEmergencyAddress:(SCIEmergencyAddress *)address reloadEmergencyAddressAndEmergencyAddressProvisioningStatus:(BOOL)reload completionHandler:(void (^)(NSError *error))block
{
	const char *address1 = address.address1.UTF8String;
	const char *address2 = address.address2.UTF8String;
	const char *city = address.city.UTF8String;
	const char *state = address.state.UTF8String;
	const char *zip = address.zipCode.UTF8String;
	
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressProvision(address1, address2, city, state, zip);

	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		
		SCICoreMajorVersionNumber majorVersion = self.coreVersion.majorVersionNumber;
		
		if (majorVersion <= SCICoreMajorVersionNumber3 && [error.localizedDescription isEqualToString:@"Provision request failed"]) {
			static NSSet *nonStates = [NSSet setWithObjects:@"AS", @"FM", @"GU", @"MH", @"MP", @"PW", @"PR", @"VI", nil];
			
			if([nonStates containsObject:address.state])
			{
				// Don't report an error if the provisioning failed because of a non US State(ie territory or protectorate).
				error = nil;
				response = (CstiCoreResponse *)1;
			}
		}

		if (response) {
			if (reload) {
				[self loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:block];
				
				shouldPerformBlock = NO;
			} else {
				errorOut = nil;
				shouldPerformBlock = YES;
			}
		} else {
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (block && shouldPerformBlock)
			block(errorOut);
	}];
}

- (void)provisionAndReloadEmergencyAddressAndEmergencyAddressProvisioningStatus:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block
{
	[self provisionEmergencyAddress:address reloadEmergencyAddressAndEmergencyAddressProvisioningStatus:YES completionHandler:block];
}


- (void)provisionEmergencyAddress:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block
{
	[self provisionEmergencyAddress:address reloadEmergencyAddressAndEmergencyAddressProvisioningStatus:NO completionHandler:block];
}

- (void)deprovisionEmergencyAddressAndProvisionEmergencyAddress:(SCIEmergencyAddress *)address completionHandler:(void (^)(NSError *error))block
{
	[self deprovisionEmergencyAddressWithCompletionHandler:^(NSError *error) {
		if (!error) {
			[self provisionAndReloadEmergencyAddressAndEmergencyAddressProvisioningStatus:address completionHandler:block];
		} else {
			if (block)
				block(error);
		}
	}];
}

- (void)deprovisionEmergencyAddressWithCompletionHandler:(void (^)(NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressDeprovision();
	
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (void)acceptEmergencyAddressUpdate
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressUpdateAccept();
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		if (response) {
			[self loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:nil];
		}
	}];
}

- (void)rejectEmergencyAddressUpdate
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->emergencyAddressUpdateReject();
	
	[self sendBasicCoreRequest:coreRequest completion:nil];
}

#pragma mark - Public API: Provider Agreements

- (void)fetchProviderAgreementTypeWithCompletion:(void (^)(NSNumber *providerAgreementTypeNumber, NSError *error))block
{
	[self fetchDynamicAgreementsFeatureEnabledWithCompletion:^(NSNumber *dynamicAgreementsFeatureEnabledNumber, NSError *error) {
		NSNumber *providerAgreementTypeNumberOut = nil;
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		
		if (dynamicAgreementsFeatureEnabledNumber) {
			int dynamicProviderAgreementsFeatureEnabledInt = dynamicAgreementsFeatureEnabledNumber.intValue;
			BOOL dynamicProviderAgreementsFeatureEnabled = (dynamicProviderAgreementsFeatureEnabledInt) ? YES : NO;

			if (self.coreVersion == nil) {
				SCILog(@"CoreVersion is null");
				[self fetchCoreVersionWithCompletion:^(SCICoreVersion *coreVersion, NSError *error) {
					NSNumber *providerAgreementTypeNumberOut = nil;
					NSError *errorOut = nil;
					
					if (coreVersion) {
						SCIProviderAgreementType providerAgreementType = SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(coreVersion, dynamicProviderAgreementsFeatureEnabled);
						providerAgreementTypeNumberOut = @(providerAgreementType);
						errorOut = nil;
					} else {
						providerAgreementTypeNumberOut = nil;
						errorOut = error;
					}
					
					if (block)
						block(providerAgreementTypeNumberOut, errorOut);
				}];
				providerAgreementTypeNumberOut = nil;
				errorOut = nil;
				shouldPerformBlock = NO;
			}
			else
			{
				SCICoreVersion *coreVersion = self.coreVersion;
				if (coreVersion) {
					SCIProviderAgreementType providerAgreementType = SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(coreVersion, dynamicProviderAgreementsFeatureEnabled);
					providerAgreementTypeNumberOut = @(providerAgreementType);
					errorOut = nil;
					shouldPerformBlock = YES;
				} else {
					providerAgreementTypeNumberOut = nil;
					errorOut = error;
					shouldPerformBlock = YES;
				}
			}
		} else {
			providerAgreementTypeNumberOut = nil;
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (block && shouldPerformBlock)
			block(providerAgreementTypeNumberOut, errorOut);
	}];
}

- (void)fetchDynamicAgreementsFeatureEnabledWithCompletion:(void (^)(NSNumber *dynamicAgreementsFeatureEnabledNumber, NSError *error))block
{
	[self fetchUserSetting:AGREEMENTS_ENABLED
			defaultSetting:[SCISettingItem settingItemWithName:@(AGREEMENTS_ENABLED) intValue:stiAGREEMENTS_ENABLED_DEFAULT]
				completion:^(SCISettingItem *setting, NSError *error) {
		NSNumber *dynamicAgreementsFeatureEnabledNumberOut = nil;
		NSError *errorOut = nil;
		
		if (setting) {
			dynamicAgreementsFeatureEnabledNumberOut = setting.value;
			errorOut = nil;
		} else {
			dynamicAgreementsFeatureEnabledNumberOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(dynamicAgreementsFeatureEnabledNumberOut, errorOut);
	}];
}

- (void)fetchStaticProviderAgreementStatusWithCompletion:(void (^)(SCIStaticProviderAgreementStatus *status, NSError *error))block
{
	[self fetchUserSetting:PropertyDef::ProviderAgreementStatus
			defaultSetting:[SCISettingItem settingItemWithName:@(PropertyDef::ProviderAgreementStatus) stringValue:nil]
				completion:^(SCISettingItem *setting, NSError *error) {
		SCIStaticProviderAgreementStatus *statusOut = nil;
		NSError *errorOut = nil;
		
		if (setting) {
			statusOut = [SCIStaticProviderAgreementStatus statusFromSettingItem:setting];
			errorOut = nil;
		} else {
			statusOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(statusOut, errorOut);
	}];
}

- (void)fetchDynamicProviderAgreementContentComponentsWithPublicID:(NSString *)agreementPublicID completionHandler:(void (^)(NSString *agreementPublicID, NSString *agreementText, NSString *agreementType, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->agreementGet(agreementPublicID.UTF8String);
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *sendError) {
		NSString *agreementPublicIDOut = nil;
		NSString *agreementTextOut = nil;
		NSString *agreementTypeOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto pszPublicID = response->ResponseStringGet(0);
			auto pszAgreementText = response->ResponseStringGet(1);
			auto pszAgreementType = response->ResponseStringGet(2);
			
			agreementPublicIDOut = (!pszPublicID.empty()) ? [NSString stringWithUTF8String:pszPublicID.c_str()] : nil;
			agreementTextOut = (!pszAgreementText.empty()) ? [NSString stringWithUTF8String:pszAgreementText.c_str()] : nil;
			agreementTypeOut = (!pszAgreementType.empty()) ? [NSString stringWithUTF8String:pszAgreementType.c_str()] : nil;
			errorOut = nil;
		} else {
			agreementPublicIDOut = nil;
			agreementTextOut = nil;
			agreementTypeOut = nil;
			errorOut = sendError;
		}
		
		if (block)
			block(agreementPublicIDOut, agreementTextOut, agreementTypeOut, errorOut);
	}];
}

- (void)fetchDynamicProviderAgreementContentWithPublicID:(NSString *)agreementPublicID completionHandler:(void (^)(SCIDynamicProviderAgreementContent *agreementContent, NSError *error))block
{
	[self fetchDynamicProviderAgreementContentComponentsWithPublicID:agreementPublicID completionHandler:^(NSString *agreementPublicID, NSString *agreementText, NSString *agreementType, NSError *error) {
		SCIDynamicProviderAgreementContent *agreementContentOut = nil;
		NSError *errorOut = nil;
		
		if (!error) {
			agreementContentOut = [SCIDynamicProviderAgreementContent agreementContentWithPublicID:agreementPublicID text:agreementText type:agreementType];
		} else {
			errorOut = error;
		}
		
		if (block)
			block(agreementContentOut, errorOut);
	}];
}

- (void)fetchDynamicProviderAgreementStatusesWithCompletionHandler:(void (^)(NSArray *agreementStatuses, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->agreementStatusGet();
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *sendError) {
		NSArray *agreementStatusesOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			std::vector<CstiCoreResponse::SAgreement> agreementList;
			response->AgreementListGet(&agreementList);

			NSMutableArray *mutableAgreementStatuses = [NSMutableArray array];
			
			std::vector<CstiCoreResponse::SAgreement>::const_iterator i;
			std::vector<CstiCoreResponse::SAgreement>::const_iterator end = agreementList.end();
			for (i = agreementList.begin (); i != end; i++) {
				CstiCoreResponse::SAgreement sAgreement = *i;
				SCIDynamicProviderAgreementStatus *status = [SCIDynamicProviderAgreementStatus statusWithSAgreement:sAgreement];
				[mutableAgreementStatuses addObject:status];
			}
			
			agreementStatusesOut = [mutableAgreementStatuses copy];
			errorOut = nil;
		} else {
			agreementStatusesOut = nil;
			errorOut = sendError;
		}
		
		if (block)
			block(agreementStatusesOut, errorOut);
	}];
}

- (void)setState:(SCIDynamicProviderAgreementState)agreementState forDynamicProviderAgreementWithPublicID:(NSString *)agreementPublicID timeout:(NSTimeInterval)timeout completionHandler:(void (^)(BOOL success, NSError *error))block
{
	BOOL accepted = (agreementState != SCIDynamicProviderAgreementStateRejected);
	[self setAllowIncomingCallsMode:accepted];
	
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->agreementStatusSet(agreementPublicID.UTF8String, SCICoreRequestAgreementStatusFromAgreementState(agreementState));
	
	[self sendCoreRequest:coreRequest timeout:timeout completion:^(CstiCoreResponse *response, NSError *sendError) {
		BOOL successOut = NO;
		NSError *errorOut = nil;
		
		if (response) {
			successOut = YES;
			errorOut = nil;
		} else {
			successOut = NO;
			errorOut = sendError;
		}
		
		if (block)
			block(successOut, errorOut);
	}];
}
- (void)setState:(SCIDynamicProviderAgreementState)agreementState forDynamicProviderAgreementWithPublicID:(NSString *)agreementPublicID completionHandler:(void (^)(BOOL success, NSError *error))block
{
	[self setState:agreementState forDynamicProviderAgreementWithPublicID:agreementPublicID timeout:SCIVideophoneEngineDefaultTimeout completionHandler:block];
}

- (void)setDynamicProviderAgreementStatus:(SCIDynamicProviderAgreementStatus *)agreementStatus timeout:(NSTimeInterval)timeout completionHandler:(void (^)(BOOL, NSError *))block
{
	[self setState:agreementStatus.state forDynamicProviderAgreementWithPublicID:agreementStatus.agreementPublicID timeout:timeout completionHandler:block];
}
- (void)setDynamicProviderAgreementStatus:(SCIDynamicProviderAgreementStatus *)agreementStatus completionHandler:(void (^)(BOOL success, NSError *error))block
{
	[self setDynamicProviderAgreementStatus:agreementStatus timeout:SCIVideophoneEngineDefaultTimeout completionHandler:block];
}

- (void)fetchDynamicProviderAgreementsWithCompletionHandler:(void (^)(NSArray *agreements, NSError *error))block
{
	[self fetchDynamicProviderAgreementStatusesWithCompletionHandler:^(NSArray *agreementStatuses, NSError *error) {
		NSArray *agreementsOut = nil;
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = NO;
		
		if (agreementStatuses) {
			agreementsOut = nil;
			errorOut = nil;
			shouldPerformBlock = NO;
			
			__block NSError *errorOut = nil;
			__block NSMutableArray *mutableAgreements = [[NSMutableArray alloc] init];
			[agreementStatuses bnr_enumerateOnQueue:NULL
									   continuation:^(id obj, NSUInteger index, void (^continuation)(void)) {
										   SCIDynamicProviderAgreementStatus *agreementStatus = (SCIDynamicProviderAgreementStatus *)obj;
										   if (agreementStatus.state != SCIDynamicProviderAgreementStateAccepted) {
											   [self fetchDynamicProviderAgreementContentWithPublicID:agreementStatus.agreementPublicID completionHandler:^(SCIDynamicProviderAgreementContent *agreementContent, NSError *error) {
												   
												   SCIDynamicProviderAgreement *agreement = [SCIDynamicProviderAgreement agreementWithStatus:agreementStatus content:agreementContent];
												   
												   [mutableAgreements addObject:agreement];
												   
												   if (!agreementContent) {
													   errorOut = error;
												   }
												   
												   continuation();
											   }];
										   } else {
											   SCIDynamicProviderAgreement *agreement = [SCIDynamicProviderAgreement agreementWithStatus:agreementStatus content:nil];
											   
											   [mutableAgreements addObject:agreement];
											   
											   continuation();
										   }
									   }
			 completion:^{
				 block([mutableAgreements copy], errorOut);
			 }];
		} else {
			agreementsOut = nil;
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (block && shouldPerformBlock)
			block(agreementsOut, errorOut);
	}];
}

#pragma mark - Public API: Update

- (void)checkForUpdateFromVersion:(NSString *)version block:(void (^)(NSString *versionAvailable, NSString *updateURL, NSError *err))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	
	coreRequest->updateVersionCheck([version UTF8String], "");
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSString *versionAvailableOut = nil;
		NSString *updateURLOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto url = response->ResponseStringGet(0);
			auto version = response->ResponseStringGet(1);
			NSString *urlString = [NSString stringWithUTF8String:url.c_str()];
			NSString *availableVersion = [NSString stringWithUTF8String:version.c_str()];
			
			if (![urlString isEqualToString:@"AppVersionOK"]) {
				versionAvailableOut = availableVersion;
				updateURLOut = urlString;
				errorOut = nil;
			} else {
				versionAvailableOut = nil; // no update
				updateURLOut = nil;
				errorOut = nil;
			}
		} else {
			versionAvailableOut = nil;
			updateURLOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(versionAvailableOut, updateURLOut, errorOut);
	}];
}

- (void)setCodecCapabilities:(BOOL)mainSupported h264Level:(NSUInteger)h264Level
{
	
	SstiVideoCapabilities* h263Capabilities = new SstiVideoCapabilities();
	//Profile and Constraint Signal Zero for h263
	SstiProfileAndConstraint h263ProfileAndConstraint;
	h263ProfileAndConstraint.eProfile = estiH263_ZERO;
	h263Capabilities->Profiles.push_back(h263ProfileAndConstraint);
	
	IstiVideoOutput::InstanceGet()->CodecCapabilitiesSet(estiVIDEO_H263, h263Capabilities);
	
	SstiH264Capabilities *h264Capabilities = new SstiH264Capabilities;
	
	if (mainSupported)
	{
		SstiProfileAndConstraint profileAndConstraint;
		profileAndConstraint.eProfile = estiH264_MAIN;
		profileAndConstraint.un8Constraints = 0xe0;
		h264Capabilities->Profiles.push_back(profileAndConstraint);
	}
	
	SstiProfileAndConstraint profileAndConstraint;
	profileAndConstraint.eProfile = estiH264_BASELINE;
	profileAndConstraint.un8Constraints = 0xa0;
	h264Capabilities->Profiles.push_back(profileAndConstraint);
	
	h264Capabilities->eLevel = (EstiH264Level)h264Level;
	IstiVideoOutput::InstanceGet()->CodecCapabilitiesSet(estiVIDEO_H264, h264Capabilities);
	
	SstiH265Capabilities* stH265Capabilities = new SstiH265Capabilities();
	stH265Capabilities->eLevel = estiH265_LEVEL_4_1;
	stH265Capabilities->eTier = estiH265_TIER_MAIN;
	//Profile for h265
	SstiProfileAndConstraint stH265ProfileAndConstraint;
	stH265ProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
	stH265Capabilities->Profiles.push_back(stH265ProfileAndConstraint);
	IstiVideoOutput::InstanceGet()->CodecCapabilitiesSet(estiVIDEO_H265, stH265Capabilities);
}


#pragma mark - Public API: Phone Numbers

- (SCIUserPhoneNumbers *)userPhoneNumbers
{
	return mUserPhoneNumbers;
}

- (void)setUserPhoneNumbers:(SCIUserPhoneNumbers *)userPhoneNumbers
{
	SstiUserPhoneNumbers sstiUserPhoneNumbers = [userPhoneNumbers sstiUserPhoneNumbers];
	stiHResult res = mVideophoneEngine->UserPhoneNumbersSet(&sstiUserPhoneNumbers);
	if (res == stiRESULT_SUCCESS) {
		mUserPhoneNumbers = userPhoneNumbers;
		self.userAccountInfo.preferredNumber = mUserPhoneNumbers.preferredPhoneNumber;
		[self postOnMainThreadNotificationName:SCINotificationUserAccountInfoUpdated userInfo:nil];
	}
}

- (NSString *)phoneNumberReformat:(NSString *)phoneNumber
{
	NSString *res = nil;
	if (phoneNumber) {
		//Expect numbers.  Fails on letters.
		std::string newPhoneNumber;
		stiHResult result = self.videophoneEngine->PhoneNumberReformat([phoneNumber UTF8String], &newPhoneNumber);
		if (result == stiRESULT_SUCCESS) {
			res = [NSString stringWithUTF8String:&(newPhoneNumber[0])];
		}
	}
	return res;
}

- (BOOL)phoneNumberIsValid:(NSString *)phoneNumber
{
	if (!phoneNumber) return NO;
	bool res = self.videophoneEngine->PhoneNumberIsValid([phoneNumber UTF8String]);
	return res ? YES : NO;
}

- (BOOL)dialStringIsValid:(NSString *)dialString
{
	BOOL res = NO;
	
	if (dialString.length > 0) {
		bool bRes = self.videophoneEngine->DialStringIsValid([dialString UTF8String]);
		res = bRes ? YES : NO;
	}
	
	return res;
}

- (BOOL)dialStringIsSorenson:(NSString *)dialString
{
	//TODO: implement this correctly.
	NSCharacterSet *dotSet = [NSCharacterSet characterSetWithCharactersInString:@"."];
	NSCharacterSet *charSet = [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLKMNOPQRSTUVWXYZ"];
	if ([dialString rangeOfCharacterFromSet:dotSet].location != NSNotFound) {
		if ([dialString rangeOfCharacterFromSet:charSet].location != NSNotFound) {
			return YES;  //if the string contains a dot and at least one character assume it's a valid domain (need better check)
		}
	}
	return NO;
}

- (void)fetchDialStringIsSorenson:(NSString *)dialString completionHandler:(void (^)(BOOL isSorenson, NSError *error))block
{
	[self performBlockOnMainThread:^{
		block([self dialStringIsValid:dialString], nil);
	}];
}

- (void)fetchPhoneNumberIsSorenson:(NSString *)phoneNumber completionHandler:(void (^)(BOOL isSorenson, NSError *error))block
{
	[self performBlockOnMainThread:^{
		block([self phoneNumberIsValid:phoneNumber], nil);
	}];
}

#pragma mark - Helpers: Core Version

- (void)loadCoreVersionWithCompletion:(void (^)(NSError *error))block
{
	[self fetchCoreVersionWithCompletion:^(SCICoreVersion *version, NSError *error) {
		NSError *errorOut = nil;
		
		if (version) {
			self.coreVersion = version;
			SCILog(@"Core Version: %@", [version createStringRepresentation]);
			errorOut = nil;
		} else {
			self.coreVersion = nil;
			SCILog(@"Failed to load Core Version.");
			errorOut = error;
		}
		
		if (block)
			block(errorOut);
	}];
}

- (void)fetchCoreVersionWithCompletion:(void (^)(SCICoreVersion *version, NSError *error))block
{
	[self fetchCoreVersionStringWithCompletion:^(NSString *versionString, NSError *error) {
		SCICoreVersion *coreVersionOut = nil;
		NSError *errorOut = nil;
		
		if (versionString) {
			coreVersionOut = [SCICoreVersion coreVersionFromString:versionString];
			errorOut = nil;
		} else {
			coreVersionOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(coreVersionOut, errorOut);
	}];
}

- (void)fetchCoreVersionStringWithCompletion:(void (^)(NSString *versionString, NSError *error))block
{
	CstiCoreRequest *request = new CstiCoreRequest();

	request->versionGet();
	
	[self sendCoreRequest:request completion:^(CstiCoreResponse *response, NSError *sendError) {
		NSString *versionOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto pszVersionNumber = response->ResponseStringGet(1);
			versionOut = [NSString stringWithUTF8String:pszVersionNumber.c_str()];
		} else {
			errorOut = sendError;
		}
		
		if (block)
			block(versionOut, errorOut);
	}];
}

- (void)handleVersionGetResultWithResponse:(CstiCoreResponse *)response
{
	if (response) {
		auto pszVersionNumber = response->ResponseStringGet(1);
		NSString *versionString = [NSString stringWithUTF8String:pszVersionNumber.c_str()];
		
		if (versionString) {
			SCICoreVersion *coreVersion = [SCICoreVersion coreVersionFromString:versionString];
			if (coreVersion) {
				self.coreVersion = coreVersion;
				SCILog(@"Core Version: %@", [coreVersion createStringRepresentation]);
			}
		}
	}
}

#pragma mark - Public API: User Photos

- (void)setUserAccountPhoto:(NSData *)data completion:(void (^)(BOOL, NSError *))block
{
	if (data)
	{
		[self uploadUserAccountPhoto:data completion:block];
	}
	else
	{
		[self deleteUserAccountPhotoWithCompletion:block];
	}
}

- (void)uploadUserAccountPhoto:(NSData *)data completion:(void (^)(BOOL, NSError *))block
{
	CstiCoreRequest *request = new CstiCoreRequest();
	request->imageUploadURLGet();
	
	__weak SCIVideophoneEngine *w_self = self;
	[self sendCoreRequest:request completion:^(CstiCoreResponse *response, NSError *error) {
		BOOL successOut = NO;
		NSError *errorOut = nil;
		BOOL shouldPerformBlock = YES;
		
		if (response) {
			auto pszImageId = response->ResponseStringGet(0);
			auto pszURL1 = response->ResponseStringGet(1);
			auto pszURL2 = response->ResponseStringGet(2);
			NSString *imageId = !pszImageId.empty() ? [NSString stringWithUTF8String:pszImageId.c_str()] : nil;
			NSString *url1 = !pszURL1.empty() ? [NSString stringWithUTF8String:pszURL1.c_str()] : nil;
			NSString *url2 = !pszURL2.empty() ? [NSString stringWithUTF8String:pszURL2.c_str()] : nil;
			
			SCIVideophoneEngine *s_self = w_self;
			s_self->mImageUploader = [[SCIHTTPImageTransferrer alloc] init];
			NSMutableArray *urls = [NSMutableArray arrayWithCapacity:2];
			if (url1) [urls addObject:url1];
			if (url2) [urls addObject:url2];
			s_self->mImageUploader.URLs = [urls copy];
			//TODO: Are these the correct parameters to use?
			s_self->mImageUploader.uploadFilename = imageId;
			s_self->mImageUploader.uploadName = imageId;
			//TODO: Add method to detrmine image type
			s_self->mImageUploader.uploadFormat = @"jpeg";
			[s_self->mImageUploader uploadImageData:data completion:^(BOOL success, NSError *error) {
				if (block)
					[self performBlockOnMainThread:^{
						block(success, error);
					}];
			}];
			
			shouldPerformBlock = NO;
		} else {
			successOut = NO;
			errorOut = error;
			shouldPerformBlock = YES;
		}
		
		if (block && shouldPerformBlock)
			block(successOut, errorOut);
	}];
}

- (void)deleteUserAccountPhotoWithCompletion:(void (^)(BOOL, NSError *))block
{
	CstiCoreRequest *request = new CstiCoreRequest();
	request->imageDelete();
	[self sendBasicCoreRequest:request completion:^(NSError *error) {
		if (block)
			block(!error, error);
	}];
}

- (void)loadPhotoForContact:(SCIContact *)contact completion:(void (^)(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block
{
	NSArray *phones = [contact phonesOfTypes:@[@(SCIContactPhoneHome), @(SCIContactPhoneWork), @(SCIContactPhoneCell)]];
	
	__block NSError *errorOut = nil;
	[phones bnr_enumerateOnQueue:NULL
					continuation:^(id obj, NSUInteger index, void (^continuation)(void)) {
						NSString *phone = (NSString *)obj;
						[self loadImageForString:phone type:CstiCoreRequest::ePHONE_NUMBER completion:^(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
							if (data) {
								block(data, phoneNumber, imageId, imageDate, error);
							} else {
								errorOut = error;
								continuation();
							}
						}];
					}
					  completion:^{
						  //If we've gotten here, that means that we did not manage to get data back for any of the phone numbers.
						  //Call the block with errorOut, which was set to be the latest error.
						  block(nil, nil, nil, nil, errorOut);
					  }];
}

- (void)setPhotoTimestampForContact:(SCIContact *)contact completion:(void (^)(void))block
{
	if (contact.photoIdentifier) {
		CstiCoreRequest *request = new CstiCoreRequest();
		request->imageDownloadURLGet(contact.photoIdentifier.UTF8String, CstiCoreRequest::ePUBLIC_ID);
		
		[self sendCoreRequest:request completion:^(CstiCoreResponse *response, NSError *error) {
			NSString *imageDate = nil;
#if APPLICATION == APP_NTOUCH_IOS
			BOOL isUpdatedDate = NO;
#endif
			if (response) {
				auto pszImageDate = response->ResponseStringGet(2);
				NSString *newDate = [NSString stringWithUTF8String:pszImageDate.c_str()];
#if APPLICATION == APP_NTOUCH_IOS
				isUpdatedDate = ![newDate isEqualToString:contact.photoTimestamp];
#endif
				imageDate = !pszImageDate.empty() ? newDate : nil;
			}
			
#if APPLICATION == APP_NTOUCH_IOS
			// Image not found or timestamp has changed, clear thumbnail cache in contact
			if (error.code == SCICoreResponseErrorCodeImageNotFound || isUpdatedDate) {
				contact.contactPhotoThumbnail = nil;
			}
#endif
	
			// Image not foud error, reset back to automatic update identifier
			if (error.code == SCICoreResponseErrorCodeImageNotFound && PhotoManagerPhotoTypeForContact(contact) == PhotoTypeSorenson) {
				contact.photoIdentifier = [NSString stringWithFormat:@"00000000-0000-0000-0000-000000000001"];
				contact.photoTimestamp = nil;
				[[SCIContactManager sharedManager] updateContact:contact];
			}
			else {
				[contact setPhotoTimestamp:imageDate];
			}
			
			if(block)
				block();
		}];
	} else {
		if(block)
			block();
	}
}

- (void)loadUserAccountPhotoWithCompletion:(void (^)(NSData *, NSError *))block
{
	if (self.coreVersion.majorVersionNumber >= SCICoreMajorVersionNumber4) {
		[self loadImageForString:self.userAccountInfo.preferredNumber type:CstiCoreRequest::ePHONE_NUMBER completion:^(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
			block(data, error);
		}];
	} else {
		[self performBlockOnMainThread:^{
			NSDictionary *dictionary = @{ NSLocalizedDescriptionKey : @"Downloading user photos is not supported on this version of Core." };
			NSError *error = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeUserAccountPhotoFeatureNotSupportedByCoreVersion userInfo:dictionary];
			block(nil, error);
		}];
	}
}

- (void)loadContactPhotoForNumber:(NSString *)number completion:(void (^)(NSData *, NSError *))block
{
	[self loadImageForString:number type:CstiCoreRequest::ePHONE_NUMBER completion:^(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error) {
		block(data, error);
	}];
}

- (void)loadPreviewImageForMessageInfo:(SCIMessageInfo *)messageInfo completion:(void (^)(NSData *, NSError *))block
{
	__weak SCIVideophoneEngine *w_self = self;
	[self performBlockOnMainThread:^{
		SCIVideophoneEngine *s_self = w_self;
		s_self->mImageDownloader = [[SCIHTTPImageTransferrer alloc] init];
		NSMutableArray *urls = [NSMutableArray arrayWithCapacity:1];
		if (messageInfo.previewImageURL) [urls addObject:messageInfo.previewImageURL];
		s_self->mImageDownloader.URLs = [urls copy];
		[s_self->mImageDownloader downloadImageDataWithCompletion:^(NSData *data, NSError *error) {
			if (block)
			{
				if ([[error domain] isEqualToString:SCIErrorDomainHTTPResponse])
				{
					NSURL *url = [[NSBundle mainBundle] URLForResource:@"PersonPlaceholder" withExtension:@"pdf"];
					
					CGPDFDocumentRef docRef = CGPDFDocumentCreateWithURL( (__bridge CFURLRef) url );
					CGPDFPageRef pageRef = CGPDFDocumentGetPage(docRef, 1);
					CGRect pageRect = CGPDFPageGetBoxRect(pageRef, kCGPDFMediaBox);
					
					int pixelsWide = pageRect.size.width;
					int pixelsHigh = pageRect.size.height;
					
					CGContextRef    context = NULL;
					CGColorSpaceRef colorSpace;
					int             bitmapBytesPerRow;
					
					bitmapBytesPerRow   = (pixelsWide * 4);
					
					colorSpace = CGColorSpaceCreateDeviceRGB();
					
					context = CGBitmapContextCreate (NULL,
													 pixelsWide,
													 pixelsHigh,
													 8,
													 bitmapBytesPerRow,
													 colorSpace,
													 (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
					CGColorSpaceRelease( colorSpace );
					
					
					CGContextSetRGBFillColor(context, 1.0,1.0,1.0,1.0);
					CGContextFillRect(context, CGRectMake(0, 0, pixelsWide, pixelsHigh));
					CGContextDrawPDFPage(context, pageRef);
					
					CGPDFDocumentRelease (docRef);
					
					CGImageRef imageRef = CGBitmapContextCreateImage(context);
					
					NSMutableData* destinationData = [NSMutableData dataWithLength:0];
					CGImageDestinationRef destination = CGImageDestinationCreateWithData((__bridge CFMutableDataRef)destinationData, CFSTR("public.jpeg"), 1, NULL);
					
					const float JPEGCompQuality = 0.85f; // JPEGHigherQuality
					NSDictionary* optionsDict = @{ (__bridge NSString*)kCGImageDestinationLossyCompressionQuality : [NSNumber numberWithFloat:JPEGCompQuality] };
					
					CGImageDestinationAddImage( destination, imageRef, (__bridge CFDictionaryRef)optionsDict );
					CGImageDestinationFinalize( destination );
					CGImageRelease(imageRef);
					CFRelease(destination);
					CFRelease(context);
					data = destinationData;
				}
				[self performBlockOnMainThread:^{
					block(data, error);
				}];
			}
		}];
	}];
}

- (void)loadImageForString:(NSString *)string type:(CstiCoreRequest::EIdType)type completion:(void (^)(NSData *data, NSString *phoneNumber, NSString *imageId, NSString *imageDate, NSError *error))block
{
	if (string) {
		CstiCoreRequest *request = new CstiCoreRequest();
		request->imageDownloadURLGet(string.UTF8String, type);
		
		__weak SCIVideophoneEngine *w_self = self;
		[self sendCoreRequest:request completion:^(CstiCoreResponse *response, NSError *error) {
			NSData *photoDataOut = nil;
			NSString *phoneNumberOut = nil;
			NSString *imageIdOut = nil;
			NSString *imageDateOut = nil;
			NSError *errorOut = nil;
			BOOL shouldPerformBlock = YES;
			
			if (response) {
				auto pszPhoneNumber = response->ResponseStringGet(0);
				auto pszImagePublicId = response->ResponseStringGet(1);
				auto pszImageDate = response->ResponseStringGet(2);
				auto pszDownloadURL1 = response->ResponseStringGet(3);
				auto pszDownloadURL2 = response->ResponseStringGet(4);
				
				NSString *phoneNumber = !pszPhoneNumber.empty() ? [NSString stringWithUTF8String:pszPhoneNumber.c_str()] : nil;
				NSString *imageId = !pszImagePublicId.empty() ? [NSString stringWithUTF8String:pszImagePublicId.c_str()] : nil;
				NSString *imageDate = !pszImageDate.empty() ? [NSString stringWithUTF8String:pszImageDate.c_str()] : nil;
				NSString *url1 = !pszDownloadURL1.empty() ? [NSString stringWithUTF8String:pszDownloadURL1.c_str()] : nil;
				NSString *url2 = !pszDownloadURL2.empty() ? [NSString stringWithUTF8String:pszDownloadURL2.c_str()] : nil;
				
				SCIVideophoneEngine *s_self = w_self;
				s_self->mImageDownloader = [[SCIHTTPImageTransferrer alloc] init];
				NSMutableArray *urls = [NSMutableArray arrayWithCapacity:2];
				if (url1) [urls addObject:url1];
				if (url2) [urls addObject:url2];
				s_self->mImageDownloader.URLs = [urls copy];
				[s_self->mImageDownloader downloadImageDataWithCompletion:^(NSData *data, NSError *error) {
					if (block)
					{
						if ([[error domain] isEqualToString:SCIErrorDomainHTTPResponse])
						{
							NSURL *url = [[NSBundle mainBundle] URLForResource:@"PersonPlaceholder" withExtension:@"pdf"];
							
							CGPDFDocumentRef docRef = CGPDFDocumentCreateWithURL( (__bridge CFURLRef) url );
							CGPDFPageRef pageRef = CGPDFDocumentGetPage(docRef, 1);
							CGRect pageRect = CGPDFPageGetBoxRect(pageRef, kCGPDFMediaBox);
							
							int pixelsWide = pageRect.size.width;
							int pixelsHigh = pageRect.size.height;
							
							CGContextRef    context = NULL;
							CGColorSpaceRef colorSpace;
							int             bitmapBytesPerRow;
							
							bitmapBytesPerRow   = (pixelsWide * 4);
							
							colorSpace = CGColorSpaceCreateDeviceRGB();
							
							context = CGBitmapContextCreate (NULL,
															 pixelsWide,
															 pixelsHigh,
															 8,
															 bitmapBytesPerRow,
															 colorSpace,
															 (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
							CGColorSpaceRelease( colorSpace );
							
							
							CGContextSetRGBFillColor(context, 1.0,1.0,1.0,1.0);
							CGContextFillRect(context, CGRectMake(0, 0, pixelsWide, pixelsHigh));
							CGContextDrawPDFPage(context, pageRef);
							
							CGPDFDocumentRelease (docRef);
							
							CGImageRef imageRef = CGBitmapContextCreateImage(context);
							
							NSMutableData* destinationData = [NSMutableData dataWithLength:0];
							CGImageDestinationRef destination = CGImageDestinationCreateWithData((__bridge CFMutableDataRef)destinationData, CFSTR("public.jpeg"), 1, NULL);
							
							const float JPEGCompQuality = 0.85f; // JPEGHigherQuality
							NSDictionary* optionsDict = @{ (__bridge NSString*)kCGImageDestinationLossyCompressionQuality : [NSNumber numberWithFloat:JPEGCompQuality] };
							
							CGImageDestinationAddImage( destination, imageRef, (__bridge CFDictionaryRef)optionsDict );
							CGImageDestinationFinalize( destination );
							CGImageRelease(imageRef);
							CFRelease(destination);
							CFRelease(context);
							data = destinationData;
						}
						[self performBlockOnMainThread:^{
							block(data, phoneNumber, imageId, imageDate, error);
						}];
					}
				}];
				
				shouldPerformBlock = NO;
			} else {
				photoDataOut = nil;
				phoneNumberOut = nil;
				imageIdOut = nil;
				imageDateOut = nil;
				errorOut = error;
				shouldPerformBlock = YES;
			}
			
			if (block && shouldPerformBlock)
				block(photoDataOut, phoneNumberOut, imageIdOut, imageDateOut, errorOut);
		}];
	} else {
		if (block)
			[self performBlockOnMainThread:^{
				NSString *localizedDescription = @"No string provided for which to fetch a photo.";
				NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDescription, NSLocalizedDescriptionKey, nil];
				NSError *error = [NSError errorWithDomain:SCIErrorDomainGeneral code:0 userInfo:userInfo];
				block(nil, nil, nil, nil, error);
			}];
	}
}

#pragma mark - SignMail

- (void)updateLastMessageViewTime {
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	if (msgMgr) {
		msgMgr->lastSignMailUpdateSet();
	}
}

- (NSUInteger)numberOfSignMailMessages
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	unsigned int count;
	msgMgr->SignMailCountGet(&count);
	return count;
}

- (NSUInteger)numberOfNewMessagesInCategory:(SCIItemId *)categoryId
{
	if (!categoryId)
		return 0;
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	unsigned int count;
	msgMgr->NewMessageCountGet(categoryId.cstiItemId, &count);
	return count;
}

- (SCIItemId *)signMailMessageCategoryId
{
	if (mVideophoneEngine == nullptr) {
		return nil;
	}
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	unsigned int categoryCount;
	msgMgr->CategoryCountGet(&categoryCount);
	for (int i = 0; i < categoryCount; i++)
	{
		CstiItemId *iterCategoryId;
		msgMgr->CategoryByIndexGet(i, &iterCategoryId);
		int categoryType;
		char szCatShortName[255];
		char szCatLongName[255];
		msgMgr->CategoryByIdGet(*iterCategoryId, NULL, &categoryType, szCatShortName, 255, szCatLongName, 255);
		if (categoryType == estiMT_SIGN_MAIL)
		{
			SCIItemId *itemIdOut = [SCIItemId itemIdWithCstiItemId:*iterCategoryId];
			delete iterCategoryId;
			return itemIdOut;
		}
		delete iterCategoryId;
	}
	return nil;
}

- (SCIMessageCategory *)messageCategoryForCategoryId:(SCIItemId *)categoryId
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	int categoryType;
	char szCatShortName[255];
	char szCatLongName[255];
	msgMgr->CategoryByIdGet(categoryId.cstiItemId, NULL, &categoryType, szCatShortName, 255, szCatLongName, 255);
	
	CstiItemId *cstiSupercategoryId = NULL;
	msgMgr->CategoryParentGet(categoryId.cstiItemId, &cstiSupercategoryId);
	
	SCIItemId *supercategoryId = nil;
	if (cstiSupercategoryId) {
		supercategoryId = [SCIItemId itemIdWithCstiItemId:*cstiSupercategoryId];
		delete cstiSupercategoryId;
	}
	
	SCIMessageCategory *messageCategory = [SCIMessageCategory messageCategoryWithId:categoryId
																	supercategoryId:supercategoryId
																			   type:(NSUInteger)categoryType
																		   longName:[NSString stringWithUTF8String:szCatLongName]
																		  shortName:[NSString stringWithUTF8String:szCatShortName]];
	return messageCategory;
}

- (SCIMessageInfo *)messageInfoForMessageId:(SCIItemId *)messageId
{
	SCIMessageInfo *res = nil;
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	CstiMessageInfo info;
	CstiItemId itemId = messageId.cstiItemId;
	info.ItemIdSet(itemId);
	
	stiHResult messageInfoGetResult = msgMgr->MessageInfoGet(&info);
	if (messageInfoGetResult == stiRESULT_SUCCESS) {
		CstiItemId *pCategoryId = NULL;
		stiHResult messageCategoryGetResult = msgMgr->MessageCategoryGet(messageId.cstiItemId, &pCategoryId);
		if (messageCategoryGetResult == stiRESULT_SUCCESS) {
			CstiItemId categoryId = *pCategoryId;
			res = [SCIMessageInfo infoWithCstiMessageInfo:info category:categoryId];
			delete pCategoryId;
			pCategoryId = NULL;
		}
	}
	
	return res;
}

- (NSArray<SCIMessageCategory *> *)messageCategories
{
	NSMutableArray<SCIMessageCategory *> *mutableMessageCategories = [NSMutableArray array];
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	unsigned int categoryCount;
	msgMgr->CategoryCountGet(&categoryCount);
	for (int i = 0; i < categoryCount; i++)
	{
		CstiItemId *iterCategoryId;
		msgMgr->CategoryByIndexGet(i, &iterCategoryId);
		
		int categoryType;
		char szCatShortName[255];
		char szCatLongName[255];
		msgMgr->CategoryByIdGet(*iterCategoryId, NULL, &categoryType, szCatShortName, 255, szCatLongName, 255);
		
		SCIItemId *itemId = [SCIItemId itemIdWithCstiItemId:*iterCategoryId];
		delete iterCategoryId;
		
		SCIMessageCategory *messageCategory = [SCIMessageCategory messageCategoryWithId:itemId
																		supercategoryId:nil
																				   type:(NSUInteger)categoryType
																			   longName:[NSString stringWithUTF8String:szCatLongName]
																			  shortName:[NSString stringWithUTF8String:szCatShortName]];
		[mutableMessageCategories addObject:messageCategory];
	}
	
	return [mutableMessageCategories copy];
}

- (NSArray<SCIMessageCategory *> *)messageSubcategoriesForCategoryId:(SCIItemId *)supercategoryId
{
	if (!supercategoryId) {
		return nil;
	}
	
	NSMutableArray<SCIMessageCategory *> *mutableSubcategories = [NSMutableArray array];
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	unsigned int subcategoryCount;
	msgMgr->SubCategoryCountGet(supercategoryId.cstiItemId, &subcategoryCount);
	for (int i = 0; i < subcategoryCount; i++) {
		CstiItemId *iterCategoryId;
		msgMgr->SubCategoryByIndexGet(supercategoryId.cstiItemId, i, &iterCategoryId);
		
		int categoryType;
		char szCatShortName[255];
		char szCatLongName[255];
		msgMgr->CategoryByIdGet(*iterCategoryId, NULL, &categoryType, szCatShortName, 255, szCatLongName, 255);
		
		SCIItemId *categoryId = [SCIItemId itemIdWithCstiItemId:*iterCategoryId];
		delete iterCategoryId;
		
		SCIMessageCategory *messageSubcategory = [SCIMessageCategory messageCategoryWithId:categoryId
																		   supercategoryId:supercategoryId
																					  type:(NSUInteger)categoryType
																				  longName:[NSString stringWithUTF8String:szCatLongName]
																				 shortName:[NSString stringWithUTF8String:szCatShortName]];
		[mutableSubcategories addObject:messageSubcategory];
	}
	
	return [mutableSubcategories copy];
}

- (NSArray<SCIMessageInfo *> *)messagesForCategoryId:(SCIItemId *)categoryId
{
	if (!categoryId) {
		return nil;
	}
	
	
	NSMutableArray<SCIMessageInfo *> *output = [NSMutableArray array];
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();

	unsigned int messageCount;
	msgMgr->MessageCountGet(categoryId.cstiItemId, &messageCount);
	
	for (int i = 0; i < messageCount; i++)
	{
		CstiMessageInfo info;
		msgMgr->MessageByIndexGet(categoryId.cstiItemId, i, &info);
	
		CstiItemId cstiCategoryId = [categoryId cstiItemId];
		SCIMessageInfo *sciInfo = [SCIMessageInfo infoWithCstiMessageInfo:info category:cstiCategoryId];
		[output addObject:sciInfo];
	}
	
	return output;
}

- (SCIMessageCategory *)messageCategoryForMessage:(SCIMessageInfo *)info
{
	if (!info) {
		return nil;
	}
	
	CstiItemId *cstiCategoryId = NULL;
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	msgMgr->MessageCategoryGet(info.messageId.cstiItemId, &cstiCategoryId);
	
	if (!cstiCategoryId) {
		return nil;
	}
	
	SCIItemId *categoryId = [SCIItemId itemIdWithCstiItemId:*cstiCategoryId];
	delete cstiCategoryId;
	
	SCIMessageCategory *messageCategory = [[SCIVideophoneEngine sharedEngine] messageCategoryForCategoryId:categoryId];
	return messageCategory;
}

- (SCIMessageCategory *)messageCategoryForMessageCategory:(SCIMessageCategory *)category
{
	if (!category) {
		return nil;
	}
	
	CstiItemId *cstiSupercategoryId = NULL;
	
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	msgMgr->CategoryParentGet(category.categoryId.cstiItemId, &cstiSupercategoryId);
	
	if (!cstiSupercategoryId) {
		return nil;
	}
	
	int categoryType;
	char szCatShortName[255];
	char szCatLongName[255];
	msgMgr->CategoryByIdGet(*cstiSupercategoryId, NULL, &categoryType, szCatShortName, 255, szCatLongName, 255);
	
	SCIItemId *supercategoryId = [SCIItemId itemIdWithCstiItemId:*cstiSupercategoryId];
	delete cstiSupercategoryId;
	
	SCIMessageCategory *messageCategory = [SCIMessageCategory messageCategoryWithId:supercategoryId
																	supercategoryId:nil
																			   type:(NSUInteger)categoryType
																		   longName:[NSString stringWithUTF8String:szCatLongName]
																		  shortName:[NSString stringWithUTF8String:szCatShortName]];
	return messageCategory;
}

- (NSUInteger)signMailCapacity
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	unsigned int count = 0;
	msgMgr->SignMailMaxCountGet(&count);
	
	return count;
}

- (void)downloadURLforMessage:(SCIMessageInfo *)message completion:(void (^)(NSString *downloadURL, NSString *viewId, NSError *sendError))block
{
	
	CstiItemId itemId = message.messageId.cstiItemId;
	CstiMessageRequest *request = new CstiMessageRequest();
	request->MessageDownloadURLGet(itemId);

	[self sendComplexMessageRequest:(CstiMessageRequest *)request completion:^(CstiMessageResponse *response, NSError *sendError) {
		NSString *downloadURLout;
		NSString *viewIdOut;
		NSError *errorOut;
		
		if (response) {
			NSError *error = NSErrorFromCstiMessageResponse(response);
			if (error) {
				downloadURLout = nil;
				errorOut = error;
			} else {
				auto url = response->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_GUID);
				auto viewID = response->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD);
				downloadURLout = [NSString stringWithUTF8String:url.c_str()];
				viewIdOut = [NSString stringWithUTF8String:viewID.c_str()];
				errorOut = error;
			}
		}
		
		if (block) {
			block(downloadURLout, viewIdOut, errorOut);
		}
	}];
}

- (void)requestDownloadURLsForMessage:(SCIMessageInfo *)message completion:(void (^)(NSArray *downloadURLs, NSString *viewId, NSError *sendError))block
{
	CstiItemId itemId = message.messageId.cstiItemId;
	CstiMessageRequest *request = new CstiMessageRequest();
	request->MessageDownloadURLListGet(itemId);
	
	[self sendComplexMessageRequest:(CstiMessageRequest *)request completion:^(CstiMessageResponse *response, NSError *sendError)
	{
		if (response)
		{
			NSError *error = NSErrorFromCstiMessageResponse(response);
			if (!error)
			{
				auto viewID = response->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD);
				
				NSMutableArray *downloadURLs = [NSMutableArray array];
				auto const& downloadURLsVector = *response->MessageDownloadUrlListGet();
				for (auto const& downloadURLItem : downloadURLsVector)
				{
					[downloadURLs addObject:[SCIMessageDownloadURLItem itemWithCstiMessageDownloadURLItem:downloadURLItem]];
				}
				
				block([downloadURLs copy], [NSString stringWithUTF8String:viewID.c_str()], nil);
			}
			else
			{
				block(nil, nil, error);
			}
		}
		else {
			block(nil, nil, sendError);
		}
	}];
}

- (void)downloadURLListforMessage:(SCIMessageInfo *)message completion:(void (^)(NSString *downloadURL, NSString *viewId, NSError *sendError))block
{
	__weak SCIVideophoneEngine *weak_self = self;
	CstiItemId itemId = message.messageId.cstiItemId;
	CstiMessageRequest *request = new CstiMessageRequest();
	request->MessageDownloadURLListGet(itemId);
	
	[self sendComplexMessageRequest:(CstiMessageRequest *)request completion:^(CstiMessageResponse *response, NSError *sendError) {
		
		__strong SCIVideophoneEngine *strong_self = weak_self;
		NSString *downloadURLout;
		NSString *viewIdOut;
		NSError *errorOut;
		
		uint32_t maxRecv, maxSend;
		strong_self.videophoneEngine->MaxSpeedsGet(&maxRecv, &maxSend);
		
		if (response) {
			NSError *error = NSErrorFromCstiMessageResponse(response);
			if (error)
			{
				downloadURLout = nil;
				errorOut = error;
			}
			else
			{
				auto viewID = response->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD);
				std::vector<CstiMessageResponse::CstiMessageDownloadURLItem> messageList = *response->MessageDownloadUrlListGet();
				NSString *downloadURL;

				if (!messageList.empty())
				{
					// Get the URL for the SIF version of the SignMail from the sorted list for a fallback
					downloadURL= [SCIMessageDownloadURLItem itemWithCstiMessageDownloadURLItem:messageList.back()].downloadURL;
					
					for (auto downloadItem : messageList)
					{
						SCIMessageDownloadURLItem *item = [SCIMessageDownloadURLItem itemWithCstiMessageDownloadURLItem:downloadItem];
						
						if (item.maxBitRate < maxRecv)
						{
							downloadURL = item.downloadURL;
							break;
						}
					}
				}
				
				downloadURLout = downloadURL;
				viewIdOut = [NSString stringWithUTF8String:viewID.c_str()];
				errorOut = error;
			}
		}
		
		if (block) {
			block(downloadURLout, viewIdOut, errorOut);
		}
	}];
}

- (void)markMessage:(SCIMessageInfo *)message viewed:(BOOL)viewed
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	CstiItemId itemId = message.messageId.cstiItemId;
	CstiMessageInfo info;
	info.ItemIdSet(itemId);
	msgMgr->MessageInfoGet(&info);
	info.ViewedSet(viewed ? estiTRUE : estiFALSE);
	msgMgr->MessageInfoSet(&info);
	
	message.viewed = viewed;
}

- (void)markMessage:(SCIMessageInfo *)message pausePoint:(NSTimeInterval)seconds
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	CstiItemId itemId = message.messageId.cstiItemId;
	CstiMessageInfo info;
	info.ItemIdSet(itemId);
	msgMgr->MessageInfoGet(&info);
	info.PausePointSet((uint32_t)seconds);
	
	// ViewID for this SCIMessageInfo must be updated when downloadURLForMessage response is handled.
	if (message.viewId) {
		info.ViewIdSet([message.viewId UTF8String]);
	}
	msgMgr->MessageInfoSet(&info);
	message.pausePoint = seconds;
	
	SCIItemId *messageItemId = [SCIItemId itemIdWithCstiItemId:itemId];
	[self postOnMainThreadNotificationName:SCINotificationMessageChanged
								  userInfo:[NSDictionary dictionaryWithObject:messageItemId forKey:SCINotificationKeyItemId]];
}

- (void)deleteMessage:(SCIMessageInfo *)message
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	CstiItemId itemId = message.messageId.cstiItemId;
	msgMgr->MessageItemDelete(itemId);
}

- (void)deleteMessagesInCategory:(SCIItemId *)categoryId
{
	IstiMessageManager *msgMgr = mVideophoneEngine->MessageManagerGet();
	
	CstiItemId itemId = categoryId.cstiItemId;
	msgMgr->MessagesInCategoryDelete(itemId);
}

- (void)registerSignMail
{
	[self registerSignMailWithCompletion:nil];
}

- (void)registerSignMailWithCompletion:(void (^)(NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->signMailRegister();
	
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (SCICall *)signMailSend:(NSString *)dialString error:(NSError **)errorOut {
	if (self.isStarted == NO)
	{
		NSString *localizedDesc = @"Connecting to server. Please try again later.";
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
		if (errorOut) {
			*errorOut = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEngineNotStarted userInfo:userInfo];
		}
		return nil;
	}
	if (dialString == nil || dialString.length == 0)
	{
		NSString *localizedDesc = @"No phone number supplied.";
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
		if (errorOut) {
			*errorOut = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEmptyDialString userInfo:userInfo];
		}
		return nil;
	}
	
	IstiCall* poCall = NULL;
	stiHResult result = stiRESULT_SUCCESS;
	result = self.videophoneEngine->SignMailSend([dialString UTF8String], EstiDialSourceForSCIDialSource(self.callDialSource), &poCall);
	if (stiIS_ERROR(result))
	{
		stiHResult resultCode = stiRESULT_CODE(result);
		if (errorOut)
		{
			NSString *localizedDesc = @"An error occurred while connecting to SignMail.";
			if (resultCode == stiRESULT_UNABLE_TO_DIAL_SELF) {
				localizedDesc = @"Sorry, you can’t send a SignMail to yourself.";
			} else if (resultCode == stiRESULT_SVRS_CALLS_NOT_ALLOWED) {
				localizedDesc = @"This number can't receive a SignMail. Call this number directly?";
			}
			NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
			*errorOut = [NSError errorWithDomain:SCIErrorDomainGeneral code:resultCode userInfo:userInfo];
		}
		return nil;
	}
	else {
		[mCallInProgressCondition lock];
		mCallInProgress = YES;
		[mCallInProgressCondition signal];
		[mCallInProgressCondition unlock];
		
		if (poCall)
		{
			SCICall *output = [self callForIstiCall:poCall];
			return output;
		}
		return NULL;
	}
	return nil;
}

#pragma mark - Ring Groups

- (void)setSipProxyAddress:(NSString *)SipProxyAddress
{
	if (mVideophoneEngine) {
		mVideophoneEngine->SipProxyAddressSet([SipProxyAddress UTF8String]);
	}
	else {
		NSLog(@"Unable to call SipProxyAddressSet");
	}
}

- (BOOL)isIPv6Enabled {
	std::string tmp;
	return stiIS_ERROR(stiGetLocalIp(&tmp, estiTYPE_IPV4))
		&& !stiIS_ERROR(stiGetLocalIp(&tmp, estiTYPE_IPV6));
}

- (void)createRingGroupWithAlias:(NSString *)alias password:(NSString *)password completion:(void (^)(NSError *))block
{
	block = [block copy];
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupCreate(alias.UTF8String, password.UTF8String);
	[self sendBasicCoreRequest:coreRequest completion:^(NSError *error) {
		block(error);
		[self loadUserAccountInfo];
	}];
}

- (void)validateRingGroupCredentialsWithPhone:(NSString *)phone password:(NSString *)password completion:(void (^)(NSError *))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupCredentialsValidate(phone.UTF8String, password.UTF8String);
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (void)fetchRingGroupInfoWithCompletion:(void (^)(NSArray *ringGroupInfos, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupInfoGet();
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSArray *ringGroupInfosOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			std::vector<CstiCoreResponse::SRingGroupInfo> list;
			response->RingGroupInfoListGet (&list);
			
			NSMutableArray *mutableRingGroupInfos = [NSMutableArray array];
			std::vector<CstiCoreResponse::SRingGroupInfo>::const_iterator i;
			std::vector<CstiCoreResponse::SRingGroupInfo>::const_iterator end = list.end();
			for (i = list.begin (); i != end; i++) {
				CstiCoreResponse::SRingGroupInfo sRingGroupInfo = *i;
				SCIRingGroupInfo *info = [SCIRingGroupInfo ringGroupInfoWithSRingGroupInfo:&sRingGroupInfo];
				[mutableRingGroupInfos addObject:info];
			}
			NSArray *ringGroupInfos = [mutableRingGroupInfos copy];
			
			ringGroupInfosOut = ringGroupInfos;
			errorOut = nil;
		} else {
			ringGroupInfosOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(ringGroupInfosOut, errorOut);
	}];
}

- (void)setRingGroupInfoWithPhone:(NSString *)phone description:(NSString *)description completion:(void (^)(NSError *error))block
{
	IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
	unsigned int requestId = 0;
	stiHResult res = ringGroupManager->ItemEdit(phone.UTF8String, description.UTF8String, &requestId);
	block = [block copy];
	if (res == stiRESULT_SUCCESS) {
		[self handleRingGroupManagerResponse:requestId withBlock:^(IstiRingGroupMgrResponse *response) {
			NSError *error = NSErrorFromIstiRingGroupMgrResponse(response);
			block(error);
		}];
	} else {
		[self performBlockOnMainThread:^{
			NSError *err = [NSError errorWithDomain:SCIErrorDomainRingGroupManagerRequestSend code:res userInfo:nil];
			if (block)
				block(err);
		}];
	}
}

- (void)fetchRingGroupInviteInfoWithCompletion:(void (^)(NSString *name, NSString *localPhone, NSString *tollFreePhone, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupInviteInfoGet();
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSString *nameOut = nil;
		NSString *localPhoneOut = nil;
		NSString *tollFreePhoneOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto pszName = response->ResponseStringGet(0);
			auto pszLocalPhone = response->ResponseStringGet(1);
			auto pszTollFreePhone = response->ResponseStringGet(2);
			
			nameOut = !pszName.empty() ? [NSString stringWithUTF8String:pszName.c_str()] : nil;
			localPhoneOut = !pszLocalPhone.empty() ? [NSString stringWithUTF8String:pszLocalPhone.c_str()] : nil;
			tollFreePhoneOut = !pszTollFreePhone.empty() ? [NSString stringWithUTF8String:pszTollFreePhone.c_str()] : nil;
			errorOut = nil;
		} else {
			nameOut = nil;
			localPhoneOut = nil;
			tollFreePhoneOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(nameOut, localPhoneOut, tollFreePhoneOut, errorOut);
	}];
}

- (void)acceptRingGroupInvitationWithPhone:(NSString *)phone completion:(void (^)(NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupInviteAccept(phone.UTF8String);
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (void)rejectRingGroupInvitationWithPhone:(NSString *)phone completion:(void (^)(NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupInviteReject(phone.UTF8String);
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (void)setRingGroupPassword:(NSString *)password completion:(void (^)(NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	__unused int result = coreRequest->ringGroupPasswordSet(password.UTF8String);
	[self sendBasicCoreRequest:coreRequest completion:block];
}

- (void)inviteRingGroupPhone:(NSString *)phone description:(NSString *)desc completion:(void (^)(NSError *error))block
{
	IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
	unsigned int requestId = 0;
	stiHResult res = ringGroupManager->ItemAdd(phone.UTF8String, desc.UTF8String, &requestId);
	block = [block copy];
	if (res == stiRESULT_SUCCESS) {
		[self handleRingGroupManagerResponse:requestId withBlock:^(IstiRingGroupMgrResponse *response) {
			NSError *error = NSErrorFromIstiRingGroupMgrResponse(response);
			block(error);
		}];
	} else {
		[self performBlockOnMainThread:^{
			NSError *err = [NSError errorWithDomain:SCIErrorDomainRingGroupManagerRequestSend code:res userInfo:nil];
			if (block)
				block(err);
		}];
	}
}

- (void)removeRingGroupPhone:(NSString *)phone completion:(void (^)(NSError *error))block
{
	IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
	unsigned int requestId = 0;
	stiHResult res = ringGroupManager->ItemDelete(phone.UTF8String, &requestId);
	block = [block copy];
	if (res == stiRESULT_SUCCESS) {
		[self handleRingGroupManagerResponse:requestId withBlock:^(IstiRingGroupMgrResponse *response) {
			NSError *error = NSErrorFromIstiRingGroupMgrResponse(response);
			block(error);
		}];
	} else {
		[self performBlockOnMainThread:^{
			NSError *err = [NSError errorWithDomain:SCIErrorDomainRingGroupManagerRequestSend code:res userInfo:nil];
			if (block)
				block(err);
		}];
	}
}

- (BOOL)isNameRingGroupMember:(NSString*)name {
	if (self.videophoneEngine) {
		IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
		std::string memberNumber;
		IstiRingGroupManager::ERingGroupMemberState state;
		const char * pszName = name.UTF8String;
		EstiBool found = estiFALSE;
		if (pszName) {
			found = ringGroupManager->ItemGetByDescription(pszName, &memberNumber, &state);
		}
		return (found == estiTRUE);
	}
	else {
		return NO;
	}
}

- (BOOL)isNumberRingGroupMember:(NSString*)number {
	if (self.videophoneEngine) {
		IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
		std::string memberName;
		IstiRingGroupManager::ERingGroupMemberState state;
		const char * pszNumber = number.UTF8String;
		EstiBool found = estiFALSE;
		if (pszNumber) {
			found = ringGroupManager->ItemGetByNumber(pszNumber, &memberName, &state);
		}
		return (found == estiTRUE || [number isEqualToString:[SCIVideophoneEngine sharedEngine].userAccountInfo.preferredNumber]);
	}
	else {
		return NO;
	}
}

- (NSInteger)ringGroupDisplayMode {
	NSInteger mode = IstiRingGroupManager::eRING_GROUP_DISABLED;
	if (self.videophoneEngine) {
		IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
		if(ringGroupManager)
			mode = ringGroupManager->ModeGet();
	}
	return mode;
}

- (BOOL)isInRingGroup {
	BOOL isInRingGroup = NO;
	if (self.videophoneEngine) {
		IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
		if(ringGroupManager)
			isInRingGroup = ringGroupManager->IsInRingGroup();
	}
	return isInRingGroup;
}

#pragma mark - Accessors: Convenience

- (IstiVideophoneEngine *)videophoneEngine
{
	return mVideophoneEngine;
}

- (IstiStatusLED *)statusLEDInterface
{
	return mStatusLEDInterface->InstanceGet();
}

- (IstiBluetooth *)bluetoothInterface
{
	return mBluetoothInterface->InstanceGet();
}

#pragma mark - Accessors

- (void)setAuthorized:(BOOL)authorized
{
	[self setAuthorized:authorized postAuthorizedDidChange:YES];
}

- (void)setAuthorized:(BOOL)authorized postAuthorizedDidChange:(BOOL)notify
{
	mAuthorized = authorized;
	
	if (notify) {
		[self postAuthorizedDidChange];
	}
	
	//If we're setting authorized to YES, then we're done authorizing.
	//If we're setting authorized to NO, then we're logging out, so
	//we're not authorizing anymore.
	self.authorizing = NO;
}

- (void)setAuthorizing:(BOOL)authorizing
{
	[self setAuthorizing:authorizing postAuthorizingDidChange:YES];
}

- (void)setAuthorizing:(BOOL)authorizing postAuthorizingDidChange:(BOOL)notify
{
	mAuthorizing = authorizing;
	
	if (notify) {
		[self postAuthorizingDidChange];
	}
}

- (void)setFetchingAuthorization:(BOOL)fetchingAuthorization
{
	[self setFetchingAuthorization:fetchingAuthorization postFetchingAuthorizationDidChange:YES];
}

- (void)setFetchingAuthorization:(BOOL)fetchingAuthorization postFetchingAuthorizationDidChange:(BOOL)notify
{
	mFetchingAuthorization = fetchingAuthorization;
	
	if (notify) {
		[self postFetchingAuthorizationDidChange];
	}
}

- (void)setAuthenticated:(BOOL)authenticated
{
	[self setAuthenticated:authenticated postAuthenticatedDidChange:YES];
}

- (void)setAuthenticated:(BOOL)authenticated postAuthenticatedDidChange:(BOOL)notify
{
	//If we are not authenticated, then we are not authorized.  Since we
	//should never be in the state where we're not authenticated but still
	//authorized, change the authorized state first.
	if (!authenticated) {
		self.authorized = NO;
	}
	
	mAuthenticated = authenticated;
	
	if (notify) {
		[self postAuthenticatedDidChange];
	}
}

- (void)setConnected:(BOOL)connected
{
	[self setConnected:connected postConnectedDidChange:YES];
}

- (void)setConnected:(BOOL)connected postConnectedDidChange:(BOOL)notify
{
	mConnected = connected;
	
	//If we are connected, then we are not connecting.  If we are
	//connecting, then we are not connected.  Do this before posting
	//the notification since the observers of the notification expect
	//both values to be changed when the notification is posted.
	self.connecting = !connected;
	
	if (notify) {
		[self postConnectedDidChange];
	}
}

- (void)setConnecting:(BOOL)connecting
{
	[self setConnecting:connecting postConnectedDidChange:NO];
}

- (void)setConnecting:(BOOL)connecting postConnectedDidChange:(BOOL)notify
{
	mConnecting = connecting;
	
	if (notify) {
		[self postConnectedDidChange];
	}
}

#ifdef stiDEBUG
- (NSString *)debugStateDescription
{
	NSString *res = ^{
		NSMutableString *mutableRes = [[NSMutableString alloc] init];
		
		[mutableRes appendString:self.description];
		[mutableRes appendString:@"\n\tDEBUG STATE DESCRIPTION BEGIN: {"];
		[mutableRes appendFormat:@"\n\t\t\tisStarted = %@", SCIStringFromBOOL(self.isStarted)];
		[mutableRes appendFormat:@"\n\t\t\tisConnecting = %@", SCIStringFromBOOL(self.isConnecting)];
		[mutableRes appendFormat:@"\n\t\t\tisConnected = %@", SCIStringFromBOOL(self.isConnected)];
		[mutableRes appendFormat:@"\n\t\t\tisAuthenticated = %@", SCIStringFromBOOL(self.isAuthenticated)];
		[mutableRes appendFormat:@"\n\t\t\tisFetchingAuthorization = %@", SCIStringFromBOOL(self.isFetchingAuthorization)];
		[mutableRes appendFormat:@"\n\t\t\tisAuthorizing = %@", SCIStringFromBOOL(self.isAuthorizing)];
		[mutableRes appendFormat:@"\n\t\t\tisAuthorized = %@", SCIStringFromBOOL(self.isAuthorized)];
		[mutableRes appendFormat:@"\n\t} : END DEBUG STATE DESCRIPTION"];
		return [mutableRes copy];
	}();
	
	return res;
}
#endif

- (NSUInteger)maximumRecvSpeed
{
	uint32_t recv, send;
	mVideophoneEngine->MaxSpeedsGet(&recv, &send);
	return recv;
}
- (NSUInteger)maximumSendSpeed
{
	uint32_t recv, send;
	mVideophoneEngine->MaxSpeedsGet(&recv, &send);
	return send;
}
- (void)setMaximumRecvSpeed:(NSUInteger)maximumRecvSpeed
{
	mVideophoneEngine->MaxSpeedsSet((uint32_t)maximumRecvSpeed, (uint32_t)self.maximumSendSpeed);
}
- (void)setMaximumSendSpeed:(NSUInteger)maximumSendSpeed
{
	mVideophoneEngine->MaxSpeedsSet((uint32_t)self.maximumRecvSpeed, (uint32_t)maximumSendSpeed);
}
- (void)setMaxAutoSpeeds:(NSUInteger)recvSpeed sendSpeed:(NSUInteger)sendSpeed
{
	mVideophoneEngine->MaxAutoSpeedsSet((uint32_t)recvSpeed, (uint32_t)sendSpeed);
}

- (SCIAutoSpeedMode)networkAutoSpeedMode
{
	NSUInteger result = stiAUTO_SPEED_MODE_DEFAULT;
	if (mVideophoneEngine)
		result = mVideophoneEngine->AutoSpeedSettingGet();
	
	return (SCIAutoSpeedMode)result;
}
- (NSString *)publicIPAddress
{
	std::string address;
	stiHResult res = mVideophoneEngine->PublicIPAddressGet(estiIP_ASSIGN_AUTO, &address, estiTYPE_IPV4);
	if (res == stiRESULT_SUCCESS)
		return [NSString stringWithUTF8String:address.c_str()];
	
	NSLog(@"%s error obtaining public IP address: %d", __PRETTY_FUNCTION__, res);
	return nil;
}

- (NSString *)localIPAddress
{
	std::string address;
	stiHResult res = stiGetLocalIp(&address, estiTYPE_IPV4);
	if (res == stiRESULT_SUCCESS)
		return [NSString stringWithUTF8String:address.c_str()];
	
	NSLog(@"%s error obtaining local IP address: %d", __PRETTY_FUNCTION__, res);
	return nil;
}

-(BOOL)rejectIncomingCalls
{
	return (mVideophoneEngine->AutoRejectGet()==estiON) ? YES : NO;
}

-(void)setRejectIncomingCalls:(BOOL)reject
{
	EstiSwitch eReject = reject ? estiON : estiOFF;
	mVideophoneEngine->AutoRejectSet(eReject);
}

- (NSUInteger)contactsMaxCount
{
	IstiContactManager *contactManager = mVideophoneEngine->ContactManagerGet();
	if (contactManager) {
		return contactManager->getMaxCount();
	}
	return 0;
}

- (void)updatePassword:(NSString *)password
{
	mVideophoneEngine->CoreAuthenticationPinUpdate([password UTF8String]);
}

- (void)setRemoteVideoPrivacyEnabled:(BOOL)remoteVideoPrivacyEnabled
{
	mRemoteVideoPrivacyEnabled = remoteVideoPrivacyEnabled;
	[self postOnMainThreadNotificationName:SCINotificationRemoteVideoPrivacyChanged userInfo:nil];
}

- (SCIPersonalGreetingPreferences *)personalGreetingPreferences
{
	SstiPersonalGreetingPreferences greetingPreferences;
	self.videophoneEngine->GreetingPreferencesGet(&greetingPreferences);
	return [SCIPersonalGreetingPreferences personalGreetingPreferencesWithSstiPersonalGreetingPreferences:&greetingPreferences];
}

- (void)setPersonalGreetingPreferences:(SCIPersonalGreetingPreferences *)personalGreetingPreferences
{
	SstiPersonalGreetingPreferences preferences = [personalGreetingPreferences createSstiPersonalGreetingPreferences];
	self.videophoneEngine->GreetingPreferencesSet(&preferences);
}

- (void)sendRemoteLogEvent:(NSString *)eventString
{
	size_t size = 256;
	char name[256] = "Unknown";
	NSString *systemVersion = @"Unknown";
    NSString * UIlanguage = [[[NSLocale preferredLanguages] firstObject] LanguageCode];

#if APPLICATION == APP_NTOUCH_MAC
	sysctlbyname("hw.model", &name, &size, NULL, 0);
	unsigned major, minor, bug;
	[NSApp getSystemVersionMajor:&major minor:&minor bugFix:&bug];
	systemVersion = [NSString stringWithFormat:@"%u.%u.%u", major, minor, bug];
#elif APPLICATION == APP_NTOUCH_IOS
	sysctlbyname("hw.machine", &name, &size, NULL, 0);
	systemVersion = [[UIDevice currentDevice] systemVersion];
#endif
	NSMutableString *deviceModel = [NSMutableString stringWithUTF8String:name];

	if (mVideophoneEngine) {
		NSString *logString = [NSString stringWithFormat:@"%@ SystemVersion=%@ DeviceModel=\"%@\" PublicIP=%@ UILanguage=%@", eventString, systemVersion, deviceModel, self.publicIPAddress, UIlanguage];
        
        // remove secondary UILanguage log if it is duplicated
        if ([eventString containsString: @" UILanguage"]) {
            NSRange lastElementRange = [logString rangeOfString:@" UILanguage=" options:NSBackwardsSearch];
            logString = [logString substringToIndex: lastElementRange.location];
        }
		mVideophoneEngine->RemoteLogEventSend([logString UTF8String]);
	}
}

- (SCIInterfaceMode)interfaceMode
{
	SCIInterfaceMode mode = SCIInterfaceModeStandard;
	if (self.videophoneEngine)
		mode = SCIInterfaceModeFromEstiInterfaceMode(self.videophoneEngine->LocalInterfaceModeGet());
	
	return mode;
}
- (void)setInterfaceMode:(SCIInterfaceMode)interfaceMode
{
	self.videophoneEngine->LocalInterfaceModeSet(EstiInterfaceModeForSCIInterfaceMode(interfaceMode));
}

- (BOOL)allowIncomingCallsMode
{
	BOOL res = NO;
#ifdef stiDEBUG
	[NSException raise:@"Method is not implemented in VPE." format:@"Called %s but IstiVideophoneEngine::AllowIncomingCallsModeGet() does not exist.", __PRETTY_FUNCTION__];
#endif
	return res;
}
- (void)setAllowIncomingCallsMode:(BOOL)allowIncomingCallsMode
{
	bool bAllow = (allowIncomingCallsMode) ? true : false;
	self.videophoneEngine->AllowIncomingCallsModeSet(bAllow);
}

- (void)pulseEnable:(BOOL)enabled API_UNAVAILABLE(macos)
{
	self.statusLEDInterface->PulseNotificationsEnable ((bool)enabled);
}

- (BOOL)isPulsePowered
{
	return self.bluetoothInterface->Powered();
}

- (void)pulseMissedCall:(BOOL)missedCalls API_UNAVAILABLE(macos)
{
	if (missedCalls) {
		self.statusLEDInterface->Start (IstiStatusLED::EstiLed::estiLED_MISSED_CALL, 0);
	} else {
		self.statusLEDInterface->Stop (IstiStatusLED::EstiLed::estiLED_MISSED_CALL);
	}
}

- (void)pulseSignMail:(BOOL)newSignMails API_UNAVAILABLE(macos)
{
	if (newSignMails) {
		self.statusLEDInterface->Start (IstiStatusLED::EstiLed::estiLED_SIGNMAIL, 0);
	} else {
		self.statusLEDInterface->Stop (IstiStatusLED::EstiLed::estiLED_SIGNMAIL);
	}
}

- (SCIDeviceLocationType)deviceLocationType
{
	return (SCIDeviceLocationType)mVideophoneEngine->deviceLocationTypeGet();
}

- (void)setDeviceLocationType:(SCIDeviceLocationType)deviceLocationType
{
	mVideophoneEngine->deviceLocationTypeSet((DeviceLocationType)deviceLocationType);
}

#pragma mark - Helpers: Block Performance

- (void)performBlockOnMainThread:(void (^)())block
{
	if (!block)
		return;
	
	if  (dispatch_queue_get_label(dispatch_get_main_queue()) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL))
		block();
	else
#if APPLICATION == APP_NTOUCH_MAC
		[NSRunLoop bnr_performOnMainRunLoopWithOrder:0 modes:@[NSRunLoopCommonModes] block:block];
		// Why NSRunLoopCommonModes? It ends up being necessary in order to post events while a modal window
		// is up, specifically via runModalForWindow: which Mac uses while doing the logout/login.  (NSAlerts
		// don't appear to interfere with this?)  Otherwise the blocks posted to the main thread using this
		// method don't get run.
#else
	dispatch_async(dispatch_get_main_queue(), ^{ block(); });
#endif

}

#pragma mark - Helpers: Notification Posting

- (NSSet *)logoutSuppressedNotifications
{
	// Why do we suppress some notifications when we're not authenticated?
	// SCINotificationEmergencyAddressUpdated has a habit of getting posted in cases where
	// we are logging out immediately after logging in, which causes a prompt to update the
	// address even though the user has logged out.  In the case of being kicked out of a
	// ring group, the AddressUpdated notification is sent shortly after SCINotificationRingGroupRemoved,
	// which causes an awkward situation where the user must dismiss the address dialog
	// before they can respond to being removed.
	static dispatch_once_t onceToken;
	__weak SCIVideophoneEngine *w_self = self;
	dispatch_once(&onceToken, ^{
		SCIVideophoneEngine *s_self = w_self;
		s_self->mLogoutSuppressedNotifications = [NSSet setWithObjects:
										  SCINotificationEmergencyAddressUpdated,
										  nil];
	});
	return mLogoutSuppressedNotifications;
}

- (NSSet *)interfaceModeSuppressedNotifications
{
	NSSet *res = nil;
	
	switch (self.interfaceMode) {
		case SCIInterfaceModeStandard:
		case SCIInterfaceModePublic:
		case SCIInterfaceModeKiosk:
		case SCIInterfaceModeInterpreter:
		case SCIInterfaceModeTechSupport:
		case SCIInterfaceModeVRI:
		case SCIInterfaceModeAbusiveCaller:
		case SCIInterfaceModePorted:
			break;
		case SCIInterfaceModeHearing: {
			static NSSet *hearingInterfaceModeSuppressedNotifications = nil;
			static dispatch_once_t onceToken;
			dispatch_once(&onceToken, ^{
				hearingInterfaceModeSuppressedNotifications = [NSSet setWithObjects:SCINotificationEmergencyAddressUpdated, nil];
			});
			res = hearingInterfaceModeSuppressedNotifications;
		} break;
	}
	
	return res;
}

- (void)postOnMainThreadNotificationName:(NSString *)name userInfo:(NSDictionary *)info
{
	if (self.isAuthenticated == NO && [self.logoutSuppressedNotifications containsObject:name])
	{
		SCILog(@"Authentication state suppressed notification: %@", name);
		return;
	}
	if ([self.interfaceModeSuppressedNotifications containsObject:name])
	{
		SCILog(@"Interface mode suppressed notification: %@", name);
		return;
	}

#if APPLICATION == APP_NTOUCH_MAC
	[self performBlockOnMainThread:^{
		[[NSNotificationCenter defaultCenter] postNotificationName:name object:self userInfo:info];
	}];
#else
	dispatch_async(dispatch_get_main_queue(), ^{
		[[NSNotificationCenter defaultCenter] postNotificationName:name object:self userInfo:info];
	});
#endif
	
}

- (void)postConnectedDidChange
{
	if (self.connected) {
		[self postOnMainThreadNotificationName:SCINotificationConnected userInfo:nil];
	} else {
		[self postOnMainThreadNotificationName:SCINotificationConnecting userInfo:nil];
	}
	[self postOnMainThreadNotificationName:SCINotificationConnectedDidChange userInfo:nil];
}

- (void)postAuthenticatedDidChange
{
	[self postOnMainThreadNotificationName:SCINotificationAuthenticatedDidChange userInfo:nil];
}

- (void)postFetchingAuthorizationDidChange
{
	[self postOnMainThreadNotificationName:SCINotificationFetchingAuthorizationDidChange userInfo:nil];
}

- (void)postAuthorizingDidChange
{
	[self postOnMainThreadNotificationName:SCINotificationAuthorizingDidChange userInfo:nil];
}

- (void)postAuthorizedDidChange
{
	[self postOnMainThreadNotificationName:SCINotificationAuthorizedDidChange userInfo:nil];
}

- (void)postCallNotificationName:(NSString *)name call:(SCICall *)call
{
	[self postOnMainThreadNotificationName:name userInfo:[NSDictionary dictionaryWithObjectsAndKeys:call, SCINotificationKeyCall, nil]];
}

- (void)postCallNotificationName:(NSString *)name call:(SCICall *)call userInfo:(NSDictionary *)partialUserInfo
{
	NSDictionary *completeUserInfo = ^{
		NSMutableDictionary *mutableUserInfo = [[NSMutableDictionary alloc] init];
		[mutableUserInfo addEntriesFromDictionary:partialUserInfo];
		mutableUserInfo[SCINotificationKeyCall] = call;
		return [mutableUserInfo copy];
	}();
	[self postOnMainThreadNotificationName:name userInfo:completeUserInfo];
}

#pragma mark - Helpers: Sending Requests: Core

- (void)sendBasicCoreRequest:(CstiCoreRequest *)req timeout:(NSTimeInterval)timeout completion:(void (^)(NSError *error))block
{
	[self sendCoreRequest:req timeout:timeout completion:^(CstiCoreResponse *response, NSError *error) {
		NSError *errorOut = nil;
		
		if (response) {
			errorOut = nil;
		} else {
			errorOut = error;
		}
		
		if (block)
			block(errorOut);
	}];
}
- (void)sendBasicCoreRequest:(CstiCoreRequest *)req completion:(void (^)(NSError *error))block
{
	[self sendBasicCoreRequest:req timeout:SCIVideophoneEngineDefaultTimeout completion:block];
}

- (void)sendCoreRequestPrimitive:(CstiCoreRequest *)req timeout:(NSTimeInterval)timeout completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block
{
	@synchronized (mCoreResponseHandlers) {
		unsigned int requestId = 0;
		stiHResult hResult = mVideophoneEngine->CoreRequestSendEx(req, &requestId);
		if (hResult == stiRESULT_SUCCESS) {
			[self initTimeout:timeout forCoreRequestId:requestId];
			[self handleCoreRequestId:requestId withBlock:^(CstiCoreResponse *response, NSError *sendError) {
				[self abortTimeoutForCoreRequestId:requestId];
				if (block)
					block(response, sendError);
			}];
		} else {
			delete req;
			if (block)
				[self performBlockOnMainThread:^{
					block(NULL, [NSError errorWithDomain:SCIErrorDomainCoreRequestSend code:hResult userInfo:nil]);
				}];
		}
	}
}
- (void)sendCoreRequestPrimitive:(CstiCoreRequest *)req completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block
{
	[self sendCoreRequestPrimitive:req timeout:SCIVideophoneEngineDefaultTimeout completion:block];
}

//-sendComplexCoreRequest:completion: will always pass any CstiCoreResponse
//that was created to the passed-in block.  This is in contrast to the
//behavior of sendCoreRequest:completion: which regards any response for
//which NSErrorFromCstiCoreResponse has a non-nil return as garbage and
//will not pass the response object to the the passed-in block.  This is
//almost always the desired behavior, but occasionally, we need to interrogate
//the response even when it describes an error; in those circumstances, we
//use sendComplexCoreRequest:completion:.
- (void)sendComplexCoreRequest:(CstiCoreRequest *)req timeout:(NSTimeInterval)timeout completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block
{
	[self sendCoreRequestPrimitive:req timeout:timeout completion:^(CstiCoreResponse *response, NSError *sendError) {
		CstiCoreResponse *responseOut = NULL;
		NSError *errorOut = nil;
		
		if (!sendError) {
			//Generally, checking whether an error is nil is an antipattern.  In this case, though,
			//we need to account for the case where the handler block is passed NULL as the response
			//argument from the code where estiMSG_CORE_REQUEST_REMOVED is dealt with; but, we
			//need to account for that _only_ if there was not a "primitive send error"--only if
			//CoreRequestSend() in sendCoreRequestPrimitve:completion: does, in fact, return
			//stiRESULT_SUCCESS.  Essentially, sendCoreRequest:completion: extends the definition of
			//the "sendError" argument passed to the completion block to include the case where the
			//request is removed.
			NSError *error = NSErrorFromCstiCoreResponse(response);
			if (!error) {
				responseOut = response;
				errorOut = nil;
			} else {
				responseOut = response;
				errorOut = error;
			}
		} else {
			responseOut = NULL;
			errorOut = sendError;
		}
		
		if (block)
			block(responseOut, errorOut);
	}];
}
- (void)sendComplexCoreRequest:(CstiCoreRequest *)req completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block
{
	[self sendComplexCoreRequest:req timeout:SCIVideophoneEngineDefaultTimeout completion:block];
}

//Calling code just needs to check whether the response object passed
//into the passed-in block is non-NULL.  In particular, calling code
//is guaranteed that if the response is non-NULL, NSErrorFromCstiCoreResponse
//will return nil for the response.  Additionally if the response is
//NULL, then the block is guaranteed to be passed an error.
- (void)sendCoreRequest:(CstiCoreRequest *)req timeout:(NSTimeInterval)timeout completion:(void (^)(CstiCoreResponse *response, NSError *error))block
{
	[self sendComplexCoreRequest:req timeout:timeout completion:^(CstiCoreResponse *response, NSError *sendError) {
		CstiCoreResponse *responseOut = NULL;
		NSError *errorOut = nil;
		
		if (response) {
			NSError *error = NSErrorFromCstiCoreResponse(response);
			if (!error) {
				responseOut = response;
				errorOut = nil;
			} else {
				responseOut = NULL;
				errorOut = error;
			}
		} else {
			responseOut = NULL;
			errorOut = sendError;
		}
		
		if (block)
			block(responseOut, errorOut);
	}];
}
- (void)sendCoreRequest:(CstiCoreRequest *)req completion:(void (^)(CstiCoreResponse *, NSError *))block
{
	[self sendCoreRequest:req timeout:SCIVideophoneEngineDefaultTimeout completion:block];
}

#pragma mark - Helpers: Sending Requests: Message

- (void)sendBasicMessageRequest:(CstiMessageRequest *)req completion:(void (^)(NSError *error))block
{
	[self sendMessageRequest:req completion:^(CstiMessageResponse *response, NSError *error) {
		NSError *errorOut = nil;
		
		if (response) {
			errorOut = nil;
		} else {
			errorOut = error;
		}
		
		if (block)
			block(errorOut);
	}];
}

- (void)sendMessageRequestPrimitive:(CstiMessageRequest *)req completion:(void (^)(CstiMessageResponse *, NSError *))block
{
	unsigned int requestId = 0;
	stiHResult hResult = mVideophoneEngine->MessageRequestSendEx(req, &requestId);
	if (hResult == stiRESULT_SUCCESS) {
		[self handleMessageRequestId:requestId withBlock:^(CstiMessageResponse *response) {
			if (block)
				block(response, nil);
		}];
	} else {
		delete req;
		if (block)
			[self performBlockOnMainThread:^{
				block(NULL, [NSError errorWithDomain:SCIErrorDomainMessageRequestSend code:hResult userInfo:nil]);
			}];
	}
}

- (void)sendComplexMessageRequest:(CstiMessageRequest *)req completion:(void (^)(CstiMessageResponse *response, NSError *sendError))block
{
	[self sendMessageRequestPrimitive:req completion:^(CstiMessageResponse *response, NSError *sendError) {
		CstiMessageResponse *responseOut = NULL;
		NSError *errorOut = nil;
		
		if (!sendError) {
			//Generally, checking whether an error is nil is an antipattern.  In this case, though,
			//we need to account for the case where the handler block is passed NULL as the response
			//argument from the code where estiMSG_MESSAGE_REQUEST_REMOVED is dealt with; but, we
			//need to account for that _only_ if there was not a "primitive send error"--only if
			//MessageRequestSend() in sendMessageRequestPrimitve:completion: does, in fact, return
			//stiRESULT_SUCCESS.  Essentially, sendCoreRequest:completion: extends the definition of
			//the "sendError" argument passed to the completion block to include the case where the
			//request is removed.
			NSError *error = NSErrorFromCstiMessageResponse(response);
			if (!error) {
				responseOut = response;
				errorOut = nil;
			} else {
				responseOut = NULL;
				errorOut = error;
			}
		} else {
			responseOut = NULL;
			errorOut = sendError;
		}
		
		if (block)
			block(responseOut, errorOut);
	}];
}

- (void)sendMessageRequest:(CstiMessageRequest *)req completion:(void (^)(CstiMessageResponse *response, NSError *error))block
{
	[self sendComplexMessageRequest:req completion:^(CstiMessageResponse *response, NSError *sendError) {
		CstiMessageResponse *responseOut = NULL;
		NSError *errorOut = nil;
		
		if (response) {
			NSError *error = NSErrorFromCstiMessageResponse(response);
			if (error) {
				responseOut = NULL;
				errorOut = error;
			} else {
				responseOut = response;
				errorOut = error;
			}
		} else {
			responseOut = NULL;
			errorOut = sendError;
		}
		
		if (block)
			block(responseOut, errorOut);
	}];
}

#pragma mark - Helpers: Sending Requests: User Settings Core Requests

- (void)fetchUserSetting:(const char *)userSettingName defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *, NSError *))block
{
	[self fetchUserSetting:userSettingName completion:^(SCISettingItem *setting, NSError *error) {
		SCISettingItem *settingOut = nil;
		NSError *errorOut = nil;
		
		if (setting) {
			settingOut = setting;
			errorOut = nil;
		} else {
			if (defaultSetting) {
				NSString *errorDomain = error.domain;
				if ([errorDomain isEqualToString:SCIErrorDomainVideophoneEngine]) {
					SCIVideophoneEngineErrorCode code = (SCIVideophoneEngineErrorCode)error.code;
					if (code == SCIVideophoneEngineErrorCodeEmptyUserSettingsList ||
						code == SCIVideophoneEngineErrorCodeNullUserSettingsList) {
						settingOut = defaultSetting;
						errorOut = nil;
					} else {
						settingOut = nil;
						errorOut = error;
					}
				} else {
					settingOut = nil;
					errorOut = error;
				}
			} else {
				settingOut = nil;
				errorOut = error;
			}
		}
		
		if (block)
			block(settingOut, errorOut);
	}];
}

- (void)fetchUserSetting:(const char *)userSettingName completion:(void (^)(SCISettingItem *setting, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->userSettingsGet(userSettingName);
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		SCISettingItem *itemOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto settingList = response->SettingListGet();
			if (!settingList.empty()) {
				NSArray *array = [NSArray arrayWithCstiSettingList:settingList];
				if (array.count > 0) {
					itemOut = array[0];
					errorOut = nil;
				} else {
					itemOut = nil;
					errorOut = ^ {
						NSString *localizedDesc = @"Successfully fetched list of user settings, but the list was empty.";
						NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
						return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEmptyUserSettingsList userInfo:userInfo];
					}();
				}
			} else {
				itemOut = nil;
				errorOut = ^ {
					NSString *localizedDesc = @"Received non-error response for the UserSettingsGet request, but the response did not contain a list of user settings.";
					NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
					return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNullUserSettingsList userInfo:userInfo];
				}();
			}
		} else {
			itemOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(itemOut, errorOut);
	}];
}

- (void)fetchSettings:(NSArray *)settingsToFetch completion:(void (^)(NSArray *settingList, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	
	for (NSString *propertyName in settingsToFetch)
	{
		switch (SCIPropertyScopeForPropertyName(propertyName))
		{
			case SCIPropertyScopePhone:
			{
				coreRequest->phoneSettingsGet(propertyName.UTF8String);

				break;
			}
				
			case SCIPropertyScopeUser:
			{
				coreRequest->userSettingsGet(propertyName.UTF8String);
				break;
			}
				
			case SCIPropertyScopeUnknown:
			{
				break;
			}

		}
	}
	
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		NSArray *itemOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto settingList = response->SettingListGet();
			if (!settingList.empty()) {
				NSArray *array = [NSArray arrayWithCstiSettingList:settingList];
				if (array.count > 0) {
					itemOut = array;
					errorOut = nil;
				} else {
					itemOut = nil;
					errorOut = ^ {
						NSString *localizedDesc = @"Successfully fetched list of user settings, but the list was empty.";
						NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
						return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEmptyUserSettingsList userInfo:userInfo];
					}();
				}
			} else {
				itemOut = nil;
				errorOut = ^ {
					NSString *localizedDesc = @"Received non-error response for the UserSettingsGet request, but the response did not contain a list of user settings.";
					NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
					return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNullUserSettingsList userInfo:userInfo];
				}();
			}
		} else {
			itemOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(itemOut, errorOut);
	}];
}

- (void)setUserSetting:(SCISettingItem *)userSettingItem completion:(void (^)(NSError *error))block
{
	[self setUserSetting:userSettingItem
		   preStoreBlock:nil
		  postStoreBlock:nil
			  completion:block];
}


- (void)setUserSetting:(SCISettingItem *)userSettingItem
		 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			   timeout:(NSTimeInterval)timeout
			completion:(void (^)(NSError *error))completionBlock
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	const char * userSettingName = userSettingItem.name.UTF8String;
	switch (userSettingItem.type) {
		case SCISettingItemTypeBool: {
			NSNumber *boolValueNumber = userSettingItem.value;
			EstiBool boolValue = (boolValueNumber.boolValue) ? estiTRUE : estiFALSE;
			coreRequest->userSettingsSet(userSettingName, boolValue);
		} break;
		case SCISettingItemTypeEnum: {
			NSNumber *enumValueNumber = userSettingItem.value;
			int enumValue = enumValueNumber.intValue;
			coreRequest->userSettingsSet(userSettingName, enumValue);
		} break;
		case SCISettingItemTypeInt: {
			NSNumber *intValueNumber = userSettingItem.value;
			int intValue = intValueNumber.intValue;
			coreRequest->userSettingsSet(userSettingName, intValue);
		} break;
		case SCISettingItemTypeString: {
			NSString *stringValue = userSettingItem.value;
			const char *pszStringValue = stringValue.UTF8String;
			coreRequest->userSettingsSet(userSettingName, pszStringValue);
		} break;
	}
	[self sendCoreRequest:coreRequest timeout:timeout completion:^(CstiCoreResponse *response, NSError *error) {
		NSError *errorOut = nil;
		
		if (response) {
			[[SCIPropertyManager sharedManager] setValueFromSettingItem:userSettingItem
													  inStorageLocation:SCIPropertyStorageLocationPersistent
														  preStoreBlock:preStoreBlock
														 postStoreBlock:postStoreBlock];
			
			errorOut = nil;
		} else {
			errorOut = error;
		}
		
		if (completionBlock)
			completionBlock(errorOut);
	}];
}
- (void)setUserSetting:(SCISettingItem *)userSettingItem
		 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			completion:(void (^)(NSError *error))completionBlock
{
	[self setUserSetting:userSettingItem
		   preStoreBlock:preStoreBlock
		  postStoreBlock:postStoreBlock
				 timeout:SCIVideophoneEngineDefaultTimeout
			  completion:completionBlock];
}

#pragma mark - Helpers: Sending Requests: Phone Settings Core Requests

- (void)fetchPhoneSetting:(const char *)phoneSettingName defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *setting, NSError *error))block
{
	[self fetchPhoneSetting:phoneSettingName completion:^(SCISettingItem *setting, NSError *error) {
		SCISettingItem *settingOut = nil;
		NSError *errorOut = nil;
		
		if (setting) {
			settingOut = settingOut;
			errorOut = nil;
		} else {
			if (defaultSetting) {
				NSString *errorDomain = error.domain;
				if ([errorDomain isEqualToString:SCIErrorDomainVideophoneEngine]) {
					SCIVideophoneEngineErrorCode code = (SCIVideophoneEngineErrorCode)error.code;
					if (code == SCIVideophoneEngineErrorCodeEmptyPhoneSettingsList ||
						code == SCIVideophoneEngineErrorCodeNullPhoneSettingsList) {
						settingOut = defaultSetting;
						errorOut = nil;
					} else {
						settingOut = nil;
						errorOut = error;
					}
				} else {
					settingOut = nil;
					errorOut = error;
				}
			} else {
				settingOut = nil;
				errorOut = error;
			}
		}
		
		if (block)
			block(settingOut, errorOut);
	}];
}

- (void)fetchPhoneSetting:(const char *)phoneSettingName completion:(void (^)(SCISettingItem *setting, NSError *error))block
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	coreRequest->phoneSettingsGet(phoneSettingName);
	[self sendCoreRequest:coreRequest completion:^(CstiCoreResponse *response, NSError *error) {
		SCISettingItem *itemOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			auto settingList = response->SettingListGet();
			if (!settingList.empty()) {
				NSArray *array = [NSArray arrayWithCstiSettingList:settingList];
				if (array.count > 0) {
					itemOut = array[0];
					errorOut = nil;
				} else {
					itemOut = nil;
					errorOut = ^ {
						NSString *localizedDesc = @"Successfully fetched list of user settings from UserSettingsGetResponse, but the list was empty.";
						NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
						return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeEmptyPhoneSettingsList userInfo:userInfo];
					}();
				}
			} else {
				itemOut = nil;
				errorOut = ^ {
					NSString *localizedDesc = @"Received non-error response for the UserSettingsGet request, but the response did not contain a list of user settings.";
					NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:localizedDesc, NSLocalizedDescriptionKey, nil];
					return [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:SCIVideophoneEngineErrorCodeNullPhoneSettingsList userInfo:userInfo];
				}();
			}
		} else {
			itemOut = nil;
			errorOut = error;
		}
		
		if (block)
			block(itemOut, errorOut);
	}];
}

- (void)setPhoneSetting:(SCISettingItem *)phoneSettingItem
		  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
				timeout:(NSTimeInterval)timeout
			 completion:(void (^)(NSError *error))completionBlock
{
	CstiCoreRequest *coreRequest = new CstiCoreRequest();
	const char *pszPhoneSettingName = phoneSettingItem.name.UTF8String;
	switch (phoneSettingItem.type) {
		case SCISettingItemTypeBool: {
			NSNumber *boolValueNumber = phoneSettingItem.value;
			EstiBool boolValue = (boolValueNumber.boolValue) ? estiTRUE : estiFALSE;
			coreRequest->phoneSettingsSet(pszPhoneSettingName, boolValue);
		} break;
		case SCISettingItemTypeEnum: {
			NSNumber *enumValueNumber = phoneSettingItem.value;
			int enumValue = enumValueNumber.intValue;
			coreRequest->phoneSettingsSet(pszPhoneSettingName, enumValue);
		} break;
		case SCISettingItemTypeInt: {
			NSNumber *intValueNumber = phoneSettingItem.value;
			int intValue = intValueNumber.intValue;
			coreRequest->phoneSettingsSet(pszPhoneSettingName, intValue);
		} break;
		case SCISettingItemTypeString: {
			NSString *stringValue = phoneSettingItem.value;
			const char *pszStringValue = stringValue.UTF8String;
			coreRequest->phoneSettingsSet(pszPhoneSettingName, pszStringValue);
		} break;
	}
	[self sendCoreRequest:coreRequest timeout:timeout completion:^(CstiCoreResponse *response, NSError *error) {
		NSError *errorOut = nil;
		
		if (response) {
			[[SCIPropertyManager sharedManager] setValueFromSettingItem:phoneSettingItem
													  inStorageLocation:SCIPropertyStorageLocationPersistent
														  preStoreBlock:preStoreBlock
														 postStoreBlock:postStoreBlock];
			
			errorOut = nil;
		} else {
			errorOut = error;
		}
		
		if (completionBlock)
			completionBlock(errorOut);
	}];
}
- (void)setPhoneSetting:(SCISettingItem *)phoneSettingItem
		  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			 completion:(void (^)(NSError *error))completionBlock
{
	[self setPhoneSetting:phoneSettingItem
			preStoreBlock:preStoreBlock
		   postStoreBlock:postStoreBlock
				  timeout:SCIVideophoneEngineDefaultTimeout
			   completion:completionBlock];
}
- (void)setPhoneSetting:(SCISettingItem *)phoneSettingItem completion:(void (^)(NSError *error))block
{
	[self setPhoneSetting:phoneSettingItem
			preStoreBlock:nil
		   postStoreBlock:nil
			   completion:block];
}

#pragma mark - Helpers: Sending Requests: Settings Core Requests

- (void)fetchSetting:(const char *)setting ofScope:(SCIPropertyScope)scope defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *setting, NSError *error))block
{
	switch (scope) {
		case SCIPropertyScopeUser: {
			[self fetchUserSetting:setting defaultSetting:defaultSetting completion:block];
		} break;
		case SCIPropertyScopePhone: {
			[self fetchPhoneSetting:setting defaultSetting:defaultSetting completion:block];
		} break;
		case SCIPropertyScopeUnknown: {
#ifdef stiDEBUG
			[NSException raise:@"Passed unknown setting scope." format:@"Called %s but passed SCIPropertyScopeUnknown.", __PRETTY_FUNCTION__];
#else
			SCILog(@"Called %s with illegal scope SCIPropertyScopeUnkown\n%@.", __PRETTY_FUNCTION__, [NSThread callStackSymbols]);
#endif
		} break;
	}
}

- (void)fetchSetting:(const char *)setting ofScope:(SCIPropertyScope)scope completion:(void (^)(SCISettingItem *setting, NSError *error))block
{
	switch (scope) {
		case SCIPropertyScopeUser: {
			[self fetchUserSetting:setting completion:block];
		} break;
		case SCIPropertyScopePhone: {
			[self fetchPhoneSetting:setting completion:block];
		} break;
		case SCIPropertyScopeUnknown: {
#ifdef stiDEBUG
			[NSException raise:@"Passed unknown setting scope." format:@"Called %s but passed SCIPropertyScopeUnknown.", __PRETTY_FUNCTION__];
#else
			SCILog(@"Called %s with illegal scope SCIPropertyScopeUnkown\n%@.", __PRETTY_FUNCTION__, [NSThread callStackSymbols]);
#endif
		} break;
	}
}

- (void)setSetting:(SCISettingItem *)settingItem
		   ofScope:(SCIPropertyScope)scope
	 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
	postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
		   timeout:(NSTimeInterval)timeout
		completion:(void (^)(NSError *error))completionBlock
{
	switch (scope) {
		case SCIPropertyScopeUser: {
			[self setUserSetting:settingItem
				   preStoreBlock:preStoreBlock
				  postStoreBlock:postStoreBlock
						 timeout:timeout
					  completion:completionBlock];
		} break;
		case SCIPropertyScopePhone: {
			[self setPhoneSetting:settingItem
					preStoreBlock:preStoreBlock
				   postStoreBlock:postStoreBlock
						  timeout:timeout
					   completion:completionBlock];
		} break;
		case SCIPropertyScopeUnknown: {
#ifdef stiDEBUG
			[NSException raise:@"Passed unknown setting scope." format:@"Called %s but passed SCIPropertyScopeUnknown.", __PRETTY_FUNCTION__];
#else
			SCILog(@"Called %s with illegal scope SCIPropertyScopeUnkown\n%@.", __PRETTY_FUNCTION__, [NSThread callStackSymbols]);
#endif
		} break;
	}
}
- (void)setSetting:(SCISettingItem *)settingItem
		   ofScope:(SCIPropertyScope)scope
	 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
	postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
		completion:(void (^)(NSError *error))completionBlock
{
	[self setSetting:settingItem
			 ofScope:scope
	   preStoreBlock:preStoreBlock
	  postStoreBlock:postStoreBlock
			 timeout:SCIVideophoneEngineDefaultTimeout
		  completion:completionBlock];
}
- (void)setSetting:(SCISettingItem *)settingItem ofScope:(SCIPropertyScope)scope completion:(void (^)(NSError *error))block
{
	[self setSetting:settingItem
			 ofScope:scope
	   preStoreBlock:nil
	  postStoreBlock:nil
		  completion:block];
}

#pragma mark - Property Changes

void PropertyChangeCallbackStub(const char *pszProperty, void *pCallbackParam);

NSNotificationName const SCINotificationPropertyChanged = @"SCINotificationPropertyChanged";
NSNotificationName const SCINotificationPropertyReceived = @"SCINotificationPropertyReceived";
NSString * const SCINotificationKeyName = @"name";

- (void)requestPropertyChangedNotificationsForProperty:(const char *)propertyName
{
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	pPM->registerPropertyChangeNotifyFunc(propertyName, PropertyChangeCallbackStub, (__bridge void *)self);
}

- (void)handlePropertyManagerChange:(const char *)propertyName
{
	if (strcmp(propertyName, PropertyDef::CallerIdNumber) == 0) {
		[self loadUserAccountInfo];
	}
	
	if (propertyName) {
		NSString *propertyNameString = [NSString stringWithUTF8String:propertyName];
		if (propertyNameString) {
			[self performBlockOnMainThread:^{
				[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationPropertyChanged
																	object:self
																  userInfo:@{ SCINotificationKeyName : propertyNameString }];
			}];
		}
	}
}

#pragma mark - Call Management

- (SCICall *)callForIstiCall:(IstiCall *)istiCall
{
	SCICall *call = nil;
	@synchronized(mActiveCalls)
	{
		for (SCICall *c in mActiveCalls)
		{
			if (c.istiCall == istiCall)
			{
				call = c;
				break;
			}
		}
		if (!call)
		{
			call = [SCICall callWithIstiCall:istiCall];
			[mActiveCalls addObject:call];
		}
	}
	return call;
}

- (void)forgetCall:(SCICall *)call
{
	@synchronized(mActiveCalls)
	{
		[mActiveCalls removeObject:call];
	}
}

- (BOOL)joinCall:(SCICall *)callOne withCall:(SCICall *)callTwo
{
	stiHResult result = self.videophoneEngine->GroupVideoChatJoin(callOne.istiCall, callTwo.istiCall);
	return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)joinCallAllowedForCall:(SCICall *)callOne withCall:(SCICall *)callTwo
{
	if (callOne != nil && callTwo != nil) {
		return (self.videophoneEngine->GroupVideoChatAllowed(callOne.istiCall, callTwo.istiCall) == true) ? YES : NO;
	} else {
		return NO;
	}
}

- (BOOL)createDHVIConference:(SCICall *)call
{
	if (call != nil) {
		return (self.videophoneEngine->dhviCreate(call.istiCall) == stiRESULT_SUCCESS) ? YES : NO;
	} else {
		return NO;
	}
}

#pragma mark - Helpers: Core Request Timeout
//-sendCoreRequestPrimitive:timeout:completion: kicks off the process of
//	timing out Core requests by calling initTimeout:forCoreRequestId:.
//	When the block to be called upon completion or failure of the Core
//	request is triggered, it calls -abortTimeoutForCoreRequestId:.  All
//	the other components of the timeout process are triggered by these
//	two method calls.  -initTimeout:forCoreRequestId: adds an entry to
//	the mCoreRequestTimeouts dictionary (the timeIntervalSince1970 at
//	which the request should timeout is set as the object for the request
//	id key) and setupTimeoutTimer is called.  -setupTimeoutTimer sets up
//	the timeout timer on the main thread.  When the timer fires, each of
//	the requestIds for the requests which have timed out are passed to
//	doTimeoutForCoreRequestId: which queues the completion block for the
//	request to be performed on the main thread.  When the completion block
//	is called, it calls -abortTimeoutForCoreRequestId:.

- (void)setupTimeoutTimer
{
	[self performBlockOnMainThread:^{
		[self doSetupTimeoutTimer];
	}];
}

- (void)doSetupTimeoutTimer
{
	if (!mTimeoutTimer) {
		//TODO:	Using a constant interval here is less than ideal for
		//		conserving power.  It would be best to set the interval
		//		to be a little bit more than the earliest timeout.
		mTimeoutTimer = [NSTimer scheduledTimerWithTimeInterval:SCIVideophoneEngineTimeoutTimerTickTime
														 target:self
													   selector:@selector(timeoutTimerTick:)
													   userInfo:nil
														repeats:YES];
	}
}

- (void)timeoutTimerTick:(NSTimer *)timer
{
	NSArray *timedOutCoreRequestIds = [self timedOutCoreRequestIds];
	for (NSNumber *timedOutCoreRequestIdNumber in timedOutCoreRequestIds) {
		unsigned int requestId = [timedOutCoreRequestIdNumber unsignedIntValue];
		[self doTimeoutForCoreRequestId:requestId];
	}
}

- (void)doTeardownTimeoutTimer
{
	if (mTimeoutTimer) {
		[mTimeoutTimer invalidate];
		mTimeoutTimer = nil;
	}
}

- (void)teardownTimeoutTimer
{
	[self performBlockOnMainThread:^{
		[self doTeardownTimeoutTimer];
	}];
}

- (void)initTimeout:(NSTimeInterval)timeout forCoreRequestId:(unsigned int)requestId
{
	if (timeout > SCIVideophoneEngineTimeoutNone) {
		NSTimeInterval now = [[NSDate date] timeIntervalSince1970];
		[self addTimeout:(now + timeout) forCoreRequestId:requestId];
		[self setupTimeoutTimer];
	}
}

- (void)doTimeoutForCoreRequestId:(unsigned int)requestId
{
	void (^handler)(CstiCoreResponse *, NSError *) = [self popHandlerForCoreRequestId:requestId];
	
	if (handler) {
		[self performBlockOnMainThread:^{
			NSError *error = [NSError errorWithDomain:SCIErrorDomainCoreRequestSend
												 code:SCICoreRequestSendErrorCodeRequestTimedOut
											 userInfo:nil];
			handler(NULL, error);
		}];
	} else {
		SCILog(@"Timed out Core request with id %u but did not have a handler.", requestId);
	}
}

- (void)abortTimeoutForCoreRequestId:(unsigned int)requestId
{
	NSUInteger remainingTimeouts = 0;
	__unused NSTimeInterval timeout = [self removeTimeoutForCoreRequestId:requestId timeoutsRemaining:&remainingTimeouts];
	if (remainingTimeouts == 0) {
		[self teardownTimeoutTimer];
	}
}

- (NSArray *)timedOutCoreRequestIds
{
	NSArray *res = nil;
	
	@synchronized(mCoreRequestTimeouts) {
		NSMutableArray *mutableRes = [[NSMutableArray alloc] init];
		
		NSTimeInterval now = [[NSDate date] timeIntervalSince1970];
		
		[mCoreRequestTimeouts enumerateKeysAndObjectsUsingBlock:^(NSNumber *requestIdNumber, NSNumber *timeoutNumber, BOOL *stop) {
			NSTimeInterval timeout = [timeoutNumber doubleValue];
			if (now >= timeout) {
				[mutableRes addObject:requestIdNumber];
			}
		}];
		
		res = [mutableRes copy];
	}
	
	return res;
}

- (void)addTimeout:(NSTimeInterval)timeout forCoreRequestId:(unsigned int)requestId
{
	@synchronized(mCoreRequestTimeouts) {
		mCoreRequestTimeouts[@(requestId)] = @(timeout);
	}
}

- (NSTimeInterval)removeTimeoutForCoreRequestId:(unsigned int)requestId timeoutsRemaining:(NSUInteger *)remainingOut
{
	NSTimeInterval res = DBL_MAX;
	NSUInteger remaining = 0;
	
	@synchronized(mCoreRequestTimeouts) {
		NSNumber *key = @(requestId);
		res = [mCoreRequestTimeouts[key] doubleValue];
		[mCoreRequestTimeouts removeObjectForKey:key];
	}
	
	if (remainingOut) {
		*remainingOut = remaining;
	}
	return res;
}

#pragma mark - Helpers: Handler Blocks

- (void)handleCoreRequestId:(unsigned int)requestId withBlock:(void (^)(CstiCoreResponse *, NSError *))handler
{
	@synchronized(mCoreResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		[mCoreResponseHandlers setObject:[handler copy] forKey:key];
	}
}

- (void (^)(CstiCoreResponse *, NSError *))popHandlerForCoreRequestId:(unsigned int)requestId
{
	@synchronized(mCoreResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		void (^handler)(CstiCoreResponse *, NSError *) = [mCoreResponseHandlers objectForKey:key];
		[mCoreResponseHandlers removeObjectForKey:key];
		return handler;
	}
}

- (void)handleMessageRequestId:(unsigned int)requestId withBlock:(void (^)(CstiMessageResponse *))handler
{
	@synchronized(mMessageResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		[mMessageResponseHandlers setObject:[handler copy] forKey:key];
	}
}

- (void (^)(CstiMessageResponse *))popHandlerForMessageRequestId:(unsigned int)requestId
{
	@synchronized(mMessageResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		void (^handler)(CstiMessageResponse *) = [mMessageResponseHandlers objectForKey:key];
		[mMessageResponseHandlers removeObjectForKey:key];
		return handler;
	}
}

- (void)handleRingGroupManagerResponse:(unsigned int)requestId withBlock:(void (^)(IstiRingGroupMgrResponse *response))handler
{
	@synchronized(mRingGroupManagerResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		[mRingGroupManagerResponseHandlers setObject:[handler copy] forKey:key];
	}
}

- (void (^)(IstiRingGroupMgrResponse *))popHandlerForRingGroupManagerResponse:(unsigned int)requestId
{
	@synchronized(mRingGroupManagerResponseHandlers)
	{
		NSNumber *key = [NSNumber numberWithUnsignedInt:requestId];
		void (^handler)(IstiRingGroupMgrResponse *) = [mRingGroupManagerResponseHandlers objectForKey:key];
		[mRingGroupManagerResponseHandlers removeObjectForKey:key];
		return handler;
	}
}

- (void)dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)response
{
	unsigned int requestId = response->RequestIdGet();
	void (^handler)(IstiRingGroupMgrResponse *) = [self popHandlerForRingGroupManagerResponse:requestId];
	[self performBlockOnMainThread:^{
		if (handler)
			handler(response);
		delete response;
	}];
}

#pragma mark - Callbacks

- (stiHResult)engineCallbackWithMessage:(int32_t)message param:(size_t)param
{
	switch (message)
	{
		case estiMSG_CB_SIP_REGISTRATION_CONFIRMED:
		{
			std::string *serverIP = (std::string *)param;
			
			// Check for NAT64 IPv6 address and convert to IPv4 before sending to UI layer
			std::string mappedServerIP = "";
			stiHResult hResult = stiMapIPv6AddressToIPv4(*serverIP, &mappedServerIP);
			
			std::string serverIPv6;
			if (!stiIS_ERROR (hResult) && !mappedServerIP.empty ())
			{
				serverIPv6 = *serverIP;
				*serverIP = mappedServerIP;
			}
			
			NSMutableDictionary *userInfo = [NSMutableDictionary dictionary];
			userInfo[SCINotificationKeySIPServerIP] = [NSString stringWithUTF8String:serverIP->c_str()];
			if (serverIPv6.empty())
			{
				userInfo[SCINotificationKeySIPServerIPv6] = [NSString stringWithUTF8String:serverIPv6.c_str()];
			}

			self.isSipRegistered = YES;

			[self postOnMainThreadNotificationName:SCINotificationSIPRegistrationConfirmed userInfo:[userInfo copy]];
			self.videophoneEngine->MessageCleanup(message, param);
			break;
		}
		case estiMSG_CONFERENCING_READY_STATE:
		{
			EstiConferencingReadyState readyState = (EstiConferencingReadyState)param;
			[self postOnMainThreadNotificationName:SCINotificationConferencingReadyStateChanged
										  userInfo:@{ SCINotificationKeyConferencingReadyState : [NSNumber numberWithInt:readyState] }];
			break;
		}
		case estiMSG_CONTACT_LIST_CHANGED:
		case estiMSG_CONTACT_LIST_DELETED:
			NSLog(@"Message 0x%02x: Contact list changed", message);
			[self performBlockOnMainThread:^{
				[[SCIContactManager sharedManager] refresh];
			}];
			break;
		case estiMSG_LDAP_DIRECTORY_CONTACT_LIST_CHANGED:
			NSLog(@"Message 0x%02x: LDAP Directory Contact list changed", message);
			[self performBlockOnMainThread:^{
				[[SCIContactManager sharedManager] refreshLDAPDirectoryContacts];
				[[SCIContactManager sharedManager] refresh];
			}];
			break;
		case estiMSG_LDAP_DIRECTORY_VALID_CREDENTIALS:
			NSLog(@"Message 0x%02x: LDAP valid credentials", message);
			[self postOnMainThreadNotificationName:SCINotificationLDAPCredentialsValid userInfo:nil];
			break;
		case estiMSG_LDAP_DIRECTORY_INVALID_CREDENTIALS:
			NSLog(@"Message 0x%02x: LDAP invalid credentials", message);
#ifdef stiLDAP_CONTACT_LIST
			[self postOnMainThreadNotificationName:SCINotificationLDAPCredentialsInvalid userInfo:nil];
			[self clearLDAPCredentialsAndContacts];
#endif
			break;
		case estiMSG_LDAP_DIRECTORY_SERVER_UNAVAILABLE:
			NSLog(@"Message 0x%02x: LDAP server unavailable", message);
#ifdef stiLDAP_CONTACT_LIST
			[self postOnMainThreadNotificationName:SCINotificationLDAPServerUnavailable userInfo:nil];
			[self clearLDAPCredentialsAndContacts];
#endif
			break;
		case estiMSG_LDAP_DIRECTORY_ERROR:
			NSLog(@"Message 0x%02x: LDAP directory error", message);
#ifdef stiLDAP_CONTACT_LIST
			[self postOnMainThreadNotificationName:SCINotificationLDAPError userInfo:nil];
#endif
			break;
		case estiMSG_CONTACT_LIST_ITEM_ADDED:
		case estiMSG_CONTACT_LIST_ITEM_EDITED:
		case estiMSG_CONTACT_LIST_ITEM_DELETED:
		{
			NSLog(@"Message 0x%02x: Contact list item added or removed", message);
			CstiItemId *itemId = (CstiItemId *)param;
			IstiContactManager *manager = mVideophoneEngine->ContactManagerGet();
			IstiContactSharedPtr contact = NULL;
			EstiBool found = manager->ContactByIDGet(*itemId, &contact);
			[self performBlockOnMainThread:^{
				if (found == estiTRUE)
				{
					if (message == estiMSG_CONTACT_LIST_ITEM_ADDED)
						[[SCIContactManager sharedManager] refreshWithAddedIstiContact:contact];
					else if (message == estiMSG_CONTACT_LIST_ITEM_EDITED)
						[[SCIContactManager sharedManager] refreshWithEditedIstiContact:contact];
					else
						[[SCIContactManager sharedManager] refreshWithRemoveItemId:*itemId];
				}
				else if (message == estiMSG_CONTACT_LIST_ITEM_DELETED)
				{
					[[SCIContactManager sharedManager] refreshWithRemoveItemId:*itemId];
				}
				else
				{
					// Just refresh if it couldn't be found:
					[[SCIContactManager sharedManager] refresh];
				}
			}];
			break;
		}
		case estiMSG_CONTACT_LIST_IMPORT_COMPLETE:
		{
			NSLog(@"Message 0x%02x: Contact list import complete", message);
			NSUInteger addedCount = (NSUInteger)param;
			[self postOnMainThreadNotificationName:SCINotificationContactListImportComplete userInfo:[NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInteger:addedCount] forKey:SCINotificationKeyCount]];
			[[SCIContactManager sharedManager] reload];
			break;
		}
		case estiMSG_AUTO_SPEED_SETTINGS_CHANGED:
		{
			NSLog(@"Message 0x%02x: Auto Speed Settings Changed", message);

			[self postOnMainThreadNotificationName:SCINotificationAutoSpeedSettingChanged userInfo:nil];
			break;
		}
		case estiMSG_MAX_SPEEDS_CHANGED:
		{
			NSLog(@"Message 0x%02x: Max Speeds Changed", message);
			
			[self postOnMainThreadNotificationName:SCINotificationMaxSpeedsChanged userInfo:nil];
			break;
		}
		case estiMSG_BLOCK_LIST_ITEM_ADDED:
		case estiMSG_BLOCK_LIST_ITEM_DELETED:
		{
			NSLog(@"Message 0x%02x: Blocked list item added or removed", message);
			//TODO :mirror behavior of SCIVPE with respect to SCIContactManager: "refreshWith{Added,Removed}ContactWithCstiItemId"
			__unused CstiItemId *itemId = (CstiItemId *)param;
			[self performBlockOnMainThread:^{
				[[SCIBlockedManager sharedManager] refresh];
			}];
			
			break;
		}
		case estiMSG_BLOCK_LIST_ITEM_EDITED:
		case estiMSG_BLOCK_LIST_CHANGED:
		{
			NSLog(@"Message 0x%02x: Blocked list changed", message);
			[self performBlockOnMainThread:^{
				[[SCIBlockedManager sharedManager] refresh];
			}];
			break;
		}
		case estiMSG_CALL_LIST_CHANGED:
		{
			NSLog(@"Message 0x%02x: Call List Changed", message);
			CstiCallList::EType eType = (CstiCallList::EType)param;
			SCICallType type = SCICallTypeFromCstiCallListType(eType);
			NSDictionary *userInfo = @{@"SCICallListType" : [NSNumber numberWithUnsignedInteger:type]};
			[self postOnMainThreadNotificationName:SCINotificationCallListChanged userInfo:userInfo];
			break;
		}
		case estiMSG_CORE_RESPONSE:
		{
			CstiCoreResponse* response = (CstiCoreResponse*)param;
			unsigned int requestId = response->RequestIDGet();
			CstiCoreResponse::EResponse responseCode = response->ResponseGet();
			CstiCoreResponse::ECoreError errorCode = response->ErrorGet();
			
			void (^handler)(CstiCoreResponse *, NSError *) = [self popHandlerForCoreRequestId:requestId];
			
			if (response->ResponseResultGet() == estiERROR && response->ErrorGet () == CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED) {
				switch (responseCode) {
					case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT: {
						NSError *error = [NSError errorWithDomain:SCIErrorDomainCoreResponse
															 code:SCICoreResponseErrorCodeOfflineAuthentication
														 userInfo:nil];
						if (self.loginBlock && !self.processingLogin) {
							[NSRunLoop bnr_performBlockOnMainRunLoop:^{
								self.processingLogin = YES;
								self.loginBlock(nil, @{ SCIErrorKeyLogin : error }, nil, nil);
								self.loginBlock = nil;
								self.processingLogin = NO;
							}];
						} else {
							[self logoutWithBlock:^{
								self.userAccountInfo = nil;
							}];
						}
					} break;
					case CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT: {
						break;
					}
					default: {
						[self postOnMainThreadNotificationName:SCINotificationCoreOfflineGenericError userInfo:nil];
					} break;
				}
			
				if (handler)
				{
					[self performBlockOnMainThread:^{
						handler(response, nil);
						response->destroy ();
					}];
				}
				else {
					response->destroy ();
				}
			}
			else if (handler)
			{
				[self performBlockOnMainThread:^{
					handler(response, nil);
					response->destroy ();
				}];
			}
			else
			{
				//Generally, SCIVideophoneEngine itself sends requests to core and then awaits responses which it handles
				//using with -popHandlerForCoreResponse:, executing the block that was passed into -handleCoreResponse:withBlock:
				//when the request was sent.  However, eUSER_ACCOUNT_INFO_GET_RESULT will occasionally be received without
				//the corresponding request having been sent to core by SCIVideophoneEngine.  In particular, when the account's
				//phone number is changed in VDM, the endpoint receives the CstiStateNotifyResponse::eUSER_ACCOUNT_INFO_SETTING_CHANGED
				//CstiStateNotifyService does not make the callback which would be received by SCIVideophoneEngine (so that it might
				//then send the userAccountInfoGet request to core--see the subcase for CstiStateNotifyResponse::eUSER_ACCOUNT_INFO_SETTING_CHANGED
				//within the case for estiMSG_STATE_NOTIFY_RESPONSE in -[SCIVideophoneEngine engineCallbackWithMessage:param:])
				//but instead itself issues the request userAccountInfoGet to core.  Subsequently, SCIVideophoneEngine receives
				//the core response CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT but has no handler for it since no handling
				//block had been pushed via -handleCoreResponse:withBlock:.  Consequently, we have to handle the core response
				//"manually", calling the method containing the code (which previously had been the body of the block passed
				//from -loadUserAccountInfo to -handleCoreResponse:withBlock: but has now been extracted into its own method)
				//for handling the reception of CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT.
				switch (responseCode) {
					case CstiCoreResponse::ePUBLIC_IP_GET_RESULT: {
						NSLog(@"Send Public IP to Core");
						WillowPM::PropertyManager *pPM = WillowPM::PropertyManager::getInstance ();
						// send public ip address as hostname
						std::string hostName = response->ResponseStringGet (0);
						pPM->setPropertyString(NetHostname, hostName.c_str(), WillowPM::PropertyManager::Persistent);
						pPM->PropertySend(NetHostname, estiPTypePhone);
						
						response->destroy ();
						break;
					}
					case CstiCoreResponse::eCLIENT_LOGOUT_RESULT: {
						self.loggingOut = NO;
						self.authenticated = NO;
						break;
					}
					case CstiCoreResponse::eVERSION_GET_RESULT: {
						NSLog(@"Message 0x%02x: Core Response not expected: 0x%x (%@), error: 0x%x (%@)", message, responseCode, NSStringFromCstiCoreResponseCode(responseCode), (unsigned)errorCode, NSStringFromECoreError(errorCode));
						if (errorCode == CstiCoreResponse::eNO_ERROR) {
							[self handleVersionGetResultWithResponse:response];
							response->destroy ();
						}
						break;
					}
					case CstiCoreResponse::eUSER_SETTINGS_GET_RESULT: {
						NSLog(@"Message 0x%02x: Core Response 0x%x: User Settings Get Result", message, responseCode);
						if (@available(macOS 10.10, *)) {
							if (response->ResponseResultGet() == estiOK) {
								[self performBlockOnMainThread:^{
									if (response) {
										auto settingList = response->SettingListGet();
										if (!settingList.empty()) {
											NSArray *settingArray = [NSArray arrayWithCstiSettingList:settingList];
											if (settingArray.count > 0) {
												for (id item in settingArray) {
													SCISettingItem *settingitem = (SCISettingItem *)item;
													// find the item with SCIPropertyNameDoNotDisturbEnabled
													if ([settingitem.name isEqualToString:SCIPropertyNameDoNotDisturbEnabled]) {
														[self performBlockOnMainThread:^{
															NSDictionary *userInfo = @{SCINotificationKeyName : SCIPropertyNameDoNotDisturbEnabled, @"SCISettingItem" : settingitem};
															[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationPropertyReceived
																												object:self
																											  userInfo:userInfo];
														}];
														break;
													}
												}
											}
										}
									}
									response->destroy ();
								}];
							}
						} else {
							response->destroy ();
						}
						break;
					}
					default: {
						NSLog(@"Message 0x%02x: Core Response not expected: 0x%x (%@), error: 0x%x (%@)", message, responseCode, NSStringFromCstiCoreResponseCode(responseCode), (unsigned)errorCode, NSStringFromECoreError(errorCode));
						response->destroy ();
					} break;
				}
			}
			
			break;
		}
        case estiMSG_CORE_REQUEST_REMOVED:
		{
			CstiVPServiceResponse *removedRequest = (CstiVPServiceResponse *)param;
			unsigned int requestId = removedRequest->RequestIDGet();
			
			void (^handler)(CstiCoreResponse *, NSError *) = [self popHandlerForCoreRequestId:requestId];
			if (handler) {
				[self performBlockOnMainThread:^{
					NSError *error = [NSError errorWithDomain:SCIErrorDomainCoreRequestSend
														 code:SCICoreRequestSendErrorCodeRequestRemoved
													 userInfo:nil];
					handler(NULL, error);
				}];
			}
			else
			{
				NSLog(@"Message 0x%02x: Core Request Removed but unexpected, requestId: %du", message, requestId);
			}
			
			//TODO: we may(?) need to delete param->pContext
			removedRequest->destroy ();
			break;
		}
		case estiMSG_CORE_SERVICE_AUTHENTICATED:
			NSLog(@"Message 0x%02x: Core Service Authenticated", message);
			self.authenticated = YES;
			self.connected = YES;
			break;
		case estiMSG_CORE_SERVICE_NOT_AUTHENTICATED:
			NSLog(@"Message 0x%02x: Core Service Not Authenticated", message);
			self.authenticated = NO;
			self.userAccountInfo = nil;
			break;
		case estiMSG_CORE_SERVICE_CONNECTED:
		{
			NSLog(@"Message 0x%02x: Core Service Connected", message);
			self.connected = YES;
			break;
		}
		case estiMSG_CORE_SERVICE_CONNECTING:
		{
			NSLog(@"Message 0x%02x: Core Service Connecting", message);
			self.connected = NO;
			break;
		}
		case estiMSG_CORE_SERVICE_RECONNECTED:
		{
			NSLog(@"Message 0x%02x: Core Service Reconnected", message);
			self.connected = YES;
			break;
		}
		case estiMSG_CONFERENCE_MANAGER_STARTUP_COMPLETE:
		{
			NSLog(@"Message 0x%02x: Conference Manager Startup Complete", message);
			self.started = YES;
			break;
		}
		case estiMSG_DIALING:
		{
			NSLog(@"Message 0x%02x: Dialing", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationCallDialing call:call];
			break;
		}
		case estiMSG_PRE_INCOMING_CALL:
		{
			NSLog(@"Message 0x%02x: Pre Incoming Call", message);
			auto istiCall = (IstiCall *)param;
			SCICall *call = [self callForIstiCall:istiCall];
			[self postCallNotificationName:SCINotificationCallPreIncoming call:call];
			break;
		}
		case estiMSG_INCOMING_CALL:
		{
			NSLog(@"Message 0x%02x: Incoming Call", message);
			auto istiCall = (IstiCall *)param;
			istiCall->LocalIsVcoActiveSet(mVideophoneEngine->VCOUseGet());
			SCICall *call = [self callForIstiCall:istiCall];
			[self postCallNotificationName:SCINotificationCallIncoming call:call];
			break;
		}
		case estiMSG_DISCONNECTING:
		{
			NSLog(@"Message 0x%02x: Call Disconnecting", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationCallDisconnecting call:call];
			break;
		}
		case estiMSG_DISCONNECTED:
		{
			NSLog(@"Message 0x%02x: Call Disconnected", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			
			[self forgetCall:call];
			
			[mCallInProgressCondition lock];
			mCallInProgress = NO;
			[mCallInProgressCondition signal];
			[mCallInProgressCondition unlock];
			
			[self postCallNotificationName:SCINotificationCallDisconnected call:call];
			
			[self setRemoteVideoPrivacyEnabled:NO];
			break;
		}
		case estiMSG_LEAVE_MESSAGE:
		{
			__unused SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Leave Message", message);
			
			[self postCallNotificationName:SCINotificationCallLeaveMessage call:call];
			break;
		}
		case estiMSG_REMOTE_RING_COUNT:
		{
			NSLog(@"Message 0x%02x: Remote (Outgoing) Ring Count: %ld", message, param);
			[self postOnMainThreadNotificationName:SCINotificationRemoteRingCountChanged
										  userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
													[NSNumber numberWithInt:(int)param], @"rings",
													nil]];
			break;
		}
		case estiMSG_LOCAL_RING_COUNT:
		{
			NSLog(@"Message 0x%02x: Local (Incoming) Ring Count: %ld", message, param);
			[self postOnMainThreadNotificationName:SCINotificationLocalRingCountChanged
										  userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
													[NSNumber numberWithInt:(int)param], @"rings",
													nil]];
			break;
		}
		case estiMSG_ANSWERING_CALL:
		{
			NSLog(@"Message 0x%02x: Answering Call", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationCallAnswering call:call];
			break;
		}
		case estiMSG_ESTABLISHING_CONFERENCE:
		{
			NSLog(@"Message 0x%02x: Establishing Conference", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationEstablishingConference call:call];
			break;
		}
		case estiMSG_CONFERENCING:
		{
			// ntouch iOS doesn't seem to do much with this.
			NSLog(@"Message 0x%02x: Conferencing", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			
			[mCallInProgressCondition lock];
			mCallInProgress = YES;
			[mCallInProgressCondition signal];
			[mCallInProgressCondition unlock];
			
			[self postCallNotificationName:SCINotificationConferencing call:call];
#if TARGET_OS_IPHONE
#ifdef stiENABLE_LIGHT_CONTROL
			ledRinger.Stop();
			[LightControl setAllBright];
#endif
#endif
			break;
		}
		case estiMSG_CALL_INFORMATION_CHANGED:
		{
			NSLog(@"Message 0x%02x: Call Information Changed", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationCallInformationChanged call:call];
			break;
		}
		case estiMSG_TRANSFERRING:
		{
			NSLog(@"Message 0x%02x: Transferring", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			if(!call.isVRSCall) //If performing a non VRS transfer clear shared text for the call
			{
				[call.sentText setString:@""];
				[call.receivedText setString:@""];
			}
			[self postCallNotificationName:SCINotificationTransferring call:call];
			break;
		}
		case estiMSG_TRANSFER_FAILED:
		{
			NSLog(@"Message 0x%02x: Transfer Failed", message);
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationTransferFailed call:call];
			break;
		}
		case estiMSG_RESOLVING_NAME:
		{
			// We are notifying the app of this change so it can modify the current and held call objects if needed.
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			[self postCallNotificationName:SCINotificationCallDialing call:call];
			break;
		}
		case estiMSG_DIRECTORY_RESOLVE_RESULT:
		{
			std::unique_ptr<SstiCallResolution> callResolution((SstiCallResolution *)param);
			SCICall *call = [self callForIstiCall:callResolution->pCall];
			NSLog(@"Message 0x%02x: Directory Resolve Result: %@", message, call);
			BOOL shouldContinueDial = YES;
			if (callResolution->nReason & SstiCallResolution::eREDIRECTED_NUMBER)
			{
				if (call.newDialString && call.newDialString.length > 0 && ![call.newDialString isEqualToString:call.dialString])
				{
					// If there is a newDialString and it is not equal to the original dialString...
					[self postCallNotificationName:SCINotificationCallRedirect call:call];
					shouldContinueDial = NO;
					break;
				}
			}
			
			if ((callResolution->nReason & SstiCallResolution::eDIRECTORY_RESOLUTION_FAILED) && self.userAccountInfo != nil)
			{
				EstiDialMethod dialMethod = EstiDialMethodForSCIDialMethod([call dialMethod]);
				if (![call.dialString isEqualToString:@"911"] && (dialMethod == estiDM_BY_VRS_PHONE_NUMBER || dialMethod == estiDM_BY_VRS_WITH_VCO))
				{
					[self postCallNotificationName:SCINotificationCallPromptUserForVRS call:call];
					shouldContinueDial = NO;
					break;
				}
			}
			
			if ((callResolution->nReason & SstiCallResolution::eDIAL_METHOD_DETERMINED) && self.userAccountInfo != nil && self.interfaceMode == SCIInterfaceModePublic)
			{
				EstiDialMethod dialMethod = EstiDialMethodForSCIDialMethod([call dialMethod]);
				if (![call.dialString isEqualToString:@"911"] && (dialMethod == estiDM_BY_VRS_PHONE_NUMBER || dialMethod == estiDM_BY_VRS_WITH_VCO))
				{
					[self postCallNotificationName:SCINotificationCallSelfCertificationRequired call:call];
					shouldContinueDial = NO;
					break;
				}
				// otherwise continue on
			}
			if (shouldContinueDial)
			{
				[call continueDial];
			}
			break;
		}
		case estiMSG_HELD_CALL_LOCAL:
		{
			__unused SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Held Call Local: %@", message, call);
			[self postCallNotificationName:SCINotificationHeldCallLocal call:call];
			break;
		}
		case estiMSG_RESUMED_CALL_LOCAL:
		{
			__unused SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Resumed Call Local: %@", message, call);
			[self postCallNotificationName:SCINotificationResumedCallLocal call:call];
			break;
		}
		case estiMSG_HELD_CALL_REMOTE:
		{
			__unused SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Held Call Remote: %@", message, call);
			[self postCallNotificationName:SCINotificationHeldCallRemote call:call];
			break;
		}
		case estiMSG_RESUMED_CALL_REMOTE:
		{
			__unused SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Resumed Call Remote: %@", message, call);
			[self postCallNotificationName:SCINotificationResumedCallRemote call:call];
			break;
		}

		case estiMSG_DISABLE_PLAYER_CONTROLS:
		{
			NSLog(@"Message 0x%02x: Disable Player Controls", message);
			break;
		}
		case estiMSG_MAILBOX_FULL:
			[self postOnMainThreadNotificationName:SCINotificationMailboxFull userInfo:nil];
			break;
		case estiMSG_MESSAGE_CATEGORY_CHANGED:
		{
			NSLog(@"Message 0x%02x: Category Changed, param: 0x%lx", message, param);
			CstiItemId *categoryItemId = (CstiItemId *)param;
			SCIItemId *itemId = [SCIItemId itemIdWithCstiItemId:*categoryItemId];
			[self postOnMainThreadNotificationName:SCINotificationMessageCategoryChanged
										  userInfo:[NSDictionary dictionaryWithObject:itemId forKey:SCINotificationKeyItemId]];
			break;
		}
		case estiMSG_MESSAGE_ITEM_CHANGED:
		{
			NSLog(@"Message 0x%02x: Item Changed, param: 0x%lx", message, param);
			CstiItemId *messageItemId = (CstiItemId *)param;
			SCIItemId *itemId = [SCIItemId itemIdWithCstiItemId:*messageItemId];
			[self postOnMainThreadNotificationName:SCINotificationMessageChanged
										  userInfo:[NSDictionary dictionaryWithObject:itemId forKey:SCINotificationKeyItemId]];
			break;
		}
		case estiMSG_SIGNMAIL_UPLOAD_URL_GET_FAILED:
		{
			NSLog(@"Message 0x%02x: SignMail Upload URL Get Failed, param: 0x%lx", message, param);
			[self postOnMainThreadNotificationName:SCINotificationSignMailUploadURLGetFailed userInfo:nil];
			break;
		}
		case estiMSG_RECORD_MESSAGE_SEND_FAILED:
		{
			NSLog(@"Message 0x%02x: Message Send Failed, param: 0x%lx", message, param);
			[self postOnMainThreadNotificationName:SCINotificationMessageSendFailed userInfo:nil];
			break;
		}
        case estiMSG_MESSAGE_RESPONSE:
		{
			CstiMessageResponse *response = (CstiMessageResponse *)param;
			unsigned int requestId = response->RequestIDGet();
			CstiMessageResponse::EResponse responseCode = response->ResponseGet();
			CstiMessageResponse::EResponseError errorCode = response->ErrorGet();
			
			void (^handler)(CstiMessageResponse *) = [self popHandlerForMessageRequestId:requestId];
			if (handler)
			{
				[self performBlockOnMainThread:^{
					handler(response);
					response->destroy ();
				}];
			}
			else
			{
				NSLog(@"Message 0x%02x: Message Response not expected: 0x%x (%@), error: 0x%x", message, responseCode, NSStringFromCstiMessageResponseCode(responseCode), (unsigned)errorCode);
				response->destroy ();
			}
			
			break;
		}
        case estiMSG_MESSAGE_REQUEST_REMOVED:
		{
			CstiVPServiceResponse *removedRequest = (CstiVPServiceResponse *)param;
			unsigned int requestId = removedRequest->RequestIDGet();
			
			void (^handler)(CstiMessageResponse *) = [self popHandlerForMessageRequestId:requestId];
			if (handler)
			{
				[self performBlockOnMainThread:^{
					handler(NULL);
				}];
			}
			else
			{
				NSLog(@"Message 0x%02x: Message Request Removed but not expected, requestId: %du", message, requestId);
			}
			
			//TODO: we may(?) need to delete param->pContext
			removedRequest->destroy ();
			break;
		}
		case estiMSG_CB_VIDEO_PLAYBACK_MUTED:
			NSLog(@"Message 0x%02x: Remote Video Muted (Privacy Enabled)", message);
			[self setRemoteVideoPrivacyEnabled:YES]; // also sends SCINotificationRemoteVideoPrivacyChanged
			break;
		case estiMSG_CB_VIDEO_PLAYBACK_UNMUTED:
			NSLog(@"Message 0x%02x: Remote Video Unmuted", message);
			[self setRemoteVideoPrivacyEnabled:NO]; // also sends SCINotificationRemoteVideoPrivacyChanged
			break;
		case estiMSG_FAVORITE_ITEM_ADDED:
		case estiMSG_FAVORITE_ITEM_EDITED:
		case estiMSG_FAVORITE_ITEM_REMOVED:
		case estiMSG_FAVORITE_LIST_CHANGED:
		case estiMSG_FAVORITE_LIST_SET:
		{
			NSLog(@"Message 0x%02x: Favorite List Changed", message);
			[self performBlockOnMainThread:^{
				[[SCIContactManager sharedManager] refresh];
				[[SCIContactManager sharedManager] refreshLDAPDirectoryContacts];

				[self postOnMainThreadNotificationName:SCINotificationFavoriteListChanged userInfo:nil];
			}];
			break;
		}
		case estiMSG_STATE_NOTIFY_RESPONSE:
		{
			CstiStateNotifyResponse *resp = (CstiStateNotifyResponse *)param;
			CstiStateNotifyResponse::EResponse respCode = resp->ResponseGet();

			switch (respCode)
			{
				case CstiStateNotifyResponse::eHEARTBEAT_RESULT:
					break; // ignore
				case CstiStateNotifyResponse::eCALL_LIST_CHANGED:
				{
					break;
				}
				case CstiStateNotifyResponse::eEMERGENCY_PROVISION_STATUS_CHANGE:
				{
					break;
				}
				case CstiStateNotifyResponse::eEMERGENCY_ADDRESS_UPDATE_VERIFY:
				{
					//Only load the emergency address at this point if we're authorized. Loading
					//the emergency address kicks off an SCINotificationEmergencyAddressUpdated
					//which we only want the UI to receive once we're already authorized.
					//If we're not authorized yet, then we will be fetching the emergency address
					//anyway as part of the authorization process.
					//NOTE: It is conceivable that the address would be changed in VDM while the
					//user is authorizing (proceeding through the EULA UI).  Even in that case,
					//however, we will still load the emergency address once we have been
					//authorized, so the user will still see the pertinent UI.
					if (self.authorized) {
						//The account's E911 address has been updated in VDM.  We need to reload the address and its
						//provisioning status.  (Note that the implementation of the callback block in
						//-[SCIVideophoneEngine loadEmergencyAddressWithCompletionHandler:] calls
						//-[SCIVideophoneEngine loadEmergencyAddressProvisioningStatusWithCompletionHandler:] with the
						//completion block that was passed in to the first method.)
						[self loadEmergencyAddressAndEmergencyAddressProvisioningStatusWithCompletionHandler:^(NSError *error) {
							//The provisioning status retrieved from core is not always VPUSERVERIFY but is instead
							//sometimes DEPROVISIONED.  If the status is DEPROVISIONED when the notification that the
							//address has been updated is posted, the user will be prompted accordingly which won't
							//be appropriate since the user should be prompted as if the retrieved status were
							//VPUSERVERIFY.  For this reason, we set the status of the emergency address to [the entry
							//in the SCIEmergencyAddressProvisioningStatus enum which corresponds to] VPUSERVERIFY.
							//This value will be set after core returns from the request to load the emergency address
							//provisioning status because, as mentioned in the prior comment, the completion handler
							//passed into -loadEmergencyAddressWithCompletionHandler: is passed as the completion handler
							//to -loadEmergencyAddressProvisioningStatusWithCompletionHandler:.
							self.emergencyAddress.status = SCIEmergencyAddressProvisioningStatusUpdatedUnverified;
						}];
					}
					break;
				}
				case CstiStateNotifyResponse::eLOGGED_OUT:
				{
					auto pszNumber = resp->ResponseStringGet(0);
					CstiStateNotifyResponse::ELogoutReason reason = (CstiStateNotifyResponse::ELogoutReason)resp->ResponseValueGet();
					NSString *number = !pszNumber.empty() ? [NSString stringWithUTF8String:pszNumber.c_str()] : nil;
					
					NSString *endpointLocalNumber = self.userAccountInfo.localNumber;
					NSString *endpointTollFreeNumber = self.userAccountInfo.tollFreeNumber;
					NSString *ringGroupLocalNumber = self.userAccountInfo.ringGroupLocalNumber;
					NSString *ringGroupTollFreeNumber = self.userAccountInfo.ringGroupTollFreeNumber;
					
					if (number && ([endpointLocalNumber isEqualToString:number]
								   || [endpointTollFreeNumber isEqualToString:number]
								   || [ringGroupLocalNumber isEqualToString:number]
								   || [ringGroupTollFreeNumber isEqualToString:number]
								  )
						)
					{
						//This response was really intended for us.  Log us out.
						if (reason == CstiStateNotifyResponse::eLOGGED_IN_NEW_PHONE) {
							[self postOnMainThreadNotificationName:SCINotificationUserLoggedIntoAnotherDevice userInfo:nil]; //This notification will call logout.
						} else {
							[self logout];
						}
						SCILog(@"User has logged in to another phone.");
					}
					break;
				}
				case CstiStateNotifyResponse::eDISPLAY_PROVIDER_AGREEMENT:
				{
					[self postOnMainThreadNotificationName:SCINotificationDisplayProviderAgreement userInfo:nil];
					break;
				}
				case CstiStateNotifyResponse::eUSER_ACCOUNT_INFO_SETTING_CHANGED:
				{
					//TODO: This situation is less than ideal.  When the endpoint receives
					//		this message, the VideophoneEngine performs a UserAccountInfoGet.
					//		SCIVPE does not receive the UserAccountInfoGetResult for that
					//		request, so it has to do its own UserAccountInfoGet request.
					//		It would be better not to make the same request twice.
					[self loadUserAccountInfo];
					break;
				}
				// Ring Group State Notify Responses:
				case CstiStateNotifyResponse::eRING_GROUP_CREATED:
				{
					SCILog(@"Ring Group Created");
					[self postOnMainThreadNotificationName:SCINotificationRingGroupCreated userInfo:nil];
					break;
				}
				case CstiStateNotifyResponse::eRING_GROUP_INVITE:
				{
					SCILog(@"Ring Group Invite");
					[self fetchRingGroupInviteInfoWithCompletion:^(NSString *name, NSString *localPhone, NSString *tollFreePhone, NSError *error) {
						// If core returns an error code, the name and localPhone will be nil
						if (!name && !localPhone) {
							return;
						}
						NSMutableDictionary *mutableUserInfo = [NSMutableDictionary dictionaryWithCapacity:3];
						[mutableUserInfo setObject:name forKey:SCINotificationKeyName];
						[mutableUserInfo setObject:localPhone forKey:SCINotificationKeyLocalPhone];
						if (tollFreePhone.length) [mutableUserInfo setObject:tollFreePhone forKey:SCINotificationKeyTollFreePhone];
						NSDictionary *userInfo = [mutableUserInfo copy];
						[self postOnMainThreadNotificationName:SCINotificationRingGroupInvitationReceived userInfo:userInfo];
					}];
					break;
				}
				case CstiStateNotifyResponse::ePASSWORD_CHANGED:
				{
					SCILog(@"Password Changed");
					auto pszNewPassword = resp->ResponseStringGet(0);
					auto pszUser = resp->ResponseStringGet(1);
					if ([self.username isEqualToString:[NSString stringWithUTF8String:pszUser.c_str()]]) {
						[self setPassword:[NSString stringWithUTF8String:pszNewPassword.c_str()]];
					}
					
					NSDictionary *userInfo = nil;
					if (!pszNewPassword.empty() && !pszUser.empty()) {
						userInfo = @{ @"password" : [NSString stringWithUTF8String:pszNewPassword.c_str()],
									      @"user" : [NSString stringWithUTF8String:pszUser.c_str()] };
					}

					[self postOnMainThreadNotificationName:SCINotificationPasswordChanged userInfo:userInfo];
					break;
				}
				case CstiStateNotifyResponse::eUPDATE_VERSION_CHECK_FORCE:
				{
					SCILog(@"Update Version Check Required");
					[self postOnMainThreadNotificationName:SCINotificationUpdateVersionCheckRequired userInfo:nil];
					break;
				}
				case CstiStateNotifyResponse::eREBOOT_PHONE:
				{
					SCILog(@"Reboot command received from Core");
					[self setRestartReason:SCIRestartReasonStateNotifyEvent];
					[self exitApplication];
					//TODO: Relaunch application
					break;
				}
				case CstiStateNotifyResponse::eAGREEMENT_CHANGED:
				{
					SCILog(@"Agreement Changed");
					[self postOnMainThreadNotificationName:SCINotificationAgreementChanged userInfo:nil];
					break;
				}
				case CstiStateNotifyResponse::ePROFILE_IMAGE_CHANGED:
				{
					SCILog(@"Profile Image Changed");
					[self postOnMainThreadNotificationName:SCINotificationUserAccountPhotoUpdated userInfo:nil];
					break;
				}
				case CstiStateNotifyResponse::eUSER_NOTIFICATION:
				{
					auto pszNotificationType = resp->ResponseStringGet(0);
					auto pszMessage = resp->ResponseStringGet(1);
					SCILog(@"User Notification: <type=\"%s\", message=\"%s\">", pszNotificationType.c_str(), pszMessage.c_str());
					
					NSString *notificationName = ^{
						NSString *res = nil;
						
						NSString *notificationType = (!pszNotificationType.empty()) ? [NSString stringWithUTF8String:pszNotificationType.c_str()] : nil;
						if ([notificationType isEqualToString:SCIUserNotificationPromoDisplayPSMG]) {
							res = SCINotificationPromoteNewFeaturePersonalSignMailGreeting;
						} else if ([notificationType isEqualToString:SCIUserNotificationPromoWebDial]) {
							res = SCINotificationPromoteNewFeatureWebDial;
						} else if ([notificationType isEqualToString:SCIUserNotificationPromoPhotosOnContacts]) {
							res = SCINotificationPromoteNewFeatureContactPhotos;
						} else if ([notificationType isEqualToString:SCIUserNotificationPromoShareText]) {
							res = SCINotificationPromoteNewFeatureSharedText;
						} else if ([notificationType isEqualToString:SCIUserNotificationPSMGAvailable]) {
							res = SCINotificationPersonalSignMailGreetingAvailable;
						} else if ([notificationType isEqualToString:SCIUserNotificationDismissibleCallCIR]) {
							res = SCINotificationSuggestionCallCIR;
						}
						
						return res;
					}();
					
					if (notificationName) {
						
						NSString *message = (!pszMessage.empty()) ? [NSString stringWithUTF8String:pszMessage.c_str()] : nil;
						NSDictionary *userInfo = ^{
							NSMutableDictionary *mutableRes = [[NSMutableDictionary alloc] init];
							if (message) {
								mutableRes[SCINotificationKeyMessage] = message;
							}
							return [mutableRes copy];
						}();
						
						[self postOnMainThreadNotificationName:notificationName userInfo:userInfo];
						
					}
					
					break;
				}
				default:
				{
					NSLog(@"Message 0x%02x: Notify Response: %d", message, respCode);
					break;
				}
			}
			
			resp->destroy ();
			
			break;
		}
		case estiMSG_FLASH_LIGHT_RING:
		{
			[self postOnMainThreadNotificationName:SCINotificationNudgeReceived userInfo:nil];
			break;
		}
		case estiMSG_RING_GROUP_INFO_CHANGED:
		{
			SCILog(@"Ring Group Info Changed");
			
			NSMutableArray *mutableRingGroupInfos = [NSMutableArray array];
			IstiRingGroupManager *ringGroupManager = self.videophoneEngine->RingGroupManagerGet();
			for (int i=0; i < ringGroupManager->ItemCountGet(); i++)
			{
				IstiRingGroupManager::ERingGroupMemberState state;
				std::string localPhoneNumber, tollFreePhoneNumber, description;
				ringGroupManager->ItemGetByIndex(i, &localPhoneNumber, &tollFreePhoneNumber, &description, &state);
				SCIRingGroupInfo *info = [SCIRingGroupInfo ringGroupInfoWithLocalNumber:&localPhoneNumber
																		 tollFreeNumber:&tollFreePhoneNumber
																			description:&description
																				  state:SCIRingGroupInfoStateFromERingGroupMemberState(state)];
				[mutableRingGroupInfos addObject:info];
			}
			NSArray *ringGroupInfos = [mutableRingGroupInfos copy];
			
			NSDictionary *userInfo = [NSDictionary dictionaryWithObject:ringGroupInfos forKey:SCINotificationKeyRingGroupInfos];
			
			[self postOnMainThreadNotificationName:SCINotificationRingGroupChanged userInfo:userInfo];
			break;
		}
		case estiMSG_RING_GROUP_REMOVED:	//! Informs the UI that this endpoint was removed from the ring group
		{
			SCILog(@"Ring Group Removed");
			[self postOnMainThreadNotificationName:SCINotificationRingGroupRemoved userInfo:nil];
			break;
		}
		case estiMSG_RING_GROUP_MODE_CHANGED:
		{
			SCILog(@"Ring Group Mode Changed");
			[self postOnMainThreadNotificationName:SCINotificationRingGroupModeChanged userInfo:nil];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_EDITED:
		{
			SCILog(@"Ring Group Item Edited");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_EDIT_ERROR:
		{
			SCILog(@"Ring Group Item Edit Error");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_ADDED:
		{
			SCILog(@"Ring Group Item Added");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_ADD_ERROR:
		{
			SCILog(@"Ring Group Item Add Error");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_DELETED:
		{
			SCILog(@"Ring Group Item Deleted");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
		case estiMSG_RING_GROUP_ITEM_DELETE_ERROR:
		{
			SCILog(@"Ring Group Item Delete Error");
			[self dispatchHandlerForRingGroupManagerResponse:(IstiRingGroupMgrResponse *)param];
			break;
		}
#ifdef stiIMAGE_MANAGER
		case estiMSG_IMAGE_LOADED:
		{
			SCILog(@"Image Loaded");
			break;
		}
#endif
		case estiMSG_FAILED_TO_ESTABLISH_TUNNEL:
		{
			[self postOnMainThreadNotificationName:SCINotificationFailedToEstablishTunnel userInfo:nil];
			break;
		}

		case estiMSG_REMOVE_TEXT:
		{
			NSLog(@"Message 0x%02x: Image Overlay Removed Remotely, param: 0x%lx", message, param);
			
			// Fake receiving a series of backspaces to remove the local remote text.
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			unichar backspaceCharacter = 8;
		
			NSString *text = [@"" stringByPaddingToLength:call.receivedText.length withString:[NSString stringWithCharacters:&backspaceCharacter length:1] startingAtIndex:0];
			[call didReceiveText:text];
			
			[self postCallNotificationName:SCINotificationRemoteTextReceived call:call userInfo:@{SCINotificationKeyMessage : text}];
			
			self.videophoneEngine->MessageCleanup(message, param);
			
			[self postCallNotificationName:SCINotificationCallRemoteRemovedTextOverlay call:call];
			break;
		}
		case estiMSG_REMOTE_TEXT_RECEIVED:
		{
			SstiTextMsg *textMessage = (SstiTextMsg *)param;
			SCICall *call = [self callForIstiCall:(IstiCall *)textMessage->poCall];
			uint16_t *pwszText = textMessage->pwszMessage;
			const unichar *characters = (const unichar *)pwszText;
			NSString *text = [NSString stringWithCharacters:characters length:unistrlen(characters)];
			[call didReceiveText:text];
			
			NSLog(@"Message 0x%02x: Remote Text Received: %@", message, text);
			
			[self postCallNotificationName:SCINotificationRemoteTextReceived call:call userInfo:@{SCINotificationKeyMessage : text}];
			
			self.videophoneEngine->MessageCleanup(message, param);
			break;
		}
		case estiMSG_INTERFACE_MODE_CHANGED:
		{
			[self postOnMainThreadNotificationName:SCINotificationInterfaceModeChanged userInfo:nil];
			[self performBlockOnMainThread:^{
				[self willChangeValueForKey:SCIKeyInterfaceMode];
				[self didChangeValueForKey:SCIKeyInterfaceMode];
			}];
			[self loadUserAccountInfo];
			break;
		}
		case estiMSG_HEARING_STATUS_CHANGED:
		{
			NSLog(@"Message 0x%02x: Hearing Status Changed", message);

			[self postOnMainThreadNotificationName:SCINotificationHearingStatusChanged userInfo:nil];

			break;
		}
		case estiMSG_CONFERENCE_SERVICE_ERROR:
		case estiMSG_CONFERENCE_SERVICE_INSUFFICIENT_RESOURCES:
		{
			NSLog(@"Message 0x%02x: Conference Service Error ***", message);

			[self postOnMainThreadNotificationName:SCINotificationConferenceServiceError userInfo:nil];
			
			break;
		}
		case estiMSG_CONFERENCE_ROOM_ADD_ALLOWED_CHANGED:
		{
			NSLog(@"Message 0x%02x: Conference Room Add Allowed Changed ***", message);
			
			[self postOnMainThreadNotificationName:SCINotificationConferenceRoomAddAllowedChanged userInfo:nil];
			
			break;
		}
		case estiMSG_BLOCK_CALLER_ID_CHANGED:
		{
			NSLog(@"Message 0x%02x: Block Caller ID Changed ***", message);
			[self performBlockOnMainThread:^{
				[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationPropertyChanged
																	object:self
																  userInfo:@{ SCINotificationKeyName : @(BLOCK_CALLER_ID) }];
			}];
			
			break;
		}
		case estiMSG_BLOCK_ANONYMOUS_CALLERS_CHANGED:
		{
			NSLog(@"Message 0x%02x: Block Anonymous Callers Changed ***", message);
			[self performBlockOnMainThread:^{
				[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationPropertyChanged
																	object:self
																  userInfo:@{ SCINotificationKeyName : @(BLOCK_ANONYMOUS_CALLERS) }];
			}];
			
			break;
		}
		case estiMSG_NAT_TRAVERSAL_SIP_CHANGED:
		{
			NSLog(@"Message 0x%02x: NATTraversalSIP Changed ***", message);
			
			[self postOnMainThreadNotificationName:SCINotificationNATTraversalSIPChanged userInfo:nil];
			
			break;
		}
		case estiMSG_RINGS_BEFORE_GREETING_CHANGED:
		{
			NSLog(@"Message 0x%02x: RingsBeforeGreeting conference parameter changed ***", message);
			[self postOnMainThreadNotificationName:SCINotificationRingsBeforeGreetingChanged userInfo:nil];
			break;
		}
		case estiMSG_PLEASE_WAIT:
		{
			NSLog(@"Message 0x%02x: Please Wait Message being displayed ***", message);
			[self postOnMainThreadNotificationName:SCINotificationDisplayPleaseWait userInfo:nil];
			break;
		}
		case estiMSG_CONFERENCE_ENCRYPTION_STATE_CHANGED:
		{
			SCICall *call = [self callForIstiCall:(IstiCall *)param];
			NSLog(@"Message 0x%02x: Conference encryption state changed: %@", message, NSStringFromSCIEncryptionState(call.encryptionState));
			[self postCallNotificationName:SCINotificationConferenceEncryptionStateChanged call:call];
			break;
		}
		default:
		{
			NSLog(@"Message 0x%02x: %@ param: 0x%lx", message, NSStringFromEstiCmMessage(message), param);
			self.videophoneEngine->MessageCleanup(message, param);
			break;
		}
	}
	return stiRESULT_SUCCESS;
}

#pragma mark - Debug
// Create trace.conf file used for toggling debug flags through the UI
- (void)saveFile
{
	DebugTools::instanceGet()->fileSave();
}

// Set debug trace flags
- (void)setTraceFlagWithName:(NSString *)name value:(NSInteger)value
{
	auto debugTool = DebugTools::instanceGet()->debugToolGet(std::string([name UTF8String], name.length));
	debugTool->valueSet(value);
}

@end

// Print debug messages from stiTrace
void nsprintf(const char* msg)
{
	NSLog(@"%s", msg);
}

// Returns nil if CstiCoreResponse has no error condition set.
NSError *NSErrorFromCstiCoreResponse(CstiCoreResponse *resp)
{
	NSError *err = nil;
	if (resp)
	{
		if (resp->ErrorGet() != CstiCoreResponse::eNO_ERROR)
		{
			auto cstrError = resp->ErrorStringGet();
			NSDictionary *userInfo;
			if (!cstrError.empty())
			{
				userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
							[[NSString alloc] initWithUTF8String:cstrError.c_str()], NSLocalizedDescriptionKey,
							nil];
			}
			err = [NSError errorWithDomain:SCIErrorDomainCoreResponse
									  code:SCICoreResponseErrorCodeFromECoreError(resp->ErrorGet())
								  userInfo:userInfo];
		}
	}
	else
	{
		//TODO: Determine if an error should be returned in this case.
		//		Determine whether this is an appropriate error (domain
		//		and code).
		//If we don't have a response, then the request failed.
		err = [NSError errorWithDomain:SCIErrorDomainCoreRequestSend
								  code:CstiCoreResponse::eUNKNOWN_ERROR
							  userInfo:nil];
	}
	return err;
}

NSError *NSErrorFromCstiMessageResponse(CstiMessageResponse *resp)
{
	NSError *err = nil;
	if (resp)
	{
		if (resp->ErrorGet() != CstiMessageResponse::eNO_ERROR)
		{
			auto cstrError = resp->ErrorStringGet();
			NSDictionary *userInfo;
			if (!cstrError.empty())
			{
				userInfo = @{NSLocalizedDescriptionKey: [[NSString alloc] initWithUTF8String:cstrError.c_str()]};
			}
			err = [NSError errorWithDomain:SCIErrorDomainMessageResponse
									  code:resp->ErrorGet()
								  userInfo:userInfo];
		}
	}
	else
	{
		//If we don't have a response, then the request failed.
		err = [NSError errorWithDomain:SCIErrorDomainMessageRequestSend
								  code:CstiMessageResponse::eUNKNOWN_ERROR
							  userInfo:nil];
	}
	return err;
}

NSError *NSErrorFromIstiRingGroupMgrResponse(IstiRingGroupMgrResponse *resp)
{
	NSError *err = nil;
	if ((CstiCoreResponse::ECoreError)resp->CoreErrorCodeGet() != CstiCoreResponse::eNO_ERROR)
	{
		const char *cstrError = NULL;
		NSDictionary *userInfo = nil;
		if (cstrError)
		{
			userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
						[[NSString alloc] initWithUTF8String:cstrError], NSLocalizedDescriptionKey,
						nil];
		}
		err = [NSError errorWithDomain:SCIErrorDomainRingGroupManagerResponse
								  code:resp->CoreErrorCodeGet()
							  userInfo:userInfo];
	}
	return err;
}

NSString *NSStringFromEstiCmMessage(int32_t m)
{
	static NSDictionary *lookup = nil;
	if (!lookup)
	{
		NSMutableDictionary *tmp = [NSMutableDictionary dictionary];
#define ADD_ESTIMSG(msg) [tmp setObject:[NSString stringWithUTF8String:( #msg )] forKey:[NSNumber numberWithInt:msg]]
		ADD_ESTIMSG(estiMSG_CONFERENCE_MANAGER_STARTUP_COMPLETE);
		ADD_ESTIMSG(estiMSG_CONFERENCE_MANAGER_CRITICAL_ERROR);
		ADD_ESTIMSG(estiMSG_DIALING);
		ADD_ESTIMSG(estiMSG_INCOMING_CALL);
		ADD_ESTIMSG(estiMSG_RINGING);
		ADD_ESTIMSG(estiMSG_REMOTE_RING_COUNT);
		ADD_ESTIMSG(estiMSG_LEAVE_MESSAGE);
		ADD_ESTIMSG(estiMSG_MAILBOX_FULL);
		ADD_ESTIMSG(estiMSG_RECORD_ERROR);
		ADD_ESTIMSG(estiMSG_RECORD_MESSAGE);
		ADD_ESTIMSG(estiMSG_RECORD_MESSAGE_SEND_FAILED);
		ADD_ESTIMSG(estiMSG_ANSWERING_CALL);
		ADD_ESTIMSG(estiMSG_ESTABLISHING_CONFERENCE);
		ADD_ESTIMSG(estiMSG_HELD_CALL_LOCAL);
		ADD_ESTIMSG(estiMSG_HELD_CALL_REMOTE);
		ADD_ESTIMSG(estiMSG_RESUMED_CALL_LOCAL);
		ADD_ESTIMSG(estiMSG_RESUMED_CALL_REMOTE);
		ADD_ESTIMSG(estiMSG_RESOLVING_NAME);
		ADD_ESTIMSG(estiMSG_DIRECTORY_RESOLVE_RESULT);
		ADD_ESTIMSG(estiMSG_CONFERENCING);
		ADD_ESTIMSG(estiMSG_DISCONNECTING);
		ADD_ESTIMSG(estiMSG_TRANSFERRING);
		ADD_ESTIMSG(estiMSG_DISCONNECTED);
		ADD_ESTIMSG(estiVRS_SIGNMAIL_CONFIRMATION_DONE);
		ADD_ESTIMSG(estiMSG_CALL_INFORMATION_CHANGED);
		ADD_ESTIMSG(estiMSG_PRINTSCREEN);
		ADD_ESTIMSG(estiMSG_CB_ERROR_REPORT);
		ADD_ESTIMSG(estiMSG_CB_PUBLIC_IP_RESOLVED);
		ADD_ESTIMSG(estiMSG_CB_CURRENT_TIME_SET);
		ADD_ESTIMSG(estiMSG_CB_SERVICE_SHUTDOWN);
		ADD_ESTIMSG(estiMSG_CB_VIDEO_PLAYBACK_MUTED);
		ADD_ESTIMSG(estiMSG_CB_VIDEO_PLAYBACK_UNMUTED);
		ADD_ESTIMSG(estiMSG_CB_DTMF_RECVD);
		ADD_ESTIMSG(estiMSG_DNS_ERROR);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_AUTHENTICATED);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_NOT_AUTHENTICATED);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_CONNECTED);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_CONNECTING);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_RECONNECTED);
		ADD_ESTIMSG(estiMSG_CORE_RESPONSE);
		ADD_ESTIMSG(estiMSG_CORE_REQUEST_REMOVED);
		ADD_ESTIMSG(estiMSG_CORE_SERVICE_SSL_FAILOVER);
		ADD_ESTIMSG(estiMSG_CORE_ASYNC_REQUEST_ID);
		ADD_ESTIMSG(estiMSG_CORE_ASYNC_REQUEST_FAILED);
		ADD_ESTIMSG(estiMSG_STATE_NOTIFY_SERVICE_CONNECTED);
		ADD_ESTIMSG(estiMSG_STATE_NOTIFY_SERVICE_CONNECTING);
		ADD_ESTIMSG(estiMSG_STATE_NOTIFY_SERVICE_RECONNECTED);
		ADD_ESTIMSG(estiMSG_STATE_NOTIFY_RESPONSE);
		ADD_ESTIMSG(estiMSG_STATE_NOTIFY_SSL_FAILOVER);
		ADD_ESTIMSG(estiMSG_MESSAGE_SERVICE_CONNECTED);
		ADD_ESTIMSG(estiMSG_MESSAGE_SERVICE_CONNECTING);
		ADD_ESTIMSG(estiMSG_MESSAGE_SERVICE_RECONNECTED);
		ADD_ESTIMSG(estiMSG_MESSAGE_RESPONSE);
		ADD_ESTIMSG(estiMSG_MESSAGE_REQUEST_REMOVED);
		ADD_ESTIMSG(estiMSG_MESSAGE_SERVICE_SSL_FAILOVER);
		ADD_ESTIMSG(estiMSG_FLASH_LIGHT_RING);
		ADD_ESTIMSG(estiMSG_FILE_PLAY_STATE);
		ADD_ESTIMSG(estiMSG_DISABLE_PLAYER_CONTROLS);
		ADD_ESTIMSG(estiMSG_REQUEST_GUID);
		ADD_ESTIMSG(estiMSG_REQUEST_UPLOAD_GUID);
		ADD_ESTIMSG(estiMSG_REBOOT_DEVICE);
		ADD_ESTIMSG(estiMSG_NETWORK_STATE_CHANGE);
		ADD_ESTIMSG(estiMSG_NETWORK_SETTINGS_CHANGED);
		ADD_ESTIMSG(estiMSG_NETWORK_WIRED_CONNECTED);
		ADD_ESTIMSG(estiMSG_NETWORK_WIRED_DISCONNECTED);
		ADD_ESTIMSG(estiMSG_CONFERENCE_PORTS_CHANGED);
		
#if APPLICATION == APP_NTOUCH
		ADD_ESTIMSG(estiMSG_USBRCU_CONNECTED);
		ADD_ESTIMSG(estiMSG_USBRCU_DISCONNECTED);
#endif
		
#ifdef stiWIRELESS
		ADD_ESTIMSG(estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS);
		ADD_ESTIMSG(estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE);
#endif
		
#ifdef stiHOLDSERVER
		ADD_ESTIMSG(estiMSG_HS_VIDEO_FROM_FILE_COMPLETE);
#endif
		
		ADD_ESTIMSG(estiMSG_IMAGE_LOADED);
		ADD_ESTIMSG(estiMSG_INTERFACE_MODE_CHANGED);
		ADD_ESTIMSG(estiMSG_MAX_SPEEDS_CHANGED);
		ADD_ESTIMSG(estiMSG_AUTO_SPEED_SETTINGS_CHANGED);
		ADD_ESTIMSG(estiMSG_PUBLIC_IP_CHANGED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_ITEM_ADDED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_ITEM_EDITED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_ITEM_DELETED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_CHANGED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_ENABLED);
		ADD_ESTIMSG(estiMSG_BLOCK_LIST_DISABLED);
		ADD_ESTIMSG(estiMSG_CONTACT_LIST_ITEM_ADDED);
		ADD_ESTIMSG(estiMSG_CONTACT_LIST_ITEM_EDITED);
		ADD_ESTIMSG(estiMSG_CONTACT_LIST_ITEM_DELETED);
		ADD_ESTIMSG(estiMSG_CONTACT_LIST_CHANGED);
		ADD_ESTIMSG(estiMSG_CONTACT_LIST_DELETED);
		ADD_ESTIMSG(estiMSG_MESSAGE_CATEGORY_CHANGED);
		ADD_ESTIMSG(estiMSG_MESSAGE_ITEM_CHANGED);
		ADD_ESTIMSG(estiMSG_SIGNMAIL_UPLOAD_URL_GET_FAILED);
		ADD_ESTIMSG(estiMSG_FAILED_TO_ESTABLISH_TUNNEL);
		ADD_ESTIMSG(estiMSG_CB_SIP_REGISTRATION_CONFIRMED);
		ADD_ESTIMSG(estiMSG_FAVORITE_ITEM_ADDED);
		ADD_ESTIMSG(estiMSG_FAVORITE_ITEM_EDITED);
		ADD_ESTIMSG(estiMSG_FAVORITE_ITEM_REMOVED);
		ADD_ESTIMSG(estiMSG_FAVORITE_LIST_CHANGED);
		ADD_ESTIMSG(estiMSG_FAVORITE_LIST_SET);
		ADD_ESTIMSG(estiMSG_LDAP_DIRECTORY_CONTACT_LIST_CHANGED);
		
#if APPLICATION == APP_NTOUCH_PC
		ADD_ESTIMSG(estiMSG_CB_DATA_RATE_CHANGE);
#endif
		
		ADD_ESTIMSG(estiMSG_CB_USER_INFO_UPDATED);
		ADD_ESTIMSG(estiMSG_HEARING_STATUS_CHANGED);
		ADD_ESTIMSG(estiMSG_PLEASE_WAIT);
		ADD_ESTIMSG(estiMSG_NEXT);
#undef ADD_ESTIMSG
		lookup = tmp;
	}
	NSString *output = [lookup objectForKey:[NSNumber numberWithInt:m]];
	if (!output)
	{
		NSString *message =  [NSString stringWithUTF8String:stiMessageStringGet(m).c_str()];
		output = [NSString stringWithFormat:@"Unknown (0x%02x) stiMessageStringGet:%@", m, message];
	}
	return output;
}

NSString *NSStringFromCstiCoreResponseCode(CstiCoreResponse::EResponse response)
{
	static NSDictionary *lookup = nil;
	if (!lookup) {
		NSMutableDictionary *tmp = [NSMutableDictionary dictionary];
#define ADD_ERESPONSE(msg) \
do { \
	[tmp setObject:[NSString stringWithUTF8String:( #msg )] forKey:[NSNumber numberWithInt:msg]]; \
} while(0)
		
		ADD_ERESPONSE(CstiCoreResponse::eRESPONSE_UNKNOWN);
		ADD_ERESPONSE(CstiCoreResponse::eRESPONSE_ERROR);
		ADD_ERESPONSE(CstiCoreResponse::eAGREEMENT_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eAGREEMENT_STATUS_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCALL_LIST_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eCLIENT_LOGOUT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_DEPROVISION_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eIMAGE_DELETE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eMISSED_CALL_ADD_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePHONE_ONLINE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePRIMARY_USER_EXISTS_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::ePUBLIC_IP_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_CREATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_CREDENTIALS_UPDATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_INVITE_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_INVITE_REJECT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_USER_INVITE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eRING_GROUP_USER_REMOVE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eSERVICE_CONTACT_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eSIGNMAIL_REGISTER_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eTIME_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eTRAINER_VALIDATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eURI_LIST_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_ACCOUNT_ASSOCIATE_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_SETTINGS_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_SETTINGS_SET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eVERSION_GET_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eLOG_CALL_TRANSFER_RESULT);
		ADD_ERESPONSE(CstiCoreResponse::eUSER_DEFAULTS_RESULT);
		
#undef ADD_ERESPONSE
		lookup = [tmp copy];
	}
	
	NSString *res = nil;
	
	res = [lookup objectForKey:[NSNumber numberWithInt:response]];
	
	return res;
}

NSString *NSStringFromCstiMessageResponseCode(CstiMessageResponse::EResponse responseCode)
{
	static NSDictionary *lookup = nil;
	if (!lookup) {
		NSMutableDictionary *tmp = [NSMutableDictionary dictionary];
#define ADD_ERESPONSE(msg) [tmp setObject:[NSString stringWithUTF8String:( #msg )] forKey:[NSNumber numberWithInt:msg]]
		
		ADD_ERESPONSE(CstiMessageResponse::eRESPONSE_UNKNOWN);
		ADD_ERESPONSE(CstiMessageResponse::eRESPONSE_ERROR);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_VIEWED_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_LIST_ITEM_EDIT_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_LIST_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_CREATE_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_DELETE_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_DELETE_ALL_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_SEND_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_LIST_COUNT_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eMESSAGE_UPLOAD_URL_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eNEW_MESSAGE_COUNT_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eRETRIEVE_MESSAGE_KEY_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eGREETING_INFO_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eGREETING_UPLOAD_URL_GET_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eGREETING_DELETE_RESULT);
		ADD_ERESPONSE(CstiMessageResponse::eGREETING_ENABLED_STATE_SET_RESULT);
#undef ADD_ERESPONSE
		lookup = [tmp copy];
	}
	
	return [lookup objectForKey:[NSNumber numberWithInt:responseCode]];
}

NSString *NSStringFromECoreError(CstiCoreResponse::ECoreError error)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddError(error) \
do { \
mutableDictionary[@((error))] = [NSString stringWithUTF8String:( #error )]; \
} while(0)
		
		AddError( CstiCoreResponse::eCONNECTION_ERROR );
		AddError( CstiCoreResponse::eNO_ERROR );
		AddError( CstiCoreResponse::eNOT_LOGGED_IN );
		AddError( CstiCoreResponse::eUSERNAME_OR_PASSWORD_INVALID );
		AddError( CstiCoreResponse::ePHONE_NUMBER_NOT_FOUND );
		AddError( CstiCoreResponse::eDUPLICATE_PHONE_NUMBER );
		AddError( CstiCoreResponse::eERROR_CREATING_USER );
		AddError( CstiCoreResponse::eERROR_ASSOCIATING_PHONE_NUMBER );
		AddError( CstiCoreResponse::eCANNOT_ASSOCIATE_PHONE_NUMBER );
		AddError( CstiCoreResponse::eERROR_DISASSOCIATING_PHONE_NUMBER );
		AddError( CstiCoreResponse::eCANNOT_DISASSOCIATE_PHONE_NUMBER );
		AddError( CstiCoreResponse::eERROR_UPDATING_USER );
		AddError( CstiCoreResponse::eUNIQUE_ID_UNRECOGNIZED );
		AddError( CstiCoreResponse::ePHONE_NUMBER_NOT_REGISTERED );
		AddError( CstiCoreResponse::eDUPLICATE_UNIQUE_ID );
		AddError( CstiCoreResponse::eCONNECTION_TO_SIGNMAIL_DOWN );
		AddError( CstiCoreResponse::eNO_INFORMATION_SENT );
		AddError( CstiCoreResponse::eUNIQUE_ID_EMPTY );
		AddError( CstiCoreResponse::eREAL_NUMBER_LIST_RELEASE_FAILED );
		AddError( CstiCoreResponse::eRELEASE_NUMBERS_NOT_SPECIFIED );
		AddError( CstiCoreResponse::eCOUNT_ATTRIBUTE_INVALID );
		AddError( CstiCoreResponse::eREAL_NUMBER_LIST_GET_FAILED );
		AddError( CstiCoreResponse::eREAL_NUMBER_SERVICE_UNAVAILABLE );
		AddError( CstiCoreResponse::eNOT_PRIMARY_USER_ON_PHONE );
		AddError( CstiCoreResponse::ePRIMARY_USER_ON_ANOTHER_PHONE );
		AddError( CstiCoreResponse::ePROVISION_REQUEST_FAILED );
		AddError( CstiCoreResponse::eNO_CURRENT_EMERGENCY_STATUS_FOUND );
		AddError( CstiCoreResponse::eNO_CURRENT_EMERGENCY_ADDRESS_FOUND );
		AddError( CstiCoreResponse::eEMERGENCY_ADDRESS_DATA_BLANK );
		AddError( CstiCoreResponse::eDEPROVISION_REQUEST_FAILED );
		AddError( CstiCoreResponse::eUNABLE_TO_CONTACT_DATABASE );
		AddError( CstiCoreResponse::eNO_REAL_NUMBER_AVAILABLE );
		AddError( CstiCoreResponse::eUNABLE_TO_ACQUIRE_NUMBER );
		AddError( CstiCoreResponse::eNO_PENDING_NUMBER_AVAILABLE );
		AddError( CstiCoreResponse::eROUTER_INFO_NOT_CLEARED );
		AddError( CstiCoreResponse::eNO_PHONE_INFO_FOUND );
		AddError( CstiCoreResponse::eQIC_DOES_NOT_EXIST );
		AddError( CstiCoreResponse::eQIC_CANNOT_BE_REACHED );
		AddError( CstiCoreResponse::eQIC_NOT_DECLARED );
		AddError( CstiCoreResponse::eIMAGE_PROBLEM );
		AddError( CstiCoreResponse::eIMAGE_PROBLEM_SAVING_DATA );
		AddError( CstiCoreResponse::eIMAGE_PROBLEM_DELETING );
		AddError( CstiCoreResponse::eIMAGE_LIST_GET_PROBLEM );
		AddError( CstiCoreResponse::eENDPOINT_MATCH_ERROR );
		AddError( CstiCoreResponse::eBLOCK_LIST_ITEM_DENIED );
		AddError( CstiCoreResponse::eCALL_LIST_PLAN_NOT_FOUND );
		AddError( CstiCoreResponse::eENDPOINT_TYPE_NOT_FOUND );
		AddError( CstiCoreResponse::ePHONE_TYPE_NOT_RECOGNIZED );
		AddError( CstiCoreResponse::eCANNOT_GET_SERVICE_CONTACT_OVERRIDES );
		AddError( CstiCoreResponse::eCANNOT_CONTACT_ITRS );
		AddError( CstiCoreResponse::eURI_LIST_SET_MISMATCH );
		AddError( CstiCoreResponse::eURI_LIST_SET_DELETE_ERROR );
		AddError( CstiCoreResponse::eURI_LIST_SET_ADD_ERROR );
		AddError( CstiCoreResponse::eSIGNMAIL_REGISTER_ERROR );
		AddError( CstiCoreResponse::eADDED_MORE_CONTACTS_THAN_ALLOWED );
		AddError( CstiCoreResponse::eCANNOT_DETERMINE_WHICH_LOGIN );
		AddError( CstiCoreResponse::eLOGIN_VALUE_INVALID );
		AddError( CstiCoreResponse::eERROR_UPDATING_ALIAS );
		AddError( CstiCoreResponse::eERROR_CREATING_GROUP );
		AddError( CstiCoreResponse::eINVALID_PIN );
		AddError( CstiCoreResponse::eUSER_CANNOT_CREATE_RINGGROUP );
		AddError( CstiCoreResponse::eENDPOINT_NOT_RINGGROUP_CAPABLE );
		AddError( CstiCoreResponse::eCALL_LIST_ITEM_NOT_OWNED );
		AddError( CstiCoreResponse::eVIDEO_SERVER_UNREACHABLE );
		AddError( CstiCoreResponse::eCANNOT_UPDATE_ENUM_SERVER );
		AddError( CstiCoreResponse::eCALL_LIST_ITEM_NOT_FOUND );
		AddError( CstiCoreResponse::eERROR_ROLLING_BACK_RING_GROUP );
		AddError( CstiCoreResponse::eNO_INVITE_FOUND );
		AddError( CstiCoreResponse::eNOT_MEMBER_OF_RING_GROUP );
		AddError( CstiCoreResponse::eCANNOT_INVITE_PHONE_NUMBER_TO_GROUP );
		AddError( CstiCoreResponse::eCANNOT_INVITE_USER );
		AddError( CstiCoreResponse::eRING_GROUP_DOESNT_EXIST );
		AddError( CstiCoreResponse::eRING_GROUP_NUMBER_NOT_FOUND );
		AddError( CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_NOT_COMPLETED );
		AddError( CstiCoreResponse::eRING_GROUP_INVITE_REJECT_NOT_COMPLETED );
		AddError( CstiCoreResponse::eRING_GROUP_NUMBER_CANNOT_BE_EMPTY );
		AddError( CstiCoreResponse::eINVALID_DESCRIPTION_LENGTH );
		AddError( CstiCoreResponse::eUSER_NOT_FOUND );
		AddError( CstiCoreResponse::eINVALID_USER );
		AddError( CstiCoreResponse::eGROUP_USER_NOT_FOUND );
		AddError( CstiCoreResponse::eUSER_BEING_REMOVED_NOT_IN_GROUP );
		AddError( CstiCoreResponse::eUSER_CANNOT_BE_REMOVED_FROM_GROUP );
		AddError( CstiCoreResponse::eUSER_NOT_MEMBER_OF_GROUP );
		AddError( CstiCoreResponse::eELEMENT_HAS_INVALID_PHONE_NUMBER );
		AddError( CstiCoreResponse::ePASSWORD_AND_CONFIRM_DO_NOT_MATCH );
		AddError( CstiCoreResponse::eCANNOT_BLOCK_RING_GROUP_MEMBER );
		AddError( CstiCoreResponse::eCANNOT_REMOVE_LAST_USER_FROM_GROUP );
		AddError( CstiCoreResponse::eURI_LIST_MUST_CONTAIN_ITRS_URIS );
		AddError( CstiCoreResponse::eINVALID_URI );
		AddError( CstiCoreResponse::eREDIRECT_TO_AND_FROM_URIS_CANNOT_BE_SAME );
		AddError( CstiCoreResponse::eREMOVE_OTHER_MEMBERS_BEFORE_REMOVING_RING_GROUP );
		AddError( CstiCoreResponse::eNEED_ONE_MEMBER_IN_RING_GROUP );
		AddError( CstiCoreResponse::eCOULD_NOT_LOCATE_ACTIVE_LOCAL_NUMBER );
		AddError( CstiCoreResponse::eUNKNOWN_ERROR_2 );
		AddError( CstiCoreResponse::eINVALID_GROUP_CREDENTIALS );
		AddError( CstiCoreResponse::eNOT_MEMBER_OF_THIS_GROUP );
		AddError( CstiCoreResponse::eERROR_SETTING_GROUP_PASSWORD );
		AddError( CstiCoreResponse::ePHONE_NUMBER_CANNOT_BE_EMPTY );
		AddError( CstiCoreResponse::eERROR_WHILE_SETTING_RING_GROUP_INFO );
		AddError( CstiCoreResponse::eERROR_DIAL_STRING_REQUIRED_ELEMENT );
		AddError( CstiCoreResponse::eERROR_CANNOT_REMOVE_RING_GROUP_CREATOR );
		AddError( CstiCoreResponse::eERROR_IMAGE_NOT_FOUND );
		AddError( CstiCoreResponse::eERROR_NO_AGREEMENT_FOUND );
		AddError( CstiCoreResponse::eERROR_UPDATING_AGREEMENT );
		AddError( CstiCoreResponse::eERROR_GUID_INCORRECT_FORMAT );
		AddError( CstiCoreResponse::eERROR_CORE_CONFIGURATION );
		AddError( CstiCoreResponse::eERROR_REMOVING_IMAGE );
		AddError( CstiCoreResponse::eERROR_UNABLE_SET_SETTING );
		AddError( CstiCoreResponse::eDATABASE_CONNECTION_ERROR );
		AddError( CstiCoreResponse::eXML_VALIDATION_ERROR );
		AddError( CstiCoreResponse::eXML_FORMAT_ERROR );
		AddError( CstiCoreResponse::eGENERIC_SERVER_ERROR );
		AddError( CstiCoreResponse::eUNKNOWN_ERROR );

#undef AddError
		
		dictionary = [mutableDictionary copy];
	});
	return dictionary[@(error)];
}

NSString *NSStringFromFilePlayState(uint32_t param)
{
	NSMutableArray *states = [NSMutableArray array];
	if (param & IstiMessageViewer::estiOPENING) [states addObject:@"Opening"];
	if (param & IstiMessageViewer::estiRELOADING) [states addObject:@"Reloading"];
	if (param & IstiMessageViewer::estiPLAY_WHEN_READY) [states addObject:@"PlayWhenReady"];
	if (param & IstiMessageViewer::estiPLAYING) [states addObject:@"Playing"];
	if (param & IstiMessageViewer::estiPAUSED) [states addObject:@"Paused"];
	if (param & IstiMessageViewer::estiCLOSING) [states addObject:@"Closing"];
	if (param & IstiMessageViewer::estiCLOSED) [states addObject:@"Closed"];
	if (param & IstiMessageViewer::estiERROR) [states addObject:@"Error"];
	if (param & IstiMessageViewer::estiVIDEO_END) [states addObject:@"VideoEnd"];
	if (param & IstiMessageViewer::estiREQUESTING_GUID) [states addObject:@"RequestingGuid"];
	if (param & IstiMessageViewer::estiRECORD_CONFIGURE) [states addObject:@"RecordConfigure"];
	if (param & IstiMessageViewer::estiWAITING_TO_RECORD) [states addObject:@"WaitingToRecord"];
	if (param & IstiMessageViewer::estiRECORDING) [states addObject:@"Recording"];
	if (param & IstiMessageViewer::estiRECORD_FINISHED) [states addObject:@"RecordFinished"];
	if (param & IstiMessageViewer::estiRECORD_CLOSING) [states addObject:@"RecordClosing"];
	if (param & IstiMessageViewer::estiRECORD_CLOSED) [states addObject:@"RecordClosed"];
	if (param & IstiMessageViewer::estiUPLOADING) [states addObject:@"Uploading"];
	if (param & IstiMessageViewer::estiUPLOAD_COMPLETE) [states addObject:@"UploadComplete"];
	if (param & IstiMessageViewer::estiPLAYER_IDLE) [states addObject:@"PlayerIdle"];
	return [states componentsJoinedByString:@", "];
}

NSString *NSStringFromFilePlayError(uint32_t param)
{
	static NSDictionary *stringFromErrorDictionary = nil;
	if (!stringFromErrorDictionary) {
		NSMutableDictionary *mutableStringFromErrorDictionary = [NSMutableDictionary dictionary];
		
#define ADD_ERROR(msg) [mutableStringFromErrorDictionary setObject:[NSString stringWithUTF8String:( #msg )] forKey:[NSNumber numberWithInt:msg]]
		ADD_ERROR(IstiMessageViewer::estiERROR_NONE);
		ADD_ERROR(IstiMessageViewer::estiERROR_GENERIC);
		ADD_ERROR(IstiMessageViewer::estiERROR_SERVER_BUSY);
		ADD_ERROR(IstiMessageViewer::estiERROR_OPENING);
		ADD_ERROR(IstiMessageViewer::estiERROR_REQUESTING_GUID);
		ADD_ERROR(IstiMessageViewer::estiERROR_PARSING_GUID);
		ADD_ERROR(IstiMessageViewer::estiERROR_PARSING_UPLOAD_GUID);
		ADD_ERROR(IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE);
		ADD_ERROR(IstiMessageViewer::estiERROR_RECORDING);
		ADD_ERROR(IstiMessageViewer::estiERROR_NO_DATA_UPLOADED);
#undef ADD_ERROR
		
		stringFromErrorDictionary = [mutableStringFromErrorDictionary copy];
	}
	return stringFromErrorDictionary[@(param)];
}

SCIVoiceCarryOverType SCIVoiceCarryOverTypeFromEstiVcoType(EstiVcoType vcoType)
{
	return (SCIVoiceCarryOverType)vcoType;
}

EstiVcoType EstiVcoTypeFromSCIVoiceCarryOverType(SCIVoiceCarryOverType voiceCarryOverType)
{
	return (EstiVcoType)voiceCarryOverType;
}

SCIInterfaceMode SCIInterfaceModeFromEstiInterfaceMode(EstiInterfaceMode mode)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define setSCIModeForEstiMode( sciMode, estiMode ) \
do { \
mutableDictionary[@((estiMode))] = @((sciMode)); \
} while(0)
		
		setSCIModeForEstiMode(SCIInterfaceModeStandard, estiSTANDARD_MODE);
		setSCIModeForEstiMode(SCIInterfaceModePublic, estiPUBLIC_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeKiosk, estiKIOSK_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeInterpreter, estiINTERPRETER_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeTechSupport, estiTECH_SUPPORT_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeVRI, estiVRI_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeAbusiveCaller, estiABUSIVE_CALLER_MODE);
		setSCIModeForEstiMode(SCIInterfaceModePorted, estiPORTED_MODE);
		setSCIModeForEstiMode(SCIInterfaceModeHearing, estiHEARING_MODE);
		
#undef setSCIModeForEstiMode
		
		dictionary = [mutableDictionary copy];
	});
	return (SCIInterfaceMode)[dictionary[@(mode)] unsignedIntegerValue];
}

EstiInterfaceMode EstiInterfaceModeForSCIInterfaceMode(SCIInterfaceMode mode)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define setEstiModeForSCIMode( estiMode, sciMode ) \
do { \
mutableDictionary[@((sciMode))] = @((estiMode)); \
} while(0)

		setEstiModeForSCIMode(estiSTANDARD_MODE, SCIInterfaceModeStandard);
		setEstiModeForSCIMode(estiPUBLIC_MODE, SCIInterfaceModePublic);
		setEstiModeForSCIMode(estiKIOSK_MODE, SCIInterfaceModeKiosk);
		setEstiModeForSCIMode(estiINTERPRETER_MODE, SCIInterfaceModeInterpreter);
		setEstiModeForSCIMode(estiTECH_SUPPORT_MODE, SCIInterfaceModeTechSupport);
		setEstiModeForSCIMode(estiVRI_MODE, SCIInterfaceModeVRI);
		setEstiModeForSCIMode(estiABUSIVE_CALLER_MODE, SCIInterfaceModeAbusiveCaller);
		setEstiModeForSCIMode(estiPORTED_MODE, SCIInterfaceModePorted);
		setEstiModeForSCIMode(estiHEARING_MODE, SCIInterfaceModeHearing);
		
#undef setEstiModeForSCIMode
		
		dictionary = [mutableDictionary copy];
	});
	return (EstiInterfaceMode)[dictionary[@(mode)] intValue];
}

EstiDialSource EstiDialSourceForSCIDialSource(SCIDialSource dialSource)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define setEstiSourceForSCISource( estiSource, sciSource ) \
do { \
mutableDictionary[@((sciSource))] = @((estiSource)); \
} while(0)
		
		setEstiSourceForSCISource(estiDS_UNKNOWN, SCIDialSourceUnknown);
		setEstiSourceForSCISource(estiDS_CONTACT, SCIDialSourceContact);
		setEstiSourceForSCISource(estiDS_CALL_HISTORY, SCIDialSourceCallHistory);
		setEstiSourceForSCISource(estiDS_SIGNMAIL, SCIDialSourceSignMail);
		setEstiSourceForSCISource(estiDS_FAVORITE, SCIDialSourceFavorite);
		setEstiSourceForSCISource(estiDS_AD_HOC, SCIDialSourceAdHoc);
		setEstiSourceForSCISource(estiDS_VRCL, SCIDialSourceVRCL);
		setEstiSourceForSCISource(estiDS_SVRS_BUTTON, SCIDialSourceSVRSButton);
		setEstiSourceForSCISource(estiDS_RECENT_CALLS, SCIDialSourceRecentCalls);
		setEstiSourceForSCISource(estiDS_INTERNET_SEARCH, SCIDialSourceInternetSearch);
		setEstiSourceForSCISource(estiDS_WEB_DIAL, SCIDialSourceWebDial);
		setEstiSourceForSCISource(estiDS_PUSH_NOTIFICATION_MISSED_CALL, SCIDialSourcePushNotificationMissedCall);
		setEstiSourceForSCISource(estiDS_PUSH_NOTIFICATION_SIGNMAIL_CALL, SCIDialSourcePushNotificationSignMailCall);
		setEstiSourceForSCISource(estiDS_LDAP, SCIDialSourceLDAP);
		setEstiSourceForSCISource(estiDS_FAST_SEARCH, SCIDialSourceFastSearch);
		setEstiSourceForSCISource(estiDS_MYPHONE, SCIDialSourceMyPhone);
		setEstiSourceForSCISource(estiDS_UI_BUTTON, SCIDialSourceUIButton);
		setEstiSourceForSCISource(estiDS_SVRS_ENGLISH_CONTACT, SCIDialSourceSVRSEnglishContact);
		setEstiSourceForSCISource(estiDS_SVRS_SPANISH_CONTACT, SCIDialSourceSVRSSpanishContact);
		setEstiSourceForSCISource(estiDS_DEVICE_CONTACT, SCIDialSourceDeviceContact);
		setEstiSourceForSCISource(estiDS_DIRECT_SIGNMAIL, SCIDialSourceDirectSignMail);
		setEstiSourceForSCISource(estiDS_DIRECT_SIGNMAIL_FAILURE, SCIDialSourceDirectSignMailFailure);
		setEstiSourceForSCISource(estiDS_CALL_HISTORY_DISCONNECTED, SCIDialSourceCallHistoryDisconnected);
		setEstiSourceForSCISource(estiDS_RECENT_CALLS_DISCONNECTED, SCIDialSourceRecentCallsDisconnected);
		setEstiSourceForSCISource(estiDS_NATIVE_DIALER, SCIDialSourceNativeDialer);
		setEstiSourceForSCISource(estiDS_BLOCKLIST, SCIDialSourceBlockList);
		setEstiSourceForSCISource(estiDS_CALL_WITH_ENCRYPTION, SCIDialSourceCallWithEncryption);
		setEstiSourceForSCISource(estiDS_DIRECTORY, SCIDialSourceDirectory);
		setEstiSourceForSCISource(estiDS_SPANISH_AD_HOC, SCIDialSourceSpanishAdHoc);

#undef setEstiSourceForSCISource
		
		dictionary = [mutableDictionary copy];
	});
	return (EstiDialSource)[dictionary[@(dialSource)] intValue];
}

EstiDialMethod EstiDialMethodForSCIDialMethod(SCIDialMethod dialMethod)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define setEstiMethodForSCIMethod( estiMethod, sciMethod ) \
do { \
mutableDictionary[@((sciMethod))] = @((estiMethod)); \
} while(0)
		
		setEstiMethodForSCIMethod(estiDM_BY_DIAL_STRING, SCIDialMethodByDialString);
		setEstiMethodForSCIMethod(estiDM_BY_DS_PHONE_NUMBER, SCIDialMethodByDSPhoneNumber);
		setEstiMethodForSCIMethod(estiDM_BY_VRS_PHONE_NUMBER, SCIDialMethodByVRSPhoneNumber);
		setEstiMethodForSCIMethod(estiDM_BY_VRS_WITH_VCO, SCIDialMethodByVRSWithVCO);
		setEstiMethodForSCIMethod(estiDM_UNKNOWN, SCIDialMethodUnknown);
		setEstiMethodForSCIMethod(estiDM_BY_OTHER_VRS_PROVIDER, SCIDialMethodByOtherVRSProvider);
		setEstiMethodForSCIMethod(estiDM_UNKNOWN_WITH_VCO, SCIDialMethodUnknownWithVCO);
		setEstiMethodForSCIMethod(estiDM_BY_VRS_DISCONNECTED, SCIDialMethodVRSDisconnected);
		
#undef setEstiMethodForSCIMethod
		
		dictionary = [mutableDictionary copy];
	});
	return (EstiDialMethod)[dictionary[@(dialMethod)] intValue];
}

SCIDialMethod SCIDialMethodForEstiDialMethod(EstiDialMethod dialMethod)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
#define setSCIMethodForEstiMethod( sciMethod, estiMethod ) \
do { \
mutableDictionary[@((estiMethod))] = @((sciMethod)); \
} while(0)
		
		setSCIMethodForEstiMethod(SCIDialMethodByDialString, estiDM_BY_DIAL_STRING );
		setSCIMethodForEstiMethod(SCIDialMethodByDSPhoneNumber, estiDM_BY_DS_PHONE_NUMBER );
		setSCIMethodForEstiMethod(SCIDialMethodByVRSPhoneNumber, estiDM_BY_VRS_PHONE_NUMBER);
		setSCIMethodForEstiMethod(SCIDialMethodByVRSWithVCO, estiDM_BY_VRS_WITH_VCO);
		setSCIMethodForEstiMethod(SCIDialMethodUnknown, estiDM_UNKNOWN);
		setSCIMethodForEstiMethod(SCIDialMethodByOtherVRSProvider, estiDM_BY_OTHER_VRS_PROVIDER);
		setSCIMethodForEstiMethod(SCIDialMethodUnknownWithVCO, estiDM_UNKNOWN_WITH_VCO);
		setSCIMethodForEstiMethod(SCIDialMethodVRSDisconnected, estiDM_BY_VRS_DISCONNECTED);
		
#undef setEstiMethodForSCIMethod
		
		dictionary = [mutableDictionary copy];
	});
	return (SCIDialMethod)[dictionary[@(dialMethod)] intValue];
}

SCICoreResponseErrorCode SCICoreResponseErrorCodeFromECoreError(CstiCoreResponse::ECoreError coreError)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetCoreResponseErrorCodeForECoreError( coreResponseErrorCode , eCoreError ) \
do { \
	mutableDictionary[@((eCoreError))] = @((coreResponseErrorCode)); \
} while(0)
		
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeConnectionError , CstiCoreResponse::eCONNECTION_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoError , CstiCoreResponse::eNO_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNotLoggedIn , CstiCoreResponse::eNOT_LOGGED_IN );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUsernameOrPasswordInvalid , CstiCoreResponse::eUSERNAME_OR_PASSWORD_INVALID );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePhoneNumberNotFound , CstiCoreResponse::ePHONE_NUMBER_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeDuplicatePhoneNumber , CstiCoreResponse::eDUPLICATE_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorCreatingUser , CstiCoreResponse::eERROR_CREATING_USER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorAssociatingPhoneNumber , CstiCoreResponse::eERROR_ASSOCIATING_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotAssociatePhoneNumber , CstiCoreResponse::eCANNOT_ASSOCIATE_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorDisassociatingPhoneNumber , CstiCoreResponse::eERROR_DISASSOCIATING_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotDisassociatePhoneNumber , CstiCoreResponse::eCANNOT_DISASSOCIATE_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorUpdatingUser , CstiCoreResponse::eERROR_UPDATING_USER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUniqueIdUnrecognized , CstiCoreResponse::eUNIQUE_ID_UNRECOGNIZED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePhoneNumberNotRegistered , CstiCoreResponse::ePHONE_NUMBER_NOT_REGISTERED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeDuplicateUniqueId , CstiCoreResponse::eDUPLICATE_UNIQUE_ID );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeConnectionToSignmailDown , CstiCoreResponse::eCONNECTION_TO_SIGNMAIL_DOWN );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoInformationSent , CstiCoreResponse::eNO_INFORMATION_SENT );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUniqueIdEmpty , CstiCoreResponse::eUNIQUE_ID_EMPTY );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRealNumberListReleaseFailed , CstiCoreResponse::eREAL_NUMBER_LIST_RELEASE_FAILED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeReleaseNumbersNotSpecified , CstiCoreResponse::eRELEASE_NUMBERS_NOT_SPECIFIED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCountAttributeInvalid , CstiCoreResponse::eCOUNT_ATTRIBUTE_INVALID );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRealNumberListGetFailed , CstiCoreResponse::eREAL_NUMBER_LIST_GET_FAILED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRealNumberServiceUnavailable , CstiCoreResponse::eREAL_NUMBER_SERVICE_UNAVAILABLE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNotPrimaryUserOnPhone , CstiCoreResponse::eNOT_PRIMARY_USER_ON_PHONE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePrimaryUserOnAnotherPhone , CstiCoreResponse::ePRIMARY_USER_ON_ANOTHER_PHONE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeProvisionRequestFailed , CstiCoreResponse::ePROVISION_REQUEST_FAILED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoCurrentEmergencyStatusFound , CstiCoreResponse::eNO_CURRENT_EMERGENCY_STATUS_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoCurrentEmergencyAddressFound , CstiCoreResponse::eNO_CURRENT_EMERGENCY_ADDRESS_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeEmergencyAddressDataBlank , CstiCoreResponse::eEMERGENCY_ADDRESS_DATA_BLANK );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeDeprovisionRequestFailed , CstiCoreResponse::eDEPROVISION_REQUEST_FAILED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUnableToContactDatabase , CstiCoreResponse::eUNABLE_TO_CONTACT_DATABASE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoRealNumberAvailable , CstiCoreResponse::eNO_REAL_NUMBER_AVAILABLE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUnableToAcquireNumber , CstiCoreResponse::eUNABLE_TO_ACQUIRE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoPendingNumberAvailable , CstiCoreResponse::eNO_PENDING_NUMBER_AVAILABLE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRouterInfoNotCleared , CstiCoreResponse::eROUTER_INFO_NOT_CLEARED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoPhoneInfoFound , CstiCoreResponse::eNO_PHONE_INFO_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeQicDoesNotExist , CstiCoreResponse::eQIC_DOES_NOT_EXIST );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeQicCannotBeReached , CstiCoreResponse::eQIC_CANNOT_BE_REACHED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeQicNotDeclared , CstiCoreResponse::eQIC_NOT_DECLARED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeImageProblem , CstiCoreResponse::eIMAGE_PROBLEM );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeImageProblemSavingData , CstiCoreResponse::eIMAGE_PROBLEM_SAVING_DATA );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeImageProblemDeleting , CstiCoreResponse::eIMAGE_PROBLEM_DELETING );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeImageListGetProblem , CstiCoreResponse::eIMAGE_LIST_GET_PROBLEM );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeEndpointMatchError , CstiCoreResponse::eENDPOINT_MATCH_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeBlockListItemDenied , CstiCoreResponse::eBLOCK_LIST_ITEM_DENIED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCallListPlanNotFound , CstiCoreResponse::eCALL_LIST_PLAN_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeEndpointTypeNotFound , CstiCoreResponse::eENDPOINT_TYPE_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePhoneTypeNotRecognized , CstiCoreResponse::ePHONE_TYPE_NOT_RECOGNIZED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotGetServiceContactOverrides , CstiCoreResponse::eCANNOT_GET_SERVICE_CONTACT_OVERRIDES );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotContactItrs , CstiCoreResponse::eCANNOT_CONTACT_ITRS );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUriListSetMismatch , CstiCoreResponse::eURI_LIST_SET_MISMATCH );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUriListSetDeleteError , CstiCoreResponse::eURI_LIST_SET_DELETE_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUriListSetAddError , CstiCoreResponse::eURI_LIST_SET_ADD_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeSignmailRegisterError , CstiCoreResponse::eSIGNMAIL_REGISTER_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeAddedMoreContactsThanAllowed , CstiCoreResponse::eADDED_MORE_CONTACTS_THAN_ALLOWED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotDetermineWhichLogin , CstiCoreResponse::eCANNOT_DETERMINE_WHICH_LOGIN );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeLoginValueInvalid , CstiCoreResponse::eLOGIN_VALUE_INVALID );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorUpdatingAlias , CstiCoreResponse::eERROR_UPDATING_ALIAS );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorCreatingGroup , CstiCoreResponse::eERROR_CREATING_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeInvalidPin , CstiCoreResponse::eINVALID_PIN );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUserCannotCreateRinggroup , CstiCoreResponse::eUSER_CANNOT_CREATE_RINGGROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeEndpointNotRinggroupCapable , CstiCoreResponse::eENDPOINT_NOT_RINGGROUP_CAPABLE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCallListItemNotOwned , CstiCoreResponse::eCALL_LIST_ITEM_NOT_OWNED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeVideoServerUnreachable , CstiCoreResponse::eVIDEO_SERVER_UNREACHABLE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotUpdateEnumServer , CstiCoreResponse::eCANNOT_UPDATE_ENUM_SERVER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCallListItemNotFound , CstiCoreResponse::eCALL_LIST_ITEM_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorRollingBackRingGroup , CstiCoreResponse::eERROR_ROLLING_BACK_RING_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoInviteFound , CstiCoreResponse::eNO_INVITE_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNotMemberOfRingGroup , CstiCoreResponse::eNOT_MEMBER_OF_RING_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotInvitePhoneNumberToGroup , CstiCoreResponse::eCANNOT_INVITE_PHONE_NUMBER_TO_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotInviteUser , CstiCoreResponse::eCANNOT_INVITE_USER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRingGroupDoesntExist , CstiCoreResponse::eRING_GROUP_DOESNT_EXIST );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRingGroupNumberNotFound , CstiCoreResponse::eRING_GROUP_NUMBER_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRingGroupInviteAcceptNotCompleted , CstiCoreResponse::eRING_GROUP_INVITE_ACCEPT_NOT_COMPLETED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRingGroupInviteRejectNotCompleted , CstiCoreResponse::eRING_GROUP_INVITE_REJECT_NOT_COMPLETED );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRingGroupNumberCannotBeEmpty , CstiCoreResponse::eRING_GROUP_NUMBER_CANNOT_BE_EMPTY );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeInvalidDescriptionLength , CstiCoreResponse::eINVALID_DESCRIPTION_LENGTH );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUserNotFound , CstiCoreResponse::eUSER_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeInvalidUser , CstiCoreResponse::eINVALID_USER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeGroupUserNotFound , CstiCoreResponse::eGROUP_USER_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUserBeingRemovedNotInGroup , CstiCoreResponse::eUSER_BEING_REMOVED_NOT_IN_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUserCannotBeRemovedFromGroup , CstiCoreResponse::eUSER_CANNOT_BE_REMOVED_FROM_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUserNotMemberOfGroup , CstiCoreResponse::eUSER_NOT_MEMBER_OF_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeElementHasInvalidPhoneNumber , CstiCoreResponse::eELEMENT_HAS_INVALID_PHONE_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePasswordAndConfirmDoNotMatch , CstiCoreResponse::ePASSWORD_AND_CONFIRM_DO_NOT_MATCH );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotBlockRingGroupMember , CstiCoreResponse::eCANNOT_BLOCK_RING_GROUP_MEMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotRemoveLastUserFromGroup , CstiCoreResponse::eCANNOT_REMOVE_LAST_USER_FROM_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUriListMustContainItrsUris , CstiCoreResponse::eURI_LIST_MUST_CONTAIN_ITRS_URIS );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeInvalidURI , CstiCoreResponse::eINVALID_URI );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRedirectToAndFromUrisCannotBeSame , CstiCoreResponse::eREDIRECT_TO_AND_FROM_URIS_CANNOT_BE_SAME );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeRemoveOtherMembersBeforeRemovingRingGroup , CstiCoreResponse::eREMOVE_OTHER_MEMBERS_BEFORE_REMOVING_RING_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNeedOneMemberInRingGroup , CstiCoreResponse::eNEED_ONE_MEMBER_IN_RING_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCouldNotLocateActiveLocalNumber , CstiCoreResponse::eCOULD_NOT_LOCATE_ACTIVE_LOCAL_NUMBER );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUnknownError2 , CstiCoreResponse::eUNKNOWN_ERROR_2 );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeInvalidGroupCredentials , CstiCoreResponse::eINVALID_GROUP_CREDENTIALS );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNotMemberOfThisGroup , CstiCoreResponse::eNOT_MEMBER_OF_THIS_GROUP );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorSettingGroupPassword , CstiCoreResponse::eERROR_SETTING_GROUP_PASSWORD );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodePhoneNumberCannotBeEmpty , CstiCoreResponse::ePHONE_NUMBER_CANNOT_BE_EMPTY );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorWhileSettingRingGroupInfo , CstiCoreResponse::eERROR_WHILE_SETTING_RING_GROUP_INFO );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeDialStringRequiredElement , CstiCoreResponse::eERROR_DIAL_STRING_REQUIRED_ELEMENT );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCannotRemoveRingGroupCreator , CstiCoreResponse::eERROR_CANNOT_REMOVE_RING_GROUP_CREATOR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeImageNotFound , CstiCoreResponse::eERROR_IMAGE_NOT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeNoAgreementFound , CstiCoreResponse::eERROR_NO_AGREEMENT_FOUND );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorUpdatingAgreement , CstiCoreResponse::eERROR_UPDATING_AGREEMENT );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeGuidIncorrectFormat , CstiCoreResponse::eERROR_GUID_INCORRECT_FORMAT );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeCoreConfiguration , CstiCoreResponse::eERROR_CORE_CONFIGURATION );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeErrorRemovingImage , CstiCoreResponse::eERROR_REMOVING_IMAGE );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUnableSetSetting , CstiCoreResponse::eERROR_UNABLE_SET_SETTING );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeOfflineAuthentication , CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED );
		
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeDatabaseConnectionError , CstiCoreResponse::eDATABASE_CONNECTION_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeXMLValidationError , CstiCoreResponse::eXML_VALIDATION_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeXMLFormatError , CstiCoreResponse::eXML_FORMAT_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeGenericServerError , CstiCoreResponse::eGENERIC_SERVER_ERROR );
		SetCoreResponseErrorCodeForECoreError( SCICoreResponseErrorCodeUnknownError , CstiCoreResponse::eUNKNOWN_ERROR );
		
#undef SetCoreResponseErrorCodeForECoreError
		
		dictionary = [mutableDictionary copy];
	});
	
	return (SCICoreResponseErrorCode)[dictionary[@(coreError)] unsignedIntegerValue];
}

NSString *NSStringFromSCICoreResponseErrorCode(SCICoreResponseErrorCode error)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddError(error) \
do { \
mutableDictionary[@((error))] = [NSString stringWithUTF8String:( #error )]; \
} while(0)
		
		AddError( SCICoreResponseErrorCodeConnectionError );
		AddError( SCICoreResponseErrorCodeNoError );
		AddError( SCICoreResponseErrorCodeNotLoggedIn );
		AddError( SCICoreResponseErrorCodeUsernameOrPasswordInvalid );
		AddError( SCICoreResponseErrorCodePhoneNumberNotFound );
		AddError( SCICoreResponseErrorCodeDuplicatePhoneNumber );
		AddError( SCICoreResponseErrorCodeErrorCreatingUser );
		AddError( SCICoreResponseErrorCodeErrorAssociatingPhoneNumber );
		AddError( SCICoreResponseErrorCodeCannotAssociatePhoneNumber );
		AddError( SCICoreResponseErrorCodeErrorDisassociatingPhoneNumber );
		AddError( SCICoreResponseErrorCodeCannotDisassociatePhoneNumber );
		AddError( SCICoreResponseErrorCodeErrorUpdatingUser );
		AddError( SCICoreResponseErrorCodeUniqueIdUnrecognized );
		AddError( SCICoreResponseErrorCodePhoneNumberNotRegistered );
		AddError( SCICoreResponseErrorCodeDuplicateUniqueId );
		AddError( SCICoreResponseErrorCodeConnectionToSignmailDown );
		AddError( SCICoreResponseErrorCodeNoInformationSent );
		AddError( SCICoreResponseErrorCodeUniqueIdEmpty );
		AddError( SCICoreResponseErrorCodeRealNumberListReleaseFailed );
		AddError( SCICoreResponseErrorCodeReleaseNumbersNotSpecified );
		AddError( SCICoreResponseErrorCodeCountAttributeInvalid );
		AddError( SCICoreResponseErrorCodeRealNumberListGetFailed );
		AddError( SCICoreResponseErrorCodeRealNumberServiceUnavailable );
		AddError( SCICoreResponseErrorCodeNotPrimaryUserOnPhone );
		AddError( SCICoreResponseErrorCodePrimaryUserOnAnotherPhone );
		AddError( SCICoreResponseErrorCodeProvisionRequestFailed );
		AddError( SCICoreResponseErrorCodeNoCurrentEmergencyStatusFound );
		AddError( SCICoreResponseErrorCodeNoCurrentEmergencyAddressFound );
		AddError( SCICoreResponseErrorCodeEmergencyAddressDataBlank );
		AddError( SCICoreResponseErrorCodeDeprovisionRequestFailed );
		AddError( SCICoreResponseErrorCodeUnableToContactDatabase );
		AddError( SCICoreResponseErrorCodeNoRealNumberAvailable );
		AddError( SCICoreResponseErrorCodeUnableToAcquireNumber );
		AddError( SCICoreResponseErrorCodeNoPendingNumberAvailable );
		AddError( SCICoreResponseErrorCodeRouterInfoNotCleared );
		AddError( SCICoreResponseErrorCodeNoPhoneInfoFound );
		AddError( SCICoreResponseErrorCodeQicDoesNotExist );
		AddError( SCICoreResponseErrorCodeQicCannotBeReached );
		AddError( SCICoreResponseErrorCodeQicNotDeclared );
		AddError( SCICoreResponseErrorCodeImageProblem );
		AddError( SCICoreResponseErrorCodeImageProblemSavingData );
		AddError( SCICoreResponseErrorCodeImageProblemDeleting );
		AddError( SCICoreResponseErrorCodeImageListGetProblem );
		AddError( SCICoreResponseErrorCodeEndpointMatchError );
		AddError( SCICoreResponseErrorCodeBlockListItemDenied );
		AddError( SCICoreResponseErrorCodeCallListPlanNotFound );
		AddError( SCICoreResponseErrorCodeEndpointTypeNotFound );
		AddError( SCICoreResponseErrorCodePhoneTypeNotRecognized );
		AddError( SCICoreResponseErrorCodeCannotGetServiceContactOverrides );
		AddError( SCICoreResponseErrorCodeCannotContactItrs );
		AddError( SCICoreResponseErrorCodeUriListSetMismatch );
		AddError( SCICoreResponseErrorCodeUriListSetDeleteError );
		AddError( SCICoreResponseErrorCodeUriListSetAddError );
		AddError( SCICoreResponseErrorCodeSignmailRegisterError );
		AddError( SCICoreResponseErrorCodeAddedMoreContactsThanAllowed );
		AddError( SCICoreResponseErrorCodeCannotDetermineWhichLogin );
		AddError( SCICoreResponseErrorCodeLoginValueInvalid );
		AddError( SCICoreResponseErrorCodeErrorUpdatingAlias );
		AddError( SCICoreResponseErrorCodeErrorCreatingGroup );
		AddError( SCICoreResponseErrorCodeInvalidPin );
		AddError( SCICoreResponseErrorCodeUserCannotCreateRinggroup );
		AddError( SCICoreResponseErrorCodeEndpointNotRinggroupCapable );
		AddError( SCICoreResponseErrorCodeCallListItemNotOwned );
		AddError( SCICoreResponseErrorCodeVideoServerUnreachable );
		AddError( SCICoreResponseErrorCodeCannotUpdateEnumServer );
		AddError( SCICoreResponseErrorCodeCallListItemNotFound );
		AddError( SCICoreResponseErrorCodeErrorRollingBackRingGroup );
		AddError( SCICoreResponseErrorCodeNoInviteFound );
		AddError( SCICoreResponseErrorCodeNotMemberOfRingGroup );
		AddError( SCICoreResponseErrorCodeCannotInvitePhoneNumberToGroup );
		AddError( SCICoreResponseErrorCodeCannotInviteUser );
		AddError( SCICoreResponseErrorCodeRingGroupDoesntExist );
		AddError( SCICoreResponseErrorCodeRingGroupNumberNotFound );
		AddError( SCICoreResponseErrorCodeRingGroupInviteAcceptNotCompleted );
		AddError( SCICoreResponseErrorCodeRingGroupInviteRejectNotCompleted );
		AddError( SCICoreResponseErrorCodeRingGroupNumberCannotBeEmpty );
		AddError( SCICoreResponseErrorCodeInvalidDescriptionLength );
		AddError( SCICoreResponseErrorCodeUserNotFound );
		AddError( SCICoreResponseErrorCodeInvalidUser );
		AddError( SCICoreResponseErrorCodeGroupUserNotFound );
		AddError( SCICoreResponseErrorCodeUserBeingRemovedNotInGroup );
		AddError( SCICoreResponseErrorCodeUserCannotBeRemovedFromGroup );
		AddError( SCICoreResponseErrorCodeUserNotMemberOfGroup );
		AddError( SCICoreResponseErrorCodeElementHasInvalidPhoneNumber );
		AddError( SCICoreResponseErrorCodePasswordAndConfirmDoNotMatch );
		AddError( SCICoreResponseErrorCodeCannotBlockRingGroupMember );
		AddError( SCICoreResponseErrorCodeCannotRemoveLastUserFromGroup );
		AddError( SCICoreResponseErrorCodeUriListMustContainItrsUris );
		AddError( SCICoreResponseErrorCodeInvalidURI );
		AddError( SCICoreResponseErrorCodeRedirectToAndFromUrisCannotBeSame );
		AddError( SCICoreResponseErrorCodeRemoveOtherMembersBeforeRemovingRingGroup );
		AddError( SCICoreResponseErrorCodeNeedOneMemberInRingGroup );
		AddError( SCICoreResponseErrorCodeCouldNotLocateActiveLocalNumber );
		AddError( SCICoreResponseErrorCodeUnknownError2 );
		AddError( SCICoreResponseErrorCodeInvalidGroupCredentials );
		AddError( SCICoreResponseErrorCodeNotMemberOfThisGroup );
		AddError( SCICoreResponseErrorCodeErrorSettingGroupPassword );
		AddError( SCICoreResponseErrorCodePhoneNumberCannotBeEmpty );
		AddError( SCICoreResponseErrorCodeErrorWhileSettingRingGroupInfo );
		AddError( SCICoreResponseErrorCodeDialStringRequiredElement );
		AddError( SCICoreResponseErrorCodeCannotRemoveRingGroupCreator );
		AddError( SCICoreResponseErrorCodeImageNotFound );
		AddError( SCICoreResponseErrorCodeNoAgreementFound );
		AddError( SCICoreResponseErrorCodeErrorUpdatingAgreement );
		AddError( SCICoreResponseErrorCodeGuidIncorrectFormat );
		AddError( SCICoreResponseErrorCodeCoreConfiguration );
		AddError( SCICoreResponseErrorCodeErrorRemovingImage );
		AddError( SCICoreResponseErrorCodeUnableSetSetting );
		AddError( SCICoreResponseErrorCodeDatabaseConnectionError );
		AddError( SCICoreResponseErrorCodeXMLValidationError );
		AddError( SCICoreResponseErrorCodeXMLFormatError );
		AddError( SCICoreResponseErrorCodeGenericServerError );
		AddError( SCICoreResponseErrorCodeUnknownError );
		
#undef AddError
		
		dictionary = [mutableDictionary copy];
	});
	return dictionary[@(error)];
}

NSString *NSStringFromSCIVideophoneEngineErrorCode(SCIVideophoneEngineErrorCode error)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddError(error) \
do { \
mutableDictionary[@((error))] = [NSString stringWithUTF8String:( #error )]; \
} while(0)
		
		AddError( SCIVideophoneEngineErrorCodeNoError );
		AddError( SCIVideophoneEngineErrorCodeEngineNotStarted );
		AddError( SCIVideophoneEngineErrorCodeEmptyDialString );
		AddError( SCIVideophoneEngineErrorCodeEmptyCallList );
		AddError( SCIVideophoneEngineErrorCodeNullDefaultUserSettingsList );
		AddError( SCIVideophoneEngineErrorCodeFailedToLoadUserAccountInfo );
		AddError( SCIVideophoneEngineErrorCodeNullUserAccountInfo );
		AddError( SCIVideophoneEngineErrorCodeEmptyUserSettingsList );
		AddError( SCIVideophoneEngineErrorCodeNullUserSettingsList );
		AddError( SCIVideophoneEngineErrorCodeEmptyPhoneSettingsList );
		AddError( SCIVideophoneEngineErrorCodeNullPhoneSettingsList );
		AddError( SCIVideophoneEngineErrorCodeNoError );
		AddError( SCIVideophoneEngineErrorCodeEngineNotStarted );
		
#undef AddError
		
		dictionary = [mutableDictionary copy];
	});
	return dictionary[@(error)];
}

IstiPlatform::EstiRestartReason IstiPlatformEstiRestartReasonFromSCIRestartReason(SCIRestartReason reason)
{
    static NSDictionary *dictionary = nil;
	
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetEstiRestartReasonForSCIRestartReason( eRestartReason , restartReason ) \
do { \
mutableDictionary[@((restartReason))] = @((eRestartReason));\
} while(0)
		
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_UNKNOWN , SCIRestartReasonUnknown );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_MEDIA_LOSS , SCIRestartReasonMediaLoss );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_VRCL_COMMAND , SCIRestartReasonVRCL );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_UPDATE , SCIRestartReasonUpdate );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_CRASH , SCIRestartReasonCrash );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_ACCOUNT_TRANSFER , SCIRestartReasonAccountTransfer );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_DNS_CHANGE , SCIRestartReasonDNSChange );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_STATE_NOTIFY_EVENT , SCIRestartReasonStateNotifyEvent );
        SetEstiRestartReasonForSCIRestartReason( IstiPlatform::estiRESTART_REASON_TERMINATED , SCIRestartReasonTerminated );
		
#undef SetEstiRestartReasonForSCIRestartReason
		
        dictionary = [mutableDictionary copy];
    });
	
	IstiPlatform::EstiRestartReason res = IstiPlatform::estiRESTART_REASON_UNKNOWN;
    NSNumber *resNumber = dictionary[@(reason)];
    if (resNumber) {
        res = (IstiPlatform::EstiRestartReason)[resNumber intValue];
    } else {
        res = IstiPlatform::estiRESTART_REASON_UNKNOWN;
    }
    return res;
}


