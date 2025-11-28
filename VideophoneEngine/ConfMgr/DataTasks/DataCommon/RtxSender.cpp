//
//  RtxSender.cpp
//  ntouch
//
//  Created by Dan Shields on 7/7/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#include "RtxSender.h"
#include "CstiDataTaskCommonP.h"
#include <bitset>

namespace vpe {

/*! \brief Checks if the request packetID is available for retransmission and where it is stored.
 *
 * \param data Data to be stored.
 * \param size The size of the data being stored.
 * \param sequenceNumber The sequence number of the packet being stored.
 */
void RtxSender::storePacket (std::shared_ptr<RtpPacket> packet)
{
	std::lock_guard<std::recursive_mutex> lk{m_mutex};
	
	if (m_rtxPackets.size () >= m_bufferSize)
	{
		// Discard an RTX buffer
		m_rtxPackets.pop_front ();
	}
	
	auto oldRtpParam = *packet->rtpParamGet ();
	if (m_rtxPayloadType != INVALID_DYNAMIC_PAYLOAD_TYPE && m_remoteSipVersion < webrtcSSV)
	{
		auto originalSeqNum = oldRtpParam.sequenceNumber;
		// We were not properly converting from host to network byte order we need to do this going forward.
		// TODO: Remove when no longer dealing with older Sorenson builds using NACK/RTX.
		if (m_remoteSipVersion > 4)
		{
			originalSeqNum = htons(oldRtpParam.sequenceNumber);
		}
		// Set the original sequence number with room for a new RTP header
		// followed by the original payload in case packet needs to be retransmitted.
		packet->headerHeadroomSet (packet->headerHeadroomGet () - sizeof (RvUint16));
		reinterpret_cast<RvUint16 *> (packet->payloadGet())[0] = originalSeqNum;
		packet->payloadSizeSet (packet->payloadSizeGet () + sizeof (RvUint16));
	}
	
	RtxPacket rtxPacket{
		packet,
		std::chrono::steady_clock::now (),
		
		oldRtpParam.payload,
		oldRtpParam.sequenceNumber,
		oldRtpParam.timestamp,
		oldRtpParam.sSrc,
		static_cast<bool>(oldRtpParam.marker)
	};
	
	m_rtxPackets.push_back (std::move (rtxPacket));
}

/*! \brief Checks if the request packetID is available for retransmission and where it is stored.
 *
 * \param packetID Packet ID to check for being stored.
 * \param bitMask 16-bit mask used to identify the next 16 contigious packets missing.
 */
std::map<uint16_t, uint16_t> RtxSender::checkForStoredPacket (
	uint16_t packetID,
	uint16_t bitMask)
{
	uint16_t rtxStartSeq = m_rtxPackets.front ().sequenceNumber;
	uint16_t rtxEndSeq = m_rtxPackets.back ().sequenceNumber;
	std::map<uint16_t, uint16_t> packetsStored;
	std::bitset<NACK_BITMASK_SIZE> additionalMissing = bitMask;
	uint16_t position = 0;
	
	if (packetID >= rtxStartSeq)
	{
		position = packetID - rtxStartSeq;
	}
	else if (packetID <= rtxEndSeq)
	{
		// We've wrapped sequence number look for the packet near the end.
		position = rtxEndSeq - packetID;
		if (position < m_rtxPackets.size ())
		{
			position = static_cast<uint16_t>(m_rtxPackets.size() - position - 1);
		}
	}
	
	if (position < m_rtxPackets.size ())
	{
		packetsStored[position] = packetID;
	}
	
	for (uint16_t i = 0; i < NACK_BITMASK_SIZE; i++)
	{
		if (additionalMissing.test(i) &&
			(position + i + 1U) < m_rtxPackets.size ())
		{
			packetsStored[position + i + 1] = packetID + i + 1;
		}
	}
	
	return packetsStored;
}

void RtxSender::processNack(uint32_t ssrcAddressee, uint16_t packetID, uint16_t bitMask, uint16_t *packetsIgnored,
	uint16_t *packetsSent,
	uint16_t *packetsNotFound,
	uint64_t *amountSent)
{
	std::lock_guard<std::recursive_mutex> lk{m_mutex};

	uint16_t numberIgnored = 0;
	uint16_t numberSent = 0;
	uint16_t numberNotFound = 0;
	uint64_t numberBytesSent = 0;
	
	if (!m_rtxPackets.empty())
	{
		auto packetsStored = checkForStoredPacket (packetID, bitMask);
		auto now = std::chrono::steady_clock::now();
		
		for (auto packetPosition : packetsStored)
		{
			auto &rtxPacket = m_rtxPackets.at(packetPosition.first);
			auto packet = rtxPacket.packet;
			
			if (rtxPacket.lastTimeSent + std::chrono::microseconds(100) < now)
			{
				if (packet && rtxPacket.sequenceNumber == packetPosition.second)
				{
					RvRtpParamConstruct (packet->rtpParamGet ());
					packet->rtpParamGet ()->timestamp = rtxPacket.timestamp;
					packet->rtpParamGet ()->marker = rtxPacket.marker;
					packet->rtpParamGet ()->bAutoCreateStream = RV_TRUE;
					packet->rtpParamGet ()->sByte = packet->headerSizeGet ();
				
					// TODO: Remove sip version check when we have implemented RFC#5576
					if (m_rtxPayloadType != INVALID_DYNAMIC_PAYLOAD_TYPE && m_remoteSipVersion < webrtcSSV)
					{
						packet->rtpParamGet()->payload = m_rtxPayloadType;
						packet->rtpParamGet()->sequenceNumber = 0;
						packet->rtpParamGet ()->bUseID = RV_TRUE;
					}
					else
					{
						packet->rtpParamGet ()->payload = rtxPacket.payloadType;
						packet->rtpParamGet ()->sequenceNumber = rtxPacket.sequenceNumber;
						m_rtxSsrc = rtxPacket.ssrc;
					}
					
					rtxPacket.lastTimeSent = now;
					RvRtpWriteEx(
						m_session->rtpGet(),
						m_rtxSsrc,
						packet->headerGet (),
						packet->headerSizeGet () + packet->payloadSizeGet (),
						packet->rtpParamGet ());
					m_rtxSsrc = packet->rtpParamGet()->sSrc;
					
					numberBytesSent += packet->headerSizeGet () + packet->payloadSizeGet ();
					numberSent++;
				}
				else
				{
					numberNotFound++;
				}
			}
			else
			{
				numberIgnored++;
			}
		}
	}
	
	*packetsIgnored = numberIgnored;
	*packetsSent = numberSent;
	*packetsNotFound = numberNotFound;
	*amountSent = numberBytesSent;
}

}
