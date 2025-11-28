//
//  PreEncoder.m
//  ntouch
//
//  Created by Todd Ouzts on 11/30/10.
//  Copyright 2010 Sorenson Communications. All rights reserved.
//

#import "PreEncoder.h"
#import "VideoExchange.h"
#import "AppDelegate.h"
#import "SService.h"
#import "SAccount.h"
#import "SSettings.h"
//#import <Accelerate/Accelerate.h>
//#import <AVFoundation/AVUtilities.h>

extern "C" {
#include "libavcodec/avcodec.h"
}

@implementation PreEncoder

@synthesize captureProcess;
@synthesize orientation;
@synthesize useCaptureScaling;
@synthesize captionImage;


#pragma mark RGB to YUV conversion

uint32_t rgb2yuv[65536];
uint8_t dealpha[256][256];

void rgb2ycbcr(uint8_t r, uint8_t g,  uint8_t b,  uint8_t *y, uint8_t *cb, uint8_t *cr)
{
	// Y  =  0.257*R + 0.504*G + 0.098*B + 16
	// CB = -0.148*R - 0.291*G + 0.439*B + 128
	// CR =  0.439*R - 0.368*G - 0.071*B + 128
	*y  = (uint8_t)((8432*(unsigned long)r + 16425*(unsigned long)g + 3176*(unsigned long)b + 16*32768)>>15);
	*cb = (uint8_t)((128*32768 + 14345*(unsigned long)b - 4818*(unsigned long)r -9527*(unsigned long)g)>>15);
	*cr = (uint8_t)((128*32768 + 14345*(unsigned long)r - 12045*(unsigned long)g-2300*(unsigned long)b)>>15);
}

void init_tables()
{
	unsigned long i;
	uint8_t y, cb, cr;
	
	for (i = 0; i < 65536; i++) {
		//  Get colour components
		uint8_t r = (i >> 11) & 0x1F;
		uint8_t g = (i >>  5) & 0x3F;
		uint8_t b = i & 0x1F;
		
		//  Scale colours to 8 bits
		
		r <<= 3;
		g <<= 2;
		b <<= 3;
		
		//  Convert to YCbCr
		rgb2ycbcr(r, g, b, &y, &cb, &cr);
		//  Store in mapping table
		rgb2yuv[i] = ((uint32_t)y << 16) | ((uint32_t)cb << 8) | (uint32_t)cr;
	}
	
	for (int a = 0; a < 256; a++) {
		for (int c = 0; c < 256; c++) {
			dealpha[a][c] = (uint8_t)(((uint16_t)c << 8) / (a + 1));
		}
	}    
}

static uint8_t *Youtput = nil;
static uint8_t *Uoutput = nil;
static uint8_t *Voutput = nil;
static uint8_t *Ubuffer = nil;
static uint8_t *Vbuffer = nil;

- (void)encodeFrame {
	AVPicture *yuvPicture = new AVPicture;
	int result = avpicture_fill(yuvPicture, Youtput, PIX_FMT_YUV420P, kFrameWidth, kFrameHeight);
	if (result) {
		MavPicture *yuvImage = [MavPicture new];
		yuvImage.picture = yuvPicture;
		yuvImage.width = kFrameWidth;
		yuvImage.height = kFrameHeight;
		yuvImage.pix_fmt = PIX_FMT_YUV420P;
		[VideoExchange shared].imageOut = yuvImage;
	}
}

