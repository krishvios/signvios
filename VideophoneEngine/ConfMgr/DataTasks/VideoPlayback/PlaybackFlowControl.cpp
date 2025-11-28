/*!
 * \file PlaybackFlowControl.cpp
 * \brief Used to calculate new maximum receive rate.
 *        This algorithm is meant to replicate AutoSpeed in effective behavior.
 *        Not everything is identical, but the overall behavior should be very
 *        similar.  The original TMMBR algorithm was written to make use of the
 *        fact that we can get feedback very quickly when we experience packet
 *        loss.  So it was more aggressive to test the bandwidth limit.
 *        Unfortunately, we can't tolerate bad video, even for a second.  Until
 *        we support NACK, we have to be content mimicing AutoSpeed behavior,
 *        which is extremely conservative in many situations.  Once NACK is
 *        available to us, we will be able to test the bandwdith limit without
 *        causing bad video.  This is when TMMBR will really shine.
 *
 *        Autospeed behavior attempts to establish an estimated bandwidth limit.
 *        In order to establish a limit, it will attepmt to raise the rate until
 *        it experiences unacceptable loss.  If it was able to raise the rate
 *        several times and then hits packet loss, it will say it hit the limit,
 *        then set the rate to some percentage of that limit.  At this point it
 *        is will take roughly 10 minutes before it tests the limit again.  The
 *        next time it experiences packet loss, it will take 30 minutes to test
 *        the limit.  60 minutes the next time it happens.
 *
 *        A few key differences in this implementation of Autospeed is that it
 *        will respond much quicker to packet loss, due to having feedback at
 *        1 second intervals instead of 5 second intervals.  It is also written
 *        as a function of time instead of number of feedback intervals, so the
 *        interval times could theoretically change without completely changing
 *        the behavior of the algorithm.
 *
 *        Another key difference is that this only represents the receive portion
 *        of the algorithm.  The counterpart to this class is RecordFlowControl,
 *        which runs on the sending side of the RTP data stream and is tasked with
 *        utilizing as much of teh published receiver's bandwidth as it can.  It
 *        will generally start with its last known good send rate (saved to disk),
 *        and gradually increase the rate over time until it reaches the max
 *        rate.  After being there for a time, it is on PlaybackFlowControl
 *        to then raise the ceiling.
 *
 *        An average rate is calculated continuously across the most recent N samples
 *        (aka the sliding window).  Average rate starts at 0 and stays there until
 *        some percentage of the window has data to ensure a somewhat reasonable
 *        starting average.
 *
 *        We also calculate an average loss ratio in an attempt to recognize constant
 *        loss scenarios.  Autospeed uses this to prevent a bandwidth limit from
 *        being set.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 - 2018 Sorenson Communications, Inc. -- All rights reserved
 */

#include "PlaybackFlowControl.h"
#include "stiTrace.h"
#include "stiOS.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace vpe
{


// Discard stats from history that are older than N seconds (window size)
constexpr uint8_t STATS_HISTORY_WINDOW_SECONDS = 30;

// Don't perform calculations until we have built up at least this much history
constexpr uint8_t RECENT_HISTORY_WINDOW_SECONDS = 15;

static_assert(RECENT_HISTORY_WINDOW_SECONDS <= STATS_HISTORY_WINDOW_SECONDS,
              "RECENT_HISTORY_WINDOW_SECONDS must be less than or equal to STATS_HISTORY_WINDOW_SECONDS");

// Bandwidth utilization must be at or above this ratio for ceiling increases
constexpr double RATE_UTILIZATION_NEEDED_FOR_INCREASE = 0.70;

// Rate increases become smaller (and further apart) with each decrease
constexpr double RATE_INCREASE_DECAY_MULTIPLIER = 0.75;

// Don't allow the algorithm to set a rate below this
constexpr uint32_t MINIMUM_RATE_BYTES_PER_SECOND = 256000;

// We allow a certain amount of packet loss depending on rate
std::vector<std::pair<uint32_t, double>> g_acceptableLossTable = {
	{ 1024000, 0.0025 },
	{  512000, 0.0100 },
	{  256000, 0.0200 }};


/*!
 * \brief Constructor
 */
PlaybackFlowControl::PlaybackFlowControl (
	uint32_t absoluteMaxRate)
:
	m_absoluteMaxRate (absoluteMaxRate),
	m_currentMaxRate (absoluteMaxRate),
	m_estBandwidthLimit (absoluteMaxRate)
{
	auto now = std::chrono::steady_clock::now();
	m_lastRateChangeTime   = now;
	m_lastRateIncreaseTime = now;
	m_lastRateDecreaseTime = now;
	m_lastPacketLossTime   = now;
	m_lastBandwidthLimitChangeTime = now;

	stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
		stiTrace ("PlaybackFlowControl: Absolute Max Rate [absoluteMaxRate: %u]\n", absoluteMaxRate);
	);
}


