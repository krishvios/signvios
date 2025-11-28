#pragma once

#include "CstiEventQueue.h"

class MonitorTaskBase: public CstiEventQueue
{
public:

	MonitorTaskBase ()
	:
		CstiEventQueue ("MonitorTask")
	{
	}

	virtual stiHResult RcuConnectedStatusGet (
		bool *pbRcuConnected) = 0;

	virtual stiHResult mpuSerialNotify (
		std::string mpuSerialNumber) {return stiRESULT_SUCCESS;};

	CstiSignal<> usbRcuConnectionStatusChangedSignal;
	CstiSignal<> usbRcuResetSignal;
	CstiSignal<int> hdmiInResolutionChangedSignal;
	CstiSignal<> hdmiOutStatusChangedSignal;
};
