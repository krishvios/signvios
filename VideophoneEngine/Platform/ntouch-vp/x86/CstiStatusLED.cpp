///
/// \file CstiStatusLED.cpp
/// \brief Definition of the Status LED class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#include <cstdio>
#include "CstiStatusLED.h"


void CstiStatusLED::Uninitialize ()
{
}


stiHResult CstiStatusLED::Start (
	EstiLed eLED,
	unsigned unBlinkRate)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiStatusLED::Stop (
	EstiLed eLED)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiStatusLED::Enable (
	EstiLed eLED,
	EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiStatusLED::PulseNotificationsEnable(bool enable)
{
	auto hResult = stiRESULT_SUCCESS;

	return hResult;
}
