//
//  SCICall.h
//  ntouchMac
//
//  Created by Adam Preble on 3/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCISignMailGreetingType.h"
#import "SCICallResult.h"
#import "SCIVideophoneEngine.h"
#import "SCIContact.h"

typedef NS_ENUM(NSUInteger, SCICallState) {
    SCICallStateUnknown			= 0x00000000,	// It isn't known.  Used only for reporting to the app
												// when they request a call state but pass an invalid
												// call handle.
    SCICallStateIdle			= 0x00000001,	// The object has been created but not connected
    SCICallStateConnecting		= 0x00000002,	// The call is being negotiated.
    SCICallStateConnected		= 0x00000004,	// The call is currently connected.
    SCICallStateHoldLocal		= 0x00000008,	// The call is currently on "hold" locally.
    SCICallStateHoldRemote		= 0x00000010,	// The call is currently on "hold" remotely.
    SCICallStateHoldBoth		= 0x00000020,	// The call is currently on "hold" both remotely and locally.
    SCICallStateDisconnecting	= 0x00000040,	// The call is being disconnected.
    SCICallStateDisconnected	= 0x00000080,	// The call has been disconnected.
    SCICallStateCriticalError	= 0x00000100,	// An unrecoverable error has occured.
    SCICallStateInitTransfer	= 0x00000200,	// We are in the process of initiating a transfer.
    SCICallStateTransferring	= 0x00000400,	// We are in the process of completing a transfer.
};

typedef NS_ENUM(NSUInteger, SCICallProtocol) {
	SCICallProtocolUnknown,
	SCICallProtocolH323,
	SCICallProtocolSIP,
};

typedef NS_ENUM(NSInteger, SCIDTMFCode) {
	SCIDTMFCodeNone = NSIntegerMin,
	SCIDTMFCodeZero = 0,
	SCIDTMFCodeOne = 1,
	SCIDTMFCodeTwo = 2,
	SCIDTMFCodeThree = 3,
	SCIDTMFCodeFour = 4,
	SCIDTMFCodeFive = 5,
	SCIDTMFCodeSix = 6,
	SCIDTMFCodeSeven = 7,
	SCIDTMFCodeEight = 8,
	SCIDTMFCodeNine = 9,
	SCIDTMFCodeAsterisk = 10,
	SCIDTMFCodePound = 11,
};

typedef NS_ENUM(NSUInteger, SCISharedTextSource) {
	SCISharedTextSourceUnknown = 0,
	SCISharedTextSourceKeyboard = 1,
	SCISharedTextSourceSavedText = 2,
};

typedef NS_ENUM(NSUInteger, SCIHearingStatus) {
	SCIHearingCallDisconnected = 0,
	SCIHearingCallConnecting = 1,
	SCIHearingCallConnected = 2
};

typedef NS_ENUM(NSUInteger, DhviState) {
	DhviStateNotAvailable, // We checked the number and it is not available for DHVI.
	DhviStateCapable, // We can create a DHVI call from this call.
	DhviStateConnecting, // We are in the process of connecting a DHVI call.
	DhviStateConnected, // We have a DHVI call established using this call.
	DhviStateFailed, // Failed to request a conference room or connect to one.
	DhviStateTimeOut // The DHV call was not answered by all members and timed out.
};

typedef NS_ENUM(NSUInteger, SCIEncryptionState) {
	SCIEncryptionStateNone = 0,   // vpe::EncryptionState::None
	SCIEncryptionStateAES128 = 1, // vpe::EncryptionState::AES_128
	SCIEncryptionStateAES256 = 2  // vpe::EncryptionState::AES_256
};

NSString *NSStringFromSCICallState(SCICallState state);
NSString *NSStringFromSCIEncryptionState(SCIEncryptionState encryptionState);

@class SCICallStatistics;

@interface SCICall : NSObject

@property (nonatomic) BOOL callHistoryItemAdded;

- (BOOL)answer;
- (BOOL)dhviDisconnect;
- (BOOL)continueDial;
- (BOOL)hangUp;
- (BOOL)hold;
- (BOOL)resume;
- (BOOL)reject;
- (BOOL)transfer:(NSString*)dialString error:(NSError **)error;
- (BOOL)nudge;

@property (nonatomic, readonly) SCICallStatistics *statistics;

@property (nonatomic, readonly) SCICallProtocol protocol;
@property (nonatomic, readonly) NSString *protocolName;

@property (nonatomic, readonly) NSString *remoteName;
@property (nonatomic, readonly) NSString *remoteAlternateName;
@property (nonatomic, readonly) NSString *localPreferredLanguage;
@property (nonatomic, readonly) NSString *calledName;
@property (nonatomic, readonly) BOOL isDirectSignMail;
@property (nonatomic, readonly) NSString *callId;
@property (nonatomic, strong) NSUUID *callKitUUID;

