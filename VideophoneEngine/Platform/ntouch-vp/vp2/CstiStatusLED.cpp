/*!
 * \file CstiStatusLED.cpp
 * \brief Controls for the status LEDs on the camera and the phone
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2016 Sorenson Communications, Inc. -- All rights reserved
 */


#include "CstiStatusLED.h"
#include "CstiMonitorTask.h"
#include "stiTools.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <rcu_api.h>
#include <fcntl.h>
#include <stdio.h>


/* these two led_id values from packet_arbiter.h */
#define LED_ID_RCU_SIGNMAIL 0x01
#define LED_ID_RCU_MISSEDCALL 0x02
/* this value comes from rcu_api.h */
#define PACKET_TYPE_SET_LEDS 0x02

constexpr const char *STATUS_LED_CLASSPATH = "/sys/class/leds/ncp5623/state";
constexpr const char *STATUS_LED_RGBPATH   = "/sys/class/leds/ncp5623/rgb";

// Nothing special about this number, it just needs to be unique
constexpr const int LED_ID_MPU_STATUS = 0x1234;


/*!
* \brief Construct the Object
* \param PstiObjectCallback (deprecated)
* \param CallbackParam (deprecated)
* \retval none
*/
CstiStatusLED::CstiStatusLED ()
{
	m_ledMap = {{estiLED_SIGNMAIL, CLed(LED_ID_RCU_SIGNMAIL)},
				{estiLED_MISSED_CALL, CLed(LED_ID_RCU_MISSEDCALL)},
				{estiLED_STATUS, CLed(LED_ID_MPU_STATUS)}};
}


/*!
* \brief Destroy the Object
* \param none
* \retval none
*/
CstiStatusLED::~CstiStatusLED ()
{
	m_mfddrvdStatusChangedSignalConnection.Disconnect ();

	if (m_rgbFd > -1)
	{
		close (m_rgbFd);
		m_rgbFd = -1;
	}

	if (m_statusFd > -1)
	{
		write (m_statusFd, "0", 1);			// disable writing to the device
		close (m_statusFd);
		m_statusFd = -1;
	}

}


/*!
* \brief Initialize operation
* \param monitorTask Instance of the monitor task
* \retval none
*/
stiHResult CstiStatusLED::Initialize(
	CstiMonitorTask *monitorTask,
	std::shared_ptr<CstiHelios> helios)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (monitorTask != nullptr, stiRESULT_ERROR);

	// Call base class initialize
	CstiStatusLEDBase::Initialize (helios);

	m_monitorTask = monitorTask;
	m_monitorTask->MfddrvdRunningGet (&m_mfddrvdRunning);

	m_mfddrvdStatusChangedSignalConnection = m_monitorTask->mfddrvdStatusChangedSignal.Connect (
		[this] {
			PostEvent([this] {
				EventMfddrvdStatusChanged ();
			});
		});

	int i;
	// The following is to setup the LEDs on the MPU (not the RCU).
	for (i = 0; i < 4; i++)
	{
		if (m_statusFd < 0 && (m_statusFd = open(STATUS_LED_CLASSPATH, O_RDWR)) < 0)
		{
			usleep (500000); // Sleep for 1/2 second and then try again
		}

		else
		{
			// If it was successful, break out of the loop.
			break;
		}
	}

	// Was it successful?
	if (m_statusFd < 0)
	{
		// Nope.  Throw and bail out.
		stiTHROWMSG(stiRESULT_ERROR, "CstiStatusLED::Initialize: can't open StatusLedClassPath\n");
	}

	write (m_statusFd, "1", 1);			// enable writing to the device

	for (i = 0; i < 4; i++)
	{
		if ((m_rgbFd = open(STATUS_LED_RGBPATH, O_RDWR)) < 0)
		{
			usleep (500000); // sleep for 1/2 second and then try again.
		}

		else
		{
			// If it was successful, break out of the loop.
			break;
		}
	}

	if (m_rgbFd  < 0)
	{
		stiTHROWMSG(stiRESULT_ERROR, "CstiStatusLED::Initialize: Can't open LedRGBPath\n");
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (m_statusFd > 0)
		{
			close (m_statusFd);
			m_statusFd = -1;
		}

		if (m_rgbFd > 0)
		{
			close (m_rgbFd);
			m_rgbFd = -1;
		}
	}

	return (hResult);
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

	if (led->IdGet() == LED_ID_MPU_STATUS)
	{
		// This is the status led on the nvp2 phone itself
		std::array<char, 7> color = {"000000"};

		if (ledOn)
		{
			uint8_t r = 0, g = 0, b = 0;
			std::tie(r,g,b) = led->ColorGet();
			snprintf (color.data(), color.size(), "%2.2x%2.2x%2.2x", r, g, b);
		}

		write (m_rgbFd, color.data(), color.size());
	}
	else
	{
		// This is an led on the RCU
		static constexpr int intensityOn = 75;  // three quarter brightness
		static constexpr int intensityOff = 0;  // turn off led

		status_led_packet packet;
		packet.type = PACKET_TYPE_SET_LEDS;
		packet.data.led_id = led->IdGet();
		packet.data.intensity = ledOn ? intensityOn : intensityOff;

		auto rv = RcuPacketSend ((char *)&packet, sizeof(status_led_packet));
		if (rv != 0)
		{
			stiASSERTMSG (
				false,
				"CstiStatusLED::EventLedLightChanged: WARNING failed to set Led light on the RCU\n");
		}
	}
}


/*!
* \brief Event handler for when the RCU is hot plugged
* \retval none
*/
void CstiStatusLED::EventMfddrvdStatusChanged ()
{
	m_monitorTask->MfddrvdRunningGet (&m_mfddrvdRunning);

	// Enable/disable the LEDs
	for (auto &kv : m_ledMap)
	{
		auto &led = kv.second;
		auto &blinkTimer = m_blinkTimerMap[kv.first];
		auto blinking = blinkTimer->IsActive();

		led.EnabledSet(m_mfddrvdRunning ?
			CLed::EnableState::Enabled :
			CLed::EnableState::Disabled);

		if (m_mfddrvdRunning && blinking)
		{
			// Get the blink timer started again
			blinkTimer->Restart();
		}
	}
}
