/*!
 * \file CLed.h
 * \brief Represents an LED
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiSignal.h"
#include <cstdint>


class CLed
{
public:
	enum class LightState { On, Off };
	enum class EnableState { Enabled, Disabled };

public:
	CLed ();
	CLed (int id);

	int IdGet() const
	{
		return m_id;
	}

	CLed::LightState LightGet() const
	{
		return m_lightState;
	}

	CLed::EnableState EnabledGet() const
	{
		return m_enabledState;
	}

	std::tuple<uint8_t, uint8_t, uint8_t> ColorGet() const
	{
		return std::make_tuple(m_red, m_green, m_blue);
	}

	// Signals the LED to be turned on/off
	void Toggle();
	void LightSet(CLed::LightState state);
	void EnabledSet(CLed::EnableState state);
	void ColorSet(uint8_t r, uint8_t g, uint8_t b);

public:
	// Signals that the led should be switched on/off
	CstiSignal<> lightChangedSignal;

private:
	int m_id = -1;
	EnableState m_enabledState = EnableState::Enabled;
	LightState m_lightState = LightState::Off;

	// Color of LED
	uint8_t m_red = 0xFF;
	uint8_t m_green = 0xFF;
	uint8_t m_blue = 0xFF;
};
