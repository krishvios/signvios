#if !defined CSTIANDROIDENCODER_H
#define CSTIANDROIDENCODER_H

#include "CstiVideoEncoder.h"

class CstiAndroidEncoder : CstiVideoEncoder
{
public:
	static stiHResult CreateVideoEncoder(CstiAndroidEncoder **pAndroidEncoder);
	
	CstiAndroidEncoder();
    virtual ~CstiAndroidEncoder();
        
    stiHResult AvEncoderClose();
    stiHResult AvInitializeEncoder();
    stiHResult AvUpdateEncoderSettings();
        
	stiHResult AvVideoCompressFrame(uint8_t *pInputFrame,
									FrameData* pOutputData);
	void AvVideoFrameRateSet(float fFrameRate);
	
	EstiVideoCodec AvEncoderCodecGet() { return estiVIDEO_H264; }
	
private:

};
#endif
