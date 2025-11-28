//
//  SCIMessageViewer+AsynchronousCompletion.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/4/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer+AsynchronousCompletion.h"
#import <objc/runtime.h>
#import "BNRUUID.h"
#import "SCIMessageViewerCpp.h"
#import "CstiMessageResponse.h"
#import "SCIVideophoneEngineCpp.h"
#import "stiHTTPServiceTools.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCISignMailGreetingInfo.h"
#import "SCISignMailGreetingInfo_Cpp.h"
#import "NSObject+BNRAssociatedDictionary.h"

@class SCIMessageViewerObserver;

@interface SCIMessageViewerObserver : NSObject
@property (nonatomic, unsafe_unretained, readonly) SCIMessageViewer *messageViewer;
- (void)startObservingMessageViewerNotifications:(SCIMessageViewer *)messageViewer;
- (void)stopObservingMessageViewerNotifications;
- (void)ifStateSatisfies:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
	  whenStateSatisfies:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
	   performCompletion:(void (^)(BOOL success))completionBlock
 passingSuccessParameter:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
	   forOperationNamed:(NSString *)operationName
			givenByBlock:(BOOL (^)(void))operation
blocksSubsequentOperations:(BOOL)blockSubsequentOperations;
- (BOOL)terminateFirstOperationNamed:(NSString *)operationName complete:(BOOL (^)(BOOL operationStarted))callCompletion success:(BOOL (^)(BOOL operationStarted))passSuccess;
@property (nonatomic) BOOL allOperationsBlocked;
@end

@interface SCIMessageViewer (AsynchronousCompletionPrivate)
@property (nonatomic) SCIMessageViewerObserver *observer;
@property (nonatomic) NSTimer *uploadRecordedGreetingTimeoutTimer;
@end

static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameLoadVideo = @"LoadVideo";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameDeleteRecordedGreeting = @"DeleteRecordedGreeting";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameUploadRecordedGreeting = @"UploadRecordedGreeting";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStopRecording = @"StopRecording";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStopRecordingMessageAndUpload = @"StopRecordingMessageAndUpload";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStartRecordingGreetingWithTextOverlay = @"StartRecordingGreetingWithTextOverlay";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStartRecordingGreeting = @"StartRecordingGreeting";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameClose = @"Close";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNamePausePlaying = @"PausePlaying";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStartPlayingFromBeginning = @"StartPlayingFromBeginning";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameStartPlaying = @"StartPlaying";
static NSString * const SCIMessageViewerAsynchronousCompletionOperationNameWaitingForState = @"WaitingForState";

@implementation SCIMessageViewer (AsynchronousCompletion)

#pragma mark - Public API: Notifications

- (void)startObservingNotifications
{
	[self.observer startObservingMessageViewerNotifications:self];
}

- (void)stopObservingNotifications
{
	[self.observer stopObservingMessageViewerNotifications];
	self.observer = nil;
}

#pragma mark - Public API: Operations

- (void)startPlayingWithCompletion:(void (^)(BOOL successs))block
{
	[self ifInState:SCIMessageViewerStateClosed | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePaused | SCIMessageViewerStateVideoEnd
 allowUninitialized:YES
   whenStateReached:SCIMessageViewerStatePlaying
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameStartPlaying
	blockSubsequent:NO
	   givenByBlock:^BOOL{
		   return [self play];
	   }];
}

- (void)startPlayingFromBeginningWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStateClosed | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePaused | SCIMessageViewerStateVideoEnd | SCIMessageViewerStatePlaying
 allowUninitialized:YES
   whenStateReached:SCIMessageViewerStatePlaying
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameStartPlayingFromBeginning
	blockSubsequent:NO
	   givenByBlock:^BOOL{
		   return [self playFromBeginning];
	   }];
}

- (void)waitToStartPlayingWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStateOpening | SCIMessageViewerStatePlayWhenReady
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStatePlaying
orFailureStateReached:SCIMessageViewerStateError
	blockOperations:NO
	   performBlock:block];
}

- (void)waitToStartPlayingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStateOpening | SCIMessageViewerStatePlayWhenReady
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStatePlaying
orFailureStateReached:SCIMessageViewerStateError
	blockOperations:NO
	   performBlock:block];
}

- (void)pausePlayingWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStatePlaying | SCIMessageViewerStatePlayWhenReady
 allowUninitialized:NO
   whenStateReached:SCIMessageViewerStatePaused
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNamePausePlaying
	blockSubsequent:YES
	   givenByBlock:^BOOL{
		   return [self pause];
	   }];
}

- (void)waitToStartPausingWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStateOpening | SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePlaying
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStatePaused
orFailureStateReached:SCIMessageViewerStateError
	blockOperations:NO
	   performBlock:block];
}

