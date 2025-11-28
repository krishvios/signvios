/*!
* \file CstiAudioPlayback.h
* \brief See CstiAudioPlayback.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "IstiAudioOutput.h"
#include "CstiRtpSession.h"
#include "stiTrace.h"
#include "CstiAudioPlaybackRead.h"
#include "MediaPlayback.h"
#include "stiPayloadTypes.h"
#include "RvSipMid.h"
#include <memory>

class CstiAudioPlayback : public vpe::MediaPlayback<CstiAudioPlaybackRead, CstiAudioPacket, EstiAudioCodec, TAudioPayloadMap>
{
public:

	CstiAudioPlayback () = delete;

	CstiAudioPlayback (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t maxRate,
		bool rtpKeepAliveEnable,
		RvSipMidMgrHandle hSipMidMgr,
		int remoteSipVersion);

	CstiAudioPlayback (const CstiAudioPlayback &other) = delete;
	CstiAudioPlayback (CstiAudioPlayback &&other) = delete;
	CstiAudioPlayback &operator= (const CstiAudioPlayback &other) = delete;
	CstiAudioPlayback &operator= (CstiAudioPlayback &&other) = delete;

	~CstiAudioPlayback () override;

	int ClockRateGet() const override;

	// Overridden base class functions
	stiHResult Initialize (
		const TAudioPayloadMap &audioPayloadMap) override;

	void Uninitialize () override;

	stiHResult AudioOutputDeviceSet(
	    const std::shared_ptr<IstiAudioOutput> &audioOutput);


private:
	stiHResult Muted (
		eMutedReason eReason) override;

	stiHResult Unmuted (
		eMutedReason eReason) override;

	stiHResult DataInfoStructLoad () override;

	stiHResult DataChannelHold () override;
	stiHResult DataChannelResume () override;

	void ConnectSignals ();
	void DisconnectSignals ();

	void ConnectAudioOutputSignals ();
	void DisconnectAudioOutputSignals ();

	static stiHResult CallbackFromSubtasks (
		int32_t n32Message, // type of message
		size_t MessageParam,	// holds data specific to the function to call
		size_t CallbackParam,			// points to the instantiated AudioPlayback object
		size_t CallbackFromId);

	std::shared_ptr<IstiAudioOutput> m_pAudioOutput = nullptr;
	
	uint32_t m_un32TimestampIncrement = 0;						// Audio playback timestamp increment

	EstiBool m_bSendToDriver = estiFALSE;								// indicator of a audio frame being ready to decode
	int m_nCurrentFrameSize = 0; 								// Frame size of SRA data
	uint16_t m_un16ExpectedPacket = 0;							// The sequence number expecting to receive in the next audio packet.

	CstiPacketQueue<CstiAudioPacket> m_oDisplayQueue;
	std::shared_ptr<CstiAudioPacket> m_displayPacket = nullptr;
	uint16_t m_nNextPacketDisplay = 0;

	CstiPacketQueue<CstiAudioPacket> m_oPacketQueue;					// Holding out-of-order packets
	std::shared_ptr<CstiAudioPacket> m_framePacket = nullptr;						// The packet object containing the audio frame ready to send to audio decoder

	//
	// Supporting functions
	//

	stiHResult SetDeviceCodec (
		EstiAudioCodec eMode) override;

	EstiResult ReadPacketProcess (
		const std::shared_ptr<CstiAudioPacket> &packet);

	stiHResult PacketProcess (
		std::shared_ptr<CstiAudioPacket> packet);

	EstiResult PacketQueueProcess (
		const std::shared_ptr<CstiAudioPacket> &readPacket);

	EstiResult PutPacketIntoQueue (
		const std::shared_ptr<CstiAudioPacket> &packet);

	stiHResult AudioQueueEmpty (
		CstiPacketQueue<CstiAudioPacket> * poQueue);

	EstiResult ProcessAvailableData ();

	EstiResult AudioPacketToDisplayQueue ();

	EstiResult SyncPlayback ();

public:

private:

	enum EEventType 
	{		
		estiAP_DEVICE_SET = estiEVENT_NEXT,
	};


	EstiResult ReSyncJitterBuffer ();

	//
	// Event Handlers
	//
	void EventDataAvailable ();
	void EventAudioOutputDeviceSet (
	    std::shared_ptr<IstiAudioOutput> audioOutput);

private:

	CstiSignalConnection m_audioOutputReadyStateChangedSignalConnection;
	CstiSignalConnection m_playbackReadDtmfReceivedSignalConnection;
	CstiSignalConnection m_playbackReadDataAvailableSignalConnection;
};