/*
 * \brief Returns the current maximum rate
 */
uint32_t PlaybackFlowControl::maxRateGet () const
{
	return m_currentMaxRate;
}


/*!
 * \brief Recommends a new maximum receive data rate based on packet loss and
 *        time.  In the event of packet loss, it will immediately recommend a
 *        lower rate.  If it has been receiving data near its max receive rate
 *        for a prolonged period of time without loss, then it will recommend
 *        in increased maximum rate.
 */
uint32_t PlaybackFlowControl::maxRateCalculate (
	uint32_t durationMs,
	uint32_t packetsReceived,
	uint32_t packetsLost,
	uint32_t packetSizeAvg,
	uint32_t actualPacketLoss)
{
	statsHistoryUpdate (
		durationMs,
		packetsReceived,
		packetsLost,
		packetSizeAvg,
		actualPacketLoss);

	uint32_t newMaxRate = m_currentMaxRate;
	auto packetsTotal = packetsReceived + packetsLost;

	// If there's no new data to process, just return the current max rate
	if (packetsTotal == 0)
	{
		return m_currentMaxRate;
	}
	
	// Adjust the Autospeed bandwidth limit if necessary
	auto bandwidthLimit = bandwidthLimitCalculate ();
	if (bandwidthLimit != m_estBandwidthLimit)
	{
		if (bandwidthLimit < m_estBandwidthLimit)
		{
			m_bandwidthLimitDecreaseCount++;
		}

		m_estBandwidthLimit = bandwidthLimit;
		m_lastBandwidthLimitChangeTime = std::chrono::steady_clock::now();
		newMaxRate = std::min (m_estBandwidthLimit, newMaxRate);

	}

	// Check if a rate decrease is needed
	if (rateDecreaseCheck())
	{
		newMaxRate = rateDecreaseCalculate ();
		if (newMaxRate != m_currentMaxRate &&
			newMaxRate < m_currentMaxRate)
		{
			++m_rateDecreaseCount;
			auto now = std::chrono::steady_clock::now();
			m_lastRateChangeTime = now;
			m_lastRateDecreaseTime = now;
			m_lastPacketLossTime = now;
		}
	}
	// Check if a rate increase is needed
	else if (rateIncreaseCheck())
	{
		newMaxRate = rateIncreaseCalculate ();
		if (newMaxRate != m_currentMaxRate &&
			newMaxRate > m_currentMaxRate)
		{
			++m_rateIncreaseCount;
			m_lastRateChangeTime = std::chrono::steady_clock::now();
			m_lastRateIncreaseTime = m_lastRateChangeTime;
		}
	}

	// Set the new maxrate internally
	m_currentMaxRate = newMaxRate;

	return newMaxRate;
}


/*!
 * \brief Returns true if a rate decrease is needed
 *        Will return false if either condition is met:
 *        - The amount of packet loss is less than the acceptable threshold for the given rate
 *        - We decreased the rate too recently (so as to give it some time to take effect)
 */
