#include "CstiAudioCallback.h"

IstiAudioCallback* CstiAudioCallback::AudioCallBack = nullptr;

void CstiAudioCallback::ReceiveDTMFTone(uint32_t callIndex, EstiDTMFDigit dtmfTone)
{
	AudioCallBack->ReceiveDTMFTone(callIndex, dtmfTone);
}

void CstiAudioCallback::SetCallback(IstiAudioCallback* callBack)
{
	AudioCallBack = callBack;
}

