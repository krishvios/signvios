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

stiHResult CstiAudibleRinger::Initialize (
	CstiAudioHandler *pAudioHandler)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!pAudioHandler)
	{
		stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "CstiAudibleRinger::Initialize - NULL pAudioHandler\n");
	}
	
	m_pAudioHandler = pAudioHandler;

STI_BAIL:

	return (hResult);
}

///
/// \brief Starts (turns on) the Audible Ringer with the pattern described by PatternSet.
/// \note This function has no effect if no pattern has been set.
///
stiHResult CstiAudibleRinger::Start ()
{
	return (m_pAudioHandler->RingerStart());
}

///
/// \brief Starts (turns on) the Audible Ringer with the pattern described by PatternSet.
/// \note This function has no effect if no pattern has been set.
///
stiHResult CstiAudibleRinger::Stop ()
{
	return (m_pAudioHandler->RingerStop());
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
	
	return (hResult);
}

stiHResult CstiAudibleRinger::VolumeSet (
	int nVolume)
{
	return (m_pAudioHandler->RingerVolumeSet (nVolume));
}

int CstiAudibleRinger::VolumeGet ()
{
	return (m_pAudioHandler->RingerVolumeGet ());
}

stiHResult CstiAudibleRinger::PitchSet (
	int nPitch)
{
	return (m_pAudioHandler->RingerPitchSet ((EstiTone)nPitch));
}

IstiAudibleRinger::EstiTone CstiAudibleRinger::PitchGet ()
{
	return (m_pAudioHandler->RingerPitchGet ());
}
