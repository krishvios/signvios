//
//  SCIMessageViewer.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer.h"
#import "SCIMessageViewerCpp.h"
#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngineCpp.h"
#import "IstiVideophoneEngine.h"
#import "IstiMessageViewer.h"
#import "IstiMessageManager.h"
#import "SCIMessageInfo.h"
#import "SCIItemId.h"
#import "SCIItemIdCpp.h"
#import "SCIMessageCategory.h"
#import "stiSVX.h"
#import "CstiMessageRequest.h"
#import "CstiMessageResponse.h"
#import <vector>
#import "SCISignMailGreetingInfo_Cpp.h"

NSNotificationName const SCINotificationMessageViewerStateChanged = @"SCINotificationMessageViewerStateChanged";
NSString * const SCINotificationKeyState = @"state";
NSString * const SCINotificationKeyError = @"error";
NSString * const SCINotificationKeyOldState = @"oldState";

NSString *NSStringFromSCIMessageViewerState(SCIMessageViewerState SCIMessageViewerState);

@interface SCIMessageViewer ()
{
	SCIMessageViewerState mState;
	
	SCIMessageInfo *mOpenedMessage;
	SCIMessageInfo *mHeldMessage;
	
	
	NSTimeInterval mClosingOpenedMessagePausePointToMark;
	BOOL mShouldMarkClosingOpenedMessagePausePoint;
	
	BOOL mCloseAfterFinishedOpening;
}

@property (nonatomic, assign, readwrite) SCIMessageViewerState state;
@property (nonatomic, assign, readwrite) SCIMessageViewerError error;
@property (nonatomic, strong, readwrite) SCIMessageInfo *heldMessage;

- (SCIVideophoneEngine *)videophoneEngine;
- (IstiMessageViewer *)istiMessageViewer;
- (IstiVideophoneEngine *)istiVideophoneEngine;

- (void)_openMessage:(SCIMessageInfo *)message startPosition:(NSTimeInterval)startPosition;
- (void)_openMessage:(SCIMessageInfo *)message;
- (stiHResult)_close;

- (void)fetchState;
@end

@implementation SCIMessageViewer

#pragma mark private property synthesizes
@synthesize state = mState;
@synthesize error = mError;
@synthesize heldMessage = mHeldMessage;


static SCIMessageViewer *sharedViewer = nil;
+ (SCIMessageViewer *)sharedViewer
{
	if (!sharedViewer)
	{
		sharedViewer = [[SCIMessageViewer alloc] init];
	}
	return sharedViewer;
}

- (id)init
{
	self = [super init];
	if (self) {
		mState = (SCIMessageViewerState)self.estiState;
		mError = (SCIMessageViewerError)self.estiError;
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - public API

- (void)openMessage:(SCIMessageInfo *)message
{
	if (self.openedMessage) {//we can't actually start playback yet.
		self.heldMessage = message;
		[self close];
	} else {
		[self _openMessage:message];
	}
}

- (BOOL)close
{
	SCIMessageViewerState state = [self estiState];	
	stiHResult res = stiRESULT_SUCCESS;
	if (state & SCIMessageViewerStateOpening) {//we can't actually close yet
		mCloseAfterFinishedOpening = YES;
	} else {
		res = [self _close];
	}
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)play
{
	stiHResult res = self.istiMessageViewer->Play();
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)playFromBeginning
{
	stiHResult res = self.istiMessageViewer->Restart();
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)pause
{
	stiHResult res = self.istiMessageViewer->Pause();
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)getDuration:(NSTimeInterval *)duration currentTime:(NSTimeInterval *)currentTime
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	IstiMessageViewer::SstiProgress progress;
	stiHResult res = istiMessageViewer->ProgressGet(&progress);
	if (res != stiRESULT_SUCCESS)
		return NO;
	
	if (progress.un64MediaTimeScale == 0)
	{
		// Avoid divide by zero.
		return NO;
	}
	
	if (duration)
		*duration = (NSTimeInterval)progress.un64MediaDuration/(NSTimeInterval)progress.un64MediaTimeScale;
	if (currentTime)
		*currentTime = (NSTimeInterval)progress.un64CurrentMediaTime/(NSTimeInterval)progress.un64MediaTimeScale;
	
	return YES;
}

- (BOOL)seekTo:(NSTimeInterval)time
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	IstiMessageViewer::SstiProgress pProgress;
	istiMessageViewer->ProgressGet(&pProgress);
	
	stiHResult res = stiRESULT_SUCCESS;
	if (pProgress.un64MediaTimeScale != 0) {//FIXME: this avoids a divide by zero crash within CFilePlay::SkipTo(1)
		res = istiMessageViewer->SkipTo(time);
	} else {
		istiMessageViewer->SetStartPosition(time);
	}
	
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)skipForward:(NSTimeInterval)time
{
	stiHResult res = self.istiMessageViewer->SkipForward((unsigned int)time);
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)skipBackward:(NSTimeInterval)time
{
	stiHResult res = self.istiMessageViewer->SkipBack((unsigned int)time);
	return (res == stiRESULT_SUCCESS);
}

