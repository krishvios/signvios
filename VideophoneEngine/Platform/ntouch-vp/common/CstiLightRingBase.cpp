///
/// \file CstiLightRingBase.cpp
/// \brief Definition of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
///
//
// Includes
//

#include <cmath>

#include "CstiLightRingBase.h"
#include "CstiMonitorTask.h"
#include "stiDebugTools.h"

//
// Constant
//
#define stiLIGHTRING_TASK_NAME				"stiLIGHTRING"
#define stiLIGHTRING_TASK_PRIORITY			251
#define stiLIGHTRING_MAX_MESSAGES_IN_QUEUE	20
#define stiLIGHTRING_MAX_MSG_SIZE			512
#define stiLIGHTRING_STACK_SIZE				2048

#ifndef MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE
#define MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE (MAX_ANIMATION_FRAME * sizeof(lightring_frame_rgb) + sizeof(lr_frame_rgb_packet))
#endif //MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE 

#define FLASHER_DURATION 100		// flash for .1 seconds
#define FLASHER_GAP 500				// flash every .5 seconds

/*! \struct SstiLightRingSample
 *
 * \brief Renders information for one measure
 * 			of the light ring pattern
 *
 */
struct SstiLightRingSample
{
	uint16_t unLeds;				///! Each of the 8 LEDs is represented by 2 bits:
									///!  00 == off;
									///!  01 == on (full);
									///!  10 == Not Used;
									///!  11 == on (specified brightness)
	uint8_t un8Brightness;			///! Brightness level (ranges from 0 - 255)
	int duration;					///! How long to "play" the entry in milliseconds
	IstiLightRing::EstiLedColor color;	///! have a different color map per frame
};


const std::vector<SstiLightRingSample> rsOff =
{
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_OFF}
};

