//
//  RtpSender.h
//  ntouch
//
//  Created by Dan Shields on 7/2/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#pragma once
#include "rtp.h"
#include "RtpPacket.h"
#include "CstiSignal.h"
#include "RtpSendQueue.h"
#include "CstiRtpSession.h"

#include <thread>
#include <condition_variable>
#include <algorithm>

/// The RtpSender should send at a rate faster than the target bitrate so it never gets backed up do to normal variations in bitrate from the encoder.
#define stiPACING_RATE_FACTOR 1.5

namespace vpe {

/// Adapted from WebRTC's IntervalBudget. Every sleep interval increase the budget, then the budget is spent by sending packets until the budget is overspent.
class IntervalBudget
{
public:
	IntervalBudget(size_t targetRate)
	{
		targetRateSet(targetRate);
	}
	
	void budgetIncrease (std::chrono::steady_clock::duration delta)
	{
		float bytes = static_cast<float>(m_targetRate) * 8 * std::chrono::duration_cast<std::chrono::duration<float>>(delta).count();
		m_bytesRemaining = static_cast<int>(bytes);
	}
	
	void budgetUse (size_t bytes)
	{
		m_bytesRemaining = std::max(m_bytesRemaining - static_cast<int>(bytes), 0);
	}
	
	int bytesRemainingGet () const
	{
		return std::max(0, m_bytesRemaining);
	}
	
	int targetRateGet () const
	{
		return m_targetRate;
	}
	
	void targetRateSet (const size_t& targetRate)
	{
		// Adjust the budget for the new bitrate
		m_targetRate = static_cast<int>(targetRate);
	}
	
private:
	int m_targetRate{0};
	int m_bytesRemaining{0};
};

/// There is one RtpSender per session. The RtpSender manages multiple streams (when not using RTP bundling, the streams are just the main media stream and optionally a retransmission stream). RtpSender also manages a thread which pulls packets from multiple stream queues, pacing outgoing packets on a given socket. An event queue isn't currently used because the timing of these packets is rather important, since sending them late persistently can cause the RtpSender to fall behind and drop packets.
class RtpSender
{
public:
	RtpSender(std::shared_ptr<vpe::RtpSession> rtpSession);
	~RtpSender();
	
	RtpSender(const RtpSender& other) = delete;
	RtpSender(RtpSender&& other) = delete;
	RtpSender& operator=(const RtpSender& other) = delete;
	RtpSender& operator=(RtpSender&& other) = delete;
	
	/// TODO: RtpSender was originally meant to multiplex over multiple send queues, for RTP bundling for example.
	/// It needs to be written such that users can register new RTP queues with a given priority.
	std::shared_ptr<RtpSendQueue> rtpSendQueueGet ()
	{
		return m_sendQueue;
	}
		
	RvRtpSession rtpSessionGet () const
	{
		return m_rtpSession->rtpGet();
	}
	
	void pacingBitRateSet (int bitRate)
	{
		m_mediaBudget.targetRateSet (bitRate);
	}
	
	void packetPacingUpdate ()
	{
		m_condition.notify_all();
	}
	
	std::mutex& mutexGet ()
	{
		return m_mutex;
	}
	
public:
	/// Any time a packet is attempted to be sent, this signal is triggered by RtpSender with the status of
	/// the send. Callback arguments are: the SSRC, the RTP packet (with RvRtpParam modified by Radvision),
	/// and RvRtpWrite's return value.
	CstiSignal<size_t, std::shared_ptr<RtpPacket>, RvStatus> packetSendSignal;
	
private:
	void rtpSendLoop ();
	
	// Sleeps for the pace interval, waking if the pace interval may have changed and sleeping for the new interval.
	// When this function exits, RTP sender should send a packet on ssrc.
	void packetPace (std::unique_lock<std::mutex>& lock);

private:
	
	std::shared_ptr<vpe::RtpSession> m_rtpSession;
	std::shared_ptr<RtpSendQueue> m_sendQueue;
	CstiSignalConnection m_mediaFrameCreatedSignalConnection;
	
	std::thread m_sendThread;
	bool m_stopThread {false};
	
	std::mutex m_mutex;
	std::condition_variable m_condition;
	
	/// Used to calculate the next packet send time
	std::chrono::steady_clock::time_point m_lastTransmitTime { std::chrono::seconds {0} };
	IntervalBudget m_mediaBudget {0};
	
	/// The amount of time we'll try to sleep for between sending packets
	static constexpr std::chrono::steady_clock::duration targetSleepTime {std::chrono::milliseconds{5}};
	
	/// If somehow we don't wake up from sleep for a long period of time, don't alot a huge budget for that group of packets to be sent.
	static constexpr std::chrono::steady_clock::duration maxIntervalTime {std::chrono::milliseconds{50}};
};

}