- (void)BGRAtoYUV420P1:(CGContextRef)context {
	// this attempt at optimization is a little slower than the version below this function
	
	NSDate *startTime = nil;
	if (YES && kLog)
		startTime = [NSDate date];
	
	// do this once only
	static BOOL didInitTables = NO;
	if (!didInitTables) {
		init_tables();
		didInitTables = YES;
	}
	
	if (!Youtput) {
		Youtput = (uint8_t *)malloc((kFrameWidth * kFrameHeight) + (kFrameWidth * kFrameHeight / 2));
		Uoutput = Youtput + (int)(kFrameWidth * kFrameHeight);
		Voutput = Uoutput + (int)(kFrameWidth * kFrameHeight / 4);
	}
	uint8_t *YoutputPtr = Youtput;
	uint8_t *UoutputPtr = Uoutput;
	uint8_t *VoutputPtr = Voutput;
	
	uint8_t R, G, B, Y, U, V;
	uint32_t YUV;
	size_t width = CGBitmapContextGetWidth(context);
	size_t height = CGBitmapContextGetHeight(context);
	uint8_t *inputPtr = (uint8_t *)CGBitmapContextGetData(context);
	// initialize buffers the first time only
	if (!Ubuffer) {
		Ubuffer = (uint8_t *)malloc(width * height);
		Vbuffer = (uint8_t *)malloc(width * height);
	}
	uint8_t *UbufferPtr = Ubuffer;
	uint8_t *VbufferPtr = Vbuffer;
	size_t row, col;
	
	// first pass
	for (row = 0; row < height; row++) {
		for (col = 0; col < width; col++) {
			
			// read B, G & R values
			B = (*inputPtr++) >> 3;
			G = (*inputPtr++) >> 2;
			R = (*inputPtr) >> 3;
			// skip the A value
			inputPtr += 2;
			
			// do the lookup
			YUV = rgb2yuv[(R << 11) | (G << 5) | B];
			
			// write the Y value
			Y = (YUV >> 16) & 0xFF;
			*YoutputPtr++ = Y;
			
			// cache the U & V values
			U = (YUV >> 8) & 0xFF;
			*UbufferPtr++ = U;
			V = YUV & 0xFF;
			*VbufferPtr++ = V;
		}
	}
	
	// second pass
	uint8_t* Ubuffer0Ptr = Ubuffer;
	uint8_t* Ubuffer1Ptr = Ubuffer + width;
	uint8_t* Vbuffer0Ptr = Vbuffer;
	uint8_t* Vbuffer1Ptr = Vbuffer + width;
	for (row = 0; row < height / 2; row++) {
		for (col = 0; col < width / 2; col++) {
			// start a cluster of 4 pixels and average their cached U & V values
			*UoutputPtr++ = (*Ubuffer0Ptr++ + *Ubuffer0Ptr++ + *Ubuffer1Ptr++ + *Ubuffer1Ptr++) / 4;
			*VoutputPtr++ = (*Vbuffer0Ptr++ + *Vbuffer0Ptr++ + *Vbuffer1Ptr++ + *Vbuffer1Ptr++) / 4;
		}
		Ubuffer0Ptr += width;
		Ubuffer1Ptr += width;
		Vbuffer0Ptr += width;
		Vbuffer1Ptr += width;
	}
	
	if (YES && kLog)
		NSLog(@"APP did BGRAtoYUV420P in:\t%f", [[NSDate date] timeIntervalSinceDate:startTime]);
	
	// encode the YUV frame
	[self encodeFrame];
}

