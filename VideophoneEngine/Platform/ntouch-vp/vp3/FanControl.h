// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020-2021 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "FanControlBase.h"


class FanControl : public FanControlBase
{
public:

	FanControl () = default;
	~FanControl () override = default;

	FanControl (const FanControl &other) = delete;
	FanControl (FanControl &&other) = delete;
	FanControl &operator= (const FanControl &other) = delete;
	FanControl &operator= (FanControl &&other) = delete;

	int dutyCycleGet () override;
	stiHResult dutyCycleSet (int dutyCycle) override;

	void testModeSet (bool testMode);

private:

	void thermaldStart ();
	void thermaldStop ();

private:

	bool m_testMode {false};
};