- (void)waitToStartPausingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL success))block
{
	[self ifInState:SCIMessageViewerStateOpening | SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePlaying
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStatePaused
orFailureStateReached:SCIMessageViewerStateError
	blockOperations:YES
	   performBlock:block];
}

- (void)closeWithCompletion:(void (^)(BOOL success))block
{
	[self ifNotInState:SCIMessageViewerStateUninitialized | SCIMessageViewerStateClosing | SCIMessageViewerStateClosed | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStateRecordClosed
  excludeUninitialized:YES
   whenStateReached:SCIMessageViewerStateClosed | SCIMessageViewerStatePaused | SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePlaying
  performCompletion:block
	 forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameClose
	   blockSubsequent:YES
		  givenByBlock:^BOOL{
			  return [self close];
		  }];
}

- (void)startRecordingGreetingWithCompletion:(void (^)(BOOL))block
{
	[self ifInState:SCIMessageViewerStatePlayerIdle | SCIMessageViewerStateClosed | SCIMessageViewerStateRecordClosed
 allowUninitialized:YES
   whenStateReached:SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStateRecording
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameStartRecordingGreeting
	blockSubsequent:NO
	   givenByBlock:^BOOL{
		   return [self recordGreeting];
	   }];
}

- (void)waitToStartRecordingGreetingWithCompletion:(void (^)(BOOL))block
{
	[self ifInState:SCIMessageViewerStateClosing | SCIMessageViewerStateClosed | SCIMessageViewerStateWaitingToRecord
 allowUninitialized:NO
   whenSuccessStateReached:SCIMessageViewerStateRecording
	 orFailureStateReached:SCIMessageViewerStateError
	 blockOperations:NO
	   performBlock:block];
}

- (void)waitToStartRecordingGreetingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL))block
{
	[self ifInState:SCIMessageViewerStateClosing | SCIMessageViewerStateClosed | SCIMessageViewerStateWaitingToRecord
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStateRecording
orFailureStateReached:SCIMessageViewerStateError
	blockOperations:YES
	   performBlock:block];
}

- (void)stopRecordingWithCompletion:(void (^)(BOOL))block
{
	[self ifInState:SCIMessageViewerStateRecording | SCIMessageViewerStateWaitingToRecord | SCIMessageViewerStateRecordClosing | SCIMessageViewerStateClosed
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStateRecordFinished
orFailureStateReached:SCIMessageViewerStateError
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameStopRecording
	blockSubsequent:YES
	   givenByBlock:^BOOL{
		   return [self stopRecording];
	   }];
}

- (void)stopRecordingMessageAndUploadWithCompletion:(void (^)(BOOL))block
{
	[self ifInState:SCIMessageViewerStateRecording | SCIMessageViewerStateWaitingToRecord | SCIMessageViewerStateRecordClosing
 allowUninitialized:NO
   whenSuccessStateReached:SCIMessageViewerStateUploadComplete
orFailureStateReached:SCIMessageViewerStateError
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameStopRecordingMessageAndUpload
	blockSubsequent:NO
	   givenByBlock:^BOOL{
		   return [self stopRecording];
	   }];
}

- (void)uploadRecordedGreetingWithCompletion:(void (^)(BOOL))block
{
	//TODO: Identify all allowed states.
	[self ifInState:SCIMessageViewerStatePlayWhenReady |  SCIMessageViewerStateRecordClosed | SCIMessageViewerStateRecordClosing | SCIMessageViewerStateVideoEnd | SCIMessageViewerStatePaused
 allowUninitialized:NO
whenSuccessStateReached:SCIMessageViewerStateUploadComplete
orFailureStateReached:SCIMessageViewerStateError
  performCompletion:^(BOOL success) {
	  [self invalidateUploadRecordedGreetingTimeoutTimer];
	  if (block)
		  block(success);
  }
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameUploadRecordedGreeting
	blockSubsequent:YES
	   givenByBlock:^BOOL{
		   BOOL res = [self uploadGreeting];
		   if (res) {
			   self.uploadRecordedGreetingTimeoutTimer = [NSTimer scheduledTimerWithTimeInterval:60.0f target:self selector:@selector(uploadRecordedGreetingTimeout:) userInfo:nil repeats:NO];
		   }
		   return res;
	   }];
}

- (void)cancelUploadingRecordedGreeting
{
	[self invalidateUploadRecordedGreetingTimeoutTimer];
	[self terminateUploadingRecordedGreeting];
}

- (void)uploadRecordedGreetingTimeout:(NSTimer *)timer
{
	[self cancelUploadingRecordedGreeting];
}

- (void)invalidateUploadRecordedGreetingTimeoutTimer
{
	[self.uploadRecordedGreetingTimeoutTimer invalidate];
	self.uploadRecordedGreetingTimeoutTimer = nil;
}

