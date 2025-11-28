/*!
 * \file RateHistoryManager.h
 * \brief See RateHistoryManager.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include <string>
#include <list>
#include <map>
#include <cstdint>

namespace vpe
{


class RateHistoryManager
{
public:
	RateHistoryManager ();
	RateHistoryManager (const RateHistoryManager &other) = delete;
	RateHistoryManager (RateHistoryManager &&other) = delete;
	RateHistoryManager &operator= (const RateHistoryManager &other) = delete;
	RateHistoryManager &operator= (RateHistoryManager &&other) = delete;
	~RateHistoryManager () = default;

	void saveRate (
		const std::string &ipAddress,
		uint32_t rate);

	uint32_t medianRateGet (
		const std::string &ipAddress);

private:
	bool loadFile ();
	bool loadLegacyFile ();
	bool saveFile ();

private:
	std::string m_filename;
	std::map<std::string, std::list<uint32_t>> m_rateMap;
};


} // namespace vpe
