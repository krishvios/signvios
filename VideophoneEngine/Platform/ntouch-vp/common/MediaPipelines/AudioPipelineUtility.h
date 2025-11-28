/*!\brief Audio pipeline utility functions
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <string>
#include <cstdio>


namespace APUtility
{
	enum class Direction {Play, Record};

	inline std::string hardwareIdGet (Direction direction, const std::string& card)
	{
		std::string result {};
		char buffer[128];
		std::string cmd {};

		if (direction == Direction::Play)
		{
			cmd = "aplay -l | grep ";
		}
		else
		{
			cmd = "arecord -l | grep ";
		}
		cmd += card;

		FILE* pipe = popen(cmd.c_str(), "r"); // Open pipe for reading
		if (!pipe)
		{
			return {};
		}

		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
		{
			result += buffer;
		}

		pclose(pipe); // Close the pipe

		auto cardStart = result.find ("card ");
		if (cardStart != std::string::npos)
		{
			result.erase (0, cardStart + 5);
			auto cardEnd = result.find (":");
			if (cardEnd != std::string::npos)
			{
				result.replace (cardEnd, 1, ",");
				auto deviceStart = result.find ("device ");
				{
					if (deviceStart != std::string::npos)
					{
						result.erase (cardEnd + 1, deviceStart - cardEnd + 6);
						auto deviceEnd = result.find (":");
						if (deviceEnd != std::string::npos)
						{
							result.erase (result.begin () + deviceEnd, result.end ());
							result.insert (0, "hw:") ;
							return result;
						}
					}
				}
			}
		}

		return {};
	}
}