- (void)terminateUploadingRecordedGreeting
{
	[self.observer terminateFirstOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameUploadRecordedGreeting
									   complete:^BOOL(BOOL operationStarted) {
										   return YES;
									   }
										success:^BOOL(BOOL operationStarted) {
											return NO;
										}];
}

- (void)stopPlayingCountdownForRecordingGreetingWithCompletion:(void (^)(BOOL))block
{
	[self ifNotInState:SCIMessageViewerStateClosed | SCIMessageViewerStateRecordFinished | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStateUploadComplete | SCIMessageViewerStateRecordClosing | SCIMessageViewerStateRecordClosed | SCIMessageViewerStateOpening
  excludeUninitialized:NO
	  whenStateReached:SCIMessageViewerStateClosed
	 performCompletion:block
	 forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameDeleteRecordedGreeting
	   blockSubsequent:NO
		  givenByBlock:^BOOL{
			  return [self deleteRecordedGreeting];
		  }];
}

- (void)deleteRecordedGreetingWithCompletion:(void (^)(BOOL))block
{
	//TODO: Use the right states here.
	[self ifNotInState:SCIMessageViewerStateClosed | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStateUploadComplete | SCIMessageViewerStateRecordClosing | SCIMessageViewerStateRecordClosed | SCIMessageViewerStatePlaying
  excludeUninitialized:YES
	  whenStateReached:SCIMessageViewerStateClosed | SCIMessageViewerStateRecordClosed
	 performCompletion:block
	 forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameDeleteRecordedGreeting
	   blockSubsequent:NO
		  givenByBlock:^BOOL{
			  return [self deleteRecordedGreeting];
		  }];
}

- (void)loadVideoFromServer:(NSString *)server
			   withFilename:(NSString *)filename
				 completion:(void (^)(BOOL success))block
{
	//TODO: Are these the correct states to allow and to be awaiting?
	[self ifInState:SCIMessageViewerStateClosed | SCIMessageViewerStateVideoEnd | SCIMessageViewerStatePlayerIdle | SCIMessageViewerStateError | SCIMessageViewerStatePlaying | SCIMessageViewerStatePaused | SCIMessageViewerStateRecordClosed
 allowUninitialized:YES
whenSuccessStateReached:SCIMessageViewerStatePlayWhenReady | SCIMessageViewerStatePaused
orFailureStateReached:SCIMessageViewerStateError
  performCompletion:block
  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameLoadVideo
	blockSubsequent:YES
	   givenByBlock:^BOOL{
		   return [self loadVideoFromServer:server
							   withFilename:filename];
	   }];
}

- (void)loadGreetingWithCompletion:(void (^)(BOOL success))block
{
	[self setAllOperationsBlocked:YES];
	[self fetchGreetingInfoWithCompletion:^(SCISignMailGreetingInfo *info, NSError *error) {
		if (!error) {
			const char *pszGreetingViewURL1 = NULL;
			const char *pszGreetingViewURL2 = NULL;
			//TODO: Once the proper type of a greeting is in the VPE, this needs to be changed
			//      back to fetching the video for the current greeting preference.
			SCISignMailGreetingType typePreference = [[SCIVideophoneEngine sharedEngine] signMailGreetingPreference];
			SCISignMailGreetingType typeToFetch = (SCISignMailGreetingTypeIsPersonal(typePreference)) ? SCISignMailGreetingTypePersonalVideoOnly : SCISignMailGreetingTypeDefault;
			NSArray *greetingViewURLs = [info URLsOfType:typeToFetch];
			NSUInteger urlCount = greetingViewURLs.count;
			if (urlCount > 0) {
				NSString *greetingViewURL1 = greetingViewURLs[0];
				pszGreetingViewURL1 = greetingViewURL1.UTF8String;
			}
			if (urlCount > 1) {
				NSString *greetingViewURL2 = greetingViewURLs[1];
				pszGreetingViewURL2 = greetingViewURL2.UTF8String;
			}
			
			char serverIP[un8stiMAX_URL_LENGTH + 1];
			char serverPort[20];
			char fileUrl[un16stiMAX_URL_LENGTH + 1];
			char serverName[un8stiMAX_URL_LENGTH + 1];
			
			if (eURL_OK == stiURLParse(pszGreetingViewURL1,
									   serverIP, sizeof(serverIP),
									   serverPort, sizeof(serverPort),
									   fileUrl, sizeof(fileUrl),
									   serverName, sizeof(serverName),
									   pszGreetingViewURL2))
			{
				
				NSString *server = [NSString stringWithUTF8String:serverName];
				NSString *filename = [NSString stringWithUTF8String:fileUrl];
				
				[self loadVideoFromServer:server
							 withFilename:filename
							   completion:block];
			} else {
				[self performCallbackBlock:^{
					if (block)
						block(NO);
				}];
			}
		} else {
			[self performCallbackBlock:^{
				if (block)
					block(NO);
			}];
		}
		[self setAllOperationsBlocked:NO];
	}];
}