bool PlaybackFlowControl::rateDecreaseCheck () const
{
	const auto &rs = m_receiveStatsHistory.front();
	uint32_t observedRate = rs.observedRate;
	double lossPercentage = lossPercentageCalculate (rs);

	// Obviously don't need a rate decrease if we're not experiencing packet loss
	if (lossPercentage <= acceptableLossAtRate(observedRate))
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();

	// Did we decrease the rate too recently?  If so, we shouldn't do it again until
	// we've given it enough time to take effect
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastRateDecreaseTime);
	const uint32_t minSecBetweenDecreases = 2;

	return (duration.count() >= minSecBetweenDecreases);
}


/*!
 * \brief Calculates a lower max rate based on max rate, observed rate and
 *        packet loss.  If we lost 10% of packets, the rate will be decreased
 *        by 10% of the current max rate.  If the decreased rate is greater
 *        than the last observed rate, then we lower it further to that rate.
 *        Additionally, the smallest decrease is limited to 5%, while the
 *        biggest drop is limited to 40% of the current rate.
 */
uint32_t PlaybackFlowControl::rateDecreaseCalculate () const
{
	constexpr double maxMultiplier = 0.95; // 5% decrease
	constexpr double minMultiplier = 0.60; // 40% decrease

	const auto &rs        = m_receiveStatsHistory.front ();
	double lossPercentage = lossPercentageCalculate (rs);
	double rateMultiplier = std::min(maxMultiplier, std::max(minMultiplier, 1.0 - lossPercentage));
	uint32_t loweredRate  = std::min(m_estBandwidthLimit, (uint32_t)(rs.observedRate * rateMultiplier));
	uint32_t newMaxRate   = std::max(MINIMUM_RATE_BYTES_PER_SECOND, loweredRate);

	auto recentAverageRate = averageRateCalculate (RECENT_HISTORY_WINDOW_SECONDS);

	stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
		stiTrace ("PlaybackFlowControl: Max Receive Rate Decrease [newMaxRate: %u, oldMaxRate: %u, averageRate: %u, recentAverageRate: %u, multiplier: %1.5f]\n",
				  newMaxRate, m_currentMaxRate, m_averageRate, recentAverageRate, rateMultiplier);
	);
	return newMaxRate;
}


/*!
 * \brief Checks the history and determines if a max rate increase is appropriate
 *        This check was modified to mimic the Autospeed algorithm
 *
 *    The following criteria determine whether or not we can increase the rate:
 *        1) Our history timeframe is populated with data
 *        2) Enough time has elapsed since last increase/decrease
 *        3) We've been receiving data near our max rate for all recent history
 *
 * \note Variable bitrates can be problematic here, since they aren't likely to
 *       consistently utilize a high percentage of our published bandwidth.
 */
bool PlaybackFlowControl::rateIncreaseCheck () const
{
	// Make sure we have established history
	uint32_t durationMs = std::accumulate (
		m_receiveStatsHistory.begin(), m_receiveStatsHistory.end(), 0,
		[](uint32_t sum, const ReceiveStats &rs){return sum + rs.durationMs;});

	uint32_t historyDurationSec = durationMs / MSEC_PER_SEC;
	bool alreadyAtBandwidthLimit = m_currentMaxRate >= m_estBandwidthLimit;

	if (alreadyAtBandwidthLimit || historyDurationSec < RECENT_HISTORY_WINDOW_SECONDS)
	{
		return false;
	}

	// Ensure we've been receiving data at a rate that is near our our current published max rate
	// No sense in raising the max rate if we're not receiving data close to our current max rate
	auto  rateThreshold = static_cast<uint32_t>(m_currentMaxRate * RATE_UTILIZATION_NEEDED_FOR_INCREASE);
	auto recentAverageRate = averageRateCalculate (RECENT_HISTORY_WINDOW_SECONDS);
	const auto &rs = m_receiveStatsHistory.front();
	if (recentAverageRate < rateThreshold || rs.observedRate < rateThreshold)
	{
		stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
					  stiTrace ("PlaybackFlowControl: (observedRate %u || recentAverageRate %u) < rateThreshold %u - utilization criteria not met for rate increase\n",
								rs.observedRate, recentAverageRate, rateThreshold); );
		return false;
	}
	
	// Don't attempt raising the rate if we are concealing an amount of packet loss above what is acceptable.
	auto actualLossPercentage = actualLossPercentageCalculate (rs);
	if (actualLossPercentage > acceptableLossAtRate(rs.observedRate))
	{
		return false;
	}

	// Allow increases after every 10 seconds of no loss up to the bandwidth limit.
	// The bandwidth limit increases/decreases independently of the current maxrate
	auto now = std::chrono::steady_clock::now ();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastRateChangeTime);
	const uint32_t secondsBetweenRateIncreases = 10;

	return duration.count() >= secondsBetweenRateIncreases;
}


