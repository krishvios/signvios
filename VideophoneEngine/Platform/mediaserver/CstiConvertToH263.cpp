#include "CstiConvertToH263.h"

CstiConvertToH263::CstiConvertToH263() : CstiConverterBase()
{
	//---------------------------------------
	IstiVideoEncoder* Encoder;
	IstiVideoEncoder::CreateVideoEncoder(&Encoder, estiVIDEO_H263);
	m_pVideoEncoder = Encoder;

	while (m_pVideoEncoder->AvInitializeEncoder() != stiRESULT_SUCCESS)
	{
		stiTrace("Failure to Initialize Encoder Convert H263");
		Sleep(10);
	}
	//---------------------------------------
	m_eVideoFormat = EstiVideoFrameFormat::estiRTP_H263_RFC2190;
}

CstiConvertToH263::CstiConvertToH263(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate) : CstiConverterBase()
{
	//---------------------------------------
	IstiVideoEncoder* Encoder;
	IstiVideoEncoder::CreateVideoEncoder(&Encoder, estiVIDEO_H263);
	m_pVideoEncoder = Encoder;
	m_eVideoFormat = EstiVideoFrameFormat::estiRTP_H263_RFC2190;
	ResetEncoder(EncodeWidth, EncodeHeight, Bitrate, Framerate);
}

CstiConvertToH263::~CstiConvertToH263()
{
}