const std::vector<SstiLightRingSample> rsWipe =
{
	{0x0001, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4004, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1010, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0440, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0100, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0100, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0440, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1010, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4004, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0001, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4400, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0101, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0044, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0010, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0010, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0044, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0101, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4400, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
};

const std::vector<SstiLightRingSample> rsFilledWipe =
{
	{0x0001, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4005, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5015, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5455, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},

	{0x5554, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1550, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0540, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0100, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},

	{0x1000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5400, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5501, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5551, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},

	{0x5155, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0155, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0054, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0010, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
};

const std::vector<SstiLightRingSample> rsFlash =
{
	{0x5555, 0, 150, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
};      //!< Flashes all LED's.

const std::vector<SstiLightRingSample> rsSingle =
{
	{0x4000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1000, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0400, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0100, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0040, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0010, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0004, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0001, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
}; //!< Rotates a single LED around the ring.

const std::vector<SstiLightRingSample> rsDouble =
{
	{0x0101, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0404, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1010, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4040, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
}; //!< Rotates 2 opposite LED's around the ring.

const std::vector<SstiLightRingSample> rsHalf =
{
	{0x0055, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0154, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0550, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1540, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5500, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5401, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5005, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4015, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
};  //!< Rotates half the LED's around the ring.

const std::vector<SstiLightRingSample> rsDarkChaser =
{
	{0x1554, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0555, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4155, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5055, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5415, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5505, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5541, 0, 100, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5550, 0, 100, IstiLightRing::estiLED_COLOR_WHITE}
};  //!< Rotates 2 dark LED's side-by-side around the ring.

const std::vector<SstiLightRingSample> rsStandard =
{
	{0x5555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 20, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 20, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 20, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0000, 0, 255, IstiLightRing::estiLED_COLOR_WHITE}
};    //!< Looks like a phone ring sounds.

const std::vector<SstiLightRingSample> rsBiFlash =
{
	{0x4444, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1111, 0, 50, IstiLightRing::estiLED_COLOR_WHITE}
};

const std::vector<SstiLightRingSample> rsFill =
{
	{0x0000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0001, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0005, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0015, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0055, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0155, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x0555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x1555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5555, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5554, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5550, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5540, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5500, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5400, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x5000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE},
	{0x4000, 0, 50, IstiLightRing::estiLED_COLOR_WHITE}
};

const std::vector<SstiLightRingSample> rsEmergency =
{
	{0x4444, 0, 100, IstiLightRing::estiLED_COLOR_RED},
	{0x1111, 0, 100, IstiLightRing::estiLED_COLOR_BLUE}
};

const std::vector<SstiLightRingSample> rsTest =
{
	{0xffff, 0, 1000, IstiLightRing::estiLED_COLOR_RED},
	{0xffff, 0, 1000, IstiLightRing::estiLED_COLOR_GREEN},
	{0xffff, 0, 1000, IstiLightRing::estiLED_COLOR_BLUE}
};


/*!
 * \brief Array of standard ring patterns.
 */
const std::map<IstiLightRing::Pattern, const std::vector<SstiLightRingSample> *> BaseRingPatternMap =
{
	{IstiLightRing::Pattern::Off, &rsOff},
	{IstiLightRing::Pattern::Wipe, &rsWipe},
	{IstiLightRing::Pattern::FilledWipe, &rsFilledWipe},
	{IstiLightRing::Pattern::Flash, &rsFlash},
	{IstiLightRing::Pattern::SingleChaser, &rsSingle},
	{IstiLightRing::Pattern::DoubleChaser, &rsDouble},
	{IstiLightRing::Pattern::HalfChaser, &rsHalf},
	{IstiLightRing::Pattern::DarkChaser, &rsDarkChaser},
	{IstiLightRing::Pattern::Pulse, &rsStandard},
	{IstiLightRing::Pattern::AlternateFlash, &rsBiFlash},
	{IstiLightRing::Pattern::FillAndUnfill, &rsFill},
	{IstiLightRing::Pattern::Emergency, &rsEmergency},
	{IstiLightRing::Pattern::Test, &rsTest}
};

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//
//
// Class Definitions
//

/*!
 * \brief Default constructor for CstiLightRingBase
 */
CstiLightRingBase::CstiLightRingBase (const std::map<IstiLightRing::EstiLedColor, lr_led_rgb> &colorMap)
:
	CstiEventQueue (stiLIGHTRING_TASK_NAME),
	m_colorMap (colorMap),
	m_lightRingTimer (0, this),
	m_flasherTimer (FLASHER_GAP, this),
	m_lightRingNotificationsTimer(0, this)
{
	// Connect the timers to some handlers
	m_signalConnections.push_back (m_lightRingTimer.timeoutSignal.Connect (
			std::bind(&CstiLightRingBase::eventTimer, this)));

	m_signalConnections.push_back (m_flasherTimer.timeoutSignal.Connect (
			std::bind(&CstiLightRingBase::eventFlasherTimer, this)));

	m_signalConnections.push_back (m_lightRingNotificationsTimer.timeoutSignal.Connect (
			std::bind(&CstiLightRingBase::eventLightRingNotificationsTimer, this)));
}


/*!
 * \brief Default destructor for CstiLightRingBase
 */
CstiLightRingBase::~CstiLightRingBase ()
{
	Stop ();
	FlasherStop ();
}


/*!
 * \brief Initialization happens once and is 
 * controlled by the  
 * 
 * \return stiHResult stiRESULT_SUCCESS
 * \return stiHResult stiRESULT_TASK_INIT_FAILED
 */
stiHResult CstiLightRingBase::Initialize (
	std::shared_ptr<CstiHelios> helios)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (nullptr != helios, stiRESULT_ERROR);

	m_helios = helios;

STI_BAIL:
	return hResult;
}


lr_led_rgb CstiLightRingBase::colorGet (
	const std::map<IstiLightRing::EstiLedColor, lr_led_rgb> &colorMap,
	IstiLightRing::EstiLedColor ledColor)
{
	lr_led_rgb rgbColor = {0xff, 0xff, 0xff};

	auto iter = colorMap.find(ledColor);

	if (iter != colorMap.end())
	{
		rgbColor = (*iter).second;
	}

	return rgbColor;
}


void CstiLightRingBase::PatternSet (
	Pattern patternId,
	EstiLedColor color,
	int brightness,
	int flasherBrightness)
{
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::PatternSet: patternId = %d\n", (int)patternId);
	);

	PostEvent (std::bind (&CstiLightRingBase::eventPatternSet, this, patternId, color, brightness, flasherBrightness));
}

void CstiLightRingBase::eventPatternSet (
	Pattern patternId,
	EstiLedColor color,
	int brightness,
	int flasherBrightness)
{
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::eventPatternSet: patternId = %d\n", (int)patternId);
	);

	auto iter = BaseRingPatternMap.find(patternId);

	if (iter == BaseRingPatternMap.end ())
	{
		return;
	}

	const std::vector<SstiLightRingSample> &pattern = *(*iter).second;

	int frameCount = std::min((int)pattern.size (), MAX_ANIMATION_FRAME);
	
	m_lightringFrames.resize (frameCount);

	for (size_t i = 0; i < m_lightringFrames.size (); i++)
	{
		m_lightringFrames[i].frame_delay = pattern[i].duration;
		
		for (size_t j = 0; j < NUM_LR_LEDS; j++)
		{
			if (pattern[i].unLeds & (0x0001 << (j * 2)))
			{
				if (color == estiLED_USE_PATTERN_COLORS)
				{
					m_lightringFrames[i].lr_vals[j] = colorGet (m_colorMap, pattern[i].color);
				}
				else
				{
					m_lightringFrames[i].lr_vals[j] = colorGet (m_colorMap, color);
				}
			}
			else
			{
				m_lightringFrames[i].lr_vals[j] = {0x00, 0x00, 0x00};
			}
		}
	}
	
	m_brightness = brightness & 0xff;
	m_flasherBrightness = std::max(std::min(flasherBrightness, FLASHER_MAX), FLASHER_MIN);
	m_color = color;

	// Setup helios paramters
	m_patternId = patternId;
}

void CstiLightRingBase::notificationPatternSet (
	NotificationPattern pattern)
{
	PostEvent (std::bind (&CstiLightRingBase::eventNotificationPatternSet, this, pattern));
}

void CstiLightRingBase::eventNotificationPatternSet (
	NotificationPattern pattern)
{
	switch (pattern)
	{
	case NotificationPattern::SignMail:
		m_lightringNotificationFrames = parseTestVectors (signMailPatternGet ());
		break;

	case NotificationPattern::MissedCall:
		m_lightringNotificationFrames = parseTestVectors (missedCallPatternGet ());
		break;

	case NotificationPattern::Both:
		m_lightringNotificationFrames = parseTestVectors (missedCallAndSignMailPatternGet ());
		break;

	case NotificationPattern::None:
		// nothing
		break;
	}
}

void CstiLightRingBase::Start (
	int duration)
{
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::Start: duration = %d\n", duration);
	);

	PostEvent (std::bind (&CstiLightRingBase::eventStart, this, duration, true));
}


/*!
 * \brief  Starts the Light Ring with set pattern. 
 *  
 * \note  If no pattern is set, this function does nothing 
 * 
 * \param int nDuration 
 */
void CstiLightRingBase::eventStart (
	int duration,
	bool includeFlasher)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::eventStart: duration = %d\n", duration);
	);
	
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_includeFlasher = includeFlasher;
	

	if (m_helios)
	{
		hResult = HeliosFramesSend ();
		stiASSERTMSG (!stiIS_ERROR(hResult), "CstiLightRingBase::eventStart: WARNING Failed to send frames to helios\n");
	}

	if (m_sendFrames)
	{
		hResult = LightRingFramesSend ();
		stiASSERTMSG (!stiIS_ERROR(hResult), "CstiLightRingBase::eventStart: WARNING Failed to send frames to lightring\n");

		if (m_includeFlasher)
		{
			m_flasherTimer.start ();
		}
	}

	if (duration)
	{
		m_lightRingTimer.timeoutSet (duration * 1000);
		m_lightRingTimer.start ();
	}
	
	m_bPlaying = true;
	
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::eventStart: exit\n");
	);
}


