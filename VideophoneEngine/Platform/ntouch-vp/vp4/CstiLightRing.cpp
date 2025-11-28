///
/// \file CstiLightRing.cpp
/// \brief Definition of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved
///
//
// Includes
//

#include "CstiLightRing.h"
#include "CstiLightRingPatterns.h"
#include <istream>
#include <iterator>
#include <math.h>

std::map<IstiLightRing::EstiLedColor, lr_led_rgb> colorMap =
{
	{IstiLightRing::estiLED_COLOR_TEAL, 		{0x00, 0xe6, 0x33}},
	{IstiLightRing::estiLED_COLOR_LIGHT_BLUE, 	{0x30, 0xff, 0xee}},
	{IstiLightRing::estiLED_COLOR_BLUE, 		{0x00, 0x00, 0xff}},
	{IstiLightRing::estiLED_COLOR_VIOLET, 		{0x64, 0x00, 0xaf}},
	{IstiLightRing::estiLED_COLOR_MAGENTA, 		{0xff, 0x00, 0x64}},
	{IstiLightRing::estiLED_COLOR_ORANGE, 		{0xc8, 0x28, 0x00}},
	{IstiLightRing::estiLED_COLOR_YELLOW, 		{0xff, 0xc8, 0x00}},
	{IstiLightRing::estiLED_COLOR_RED, 			{0xff, 0x00, 0x00}},
	{IstiLightRing::estiLED_COLOR_PINK, 		{0xff, 0x23, 0x3c}},
	{IstiLightRing::estiLED_COLOR_GREEN, 		{0x00, 0xff, 0x00}},
	{IstiLightRing::estiLED_COLOR_LIGHT_GREEN, 	{0x00, 0xff, 0x32}},
	{IstiLightRing::estiLED_COLOR_CYAN, 		{0x00, 0xf0, 0xff}},
	{IstiLightRing::estiLED_COLOR_WHITE, 		{0xff, 0xff, 0xff}}
};

CstiLightRing::CstiLightRing ()
	:
	CstiLightRingBase (colorMap)
{
}

stiHResult CstiLightRing::Initialize (
	std::shared_ptr<CstiHelios> helios,
	const std::string &serialNumber)
{
	m_serialNumber = serialNumber;
	return CstiLightRingBase::Initialize (helios);
}


void CstiLightRing::lightRingTurnOn (
	uint8_t red, uint8_t green, uint8_t blue,
	uint8_t intensity)
{
	lightring_frame_rgb frame {}; // frame_delay set to 0

	for (auto &led : frame.lr_vals)
	{
		led.r = red;
		led.g = green;
		led.b = blue;
	}

	lightring_set_brightness_scale (intensity);
	lightringPatternStart (1, &frame);
}


void CstiLightRing::lightRingTurnOff ()
{
	lightringPatternStop ();
}


void CstiLightRing::lightRingTestPattern (bool on)
{
	if (on)
	{
		const int brightness {255};

		PostEvent ([this]{
			eventPatternSet (IstiLightRing::Pattern::Test,
							 IstiLightRing::EstiLedColor::estiLED_USE_PATTERN_COLORS,
							 brightness, brightness); // brightness for lightring, flasher
			eventStart (0 , false);
			});
	}
	else
	{
		PostEvent ([this]{eventStop ();});
	}
}



void CstiLightRing::alertLedsTurnOn (
	uint16_t intensity)
{
	packet_data_flasher_seq sequence {}; // length_off set to 0
	sequence.intensity = intensity;
	sequence.length_on = 100;

	alertPatternStart (&sequence);
}


void CstiLightRing::alertLedsTurnOff ()
{
	alertPatternStop ();
}


void CstiLightRing::alertLedsTestPattern (
	int intensity)
{
	if (intensity > 0)
	{
		packet_data_flasher_seq sequence {}; // length_off set to 0
		sequence.intensity = intensity;
		sequence.length_on = 1000;
		sequence.length_off = 1000;

		alertPatternStart (&sequence);
	}
	else
	{
		alertPatternStop ();
	}
	
}

