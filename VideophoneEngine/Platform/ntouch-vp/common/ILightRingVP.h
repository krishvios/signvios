/*!
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "IstiLightRing.h"


class ILightRingVP : public IstiLightRing
{
public:

	virtual void lightRingTurnOn (uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity) = 0;
	virtual void lightRingTurnOff () = 0;

	virtual void lightRingTestPattern (bool on) = 0;

	virtual void alertLedsTurnOn (uint16_t intensity) = 0;
	virtual void alertLedsTurnOff () = 0;

	virtual void alertLedsTestPattern (int intensity) = 0;
};
