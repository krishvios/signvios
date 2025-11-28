//
//  CstiAudioBridge.cpp
//  ntouchMac
//
//  Created by Dennis Muhlestein on 11/20/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#include "CstiAudioBridge.h"


#define MAX_AUDIO_FRAMES	6
#define MAX_AUDIO_BUFFER_SIZE	320


CstiAudioBridge::CstiAudioBridge()
{
	// TODO put in init and handle errors?
	stiOSSignalCreate (&m_fdRSignal);
	stiOSSignalClear(m_fdRSignal);
	stiOSSignalCreate (&m_fdPSignal);
	stiOSSignalSet(m_fdPSignal); // always ready to playback

	for (int i = 0; i < MAX_AUDIO_FRAMES; ++i)
	{
		auto pTmpFrame = new SsmdAudioFrame;
		pTmpFrame->pun8Buffer = new uint8_t[MAX_AUDIO_BUFFER_SIZE];
		pTmpFrame->unBufferMaxSize = MAX_AUDIO_BUFFER_SIZE;

		m_AvailableFrames.push (pTmpFrame);
	}
}


CstiAudioBridge::~CstiAudioBridge()
{
	while (!m_AvailableFrames.empty ())
	{
		SsmdAudioFrame *pTmp = m_AvailableFrames.front();
		m_AvailableFrames.pop();

		if (pTmp->pun8Buffer)
		{
			delete [] pTmp->pun8Buffer;
			pTmp->pun8Buffer = nullptr;
		}

		delete pTmp;
		pTmp = nullptr;
	}

	while (!m_playbackFrames.empty ())
	{
		SsmdAudioFrame *pTmp = m_playbackFrames.front();
		m_playbackFrames.pop();

		if (pTmp->pun8Buffer)
		{
			delete [] pTmp->pun8Buffer;
			pTmp->pun8Buffer = nullptr;
		}

		delete pTmp;
		pTmp = nullptr;
	}

	stiOSSignalClose(&m_fdRSignal);
	stiOSSignalClose(&m_fdPSignal);
}


// helper
stiHResult copy(SsmdAudioFrame &dst, SsmdAudioFrame &src)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	dst.bIsSIDFrame = src.bIsSIDFrame;
	dst.esmdAudioMode = src.esmdAudioMode;
	dst.un8NumFrames = src.un8NumFrames;
	dst.unFrameDuration=src.unFrameDuration;
	stiTESTCOND(dst.pun8Buffer, stiRESULT_ERROR);
	stiTESTCOND(dst.unBufferMaxSize>=src.unFrameSizeInBytes , stiRESULT_BUFFER_TOO_SMALL);
	dst.unFrameSizeInBytes=src.unFrameSizeInBytes;
	memcpy(dst.pun8Buffer,src.pun8Buffer,src.unFrameSizeInBytes);
	dst.un32RtpTimestamp = src.un32RtpTimestamp;
STI_BAIL:
	return hResult;
}

// audio record methods

stiHResult CstiAudioBridge::AudioRecordPacketGet (
	SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SsmdAudioFrame *pTmp = nullptr;
	
	std::lock_guard<std::recursive_mutex> thread_safe(m_Mutex);
	
	stiTESTCOND (!m_playbackFrames.empty(), stiRESULT_ERROR);

	pTmp = m_playbackFrames.front();
	m_playbackFrames.pop();
	
	hResult = copy(*pAudioFrame, *pTmp);
	
	m_AvailableFrames.push (pTmp);

	stiOSSignalClear(m_fdRSignal);

	if (m_playbackFrames.empty())
	{
		packetReadyStateSignal.Emit (false);
	}

STI_BAIL:

	return hResult;

}

int CstiAudioBridge::GetDeviceFD () const
{
	return stiOSSignalDescriptor(m_fdRSignal);	
}


/*!
 * \brief Allow frames to start entering the audio bridge
 */
stiHResult CstiAudioBridge::AudioPlaybackStart ()
{
	readyStateChangedSignal.Emit (true);
	return stiRESULT_SUCCESS;
}


/*!
 * \brief Stop frames from flowing through the audio bridge
 */
stiHResult CstiAudioBridge::AudioPlaybackStop ()
{
	readyStateChangedSignal.Emit (false);
	return stiRESULT_SUCCESS;
}


stiHResult CstiAudioBridge::AudioPlaybackCodecSet (
    EstiAudioCodec /*audioCodec*/)
{
	return stiRESULT_SUCCESS;
}


// audio playback methods
stiHResult CstiAudioBridge::AudioPlaybackPacketPut (
	SsmdAudioFrame * pAudioFrame)
{

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> thread_safe(m_Mutex);

	if (!m_AvailableFrames.empty ())
	{
		SsmdAudioFrame *pTmp = m_AvailableFrames.front();
		m_AvailableFrames.pop();

		hResult = copy(*pTmp, *pAudioFrame);
		stiTESTRESULT();

		// TODO check errors
		m_playbackFrames.push(pTmp);

		stiOSSignalSet(m_fdRSignal);

		// Notify videoRecord that a packet is ready
		packetReadyStateSignal.Emit (true);

		// Don't let the frame buffers become depleted...  if we've used them all,
		// start dropping frames so new ones can continue to flow in.  This was causing
		// problems with audio to not work properly through the bridge
		if (m_AvailableFrames.empty() && !m_playbackFrames.empty())
		{
			auto *audioFrame = m_playbackFrames.front ();
			m_playbackFrames.pop ();
			m_AvailableFrames.push (audioFrame);
		}
	}

STI_BAIL:

	return hResult;
	
}

int CstiAudioBridge::GetDeviceFD ()
{
	return stiOSSignalDescriptor(m_fdPSignal);
}


/*!
* \brief notifies us when a DTMF event has been received
* 
* \param eDtmfDigit 
* 
* \return nothing
*/
void CstiAudioBridge::DtmfReceived (EstiDTMFDigit eDtmfDigit) 
{
	IstiAudioOutput::InstanceGet()->DtmfReceived(eDtmfDigit);
}
