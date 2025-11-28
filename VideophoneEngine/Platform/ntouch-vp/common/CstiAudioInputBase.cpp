// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioInputBase.h"
#include "stiTrace.h"
#include "stiTools.h"

#include <err.h>
#include <sys/time.h>
#include <cmath>
#include <tuple>

#define MAX_SAMPLE_RATE 30		// in ms

//#define DISABLE_AUDIO_INPUT

/*!
 * \brief Default Constructor
 */
CstiAudioInputBase::CstiAudioInputBase ()
:
	CstiEventQueue ("CstiAudioInputBase")
{
}


CstiAudioInputBase::~CstiAudioInputBase ()
{
	StopEventLoop ();
}


stiHResult CstiAudioInputBase::Initialize (
	CstiAudioHandler *pAudioHandler)
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifndef DISABLE_AUDIO_INPUT

	stiTESTCOND (pAudioHandler, stiRESULT_TASK_INIT_FAILED);

	m_pAudioHandler = pAudioHandler;

	m_signalConnections.push_back(m_pAudioHandler->m_newRecordSampleSignal.Connect (
		[this](EstiAudioCodec codec, const GStreamerSample &sample)
		{
			PostEvent ([this, codec, sample]()
			{
				addBufferToQueue (codec, sample);
			});
		}
	));

	m_signalConnections.push_back(m_pAudioHandler->m_audioLevelSignal.Connect (
			[this](int level)
			{
				m_audioLevel = level;
			}
		));

	packetReadyStateSignal.Emit (false);

STI_BAIL:

#endif
	return hResult;
}


stiHResult CstiAudioInputBase::Startup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!CstiEventQueue::StartEventLoop ())
	{
		hResult = stiRESULT_ERROR;
	}

	return hResult;
}

stiHResult CstiAudioInputBase::AudioRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiAudioEncodeDebug,
		stiTrace ("CstiAudioInputBase::AudioRecordStart\n");
	);

	{
		// We need to ensure that the queue is empty before we start recording.
		// Otherwise, the packetReadyStateSignal will not work properly.
		std::lock_guard<std::recursive_mutex> lockAudioLevel(m_audioLevelMutex);
		while (!m_BufferQueue.empty ())
		{
			m_BufferQueue.pop ();
		}
	}

	m_pAudioHandler->recordStart ();

	return hResult;
}


/*!\brief Stops Audio Record
 *
 * \return stiHResult
 */
stiHResult CstiAudioInputBase::AudioRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pAudioHandler->recordStop ();

	return (hResult);
}


/*!
 * \brief Sets audio record codec 
 * This sets which of the audio record 
 * codecs to be used for recording an audio 
 * stream 
 * 
 * \param EstiAudioCodec audioCodec
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInputBase::AudioRecordCodecSet (
	EstiAudioCodec audioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pAudioHandler->recordCodecSet (audioCodec);

	return (hResult);
}


/*!
 * \brief Audio Record Setting Set 
 * uses the SstiAudioRecordSettings struct to setup 
 * the audio stream 
 * 
 * \param const SstiAudioRecordSettings* pAudioRecordSettings 
 *    contains un16SamplesPerPacket which holds
 *    samples per audio frame/packet
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInputBase::AudioRecordSettingsSet (
	const SstiAudioRecordSettings *audioRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pAudioHandler->recordSettingsSet (*audioRecordSettings);

	return (hResult);
}


/*!
 * \brief Sets Echo mode
 * 
 * \param EsmdEchoMode eEchoMode 
 * enum setting echo mode to on or off 
 * 
 * \return stiHResult 
 *
 * (note: don't do anything)
 */
stiHResult CstiAudioInputBase::EchoModeSet (
	EsmdEchoMode eEchoMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

/*!
 * \brief Sets Privacy to on or off
 * 
 * \param bool bEnable 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInputBase::PrivacySet (
	EstiBool bEnable)
{
	stiDEBUG_TOOL(g_stiAudioEncodeDebug,
		stiTrace("CstiAudioInputBase::PrivacySet - %s\n", torf(bEnable));
	);

	PostEvent([this, bEnable]
	{
		eventAudioPrivacySet(bEnable);
	});

	return stiRESULT_SUCCESS;
}

	
void CstiAudioInputBase::eventAudioPrivacySet (
	EstiBool enable)
{
	if (enable != m_bPrivacy)
	{
		m_bPrivacy = enable;

		audioPrivacySignal.Emit(enable);
	}
}


/*!
 * \brief Gets the privacy status
 * 
 * \param bool* pbEnabled 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInputBase::PrivacyGet (
	EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_audioLevelMutex);
	*pbEnabled = (EstiBool)m_bPrivacy;
	
	return hResult;
}

/*!
 * \brief Get Device File Descriptor
 * 
 * \return int 
 */
int CstiAudioInputBase::GetDeviceFD () const
{
	return -1;
}

/*!
 * \brief Get the maximum sample rate (in ms)
 *
 * \return unsigned int
 */
unsigned int CstiAudioInputBase::MaxSampleRateGet() const
{
	return (MAX_SAMPLE_RATE);
}


int CstiAudioInputBase::AudioLevelGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_audioLevelMutex);

	if (m_bPrivacy)
	{
		return 0;
	}

	return m_audioLevel;
}


stiHResult CstiAudioInputBase::headsetMicrophoneForce(bool set)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pAudioHandler->headsetAudioForce(set);

	return (hResult);
}


/*!
 * \brief Gets audio record packet
 *
 * \param SsmdAudioFrame* pAudioFrame
 *
 * \return stiHResult
 */
stiHResult CstiAudioInputBase::AudioRecordPacketGet (
	SsmdAudioFrame * pAudioFrame)
{
	int offs = 0;

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lockExec(m_execMutex);
	std::lock_guard<std::recursive_mutex> lockAudioLevel(m_audioLevelMutex);

	EstiAudioCodec codec = estiAUDIO_NONE;

	while (!m_BufferQueue.empty ())
	{
		GStreamerBuffer buffer;
		std::tie(codec, buffer) = m_BufferQueue.front ();

		if (offs + buffer.sizeGet () > pAudioFrame->unBufferMaxSize)
		{
			break;
		}

		memcpy (pAudioFrame->pun8Buffer + offs, buffer.dataGet (), buffer.sizeGet ());
		offs += buffer.sizeGet ();

		m_BufferQueue.pop ();
	}

	if (m_BufferQueue.empty())
	{
		packetReadyStateSignal.Emit (false);
	}

	pAudioFrame->esmdAudioMode = codec;
	pAudioFrame->unFrameSizeInBytes = offs;

	return (hResult);
}

void CstiAudioInputBase::addBufferToQueue (
	EstiAudioCodec codec,
	const GStreamerSample &sample)
{
	if (sample.get () != nullptr)
	{
		std::lock_guard<std::recursive_mutex> lock(m_audioLevelMutex);

		if (!m_bPrivacy)
		{
			auto buffer = sample.bufferGet ();

			/*
			 * if privacy is not set, queue it up, otherwise throw it away
			 */
			if (buffer.get () != nullptr)
			{
				m_BufferQueue.push (std::make_tuple(codec, buffer));

				if (m_BufferQueue.size() == 1)
				{
					packetReadyStateSignal.Emit (true);
				}
			}
		}
	}
}
