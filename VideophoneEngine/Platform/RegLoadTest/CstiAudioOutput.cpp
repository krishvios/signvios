// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioOutput.h"
#include "CstiSignal.h"

/*!
 * \brief default constructor
 */
CstiAudioOutput::CstiAudioOutput ()
{
}

CstiAudioOutput::~CstiAudioOutput ()
{
}
	
/*!
 * \brief Initialize 
 * Usually called once for object 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}

/*!
 * \brief Start audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

/*!
 * \brief Stops audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

/*!
 * \brief Sets codec for audio playback
 * 
 * \param EstiAudioCodec eVideoCodec 
 * One of the below 
 *	estiAUDIO_NONE
 *	estiAUDIO_G711_ALAW	  G.711 ALAW audio
 *	estiAUDIO_G711_MULAW  G.711 MULAW audio
 *	esmdAUDIO_G723_5	  G.723.5 compressed audio
 *	esmdAUDIO_G723_6	  G.723.6 compressed audio
 *  estiAUDIO_RAW		  Raw Audio (8000 hz, signed 16 bits, mono)
 *  
 *  \see EstiAudioCodec
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackCodecSet (
		EstiAudioCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

/*!
 * \brief Audio playback packet put 
 * 
 * \param SsmdAudioFrame* pAudioFrame 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


/*!
* \brief notifies us when a DTMF event has been received
* 
* \param eDtmfDigit 
* 
* \return nothing
*/
void CstiAudioOutput::DtmfReceived (EstiDTMFDigit eDtmfDigit) 
{
	dtmfReceivedSignalGet ().Emit (eDtmfDigit);
}


/*!
 * \brief Gets the device file descriptor
 * 
 * \return int 
 */
int CstiAudioOutput::GetDeviceFD ()
{
	return 0;
}




