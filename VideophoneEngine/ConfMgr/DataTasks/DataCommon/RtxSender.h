//
//  RtxSender.hpp
//  ntouch
//
//  Created by Dan Shields on 7/7/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#pragma once

#include "RtpSender.h"
#include "CstiRtpSession.h"

namespace vpe {

class RtxSender
{
public:
	/// ssrcToRtx is the SSRC that we want to retransmit with this RtxSender.
	RtxSender(std::shared_ptr<vpe::RtpSession> session, RtpSender& sender, uint32_t ssrcToRtx, uint32_t payloadFormat, size_t bufferSize, int remoteSipVersion)
		: m_session(session),
		  m_bufferSize(bufferSize),
		  m_rtxPayloadType(payloadFormat),
		  m_remoteSipVersion(remoteSipVersion)
	{
		m_sendConnection = sender.packetSendSignal.Connect ([this, ssrcToRtx](uint32_t ssrc, std::shared_ptr<RtpPacket> packet, RvStatus status) {
			if (ssrc == ssrcToRtx && packet->rtpParamGet()->sequenceNumber != 0) {
				packet->rtpParamGet ()->sSrc = ssrc;
				storePacket(packet);
			}
		});
	}
	
	// Process received RTCP NACK feedback. Schedules packets on RtpSender if needed.
	void processNack (
		uint32_t ssrcAddressee,
		uint16_t packetID,
		uint16_t bitMask,
		uint16_t *packetsIgnored,
		uint16_t *packetsSent,
		uint16_t *packetsNotFound,
		uint64_t *amountSent);
	
private:
	std::map<uint16_t, uint16_t> checkForStoredPacket (
		uint16_t packetID,
		uint16_t bitMask);
	
	void storePacket (std::shared_ptr<RtpPacket> packet);
	
private:
	struct RtxPacket
	{
		std::shared_ptr<RtpPacket> packet;
		std::chrono::steady_clock::time_point lastTimeSent;
		
		// We need to keep some information from the original packet around for retransmission
		uint8_t payloadType;
		uint16_t sequenceNumber;
		uint32_t timestamp;
		uint32_t ssrc;
		bool marker;
	};
	
	std::recursive_mutex m_mutex;
	
	std::shared_ptr<vpe::RtpSession> m_session;
	size_t m_bufferSize;
	uint32_t m_rtxSsrc {0};
	int32_t m_rtxPayloadType;
	std::deque<RtxPacket> m_rtxPackets;
	
	CstiSignalConnection m_sendConnection;
	
	int m_remoteSipVersion{};
};

}
