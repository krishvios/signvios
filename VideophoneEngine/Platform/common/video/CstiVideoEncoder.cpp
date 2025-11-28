//
//  CstiVideoEncoder.cpp
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//

#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <stdint.h>
#endif

#include "CstiVideoEncoder.h"
#ifdef X264_LINK_STATIC
#include "CstiX264Encoder.h"
#endif
#ifdef stiENABLE_H265_ENCODE
    #if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
	#include "CstiX265Encoder.h"
	#endif
#endif
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
#include "CstiH264VTEncoder.h"
#endif

#define DEFAULT_VIDEO_WIDTH 352
#define DEFAULT_VIDEO_HEIGHT 288
#define DEFAULT_VIDEO_BITRATE 256000



// framedata helper
FrameData::FrameData() : m_pFrameData(NULL), m_unFrameLen(0), m_unDataLen(0), m_IsKeyFrame(false) {}
FrameData::~FrameData() {
	if (m_pFrameData) delete [] m_pFrameData;
}
void FrameData::AddFrameData(uint8_t *pData, uint32_t len) {
	if (!m_pFrameData || m_unDataLen + len > m_unFrameLen) {
	  uint8_t *tmp = new uint8_t[m_unDataLen + len*3]; // more smart size needed
		if (m_pFrameData) {
			memcpy(tmp, m_pFrameData, m_unDataLen);
			delete [] m_pFrameData;
		}
		m_pFrameData=tmp;
		m_unFrameLen = m_unDataLen+len*3;
	}
	
	memcpy(m_pFrameData+m_unDataLen, pData, len);
	m_unDataLen += len;
	
}

void FrameData::GetFrameData(uint8_t *pData) const {
	memcpy(pData, m_pFrameData, m_unDataLen);
}


stiHResult IstiVideoEncoder::CreateVideoEncoder(IstiVideoEncoder **pEncoder, EstiVideoCodec eCodec)
{
	switch (eCodec) 
	{
		case estiVIDEO_H263:
		case estiVIDEO_NONE: 
		{
			*pEncoder = NULL;
			return stiRESULT_INVALID_CODEC;
		} 
		break;

		case estiVIDEO_H264: 
		{
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
			return CstiH264VTEncoder::CreateVideoEncoder((CstiH264VTEncoder **)pEncoder);
#else
			return CstiX264Encoder::CreateVideoEncoder((CstiX264Encoder **)pEncoder);
#endif
		}
		break;

#if defined (stiENABLE_H265_ENCODE) && APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
		case estiVIDEO_H265:
		{
			return CstiX265Encoder::CreateVideoEncoder((CstiX265Encoder **)pEncoder);
		} 
		break;
#endif
		
		default:
		{
			*pEncoder = NULL;
			return stiRESULT_INVALID_CODEC;
		} 
		break;
	}
}

CstiVideoEncoder::CstiVideoEncoder():
	m_un32Bitrate (DEFAULT_VIDEO_BITRATE),
	m_n32VideoHeight (DEFAULT_VIDEO_HEIGHT),
	m_n32VideoWidth (DEFAULT_VIDEO_WIDTH),
	m_fFrameRate(15.0),
#ifdef ACT_FRAME_RATE_METHOD
	m_fActualFrameRate(0.0),
#endif
	m_nIFrame (0),
	m_bRequestKeyframe(true) // first frame should always be a keyframe
{
}

void CstiVideoEncoder::AvVideoBitrateSet(uint32_t un32Bitrate)
{
	m_un32Bitrate = un32Bitrate;
	stiTrace( "New Bitrate: %d\n", m_un32Bitrate );
}

#ifdef ACT_FRAME_RATE_METHOD
void CstiVideoEncoder::AvVideoActualFrameRateSet(float fActualFrameRate)
{
	m_fActualFrameRate = fActualFrameRate;
//	stiTrace( "New Actual Frame Rate: %f\n", m_fActualFrameRate );
}
#endif

void CstiVideoEncoder::AvVideoFrameSizeSet(uint32_t un32VideoWidth,
						 uint32_t un32VideoHeight)
{
	m_n32VideoWidth = un32VideoWidth;
	m_n32VideoHeight = un32VideoHeight;
}

void CstiVideoEncoder::AvVideoFrameSizeGet(uint32_t *pun32VideoWidth,
						 uint32_t *pun32VideoHeight)
{
	*pun32VideoWidth = m_n32VideoWidth;
	*pun32VideoHeight = m_n32VideoHeight;
}

void CstiVideoEncoder::AvVideoRequestKeyFrame() 
{ 
	m_bRequestKeyframe = true;
}

void CstiVideoEncoder::AvVideoIFrameSet(int nIFrame)
{
	m_nIFrame = nIFrame;
}

stiHResult CstiVideoEncoder::AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int  unLevel, unsigned int  unConstraints)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eProfile = eProfile;
	m_unLevel = unLevel;
	m_unConstraints = unConstraints;
	return hResult;
}



