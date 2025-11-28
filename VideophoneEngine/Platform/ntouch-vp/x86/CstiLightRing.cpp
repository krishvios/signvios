///
/// \file CstiLightRing.cpp
/// \brief Definition of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#include "stiTrace.h"
#include "CstiLightRing.h"


stiHResult CstiLightRing::Initialize (
	CstiMonitorTask *monitorTask,
	std::shared_ptr<CstiHelios> &helios)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiLightRing::Startup ()
{
	return stiRESULT_SUCCESS;
}



///
/// \brief Sets the Light Ring pattern to use when started.
///
///  Replaces any previous pattern.  If a replaced pattern is currently displaying,
///   the new pattern is loaded and the LightRing continues to execute with the new pattern.
///
void CstiLightRing::PatternSet (
	Pattern patternId,
	EstiLedColor color,
	int brightness,
	int flasherBrightness)
{
}

///
/// \brief Sets the Light Ring pattern to use for notifications when started.
///
///  Replaces any previous pattern.  If a replaced pattern is currently displaying,
///   the new pattern is loaded and the LightRing continues to execute with the new pattern.
///
void CstiLightRing::notificationPatternSet (
	NotificationPattern pattern)
{
}

///
/// \brief Starts the Light Ring with set pattern.
/// \note If no pattern is set, this function does nothing
///
void CstiLightRing::Start (
	int nSeconds)
{
	stiDEBUG_TOOL (g_stiLightRingDebug,
		stiTrace("CstiLightRing::Start\n");
	);
}

///
/// \brief Stops the Light Ring
///
void CstiLightRing::Stop ()
{
	stiDEBUG_TOOL (g_stiLightRingDebug,
			stiTrace("CstiLightRing::Stop\n");
	);
}

///
/// \brief Starts the Light Ring Notifications
///
void CstiLightRing::notificationsStart (
	int nSeconds,
	int nBrightness)
{
	stiDEBUG_TOOL (g_stiLightRingDebug,
			stiTrace("CstiLightRing::Start Light Ring Notifications\n");
	);
}

///
/// \brief Stops he Light Ring Notifications
///
void CstiLightRing::notificationsStop ()
{
	stiDEBUG_TOOL (g_stiLightRingDebug,
			stiTrace("CstiLightRing::Stop Light Ring Notifications\n");
	);
}

ISignal<>& CstiLightRing::lightRingStoppedSignalGet ()
{
	return lightRingStoppedSignal;
}

ISignal<>& CstiLightRing::notificationStoppedSignalGet ()
{
	return notificationStoppedSignal;
}
