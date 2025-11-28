/*!
 * \file AudioIO.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "BaseAudioInput.h"
#include "BaseAudioOutput.h"
#include "CstiTimer.h"
#include "AudioHandler.h"

namespace vpe {


class AudioIO : public BaseAudioInput, public BaseAudioOutput {
public:
	// --- Unused by this platform
	virtual stiHResult EchoModeSet (EsmdEchoMode eEchoMode) { return stiRESULT_SUCCESS; }
	virtual int GetDeviceFD () const { return -1; }
	virtual int GetDeviceFD () { return -1; }
	void DtmfReceived (EstiDTMFDigit dtmfDigit) { dtmfReceivedSignal.Emit (dtmfDigit); }
	// ---

	AudioIO ();
	virtual ~AudioIO ();

	stiHResult Initialize ();
	stiHResult Uninitialize ();

	stiHResult AudioRecordStart ();
	stiHResult AudioRecordStop ();
	stiHResult AudioRecordPacketGet (SsmdAudioFrame * pAudioFrame);
	stiHResult AudioRecordCodecSet (EstiAudioCodec eAudioCodec);
	stiHResult AudioRecordSettingsSet (const SstiAudioRecordSettings * pAudioRecordSettings);
	stiHResult PrivacySet (EstiBool bEnable);
	stiHResult PrivacyGet (EstiBool *pbEnabled) const;

	stiHResult AudioPlaybackStart ();
	stiHResult AudioPlaybackStop ();
	stiHResult AudioPlaybackCodecSet (EstiAudioCodec eAudioCodec);
	stiHResult AudioPlaybackPacketPut (SsmdAudioFrame * pAudioFrame);
	stiHResult CodecsGet (std::list<EstiAudioCodec> *pCodecs);
	stiHResult AudioOut (EstiSwitch eSwitch);

private:
	bool m_recordingEnabled = false;
	bool m_playbackEnabled = true;
	bool m_recording = false;
	bool m_playing = false;
	
	stiHResult UpdateHandlerState ();

	CstiTimer m_startHandlerRetryTimer;
	CstiSignalConnection::Collection m_connections;

	AudioHandler m_handler;
};


} // namespace vpe