- (void)ifInState:(SCIMessageViewerState)expectedState
allowUninitialized:(BOOL)allowUninitialized
 whenSuccessStateReached:(SCIMessageViewerState)successState
   orFailureStateReached:(SCIMessageViewerState)failureState
  blockOperations:(BOOL)blockOperations
	 performBlock:(void (^)(BOOL success))block
{
	if ([self stateMatchesState:successState]) {
		[self performCallbackBlock:^{
			if (block)
				block(YES);
		}];
	} else if ([self stateMatchesState:failureState]) {
		[self performCallbackBlock:^{
			if (block)
				block(NO);
		}];
	} else {
		[self ifInState:expectedState
	 allowUninitialized:allowUninitialized
whenSuccessStateReached:successState
  orFailureStateReached:failureState
	  performCompletion:block
	  forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameWaitingForState
		blockSubsequent:blockOperations
		   givenByBlock:^BOOL{return YES;}];
	}
}

- (void)ifNotInState:(SCIMessageViewerState)unexpectedState
excludeUninitialized:(BOOL)excludeUninitialized
whenSuccessStateReached:(SCIMessageViewerState)successState
orFailureStateReached:(SCIMessageViewerState)failureState
  blockOperations:(BOOL)blockOperations
		performBlock:(void (^)(BOOL success))block
{
	if ([self stateMatchesState:successState]) {
		[self performCallbackBlock:^{
			if (block)
				block(YES);
		}];
	} else if ([self stateMatchesState:failureState]) {
		[self performCallbackBlock:^{
			if (block)
				block(NO);
		}];
	} else {
		[self ifNotInState:unexpectedState
	  excludeUninitialized:excludeUninitialized
   whenSuccessStateReached:successState
	 orFailureStateReached:failureState
		 performCompletion:block
		 forOperationNamed:SCIMessageViewerAsynchronousCompletionOperationNameWaitingForState
		   blockSubsequent:blockOperations
			  givenByBlock:^BOOL{return YES;}];
	}
}

#pragma mark - Helpers

- (BOOL)stateMatchesState:(SCIMessageViewerState)state
{
	return ((self.state & state) > 0);
}

#pragma mark - Helpers: Performing Callback Blocks

- (void)performCallbackBlock:(void (^)(void))block
{
	if (block) {
		dispatch_async(dispatch_get_main_queue(), ^{
				block();
		});
	}
}

- (void)performCallbackBlock:(void (^)(BOOL))block boolArgument:(BOOL)argument
{
	[self performCallbackBlock:^{
		block(argument);
	}];
}

#pragma mark - Helpers: Registering Blocks

- (void)ifNotInState:(SCIMessageViewerState)unexpectedCurrentState
excludeUninitialized:(BOOL)excludeUninitialized
	whenStateReached:(SCIMessageViewerState)targetState
   performCompletion:(void (^)(BOOL success))block
   forOperationNamed:(NSString *)operationName
  blockSubsequent:(BOOL)blockSubsequentOperations
		givenByBlock:(BOOL (^)(void))operation
{
	[self ifStateSatisfies:^BOOL(SCIMessageViewerState currentState, SCIMessageViewerError currentError) {
		BOOL res = ![self stateMatchesState:unexpectedCurrentState];
		if (excludeUninitialized) {
			res = res && (self.state != SCIMessageViewerStateUninitialized);
		}
		return res;
	}
		whenStateSatisfies:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
			return ((newState & targetState) > 0);
		}
		 performCompletion:block
   passingSuccessParameter:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
	   return ((newState & targetState) > 0);
   }
		 forOperationNamed:operationName
		   blockSubsequent:blockSubsequentOperations
			  givenByBlock:operation];
}

- (void)ifNotInState:(SCIMessageViewerState)unexpectedCurrentState
excludeUninitialized:(BOOL)excludeUninitialized
whenSuccessStateReached:(SCIMessageViewerState)targetSuccessState
orFailureStateReached:(SCIMessageViewerState)targetFailureState
   performCompletion:(void (^)(BOOL success))block
   forOperationNamed:(NSString *)operationName
  blockSubsequent:(BOOL)blockSubsequentOperations
		givenByBlock:(BOOL (^)(void))operation
{
	[self ifStateSatisfies:^BOOL(SCIMessageViewerState currentState, SCIMessageViewerError currentError) {
		BOOL res = ![self stateMatchesState:unexpectedCurrentState];
		if (excludeUninitialized) {
			res = res && (self.state != SCIMessageViewerStateUninitialized);
		}
		return res;
	}
		whenStateSatisfies:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
			return ((newState & (targetSuccessState | targetFailureState)) > 0);
		}
		 performCompletion:block
   passingSuccessParameter:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
	   BOOL res = NO;
	   if ((newState & targetSuccessState) > 0) {
		   res = YES;
	   } else if ((newState & targetFailureState) > 0) {
		   res = NO;
	   }
	   return res;
   }
		 forOperationNamed:operationName
		   blockSubsequent:blockSubsequentOperations
			  givenByBlock:operation];
}

