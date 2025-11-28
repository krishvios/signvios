//
//  MyRumble.h
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface MyRumble : NSObject {
    NSString *pattern;
    BOOL repeats;
    NSUInteger beat;
    NSTimer *timer;
#if !TARGET_IPHONE_SIMULATOR
	AVCaptureDevice *captureDevice; // for flashing the LED
	AVCaptureSession *captureSession; // for flashing the LED
	AVCaptureVideoDataOutput *captureOutput; // for flashing the LED
#endif
}

@property (nonatomic, retain) NSString *pattern;
@property (nonatomic, assign) BOOL repeats;
@property (nonatomic, assign) NSUInteger beat;
@property (nonatomic, retain) NSTimer *timer;
#if !TARGET_IPHONE_SIMULATOR
@property (nonatomic, retain) AVCaptureDevice *captureDevice;
@property (nonatomic, retain) AVCaptureSession *captureSession;
@property (nonatomic, retain) AVCaptureVideoDataOutput *captureOutput;
#endif

+ (void)vibrate;
+ (BOOL)deviceHasVibrator;
+ (BOOL)deviceHasFlash;
- (NSArray *)beatsAtIndex:(NSUInteger)index;
- (NSString *)beatsToStringAtIndex:(NSUInteger)index;
- (NSString *)patternNameAtIndex:(NSUInteger)index;
- (void)startWithPattern:(NSString *)aPattern repeating:(BOOL)repeating;
- (void)stop;

@end
