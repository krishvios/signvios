#pragma once
#define SLICE_TYPE_IDR		5

#include "CstiFFMpegContext.h"
#include "CstiVideoEncoder.h"
#include "IstiVideoPacket.h"

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "libswscale\swscale.h"
}

using namespace Common;

class CstiConverterBase
{
public:
	virtual VideoPacket* DecodeFrame(unsigned char* buffer, int length);
	virtual VideoPacket* EncodeWaterMark();
	void AddWaterMark(uint8_t* WaterMark);
	void RequestKeyFrame();
	stiHResult ResetEncoder(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate);
	stiMUTEX_ID m_UsageLock;
protected:
	CstiConverterBase();
	~CstiConverterBase();

	IstiVideoEncoder *m_pVideoEncoder;
	EstiVideoFrameFormat m_eVideoFormat;

private:
	void ApplyWaterMark(uint8_t* pBuffer);
	
	SwsContext* m_pContext;
	AVCodec* m_pDecodeCodec;
	AVCodecContext* m_pVideoDecodeContext;
	AVFrame* m_pFrame;
	uint8_t* m_pEncodeBuffer;

	uint32_t m_nEncodeWidth;
	uint32_t m_nEncodeHeight;
	uint32_t m_nBitrate;
	uint32_t m_nFrameRate;


	uint32_t m_nWidth;
	uint32_t m_nHeight;
	uint8_t* m_pBytes;

	uint8_t* m_pWaterMark;

	FrameData* m_pFrameStorage;
	int Clamp(int Value, int Max, int Min);

	void AllocateFrame(uint32_t width, uint32_t height);
};


