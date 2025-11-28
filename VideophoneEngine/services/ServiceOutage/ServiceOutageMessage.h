/*!
* \file ServiceOutageMessage.h
* \brief A data model of what is returned by the service outage service.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2017 - 2018 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once
#include <string>

namespace vpe
{
	enum class ServiceOutageMessageType
	{
		None,
		Error,
	};

	class ServiceOutageMessage
	{
	public:
		std::string MessageText;
		ServiceOutageMessageType MessageType;
	};
}