void CstiLightRingBase::Stop ()
{
	PostEvent (std::bind(&CstiLightRingBase::eventStop, this));
}


/*!
 * \brief Stops the Light Ring 
 *  
 */
void CstiLightRingBase::eventStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	lr_packet LightringPacket;
	
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::eventStop: enter\n");
	);
	
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_lightRingTimer.stop ();
	
	hResult = m_helios->Stop ();
	
	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG (false, "CstiLightRingBase::eventStop: WARNING Failed to Stop helios\n");
	}

	if (m_sendFrames)
	{
		int nReturn = 0;
		
		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRingBase::eventStop() - MFDDRVD is running\n");
		);

		memset (&LightringPacket, 0, sizeof(lr_packet));
		
		LightringPacket.type = PACKET_TYPE_SET_LRANIMATION_OFF;
		
		nReturn = RcuPacketSend ((char *)&LightringPacket, sizeof(lr_packet));
	
		if (nReturn != 0)
		{
			stiASSERTMSG (estiFALSE, "CstiLightRingBase::eventStop: WARNING failed to stop lightring\n");
		}

		if (m_includeFlasher)
		{
			FlasherStop ();
		}
	}
	
	m_bPlaying = false;
	lightRingStoppedSignal.Emit();

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::eventStop: exit\n");
	);
}

