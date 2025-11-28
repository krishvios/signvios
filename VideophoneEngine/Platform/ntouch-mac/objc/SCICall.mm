//
//  SCICall.mm
//  ntouchMac
//
//  Created by Adam Preble on 3/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICall.h"
#import "stiDefs.h"
#import "stiSVX.h"
#import "CstiCall.h"
#import "ICallInfo.h"
#import "SCICallCpp.h"
#import "SCICallStatistics.h"
#import "SCICallStatistics+SstiCallStatistics.h"
#import "IstiVideoInput.h"
#import "IstiAudioInput.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCISignMailGreetingInfo_Cpp.h"
#import "NSMutableString+SCIAdditions.h"
#import "NSString+SCIAdditions.h"
#import "SCICall+DTMF.h"
#import "SCICallResultCpp.h"

@interface SCICall () {
	std::shared_ptr<IstiCall> mCall;
	CstiSignalConnection::Collection mSignalConnections;
}
@property (nonatomic, readwrite) NSMutableString *sentText;
@property (nonatomic, readwrite) NSMutableString *receivedText;
@end

@implementation SCICall

#pragma mark - Factory Methods

+ (SCICall*)callWithIstiCall:(IstiCall*)call
{
    return [[SCICall alloc] initWithIstiCall:call];
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.sentText = [[NSMutableString alloc] init];
		self.receivedText = [[NSMutableString alloc] init];
		self.textHidden = YES;
	}
	return self;
}

- (id)initWithIstiCall:(IstiCall*)call
{
	self = [self init];
    if (self)
	{
        mCall = call->sharedPointerGet();

		__weak SCICall *weakSelf = self;
		mSignalConnections.push_back(mCall->dhviStateChangeSignalGet ().Connect ([weakSelf](IstiCall::DhviState dhState)
		{
			weakSelf.dhviState = (DhviState)dhState;
		}));
		
		self.dhviState = (DhviState)mCall->dhviStateGet ();
    }
    return self;
}

- (void)dealloc
{
    mCall = NULL;
}

- (IstiCall *)istiCall
{
	return mCall.get();
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@ %p name=%@ mCall=%p state=0x%x>",
			NSStringFromClass([self class]),
			self,
			self.remoteName,
			mCall.get(),
			mCall ? mCall->StateGet() : 0];
}

