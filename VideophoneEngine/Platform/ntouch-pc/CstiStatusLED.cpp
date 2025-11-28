///
/// \file CstiStatusLED.cpp
/// \brief Definition of the Status LED class
///
///
/// Copyright (C) 2010 Sorenson Communications, Inc. All rights reserved. **
///


#include <stdio.h>
#include "CstiStatusLED.h"

CstiStatusLED::CstiStatusLED ()
{
}


CstiStatusLED::~CstiStatusLED ()
{
}


stiHResult CstiStatusLED::Start (
	EstiLed eLED,
	unsigned unBlinkRate)
{
	printf("CstiStatusLED::Start -- LED: %d, Blink Rate: %d\n", eLED, unBlinkRate);

	return stiRESULT_SUCCESS;
}


stiHResult CstiStatusLED::Stop (
	EstiLed eLED)
{
	printf("CstiStatusLED::Stop -- LED: %d\n", eLED);
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiStatusLED::Enable (
	EstiLed eLED,
	EstiBool bEnable)
{
	printf("CstiStatusLED::Enable -- LED: %d on: %d\n", eLED, bEnable ? 1 : 0);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


