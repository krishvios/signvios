// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiNetwork.h
 * \brief See CstiNetwork.cpp
 */

#pragma once

#include "CstiNetworkBase.h"

class CstiNetwork : public CstiNetworkBase
{
public:
	CstiNetwork(CstiBluetooth *bluetooth);

	~CstiNetwork() override = default;

	stiHResult Initialize();
};