- (BOOL)answer
{
    SCILog(@"Start");
    stiHResult result = mCall->Answer();
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)dhviDisconnect
{
    SCILog(@"Start");
    stiHResult result = mCall->dhviMcuDisconnect();
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)continueDial
{
    SCILog(@"Start");
    stiHResult result = mCall->ContinueDial();
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)hangUp
{
    SCILog(@"Start");
	
    stiHResult result = mCall->HangUp();
	
	// Don't call -[SCIVideophoneEngine removeCall:] here. The call may not get into the right state before
	// the state is checked in CallStorage::Remove(), which will result in it not being removed.
	// Instead, we removeCall: in estiMSG_DISCONNECTED.
	
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)hold
{
    SCILog(@"Start");
    stiHResult result = mCall->Hold();
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)resume
{
    SCILog(@"Start");
    stiHResult result = mCall->Resume();
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)reject
{
    SCILog(@"Start");
    stiHResult result = mCall->Reject(estiRESULT_LOCAL_SYSTEM_BUSY);
    return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (BOOL)transfer:(NSString *)phoneNumber error:(NSError **)error
{
    SCILog(@"Start");
	stiHResult result = stiRESULT_ERROR;
	result = mCall->Transfer(std::string(phoneNumber.UTF8String, [phoneNumber lengthOfBytesUsingEncoding:NSUTF8StringEncoding]));
	
	if (stiIS_ERROR(result) && error != nil) {
		NSString *failureReason;
		stiHResult resultCode = stiRESULT_CODE(result);
		switch (resultCode) {
		case stiRESULT_UNABLE_TO_TRANSFER_TO_911:
			failureReason = @"You cannot transfer a call to 911.";
			break;
			
		case stiRESULT_ERROR:
		default:
			failureReason = @"Your call could not be transferred to the specified phone number.";
			break;
		}
		
		NSDictionary *userInfo = @{
			NSLocalizedDescriptionKey: @"Could not transfer call.",
			NSLocalizedFailureReasonErrorKey: failureReason
		};
		
		*error = [NSError errorWithDomain:SCIErrorDomainVideophoneEngine code:resultCode userInfo:userInfo];
	}
	
	return !stiIS_ERROR(result);
}

- (BOOL)nudge
{
	SCILog(@"Start");
	stiHResult result = mCall->RemoteLightRingFlash();
	return (stiRESULT_SUCCESS == result) ? YES : NO;
}

- (SCICallStatistics *)statistics
{
	SstiCallStatistics sstiCallStatistics;
	memset(&sstiCallStatistics, 0, sizeof(sstiCallStatistics));
	mCall->StatisticsGet(&sstiCallStatistics);
	return [SCICallStatistics callStatisticsWithSstiCallStatistics:sstiCallStatistics];
}

- (NSString *)remoteName
{
	std::string value;
	mCall->RemoteNameGet(&value);
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)remoteAlternateName
{
	std::string value;
	mCall->RemoteAlternateNameGet(&value);
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)localPreferredLanguage
{
	return [NSString stringWithUTF8String:mCall->LocalPreferredLanguageGet().c_str()];
}

- (NSString *)calledName
{
	std::string value;
	mCall->CalledNameGet(&value);
	return [NSString stringWithUTF8String:value.c_str()];
}

- (BOOL)isDirectSignMail
{
	return mCall->directSignMailGet() ? YES : NO;
}

- (NSString *)callId
{
	std::string value;
	mCall->CallIdGet(&value);
	return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)dialString
{
	EstiDialMethod eDialMethod; std::string value;
	stiHResult hResult = mCall->DialStringGet(&eDialMethod, &value); hResult = hResult;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)newDialString
{
	std::string value;
	mCall->NewDialStringGet(&value);
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)intendedDialString
{
	std::string value;
	mCall->IntendedDialStringGet(&value);
    return [NSString stringWithUTF8String:value.c_str()];
}

- (SCIDialMethod)dialMethod
{
	return SCIDialMethodForEstiDialMethod(mCall->DialMethodGet());
}

- (NSString *)remoteIpAddress
{
	return [NSString stringWithUTF8String:mCall->RemoteIpAddressGet().c_str()];
}

- (NSTimeInterval)callDuration
{
	NSTimeInterval duration = mCall->CallDurationGet();
    return duration;
}

- (NSString *)preferredPhoneNumber
{
	std::string value;
    if (mCall->RemoteCallInfoGet() && mCall->RemoteCallInfoGet()->UserPhoneNumbersGet())
    	value = mCall->RemoteCallInfoGet()->UserPhoneNumbersGet()->szPreferredPhoneNumber;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)sorensonPhoneNumber
{
	std::string value;
    if (mCall->RemoteCallInfoGet() && mCall->RemoteCallInfoGet()->UserPhoneNumbersGet())
    	value = mCall->RemoteCallInfoGet()->UserPhoneNumbersGet()->szSorensonPhoneNumber;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)tollFreePhoneNumber
{
	std::string value;
    if (mCall->RemoteCallInfoGet() && mCall->RemoteCallInfoGet()->UserPhoneNumbersGet())
    	value = mCall->RemoteCallInfoGet()->UserPhoneNumbersGet()->szTollFreePhoneNumber;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)localPhoneNumber
{
	std::string value;
    if (mCall->RemoteCallInfoGet() && mCall->RemoteCallInfoGet()->UserPhoneNumbersGet())
    	value = mCall->RemoteCallInfoGet()->UserPhoneNumbersGet()->szLocalPhoneNumber;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)hearingPhoneNumber
{
	std::string value;
    if (mCall->RemoteCallInfoGet() && mCall->RemoteCallInfoGet()->UserPhoneNumbersGet())
    	value = mCall->RemoteCallInfoGet()->UserPhoneNumbersGet()->szHearingPhoneNumber;
    return [NSString stringWithUTF8String:value.c_str()];
}

- (NSString *)agentId
{
	std::string agentId = mCall->vrsAgentIdGet();
	if (!agentId.empty()) {
		return [NSString stringWithUTF8String:agentId.c_str()];
	}
	else {
		return nil;
	}
}

- (NSString *)messageGreetingURL
{
	NSString *res = nil;
	
#ifdef stiDISABLE_FILE_PLAY
	const char *pszMessageGreetingURL = mCall->MessageGreetingURLGet();
	res = (pszMessageGreetingURL) ? [NSString stringWithUTF8String:pszMessageGreetingURL] : nil;
#endif
	
	return res;
}

- (NSString *)messageGreetingURL2
{
	NSString *res = nil;
	
#ifdef stiDISABLE_FILE_PLAY
	//TODO: Uncomment when this member function is added.
	const char *pszMessageGreetingURL2 = mCall->MessageGreetingURL2Get();
	res = (pszMessageGreetingURL2) ? [NSString stringWithUTF8String:pszMessageGreetingURL2] : nil;
#endif
	
	return res;
}

- (NSString *)messageGreetingText
{
	auto greetingText = mCall->MessageGreetingTextGet();
	return greetingText.empty() ? nil : [NSString stringWithUTF8String:greetingText.c_str()];
}

- (SCISignMailGreetingType)messageGreetingType
{
	SCISignMailGreetingType res = SCISignMailGreetingTypeNone;
	
	eSignMailGreetingType eGreetingType = mCall->MessageGreetingTypeGet();
	res = SCISignMailGreetingTypeFromESignMailGreetingType(eGreetingType);
	
	return res;
}

- (int)messageRecordTime
{
	return mCall->MessageRecordTimeGet();
}

- (SCICallState)state
{
	SCICallState res = SCICallStateUnknown;
	
	EsmiCallState eState = mCall->StateGet();
	res = SCICallStateFromEsmiCallState(eState);
	
	return res;
}

- (SCIEncryptionState)encryptionState
{
	return (SCIEncryptionState)mCall->encryptionStateGet();
}

- (BOOL)isTransferable
{
	BOOL res = NO;
	
	res = (mCall->IsTransferableGet() == estiTRUE)
		&& !self.isHoldRemote
		&& !self.isInterpreter
		&& !self.isHoldServer
		&& !self.isTechSupport
		&& !self.isVRSCall
		&& self.isSIP;
	
	return res;
}

- (BOOL)isPointToPointCall
{
	BOOL res = NO;
	
	EstiDialMethod dialMethod = mCall->DialMethodGet();
	if (dialMethod == estiDM_BY_DIAL_STRING ||
		dialMethod == estiDM_BY_DS_PHONE_NUMBER) {
		res = YES;
	}
	
	return res;
}

- (BOOL)isVRSCall
{
	return (estiDM_BY_VRS_PHONE_NUMBER==mCall->DialMethodGet() || estiDM_BY_VRS_WITH_VCO==mCall->DialMethodGet());
}

- (BOOL)isIncoming
{
	return (estiINCOMING==mCall->DirectionGet());
}

- (BOOL)isOutgoing
{
	return (estiOUTGOING==mCall->DirectionGet());
}

- (BOOL)isSignMailInitiated
{
    return mCall->SignMailInitiatedGet();
}

- (BOOL)isHoldServer
{
	return mCall->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_HOLD_SERVER);
}

