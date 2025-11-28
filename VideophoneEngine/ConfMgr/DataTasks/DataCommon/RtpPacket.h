//
//  RtpSendQueue.h
//  ntouch
//
//  Created by Dan Shields on 6/3/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#pragma once

#include "stiSVX.h"
#include <deque>
#include "Packetization.h"
#include <numeric>
#include <stack>
#include <mutex>

namespace vpe {

// A buffer used for RTP packets. Unlike CstiPacket, this packet buffer is used exclusively for packets before they're
// assembled into frames or after the frames have been disassembled into packets.
class RtpPacket
{
public:
	RtpPacket (size_t capacity)
	:
		m_buffer (capacity)
	{
		RvRtpParamConstruct(&m_rtpParam);
	}
	
	void payloadSizeSet (size_t payloadSize)
	{
		stiASSERTMSG (payloadSize <= m_buffer.size () - m_headerCapacity, "Payload size %d exceeds payload capacity %d", payloadSize, m_totalCapacity - m_headerCapacity);
		m_payloadSize = payloadSize;
	}
	
	void headerSizeSet (size_t headerSize)
	{
		stiASSERTMSG (headerSize <= m_headerCapacity, "Header size %d exceeds header capacity %d", headerSize, m_headerCapacity);
		m_headerSize = headerSize;
	}
	
	size_t payloadSizeGet () const
	{
		return m_payloadSize;
	}
	
	size_t headerSizeGet () const
	{
		return m_headerSize;
	}
	
	/// How much space in the packet buffer is allocated to the packet header.
	size_t headerHeadroomGet () const
	{
		return m_headerCapacity;
	}
	
	/// Changing this will change where the payload starts.
	void headerHeadroomSet (size_t headroom)
	{
		m_headerCapacity = headroom;
	}
	
	uint8_t *payloadGet ()
	{
		return &m_buffer[m_headerCapacity];
	}
	
	uint8_t *headerGet ()
	{
		return &m_buffer[m_headerCapacity - m_headerSize];
	}
	
	RvRtpParam *rtpParamGet ()
	{
		return &m_rtpParam;
	}
	
	// Get whether this packet belongs to a keyframe
	bool isKeyframeGet ()
	{
		return m_isKeyframe;
	}
	
	// Set whether this packet belongs to a keyframe
	void isKeyframeSet(bool isKeyframe)
	{
		m_isKeyframe = isKeyframe;
	}
	
private:
	std::vector<uint8_t> m_buffer;

	size_t m_headerCapacity {0};
	size_t m_totalCapacity {0};
	size_t m_headerSize {0};
	size_t m_payloadSize {0};
	
	RvRtpParam m_rtpParam {};
	
	bool m_isKeyframe {false};
};

class RtpPacketPool
{
public:
	
	RtpPacketPool () = default;
	
	~RtpPacketPool ()
	{
		if (m_pool != nullptr)
		{
			destroy ();
		}
	}
	
	RtpPacketPool(const RtpPacketPool& other) = delete;
	RtpPacketPool(RtpPacketPool&& other) = delete;
	RtpPacketPool& operator=(const RtpPacketPool& other) = delete;
	RtpPacketPool& operator=(RtpPacketPool&& other) = delete;
	
	void create (size_t size, size_t capacity)
	{
		std::lock_guard<std::recursive_mutex> lk{m_mutex};
		m_size = size;
		m_pool = (RtpPacket *) ::operator new (sizeof (RtpPacket) * m_size);
		for (size_t i = 0; i < m_size; ++i)
		{
			m_available.push (new (m_pool + i) RtpPacket (capacity));
		}
	}
	
	void destroy ()
	{
		std::lock_guard<std::recursive_mutex> lk{m_mutex};
		for (size_t i = 0; i < m_size; ++i)
		{
			m_pool[i].~RtpPacket ();
			m_available.pop();
		}
		
		::operator delete (m_pool);
		m_pool = nullptr;
	}
	
	bool isEmptyGet ()
	{
		std::lock_guard<std::recursive_mutex> lk{m_mutex};
		return m_available.empty ();
	}
	
	std::shared_ptr<RtpPacket> pop ()
	{
		std::lock_guard<std::recursive_mutex> lk{m_mutex};
		if (!m_available.empty ())
		{
			auto packet = m_available.top ();
			m_available.pop ();
			return std::shared_ptr<RtpPacket> (packet, [this](RtpPacket *packet)
			{
				std::lock_guard<std::recursive_mutex> lk{m_mutex};
				m_available.push (packet);
			});
		}
		
		return nullptr;
	}
	
private:
	std::recursive_mutex m_mutex;
	RtpPacket *m_pool {nullptr};
	size_t m_size {0};
	std::stack<RtpPacket *> m_available;
};

}
