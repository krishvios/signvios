/*!
* \file ClientAuthenticateResult.h
* \brief Contains the data received from Core for ClientAuthenticate
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 - 2019 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once
#include <string>
#include "CstiCoreResponse.h"


struct ClientAuthenticateResult
{
	std::string userId;
	std::string groupUserId;
	std::vector<CstiCoreResponse::SRingGroupInfo> ringGroupList{};
	std::string authToken;
};
