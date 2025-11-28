/*!
 * \file RecordFlowControl.cpp
 * \brief Used to calculate new send rate, up to the currently published
 *        maximum rate.  This class is simpler than it's counterpart
 *        (PlaybackFlowControl), since it only has to deal with increasing
 *        its current send rate up toward the maximum rate.  Indeed its
 *        goal is to send as close to the maximum rate as it can.  It
 *        does not have to deal directly with packet loss since a TMMBR
 *        request will immediately lower the max rate if any occurs.
 *
 *        The sender does have to approach the max rate with some caution,
 *        because networks can get saturated, preventing TMMBR requests from
 *        arriving.  For example, the receiver says it can receive 5 MBits,
 *        but the sender's network can only handle 512 KB.  To combat this
 *        problem, we save off the last few known good send rates to disk,
 *        and use the median of them as a starting point for sending data.
 *        This will charactarize the sender's network and give us a safe
 *        starting point from which to start climbing.
 *
 *        Generally, we want to increase the rate frequently by small
 *        amounts so that any packet loss that might occur is minimal.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "RecordFlowControl.h"
#include "stiTrace.h"
#include <algorithm>

namespace vpe
{

// When increasing the send rate, increase by this percentage of the current send rate
constexpr float RATE_INCREASE_PERCENTAGE = 0.016F;  // 1.6% per 2 sec  (Autospeed is 8% per 10 sec)

// No matter how many times we call sendRateCalculate(), it will never
// increase the rate more frequently than this many seconds
constexpr uint32_t MIN_SEND_RATE_INCREASE_INTERVAL_SEC = 2;

// Rate increases are never smaller than this
constexpr uint32_t MIN_SEND_RATE_INCREASE_BITS_PER_SEC = 8000;

// If we don't have any rate history saved to disk, then start at this speed
// rather than the published maximum
constexpr uint32_t MIN_STARTING_SEND_RATE_BITS_PER_SEC = 512000;


/*!
 * \brief Constructor
 */
RecordFlowControl::RecordFlowControl (
	const std::string &ipAddress,
	uint32_t initialMaxRate)
:
	m_currentMaxRate (initialMaxRate),
	m_ipAddress (ipAddress)
{
	m_lastRateChangeTime = std::chrono::steady_clock::now();

	stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
		stiTrace ("RecordFlowControl: Initial Max Rate [initialMaxRate: %u])\n", initialMaxRate);
	);
}


/*!
 * \brief Destructor - save off current send rate to history file,
 *        but only if it was used
 */
RecordFlowControl::~RecordFlowControl ()
{
	// Save the send rate to our rate history if it was ever initialized
	if (m_currentSendRate > 0)
	{
		m_rateHistoryManager.saveRate (m_ipAddress, m_currentSendRate);

		stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
			stiTrace ("RecordFlowControl: Storing Rate History [ipAddress: %s,  currentSendRate: %u])\n",
			          m_ipAddress.c_str(), m_currentSendRate);
		);
	}
}


/*!
 * \brief Sets a new maximum rate.  This usually happens when a TMMBR request
 *        is received to slow things down.
 */
void RecordFlowControl::maxRateSet (
	uint32_t maxRate)
{
	m_currentMaxRate = maxRate;
	m_lastRateChangeTime = std::chrono::steady_clock::now();

	stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
		stiTrace ("RecordFlowControl: Setting New Max Rate [maxRate: %u]\n", maxRate);
	);

	// Update the min rate if the new max is lower
	m_currentMinRate = std::min (m_currentMinRate, maxRate);
}


/*!
 * \brief Returns the current max rate
 */
uint32_t RecordFlowControl::maxRateGet () const
{
	return m_currentMaxRate;
}


/*!
 * \brief Returns the min send rate so far
 */
uint32_t RecordFlowControl::minRateGet () const
{
	return m_currentMinRate;
}


/*!
 * \brief Returns the current send rate
 */
uint32_t RecordFlowControl::sendRateGet () const
{
	return m_currentSendRate;
}


/*!
 * \brief Returns an average rate since the beginning
 */
uint32_t RecordFlowControl::avgRateGet () const
{
	auto timeNow = std::chrono::steady_clock::now();
	auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(timeNow - m_lastRateChangeTime);

	// The rate total is the addition of each chunk of time multiplied by the rate during that time
	uint64_t rateTotal = m_rateAccumulator + (m_currentSendRate * elapsedSec.count());
	auto  timeTotal = static_cast<uint32_t>(m_timeAccumulatorSec + elapsedSec.count());
	uint32_t avgRate = 0;

	if (rateTotal > 0 && timeTotal > 0)
	{
		avgRate = static_cast<uint32_t>(rateTotal / timeTotal);
	}

	return avgRate;
}