- (void)BGRAtoYUV420P:(CGContextRef)context {
	
	NSDate *startTime = nil;
	if (NO && kLog)
		startTime = [NSDate date];
	
	// do this once only
	static BOOL didInitTables = NO;
	if (!didInitTables) {
		init_tables();
		didInitTables = YES;
	}
	
	if (!Youtput) {
		Youtput = (uint8_t *)malloc((kFrameWidth * kFrameHeight) + (kFrameWidth * kFrameHeight / 2));
		Uoutput = Youtput + (int)(kFrameWidth * kFrameHeight);
		Voutput = Uoutput + (int)(kFrameWidth * kFrameHeight / 4);
	}
	
	UInt8 R, G, B, Y, U, V;
	UInt32 YUV;
	size_t width = CGBitmapContextGetWidth(context);
	size_t height = CGBitmapContextGetHeight(context);
	UInt8 *inputBuffer = (UInt8 *)CGBitmapContextGetData(context);
	UInt8 *inputPtr = inputBuffer;
	UInt8 *YoutputPtr = Youtput;
	size_t index;
	size_t bytesPerRow = CGBitmapContextGetBytesPerRow(context);
	size_t bytesPerPixel = 4;
	size_t superCol, superRow, subCol, subRow;
	size_t Usum, Vsum;
	
	for (superRow = 0; superRow < height / 2; superRow++) {
		for (superCol = 0; superCol < width / 2; superCol++) {
			// start a cluster of 4 pixels and sum their U & V
			Usum = 0;
			Vsum = 0;
			for (subRow = 0; subRow < 2; subRow++) {
				for (subCol = 0; subCol < 2; subCol++) {
					
					inputPtr = inputBuffer + ((superRow * 2 + subRow) * bytesPerRow) + ((superCol * 2 + subCol) * bytesPerPixel);
					//inputPtr = inputBuffer + (2 * bytesPerRow * superRow) + (2*bytesPerPixel*superCol) + (bytesPerRow*subRow) + (bytesPerPixel*subCol);					
					B = (*inputPtr++) >> 3;
					G = (*inputPtr++) >> 2;
					R = (*inputPtr) >> 3;
					
					// do the lookup
					YUV = rgb2yuv[(R << 11) | (G << 5) | B];
					
					// write the Y value
					Y = YUV >> 16;
					
					YoutputPtr = Youtput + ((superRow * 2 + subRow) * width) + (superCol * 2 + subCol);
					//YoutputPtr = Youtput + (2 * width * superRow) + (2 * superCol )+ (width * subRow )+ subCol;
					
					*YoutputPtr = Y;
					
					// sum the U & V values
					U = YUV >> 8;
					V = YUV;
					Usum += U;
					Vsum += V;
				}
			}
			// write the average U & V for this cluster
			index = (superRow * width / 2) + superCol;
			//Uoutput[index] = Voutput[index] = 0;
			Uoutput[index] = Usum / 4;
			Voutput[index] = Vsum / 4;
		}
	}
	
	if (NO && kLog)
		NSLog(@"APP did BGRAtoYUV420P in:\t%f", [[NSDate date] timeIntervalSinceDate:startTime]);
	
	// encode the YUV frame
	[self encodeFrame];
}

static BOOL doDumpBuffer = NO;

- (void)dumpBuffer:(CVPixelBufferRef)pixelBuffer {
	doDumpBuffer = NO;
	// dump the buffer out for examination
	NSData *data = [NSData dataWithBytes:CVPixelBufferGetBaseAddress(pixelBuffer) length:CVPixelBufferGetDataSize(pixelBuffer)];
	NSString *path = [appDelegate applicationDocumentsDirectory];
	path = [path stringByAppendingPathComponent:@"PixelBuffer.yuv"];
	[data writeToFile:path atomically:NO];
}

- (void)cropAndRotateYUVonGPU:(CVPixelBufferRef)pixelBuffer {
	/*
	 http://developer.apple.com/library/ios/documentation/Accelerate/Reference/AccelerateFWRef/AccelerateFWRef.pdf
	 Matrix Transposition
	 vDSP_mtrans (page 314) Creates a transposed matrix C from a source matrix A; single precision.
	 vDSP_mtransD (page 315) Creates a transposed matrix C from a source matrix A; double precision.
	 Matrix and Submatrix Copying
	 vDSP_mmov (page 311) Copies the contents of a submatrix to another submatrix.
	 vDSP_mmovD (page 311) Copies the contents of a submatrix to another submatrix.
	 */
	//	cblas_srot(const int N, float *X, const int incX, float *Y, const int incY, const float c, const float s) __OSX_AVAILABLE_STARTING(__MAC_10_2,__IPHONE_4_0);
}

