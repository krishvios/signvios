#pragma once
#include "CstiConverterBase.h"

class CstiScaleH264 : public CstiConverterBase
{
public:
	CstiScaleH264();
	CstiScaleH264(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate);
	~CstiScaleH264();
};