/*!
 * \brief Calculates a higher current maxrate, the magnitude of which depends on
 *        how many times we've had to decrease the rate
 */
uint32_t PlaybackFlowControl::rateIncreaseCalculate () const
{
	constexpr double minMultiplier = 1.02;
	constexpr double scalar = 1.08;
	auto multiplier = std::max (minMultiplier, scalar * ::pow(RATE_INCREASE_DECAY_MULTIPLIER, m_bandwidthLimitDecreaseCount));
	uint32_t newMaxRate = std::min (m_estBandwidthLimit, static_cast<uint32_t>(m_currentMaxRate * multiplier));

	auto recentAverageRate = averageRateCalculate (RECENT_HISTORY_WINDOW_SECONDS);
	stiUNUSED_ARG(recentAverageRate);

	stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
		stiTrace ("PlaybackFlowControl: Max Receive Rate Increase [new: %u, old: %u, avg: %u, recentAvg: %u, multiplier: %1.5f]\n",
				  newMaxRate, m_currentMaxRate, m_averageRate, recentAverageRate, multiplier);
	);

	return newMaxRate;
}

/*!
 * \brief Add stats to history and purge any that are older
 *        than the specified time limit
 */
void PlaybackFlowControl::statsHistoryUpdate (
	uint32_t durationMs,
	uint32_t packetsReceived,
	uint32_t packetsLost,
	uint32_t packetSizeAvg,
	uint32_t actualPacketLoss)
{
	// If the duration is zero, do nothing
	if (durationMs == 0)
	{
		return;
	}

	// Calculate the observed data rate for this period
	double durationSec = durationMs / (double)MSEC_PER_SEC;
	uint32_t packetSizeAvgBits = packetSizeAvg * 8;
	auto  observedRate = static_cast<uint32_t>(packetsReceived * packetSizeAvgBits / durationSec);

	stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
	    stiTrace ("PlaybackFlowControl: New History [rate: %u, rcvd: %u, lost: %u, avgSize: %u, durationMs: %u]\n",
	              observedRate, packetsReceived, packetsLost, packetSizeAvg, durationMs);
	);

	// Add stats to history
	m_receiveStatsHistory.emplace_front (
		durationMs, packetsReceived, packetsLost, packetSizeAvg, observedRate, actualPacketLoss);

	// Only keep stats that were received within the last N seconds (window)
	const uint32_t timeThresholdMs = STATS_HISTORY_WINDOW_SECONDS * MSEC_PER_SEC;
	uint32_t totalDurationMs = 0;

	// Find the first entry that is too old to keep around
	auto itr = m_receiveStatsHistory.begin ();
	for (; itr != m_receiveStatsHistory.end(); itr++)
	{
		totalDurationMs += itr->durationMs;
		if (totalDurationMs > timeThresholdMs)
		{
			break;
		}
	}

	// Remove that entry and any that are older than it (if any)
	if (itr != m_receiveStatsHistory.end())
	{
		m_receiveStatsHistory.erase (itr, m_receiveStatsHistory.end());
	}

	// Recalculate average rate and average loss ratio, but only if we have enough history
	const uint32_t totalDurationSec = totalDurationMs / MSEC_PER_SEC;
	if (totalDurationSec >= RECENT_HISTORY_WINDOW_SECONDS)
	{
		m_averageRate = averageRateCalculate (totalDurationSec);

		// Limit fraction lost to 5% to help detect constant loss (TODO: use median instead?)
		double fractionLostSum = std::accumulate (
			m_receiveStatsHistory.begin(), m_receiveStatsHistory.end(), 0,
			[this](uint32_t sum, const ReceiveStats &rs){ return sum + static_cast<uint32_t>(std::min(0.05, lossPercentageCalculate(rs))); });

		if (!m_receiveStatsHistory.empty ())
		{
			m_averageLossRatio = fractionLostSum / m_receiveStatsHistory.size();
		}
	}
}


