/*!
 * \file CLed.cpp
 * \brief Represents an LED
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CLed.h"


CLed::CLed ()
	:
	CLed (-1)
{
}

CLed::CLed (int id)
	:
	m_id(id)
{
}


/*!\brief Toggles the LED light state asynchronously
 * \retval none
 */
void CLed::Toggle()
{
	LightSet(m_lightState == LightState::On ?
		LightState::Off :
		LightState::On);
}


/*!\brief Sets the LED light on or off asynchronously
 * \param state On or Off state the LED should be set to
 * \retval none
 */
void CLed::LightSet(LightState state)
{
	if (m_lightState != state)
	{
		m_lightState = state;
		if (m_enabledState == EnableState::Enabled)
		{
			lightChangedSignal.Emit();
		}
	}
}

/*!\brief Enables or disables the LED
 * \param state Enabled or Disabled state the LED should be set to
 * \retval none
 */
void CLed::EnabledSet(EnableState state)
{
	m_enabledState = state;
	lightChangedSignal.Emit();
}


/*!\brief Sets the color of the LED
 * \param r Red portion of RGB color
 * \param g Green portion of RGB color
 * \param b Blue portion of RGB color
 * \retval none
 */
void CLed::ColorSet(uint8_t r, uint8_t g, uint8_t b)
{
	m_red = r;
	m_green = g;
	m_blue = b;

	if (m_enabledState == EnableState::Enabled)
	{
		lightChangedSignal.Emit();
	}
}
