/*!
* \file Address.h
* \brief Data to represent an address
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 - 2019 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once
#include <string>

namespace vpe
{
	struct Address
	{
		std::string street;
		std::string apartmentNumber;
		std::string city;
		std::string state;
		std::string zipCode;
	};
}