/*!
 * \brief Finds the acceptable loss percentage based on observed rate
 */
double PlaybackFlowControl::acceptableLossAtRate (
	uint32_t observedRate) const
{
	auto itr = g_acceptableLossTable.rbegin();
	auto lowerItr = itr;

	for (; itr != g_acceptableLossTable.rend(); itr++)
	{
		if (observedRate <= itr->first)
		{
			break;
		}
		lowerItr = itr;
	}

	// If the rate is higher than the top of the table, just return the highest entry's acceptable loss value
	if (itr == g_acceptableLossTable.rend())
	{
		return g_acceptableLossTable.front().second;
	}
	// If the rate is at or below the bottom of the table, just return the lowest entry's acceptable loss value

	if (itr == g_acceptableLossTable.rbegin())
	{
		return itr->second;
	}

	// Otherwise, interpolate between the two straddling rates in the table
	{
		double rate0 = lowerItr->first;
		double rate1 = itr->first;
		double ratio = (observedRate - rate0) / (rate1 - rate0);
		double loss0 = itr->second;
		double loss1 = lowerItr->second;

		return loss0 + ratio * (loss1 - loss0);
	}
}


/*!
 * \brief Calculates the packet loss percentage before repair and RTX from a ReceiveStats entry
 */
double PlaybackFlowControl::actualLossPercentageCalculate (
	const ReceiveStats &rs) const
{
	auto totalPackets = rs.packetsReceived + rs.packetsLost;
	if (totalPackets == 0)
	{
		return 0;
	}
	
	return rs.actualPacketLoss / static_cast<double>(totalPackets);
}


/*!
 * \brief Calculates the packet loss percentage from a ReceiveStats entry
 */
double PlaybackFlowControl::lossPercentageCalculate (
    const ReceiveStats &rs) const
{
	auto totalPackets = rs.packetsReceived + rs.packetsLost;
	if (totalPackets == 0)
	{
		return 0;
	}

	return rs.packetsLost / static_cast<double>(totalPackets);
}


/*!
 * \brief Calculates and returns a bandwidth limit according to Autospeed rules
 */
