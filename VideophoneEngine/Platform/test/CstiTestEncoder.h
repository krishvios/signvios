//
//  CstiTestEncoder.h
//  gktest
//
//  Created by Dennis Muhlestein on 12/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#ifndef __gktest__CstiTestEncoder__
#define __gktest__CstiTestEncoder__

#include "CstiVideoEncoder.h"


class CstiTestEncoder : public CstiVideoEncoder
{

  private:
		uint8_t *m_encodedVideo;
		uint32_t m_encodedVideoLen;
		uint32_t m_encodedVideoOffset;

	public:
		static stiHResult CreateVideoEncoder(CstiTestEncoder **pTestEncoder);
		
		CstiTestEncoder() : m_encodedVideo(NULL),
			m_encodedVideoLen(0), m_encodedVideoOffset(0) {}
		stiHResult AvEncoderClose();
		stiHResult AvInitializeEncoder();
		EstiVideoCodec AvEncoderCodecGet() { return estiVIDEO_H264; }

		stiHResult AvUpdateEncoderSettings() { return stiRESULT_SUCCESS;}
		
		stiHResult AvVideoCompressFrame(uint8_t *pInputFrame, 
												FrameData *pOutputFrame);
		
		void AvVideoFrameRateSet(float fFrameRate) {}
};

#endif /* defined(__gktest__CstiTestEncoder__) */