- (SCIMessageInfo *)openedMessage
{
	return mOpenedMessage;
}

- (BOOL)stopRecording
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	stiHResult res = istiMessageViewer->RecordStop();
	return res == stiRESULT_SUCCESS;
}
- (BOOL)signMailRerecord {
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	stiHResult res = istiMessageViewer->SignMailRerecord();
	return res == stiRESULT_SUCCESS;
}
- (BOOL)sendRecordedMessage
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	stiHResult res = istiMessageViewer->SendRecordedMessage();
	return res == stiRESULT_SUCCESS;
}
- (BOOL)deleteRecordedMessage
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	stiHResult res = istiMessageViewer->DeleteRecordedMessage();
	return res == stiRESULT_SUCCESS;
}
- (BOOL)clearRecordP2PMessageInfo
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	stiHResult res = istiMessageViewer->RecordP2PMessageInfoClear();
	return res == stiRESULT_SUCCESS;
}
- (void)skipSignMailGreeting
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	if (istiMessageViewer)
		istiMessageViewer->SkipSignMailGreeting();
}

- (BOOL)recordGreeting
{
	stiHResult res = self.istiMessageViewer->GreetingRecord();
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)deleteRecordedGreeting
{
	stiHResult res = self.istiMessageViewer->GreetingDeleteRecorded();
	return (res == stiRESULT_SUCCESS);
}

- (BOOL)uploadGreeting
{
	stiHResult res = self.istiVideophoneEngine->GreetingUpload();
	return (res == stiRESULT_SUCCESS);
}

- (void)fetchGreetingInfoWithCompletion:(void (^)(SCISignMailGreetingInfo *info, NSError *error))block
{
	CstiMessageRequest *request = new CstiMessageRequest();
	request->GreetingInfoGet();
	request->RemoveUponCommunicationError () = estiTRUE;

	[self.videophoneEngine sendMessageRequest:request completion:^(CstiMessageResponse *response, NSError *sendError) {
		SCISignMailGreetingInfo *infoOut = nil;
		NSError *errorOut = nil;
		
		if (response) {
			std::vector <CstiMessageResponse::SGreetingInfo> *pGreetingInfoList = response->GreetingInfoListGet();
			if (pGreetingInfoList) {
				std::vector <CstiMessageResponse::SGreetingInfo> greetingInfoList = *pGreetingInfoList;
				SCISignMailGreetingInfo *info = [SCISignMailGreetingInfo greetingInfoFromSGreetingInfoVector:greetingInfoList];
				
				infoOut = info;
				errorOut = nil;
			} else {
				infoOut = nil;
				//TODO: generate an error and set errorOut
			}
		} else {
			infoOut = nil;
			errorOut = sendError;
		}
		
		if (block)
			block(infoOut, errorOut);
	}];
}