- (BOOL)isVideoServer
{
	return mCall->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_VIDEO_SERVER);
}

- (BOOL)isVideoPhone
{
	return mCall->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_VIDEOPHONE);
}

- (BOOL)isSorensonDevice
{
	return mCall->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE);
}

- (BOOL)isInterpreter
{
	return (estiINTERPRETER_MODE==mCall->RemoteInterfaceModeGet());
}

- (BOOL)isTechSupport
{
	
	return (estiTECH_SUPPORT_MODE==mCall->RemoteInterfaceModeGet());
}

- (BOOL)isConnecting
{
	return (mCall->StateGet() & esmiCS_CONNECTING) != 0;
}

- (BOOL)isConnected
{
	return (mCall->StateGet() & esmiCS_CONNECTED) != 0;
}

- (BOOL)isConferencing
{
	return self.isConnected && mCall->ConferencedGet();
}

- (BOOL)isDisconnecting
{
	return (mCall->StateGet() & esmiCS_DISCONNECTING) != 0;
}

- (BOOL)isDisconnected
{
	return (mCall->StateGet() & esmiCS_DISCONNECTED) != 0;
}

- (BOOL)isHoldLocal
{
	return (mCall->StateGet() & esmiCS_HOLD_LCL) != 0;
}

- (BOOL)isHoldRemote
{
	return (mCall->StateGet() & esmiCS_HOLD_RMT) != 0;
}

- (BOOL)isHoldBoth
{
	return (mCall->StateGet() & esmiCS_HOLD_BOTH) != 0;
}

