#include "CstiScaleH264.h"

CstiScaleH264::CstiScaleH264() : CstiConverterBase()
{
	IstiVideoEncoder* Encoder;
	IstiVideoEncoder::CreateVideoEncoder(&Encoder, estiVIDEO_H264);
	m_pVideoEncoder = Encoder;

	while (m_pVideoEncoder->AvInitializeEncoder() != stiRESULT_SUCCESS)
	{
		stiTrace("Failure to Initialize Encoder Scale H264");
		Sleep(10);
	}

	m_eVideoFormat = EstiVideoFrameFormat::estiLITTLE_ENDIAN_PACKED;
}


CstiScaleH264::CstiScaleH264(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate) : CstiConverterBase()
{
	IstiVideoEncoder* Encoder;
	IstiVideoEncoder::CreateVideoEncoder(&Encoder, estiVIDEO_H264);
	m_pVideoEncoder = Encoder;
	m_eVideoFormat = EstiVideoFrameFormat::estiLITTLE_ENDIAN_PACKED;
	ResetEncoder(EncodeWidth, EncodeHeight, Bitrate, Framerate);
}


CstiScaleH264::~CstiScaleH264()
{
}
