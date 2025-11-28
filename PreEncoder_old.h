//
//  PreEncoder.h
//  ntouch
//
//  Created by Todd Ouzts on 11/30/10.
//  Copyright 2010 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#define kFrameWidth 352.0
#define kFrameHeight 288.0
#define kLoopBackCoreGraphicsFrames NO				// for testing purposes, soon to be deprecated
#define kDSInset 20

enum CaptureProcess {
	useCoreGraphics,
	useDirectMemoryAccess,
	useOpenGL
};

// DeafStar settings
enum DSGravity {
	eDSGravityBottomRight,
	eDSGravityBottom,
	eDSGravityBottomLeft,
	eDSGravityTopLeft,
	eDSGravityTop,
	eDSGravityTopRight,
	eDSGravities
};

@class VideoExchange;

@interface PreEncoder : NSObject {
	CaptureProcess captureProcess;
	UIInterfaceOrientation orientation;
	BOOL useCaptureScaling; // affects CoreGraphics landscape views only
	CGImageRef captionImage;
}

@property (nonatomic, assign) CaptureProcess captureProcess;
@property (nonatomic, assign) UIInterfaceOrientation orientation;
@property (nonatomic, assign) BOOL useCaptureScaling;
@property (nonatomic, assign) CGImageRef captionImage;

static inline double radians (double degrees) {return degrees / 57.2958;} // or degrees * M_PI / 180

- (UIImage *)processBuffer:(CMSampleBufferRef)sampleBuffer;

@end