- (BOOL)isHoldEither
{
	return self.isHoldLocal || self.isHoldRemote || self.isHoldBoth;
}

- (BOOL)isHoldable
{
	EstiBool isHoldable = mCall->IsHoldableGet();
	return (isHoldable != estiFALSE);
}

- (BOOL)isVideoPrivacyEnabledRemote
{
	BOOL res = NO;
	
	//If we call VideoPlaybackMutGet when self.isConnecting we will  occasionally deadlock.
	if (self.isConnected){
		res = (mCall->VideoPlaybackMuteGet() == estiON);
	} else {
		//If we call VideoPlaybackMutGet at this point, we will deadlock,
		//so just return NO.
		res = NO;
	}
	
	return res;
}

- (BOOL)isH323
{
	return (self.protocol == SCICallProtocolH323);
}

- (BOOL)isSIP
{
	return (self.protocol == SCICallProtocolSIP);
}

- (BOOL)isConnectedToGVC
{
	return (mCall->ConnectedWithMcuGet(estiMCU_GVC) == estiTRUE) ? YES : NO;
}

- (BOOL)isConnectedToMCU
{
	return (mCall->ConnectedWithMcuGet(estiMCU_ANY) == estiTRUE) ? YES : NO;
}

- (SCICallResult)result
{
	return SCICallResultFromEstiCallResult(mCall->ResultGet());
}

- (UInt32) videoRecordWidth
{
	uint32_t un32Width, un32Height = 0;
	mCall->VideoRecordSizeGet(&un32Width, &un32Height);
	return un32Width;
}

- (UInt32) videoRecordHeight
{
	uint32_t un32Width, un32Height = 0;
	mCall->VideoRecordSizeGet(&un32Width, &un32Height);
	return un32Height;
}

- (BOOL)canSendText
{
	return mCall->TextSupportedGet() == estiTRUE && !self.isHoldEither;
}

- (BOOL)canSendDTMF
{
	return mCall->DtmfSupportedGet() == estiTRUE && !self.isHoldEither;
}

- (BOOL)canShareContact
{
	return mCall->ShareContactSupportedGet() == estiTRUE && !self.isHoldEither;
}

- (BOOL)shareContactWithName:(NSString*)name companyName:(NSString *)companyName dialString:(NSString*)dialString numberType:(SCIContactPhone)numberType
{
	SstiSharedContact sharedContact;
	if (name != nil) {
		sharedContact.Name = [name UTF8String];
	}
	if (companyName != nil) {
		sharedContact.CompanyName = [companyName UTF8String];
	}
	sharedContact.DialString = [dialString UTF8String];
	sharedContact.eNumberType = (EPhoneNumberType)numberType;
	stiHResult result = mCall->ContactShare(sharedContact);
	return (stiRESULT_SUCCESS == result) ? YES : NO;
}

//TODO: remove? undeclared, unused holdover from MstiCall, deprecated by -(SCICallStatistics *)statistics
- (SstiCallStatistics *)getStatistics
{
	static SstiCallStatistics stats;
	mCall->StatisticsGet(&stats);
	return &stats;
}

- (NSString *)protocolName
{
	NSString * res = nil;
	
	switch (mCall->ProtocolGet()) {
		case estiSIP:
			res = SCICallProtocolNameSIP;
			break;
		case estiPROT_UNKNOWN:
		default:
			res = SCICallProtocolNameUnknown;
			break;
	}
	
	return res;
}

- (SCICallProtocol)protocol
{
	SCICallProtocol res = SCICallProtocolUnknown;
	
	EstiProtocol protocol = mCall->ProtocolGet();
	res = SCICallProtocolFromEstiProtocol(protocol);
	
	return res;
}


// This has nothing to do with an IstiCall, but it seemed like a decent place to put it, given its somewhat ephemeral nature.
- (void)sendVideoPrivacyEnabled:(BOOL)enabled
{
	// Tell the remote end that we have enabled video privacy.
	if(IstiVideoInput::InstanceGet())
		IstiVideoInput::InstanceGet()->PrivacySet(enabled ? estiTRUE : estiFALSE);
}

-(void)sendAudioPrivacyEnabled:(BOOL)enabled
{
	if(IstiVideoInput::InstanceGet())
		IstiAudioInput::InstanceGet()->PrivacySet(enabled ? estiTRUE : estiFALSE);
}

- (void)sendDTMFCode:(SCIDTMFCode)code
{
	mCall->DtmfToneSend((EstiDTMFDigit)code);
}

