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

stiHResult CstiLightRing::Initialize(std::shared_ptr<CstiHelios> helios, const std::string &serialNumber)
{
	m_serialNumber = serialNumber;
	return CstiLightRingBase::Initialize(helios);
}


void CstiLightRing::lightRingTurnOn (uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity)
{
	lightring_frame_rgb frame {}; // frame_delay set to 0
	for (auto &led : frame.lr_vals)
	{
		led.r = red;
		led.g = green;
		led.b = blue;
	}

	lightring_set_brightness_scale (intensity);
	ringPatternStart (1, &frame);
}


void CstiLightRing::lightRingTurnOff ()
{
	ringPatternStop ();
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


void CstiLightRing::alertLedsTurnOn (uint16_t intensity)
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


void CstiLightRing::alertLedsTestPattern (int intensity)
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

void CstiLightRing::PatternSet (
	Pattern patternId,
	EstiLedColor color,
	int brightness,
	int flasherBrightness)
{
/*
 * TODO: depending on the serial number of the system we have different 
 * brightness transform functions. This needs to be revisited for HW revision 'C' 
 * of Lumina.
 * 
 * Serial Number (SN) Structure:
 * =============================
 * 
 *  SN Value:      Position:       Description:
 *  =========      =========       ============
 * 
 *  J ------------ 0 ------------- Month of production
 *  2 ------------ 1 ------------- Year of production
 *  0 ------------ 2 ------------- Year of production
 *  2 ------------ 3 ------------- Manufacturer code
 *  2 ------------ 4 ------------- Factory code
 *  D ------------ 5 ------------- Product and model
 *  A ------------ 6 ------------- Model revision (e.g. 'D' is Lumina)
 *  0 ------------ 7 ------------- Unique 7 digit serial number
 *  0 ------------ 8 ------------- Unique 7 digit serial number
 *  0 ------------ 9 ------------- Unique 7 digit serial number
 *  0 ------------ 10 ------------ Unique 7 digit serial number
 *  9 ------------ 11 ------------ Unique 7 digit serial number
 *  4 ------------ 12 ------------ Unique 7 digit serial number
 *  2 ------------ 13 ------------ Unique 7 digit serial number
 */
	int vp3Brightness = brightness & 0xff;
	
	vp3Brightness = brightnessTransform(vp3Brightness);

	CstiLightRingBase::PatternSet(patternId, color, vp3Brightness, flasherBrightness);
}

#include <math.h>
int CstiLightRing::brightnessTransform(int val)
{
	int ret;

	double x = val;
	double y = 4.05322914136626;
	double b = .00000004916689360024;

	ret = pow(x,y) * b;

	return std::min(255, ret);
}

