//*****************************************************************************
//
// FileName:        CstiMF265Encoder.h
//
// Abstract:        Declaration of the CstiMF265Encoder class used to encode
//					H.265 videos with the MediaFoundation 265 encoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************
#pragma once

#include "CstiVideoEncoder.h"
#include "CstiMediaFoundation.h"
#include "CstiMFBufferWrapper.h"

class CstiMF265Encoder : public CstiVideoEncoder
{
public:
	static stiHResult CreateVideoEncoder (CstiMF265Encoder **pMF265Encoder);
	~CstiMF265Encoder ();
	stiHResult AvEncoderClose ();
	stiHResult AvInitializeEncoder ();
	stiHResult AvUpdateEncoderSettings ();

	stiHResult AvVideoCompressFrame (uint8_t *pInputFrame, FrameData* pOutputData, EstiColorSpace colorSpace);

	void AvVideoFrameRateSet (float fFrameRate);

	EstiVideoCodec AvEncoderCodecGet () { return estiVIDEO_H265; }
	EstiVideoFrameFormat AvFrameFormatGet () { return estiBYTE_STREAM; }

	virtual void AvVideoRequestKeyFrame ();

	stiHResult AvVideoProfileSet (EstiVideoProfile eProfile, unsigned int unLevel, unsigned int unConstraints);
private:
	CstiMF265Encoder (IMFTransform *pEncoder, ICodecAPI* pCodecAPI);

	stiHResult MediaTypesConfigure ();

	IMFMediaType * m_pInputType;
	IMFMediaType * m_pOutputType;
	IMFTransform * m_pEncoder;
	IMFSample * m_pOutputSample;
	ICodecAPI * m_pCodecAPI;
	IMFMediaBuffer* m_pMediaBuffer;

	EstiVideoProfile m_eProfile;
	unsigned int m_unLevel;
	long long m_nTimeStamp;

	HRESULT SetPropertyInt (GUID id, UINT32 value);
	HRESULT SetPropertyBool (GUID id, bool value);

};


