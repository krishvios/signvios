/*!
 * \file CstiStatusLEDBase.h
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <cstdint>
#include <memory>
#include <map>

#include "IstiStatusLED.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiTimer.h"
#include "CLed.h"
#include "CstiHelios.h"


class CstiStatusLEDBase : public IstiStatusLED, public CstiEventQueue
{
public:

	CstiStatusLEDBase ();
	~CstiStatusLEDBase () override;

	CstiStatusLEDBase (const CstiStatusLEDBase &other) = delete;
	CstiStatusLEDBase (CstiStatusLEDBase &&other) = delete;
	CstiStatusLEDBase &operator = (const CstiStatusLEDBase &other) = delete;
	CstiStatusLEDBase &operator = (CstiStatusLEDBase &&other) = delete;

	stiHResult Initialize (
		std::shared_ptr<CstiHelios> helios);

	void Uninitialize ();

	stiHResult Startup ();

	static IstiStatusLED *InstanceGet ();

	stiHResult Start (
		EstiLed eLED,
		unsigned int blinkRate) override;

	stiHResult Stop (
		EstiLed eLED) override;

	 stiHResult Enable (
		EstiLed eLED,
		EstiBool bEnable) override;

	 stiHResult PulseNotificationsEnable(
		 bool enable) override;

	// unused
	void ColorSet (
		EstiLed /*eLED*/,
		uint8_t /*red*/,
		uint8_t /*green*/,
		uint8_t /*blue*/) override;

protected:
	// Event Handlers
	virtual void EventLedLightChanged (CLed *led) = 0;

protected:
	std::map<int, CLed> m_ledMap;

	CstiTimer m_blinkTimer;
	std::map<int, std::unique_ptr<CstiTimer>> m_blinkTimerMap;

	CstiSignalConnection::Collection m_signalConnections;

	std::shared_ptr<CstiHelios> m_helios;
	bool m_pulseLEDsEnabled = false;
};
