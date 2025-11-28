///
/// \file CstiAudibleRinger.cpp
/// \brief Definition of the AudibleRinger class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#include "stiTrace.h"
#include "CstiAudibleRinger.h"


CstiAudibleRinger::CstiAudibleRinger ()
	:
	m_pstiTone(NULL)
{
}


CstiAudibleRinger::~CstiAudibleRinger ()
{
}

///
/// \brief Starts (turns on) the Audible Ringer with the pattern described by PatternSet.
/// \note This function has no effect if no pattern has been set.
///
stiHResult CstiAudibleRinger::Start ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiAudibleRingerDebug,
			stiTrace("CstiAudibleRinger::Start\n");
	);

	return hResult;
}


///
/// \brief Stops (turns off) the audible ringer.
///
stiHResult CstiAudibleRinger::Stop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiAudibleRingerDebug,
			stiTrace("CstiAudibleRinger::Stop\n");
	);
	
	return hResult;
}



///
/// \brief Sets the tones.
///
stiHResult CstiAudibleRinger::TonesSet (
	EstiTone eTone,
	const SstiTone *pstiTone,		/// Pointer to the array of tones structures
	unsigned int unCount,			/// Number of entries in the array of tones
	unsigned int unRepeatCount)		/// Number of times the array of tones should be played before stopping
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiAudibleRingerDebug,
		stiTrace("CstiAudibleRinger::TonesSet\n");
	);

	return hResult;
}

