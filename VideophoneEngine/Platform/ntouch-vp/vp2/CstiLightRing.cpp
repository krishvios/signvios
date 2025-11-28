///
/// \file CstiLightRing.cpp
/// \brief Definition of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///
//
// Includes
//

#include "CstiLightRing.h"
#include "CstiMonitorTask.h"
#include "stiTrace.h"

std::map<IstiLightRing::EstiLedColor, lr_led_rgb> colorMap =
{
	{IstiLightRing::estiLED_COLOR_TEAL, 		{0x00, 0xff, 0x40}},
	{IstiLightRing::estiLED_COLOR_LIGHT_BLUE, 	{0x30, 0xff, 0xee}},
	{IstiLightRing::estiLED_COLOR_BLUE, 		{0x00, 0x00, 0xff}},
	{IstiLightRing::estiLED_COLOR_VIOLET, 		{0x80, 0x00, 0xc0}},
	{IstiLightRing::estiLED_COLOR_MAGENTA, 		{0xcc, 0x00, 0x40}},
	{IstiLightRing::estiLED_COLOR_ORANGE, 		{0xff, 0x40, 0x00}},
	{IstiLightRing::estiLED_COLOR_YELLOW, 		{0xff, 0xff, 0x00}},
	{IstiLightRing::estiLED_COLOR_RED, 			{0xff, 0x00, 0x00}},
	{IstiLightRing::estiLED_COLOR_PINK, 		{0xff, 0x00, 0x78}},
	{IstiLightRing::estiLED_COLOR_GREEN, 		{0x00, 0xff, 0x00}},
	{IstiLightRing::estiLED_COLOR_LIGHT_GREEN, 	{0x00, 0xff, 0xa0}},
	{IstiLightRing::estiLED_COLOR_CYAN, 		{0x00, 0xf0, 0xff}},
	{IstiLightRing::estiLED_COLOR_WHITE, 		{0xff, 0xff, 0xff}}
};


CstiLightRing::CstiLightRing ()
	:
	CstiLightRingBase (colorMap)
{
}

/*!
 * \brief Initialization happens once and is 
 * controlled by the  
 * 
 * \return stiHResult stiRESULT_SUCCESS
 * \return stiHResult stiRESULT_TASK_INIT_FAILED
 */
stiHResult CstiLightRing::Initialize (
	CstiMonitorTask *monitorTask,
	std::shared_ptr<CstiHelios> helios)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (nullptr != monitorTask, stiRESULT_ERROR);
	hResult = CstiLightRingBase::Initialize(helios);
	stiTESTRESULT();
	
	m_monitorTask = monitorTask;

	m_signalConnections.push_back (m_monitorTask->mfddrvdStatusChangedSignal.Connect (
		[this]()
		{
			PostEvent(
				std::bind(&CstiLightRing::eventMfddrvdStatusChanged, this));
		}));
	m_monitorTask->MfddrvdRunningGet (&m_sendFrames);

STI_BAIL:
	return hResult;
}


void CstiLightRing::lightRingTurnOn (uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity)
{
	stiTrace ("lightRingTurnOn is not implemented on VP2\n");
}


void CstiLightRing::lightRingTurnOff ()
{
	stiTrace ("lightRingTurnOff is not implemented on VP2\n");
}


void CstiLightRing::lightRingTestPattern (bool on)
{
	stiTrace ("lightRingTestPattern is not implemented on VP2\n");
}


void CstiLightRing::alertLedsTurnOn (uint16_t intensity)
{
	stiTrace ("alertLedsTurnOn is not implemented on VP2\n");
}


void CstiLightRing::alertLedsTurnOff ()
{
	stiTrace ("alertLedsTurnOff is not implemented on VP2\n");
}

void CstiLightRing::alertLedsTestPattern (int intensity)
{
	stiTrace ("alertLedsTestPattern is not implemented on VP2\n");
}


void CstiLightRing::eventMfddrvdStatusChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_monitorTask->MfddrvdRunningGet (&m_sendFrames);
	stiTESTRESULT ();

	hResult = RcuHotplug ();
	stiTESTRESULT ();

STI_BAIL:

	return;
}


stiHResult CstiLightRing::RcuHotplug ()
{

	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_sendFrames)
	{
		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::RcuHotplug: MFDDRVD running\n");
		);

		if (m_bPlaying)
		{
			Start(0);
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiLightRingDebug,
			stiTrace("CstiLightRing::RcuHotplug: MFDDRVD stopped\n");
		);
		
		Stop ();
		
		FlasherStop ();
	}

//STI_BAIL:

	return (hResult);
}
