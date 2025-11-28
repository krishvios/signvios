// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "ISLICRinger.h"
#include "CstiEventQueue.h"


class CstiSLICRingerBase : public ISLICRinger, public CstiEventQueue
{
public:

	CstiSLICRingerBase ();
	~CstiSLICRingerBase () override;

	CstiSLICRingerBase (const CstiSLICRingerBase &other) = delete;
	CstiSLICRingerBase (CstiSLICRingerBase &&other) = delete;
	CstiSLICRingerBase &operator = (const CstiSLICRingerBase &other) = delete;
	CstiSLICRingerBase &operator = (CstiSLICRingerBase &&other) = delete;

	stiHResult start() override;	 
	stiHResult stop() override;
	void deviceDetect () override;

	virtual stiHResult startup ();

protected:

	virtual void eventRingerStart () = 0;
	virtual void eventRingerStop () = 0;
	virtual void eventDeviceDetect () = 0;

	bool m_bFlashing {false};

	CstiSignalConnection::Collection m_signalConnections;
};
