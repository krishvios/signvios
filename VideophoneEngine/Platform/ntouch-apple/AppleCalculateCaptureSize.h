//
//  AppleCalculateCaptureSize.h
//  ntouch
//
//  Created by Daniel Shields on 5/7/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

#pragma once

#include "stiSVX.h"

extern "C" void CalculateCaptureSizeApple(
	unsigned int unMaxFS,			// The maximum frame size in macroblocks
	unsigned int unMaxMBps,			// The maximum macroblocks per second
	int nCurrentDataRate,			// The current bit rate
	EstiVideoCodec eVideoCodec,		// The current video codec being used
	unsigned int preferredWidth,
	unsigned int preferredHeight,
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate);	// return the calculated frame rate
