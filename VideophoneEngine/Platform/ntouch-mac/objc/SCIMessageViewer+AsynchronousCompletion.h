//
//  SCIMessageViewer+AsynchronousCompletion.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/4/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer.h"

@interface SCIMessageViewer (AsynchronousCompletion)

- (void)startObservingNotifications;
- (void)stopObservingNotifications;

- (void)startPlayingWithCompletion:(void (^)(BOOL success))block;
- (void)startPlayingFromBeginningWithCompletion:(void (^)(BOOL success))block;
- (void)waitToStartPlayingWithCompletion:(void (^)(BOOL success))block;
- (void)waitToStartPlayingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL success))block;
- (void)pausePlayingWithCompletion:(void (^)(BOOL success))block;
- (void)waitToStartPausingWithCompletion:(void (^)(BOOL success))block;
- (void)waitToStartPausingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL success))block;
- (void)closeWithCompletion:(void (^)(BOOL success))block;

- (void)loadVideoFromServer:(NSString *)server
			   withFilename:(NSString *)filename
				 completion:(void (^)(BOOL success))block;

- (void)startRecordingGreetingWithCompletion:(void (^)(BOOL success))block;
- (void)stopPlayingCountdownForRecordingGreetingWithCompletion:(void (^)(BOOL))block;
- (void)waitToStartRecordingGreetingWithCompletion:(void (^)(BOOL))block;
- (void)waitToStartRecordingGreetingAndBlockOtherOperationsWithCompletion:(void (^)(BOOL))block;
- (void)stopRecordingWithCompletion:(void (^)(BOOL success))block;
- (void)stopRecordingMessageAndUploadWithCompletion:(void (^)(BOOL))block;
- (void)uploadRecordedGreetingWithCompletion:(void (^)(BOOL success))block;
- (void)cancelUploadingRecordedGreeting;
- (void)deleteRecordedGreetingWithCompletion:(void (^)(BOOL success))block;
- (void)loadGreetingWithCompletion:(void (^)(BOOL success))block;

@end
