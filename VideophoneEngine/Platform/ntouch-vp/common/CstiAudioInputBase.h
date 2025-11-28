// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOINPUT_BASE_H
#define CSTIAUDIOINPUT_BASE_H

#include "stiSVX.h"
#include "stiError.h"

#include "IAudioInputVP2.h"
#include "IstiBluetooth.h"
#include "CstiAudioHandler.h"
#include "GStreamerSample.h"
#include "CstiEventQueue.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <queue>
#include <memory>
#include <tuple>

namespace vpe
{
class BluetoothAudio;
}

class CstiAudioInputBase : public CstiEventQueue, public IAudioInputVP2
{
public:

	CstiAudioInputBase ();
	
	~CstiAudioInputBase() override;
	
	CstiAudioInputBase (const CstiAudioInputBase &other) = delete;
	CstiAudioInputBase (CstiAudioInputBase &&other) = delete;
	CstiAudioInputBase &operator= (const CstiAudioInputBase &other) = delete;
	CstiAudioInputBase &operator= (CstiAudioInputBase &&other) = delete;

	stiHResult Initialize (
		CstiAudioHandler *audioHandler);

	stiHResult Startup ();

	stiHResult AudioRecordStart () override;

	stiHResult AudioRecordStop () override;

	stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec) override;

	stiHResult AudioRecordSettingsSet (
		const SstiAudioRecordSettings * pAudioRecordSettings) override;

	/*
	 * need this prototype because it is pure virtual
	 */
	stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode) override;

	stiHResult PrivacySet (
		EstiBool bEnable) override;
		
	stiHResult PrivacyGet (
		EstiBool *pbEnabled) const override;

	int GetDeviceFD () const override;

	virtual unsigned int MaxSampleRateGet() const;

	stiHResult headsetMicrophoneForce(bool set) override;

	stiHResult AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame) override;

public:
	int AudioLevelGet () const override;

protected:

	void addBufferToQueue (
		EstiAudioCodec codec,
		const GStreamerSample &sample);

	mutable std::recursive_mutex m_audioLevelMutex {};
	bool m_bPrivacy {false};
	std::atomic<int> m_audioLevel{0};

	std::queue<std::tuple<EstiAudioCodec, GStreamerBuffer>> m_BufferQueue {};

	CstiSignalConnection::Collection m_signalConnections {};

	CstiAudioHandler *m_pAudioHandler {nullptr};

private:

	void eventAudioPrivacySet (EstiBool enable);
};

#endif // CSTIAUDIOINPUT_H
