//*****************************************************************************
//
// FileName:        CstiMF265Decoder.h
//
// Abstract:        Declaration of the CstiMF265Decoder class used to decode
//					H.265 videos with the MediaFoundation 265 decoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************
#pragma once

#include "IstiVideoDecoder.h"
#include "CstiMediaFoundation.h"
#include "CstiMFBufferWrapper.h"
#include "CstiBitStreamReader.h"

#define HEVC_MAX_WIDTH 1920
#define HEVC_MAX_HEIGHT 1080

class CstiMF265Decoder : public IstiVideoDecoder
{
public:
	static stiHResult CreateMF265Decoder (IstiVideoDecoder **pDedoder);

	~CstiMF265Decoder ();

	virtual stiHResult AvDecoderInit (EstiVideoCodec esmdVideoCodec);
	virtual stiHResult AvDecoderClose ();
	virtual stiHResult AvDecoderDecode (uint8_t *pBytes, uint32_t un32Len, uint8_t * flags);
	virtual stiHResult AvDecoderPictureGet (uint8_t *pBytes, uint32_t un32Len, uint32_t *width, uint32_t *height);
	virtual EstiVideoCodec AvDecoderCodecGet () { return estiVIDEO_H265; }

private:
	CstiMF265Decoder (IMFTransform *pDecoder);
	stiHResult MediaTypesConfigure (uint32_t width, uint32_t height);
	uint32_t m_un32VideoWidth;
	uint32_t m_un32VideoHeight;
	IMFMediaType * m_pInputType;
	IMFMediaType * m_pOutputType;
	IMFTransform * m_pDecoder;

	IMFSample * m_pOutputSample;
	IMFMediaBuffer* m_pMediaBuffer;
	bool GrabVideoData (uint8_t *pBytes, uint32_t un32Len);
	int FindStartCode (uint8_t *pBytes, uint32_t un32Len, int Index);
};

