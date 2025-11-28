// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiCECControl.h
 * \brief See CstiCECControl.cpp
 */

#pragma once

#include "CstiCECControlBase.h"

class CstiCECControl : public CstiCECControlBase
{
public:
	CstiCECControl() = default;

	~CstiCECControl() override = default;

	stiHResult Initialize(CstiMonitorTask *monitorTask, const std::string &device, const std::string &name);
};

