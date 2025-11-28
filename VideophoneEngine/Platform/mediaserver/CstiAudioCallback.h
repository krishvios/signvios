#pragma once
#include "IstiAudioCallback.h"

class CstiAudioCallback
{
public:

	static void ReceiveDTMFTone (uint32_t callIndex, EstiDTMFDigit dtmfDigit);
	static void SetCallback (IstiAudioCallback* callBack);

private:
	static IstiAudioCallback* AudioCallBack;
	CstiAudioCallback () {};

};