//*****************************************************************************
//
// FileName:        CstiFFMpegEncoder.h
//
// Abstract:        Declairation of the CstiFFMpegEncoder class used to decode
//					H.264 videos.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#if !defined CSTIFFMPEGENCODER_H
#define CSTIFFMPEGENCODER_H

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

extern "C" 
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

//extern struct PPS;
//extern struct SPS;

class CstiFFMpegEncoder
{
public:
	~CstiFFMpegEncoder();

	stiHResult AvEncoderClose();
	stiHResult AvInitializeEncoder();
	stiHResult AvUpdateEncoderSettings();

	void AvVideoBitrateSet(uint32_t un32Bitrate)
	{
		m_un32Bitrate = un32Bitrate;
	}

	stiHResult AvVideoCompressFrame(
		AVFrame *pAvInputFrame,
		uint64_t un64CurrentTime,
		AVPacket *pAvCompressedPacket);

	void AvVideoFrameRateSet(float fFrameRate);

	void AvVideoFrameSizeSet(
		uint32_t un32VideoWidth,
		uint32_t un32VideoHeight)
	{
		m_n32VideoWidth = un32VideoWidth;
		m_n32VideoHeight = un32VideoHeight;
	}

	void AvVideoFrameSizeGet(
		uint32_t *pun32VideoWidth,
		uint32_t *pun32VideoHeight)
	{
		*pun32VideoWidth = m_n32VideoWidth;
		*pun32VideoHeight = m_n32VideoHeight;
	}

	void AvVideoIFrameSet(int nIFrame)
	{
		m_nIFrame = nIFrame;
	}

	stiHResult AvVideoNalUnitsGet(
		uint8_t *pun8Buffer,
		uint32_t un32BufSize,
		uint32_t *pBufUsed);

	void AvVideoTimeBaseSet(
		int nTimeBaseNum,
		int nTimeBaseDem)
	{
		m_avTimeBase.num = nTimeBaseNum;
		m_avTimeBase.den = nTimeBaseDem;
	}

	static stiHResult CreateFFMpegEncoder(
		CstiFFMpegEncoder **pFFMpegEncoder);

private:
	AVRational m_avFrameRate;
	AVRational m_avTimeBase;
	uint32_t m_un32Bitrate;
	int32_t m_n32VideoHeight;
	int32_t m_n32VideoWidth;
	int m_nIFrame;
	int m_nOutputBufferSize;
	uint8_t *m_pun8OutputBuffer;
	AVCodecContext *m_pAvCodecContext;
	AVFormatContext *m_pAvFormatContext;
	AVFrame *m_pAvFrame;
	AVOutputFormat *m_pAvOutputFormat;
	AVStream *m_pAvVideoStream;

#if 0
//	int m_nVideoStream;
//	struct SwsContext *m_pAvFrameConvert;
#endif
	CstiFFMpegEncoder();

//	HRESULT AvDimensionsGet(
//		int *pnVideoWidth,
//		int *pnVideoHeight);
};
#endif
