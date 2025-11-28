/*!
* \file CstiTextPlayback.h
* \brief See CstiTextPlayback.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "CstiRtpSession.h"
#include "stiTrace.h"
#include "CstiTextPlaybackRead.h"
#include "MediaPlayback.h"
#include "stiPayloadTypes.h"
#include "RvSipMid.h"
#include "RtpPayloadAddon.h"

class CstiTextPlayback : public vpe::MediaPlayback<CstiTextPlaybackRead, CstiTextPacket, EstiTextCodec, TTextPayloadMap>
{
public:

	CstiTextPlayback () = delete;

	CstiTextPlayback (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t maxRate,
		bool rtpKeepAliveEnable,
		RvSipMidMgrHandle sipMidMgr,
		int remoteSipVersion);

	CstiTextPlayback (const CstiTextPlayback &other) = delete;
	CstiTextPlayback (CstiTextPlayback &&other) = delete;
	CstiTextPlayback &operator= (const CstiTextPlayback &other) = delete;
	CstiTextPlayback &operator= (CstiTextPlayback &&other) = delete;

	~CstiTextPlayback () override = default;

	// Overridden base class functions

	int ClockRateGet() const override;

public:
	CstiSignal<uint16_t*> remoteTextReceivedSignal;

private:

	stiHResult Muted (
		eMutedReason eReason) override;

	stiHResult Unmuted (
		eMutedReason eReason) override;

	stiHResult DataInfoStructLoad () override;

	stiHResult DataChannelHold () override;
	stiHResult DataChannelResume () override;

	vpe::EventTimer m_checkForDelayedBlocksTimer;

	CstiSignalConnection::Collection m_signalConnections;

	EstiBool m_bReceivedPrintableText = estiFALSE;

	//
	// Supporting functions
	//

	stiHResult ReadPacketProcess (
		const std::shared_ptr<CstiTextPacket> &packet);

	stiHResult PacketProcess (
		std::shared_ptr<CstiTextPacket> packet);

	stiHResult PacketQueueProcess (bool *pbFoundEntry);

	stiHResult Playback (SstiT140Block *pstT140Block, bool bInformOfLoss);

	stiHResult SetDeviceCodec (EstiTextCodec eMode) override;

	void EventCheckForDelayedBlocksTimerTimeout ();
	void EventDataAvailable ();

	std::map<RvUint32, SstiT140Block*> m_oooT140Blocks;


};
