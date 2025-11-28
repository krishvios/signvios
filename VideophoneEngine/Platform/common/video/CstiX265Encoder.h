//*****************************************************************************
//
// FileName:        CstiX265Encoder.h
//
// Abstract:        Declaration of the CstiX265Encoder class used to encode
//					H.265 videos with the x265 encoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#if !defined CSTIX265ENCODER_H
#define CSTIX265ENCODER_H

#include "CstiVideoEncoder.h"

#ifdef WIN32
#define X265_API_IMPORTS
#endif

extern "C" 
{
#include <x265.h>
}

//extern struct PPS;
//extern struct SPS;


class CstiX265Encoder : public CstiVideoEncoder
{
public:
	static stiHResult CreateVideoEncoder(CstiX265Encoder **pX265Encoder);
	
	CstiX265Encoder();
    ~CstiX265Encoder();
        
    stiHResult AvEncoderClose();
	stiHResult AvInitializeEncoder();
    stiHResult AvUpdateEncoderSettings();
        
	stiHResult AvVideoCompressFrame(uint8_t *pInputFrame, FrameData* pOutputData, EstiColorSpace colorSpace);
	void AvVideoFrameRateSet(float fFrameRate);
	
	EstiVideoCodec AvEncoderCodecGet() { return estiVIDEO_H265; }
	EstiVideoFrameFormat AvFrameFormatGet() { return estiLITTLE_ENDIAN_PACKED; }

	stiHResult AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int unLevel, unsigned int unConstraints);

protected:
	EstiVideoProfile m_eProfile;
	unsigned int  m_unConstraints;
	unsigned int m_unLevel;

private:
	void FlushEncoder();
	x265_param* m_context;
	x265_encoder* m_codec;
	bool m_bMustRestart;
};
#endif