stiHResult CstiLightRingBase::HeliosFramesSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::HeliosFramesSend\n");
	);

	if (m_helios)
	{
		lr_led_rgb lrLedRgb = colorGet (m_helios->ColorMapGet (), m_color);
		uint8_t brightness = m_helios->BrightnessMap (m_brightness);

		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRingBase::HeliosFramesSend: m_patternId = %d\n", (int)m_patternId);
		);

		hResult = m_helios->PresetPatternStart ((int)m_patternId, lrLedRgb.r, lrLedRgb.g, lrLedRgb.b, brightness);
		stiTESTRESULTMSG ("CstiLightRingBase::HeliosFramesSend: WARNING Failed to send frames to helios\n");
	}

STI_BAIL:
	return hResult;
}


stiHResult CstiLightRingBase::LightRingFramesSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn = 0;

	if (m_lightringFrames.size () > 0)
	{
		char PacketBuffer[MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE] = {0};
		int nPacketBufferSize = 0;

		lightring_set_brightness_scale (m_brightness);

		if (m_lightringFrames.size () == 1)
		{
			lr_packet *pLrPacket;

			pLrPacket = (lr_packet *)PacketBuffer;
			pLrPacket->type = PACKET_TYPE_SET_LIGHTRING;
			memcpy((void *)&pLrPacket->leds, (void *)&m_lightringFrames[0].lr_vals, NUM_LR_LEDS * sizeof(lr_led));
			nPacketBufferSize = sizeof(lr_packet);
		}
		else
		{

			lr_frame_rgb_packet *pLrFramePacket;

			pLrFramePacket = (lr_frame_rgb_packet *)PacketBuffer;
			pLrFramePacket->type = PACKET_TYPE_SET_LRANIMATION_RGB;
			pLrFramePacket->numframes = m_lightringFrames.size ();
			memcpy((void *)&pLrFramePacket->frames, m_lightringFrames.data (), sizeof(lightring_frame_rgb) * m_lightringFrames.size ());
			nPacketBufferSize = sizeof(lr_frame_rgb_packet) + sizeof(lightring_frame_rgb) * (m_lightringFrames.size () - 1);
		}

		nReturn = RcuPacketSend (PacketBuffer, nPacketBufferSize);

		stiTESTCONDMSG ((nReturn == 0), stiRESULT_ERROR,  "CstiLightRingBase::LightRingFramesSend: WARNING failed to start light ring\n");
	}

STI_BAIL:

	return hResult;

}

void CstiLightRingBase::notificationsStart (
	int duration,
	int brightness)
{
	PostEvent (std::bind (&CstiLightRingBase::eventNotificationsStart, this, duration, brightness));
}


/*!
 * \brief  Start light ring notifications for SignMail and Missed Calls. 
 *  
 * \note  If no pattern is set, this function does nothing 
 * 
 * \param int duration in seconds 
 */
