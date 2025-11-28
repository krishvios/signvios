/*!
 * \file RecordFlowControl.h
 * \brief See RecordFlowControl.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "RateHistoryManager.h"
#include <string>
#include <chrono>

namespace vpe
{


class RecordFlowControl
{
public:
	RecordFlowControl (
		const std::string &ipAddress,
		uint32_t initialMaxRate);

	RecordFlowControl (const RecordFlowControl &other) = delete;
	RecordFlowControl (RecordFlowControl &&other) = delete;
	RecordFlowControl &operator= (const RecordFlowControl &other) = delete;
	RecordFlowControl &operator= (RecordFlowControl &&other) = delete;

	~RecordFlowControl ();

	void maxRateSet (
	    uint32_t maxRate);

	uint32_t maxRateGet () const;

	uint32_t minRateGet () const;

	uint32_t sendRateGet () const;

	uint32_t sendRateCalculate ();

	uint32_t avgRateGet () const;

	void initialSendRateSet (uint32_t rate);
	
	void reservedBitRateSet (uint32_t reservedBitRate);

private:
	uint32_t initialSendRateGet ();

private:
	uint32_t m_currentMinRate = 0;
	uint32_t m_currentMaxRate = 0;
	uint32_t m_currentSendRate = 0;
	uint32_t m_reservedBitRate = 0;
	std::string m_ipAddress;

	// Used to calculate a running weighted average rate
	uint64_t m_rateAccumulator = 0;
	uint32_t m_timeAccumulatorSec = 0;

	std::chrono::steady_clock::time_point m_lastRateChangeTime;
	RateHistoryManager m_rateHistoryManager;

};


} // namespace vpe