- (BOOL)sendText:(NSString *)text fromSource:(SCISharedTextSource)source
{
	SCILog(@"Start");
	BOOL res = YES;
	
	if (text.length > 0) {
		[self.sentText sci_appendDeletingBackspaces:text];
		
		if (self.canSendDTMF) {
			NSArray *dtmfCodeNumbers = SCIDTMFCodesForNSString(text);
			for (NSNumber *dtmfCodeNumber in dtmfCodeNumbers) {
				SCIDTMFCode dtmfCode = (SCIDTMFCode)dtmfCodeNumber.intValue;
				[self sendDTMFCode:dtmfCode];
			}
		}

		if (self.canSendText) {
			text = [text stringByReplacingOccurrencesOfString:@"\n" withString:@"\u2028"];
			NSArray *components = [text sci_componentsOfLengthAtMost:48];
			for (NSString *component in components) {
				res = [self _sendText:component fromSource:source];
			}
		}
// Text overlay disabled for 8.0
//		else {
//			[self _updateOverlayText];
//		}
		self.textHidden = NO;
	}
	
	return res;
}

- (BOOL)_sendText:(NSString *)text fromSource:(SCISharedTextSource)source
{
	stiHResult result = stiRESULT_SUCCESS;
	NSUInteger textLength = text.length;
	unichar *uniText = (unichar *)calloc(textLength + 1, sizeof(unichar));
	[text getCharacters:uniText range:NSMakeRange(0, textLength)];
	uint16_t *pun16Text = (uint16_t *)uniText;
	result = mCall->TextSend(pun16Text,(EstiSharedTextSource)source);
	free(uniText);
	
	return (result == stiRESULT_SUCCESS);
}

- (void)didReceiveText:(NSString *)text
{
	text = [text stringByReplacingOccurrencesOfString:@"\u2028" withString:@"\n"];
	[self.receivedText sci_appendDeletingBackspaces:text];
	self.textHidden = NO;
}

- (void)removeRemoteText
{
	mCall->TextShareRemoveRemote();
}

+ (void)setWillSendVideoPrivacyEnabled:(BOOL)enabled
{
	if(IstiVideoInput::InstanceGet())
		IstiVideoInput::InstanceGet()->PrivacySet(enabled ? estiTRUE : estiFALSE);
}

+ (void)setWillSendAudioPrivacyEnabled:(BOOL)enabled
{
	if(IstiAudioInput::InstanceGet())
		IstiAudioInput::InstanceGet()->PrivacySet(enabled ? estiTRUE : estiFALSE);
}

- (SCIHearingStatus)hearingStatus
{
	return (SCIHearingStatus)mCall->HearingCallStatusGet();
}

- (BOOL)isIstiCallEqual:(SCICall *)call
{
	return self.istiCall == call.istiCall;
}

- (BOOL)isEqual:(id)object
{
	if ([object isKindOfClass:SCICall.class]) {
		return [self isIstiCallEqual:(SCICall *)object];
	}
	
	return false;
}

- (NSUInteger)hash
{
	return (NSUInteger)self.istiCall;
}

- (BOOL)getGeolocationRequested
{
	return mCall->geolocationRequestedGet ();
}

- (void)setGeolocationRequestedWithAvailability:(GeolocationAvailable)geolocationAvailable
{
	mCall->geolocationRequestedSet (true);
	mCall->geolocationAvailableSet (geolocationAvailable);
}

@end


NSString * const SCICallProtocolNameUnknown = @"Unknown";
NSString * const SCICallProtocolNameSIP = @"sip";
NSString * const SCICallProtocolNameH323 = @"H323";

SCICallProtocol SCICallProtocolFromEstiProtocol(EstiProtocol protocol)
{
	SCICallProtocol res = SCICallProtocolUnknown;
	
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetSCIProtocolForEstiProtocol( sciProtocol, estiProtocol ) \
do { \
	mutableDictionary[@((estiProtocol))] = @((sciProtocol)); \
} while(0)
		
		SetSCIProtocolForEstiProtocol( SCICallProtocolUnknown , estiPROT_UNKNOWN );
		SetSCIProtocolForEstiProtocol( SCICallProtocolSIP , estiSIP );
		
#undef SetSCIProtocolForEstiProtocol
		
		dictionary = [mutableDictionary copy];
	});
	
	NSNumber *resNumber = dictionary[@(protocol)];
	if (resNumber) {
		res = (SCICallProtocol)resNumber.unsignedIntegerValue;
	}
	return res;
}