- (BOOL)loadVideoFromServer:(NSString *)server
			   withFilename:(NSString *)filename
{
	BOOL res = NO;
	
	const char *pszServer = server ? server.UTF8String : NULL;
	const char *pszFilename = filename ? filename.UTF8String : NULL;
	
	//IstiMessageViewer::Open() doesn't return an hResult.  We must
	//assume that it is always successful.
	self.istiMessageViewer->Open(pszServer, pszFilename);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	res = (hResult == stiRESULT_SUCCESS);
	
	return res;
}

- (void)fetchState
{
	IstiMessageViewer::EState estiState = self.estiState;
	IstiMessageViewer::EError estiError = self.estiError;
	SCIMessageViewerState storedState = mState;
	SCIMessageViewerState presentState = (SCIMessageViewerState)estiState;
	if (storedState != presentState) {
		[self updateStateWithEstiState:estiState];
		self.error = estiError;
	}
}

#pragma mark - Cpp API

- (void)updateStateWithEstiState:(IstiMessageViewer::EState)state
{
	SCIMessageViewerState newState = (SCIMessageViewerState)state;
	SCIMessageViewerState oldState = self.state;
	self.state = newState;
	
	SCIMessageViewerState stateChanges = (oldState ^ newState);
	SCIMessageViewerState stateAdditions = (stateChanges & newState);
	SCIMessageViewerState stateSubtractions = (stateChanges & oldState);
	
	if (stateAdditions & SCIMessageViewerStateClosed) {
		//mark the now closed message's pause point
		if (mOpenedMessage && mShouldMarkClosingOpenedMessagePausePoint) {
			if (mClosingOpenedMessagePausePointToMark > 0) {
				[[SCIVideophoneEngine sharedEngine] markMessage:mOpenedMessage pausePoint:mClosingOpenedMessagePausePointToMark];
			}
			mShouldMarkClosingOpenedMessagePausePoint = NO;
		}
		mClosingOpenedMessagePausePointToMark = 0;
		mOpenedMessage = nil;
		
		//then check whether another message is held
		SCIMessageInfo *heldMessage = self.heldMessage;
		if (heldMessage) {
			[self openMessage:heldMessage];
			self.heldMessage = nil;
		}
	}
	if (stateSubtractions & SCIMessageViewerStateOpening) {
		//the message was asked to be closed while it was still opening, close it now
		if (mCloseAfterFinishedOpening) {
			[self _close];
			mCloseAfterFinishedOpening = NO;
		}
		
		//IFrames are not being sent with sign mail from iOS and Mac endpoints, so calling IstiMessageViewer::SetStartPosition(1)
		//	has no effect.  As a work-around, once the message has finished opening, skip to the right point using -seekTo: when
		//	the message category's type is estiMT_SIGN_MAIL.
		NSTimeInterval pausePoint = self.openedMessage.pausePoint;
		if ([[[SCIVideophoneEngine sharedEngine] messageCategoryForMessage:self.openedMessage] type] == SCIMessageTypeSignMail &&
			pausePoint > 0) {
			[self seekTo:pausePoint];
		}
	}
	
	if (stateAdditions & SCIMessageViewerStateVideoEnd) {//we have reached the end of the message.  set its pause-point to 0.
		SCIMessageInfo *message = [self openedMessage];
		message.pausePoint = 0.0;
	}
	
	if (newState & SCIMessageViewerStateError || oldState & SCIMessageViewerStateError) {
		//Update self.error if we have an error now or we used to have an error.  We can't & with
		//stateChanges because then we won't update the error in the case that we had an error before
		//and we have an error now, even though the error may have changed.
		self.error = (SCIMessageViewerError)self.estiError;
	}
	else {
		self.error = SCIMessageViewerErrorNone;
	}
	
	if ((newState & SCIMessageViewerStateRecordClosing) && (self.estiError & SCIMessageViewerErrorRecording)) {
		// Check for this one condition, CFilePlay will skip Callback notifying UI that stateChanged to Error
		// So this handler will only see RECORD_CLOSING and ERROR_RECORDING
		self.error = (SCIMessageViewerError)self.estiError;
	}
	
	NSDictionary *userInfo = @{SCINotificationKeyState : [NSNumber numberWithUnsignedInteger:self.state],
							SCINotificationKeyOldState : [NSNumber numberWithUnsignedInteger:oldState],
							SCINotificationKeyError :[NSNumber numberWithUnsignedInteger:self.error]};
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationMessageViewerStateChanged
														object:self 
													  userInfo:userInfo];
}

