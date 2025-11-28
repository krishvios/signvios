///
/// \file CstiLightRing.cpp
/// \brief Definition of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved
///


#include "stiTrace.h"
#include "CstiLightRing.h"


CstiLightRing::CstiLightRing ()
{
}


CstiLightRing::~CstiLightRing ()
{
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
	int flasherBrightness
	)
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



