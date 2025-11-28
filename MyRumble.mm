//
//  MyRumble.mm
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "MyRumble.h"
#import <AudioToolbox/AudioToolbox.h>
#import "AppDelegate.h"
#import "SCIDefaults.h"

#define kVibrationSeconds 0.4 // iOS vibrations are a fixed duration, unfortunately

@interface MyRumble ()

@property (nonatomic, strong) SCIDefaults *defaults;

@end

@implementation MyRumble

@synthesize pattern;
@synthesize repeats;
@synthesize beat;
@synthesize timer;
#if !TARGET_IPHONE_SIMULATOR
@synthesize captureDevice;
@synthesize captureSession;
@synthesize captureOutput;
#endif

// this class performs a repeatable, interruptable and synchronized vibration/flashing LED pattern based on a string of the format "– –– –––"

- (SCIDefaults *)defaults {
	return [SCIDefaults sharedDefaults];
}

+ (void)vibrate {
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
}

+ (BOOL)deviceHasVibrator {
#if TARGET_IPHONE_SIMULATOR
	return NO;
#else
	NSString *device = [[UIDevice currentDevice] model];
	return ([device hasPrefix:@"iPhone"]);
#endif
}

- (void)turnTorchOn {
#if !TARGET_IPHONE_SIMULATOR
	if (captureDevice && captureDevice.hasFlash && [captureDevice isTorchModeSupported:AVCaptureTorchModeOn])
		if ([captureDevice lockForConfiguration:nil]) { 
			captureDevice.torchMode = AVCaptureTorchModeOn;
			[captureDevice unlockForConfiguration];
		}
	NSTimer *torchtimer = [NSTimer timerWithTimeInterval:kVibrationSeconds target:self selector:@selector(turnTorchOff) userInfo:nil repeats:NO];
	// run on main loop so event will fire if view scrolled
	[[NSRunLoop mainRunLoop] addTimer:torchtimer forMode:NSRunLoopCommonModes];

#endif
}

- (void)turnTorchOff {
#if !TARGET_IPHONE_SIMULATOR
	if (captureDevice && captureDevice.hasFlash && [captureDevice isTorchModeSupported:AVCaptureTorchModeOff])
		if ([captureDevice lockForConfiguration:nil]) { 
			captureDevice.torchMode = AVCaptureTorchModeOff;
			[captureDevice unlockForConfiguration];
		}
#endif
}

+ (BOOL)deviceHasFlash {
#if TARGET_IPHONE_SIMULATOR
	return NO;
#else
	NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice *camera in cameras)
		if (camera.position == AVCaptureDevicePositionBack && camera.hasFlash)
			return YES;
	return NO;
#endif
}

- (void)stopFlashing {
#if !TARGET_IPHONE_SIMULATOR
	if (!captureDevice || !captureDevice.hasFlash)
		return;
    // turn the flash off if it's on
	if (captureDevice.torchMode == AVCaptureTorchModeOn && [captureDevice lockForConfiguration:nil])
		[captureDevice unlockForConfiguration];
	[captureSession stopRunning];
	captureSession = nil;
	captureDevice = nil;
	captureOutput = nil;
#endif
}

