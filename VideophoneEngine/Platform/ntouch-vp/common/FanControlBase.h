// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IFanControl.h"


class FanControlBase : public IFanControl
{
public:

	FanControlBase () = default;
	~FanControlBase () override = default;

	FanControlBase (const FanControlBase &other) = delete;
	FanControlBase (FanControlBase &&other) = delete;
	FanControlBase &operator= (const FanControlBase &other) = delete;
	FanControlBase &operator= (FanControlBase &&other) = delete;

	void inCallConfigSet (bool inCall) override;
	int dutyCycleGet () override;
	stiHResult dutyCycleSet (int dutyCycle) override;

private:

};
