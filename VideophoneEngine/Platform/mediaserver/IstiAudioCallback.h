#pragma once
#include "stisvx.h"
#include "IPlatformAudioInput.h"
#include "IPlatformAudioOutput.h"
class IPlatformAudioInput;
class IPlatformAudioOutput;

class IstiAudioCallback
{
public:
	virtual void ReceiveDTMFTone(uint32_t, EstiDTMFDigit) = 0;
};