uint32_t PlaybackFlowControl::bandwidthLimitCalculate () const
{
	uint32_t bandwidthLimit = m_estBandwidthLimit;

	// Return initial bandwidth limit if we have no history
	if (m_receiveStatsHistory.empty())
	{
		return bandwidthLimit;
	}

	const auto &rs = m_receiveStatsHistory.front();
	auto recentAverageRate = averageRateCalculate (RECENT_HISTORY_WINDOW_SECONDS);
	auto lossPercentage = lossPercentageCalculate (rs);
	auto actualLossPercentage = actualLossPercentageCalculate (rs);
	auto acceptableLossPercentage = acceptableLossAtRate(rs.observedRate);

	// If no losses this time including losses we repair, we can consider raising the bandwidth limit
	if ((lossPercentage <= acceptableLossPercentage) &&
		(actualLossPercentage <= acceptableLossPercentage))
	{
		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastBandwidthLimitChangeTime);
		const uint32_t minSecBetweenIncreases = 60;

		// Has it been long enough since we last changed the bandwidth limit,
		// and is rate utilization high enough to justify an increase?
		if (duration.count() >= minSecBetweenIncreases &&
			recentAverageRate >= m_estBandwidthLimit * RATE_UTILIZATION_NEEDED_FOR_INCREASE)
		{
			std::vector<float> multipliers = { 1.0226F, 1.0226F, 1.0226F, 1.0075F, 1.00375F };  // 2.26%, .75%, .375% increases
			auto index = std::min (m_bandwidthLimitDecreaseCount, static_cast<uint16_t>(multipliers.size()-1));
			auto multiplier = multipliers[index];
			bandwidthLimit = std::min (m_absoluteMaxRate, static_cast<uint32_t>(m_estBandwidthLimit * multiplier));
		}
	}
	// We may need a lower bandwidth limit
	else if (lossPercentage > acceptableLossPercentage)
	{
		double totalDurationMs = std::accumulate (
			m_receiveStatsHistory.begin(), m_receiveStatsHistory.end(), 0,
			[](uint32_t totalMs, const ReceiveStats &rs){ return totalMs + rs.durationMs; });

		// We only want to set a new bandwidth limit if we have enough history
		if (totalDurationMs >= RECENT_HISTORY_WINDOW_SECONDS * MSEC_PER_SEC)
		{
			auto lossPercentage = lossPercentageCalculate(rs);

			// Ensure this loss was due to a rate climb, and not random or constant loss
			// Scalar values help to ensure significance and prevent false positives
			if (rs.observedRate * 1.15 > recentAverageRate &&
				lossPercentage > m_averageLossRatio * 1.25)
			{
				constexpr double bandwidthFraction = 0.80;  // 80% of limit
				bandwidthLimit = std::max(MINIMUM_RATE_BYTES_PER_SECOND, static_cast<uint32_t>(recentAverageRate * bandwidthFraction));
			}
		}
	}

	if (bandwidthLimit != m_estBandwidthLimit)
	{
		stiDEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug,
			stiTrace ("PlaybackFlowControl: Bandwidth Limit Changed [new: %u, old: %u, recentAverageRate: %u]\n",
					  bandwidthLimit, m_estBandwidthLimit, recentAverageRate);
		);
	}

	return bandwidthLimit;
}


/*!
 * \brief Calculates the average rate for a given slice of the history
 */
uint32_t PlaybackFlowControl::averageRateCalculate (
	uint32_t windowSizeSeconds) const
{
	const uint32_t windowSizeMs = windowSizeSeconds * MSEC_PER_SEC;
	uint64_t rateSum = 0;
	uint32_t durationMs = 0;
	uint32_t count = 0;

	for (const auto &rs : m_receiveStatsHistory)
	{
		++count;
		rateSum += rs.observedRate;
		durationMs += rs.durationMs;
		if (durationMs >= windowSizeMs)
		{
			break;
		}
	}

	return count ? static_cast<uint32_t>(rateSum / count) : 0;
}


/*!
 * \brief Returns true if any unacceptable losses were experienced within the
 *        last N seconds, excluding the most recent entry
 */
bool PlaybackFlowControl::unacceptableLossesLocate (
	uint32_t windowSizeSeconds) const
{
	const uint32_t windowSizeMs = windowSizeSeconds * MSEC_PER_SEC;
	uint32_t totalDurationMs = 0;
	bool lossesFound = false;

	auto itr = m_receiveStatsHistory.begin();
	for (; itr != m_receiveStatsHistory.end(); itr++)
	{
		// skip the most recent entry
		if (itr == m_receiveStatsHistory.begin())
		{
			continue;
		}

		const auto &rs = *itr;
		totalDurationMs += rs.durationMs;
		auto lossPercentage = lossPercentageCalculate (rs);
		auto acceptableLossThreshold = acceptableLossAtRate (rs.observedRate);
		if (lossPercentage > acceptableLossThreshold)
		{
			lossesFound = true;
			break;
		}

		if (totalDurationMs > windowSizeMs)
		{
			break;
		}
	}

	return lossesFound;
}



} // namespace vpe