void CstiLightRingBase::eventNotificationsStart (int duration, int brightness)
{
	lightring_frame_rgb lightringFrame;
	std::vector<lightring_frame_rgb> lightringFrameVector;
	int nReturn = 0;
	float brightnessModifier = 1.0;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Flasher should not be on when light ring notifications are used
	m_includeFlasher = false;

	// Adjust brightness based on selection
	if (brightness == 4) {
		// AUTO
		brightnessModifier = 1.0;
	} else if (brightness == 3) {
		// HIGH
		brightnessModifier = 1.0;
	} else if (brightness == 2) {
		// MEDIUM
		brightnessModifier = 0.33;
	} else if (brightness == 1) {
		// LOW
		brightnessModifier = 0.10;
	} else {
		brightnessModifier = 1.0;
	}

	// iterate through all frames and do brightness/RGB for each frame
	for (auto &frame : m_lightringNotificationFrames)
	{
		// Set frame RGB values and send packet
		char PacketBuffer[MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE] = {0};
		lr_frame_rgb_packet *pLrFramePacket;
		int nPacketBufferSize = 0;

		// Set frame brightness
		lightring_set_brightness_scale(std::floor(frame.intensity * brightnessModifier));

		// Prepare frame format
		lightringFrame.frame_delay = frame.duration; 
		for(int i = 0; i < NUM_LR_LEDS; i++)
		{
			lightringFrame.lr_vals[i].r = frame.lr_vals[i].r;
			lightringFrame.lr_vals[i].g = frame.lr_vals[i].g;
			lightringFrame.lr_vals[i].b = frame.lr_vals[i].b;
		}

		lightringFrameVector.push_back(lightringFrame);

		pLrFramePacket = (lr_frame_rgb_packet *)PacketBuffer;
		pLrFramePacket->type = PACKET_TYPE_SET_LRANIMATION_RGB;
		pLrFramePacket->numframes = lightringFrameVector.size ();	
		memcpy((void *)&pLrFramePacket->frames, lightringFrameVector.data (), sizeof(lightring_frame_rgb) * lightringFrameVector.size ());
		nPacketBufferSize = sizeof(lr_frame_rgb_packet) + sizeof(lightring_frame_rgb) * (lightringFrameVector.size () - 1);

		nReturn = RcuPacketSend (PacketBuffer, nPacketBufferSize);

		lightringFrameVector.clear();

		if (nReturn != 0)
		{
			stiASSERTMSG (estiFALSE, "RcuPacketSend failed to send a valid packet\n");
		}

		// Sleep for duration, primarily for a pulsed notification
		std::this_thread::sleep_for(std::chrono::milliseconds(frame.duration));
	}

	// duration = 0, ring pattern will sustain until shutdown
	// duration = anything else, ring pattern will shutdown based on duration value (seconds)
	if (duration)
	{
		m_lightRingNotificationsTimer.timeoutSet (duration * 1000);
		m_lightRingNotificationsTimer.start ();
	}
}

void CstiLightRingBase::notificationsStop ()
{
	PostEvent (std::bind(&CstiLightRingBase::eventNotificationsStop, this));
}


/*!
 * \brief Stops light ring notifications
 *  
 */
void CstiLightRingBase::eventNotificationsStop ()
{
	lr_packet LightringPacket;
	int nReturn = 0;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::Stop: enter eventStopNotifications ()\n");
	);
	
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_lightRingNotificationsTimer.stop ();

	// If the LightRing is playing a non-notification, don't actually stop it.
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);
	stiTESTCOND_NOLOG(!m_bPlaying, stiRESULT_ERROR);
	
	memset (&LightringPacket, 0, sizeof(lr_packet));
	
	LightringPacket.type = PACKET_TYPE_SET_LRANIMATION_OFF;
	
	nReturn = RcuPacketSend ((char *)&LightringPacket, sizeof(lr_packet));

	if (nReturn != 0)
	{
		stiASSERTMSG (estiFALSE, "CstiLightRingBase::Stop: WARNING failed to stop lightring\n");
	}

STI_BAIL:
	notificationStoppedSignal.Emit();

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::Stop: exit eventStopNotifications ()\n");
	);
}

