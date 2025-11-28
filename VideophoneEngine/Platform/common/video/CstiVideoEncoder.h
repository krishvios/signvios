//
//  CstiVideoEncoder.h
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef CSTIVIDEOENCODER_h
#define CSTIVIDEOENCODER_h

#include "IstiVideoEncoder.h"


// data structure
class FrameData {
	private:
		uint8_t *m_pFrameData;
		uint32_t m_unFrameLen;
		uint32_t m_unDataLen;
		bool m_IsKeyFrame;
		EstiVideoFrameFormat m_eFormat;	//!< The format of the frame
		
	public:
		FrameData();
		~FrameData();
		void AddFrameData(uint8_t *pData, uint32_t len);
		void Reset() { m_unDataLen = 0; m_IsKeyFrame=false;}
		uint32_t GetDataLen() { return m_unDataLen; }
		void GetFrameData (uint8_t *pData) const;
		void SetKeyFrame() { m_IsKeyFrame=true; }
		bool IsKeyFrame() const { return m_IsKeyFrame; }
		EstiVideoFrameFormat GetFrameFormat() { return m_eFormat; }
		void SetFrameFormat(EstiVideoFrameFormat eFormat) { m_eFormat=eFormat; }
};

class CstiVideoEncoder : public IstiVideoEncoder
{
public:
	virtual ~CstiVideoEncoder() {}
	
	virtual stiHResult AvEncoderClose() = 0;
	virtual stiHResult AvInitializeEncoder() = 0;
	virtual stiHResult AvUpdateEncoderSettings() = 0;
	
	void AvVideoBitrateSet(uint32_t un32Bitrate);
	
	virtual stiHResult AvVideoCompressFrame(uint8_t *pInputFrame, FrameData *pOutputFrame, EstiColorSpace colorSpace) = 0;
	
	void AvVideoFrameRateSet(float fFrameRate) {
		m_fFrameRate = fFrameRate;
	}
	
	stiHResult AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int  unLevel, unsigned int  unConstraints);

#ifdef ACT_FRAME_RATE_METHOD
	void AvVideoActualFrameRateSet(float fActualFrameRate);
#endif
	
	void AvVideoFrameSizeSet(uint32_t un32VideoWidth,
							 uint32_t un32VideoHeight);
	void AvVideoFrameSizeGet(uint32_t *pun32VideoWidth,
							 uint32_t *pun32VideoHeight);
	
	virtual void AvVideoRequestKeyFrame();
	
	void AvVideoIFrameSet(int nIFrame);
	
protected:
	CstiVideoEncoder();
	
	uint32_t m_un32Bitrate;
	int32_t m_n32VideoHeight;
	int32_t m_n32VideoWidth;
	int m_nIFrame;
	
	float m_fFrameRate;
	
#ifdef ACT_FRAME_RATE_METHOD
	float m_fActualFrameRate;
#endif
	
	bool m_bRequestKeyframe;
	unsigned int m_unLevel;
	EstiVideoProfile m_eProfile;
	unsigned int  m_unConstraints;
};


#endif
