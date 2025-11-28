///
/// \file CstiLightRing.cpp
/// \brief Definition of the LightRing class
///
///
/// Copyright (C) 2010 Sorenson Communications, Inc. All rights reserved. **
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
	int brightness)
{
}
	

///
/// \brief Starts the Light Ring with set pattern.
/// \note If no pattern is set, this function does nothing
///
void CstiLightRing::Start (
	int nSeconds)
{
	stiTrace("CstiLightRing::Start\n");
}

///
/// \brief Stops the Light Ring
///
void CstiLightRing::Stop ()
{
	stiTrace("CstiLightRing::Stop\n");
}



