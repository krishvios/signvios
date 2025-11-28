//
//  AppleCalculateCaptureSize.mm
//  ntouch
//
//  Created by Daniel Shields on 5/7/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

#import "AppleCalculateCaptureSize.h"
#import "AppleUtilities.h"
#import "ntouch-Swift.h"

extern "C" void CalculateCaptureSizeApple(
	unsigned int unMaxFS,			// The maximum frame size in macroblocks
	unsigned int unMaxMBps,			// The maximum macroblocks per second
	int nCurrentDataRate,			// The current bit rate
	EstiVideoCodec eVideoCodec,		// The current video codec being used
	unsigned int preferredWidth,
	unsigned int preferredHeight,
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{
	CMVideoDimensions dimensions;
	CMTime frameDuration;
	CMVideoDimensions preferredSize = { (int32_t)preferredWidth, (int32_t)preferredHeight };

	[SCICaptureController.shared
	 calculateCaptureSizeWithMaxMacroblocksPerFrame:unMaxFS
	 maxMacroblocksPerSecond:unMaxMBps
	 bitRate:nCurrentDataRate
	 videoCodec: vpe::ConvertVideoCodecToFourCharCode(eVideoCodec)
	 preferredSize:preferredSize
	 outVideoSize:&dimensions
	 outFrameDuration:&frameDuration];

	*punVideoSizeCols = dimensions.width;
	*punVideoSizeRows = dimensions.height;
	*punFrameRate = frameDuration.timescale / frameDuration.value;
}
