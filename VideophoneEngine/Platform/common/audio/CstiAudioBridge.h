//
//  CstiAudioBridge.h
//  ntouchMac
//
//  Created by Dennis Muhlestein on 11/20/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef __CstiAudioBridge_h__
#define __CstiAudioBridge_h__

#include "BaseAudioInput.h"
#include "BaseAudioOutput.h"
#include "stiOSSignal.h"

#include <queue>
#include <memory>

class CstiAudioBridge : public BaseAudioInput, public BaseAudioOutput
{
public:

	CstiAudioBridge();

	CstiAudioBridge (const CstiAudioBridge &other) = delete;
	CstiAudioBridge (CstiAudioBridge &&other) = delete;
	CstiAudioBridge &operator= (const CstiAudioBridge &other) = delete;
	CstiAudioBridge &operator= (CstiAudioBridge &&other) = delete;

	~CstiAudioBridge() override;

	// audio input methods


	// ignoring start/stop
	// because we're just going to pass audio through to the other side
	stiHResult AudioRecordStart () override { return stiRESULT_SUCCESS; }
	stiHResult AudioRecordStop () override { return stiRESULT_SUCCESS; }
	stiHResult headsetMicrophoneForce(bool) { return stiRESULT_SUCCESS; }
	
	
	stiHResult AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame) override;
	stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec) override { return stiRESULT_SUCCESS; }
	stiHResult AudioRecordSettingsSet (
		const IstiAudioInput::SstiAudioRecordSettings * pAudioRecordSettings) override { return stiRESULT_SUCCESS; }
	stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode) override { return stiRESULT_SUCCESS; }
	stiHResult PrivacySet (
		EstiBool bEnable) override { return stiRESULT_SUCCESS; }
	stiHResult PrivacyGet (
		EstiBool *pbEnabled) const override { *pbEnabled=estiFALSE; return stiRESULT_SUCCESS; }
	
	int GetDeviceFD () const override;
	
	
	// audio output methods
	stiHResult AudioPlaybackStart () override;
	stiHResult AudioPlaybackStop () override;
	stiHResult AudioPlaybackCodecSet (EstiAudioCodec audioCodec) override;
		stiHResult AudioPlaybackPacketPut (
			SsmdAudioFrame * pAudioFrame) override;
	int GetDeviceFD () override;
	

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || defined(stiMVRS_APP)
	stiHResult AudioOut (
		EstiSwitch eSwitch) override { return stiRESULT_SUCCESS; };
#endif
		
	void DtmfReceived (EstiDTMFDigit eDtmfDigit) override;
	
	stiHResult CodecsGet (
		std::list<EstiAudioCodec> *pCodecs) override {return stiRESULT_SUCCESS; };
	
#ifdef stiSIGNMAIL_AUDIO_ENABLED
	virtual stiHResult AudioPlaybackSettingsSet(
		SstiAudioDecodeSettings *pAudioDecodeSettings)
	{
		return stiRESULT_SUCCESS;
	}
#endif

private:
		STI_OS_SIGNAL_TYPE m_fdRSignal{}; // read
		STI_OS_SIGNAL_TYPE m_fdPSignal{}; // playback
		std::queue<SsmdAudioFrame *> m_AvailableFrames;
		std::queue<SsmdAudioFrame *> m_playbackFrames;
		std::recursive_mutex m_Mutex;
	
};

using CstiAudioBridgeSharedPtr = std::shared_ptr<CstiAudioBridge>;

#endif /* defined(__ntouchMac__CstiAudioBridge__) */
