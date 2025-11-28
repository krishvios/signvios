#pragma once
#include "CstiConverterBase.h"

class CstiConvertToH263 : public CstiConverterBase
{
public:
	CstiConvertToH263();
	CstiConvertToH263(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate);
	~CstiConvertToH263();
};

