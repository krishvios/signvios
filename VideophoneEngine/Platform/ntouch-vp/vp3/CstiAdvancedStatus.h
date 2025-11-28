// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiAdvancedStatusBase.h"


class CstiAdvancedStatus : public CstiAdvancedStatusBase
{
public:
	
	CstiAdvancedStatus () = default;
	~CstiAdvancedStatus () override = default;

	CstiAdvancedStatus (const CstiAdvancedStatus &other) = delete;
	CstiAdvancedStatus (CstiAdvancedStatus &&other) = delete;
	CstiAdvancedStatus &operator= (const CstiAdvancedStatus &other) = delete;
	CstiAdvancedStatus &operator= (CstiAdvancedStatus &&other) = delete;

private:

	stiHResult mpuSerialSet () override;

	void staticStatusGet () override;
	void dynamicStatusGet () override;
};
