// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiEventQueue.h"
#include <functional>


class ToFSensor : public CstiEventQueue
{
public:

	ToFSensor ();
	~ToFSensor () override;

	ToFSensor (const ToFSensor &other) = delete;
	ToFSensor (ToFSensor &&other) = delete;
	ToFSensor &operator = (const ToFSensor &other) = delete;
	ToFSensor &operator = (ToFSensor &&other) = delete;

	stiHResult initialize ();
	stiHResult startup ();

	enum class DistanceMode : uint16_t
	{
		Short = 1,
		Long = 2,
	};

	stiHResult distanceGet (const std::function<void(int)> &onSuccess);
	stiHResult distanceModeSet (DistanceMode mode);
	stiHResult timingBudgetSet (int ms);
	stiHResult interMeasurementSet (int ms);

private:

	bool timingBudgetAllowed (int ms);

	void eventDistanceGet (const std::function<void(int)> &onSuccess);

private:

	uint16_t m_deviceAddress {0x29};
	uint16_t m_distanceMode {2};
	uint16_t m_timingBudget {100};
	uint16_t m_interMeasurement {100};

	bool m_deviceInitialized {false};
};