lr_led_rgb CstiLightRing::uint32ToRgbValue (
	CstiLightRing::EstiLedColor color,
	uint8_t lightRingBrightness,
	uint32_t uint32Value)
{
	lr_led_rgb rgbValue;
	uint8_t brightness;

	// Get the per LED brightness
	brightness = (uint8_t)(uint32Value & 0x000000FF);

	// Get the LED rgb values
	rgbValue.r = (uint8_t)((uint32Value & 0xFF000000) >> 24);
	rgbValue.g = (uint8_t)((uint32Value & 0x00FF0000) >> 16);
	rgbValue.b = (uint8_t)((uint32Value & 0x0000FF00) >> 8);

	// Change the color if specified
	if (color != CstiLightRing::estiLED_USE_PATTERN_COLORS)
	{
		// If there is any value then set the rgbValue to color
		if (rgbValue.r || rgbValue.g || rgbValue.b)
		{
			rgbValue = colorGet (m_colorMap, color);
		}
	}

	// Change brightness for this LED
	rgbValue.r = (rgbValue.r * brightness) / 255;
	rgbValue.g = (rgbValue.g * brightness) / 255;
	rgbValue.b = (rgbValue.b * brightness) / 255;

	// Change the overall lightring brightness
	rgbValue.r = (rgbValue.r * lightRingBrightness) / 255;
	rgbValue.g = (rgbValue.g * lightRingBrightness) / 255;
	rgbValue.b = (rgbValue.b * lightRingBrightness) / 255;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("uint32ToRgbValue: uint32Value = %08x\n", uint32Value);
		stiTrace("uint32ToRgbValue: color = %d\n", (int)color);
		stiTrace("uint32ToRgbValue: lightRingBrightness = %d\n", lightRingBrightness);
		stiTrace("uint32ToRgbValue: brightness = %08x\n", brightness);
	);

	return rgbValue;
}



stiHResult CstiLightRing::customPatternStringVectorToFrameRGBVector (
	std::vector<std::string> patternStringVector,
	CstiLightRing::EstiLedColor color,
	uint8_t lightRingBrightness,
	std::vector<lightring_frame_rgb> &frameRGBVector)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int frameCount = 0;

	stiTESTCONDMSG ((patternStringVector.size () <= MAX_ANIMATION_FRAME), stiRESULT_ERROR,  "CstiLightRing::customPatternStringVectorToFrameRGBVector: Error: To many frames. patternStringVector.size () = %d\n", patternStringVector.size ());

	frameCount = std::min((int)patternStringVector.size (), MAX_ANIMATION_FRAME);

	frameRGBVector.resize (frameCount);

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: frameCount = %d\n", frameCount);
	);

	for (int i = 0; i < frameCount; i++)
	{
		std::istringstream oneFrameStringStream (patternStringVector[i]);

		std::vector<std::string> oneFrameStringVector (std::istream_iterator<std::string>(oneFrameStringStream), std::istream_iterator<std::string>{});

		// Verify size
		stiTESTCONDMSG ((oneFrameStringVector.size () == NUM_LR_LEDS + 1), stiRESULT_ERROR,  "CstiLightRing::customPatternStringVectorToFrameRGBVector: ERROR Wrong frame vector size\n");

		stiDEBUG_TOOL(g_stiLightRingDebug == 2,
			stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: patternString = %s\n", patternStringVector[i].c_str ());
			stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: onePatternStringVector.size () = %d\n", oneFrameStringVector.size ());
		);

		frameRGBVector[i].frame_delay = std::stoull (oneFrameStringVector[0], NULL, 0);

		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: frameRGBVector[i].frame_delay = %d\n", frameRGBVector[i].frame_delay);
		);

		// LED numbers are reversed compared to Lumina1 and helios
		std::reverse (oneFrameStringVector.begin(), oneFrameStringVector.end());

		for (int j = 0; j < NUM_LR_LEDS; j++)
		{
			frameRGBVector[i].lr_vals[j] = uint32ToRgbValue (color, lightRingBrightness, std::stoull (oneFrameStringVector[j], NULL, 0));

			stiDEBUG_TOOL((g_stiLightRingDebug == 2) && (j < 3),
				stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: frameRGBVector[%d].lr_vals[%d].r = %x\n", i, j, frameRGBVector[i].lr_vals[j].r);
				stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: frameRGBVector[%d].lr_vals[%d].g = %x\n", i, j, frameRGBVector[i].lr_vals[j].g);
				stiTrace("CstiLightRing::customPatternStringVectorToFrameRGBVector: frameRGBVector[%d].lr_vals[%d].b = %x\n", i, j, frameRGBVector[i].lr_vals[j].b);
			);
		}
	}