- (void)ifInState:(SCIMessageViewerState)expectedCurrentState
allowUninitialized:(BOOL)allowUninitialized
 whenStateReached:(SCIMessageViewerState)targetState
performCompletion:(void (^)(BOOL success))block
forOperationNamed:(NSString *)operationName
  blockSubsequent:(BOOL)blockSubsequentOperations
	 givenByBlock:(BOOL (^)(void))operation
{
	[self ifStateSatisfies:^BOOL(SCIMessageViewerState currentState, SCIMessageViewerError currentError) {
		BOOL res = [self stateMatchesState:expectedCurrentState];
		if (allowUninitialized) {
			res = res || (self.state == SCIMessageViewerStateUninitialized);
		}
		return res;
	}
		whenStateSatisfies:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
			return ((newState & targetState) > 0);
		}
		 performCompletion:block
   passingSuccessParameter:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
	   return ((newState & targetState) > 0);
   }
		 forOperationNamed:operationName
		   blockSubsequent:blockSubsequentOperations
			  givenByBlock:operation];
}

- (void)ifInState:(SCIMessageViewerState)expectedCurrentState
allowUninitialized:(BOOL)allowUninitialized
whenSuccessStateReached:(SCIMessageViewerState)targetSuccessState
orFailureStateReached:(SCIMessageViewerState)targetFailureState 
performCompletion:(void (^)(BOOL))block
forOperationNamed:(NSString *)operationName
  blockSubsequent:(BOOL)blockSubsequentOperations
	 givenByBlock:(BOOL (^)(void))operation
{
	[self ifStateSatisfies:^BOOL(SCIMessageViewerState currentState, SCIMessageViewerError currentError) {
		BOOL res = [self stateMatchesState:expectedCurrentState];
		if (allowUninitialized) {
			res = res || (self.state == SCIMessageViewerStateUninitialized);
		}
		return res;
	}
		whenStateSatisfies:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
			return ((newState & (targetSuccessState | targetFailureState)) > 0);
		}
		 performCompletion:block
	  passingSuccessParameter:^BOOL(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error) {
		  BOOL res = NO;
		  if ((newState & targetSuccessState) > 0) {
			  res = YES;
		  } else if ((newState & targetFailureState) > 0) {
			  res = NO;
		  }
		  return res;
	  }
		 forOperationNamed:operationName
		   blockSubsequent:blockSubsequentOperations
			  givenByBlock:operation];
}

- (void)ifStateSatisfies:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
	  whenStateSatisfies:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
	   performCompletion:(void (^)(BOOL success))completionBlock
 passingSuccessParameter:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
	   forOperationNamed:(NSString *)operationName
		 blockSubsequent:(BOOL)blockSubsequentOperations
			givenByBlock:(BOOL (^)(void))operation
{
	[self.observer ifStateSatisfies:testEligibleBlock
				 whenStateSatisfies:testCompleteBlock
				  performCompletion:completionBlock
			passingSuccessParameter:testSuccessBlock
				  forOperationNamed:operationName
					   givenByBlock:operation
		 blocksSubsequentOperations:blockSubsequentOperations];
}

- (void)setAllOperationsBlocked:(BOOL)allOperationsBlocked
{
	[self.observer setAllOperationsBlocked:allOperationsBlocked];
}

- (BOOL)allOperationsBlocked
{
	return self.observer.allOperationsBlocked;
}

@end

@interface SCIMessageViewerStateChangedBlockWrapper : NSObject
+ (SCIMessageViewerStateChangedBlockWrapper *)wrapperWithTestCompleteBlock:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
														  testSuccessBlock:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
														   completionBlock:(void (^)(BOOL success))completionBlock
															 operationName:(NSString *)operationName
																identifier:(id)identifier
												 blockSubsequentOperations:(BOOL)blockSubsequentOperations;
@property (nonatomic, copy) id identifier;
@property (nonatomic) NSString *operationName;
@property (nonatomic) BOOL blockSubsequentOperations;
@property (nonatomic, copy) BOOL (^testCompleteBlock)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error);
@property (nonatomic, copy) BOOL (^testSuccessBlock)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error);
@property (nonatomic, copy) void (^completionBlock)(BOOL success);
@end

@interface SCIMessageViewerOperationWrapper : NSObject
+ (SCIMessageViewerOperationWrapper *)wrapperWithTestEligibleBlock:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
														 operation:(BOOL (^)(void))operation
												 completionWrapper:(SCIMessageViewerStateChangedBlockWrapper *)completionWrapper;
