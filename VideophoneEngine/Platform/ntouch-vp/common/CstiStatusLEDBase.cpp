/*!
 * \file CstiStatusLEDBase.cpp
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2019 Sorenson Communications, Inc. -- All rights reserved
 */


#include "CstiStatusLEDBase.h"
#include "stiTools.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


CstiStatusLEDBase::CstiStatusLEDBase ()
	:
	CstiEventQueue ("CstiStatusLED")
{
}


/*!
* \brief Destroy the Object
* \param none
* \retval none
*/
CstiStatusLEDBase::~CstiStatusLEDBase ()
{
	CstiEventQueue::StopEventLoop();
}


/*!
* \brief Initialize operation
* \param helios shared pointer to global Helios object
* \retval none
*/
stiHResult CstiStatusLEDBase::Initialize (
	std::shared_ptr<CstiHelios> helios)
{
	m_helios = helios;

	for (auto &kv : m_ledMap)
	{
		auto &key = kv.first;
		auto &led = kv.second;
		// Allows us to change the light state from the event queue so that
		// the calling thread never blocks on RcuPacketSend()
		m_signalConnections.push_back (led.lightChangedSignal.Connect (
			[this, &led] {
				PostEvent ([this, &led] {
					EventLedLightChanged (&led);
				});
			}));

		// Create a blink timer for this LED
		m_blinkTimerMap.emplace(key, sci::make_unique<CstiTimer>(0));

		auto &blinkTimer = m_blinkTimerMap[key];
		m_signalConnections.push_back (blinkTimer->timeoutEvent.Connect (
			[this, &led, &blinkTimer]() {
				led.Toggle();
				auto blinking = blinkTimer->IsActive();
				if (blinking && (led.EnabledGet() == CLed::EnableState::Enabled))
				{
					blinkTimer->Restart();
				}
			}));
	}

	return stiRESULT_SUCCESS;
}


void CstiStatusLEDBase::Uninitialize ()
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	m_signalConnections.clear ();
	
	for (auto &kv : m_ledMap)
	{
		auto &led = kv.second;
		led.LightSet (CLed::LightState::Off);
		EventLedLightChanged (&led);
	}
}


/*!
* \brief Starts the event loop for the StatusLED task
* \retval stiHResult
*/
stiHResult CstiStatusLEDBase::Startup ()
{
	CstiEventQueue::StartEventLoop();
	return stiRESULT_SUCCESS;
}


/*!
* \brief Start an LED blinking
* \param eLED which led to start
* \param blinkRate the blink rate in milliseconds
* \retval stiHResult
*/
stiHResult CstiStatusLEDBase::Start (
	EstiLed eLED,
	unsigned int blinkRate)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	auto hResult = stiRESULT_SUCCESS;
	
	if (m_ledMap.count(eLED) > 0)
	{
		auto &led = m_ledMap[eLED];
		auto &blinkTimer = m_blinkTimerMap[eLED];
		led.LightSet(CLed::LightState::On);
		blinkTimer->TimeoutSet(blinkRate);
		if ((led.EnabledGet()==CLed::EnableState::Enabled) && blinkRate > 0)
		{
			blinkTimer->Restart();
		}
	}

	if (m_pulseLEDsEnabled)
	{
		if (eLED == estiLED_SIGNMAIL)
		{
			hResult = m_helios->SignMailSet (true);
			stiTESTRESULT();
		}
		else if (eLED == estiLED_MISSED_CALL)
		{
			hResult = m_helios->MissedCallSet (true);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	return hResult;
}


/*!
* \brief Stop an LED from blinking
* \param eLED which led to stop
* \retval stiHResult
*/
stiHResult CstiStatusLEDBase::Stop (
	EstiLed eLED)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	auto hResult = stiRESULT_SUCCESS;
	
	if (m_ledMap.count(eLED) > 0)
	{
		auto &led = m_ledMap[eLED];
		auto &blinkTimer = m_blinkTimerMap[eLED];
		blinkTimer->Stop();
		led.LightSet(CLed::LightState::Off);
	}

	if (eLED == estiLED_SIGNMAIL)
	{
		hResult = m_helios->SignMailSet (false);
		stiTESTRESULT();
	}
	else if (eLED == estiLED_MISSED_CALL)
	{
		hResult = m_helios->MissedCallSet (false);
		stiTESTRESULT();
	}

STI_BAIL:
	return hResult;
}


/*!
* \brief Enable/Disable an LED. When Enabling it returns to the state it was in (Started/Stopped)
* \param eLED which led to stop
* \param enable enable/disable the LED
* \retval stiHResult
*/
stiHResult CstiStatusLEDBase::Enable (
	EstiLed eLED,
	EstiBool enabled)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	auto hResult = stiRESULT_SUCCESS;
	
	if (m_ledMap.count(eLED) > 0)
	{
		auto &led = m_ledMap[eLED];
		auto &blinkTimer = m_blinkTimerMap[eLED];
		auto blinking = blinkTimer->IsActive();
		led.EnabledSet(enabled ?
			CLed::EnableState::Enabled :
			CLed::EnableState::Disabled);

		if (blinking && blinkTimer->TimeoutGet() > 0)
		{
			blinkTimer->Restart();
		}
	}

//STI_BAIL:
	return hResult;
}


/*!
 * \brief Enables/disables the SignMail/Missed Call notification LED on the Pulse device
 *
 * \param enable Enables/disables Pulse LEDs
 *
 * \return stiHResult
 */
stiHResult CstiStatusLEDBase::PulseNotificationsEnable (
	bool enable)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	auto hResult = stiRESULT_SUCCESS;

	m_pulseLEDsEnabled = enable;

	if (!enable)
	{
		// Make sure we turn the LEDs off if we're disabling them
		hResult = m_helios->SignMailSet (false);
		stiTESTRESULT();

		hResult = m_helios->MissedCallSet (false);
		stiTESTRESULT();
	}

STI_BAIL:
	return hResult;
}


/*!
* \note Appears to be deprecated but part of IstiStatusLED.h
*/
void CstiStatusLEDBase::ColorSet (
	EstiLed eLED,
	uint8_t red,
	uint8_t green,
	uint8_t blue)
{
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_ledMap.count(eLED) > 0)
	{
		auto &led = m_ledMap[eLED];
		led.ColorSet (red, green, blue);
	}
}
