//
//  SCIMessageViewer.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIVideophoneEngine.h"
#import "SCISignMailGreetingInfo.h"

typedef NSInteger SCIMessageViewerState NS_TYPED_ENUM;
typedef NSInteger SCIMessageViewerError NS_TYPED_ENUM;

extern const SCIMessageViewerState SCIMessageViewerStateUninitialized;
extern const SCIMessageViewerState SCIMessageViewerStateOpening;
extern const SCIMessageViewerState SCIMessageViewerStateReloading;
extern const SCIMessageViewerState SCIMessageViewerStatePlayWhenReady;
extern const SCIMessageViewerState SCIMessageViewerStatePlaying;
extern const SCIMessageViewerState SCIMessageViewerStatePaused;
extern const SCIMessageViewerState SCIMessageViewerStateClosing;
extern const SCIMessageViewerState SCIMessageViewerStateClosed;
extern const SCIMessageViewerState SCIMessageViewerStateError;
extern const SCIMessageViewerState SCIMessageViewerStateVideoEnd;
extern const SCIMessageViewerState SCIMessageViewerStateRequestingGuid;
extern const SCIMessageViewerState SCIMessageViewerStateRecordConfigure;
extern const SCIMessageViewerState SCIMessageViewerStateWaitingToRecord;
extern const SCIMessageViewerState SCIMessageViewerStateRecording;
extern const SCIMessageViewerState SCIMessageViewerStateRecordFinished;
extern const SCIMessageViewerState SCIMessageViewerStateRecordClosing;
extern const SCIMessageViewerState SCIMessageViewerStateRecordClosed;
extern const SCIMessageViewerState SCIMessageViewerStateUploading;
extern const SCIMessageViewerState SCIMessageViewerStateUploadComplete;
extern const SCIMessageViewerState SCIMessageViewerStatePlayerIdle;

extern const SCIMessageViewerError SCIMessageViewerErrorNone;
extern const SCIMessageViewerError SCIMessageViewerErrorGeneric;
extern const SCIMessageViewerError SCIMessageViewerErrorServerBusy;
extern const SCIMessageViewerError SCIMessageViewerErrorOpening;
extern const SCIMessageViewerError SCIMessageViewerErrorRequestingGuid;
extern const SCIMessageViewerError SCIMessageViewerErrorParsingGuid;
extern const SCIMessageViewerError SCIMessageViewerErrorParsingUploadGuid;
extern const SCIMessageViewerError SCIMessageViewerErrorRecordConfigIncomplete;
extern const SCIMessageViewerError SCIMessageViewerErrorRecording;
extern const SCIMessageViewerError SCIMessageViewerErrorNoDataUploaded;

@class SCIMessageInfo;

@interface SCIMessageViewer : NSObject

typedef NSInteger SCIMessageViewerState NS_TYPED_ENUM NS_SWIFT_NAME(SCIMessageViewer.State);
typedef NSInteger SCIMessageViewerError NS_TYPED_ENUM NS_SWIFT_NAME(SCIMessageViewer.Error);

@property (class, readonly, nonnull) SCIMessageViewer *sharedViewer NS_SWIFT_NAME(shared);

- (void)openMessage:(nonnull SCIMessageInfo *)message;
- (BOOL)close;
- (BOOL)play;
- (BOOL)playFromBeginning;
- (BOOL)pause;
- (BOOL)getDuration:(nullable NSTimeInterval *)duration currentTime:(nullable NSTimeInterval *)currentTime;
- (BOOL)seekTo:(NSTimeInterval)time;
- (BOOL)skipForward:(NSTimeInterval)time;
- (BOOL)skipBackward:(NSTimeInterval)time;

@property (readonly, nullable) SCIMessageInfo *openedMessage;
@property (readonly, nullable) SCIMessageInfo *heldMessage;

- (void)fetchState;

@property (readonly) SCIMessageViewerState state;
@property (readonly) SCIMessageViewerError error;

- (BOOL)stopRecording;
- (BOOL)signMailRerecord;
- (BOOL)sendRecordedMessage;
- (BOOL)deleteRecordedMessage;
- (BOOL)clearRecordP2PMessageInfo;
- (void)skipSignMailGreeting;

- (BOOL)recordGreeting;
- (BOOL)deleteRecordedGreeting;
- (BOOL)uploadGreeting;
- (void)fetchGreetingInfoWithCompletion:(nonnull void (^)(SCISignMailGreetingInfo * _Nullable info, NSError * _Nullable error))block;

- (BOOL)loadVideoFromServer:(nonnull NSString *)server withFilename:(nonnull NSString *)filename;

@end

@interface SCIMessageViewer (Deprecated)
- (BOOL)messageViewerRecordStop;
- (BOOL)messageViewerRecordConfig;
- (BOOL)messageViewerSendRecordedMessage;
- (BOOL)messageViewerDeleteRecordedMessage;
- (BOOL)messageViewerRecordP2PMessageInfoClear;
@end

extern NSNotificationName _Nonnull const SCINotificationMessageViewerStateChanged; // SCINotificationKeyState -> SCIMessageViewerState
extern NSString * _Nonnull const SCINotificationKeyState;
extern NSString * _Nonnull const SCINotificationKeyOldState;
extern NSString * _Nonnull const SCINotificationKeyError;

#ifdef __cplusplus
extern "C" {
#endif
NSString * _Nonnull NSStringFromSCIMessageViewerState(SCIMessageViewerState state);
NSString * _Nonnull NSStringFromSCIMessageViewerError(SCIMessageViewerError error);
#ifdef __cplusplus
}
#endif
