//
//  AudioIO.cpp
//  ntouch
//
//  Created by Daniel Shields on 12/18/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

#include "AudioIO.h"

namespace vpe {

AudioIO::AudioIO ()
	: m_handler (packetReadyStateSignal, readyStateChangedSignal)
{
	m_startHandlerRetryTimer.TimeoutSet (1000);
	m_startHandlerRetryTimer.RepeatLimitSet (20);
	m_connections.push_back(m_startHandlerRetryTimer.timeoutEvent.Connect ([this] ()
	{
		if (!stiIS_ERROR (m_handler.Start ()))
		{
			m_startHandlerRetryTimer.Stop ();
		}
	}));
}


AudioIO::~AudioIO ()
{}


stiHResult AudioIO::Initialize ()
{
	return m_handler.Initialize ();
}


stiHResult AudioIO::Uninitialize ()
{
	return m_handler.Uninitialize ();
}


stiHResult AudioIO::UpdateHandlerState ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if ((m_handler.IsPlaybackEnabled () || m_handler.IsRecordEnabled ()) && !m_handler.IsRunning ())
	{
		if (stiIS_ERROR (m_handler.Start ()))
		{
			m_startHandlerRetryTimer.Start ();
		}
		else
		{
			m_startHandlerRetryTimer.Stop ();
		}
	}
	else if (!m_handler.IsPlaybackEnabled () && !m_handler.IsRecordEnabled () && m_handler.IsRunning ())
	{
		hResult = m_handler.Stop ();
		m_startHandlerRetryTimer.Stop ();
	}
	
	return hResult;
}


stiHResult AudioIO::AudioRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.RecordEnable (m_recordingEnabled);
	stiTESTRESULT ();
	
	hResult = UpdateHandlerState ();
	stiTESTRESULT ();

	m_recording = true;

STI_BAIL:
	return hResult;
}


stiHResult AudioIO::AudioRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.RecordEnable (false);
	stiTESTRESULT ();

	hResult = UpdateHandlerState ();
	stiTESTRESULT ();

	m_recording = false;

STI_BAIL:
	return hResult;
}


stiHResult AudioIO::AudioRecordCodecSet (EstiAudioCodec audioCodec)
{
	return m_handler.RecordCodecSet (audioCodec);
}


stiHResult AudioIO::AudioRecordSettingsSet (const SstiAudioRecordSettings * audioRecordSettings)
{
	// Some codecs might use this but we don't implement any that do.
	return stiRESULT_SUCCESS;
}


stiHResult AudioIO::AudioRecordPacketGet (SsmdAudioFrame * audioFrame)
{
	return m_handler.PacketGet (audioFrame);
}


stiHResult AudioIO::PrivacySet (EstiBool enable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.RecordEnable (m_recording && !enable);
	stiTESTRESULT ();
	
	hResult = UpdateHandlerState ();
	stiTESTRESULT ();

	m_recordingEnabled = !enable;
	
	 audioPrivacySignal.Emit (!m_recordingEnabled);
	

STI_BAIL:
	return hResult;
}


stiHResult AudioIO::PrivacyGet (EstiBool *enabled) const
{
	if (enabled != nullptr)
	{
		*enabled = m_recordingEnabled ? estiFALSE : estiTRUE;
	}

	return stiRESULT_SUCCESS;
}


stiHResult AudioIO::AudioPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.PlaybackEnable (m_playbackEnabled);
	stiTESTRESULT ();
	
	hResult = UpdateHandlerState ();
	stiTESTRESULT ();
	
	m_playing = true;
	readyStateChangedSignal.Emit (true);

STI_BAIL:
	return hResult;
}


stiHResult AudioIO::AudioPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.PlaybackEnable (false);
	stiTESTRESULT ();

	hResult = UpdateHandlerState ();
	stiTESTRESULT ();

	m_playing = false;

STI_BAIL:
	return hResult;
}


stiHResult AudioIO::AudioPlaybackCodecSet (EstiAudioCodec audioCodec)
{
	return m_handler.PlaybackCodecSet (audioCodec);
}


stiHResult AudioIO::AudioPlaybackPacketPut (SsmdAudioFrame * audioFrame)
{
	return m_handler.PacketPut (audioFrame);
}


stiHResult AudioIO::CodecsGet (std::list<EstiAudioCodec> *codecs)
{
	return m_handler.CodecsGet (codecs);
}


stiHResult AudioIO::AudioOut (EstiSwitch eSwitch)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = m_handler.PlaybackEnable (m_playing && eSwitch == estiON);
	stiTESTRESULT ();
	
	hResult = UpdateHandlerState ();
	stiTESTRESULT ();

	m_playbackEnabled = (eSwitch == estiON);

STI_BAIL:
	return hResult;
}


} // namespace vpe
