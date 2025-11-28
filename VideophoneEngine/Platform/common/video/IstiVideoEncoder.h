//
//  IstiVideoEncoder.h
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef ISTIVIDEOENCODER_h
#define ISTIVIDEOENCODER_h

#if defined stiMVRS_APP || APPLICATION == APP_NTOUCH_PC
#define ACT_FRAME_RATE_METHOD
#endif

#include "stiSVX.h"
#include "stiError.h"
#include "VideoControl.h"
#include "IstiVideoInput.h"

class FrameData;

class IstiVideoEncoder 
{
public:
	static stiHResult CreateVideoEncoder(IstiVideoEncoder **pEncoder, EstiVideoCodec eCodec);
		
	virtual ~IstiVideoEncoder() {}
	
	virtual EstiVideoCodec AvEncoderCodecGet()=0;
	
	virtual stiHResult AvEncoderClose() = 0;
	virtual stiHResult AvInitializeEncoder() = 0;
	virtual stiHResult AvUpdateEncoderSettings() = 0;
	
	virtual void AvVideoBitrateSet(uint32_t un32Bitrate) = 0;
	
	
	virtual stiHResult AvVideoCompressFrame(uint8_t *pInputFrame,  FrameData* pOutputFrame, EstiColorSpace colorSpace) = 0;
	
	virtual void AvVideoFrameRateSet(float fFrameRate) = 0;
	
#ifdef ACT_FRAME_RATE_METHOD
	virtual void AvVideoActualFrameRateSet(float fActualFrameRate) = 0;
#endif
	
	virtual void AvVideoFrameSizeSet(uint32_t un32VideoWidth,
									 uint32_t un32VideoHeight) = 0;
	virtual void AvVideoFrameSizeGet(uint32_t *pun32VideoWidth,
									 uint32_t *pun32VideoHeight) = 0;
	
	virtual stiHResult AvVideoProfileSet(EstiVideoProfile eProfile, 
		unsigned int unLevel,
		unsigned int unConstraints) = 0;

	virtual void AvVideoRequestKeyFrame() = 0;
	
	virtual void AvVideoIFrameSet(int nIFrame) = 0;
	
	virtual EstiVideoFrameFormat AvFrameFormatGet() = 0;
};


#endif
