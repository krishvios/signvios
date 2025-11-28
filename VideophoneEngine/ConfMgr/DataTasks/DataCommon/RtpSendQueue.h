//
//  RtpSendQueue.h
//  ntouch
//
//  Created by Dan Shields on 7/7/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#pragma once

#include "RtpPacket.h"
#include "CstiSignal.h"

namespace vpe {

class RtpSendQueue {
public:
	
	RtpSendQueue()
	{
		m_mediaFrameCreatedConnection = mediaFrameCreated.Connect([this](size_t frameSize) {
			m_sizeOfLastFrame = frameSize;
		});
	}
	
	/// Pushes a new packet onto the queue
	void push (std::shared_ptr<RtpPacket> packet, std::chrono::steady_clock::time_point time)
	{
		m_items.push_back (Item { packet, time });
	}
	
	/// Pops the oldest packet off of the queue
	std::shared_ptr<RtpPacket> pop ()
	{
		if (!m_items.empty ())
		{
			auto packet = m_items.front ().packet;
			m_items.pop_front ();
			return packet;
		}
		
		return nullptr;
	}
	
	/// Looks at the oldest packet on the queue without popping it off the queue.
	std::shared_ptr<RtpPacket> peek ()
	{
		return m_items.front ().packet;
	}
	
	/// The size of the newest frame to be added to the queue. Clients should use the mediaFrameCreated method to populate this.
	size_t sizeOfLastFrameGet ()
	{
		return m_sizeOfLastFrame;
	}
	
	/// Empty the queue of packets
	void clear ()
	{
		m_sizeOfLastFrame = 0;
		
		auto packetsDropped = m_items.size ();
		m_items.clear ();
		packetsDroppedSignal.Emit (packetsDropped);
	}
	
	/// The number of packets in the queue
	size_t sizeGet ()
	{
		return m_items.size ();
	}
	
	/// Whether there are packets in the buffer
	bool isEmptyGet ()
	{
		return m_items.empty ();
	}
	
	/// The total number of bytes in the queue. This is only an estimate because we don't know what radvision will send
	/// until it sends the data.
	size_t estimatedRtpBytesGet ()
	{
		return std::accumulate(m_items.begin (), m_items.end (), 0, [](const size_t& size, const Item& item) {
			return size + item.packet->payloadSizeGet () + item.packet->headerSizeGet ();
		});
	}
	
	/// How behind are we sending frames?
	std::chrono::steady_clock::time_point oldestTimestampGet () {
		return m_items.front().time;
	}
	
public:
	/// Signal is emitted whenever packets are dropped by the RtpQueue (for example, to maintain a maximum
	/// queue delay). The parameter is the number of packets dropped.
	CstiSignal<size_t> packetsDroppedSignal;
	
	/// This signal is used to tell RtpSender and any stats gathering that a new media frame (with size as
	/// the parameter) was created and added to the queue.
	CstiSignal<size_t> mediaFrameCreated;

private:
	
	CstiSignalConnection m_mediaFrameCreatedConnection;
	
	struct Item {
		std::shared_ptr<RtpPacket> packet;
		std::chrono::steady_clock::time_point time;
	};
	
	std::deque<Item> m_items;
	size_t m_sizeOfLastFrame {0};
};

}
