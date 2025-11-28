/*!
 * \file CstiStatusLED.h
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiStatusLEDBase.h"

//
// Forward Declarations
//
class CstiMonitorTask;


class CstiStatusLED : public CstiStatusLEDBase
{
public:

	CstiStatusLED ();
	~CstiStatusLED () override;

	CstiStatusLED (const CstiStatusLED &other) = delete;
	CstiStatusLED (CstiStatusLED &&other) = delete;
	CstiStatusLED &operator = (const CstiStatusLED &other) = delete;
	CstiStatusLED &operator = (CstiStatusLED &&other) = delete;

	stiHResult Initialize (
		CstiMonitorTask *monitorTask,
		std::shared_ptr<CstiHelios> helios);

private:
	// Event Handlers
	void EventLedLightChanged (
		CLed *led) override;

	void EventMfddrvdStatusChanged ();

private:
	int m_statusFd = -1;
	int m_rgbFd = -1;
	int m_sock = -1;

	CstiMonitorTask *m_monitorTask = nullptr;
	bool m_mfddrvdRunning = false;

	CstiSignalConnection m_mfddrvdStatusChangedSignalConnection;
};