@property (nonatomic, copy) BOOL (^testEligibleBlock)(SCIMessageViewerState currentState, SCIMessageViewerError currentError);
@property (nonatomic, copy) BOOL (^operation)(void);
@property (nonatomic) SCIMessageViewerStateChangedBlockWrapper *completionWrapper;
@end

@interface SCIMessageViewerObserver ()
@property (nonatomic) NSMutableArray *completionBlocks;
@property (nonatomic) NSMutableArray *queuedOperations;
@end

@implementation SCIMessageViewerObserver

@synthesize allOperationsBlocked = mAllOperationsBlocked;

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.completionBlocks = [[NSMutableArray alloc] init];
		self.queuedOperations = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	
}

#pragma mark - Helpers: Convenience Accessors

- (SCIMessageViewer *)viewer
{
	return [SCIMessageViewer sharedViewer];
}

#pragma mark - Message Viewer API: Notifications

- (void)startObservingMessageViewerNotifications:(SCIMessageViewer *)messageViewer
{
	_messageViewer = messageViewer;
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(messageViewerStateChanged:)
												 name:SCINotificationMessageViewerStateChanged
											   object:self.messageViewer];
}

- (void)stopObservingMessageViewerNotifications
{
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:SCINotificationMessageViewerStateChanged
												  object:self.messageViewer];
	_messageViewer = nil;
}

#pragma mark - Helpers: Performing Callback Blocks

- (void)performCallbackBlock:(void (^)(void))block
{
	if (block) {
		dispatch_async(dispatch_get_main_queue(), ^{
			block();
		});
	}
}

- (void)performCallbackBlock:(void (^)(BOOL))block boolArgument:(BOOL)argument
{
	[self performCallbackBlock:^{
		block(argument);
	}];
}

#pragma mark - Message Viewer API: Accessors

- (void)setAllOperationsBlocked:(BOOL)allOperationsBlocked
{
	mAllOperationsBlocked = allOperationsBlocked;
	[self performEnqueuedOperations];
}

#pragma mark - Message Viewer API: Blocks

- (void)ifStateSatisfies:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
	  whenStateSatisfies:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
	   performCompletion:(void (^)(BOOL success))completionBlock
 passingSuccessParameter:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
	   forOperationNamed:(NSString *)operationName
			givenByBlock:(BOOL (^)(void))operation
blocksSubsequentOperations:(BOOL)blocksSubsequentOperations
{
	
	if (![self operationsBlocked]) {
		[self _ifStateSatisfies:testEligibleBlock
			 whenStateSatisfies:testCompleteBlock
			  performCompletion:completionBlock
		passingSuccessParameter:testSuccessBlock
			  forOperationNamed:operationName
				   givenByBlock:operation
	 blocksSubsequentOperations:blocksSubsequentOperations];
	} else {
		[self _onceOperationsUnblockedIfStateSatisfies:testEligibleBlock
									whenStateSatisfies:testCompleteBlock
									 performCompletion:completionBlock
							   passingSuccessParameter:testSuccessBlock
									 forOperationNamed:operationName
										  givenByBlock:operation
							blocksSubsequentOperations:blocksSubsequentOperations];
	}
}


- (BOOL)terminateFirstOperationNamed:(NSString *)operationName complete:(BOOL (^)(BOOL operationStarted))callCompletion success:(BOOL (^)(BOOL operationStarted))passSuccess
{
	BOOL res = NO;
	BOOL operationStarted = NO;
	void (^completionBlock)(BOOL);
	
	//First check completion blocks since any operations which have already been started
	//	are prior to any operations which have only been queued but not started.
	@synchronized(self.completionBlocks) {
		SCIMessageViewerStateChangedBlockWrapper *theWrapper = nil;
		
		for (SCIMessageViewerStateChangedBlockWrapper *wrapper in self.completionBlocks) {
			if ([wrapper.operationName isEqualToString:operationName]) {
				theWrapper = wrapper;
				break;
			}
		}
		
		if (theWrapper) {
			[self.completionBlocks removeObject:theWrapper];
			
			completionBlock = theWrapper.completionBlock;
			operationStarted = YES;
			res = YES;
		}
	}
	
	//If we didn't find any completion block for an operation which had already been
	//	started, check for the operation in the array of unstarted operations.
	if (!operationStarted) {
		SCIMessageViewerOperationWrapper *theOperation = nil;
		
		@synchronized(self.queuedOperations) {
			for (SCIMessageViewerOperationWrapper *operation in self.queuedOperations) {
				if ([operation.completionWrapper.operationName isEqualToString:operationName]) {
					theOperation = operation;
					break;
				}
			}
		}
		
		if (theOperation) {
			[self.queuedOperations removeObject:theOperation];
			
			completionBlock = theOperation.completionWrapper.completionBlock;
			operationStarted = NO;
			res = YES;
		}
	}

	if (completionBlock &&
		callCompletion(operationStarted)) {
		[self performCallbackBlock:completionBlock boolArgument:passSuccess(operationStarted)];
	}
				
	return res;
}

