//
//  RtpSender.cpp
//  ntouch
//
//  Created by Dan Shields on 7/2/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#include "RtpSender.h"

#define RTP_ASSERT_FREQ_SECONDS 5

namespace vpe {

constexpr std::chrono::steady_clock::duration RtpSender::targetSleepTime;
constexpr std::chrono::steady_clock::duration RtpSender::maxIntervalTime;

RtpSender::RtpSender(std::shared_ptr<vpe::RtpSession> rtpSession)
	: m_rtpSession(rtpSession)
{
	m_sendQueue = std::make_shared<RtpSendQueue>();
	m_sendThread = std::thread{std::bind(&RtpSender::rtpSendLoop, this)};
	
	m_mediaFrameCreatedSignalConnection = m_sendQueue->mediaFrameCreated.Connect([this](size_t)
	{
		// Wake up the pacing thread in case it's waiting indefinitely for more packets
		std::lock_guard<std::mutex> lk { mutexGet () };
		packetPacingUpdate();
	});
}

RtpSender::~RtpSender()
{
	std::unique_lock<std::mutex> lock { mutexGet () };
	m_stopThread = true;
	m_condition.notify_all(); // Make sure the thread hasn't stopped to wait for more packets.
	
	// Unlock so the rtpSendLoop will stop waiting on the condition and it can exit.
	lock.unlock();
	m_sendThread.join();
}

void RtpSender::packetPace (std::unique_lock<std::mutex>& lock)
{
	// Sleep for the pace interval
	auto paceInterval = std::max (targetSleepTime - (std::chrono::steady_clock::now () - m_lastTransmitTime), std::chrono::steady_clock::duration{0});
	
	std::this_thread::sleep_for(paceInterval);
	
	// Sleep until we get more packets
	if (m_sendQueue->isEmptyGet())
	{
		m_condition.wait (lock, [this]() { return !m_sendQueue->isEmptyGet() || m_stopThread; });
	}
	
	// Packets are available, start sending packets
	auto now = std::chrono::steady_clock::now();
	m_mediaBudget.budgetIncrease (std::min (now - m_lastTransmitTime, maxIntervalTime));
	m_lastTransmitTime = now;
}

void RtpSender::rtpSendLoop()
{
	std::unique_lock<std::mutex> lock { mutexGet () };
	std::chrono::steady_clock::time_point lastRtpAssertTime { std::chrono::seconds {0} };
	
	for (;;)
	{
		packetPace(lock);
		
		if (m_stopThread)
		{
			break;
		}
		
		while (m_mediaBudget.bytesRemainingGet () > 0 && !m_sendQueue->isEmptyGet())
		{
			auto packet = m_sendQueue->pop ();
			auto rtpSession = m_rtpSession->rtpGet();
			if (rtpSession)
			{
				auto rvStatus = RvRtpWriteEx(
											 rtpSession,
											 RvRtpGetSSRC(rtpSession),
											 packet->headerGet (),
											 packet->headerSizeGet () + packet->payloadSizeGet (),
											 packet->rtpParamGet ());
				auto now = std::chrono::steady_clock::now();
				packetSendSignal.Emit (RvRtpGetSSRC(rtpSession), packet, rvStatus);
				
				if (rvStatus == RV_OK)
				{
					m_mediaBudget.budgetUse (packet->headerSizeGet () + packet->payloadSizeGet ());
					m_lastTransmitTime = now;
				}
				else if (now - lastRtpAssertTime >= std::chrono::seconds (RTP_ASSERT_FREQ_SECONDS))
				{
					std::string remoteAddress;
					int remotePort = 0;
					m_rtpSession->RemoteAddressGet (&remoteAddress, &remotePort, nullptr, nullptr);
					stiASSERTMSG(false, "RvRtpWriteEx is returning an error: %d remAddress: %s port: %d", rvStatus, remoteAddress.c_str(), remotePort);
					
					lastRtpAssertTime = now;
				}
			}
			else
			{
				stiASSERTMSG(false, "No RvRtpSession available to RtpSender.");
				break;
			}
		}
	}
}

}
