// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioInput.h"
#include "stiTrace.h"

#define MAX_SAMPLE_RATE 30		// in ms

#define MIC_INITIALIZATION_COMMAND \
	"amixer -c 0 cset name='Int Mic Switch' off > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Left Input PGA Common Mode Switch' 0 > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Left Capture Mux' 0 > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Right Capture Mux' 0 > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Left Input PGA Switch' on > /dev/null 2>&1;" \
	"amixer -c 0 cset name='ADC Input' 0 > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Mic Jack Switch' on > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Left Input PGA Volume' 21 > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Right Input PGA Switch' off > /dev/null 2>&1;" \
	"amixer -c 0 cset name='Right Input PGA Volume' 0 > /dev/null 2>&1"

/*!
 * \brief Default Constructor
 */
stiHResult CstiAudioInput::Initialize (
	CstiAudioHandler *audioHandler)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = CstiAudioInputBase::Initialize(audioHandler);
	stiTESTRESULT();

	if (system (MIC_INITIALIZATION_COMMAND))
	{
		stiASSERTMSG (false, "error: initializing mic");
	}

STI_BAIL:

	return hResult;
}
