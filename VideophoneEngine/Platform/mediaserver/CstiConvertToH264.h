#pragma once
#define SLICE_TYPE_IDR		5

#include "CstiFFMpegContext.h"
#include "CstiVideoEncoder.h"
#include "SstiMSPacket.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

}

class CstiConvertToH264
{
public:
	CstiConvertToH264(uint32_t nFrameRate);
	~CstiConvertToH264();
	SstiMSPacket* DecodeFrame(unsigned char* buffer, int length);
private:
	AVCodec* m_pDecodeCodec;
	AVCodecContext* m_pVideoDecodeContext;
	AVFrame* m_pFrame;

	uint32_t m_nWidth;
	uint32_t m_nHeight;
	uint8_t* m_pBytes;

	FrameData* m_pFrameStorage;
	IstiVideoEncoder *m_pVideoEncoder;

	void AllocateFrame(uint32_t width, uint32_t height);

};