- (void)cropAndRotateYUV:(CVPixelBufferRef)pixelBuffer {
	
	NSDate *startTime = nil;
	if (NO && kLog)
		startTime = [NSDate date];
	
	if (!pixelBuffer)
		return;
	
	size_t inputWidth = CVPixelBufferGetWidth(pixelBuffer);
	size_t inputHeight = CVPixelBufferGetHeight(pixelBuffer);
	if (inputWidth != 480 || inputHeight != 360)
		return;
	
	//	just to be sure these were all zero
	//	size_t extraColumnsOnLeft, extraColumnsOnRight, extraRowsOnTop, extraRowsOnBottom;
	//	CVPixelBufferGetExtendedPixels(pixelBuffer, &extraColumnsOnLeft, &extraColumnsOnRight, &extraRowsOnTop, &extraRowsOnBottom);
	
	uint8_t *Yinput = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
	uint8_t *UVinput = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
	
	if (!Youtput) {
		Youtput = (uint8_t *)malloc((kFrameWidth * kFrameHeight) + (kFrameWidth * kFrameHeight / 2));
		Uoutput = Youtput + (int)(kFrameWidth * kFrameHeight);
		Voutput = Uoutput + (int)(kFrameWidth * kFrameHeight / 4);
	}
	
	uint8_t *YinputPtr = Yinput;
	uint8_t *UVinputPtr = UVinput; // UVinputPtr points to interleaved U & V bytes
	uint8_t *inputRowPtr = nil;
	uint8_t *UinputPtr = nil;
	uint8_t *VinputPtr = nil;
	
	uint8_t *YoutputPtr = Youtput;
	uint8_t *UoutputPtr = Uoutput;
	uint8_t *VoutputPtr = Voutput;
	
	size_t rowInset = 0, colInset = 0, row = 0, col = 0, bytesToSkip = 0;
	size_t outputWidth = kFrameWidth;
	size_t outputHeight = kFrameHeight;
	
	switch (orientation) {
			
		case UIInterfaceOrientationPortrait: {
			// crop and rotate 90° CW
			
			// Y
			rowInset = (inputHeight - outputWidth) / 2;
			colInset = (inputWidth - outputHeight) / 2;
			inputRowPtr = Yinput + ((inputHeight - rowInset) * inputWidth) + colInset;
			row = 0;
			while (row++ < outputHeight) {
				YinputPtr = inputRowPtr;
				col = 0;
				while (col++ < outputWidth) {
					*YoutputPtr++ = *YinputPtr;
					YinputPtr -= inputWidth;
				}
				inputRowPtr++;
			}
			
			// U & V interleaved
			inputHeight /= 2;
			rowInset /= 2;
			outputWidth /= 2;
			outputHeight /= 2;
			inputRowPtr = UVinput + ((inputHeight - rowInset) * inputWidth) + colInset;
			row = 0;
			while (row++ < outputHeight) {
				UinputPtr = inputRowPtr;
				VinputPtr = inputRowPtr + 1;
				col = 0;
				while (col++ < outputWidth) {
					*UoutputPtr++ = *UinputPtr;
					UinputPtr -= inputWidth;
					*VoutputPtr++ = *VinputPtr;
					VinputPtr -= inputWidth;
				}
				inputRowPtr += 2;
			}
		} break;
			
		case UIInterfaceOrientationPortraitUpsideDown:
			// crop and rotate 90° CCW
			
			// Y
			rowInset = (inputHeight - outputWidth) / 2;
			colInset = (inputWidth - outputHeight) / 2;
			inputRowPtr = Yinput + (rowInset * inputWidth) + (inputWidth - colInset);
			row = 0;
			while (row++ < outputHeight) {
				YinputPtr = inputRowPtr;
				col = 0;
				while (col++ < outputWidth) {
					*YoutputPtr++ = *YinputPtr;
					YinputPtr += inputWidth;
				}
				inputRowPtr--;
			}
			
			// U & V interleaved
			inputHeight /= 2;
			rowInset /= 2;
			outputWidth /= 2;
			outputHeight /= 2;
			inputRowPtr = UVinput + (rowInset * inputWidth) + (inputWidth - colInset);
			row = 0;
			while (row++ < outputHeight) {
				UinputPtr = inputRowPtr;
				VinputPtr = inputRowPtr + 1;
				col = 0;
				while (col++ < outputWidth) {
					*UoutputPtr++ = *UinputPtr;
					UinputPtr += inputWidth;
					*VoutputPtr++ = *VinputPtr;
					VinputPtr += inputWidth;
				}
				inputRowPtr -= 2;
			}
			break;
			
		case UIInterfaceOrientationLandscapeLeft:
			// this is the fastest possible solution because there's no transposition
			
			// Y
			rowInset = (inputHeight - outputHeight) / 2;
			colInset = (inputWidth - outputWidth) / 2;
			YinputPtr += (rowInset * inputWidth) + colInset;
			row = 0;
			while (row++ < outputHeight) {
				// crop only
				memcpy(YoutputPtr, YinputPtr, outputWidth);
				YinputPtr += inputWidth;
				YoutputPtr += outputWidth;
			}
			
			// U & V interleaved
			rowInset /= 2;
			outputWidth /= 2;
			outputHeight /= 2;
			UVinputPtr += (rowInset * inputWidth) + colInset;
			bytesToSkip = (colInset * 2);
			row = 0;
			while (row++ < outputHeight) {
				col = 0;
				while (col++ < outputWidth) {
					*UoutputPtr++ = *UVinputPtr++;
					*VoutputPtr++ = *UVinputPtr++;
				}
				UVinputPtr += bytesToSkip;
			}
			break;
			
		case UIInterfaceOrientationLandscapeRight:
			// crop and rotate 180° (flip and mirror)
			
			// Y
			rowInset = (inputHeight - outputHeight) / 2;
			colInset = (inputWidth - outputWidth) / 2;
			YinputPtr += ((inputHeight - rowInset) * inputWidth) + (inputWidth - colInset);
			bytesToSkip = (colInset * 2);
			row = 0;
			while (row++ < outputHeight) {
				col = 0;
				while (col++ < outputWidth) {
					*YoutputPtr++ = *YinputPtr--;
				}
				YinputPtr -= bytesToSkip;
			}
			
			// U & V interleaved
			rowInset /= 2;
			outputWidth /= 2;
			outputHeight /= 2;
			inputHeight /= 2;
			UVinputPtr += ((inputHeight - rowInset) * inputWidth) + (inputWidth - colInset);
			bytesToSkip = (colInset * 2);
			row = 0;
			while (row++ < outputHeight) {
				col = 0;
				while (col++ < outputWidth) {
					*UoutputPtr++ = *UVinputPtr--;
					*VoutputPtr++ = *UVinputPtr--;
				}
				UVinputPtr -= bytesToSkip;
			}
			break;
	}
	
	// encode the YUV frame
	[self encodeFrame];
	
	if (NO && kLog)
		NSLog(@"APP did cropAndRotateYUV in:\t%f", [[NSDate date] timeIntervalSinceDate:startTime]);
}

