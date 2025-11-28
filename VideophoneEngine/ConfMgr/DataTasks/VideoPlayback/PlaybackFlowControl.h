/*!
 * \file PlaybackFlowControl.h
 * \brief See PlaybackFlowControl.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <deque>
#include <map>

namespace vpe
{


class PlaybackFlowControl
{
	struct ReceiveStats
	{
		ReceiveStats (
			uint32_t durationMs0,
			uint32_t packetsReceived0,
			uint32_t packetsLost0,
			uint32_t packetSizeAvg0,
			uint32_t observedRate0,
			uint32_t actualPacketLoss0)
		:
			durationMs (durationMs0),
			packetsReceived (packetsReceived0),
			packetsLost (packetsLost0),
			packetSizeAvg (packetSizeAvg0),
			observedRate (observedRate0),
			actualPacketLoss (actualPacketLoss0)
		{
		}

		uint32_t durationMs = 0;
		uint32_t packetsReceived = 0;
		uint32_t packetsLost = 0;
		uint32_t packetSizeAvg = 0;
		uint32_t observedRate = 0;
		uint32_t actualPacketLoss = 0;
	};

public:
	PlaybackFlowControl (
		uint32_t absoluteMaxRate);

	PlaybackFlowControl (const PlaybackFlowControl &other) = delete;
	PlaybackFlowControl (PlaybackFlowControl &&other) = delete;
	PlaybackFlowControl &operator= (const PlaybackFlowControl &other) = delete;
	PlaybackFlowControl &operator= (PlaybackFlowControl &&other) = delete;
	~PlaybackFlowControl () = default;

	uint32_t maxRateCalculate (
		uint32_t durationMs,       // How much time this period spanned
		uint32_t packetsReceived,  // Num packets received during this period
		uint32_t packetsLost,      // Num packets lost during this period
		uint32_t packetSizeAvg,   // Avg packet size during this period
		uint32_t actualPacketLoss); // Actual packets lost before NACK and RTX

	uint32_t maxRateGet () const;

private:
	void statsHistoryUpdate (
		uint32_t durationMs,
		uint32_t packetsReceived,
		uint32_t packetsLost,
		uint32_t packetSizeAvg,
		uint32_t actualPacketLoss);

	bool rateIncreaseCheck () const;
	bool rateDecreaseCheck () const;

	uint32_t rateIncreaseCalculate () const;
	uint32_t rateDecreaseCalculate () const;

	double acceptableLossAtRate (
		uint32_t observedRate) const;

	double lossPercentageCalculate (const ReceiveStats &rs) const;
	double actualLossPercentageCalculate (const ReceiveStats &rs) const;

	uint32_t bandwidthLimitCalculate () const;
	uint32_t averageRateCalculate (uint32_t windowSizeSeconds) const;
	bool unacceptableLossesLocate (uint32_t windowSizeSeconds) const;

private:
	std::deque<ReceiveStats> m_receiveStatsHistory;
	uint32_t m_absoluteMaxRate = 0;
	uint32_t m_currentMaxRate = 0;
	uint32_t m_averageRate = 0;          // Average rate for entire window
	double m_averageLossRatio = 0.0;     // Average fraction of lost packets for window (limited to 5%)

	uint16_t m_rateDecreaseCount = 0;    // Number of times we've decreased the maximum rate
	uint16_t m_rateIncreaseCount = 0;    // Number of times we've increased the maximum rate

	uint32_t m_estBandwidthLimit = 0;    // Estimate of the available bandwidth
	uint16_t m_bandwidthLimitDecreaseCount = 0;

	// The last time when the rate either increased or decreased
	std::chrono::steady_clock::time_point m_lastRateChangeTime;

	// The last time when the rate was increased.
	std::chrono::steady_clock::time_point m_lastRateIncreaseTime;

	// The last time when the rate was decreased.  This is to prevent decreasing the rate multiple
	// times in rapid succession.  We need to give a little time for the first decrease to take effect.
	std::chrono::steady_clock::time_point m_lastRateDecreaseTime;

	// The last time the bandwidth limit changed
	std::chrono::steady_clock::time_point m_lastBandwidthLimitChangeTime;

	// The last time when packet loss ocurred.  This can be different that lastRateDecreaseTime when
	// the rate has bottomed out
	std::chrono::steady_clock::time_point m_lastPacketLossTime;
};


} // namespace vpe