- (IstiMessageViewer::EState)estiState
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	
	IstiMessageViewer::EState res = (IstiMessageViewer::EState)0;
	if (istiMessageViewer)
		res = self.istiMessageViewer->StateGet();
	return res;
}

- (IstiMessageViewer::EError)estiError
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;
	
	IstiMessageViewer::EError res = (IstiMessageViewer::EError)0;
	if (istiMessageViewer)
		res = self.istiMessageViewer->ErrorGet();
	return res;
}

#pragma mark - private methods

- (SCIVideophoneEngine *)videophoneEngine
{
	return [SCIVideophoneEngine sharedEngine];
}

- (IstiVideophoneEngine *)istiVideophoneEngine
{
	return self.videophoneEngine.videophoneEngine;
}

- (IstiMessageViewer *)istiMessageViewer
{
	IstiMessageViewer *res = NULL;
	
	IstiVideophoneEngine *istiVideophoneEngine = self.istiVideophoneEngine;
	if (istiVideophoneEngine) {
		res = IstiMessageViewer::InstanceGet();
	} else {
		res = NULL;
	}
	
	return res;
}

- (void)_openMessage:(SCIMessageInfo *)message startPosition:(NSTimeInterval)startPosition
{
	IstiMessageViewer *istiMessageViewer = self.istiMessageViewer;

	istiMessageViewer->SetStartPosition((uint32_t)startPosition);
	
	CstiItemId cstiItemId = message.messageId.cstiItemId;
	istiMessageViewer->Open(cstiItemId, [[message name] UTF8String], [[message dialString] UTF8String]);
	
	mOpenedMessage = message;
}

- (void)_openMessage:(SCIMessageInfo *)message
{
	[self _openMessage:message startPosition:message.pausePoint];
}

- (stiHResult)_close
{
	NSTimeInterval duration, currentTime;
	mShouldMarkClosingOpenedMessagePausePoint = [self getDuration:&duration
													  currentTime:&currentTime];
	if (mShouldMarkClosingOpenedMessagePausePoint) {
		if ((duration - currentTime) < 1) {
			mClosingOpenedMessagePausePointToMark = 0;
		} else {
			mClosingOpenedMessagePausePointToMark = currentTime;
		}
	}
	
	stiHResult res = self.istiMessageViewer->Close();
	
	return res;
}

@end

@implementation SCIMessageViewer (Deprecated)

- (BOOL)messageViewerRecordStop
{
	return [self stopRecording];
}

- (BOOL)messageViewerSendRecordedMessage
{
	return [self sendRecordedMessage];
}

- (BOOL)messageViewerDeleteRecordedMessage
{
	return [self deleteRecordedMessage];
}

- (BOOL)messageViewerRecordP2PMessageInfoClear
{
	return [self clearRecordP2PMessageInfo];
}

@end

