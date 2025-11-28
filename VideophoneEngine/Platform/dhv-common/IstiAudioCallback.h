#pragma once
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
class CstiAudioInput;
class CstiAudioOutput;

class IstiAudioCallback
{
public:
	virtual void ReceiveDTMFTone(uint32_t, EstiDTMFDigit) = 0;
};