@property (nonatomic, readonly) NSString *dialString;
@property (nonatomic, readonly) NSString *newDialString;
@property (nonatomic, readonly) NSString *intendedDialString;
@property (nonatomic, readonly) SCIDialMethod dialMethod;
@property (nonatomic, readonly) NSString *remoteIpAddress;
@property (nonatomic, readonly) NSTimeInterval callDuration;
@property (nonatomic, readonly) NSString *preferredPhoneNumber;
@property (nonatomic, readonly) NSString *sorensonPhoneNumber;
@property (nonatomic, readonly) NSString *tollFreePhoneNumber;
@property (nonatomic, readonly) NSString *localPhoneNumber;
@property (nonatomic, readonly) NSString *hearingPhoneNumber;
@property (nonatomic, readonly) NSString *agentId;
@property (nonatomic, readonly) NSString *messageGreetingURL;
@property (nonatomic, readonly) NSString *messageGreetingURL2;
@property (nonatomic, readonly) NSString *messageGreetingText;
@property (nonatomic, readonly) SCISignMailGreetingType messageGreetingType;
@property (nonatomic, readonly) int messageRecordTime;
@property (nonatomic, readonly) SCICallState state;
@property (nonatomic, readonly) BOOL isTransferable;
@property (nonatomic, readonly) BOOL isPointToPointCall;
@property (nonatomic, readonly) BOOL isVRSCall;
@property (nonatomic, readonly) BOOL isIncoming;
@property (nonatomic, readonly) BOOL isOutgoing;
@property (nonatomic, readonly) BOOL isSignMailInitiated;
@property (nonatomic, readonly) BOOL isHoldServer;
@property (nonatomic, readonly) BOOL isVideoServer;
@property (nonatomic, readonly) BOOL isVideoPhone;
@property (nonatomic, readonly) BOOL isSorensonDevice;
@property (nonatomic, readonly) BOOL isInterpreter;
@property (nonatomic, readonly) BOOL isTechSupport;
@property (nonatomic, readonly) BOOL isConnecting;
@property (nonatomic, readonly) BOOL isConnected;
@property (nonatomic, readonly) BOOL isConferencing;
@property (nonatomic, readonly) BOOL isDisconnecting;
@property (nonatomic, readonly) BOOL isDisconnected;
@property (nonatomic, readonly) BOOL isHoldLocal;
@property (nonatomic, readonly) BOOL isHoldRemote;
@property (nonatomic, readonly) BOOL isHoldBoth;
@property (nonatomic, readonly) BOOL isHoldEither;
@property (nonatomic, readonly) BOOL isHoldable;
@property (nonatomic, readonly) BOOL isVideoPrivacyEnabledRemote;
@property (nonatomic, readonly) BOOL isH323;
@property (nonatomic, readonly) BOOL isSIP;
@property (nonatomic, readonly) BOOL isConnectedToGVC;
@property (nonatomic, readonly) BOOL isConnectedToMCU;
@property (nonatomic, readonly) SCICallResult result;
@property (nonatomic, readonly) UInt32 videoRecordWidth;
@property (nonatomic, readonly) UInt32 videoRecordHeight;
@property (nonatomic, readonly) BOOL canSendText;
@property (nonatomic, readonly) BOOL canSendDTMF;

@property (nonatomic, readonly) NSMutableString *sentText;
@property (nonatomic, readonly) NSMutableString *receivedText;
@property (nonatomic, readwrite) BOOL textHidden;

@property (readonly) SCIHearingStatus hearingStatus;

@property (nonatomic, readwrite) DhviState dhviState;
@property (nonatomic, readonly) SCIEncryptionState encryptionState;

- (void)sendVideoPrivacyEnabled:(BOOL)enabled;
- (void)sendAudioPrivacyEnabled:(BOOL)enabled;

- (void)sendDTMFCode:(SCIDTMFCode)code;

- (BOOL)sendText:(NSString*)text fromSource:(SCISharedTextSource)source;

- (void)removeRemoteText;

- (BOOL)isIstiCallEqual:(SCICall *)call;

+ (void)setWillSendVideoPrivacyEnabled:(BOOL)enabled;
+ (void)setWillSendAudioPrivacyEnabled:(BOOL)enabled;

@property (readonly) BOOL canShareContact;
- (BOOL)shareContactWithName:(NSString*)name companyName:(NSString *)companyName dialString:(NSString*)dialString numberType:(SCIContactPhone)numberType;

@end

extern NSString * const SCICallProtocolNameUnknown;
extern NSString * const SCICallProtocolNameSIP;
extern NSString * const SCICallProtocolNameH323;
