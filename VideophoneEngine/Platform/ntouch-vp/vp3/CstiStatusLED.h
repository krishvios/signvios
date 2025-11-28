/*!
 * \file CstiStatusLED.h
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2024 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiStatusLEDBase.h"


class CstiStatusLED : public CstiStatusLEDBase
{
public:

	CstiStatusLED ();
	~CstiStatusLED () override = default;

	CstiStatusLED (const CstiStatusLED &other) = delete;
	CstiStatusLED (CstiStatusLED &&other) = delete;
	CstiStatusLED &operator = (const CstiStatusLED &other) = delete;
	CstiStatusLED &operator = (CstiStatusLED &&other) = delete;

private:
	// Event Handlers
	void EventLedLightChanged (
		CLed *led) override;
};
