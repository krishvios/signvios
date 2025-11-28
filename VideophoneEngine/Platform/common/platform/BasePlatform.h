#pragma once
#include "IstiPlatform.h"
#include "CstiSignal.h"

class BasePlatform : virtual public IstiPlatform
{
public:
	BasePlatform () = default;
	~BasePlatform () override = default;

	BasePlatform (const BasePlatform &) = delete;
	BasePlatform (BasePlatform &&) = delete;
	BasePlatform &operator= (const BasePlatform &) = delete;
	BasePlatform &operator= (BasePlatform &&) = delete;

	// Event Signals
	ISignal<const std::string&>& geolocationChangedSignalGet () override;
	ISignal<>& geolocationClearSignalGet () override;
	ISignal<int>& hdmiInStatusChangedSignalGet () override;
	ISignal<const PlatformCallStateChange &>& callStateChangedSignalGet () override;

	// Event Signals
	CstiSignal<const PlatformCallStateChange &> callStateChangedSignal;
	CstiSignal<const std::string&> geolocationChangedSignal;
	CstiSignal<> geolocationClearSignal;
	CstiSignal<int> hdmiInStatusChangedSignal;
};