#pragma mark - Helpers: Queuing Operations

- (BOOL)operationsBlocked
{
	BOOL res = NO;
	
	if (self.allOperationsBlocked) {
		res = YES;
	} else {
		@synchronized(self.completionBlocks) {
			for (SCIMessageViewerStateChangedBlockWrapper *wrapper in self.completionBlocks) {
				if (wrapper.blockSubsequentOperations == YES) {
					res = YES;
					break;
				}
			}
		}
	}
		
	return res;
}

- (void)_onceOperationsUnblockedIfStateSatisfies:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
							  whenStateSatisfies:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
							   performCompletion:(void (^)(BOOL success))completionBlock
						 passingSuccessParameter:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
							   forOperationNamed:(NSString *)operationName
									givenByBlock:(BOOL (^)(void))operation
					  blocksSubsequentOperations:(BOOL)blockSubsequentOperations
{
	SCIMessageViewerStateChangedBlockWrapper *completionWrapper = [SCIMessageViewerStateChangedBlockWrapper wrapperWithTestCompleteBlock:testCompleteBlock
																														testSuccessBlock:testSuccessBlock
																														 completionBlock:completionBlock
																														   operationName:operationName
																															  identifier:nil
																											   blockSubsequentOperations:blockSubsequentOperations];
	SCIMessageViewerOperationWrapper *operationWrapper = [SCIMessageViewerOperationWrapper wrapperWithTestEligibleBlock:testEligibleBlock
																											  operation:operation
																									  completionWrapper:completionWrapper];
	[self enqueueOperation:operationWrapper];
}

- (void)enqueueOperation:(SCIMessageViewerOperationWrapper *)operation
{
	@synchronized(self.queuedOperations) {
		[self.queuedOperations addObject:operation];
	}
}

- (void)performEnqueuedOperations
{
	@synchronized(self.queuedOperations) {
		NSMutableArray *performedOperations = [[NSMutableArray alloc] init];
		
		for (SCIMessageViewerOperationWrapper *operation in self.queuedOperations) {
			if (!self.operationsBlocked) {
				[self _ifStateSatisfies:operation.testEligibleBlock
					 whenStateSatisfies:operation.completionWrapper.testCompleteBlock
					  performCompletion:operation.completionWrapper.completionBlock
				passingSuccessParameter:operation.completionWrapper.testSuccessBlock
					  forOperationNamed:operation.completionWrapper.operationName
						   givenByBlock:operation.operation
			 blocksSubsequentOperations:operation.completionWrapper.blockSubsequentOperations];
				[performedOperations addObject:operation];
			} else {
				break;
			}
		}
		[self.queuedOperations removeObjectsInArray:performedOperations];
	}
}

#pragma mark - Helpers: Handling Operations

- (void)_ifStateSatisfies:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
	  whenStateSatisfies:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
	   performCompletion:(void (^)(BOOL success))completionBlock
 passingSuccessParameter:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
	   forOperationNamed:(NSString *)operationName
			givenByBlock:(BOOL (^)(void))operation
blocksSubsequentOperations:(BOOL)blockSubsequentOperations
{
	if (testEligibleBlock(self.viewer.state, self.viewer.error)) {
		id identifier = [self handleMessageViewerStateSatisfying:testCompleteBlock
												successRequiring:testSuccessBlock
													byPerforming:completionBlock
												   operationName:operationName
									   blockSubsequentOperations:blockSubsequentOperations];
		BOOL success = operation();
		if (!success) {
			[self removeCompletionBlockIdentifiedBy:identifier];
			completionBlock(NO);
		}
	} else {
		[self performCallbackBlock:completionBlock boolArgument:NO];
	}
}

- (id)handleMessageViewerStateSatisfying:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
						  successRequiring:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
							byPerforming:(void (^)(BOOL success))completionBlock
						   operationName:(NSString *)operationName
blockSubsequentOperations:(BOOL)blockSubsequentOperations
{
	id res = nil;
	@synchronized(self.completionBlocks) {
		BNRUUID *identifier = [BNRUUID UUID];
		SCIMessageViewerStateChangedBlockWrapper *wrapper = [SCIMessageViewerStateChangedBlockWrapper wrapperWithTestCompleteBlock:testCompleteBlock
																												  testSuccessBlock:testSuccessBlock
																												   completionBlock:completionBlock
																													 operationName:operationName
																														identifier:identifier
																										 blockSubsequentOperations:blockSubsequentOperations];
		[self.completionBlocks addObject:wrapper];
		res = identifier;
	}
	return res;
}

