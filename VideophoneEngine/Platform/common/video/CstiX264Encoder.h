//*****************************************************************************
//
// FileName:        CstiFFMpegEncoder.h
//
// Abstract:        Declairation of the CstiFFMpegEncoder class used to decode
//					H.264 videos.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#if !defined CSTIX264ENCODER_H
#define CSTIX264ENCODER_H

#include "CstiVideoEncoder.h"

#ifdef WIN32
#define X264_API_IMPORTS
#endif

extern "C" 
{
#include <x264.h>
}

//extern struct PPS;
//extern struct SPS;


class CstiX264Encoder : public CstiVideoEncoder
{
public:
	enum class EncoderPresets
	{
		UltraFast,
		SuperFast,
		VeryFast,
		Faster,
		Fast,
		Medium,
		Slow,
		Slower,
		VerySlow,
		Placebo
	};

	static stiHResult CreateVideoEncoder(CstiX264Encoder **pX264Encoder);
	
	CstiX264Encoder();
    ~CstiX264Encoder();
        
    stiHResult AvEncoderClose();
    stiHResult AvInitializeEncoder();
    stiHResult AvUpdateEncoderSettings();
        
	stiHResult AvVideoCompressFrame(uint8_t *pInputFrame, FrameData* pOutputData, EstiColorSpace colorSpace);
	void AvVideoFrameRateSet(float fFrameRate);
	
	EstiVideoCodec AvEncoderCodecGet() {
		return estiVIDEO_H264;
	}
	
	EstiVideoFrameFormat AvFrameFormatGet() {
		return estiLITTLE_ENDIAN_PACKED;
	}

	stiHResult AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int unLevel, unsigned int unConstraints);

	void AVEncoderPresetSet(EncoderPresets);

protected:
	EstiVideoProfile m_eProfile;
	unsigned int  m_unConstraints;
	unsigned int m_unLevel;

private:
	std::string encoderPreset = "ultrafast";
   x264_param_t m_context;
   x264_t       * m_codec;
	EstiBool	m_bMustRestart;

};
#endif