/*!
 * \brief Recommends a new send rate, up to the current maximum
 *        This method should be called at regular and frequent
 *        intervals to determine if an increase in send rate is
 *        appropriate.
 */
uint32_t RecordFlowControl::sendRateCalculate ()
{
	// If our send rate has not been initialized, initialize it now
	if (m_currentSendRate == 0)
	{
		m_currentSendRate = initialSendRateGet ();
		m_currentMinRate = m_currentSendRate;
		return m_currentSendRate;
	}
	
	auto maxRate = m_currentMaxRate;
	
	if (maxRate > m_reservedBitRate)
	{
		maxRate -= m_reservedBitRate;
	}
	else
	{
			stiASSERTMSG (false,
						  "sendRateCalculate data rate to reserve = %u is higher than current max rate = %u",
						  m_reservedBitRate, maxRate);
		
			reservedBitRateSet (0); // Zero out reserved bitrate to reduce logging of assert.
	}

	// Are we already sending at the maximum rate?
	if (m_currentSendRate >= maxRate)
	{
		m_currentSendRate = maxRate;
		return maxRate;
	}

	// Did we change the rate too recently?
	auto timeNow = std::chrono::steady_clock::now();
	auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(timeNow - m_lastRateChangeTime);

	if (elapsedSec.count() < MIN_SEND_RATE_INCREASE_INTERVAL_SEC)
	{
		return m_currentSendRate;
	}

	// Increase the rate towards the current maximum
	uint32_t deltaRate = std::max (MIN_SEND_RATE_INCREASE_BITS_PER_SEC, (uint32_t)(m_currentSendRate * RATE_INCREASE_PERCENTAGE));
	uint32_t newSendRate = std::min (maxRate, m_currentSendRate + deltaRate);

	stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
	    stiTrace ("RecordFlowControl: Increasing Send Rate [currentSendRate: %u,  newSendRate: %u,  deltaRate: %u,  targetRate: %u)\n",
	            m_currentSendRate, newSendRate, newSendRate - m_currentSendRate, maxRate);
	);

	// Keep track of rate over time so we can calculate an average rate on demand
	m_rateAccumulator += m_currentSendRate * elapsedSec.count();
	m_timeAccumulatorSec += static_cast<uint32_t>(elapsedSec.count());

	m_currentSendRate = newSendRate;
	m_lastRateChangeTime = timeNow;

	return newSendRate;
}


/*!
 * \brief Returns an initial send rate.  If we have rate history, we use the
 *        median of that history, otherwise use a hard-coded initial rate
 */
uint32_t RecordFlowControl::initialSendRateGet ()
{
	uint32_t initialRate = MIN_STARTING_SEND_RATE_BITS_PER_SEC;

	// If we have rate history, use the median of the stored rates, unless
	// that value is less than the min starting rate.
	auto medianRate = m_rateHistoryManager.medianRateGet (m_ipAddress);
	if (medianRate > 0)
	{
		initialRate = std::min (medianRate, m_currentMaxRate);
		initialRate = std::max (initialRate, std::min(MIN_STARTING_SEND_RATE_BITS_PER_SEC, m_currentMaxRate));
	}

	stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
		stiTrace ("RecordFlowControl: Initial Send Rate [initialRate: %u,  medianRate: %u,  m_currentMaxRate: %u]\n",
		          initialRate, medianRate, m_currentMaxRate);
	);

	return initialRate;
}


/*!
 * \brief Sets the initial sending rate.  If this is set prior to calling sendRateCalculate,
 *        for the first time, it will use this rate instead of looking at the rate history
 *        file to determine the starting send rate.
 *        This is necessary for interpreter and customer support phones, since we know they
 *        are on high speed networks
 */
void RecordFlowControl::initialSendRateSet (uint32_t rate)
{
	if (m_currentSendRate == 0)
	{
		m_currentSendRate = std::min (rate, m_currentMaxRate);

		stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
			stiTrace ("RecordFlowControl: Overriding Initial Send Rate [rate: %u,  m_currentMaxRate: %u]\n",
			          rate, m_currentMaxRate);
		);
	}
}
	

/*!
 * \brief Sets the bit rate to reserve that is used by other media (currently only audio).
 */
void RecordFlowControl::reservedBitRateSet (uint32_t reservedBitRate)
{
	m_reservedBitRate = reservedBitRate;
}


} // namespace vpe
