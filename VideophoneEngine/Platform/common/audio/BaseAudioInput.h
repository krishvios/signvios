#pragma once
#include "CstiSignal.h"
#include "IstiAudioInput.h"

class BaseAudioInput : virtual public IstiAudioInput
{
public:
	BaseAudioInput () = default;
	~BaseAudioInput () override = default;

	BaseAudioInput (const BaseAudioInput &) = delete;
	BaseAudioInput (BaseAudioInput &&) = delete;
	BaseAudioInput &operator= (const BaseAudioInput &) = delete;
	BaseAudioInput &operator= (BaseAudioInput &&) = delete;

	ISignal<bool>& packetReadyStateSignalGet () override;
	ISignal<bool>& audioPrivacySignalGet () override;

	CstiSignal<bool> packetReadyStateSignal;
	CstiSignal<bool> audioPrivacySignal;
};