STI_BAIL:

	return hResult;
}

void CstiLightRing::eventPatternSet (
	Pattern patternId,
	EstiLedColor color,
	int lightRingBrightness,
	int flasherBrightness)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRing::eventPatternSet\n");
	);

	auto iter = RingPatternMap.find (patternId);

	if (iter == RingPatternMap.end ())
	{
		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::eventPatternSet: could not find patternId = %d\n", patternId);
		);
		return;
	}

	const std::vector<std::string> &pattern = *(*iter).second;

	hResult = customPatternStringVectorToFrameRGBVector (pattern, color, lightRingBrightness, m_lightringFrames);
	stiASSERTRESULTMSG (hResult, "CstiLightRingBase::eventPatternSet: WARNING Failed to convert pattern\n");

	m_brightness = lightRingBrightness & 0xff;
	m_flasherBrightness = std::max(std::min(flasherBrightness, FLASHER_MAX), FLASHER_MIN);
	m_color = color;

	// Setup helios paramters
	m_patternId = patternId;

}

stiHResult CstiLightRing::HeliosFramesSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRing::HeliosFramesSend\n");
	);

	if (m_helios)
	{
		lr_led_rgb lrLedRgb = colorGet (m_helios->ColorMapGet (), m_color);
		uint8_t brightness = m_helios->BrightnessMap (m_brightness);

		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::HeliosFramesSend: m_patternId = %d\n", (int)m_patternId);
		);

		hResult = m_helios->PresetPatternStart ((int)m_patternId, lrLedRgb.r, lrLedRgb.g, lrLedRgb.b, brightness);
		stiTESTRESULTMSG ("CstiLightRingBase::HeliosFramesSend: WARNING Failed to send frames to helios\n");

		// This is for testing custom frames to helios. We have not had need to do this yet.
		// Currently there is a bug in helios code that does not set the brightness for custom patterns.
		//hResult = m_helios->CustomPatternStart (HeliosFlashingTestPatternVector);
		//stiTESTRESULTMSG ("CstiLightRingBase::HeliosFramesSend: WARNING Failed to send frames to helios\n");
	}

STI_BAIL:
	return hResult;
}


stiHResult CstiLightRing::LightRingFramesSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn = 0;

	stiDEBUG_TOOL(g_stiLightRingDebug,
		stiTrace("CstiLightRing::LightRingFramesSend: m_lightringFrames.size () = %d\n", m_lightringFrames.size ());
	);

	if (m_lightringFrames.size () > 0)
	{
		char PacketBuffer[MAX_PACKET_TYPE_LRANIMATION_RGB_BUFFERSIZE] = {0};
		int nPacketBufferSize = 0;

		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::LightRingFramesSend: m_brightness = %d\n", m_brightness);
		);

		lightring_set_brightness_scale (m_brightness);

		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::LightRingFramesSend: m_lightringFrames.size () = %d\n", m_lightringFrames.size ());
		);

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

		stiTESTCONDMSG ((nReturn == 0), stiRESULT_ERROR,  "CstiLightRing::LightRingFramesSend: WARNING failed to start light ring\n");
	}

STI_BAIL:
	return hResult;

}


