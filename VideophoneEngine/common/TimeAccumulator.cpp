/*!
* \brief Used to collect time durations and counts for performance metrics
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#include "TimeAccumulator.h"


TimeAccumulator::TimeAccumulator (
	const std::string &name)
:
	m_name (name)
{
}


void TimeAccumulator::timeStart (const std::string &bucket)
{
	std::unique_lock<std::mutex> lock (m_timeLock);

	auto now = std::chrono::steady_clock::now();

	// If time is accumulating for a bucket then
	// measure the elapsed time and store it.
	if (!m_bucket.empty ())
	{
		auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(now - m_timeStart);

		m_timeAccumulators[m_bucket] += elapsedTime;
	}

	m_timeStart = now;
	m_bucket = bucket;
}


void TimeAccumulator::timeStop ()
{
	std::unique_lock<std::mutex> lock (m_timeLock);

	// If time is accumulating for a bucket then
	// measure the elapsed time and store it.
	if (!m_bucket.empty ())
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(now - m_timeStart);

		m_timeAccumulators[m_bucket] += elapsedTime;

		m_bucket.clear ();
	}
}

void TimeAccumulator::increment (
	const std::string &bucket)
{
	++m_counters[bucket];
}


TimeAccumulator::Stats TimeAccumulator::statsGet ()
{
	Stats stats;

	std::unique_lock<std::mutex> lock (m_timeLock);

	stats.name = m_name;
	stats.timeAccumulators = m_timeAccumulators;
	stats.counters = m_counters;

	// If time is accumulating for a bucket then
	// measure the current elapsed time, store it and
	// set a new start time.
	if (!m_bucket.empty ())
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(now - m_timeStart);

		m_timeAccumulators[m_bucket] += elapsedTime;
		m_timeStart = now;
	}

	return stats;
}


