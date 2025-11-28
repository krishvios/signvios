// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiAdvancedStatusBase.h"

class CstiAdvancedStatus : public CstiAdvancedStatusBase
{
public:
	
	CstiAdvancedStatus () = default;
	~CstiAdvancedStatus () override = default;

	CstiAdvancedStatus (const CstiAdvancedStatus &other) = delete;
	CstiAdvancedStatus (CstiAdvancedStatus &&other) = delete;
	CstiAdvancedStatus &operator= (const CstiAdvancedStatus &other) = delete;
	CstiAdvancedStatus &operator= (CstiAdvancedStatus &&other) = delete;

	stiHResult initialize (
		CstiMonitorTask *monitorTask,
		CstiAudioHandler *audioHandler,
		CstiAudioInput *audioInput,
		CstiVideoInput *videoInput,
		CstiVideoOutput *videoOutput,
		CstiBluetooth *bluetooth) override;

private:

	stiHResult mpuSerialSet () override;

	void staticStatusGet () override;
	void dynamicStatusGet () override;

	stiHResult rcuStaticStatusRead ();
	stiHResult rcuDynamicStatusRead ();

	//
	// Event Handlers
	//
	CstiSignalConnection::Collection m_signalConnections;
	void eventMfddrvdStatusChanged ();
};
