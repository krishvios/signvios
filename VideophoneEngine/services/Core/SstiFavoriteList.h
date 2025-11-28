/*!
 * \file CstiFavoriteList.h
 * \brief Defines a list of Favorites
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#pragma once

#include "CstiFavorite.h"
#include <vector>

struct SstiFavoriteList
{
	std::vector<CstiFavorite> favorites;
	int maxCount = 0;
};

