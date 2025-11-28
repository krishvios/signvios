// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include <mutex>

#include "IstiPlatform.h"
#include "CstiMonitorTask.h"
#include "CstiVideoInput.h"
#include "CstiVideoOutput.h"
//#include "CstiGStreamerVideoGraph.h"
#include "CstiAudioHandler.h"
#include "CstiBluetooth.h"
//#include "CstiAudioOutput.h"
#include "CstiAudioInput.h"

class CstiAdvancedStatusBase
{
protected:

	CstiAdvancedStatusBase () = default;
	virtual ~CstiAdvancedStatusBase () = default;

public:
	
	CstiAdvancedStatusBase (const CstiAdvancedStatusBase &other) = delete;
	CstiAdvancedStatusBase (CstiAdvancedStatusBase &&other) = delete;
	CstiAdvancedStatusBase &operator= (const CstiAdvancedStatusBase &other) = delete;
	CstiAdvancedStatusBase &operator= (CstiAdvancedStatusBase &&other) = delete;
	
	virtual stiHResult initialize (
			CstiMonitorTask *monitorTask,
			CstiAudioHandler *audioHandler,
			CstiAudioInput *audioInput,
			CstiVideoInput *videoInput,
			CstiVideoOutput *videoOutput,
			CstiBluetooth *bluetooth);

	stiHResult advancedStatusGet (
		SstiAdvancedStatus &advancedStatus);

	std::string mpuSerialGet ();
	std::string rcuSerialGet ();
	
protected:

	virtual stiHResult mpuSerialSet () = 0;

	virtual void staticStatusGet () = 0;
	virtual void dynamicStatusGet () = 0;
	
	SstiAdvancedStatus m_advancedStatus;
	std::recursive_mutex m_statusMutex;
	
	CstiMonitorTask *m_monitorTask = nullptr;
	CstiAudioHandler *m_audioHandler = nullptr;
	CstiAudioInput *m_audioInput = nullptr;
	CstiVideoInput *m_videoInput = nullptr;
	CstiVideoOutput *m_videoOutput = nullptr;
	CstiBluetooth *m_bluetooth = nullptr;

private:

	void avAndSystemStatusUpdate ();
	void wifiStatusUpdate ();
};
