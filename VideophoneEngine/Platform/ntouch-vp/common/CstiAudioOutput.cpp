// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioOutput.h"
#include "stiTools.h"

#include <gst/app/gstappsrc.h>

/*!
 * \brief default constructor
 */
CstiAudioOutput::CstiAudioOutput ()
{
	m_pAudioHandler = nullptr;
}

/*!
 * \brief Initialize 
 * Usually called once for object 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::Initialize (
	CstiAudioHandler *pAudioHandler,
	EstiAudioCodec eAudioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!pAudioHandler)
	{
		stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "CstiAudioOutput::Initialize - NULL pAudioHandler\n");
	}
	
	m_pAudioHandler = pAudioHandler;
	
	if (eAudioCodec != estiAUDIO_NONE)
	{
		hResult = m_pAudioHandler->CodecSet (eAudioCodec);
		stiTESTRESULT();
	}

	// Propogate the signal from AudioHandler up to AudioPlayback
	// indicating whether or not we are ready to receive more data
	if (!m_audioHandlerReadyStateChangedSignalConnection)
	{
		 m_audioHandlerReadyStateChangedSignalConnection = m_pAudioHandler->readyStateChangedSignal.Connect (
			[this](bool state){
				readyStateChangedSignal.Emit (state);
			});
	}
	
STI_BAIL:

	return hResult;
}


CstiAudioOutput::~CstiAudioOutput ()
{
	m_pAudioHandler = nullptr;
}

/*!
 * \brief Sends the event that will start audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackStart ()
{
	return (m_pAudioHandler->DecodeStart ());
}

/*!
 * \brief Sends the event that will stop audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackStop ()
{
	return (m_pAudioHandler->DecodeStop ());
}

/*!
 * \brief Sends audio out
 *
 * \param EstiSwitch eSwitch
 *
 * \return stiHResult
 */
stiHResult CstiAudioOutput::AudioOut (
	EstiSwitch eSwitch)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (estiON == eSwitch)
	{
		hResult = m_pAudioHandler->OutputEnable (true);
		stiTESTRESULT();
	}
	else
	{
		hResult = m_pAudioHandler->OutputEnable (false);
		stiTESTRESULT();
	}
	
STI_BAIL:

	return (hResult);
}

#ifdef stiSIGNMAIL_AUDIO_ENABLED
stiHResult CstiAudioOutput::AudioPlaybackSettingsSet(
	SstiAudioDecodeSettings *pAudioDecodeSettings)
{
	return (m_pAudioHandler->DecodeSettingsSet(pAudioDecodeSettings));
}
#endif

stiHResult CstiAudioOutput::AudioPlaybackCodecSet (
	EstiAudioCodec eAudioCodec)
{
	return (m_pAudioHandler->CodecSet (eAudioCodec));
}


/*!
 * \brief Audio playback packet put 
 * 
 * \param SsmdAudioFrame* pAudioFrame 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioOutput::AudioPlaybackPacketPut (
	SsmdAudioFrame *pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = m_pAudioHandler->PacketPut (pAudioFrame);
	stiTESTRESULT();
	
STI_BAIL:

	return (hResult);
}

/*!
* \brief notifies us when a DTMF event has been received
* 
* \param eDtmfDigit 
* 
* \return nothing
*/
void CstiAudioOutput::DtmfReceived (
	EstiDTMFDigit eDtmfDigit) 
{
	dtmfReceivedSignal.Emit (eDtmfDigit);
}


/*!
 * \brief Gets the device file descriptor
 * 
 * \return int 
 */
int CstiAudioOutput::GetDeviceFD ()
{
	return (m_pAudioHandler->GetDeviceFD ());
}

///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiAudioOutput::CodecsGet (
	std::list<EstiAudioCodec> *pCodecs)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Add the supported audio codecs to the list
#ifndef stiINTEROP_EVENT
	pCodecs->push_front (estiAUDIO_G722);
	pCodecs->push_back (estiAUDIO_G711_MULAW);
	pCodecs->push_back (estiAUDIO_G711_ALAW);
//	pCodecs->push_back (estiAUDIO_RAW);
#else
	if (g_stiEnableAudioCodecs & 4)
	{
		pCodecs->push_front (estiAUDIO_G722);
	}

	if (g_stiEnableAudioCodecs & 2)
	{
		pCodecs->push_back (estiAUDIO_G711_MULAW);
	}

	if (g_stiEnableAudioCodecs & 1)
	{
		pCodecs->push_back (estiAUDIO_G711_ALAW);
	}
#endif

	return (hResult);
}

