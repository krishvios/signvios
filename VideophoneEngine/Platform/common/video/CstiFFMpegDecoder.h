//*****************************************************************************
//
// FileName:     CstiFFMpegDecoder.h
//
// Abstract:     Declairation of the CstiFFMpegDecoder class used to decode
//				 H.264 videos.
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#if !defined CSTIFFMPEGDECODER_H
#define CSTIFFMPEGDECODER_H

#include "IstiVideoDecoder.h"
#include "IstiVideoOutput.h"
#include "stiError.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif


extern "C" 
{
#include "libavcodec/avcodec.h"
}

class CstiFFMpegDecoder : IstiVideoDecoder
{
public:
	static stiHResult CreateFFMpegDecoder(IstiVideoDecoder **pFFMpegDedoder);
	~CstiFFMpegDecoder();

	virtual stiHResult AvDecoderInit(EstiVideoCodec esmdVideoCodec);
	virtual stiHResult AvDecoderClose();
	virtual stiHResult AvDecoderDecode(uint8_t *pBytes, uint32_t un32Len, uint8_t * flags);
	virtual stiHResult AvDecoderDecode(IstiVideoPlaybackFrame *pFrame, uint8_t * flags);
	virtual stiHResult AvDecoderPictureGet(uint8_t *pBytes, uint32_t un32Len, uint32_t *width, uint32_t *height);
	virtual EstiVideoCodec AvDecoderCodecGet() { return m_esmdVideoCodec; }

private:

	EstiVideoCodec m_esmdVideoCodec;
	AVCodec *m_pAvCodec;
	AVCodecContext *m_pAvCodecContext;
	AVFrame *m_pAvFrame;
	uint32_t m_un32VideoWidth;
	uint32_t m_un32VideoHeight;
	uint32_t m_un32ErrorCount;

	CstiFFMpegDecoder();
	
	stiHResult AvDimensionsGet(int *pnVideoWidth, int *pnVideoHeight);
};
#endif
