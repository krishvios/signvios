/*!
* \brief Used to collect time durations and counts for performance metrics
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include <string>
#include <map>
#include <chrono>
#include <mutex>

class TimeAccumulator
{
public:

	TimeAccumulator (
		const std::string &name);

	~TimeAccumulator () = default;

	TimeAccumulator (const TimeAccumulator &other) = delete;
	TimeAccumulator (TimeAccumulator &&other) = delete;
	TimeAccumulator &operator = (const TimeAccumulator &other) = delete;
	TimeAccumulator &operator = (TimeAccumulator &&other) = delete;

	void timeStart (const std::string &bucket);
	void timeStop ();

	void increment (const std::string &bucket);

	struct Stats
	{
		std::string name;
		std::map<std::string, std::chrono::microseconds> timeAccumulators;
		std::map<std::string, int> counters;
	};

	Stats statsGet ();

private:

	std::string m_name;
	std::string m_bucket;
	std::mutex m_timeLock;
	std::chrono::steady_clock::time_point m_timeStart;

	std::map<std::string, std::chrono::microseconds> m_timeAccumulators;
	std::map<std::string, int> m_counters;
};
