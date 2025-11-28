/*!
 * \file CstiStatusLED.cpp
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2024 Sorenson Communications, Inc. -- All rights reserved
 */


#include "CstiStatusLED.h"
#include "LedApi.h"


CstiStatusLED::CstiStatusLED ()
{
	m_ledMap = {{estiLED_MISSED_CALL, CLed (static_cast<int>(LED_TYPE::MISSED_CALL))},
				{estiLED_SIGNMAIL, CLed (static_cast<int>(LED_TYPE::SIGNMAIL))}};
}

/*!
* \brief Event handler for when a LED's light state should be modified -
*        communicates to the hardware to change the light state
* \param led The LED whose light state needs to be set
* \retval none
*/
void CstiStatusLED::EventLedLightChanged (CLed *led)
{ 
	stiLOG_ENTRY_NAME (CstiStatusLED::EventLedStateChanged);


	bool ledOn = (led->LightGet() == CLed::LightState::On &&
				led->EnabledGet() == CLed::EnableState::Enabled);

	uint8_t r = 0, g = 0, b = 0;
	
	if (ledOn)
	{
		std::tie(r,g,b) = led->ColorGet();
	}
	
	statusLedSet (static_cast<LED_TYPE>(led->IdGet ()), r, g, b);

}