- (void)removeCompletionBlockIdentifiedBy:(id)identifier
{
	@synchronized(self.completionBlocks) {
		SCIMessageViewerStateChangedBlockWrapper *wrapperToRemove = nil;
		
		for (SCIMessageViewerStateChangedBlockWrapper *wrapper in self.completionBlocks) {
			if ([wrapper.identifier isEqual:identifier]) {
				wrapperToRemove = wrapper;
				break;
			}
		}
		
		if (wrapperToRemove) {
			[self.completionBlocks removeObject:wrapperToRemove];
		}
	}
}

#pragma mark - Helpers: Blocks

- (void)performHandlersForViewerStateChangeWithDictionary:(NSDictionary *)dictionary
{
	SCIMessageViewerState state = [(NSNumber *)dictionary[SCINotificationKeyState] unsignedIntegerValue];
	SCIMessageViewerState oldState = [(NSNumber *)dictionary[SCINotificationKeyOldState] unsignedIntegerValue];
	SCIMessageViewerError error = [(NSNumber *)dictionary[SCINotificationKeyError] unsignedIntegerValue];
	NSArray *wrappers = nil;
	@synchronized(self.completionBlocks) {
		NSMutableArray *handled = [[NSMutableArray alloc] init];
		
		for (SCIMessageViewerStateChangedBlockWrapper *wrapper in self.completionBlocks) {
			if (wrapper.testCompleteBlock(state, oldState, error)) {
				[handled addObject:wrapper];
			}
		}
		
		[self.completionBlocks removeObjectsInArray:handled];
		
		wrappers = handled;
	}
	for (SCIMessageViewerStateChangedBlockWrapper *wrapper in wrappers) {
		wrapper.completionBlock(wrapper.testSuccessBlock(state, oldState, error));
	}
	[self performEnqueuedOperations];
}

#pragma mark - Notifications

- (void)messageViewerStateChanged:(NSNotification *)note // SCINotificationMessageViewerStateChanged
{
	[self performHandlersForViewerStateChangeWithDictionary:note.userInfo];
}

@end

@implementation SCIMessageViewerStateChangedBlockWrapper
+ (SCIMessageViewerStateChangedBlockWrapper *)wrapperWithTestCompleteBlock:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testCompleteBlock
														  testSuccessBlock:(BOOL (^)(SCIMessageViewerState newState, SCIMessageViewerState oldState, SCIMessageViewerError error))testSuccessBlock
														   completionBlock:(void (^)(BOOL success))completionBlock
															 operationName:(NSString *)operationName
																identifier:(id)identifier
												 blockSubsequentOperations:(BOOL)blockSubsequentOperations
{
	SCIMessageViewerStateChangedBlockWrapper *res = [[self alloc] init];
	res.testCompleteBlock = testCompleteBlock;
	res.testSuccessBlock = testSuccessBlock;
	res.completionBlock = completionBlock;
	res.operationName = operationName;
	res.identifier = identifier;
	res.blockSubsequentOperations = blockSubsequentOperations;
	return res;
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p|%@>", NSStringFromClass(self.class), self, self.operationName];
}
@end

@implementation SCIMessageViewerOperationWrapper
+ (SCIMessageViewerOperationWrapper *)wrapperWithTestEligibleBlock:(BOOL (^)(SCIMessageViewerState currentState, SCIMessageViewerError currentError))testEligibleBlock
														 operation:(BOOL (^)(void))operation
												 completionWrapper:(SCIMessageViewerStateChangedBlockWrapper *)completionWrapper
{
	SCIMessageViewerOperationWrapper *wrapper = [[self alloc] init];
	wrapper.testEligibleBlock = testEligibleBlock;
	wrapper.operation = operation;
	wrapper.completionWrapper = completionWrapper;
	return wrapper;
}
@end



@implementation SCIMessageViewer (AsynchronousCompletionPrivate)

static NSString * const SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyObserver = @"SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyObserver";
- (SCIMessageViewerObserver *)observer
{
	return [self bnr_associatedObjectGeneratedBy:^id{return [[SCIMessageViewerObserver alloc] init];} forKey:SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyObserver];
}
- (void)setObserver:(SCIMessageViewerObserver *)observer
{
	[self bnr_setAssociatedObject:observer forKey:SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyObserver];
}

static NSString * const SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyUploadRecordedGreetingTimeoutTimer = @"SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyUploadRecordedGreetingTimeoutTimer";
- (NSTimer *)uploadRecordedGreetingTimeoutTimer
{
	return [self bnr_associatedObjectForKey:SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyUploadRecordedGreetingTimeoutTimer];
}
- (void)setUploadRecordedGreetingTimeoutTimer:(NSTimer *)uploadRecordedGreetingTimeoutTimer
{
	[self bnr_setAssociatedObject:uploadRecordedGreetingTimeoutTimer forKey:SCIMessageViewerAsynchronousCompletionPrivateAssociatedObjectKeyUploadRecordedGreetingTimeoutTimer];
}

@end