#pragma mark -

// Create an image from sample buffer data
- (UIImage *)processBuffer:(CMSampleBufferRef)sampleBuffer {
	
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(imageBuffer, 0);
	UIImage *uiImage = nil;
	
	switch (captureProcess) {
		case useCoreGraphics: {
			CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB(); 
			CGContextRef context1 = CGBitmapContextCreate(CVPixelBufferGetBaseAddress(imageBuffer),
														  CVPixelBufferGetWidth(imageBuffer),
														  CVPixelBufferGetHeight(imageBuffer),
														  8,
														  CVPixelBufferGetBytesPerRow(imageBuffer),
														  colorSpace,
														  kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst); // ?
			CGColorSpaceRelease(colorSpace);
			CGImageRef cgImage = CGBitmapContextCreateImage(context1);
			size_t w = CGImageGetWidth(cgImage);
			size_t h = CGImageGetHeight(cgImage);
			
			size_t bytesPerRow = kFrameWidth * 4;
			//bytesPerRow += (16 - bytesPerRow % 16) % 16;
			CGContextRef context = CGBitmapContextCreate(NULL,
														 kFrameWidth,
														 kFrameHeight,
														 CGBitmapContextGetBitsPerComponent(context1),
														 bytesPerRow,
														 CGBitmapContextGetColorSpace(context1),
														 CGBitmapContextGetBitmapInfo(context1));
			
			//	CGContextClipToRect(context, kFrameWidth, kFrame);
			CGContextSaveGState(context);
			CGRect rect;
			
			switch (orientation) {
				case UIInterfaceOrientationLandscapeRight: // home button on the right
					CGContextRotateCTM(context, radians(180));
					CGContextTranslateCTM(context, -kFrameWidth, -kFrameHeight);
					if (useCaptureScaling)
						rect = CGRectMake(0, 0, kFrameWidth, kFrameHeight); // scaling is 4 times slower but yields a wider field of view
					else
						rect = CGRectMake((kFrameWidth - w) / 2, (kFrameHeight - h) / 2, w, h); // clip
					break;
				case UIInterfaceOrientationLandscapeLeft: // home button on the left
					if (useCaptureScaling)
						rect = CGRectMake(0, 0, kFrameWidth, kFrameHeight); // scaling is 4 times slower but yields a wider field of view
					else
						rect = CGRectMake((kFrameWidth - w) / 2, (kFrameHeight - h) / 2, w, h); // clip
					break;
				case UIInterfaceOrientationPortraitUpsideDown:
					CGContextRotateCTM(context, radians(90));
					CGContextTranslateCTM(context, 0, -kFrameWidth);
					rect = CGRectMake((kFrameHeight - w) / 2, (kFrameWidth - h) / 2, w, h);
					break;
				case UIInterfaceOrientationPortrait:
				default:
					CGContextRotateCTM(context, radians(-90));
					CGContextTranslateCTM(context, -kFrameHeight, 0);
					rect = CGRectMake((kFrameHeight - w) / 2, (kFrameWidth - h) / 2, w, h);
					break;
			}
			CGContextDrawImage(context, rect, cgImage);
			CGImageRelease(cgImage);
			CGContextRestoreGState(context);
			
			// composite any cached DeafStar caption image into place
			if (captionImage) {
				CGSize captionSize = CGSizeMake(CGImageGetWidth(captionImage), CGImageGetHeight(captionImage));
				CGRect insetFrame = CGRectZero;
				switch (appDelegate.service.account.settings.deafStarGravity.integerValue) {
					case eDSGravityTop:
						insetFrame = CGRectMake(round((kFrameWidth - captionSize.width) / 2), kFrameHeight - kDSInset - captionSize.height, captionSize.width, captionSize.height);
						break;
					case eDSGravityBottom:
						insetFrame = CGRectMake(round((kFrameWidth - captionSize.width) / 2), kDSInset, captionSize.width, captionSize.height);
						break;
					case eDSGravityTopLeft:
						insetFrame = CGRectMake(kDSInset, kFrameHeight - kDSInset - captionSize.height, captionSize.width, captionSize.height);
						break;
					case eDSGravityTopRight:
						insetFrame = CGRectMake(kFrameWidth - captionSize.width - kDSInset, kFrameHeight - kDSInset - captionSize.height, captionSize.width, captionSize.height);
						break;
					case eDSGravityBottomLeft:
						insetFrame = CGRectMake(kDSInset, kDSInset, captionSize.width, captionSize.height);
						break;
					case eDSGravityBottomRight:
						insetFrame = CGRectMake(kFrameWidth - captionSize.width - kDSInset, kDSInset, captionSize.width, captionSize.height);
						break;
				}
				CGContextDrawImage(context, insetFrame, captionImage);
			}
			
			if (kLoopBackCoreGraphicsFrames) {
				CGImageRef processedImage = CGBitmapContextCreateImage(context);
				if (processedImage) {
					uiImage = [[UIImage alloc] initWithCGImage:processedImage];
					CGImageRelease(processedImage);
				}
			}
			else
				[self BGRAtoYUV420P:context];
			
			CGContextRelease(context);
			
		} break;
			
		case useDirectMemoryAccess:
			[self cropAndRotateYUV:imageBuffer];
			break;
	}
	
	CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
	return uiImage;
}

- (void)dealloc {
	if (captionImage)
		CGImageRelease(captionImage);
	[super dealloc];
}

@end
