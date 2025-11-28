#pragma once

#include "MonitorTaskBase.h"

class CstiMonitorTask: public MonitorTaskBase
{
public:

	CstiMonitorTask () = default;

	stiHResult Initialize ();

	stiHResult Startup ();

	bool headphoneConnectedGet();
	bool microphoneConnectedGet();
	stiHResult RcuConnectedStatusGet (
		bool *pbRcuConnected) override;

	CstiSignal<int> hdmiInStatusChangedSignal;

	CstiSignal<> mfddrvdStatusChangedSignal;

	CstiSignal<bool> headphoneConnectedSignal;
	CstiSignal<bool> microphoneConnectedSignal;
};