const SCIMessageViewerState SCIMessageViewerStateUninitialized = 0;
const SCIMessageViewerState SCIMessageViewerStateOpening = IstiMessageViewer::estiOPENING; //1
const SCIMessageViewerState SCIMessageViewerStateReloading = IstiMessageViewer::estiRELOADING; //2
const SCIMessageViewerState SCIMessageViewerStatePlayWhenReady = IstiMessageViewer::estiPLAY_WHEN_READY; //4
const SCIMessageViewerState SCIMessageViewerStatePlaying = IstiMessageViewer::estiPLAYING; //8
const SCIMessageViewerState SCIMessageViewerStatePaused = IstiMessageViewer::estiPAUSED; //16
const SCIMessageViewerState SCIMessageViewerStateClosing = IstiMessageViewer::estiCLOSING; //32
const SCIMessageViewerState SCIMessageViewerStateClosed = IstiMessageViewer::estiCLOSED; //64
const SCIMessageViewerState SCIMessageViewerStateError = IstiMessageViewer::estiERROR; //128
const SCIMessageViewerState SCIMessageViewerStateVideoEnd = IstiMessageViewer::estiVIDEO_END; //256
const SCIMessageViewerState SCIMessageViewerStateRequestingGuid = IstiMessageViewer::estiREQUESTING_GUID; //512
const SCIMessageViewerState SCIMessageViewerStateRecordConfigure = IstiMessageViewer::estiRECORD_CONFIGURE; //1024
const SCIMessageViewerState SCIMessageViewerStateWaitingToRecord = IstiMessageViewer::estiWAITING_TO_RECORD; //2048
const SCIMessageViewerState SCIMessageViewerStateRecording = IstiMessageViewer::estiRECORDING; //4096
const SCIMessageViewerState SCIMessageViewerStateRecordFinished = IstiMessageViewer::estiRECORD_FINISHED; //8192
const SCIMessageViewerState SCIMessageViewerStateRecordClosing = IstiMessageViewer::estiRECORD_CLOSING; //16384
const SCIMessageViewerState SCIMessageViewerStateRecordClosed = IstiMessageViewer::estiRECORD_CLOSED; //32768
const SCIMessageViewerState SCIMessageViewerStateUploading = IstiMessageViewer::estiUPLOADING; //65336
const SCIMessageViewerState SCIMessageViewerStateUploadComplete = IstiMessageViewer::estiUPLOAD_COMPLETE; //130672
const SCIMessageViewerState SCIMessageViewerStatePlayerIdle = IstiMessageViewer::estiPLAYER_IDLE; //261344

const SCIMessageViewerError SCIMessageViewerErrorNone = IstiMessageViewer::estiERROR_NONE;
const SCIMessageViewerError SCIMessageViewerErrorGeneric = IstiMessageViewer::estiERROR_GENERIC;
const SCIMessageViewerError SCIMessageViewerErrorServerBusy = IstiMessageViewer::estiERROR_SERVER_BUSY;
const SCIMessageViewerError SCIMessageViewerErrorOpening = IstiMessageViewer::estiERROR_OPENING;
const SCIMessageViewerError SCIMessageViewerErrorRequestingGuid = IstiMessageViewer::estiERROR_REQUESTING_GUID;
const SCIMessageViewerError SCIMessageViewerErrorParsingGuid = IstiMessageViewer::estiERROR_PARSING_GUID;
const SCIMessageViewerError SCIMessageViewerErrorParsingUploadGuid = IstiMessageViewer::estiERROR_PARSING_UPLOAD_GUID;
const SCIMessageViewerError SCIMessageViewerErrorRecordConfigIncomplete = IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE;
const SCIMessageViewerError SCIMessageViewerErrorRecording = IstiMessageViewer::estiERROR_RECORDING;
const SCIMessageViewerError SCIMessageViewerErrorNoDataUploaded = IstiMessageViewer::estiERROR_NO_DATA_UPLOADED;

NSString *NSStringFromSCIMessageViewerState(SCIMessageViewerState state)
{
	return NSStringFromFilePlayState((uint32_t)state);
}

NSString *NSStringFromSCIMessageViewerError(SCIMessageViewerError error)
{
	return NSStringFromFilePlayError((uint32_t)error);
}









