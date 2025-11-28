// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiError.h"


class IFanControl
{
public:

	IFanControl (const IFanControl &other) = delete;
	IFanControl (IFanControl &&other) = delete;
	IFanControl &operator= (const IFanControl &other) = delete;
	IFanControl &operator= (IFanControl &&other) = delete;

	static IFanControl* InstanceGet ();

	virtual void inCallConfigSet (bool inCall) = 0;
	virtual int dutyCycleGet () = 0;
	virtual stiHResult dutyCycleSet (int dutyCycle) = 0;

protected:

	IFanControl () = default;
	virtual ~IFanControl () = default;
};
