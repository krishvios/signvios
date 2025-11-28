/*!
 * \file CstiVideoPlaybackRead.h
 * \brief contains CstiVideoPlaybackRead class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "stiTrace.h"
#include "CstiDataTaskCommonP.h"
#include "PacketPool.h"
#include "CstiPacketQueue.h"
#include "stiPayloadTypes.h"
#include "IstiVideoOutput.h"
#include "CstiRtpSession.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiVideoPacket.h"
#include "MediaPlaybackRead.h"

extern "C"
{
#include "RvTurn.h"
}
#include <string>
#include <array>
#include <tuple>

class CstiVideoPlaybackRead : public vpe::MediaPlaybackRead<CstiVideoPacket, TVideoPayloadMap>
{
public:

	CstiVideoPlaybackRead () = delete;

	CstiVideoPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session);

	CstiVideoPlaybackRead (const CstiVideoPlaybackRead &other) = delete;
	CstiVideoPlaybackRead (CstiVideoPlaybackRead &&other) = delete;
	CstiVideoPlaybackRead &operator= (const CstiVideoPlaybackRead &other) = delete;
	CstiVideoPlaybackRead &operator= (CstiVideoPlaybackRead &&other) = delete;

	~CstiVideoPlaybackRead() override;

	stiHResult Initialize () override;

	// Data Movement Functions
	
#ifdef ercDebugPacketQueue
void VideoPacketQueuesLock ();
void VideoPacketQueuesUnlock ();
int VideoPacketFullCountGet (){return m_oFullQueue.CountGet ();};
int VideoPacketEmptyCountGet (){return m_oEmptyQueue.CountGet ();};
#endif //#ifdef ercDebugPacketQueue

	std::shared_ptr<CstiVideoPacket> MediaPacketFullGet (uint16_t un16ExpectedPacket) override;

	// Data flow functions
	stiHResult DataChannelClose () override;

	stiHResult DataChannelInitialize (
		uint32_t un32InitialRtcpTime,
		const TVideoPayloadMap *PayloadMap) override;
	
	void RTPOnReadEvent(RvRtpSession hRTP) override;

private:
	//
	// Supporting functions
	//
	std::tuple<EstiVideoCodec, EstiPacketizationScheme> CodecLookup (
		uint8_t payload);
	
	EstiResult DataUnPack (
		const std::shared_ptr<CstiVideoPacket> &packet);

	EstiResult ReadPacketProcess (
		const std::shared_ptr<CstiVideoPacket> &packet);

	EstiPacketResult PacketRead (
		const std::shared_ptr<CstiVideoPacket> &packet);

	EstiPacketResult PacketProcessFromPort ();

	EstiResult RTCPReportProcess ();

	stiHResult ReceivedPacketsProcess ();

	//
	// Event Handlers
	//
	void EventRtpSocketDataAvailable () override;
	void EventRtpDataAvailable ();


private:

#ifdef stiDEBUGGING_TOOLS
	int m_nDiscardedVideoPackets = 0;			// Number of discarded packets
#endif

	std::array<
		std::array<uint8_t, nMAX_VIDEO_PLAYBACK_PACKET_BUFFER + 1 + 1>,
		nMAX_VP_BUFFERS
		> m_videoBuffer{};

	vpe::VideoFrame m_astVideoStruct[nMAX_VP_BUFFERS]{};

}; // end CstiVideoPlaybackRead