- (void)startFlashing {
#if !TARGET_IPHONE_SIMULATOR
	if (!self.defaults.myRumbleFlasher.boolValue)
		return;
	captureDevice = nil;
	
	// look for a back-facing video camera
	NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice *camera in cameras)
		if (camera.position == AVCaptureDevicePositionBack) {
			captureDevice = camera;
			break;
		}
    
	// bail out if no video camera was found
	if (!captureDevice)
		return;
	
	if (!captureDevice.hasFlash) {
		captureDevice = nil;
		return;
	}
    
	if ([captureDevice isTorchModeSupported:AVCaptureTorchModeOff]) {
		if ([captureDevice lockForConfiguration:nil]) { 
			captureDevice.torchMode = AVCaptureTorchModeOff;
			[captureDevice unlockForConfiguration];
		}
	}
	
	// create a bogus capture session
	captureSession = nil;
	captureSession = [[AVCaptureSession alloc] init];
	if ([captureSession canSetSessionPreset:AVCaptureSessionPresetLow]) {
		[captureSession beginConfiguration]; 
		[captureSession setSessionPreset:AVCaptureSessionPresetLow];
		[captureSession commitConfiguration]; 
	}
	
	// configure a bogus input
	NSError *error = nil;
	AVCaptureDeviceInput *videoInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];
	if (videoInput)
		[captureSession addInput:videoInput];
	else
		return;
    
	// configure a bogus output
	captureOutput = [[AVCaptureVideoDataOutput alloc] init];
	if (captureOutput)
		[captureSession addOutput:captureOutput];
	
	// start the bogus session
	[captureSession startRunning];
#endif
}

- (void)rumble {
#if !TARGET_IPHONE_SIMULATOR
	if (self.defaults.myRumbleFlasher.boolValue) {
		if (!captureDevice)
			[self startFlashing];
		[self turnTorchOn];
	}
	[MyRumble vibrate];
#endif
}

- (void)playBeat {
    // avoid an exception
    if (!pattern || pattern.length == 0 || beat > pattern.length - 1)
        return;
    
    switch ([pattern characterAtIndex:beat]) {
        // this is a switch statement in case we add special control characters
        case ' ':
            // nothing to do here unless I insure the flash is turned off;
            break;
        default:
            [self rumble];
            break;
    }
    beat++;
    if (beat >= [pattern length])
        if (repeats)
            beat = 0;
        else
            [self stop];
}

- (void)startTimer {
	[self.timer invalidate];
	self.timer = nil;
	self.timer = [NSTimer timerWithTimeInterval:kVibrationSeconds target:self selector:@selector(playBeat) userInfo:nil repeats:YES];
	// run on main loop so MyRumble pattern will continue if view scrolled
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

- (void)startWithPattern:(NSString *)aPattern repeating:(BOOL)repeating {
	[self stop];
    pattern = [aPattern copy];
    repeats = repeating;
    if (repeats)
        pattern = [pattern stringByAppendingString:@"   "]; // add 3 beats between repetitions of the pattern, indicated by spaces
    beat = 0;
    // defer this so the timer can be reused
    [self performSelector:@selector(startTimer) withObject:nil afterDelay:0];
}

- (void)stop {
    [self.timer invalidate];
    self.timer = nil;
    [self stopFlashing];
}

- (void)dealloc {
    [self stop];
}

#pragma mark -

- (NSString *)beatsToString:(NSArray *)beats {
	if (!beats)
		return nil;
	NSString *s = @"";
	for (NSNumber *aBeat in beats) {
		NSInteger rests = round([aBeat floatValue] / kVibrationSeconds) - 1;
		for (NSInteger rest = 1; rest < rests; rest++)
			s = [s stringByAppendingString:@" "];
		s = [s stringByAppendingString:@"–"];
	}
	return s;
}

- (NSArray *)beatsAtIndex:(NSUInteger)index {
	NSArray *patterns = [appDelegate.myRumblePatterns objectForKey:@"patterns"];
	if (!patterns)
		return nil;
	NSDictionary *aPattern = [patterns objectAtIndex:index];
	if (!aPattern)
		return nil;
	return [aPattern objectForKey:@"beats"];
}

- (NSString *)beatsToStringAtIndex:(NSUInteger)index {
	return [self beatsToString:[self beatsAtIndex:index]];
}

- (NSString *)patternNameAtIndex:(NSUInteger)index {
	NSArray *patterns = [appDelegate.myRumblePatterns objectForKey:@"patterns"];
	NSDictionary *aPattern = [patterns objectAtIndex:index];
	if (!aPattern)
		return nil;
	return [aPattern valueForKey:@"name"];
}

@end
