// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiCECControl.h
 * \brief See CstiCECControl.cpp
 */

#pragma once

#include "stiDefs.h"

class CstiCECControl 
{
public:
	CstiCECControl() = default;

	CstiCECControl(const CstiCECControl &other) = delete;
	CstiCECControl(CstiCECControl &&other) = delete;
	CstiCECControl &operator= (const CstiCECControl &other) = delete;
	CstiCECControl &operator= (CstiCECControl &&other) = delete;

	~CstiCECControl() = default;

	stiHResult Initialize(CstiMonitorTask *, const std::string, const std::string)
	{
		return stiRESULT_SUCCESS;
	}

	stiHResult Startup()
	{
		return stiRESULT_SUCCESS;
	}
};

