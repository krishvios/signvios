//*****************************************************************************
//
// FileName:     CstiFFMpegDecoder.h
//
// Abstract:     Declairation of the CstiFFMpegDecoder class used to decode
//				 H.264 videos.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#if !defined CSTIFFMPEGDECODER_H
#define CSTIFFMPEGDECODER_H

// __STDC_CONSTANT_MACROS must be defined so that UINT64_C will be defined in stdint.h
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#endif

#include "stiSVX.h"
#include "stiError.h"
#include <stdio.h>
#include <stdlib.h>


extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class CstiFFMpegDecoder
{
public:
	~CstiFFMpegDecoder();

	stiHResult OpenInputFile(
		const char *ptszInputFile);

	stiHResult AvDecoderClose();

	stiHResult AvFrameGet(
		AVFrame **pAvFrameSample,
		uint64_t *pun64FrameSampleTime);

	stiHResult AvVideoFrameResize(
		uint32_t un32FrameWidth,
		uint32_t un32FrameHeight);

	void AvVideoFrameSizeGet(
		uint32_t *pun32Width,
		uint32_t *pun32Height)
	{
		*pun32Width = m_un32VideoWidth;
		*pun32Height = m_un32VideoHeight;
	}

	static stiHResult CreateFFMpegDecoder(
		CstiFFMpegDecoder **pFFMpegDedoder);

private:
	AVFormatContext *m_pAvFormatContext;
	AVCodecContext *m_pAvCodecContext;
	AVFrame *m_pAvFrame;
	struct SwsContext *m_pAvFrameConvert;
	AVFrame *m_pAvResizeFrame;
	uint32_t m_un32VideoWidth;
	uint32_t m_un32VideoHeight;
	int m_nVideoStream;

	CstiFFMpegDecoder();

//	HRESULT AvDimensionsGet(
//		int *pnVideoWidth,
//		int *pnVideoHeight);
};
#endif