stiHResult CstiLightRingBase::FlasherStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn = 0;
	
	stiDEBUG_TOOL(g_stiLightRingDebug > 1,
		stiTrace("CstiLightRingBase::FlasherStart: enter\n");
	);
	
	flasher_seq_packet FlasherPacket;
	FlasherPacket.type = PACKET_TYPE_SET_FLASH_SEQ;
	FlasherPacket.data.intensity = m_flasherBrightness;
	FlasherPacket.data.length_on = FLASHER_DURATION;
	FlasherPacket.data.length_off = FLASHER_GAP;
	FlasherPacket.data.flash_count = 1;
	FlasherPacket.data.bank_mask = eFLASHER_ALL;
	
	nReturn = RcuPacketSend ((char *)&FlasherPacket, sizeof(flasher_seq_packet));
	
	if (nReturn != 0)
	{
		stiASSERTMSG (estiFALSE, "CstiLightRingBase::FlasherStart: WARNING failed to set PACKET_TYPE_SET_FLASH\n");
	}
	
	stiDEBUG_TOOL(g_stiLightRingDebug > 1,
		stiTrace("CstiLightRingBase::FlasherStart: exit\n");
	);
	
	return (hResult);
}


stiHResult CstiLightRingBase::FlasherStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn = 0;
	
	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRingBase::FlasherStop: enter\n");
	);

	m_flasherTimer.stop ();

	flasher_seq_packet FlasherSequencePacket;
	FlasherSequencePacket.type = PACKET_TYPE_SET_FLASH_SEQ;
	FlasherSequencePacket.data.intensity = 0;
	FlasherSequencePacket.data.length_on = 0;
	FlasherSequencePacket.data.length_off = 0;
	FlasherSequencePacket.data.flash_count = 0;
	FlasherSequencePacket.data.bank_mask = eFLASHER_OFF;
	
	nReturn = RcuPacketSend ((char *)&FlasherSequencePacket, sizeof(flasher_seq_packet));
	
	stiTESTCONDMSG (nReturn == 0, stiRESULT_ERROR, "CstiLightRingBase::FlasherStop: WARNING failed to stop flasher\n");
	
STI_BAIL:

	return (hResult);
} 


/*!
* \brief Start up the LightRing task
* \retval None
*/
stiHResult CstiLightRingBase::Startup ()
{
	StartEventLoop ();

	return stiRESULT_SUCCESS;
}


stiHResult CstiLightRingBase::Shutdown ()
{
	m_lightRingTimer.stop ();
	m_flasherTimer.stop ();

	StopEventLoop ();

	return stiRESULT_SUCCESS;
}


void CstiLightRingBase::eventTimer ()
{
	Stop ();
}


/*!
* \brief Turn off the light ring after timer expires
* \param poEvent - the event to handle
* \retval 
*/
void CstiLightRingBase::eventFlasherTimer ()
{
	auto hResult = FlasherStart ();
	stiTESTRESULT ();
	
	if (m_bPlaying)
	{
		// failsafe - don't start the timer again if we're not playing
		m_flasherTimer.start ();
	}
	
STI_BAIL:

	return;
}

/*!
* \brief Event timer to stop light ring notifications
*
* \retval hResult
*/
void CstiLightRingBase::eventLightRingNotificationsTimer ()
{
	notificationsStop ();
}

std::vector<delayRGBIntensity> CstiLightRingBase::parseTestVectors(std::vector<std::string> strings)
{
	std::vector<delayRGBIntensity> values;

	// Identify duration/intensity/RGB
	for (auto &string : strings)
	{
		delayRGBIntensity value;

		value.duration = std::stoi(string.substr(2, 2), 0, 16);
		value.intensity = static_cast<uint8_t>(std::stoi(string.substr(7, 2), 0, 16));
		for(int i = 0; i < NUM_LR_LEDS; i++)
		{
			value.lr_vals[i].b = static_cast<uint8_t>(std::stoi(string.substr((i * 9) + 16, 2), 0, 16));
			value.lr_vals[i].g = static_cast<uint8_t>(std::stoi(string.substr((i * 9) + 14, 2), 0, 16));
			value.lr_vals[i].r = static_cast<uint8_t>(std::stoi(string.substr((i * 9) + 12, 2), 0, 16));
		}
		values.push_back(value);
	}

	return values;
}

ISignal<>& CstiLightRingBase::lightRingStoppedSignalGet ()
{
	return lightRingStoppedSignal;
}

ISignal<>& CstiLightRingBase::notificationStoppedSignalGet ()
{
	return notificationStoppedSignal;
}

