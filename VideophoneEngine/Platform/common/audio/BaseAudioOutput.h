#pragma once
#include "CstiSignal.h"
#include "IstiAudioOutput.h"
#include "stiSVX.h"

class BaseAudioOutput : virtual public IstiAudioOutput
{
public:
	BaseAudioOutput () = default;
	~BaseAudioOutput () override = default ;

	BaseAudioOutput (const BaseAudioOutput &) = delete;
	BaseAudioOutput (BaseAudioOutput &&) = delete;
	BaseAudioOutput &operator= (const BaseAudioOutput &) = delete;
	BaseAudioOutput &operator= (BaseAudioOutput &&) = delete;

	ISignal<bool>& readyStateChangedSignalGet () override;
	ISignal<EstiDTMFDigit>& dtmfReceivedSignalGet () override;

	CstiSignal<bool> readyStateChangedSignal;  // Ready to receive more data packets to play
	CstiSignal<EstiDTMFDigit> dtmfReceivedSignal;
};