NSString *NSStringFromEsmiCallState(EsmiCallState state)
{
	NSMutableString *mutableRes = [NSMutableString string];
	BOOL priorFlag = NO;
	
#define AppendState( flag ) \
do { \
	if (state & flag)  { \
		if (priorFlag) { \
			[mutableRes appendString:@" | "]; \
		} \
		priorFlag = YES; \
		[mutableRes appendString:@(#flag)]; \
	} \
} while(0)
	
	AppendState( esmiCS_UNKNOWN );
	AppendState( esmiCS_IDLE );
	AppendState( esmiCS_CONNECTING );
	AppendState( esmiCS_CONNECTED );
	AppendState( esmiCS_HOLD_LCL );
	AppendState( esmiCS_HOLD_RMT );
	AppendState( esmiCS_HOLD_BOTH );
	AppendState( esmiCS_DISCONNECTING );
	AppendState( esmiCS_DISCONNECTED );
	AppendState( esmiCS_CRITICAL_ERROR );
	
#undef AppendState
	
	return [mutableRes copy];
}

NSString *NSStringFromSCIEncryptionState(SCIEncryptionState encryptionState)
{
	NSString *rv;
	switch (encryptionState) {
		case SCIEncryptionStateNone:
			rv = @"SCIEncryptionStateNone";
			break;
		case SCIEncryptionStateAES128:
			rv = @"SCIEncryptionStateAES128";
			break;
		case SCIEncryptionStateAES256:
			rv = @"SCIEncryptionStateAES256";
			break;
		default:
			rv = @"Unknown SCIEncryptionState encountered";
			break;
	}
	return rv;
}

SCICallState SCICallStateFromEsmiCallState(EsmiCallState state)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define AddStateForState( callState , eCallState ) \
do { \
mutableDictionary[@((eCallState))] = @((callState)); \
} while(0)
		
		AddStateForState( SCICallStateUnknown , esmiCS_UNKNOWN );
		AddStateForState( SCICallStateIdle , esmiCS_IDLE );
		AddStateForState( SCICallStateConnecting , esmiCS_CONNECTING );
		AddStateForState( SCICallStateConnected , esmiCS_CONNECTED );
		AddStateForState( SCICallStateHoldLocal , esmiCS_HOLD_LCL );
		AddStateForState( SCICallStateHoldRemote , esmiCS_HOLD_RMT );
		AddStateForState( SCICallStateHoldBoth , esmiCS_HOLD_BOTH );
		AddStateForState( SCICallStateDisconnecting , esmiCS_DISCONNECTING );
		AddStateForState( SCICallStateDisconnected , esmiCS_DISCONNECTED );
		AddStateForState( SCICallStateCriticalError , esmiCS_CRITICAL_ERROR );
		AddStateForState( SCICallStateInitTransfer , esmiCS_INIT_TRANSFER );
		AddStateForState( SCICallStateTransferring , esmiCS_TRANSFERRING );
		
#undef AddStateForState
		
		dictionary = [mutableDictionary copy];
	});
	
	NSNumber *resNumber = dictionary[@(state)];
	return (SCICallState)resNumber.unsignedIntegerValue;
}

NSString *NSStringFromSCICallState(SCICallState state)
{
	NSMutableString *mutableRes = [NSMutableString string];
	BOOL priorFlag = NO;
	
#define AppendState( flag ) \
do { \
	if (state & flag)  { \
		if (priorFlag) { \
			[mutableRes appendString:@" | "]; \
		} \
		priorFlag = YES; \
		[mutableRes appendString:@(#flag)]; \
	} \
} while(0)
	
	AppendState( SCICallStateUnknown );
	AppendState( SCICallStateIdle );
	AppendState( SCICallStateConnecting );
	AppendState( SCICallStateConnected );
	AppendState( SCICallStateHoldLocal );
	AppendState( SCICallStateHoldRemote );
	AppendState( SCICallStateHoldBoth );
	AppendState( SCICallStateDisconnecting );
	AppendState( SCICallStateDisconnected );
	AppendState( SCICallStateCriticalError );
	AppendState( SCICallStateInitTransfer );
	AppendState( SCICallStateTransferring );
	
#undef AppendState
	
	return [mutableRes copy];
}
