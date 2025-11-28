// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "ToFSensor.h"
#include "stiAssert.h"
#include "stiTrace.h"
#include "stmvl53l1-uld/VL53L1X_api.h"



ToFSensor::ToFSensor ()
:
	CstiEventQueue ("ToFSensor")
{	
}


ToFSensor::~ToFSensor ()
{
	CstiEventQueue::StopEventLoop ();
	
	VL53L1X_StopRanging (m_deviceAddress);
}


stiHResult ToFSensor::initialize ()
{
	uint8_t bootState {0};

	if (VL53L1X_BootState (m_deviceAddress, &bootState) == 0)
	{
		if (bootState > 0 && VL53L1X_SensorInit (m_deviceAddress) == 0)
		{
			VL53L1X_SetDistanceMode (m_deviceAddress, m_distanceMode);
			VL53L1X_SetTimingBudgetInMs (m_deviceAddress, m_timingBudget);
			VL53L1X_SetInterMeasurementInMs (m_deviceAddress, m_interMeasurement);

			m_deviceInitialized = true;

			return stiRESULT_SUCCESS;
		}
	}

	return stiRESULT_DEVICE_OPEN_FAILED;
}


stiHResult ToFSensor::startup ()
{
	CstiEventQueue::StartEventLoop ();

	return stiRESULT_SUCCESS;
}


stiHResult ToFSensor::distanceGet (const std::function<void(int)> &onSuccess)
{
	if (!m_deviceInitialized)
	{
		return stiRESULT_DEVICE_OPEN_FAILED;
	}

	PostEvent ([this, onSuccess]{eventDistanceGet (onSuccess);});
	return stiRESULT_SUCCESS;
}


void ToFSensor::eventDistanceGet (const std::function<void(int)> &onSuccess)
{
	if (VL53L1X_StartRanging (m_deviceAddress) == 0)
	{
		VL53L1_WaitMs (m_deviceAddress, m_timingBudget);
		
		uint8_t dataReady {0};
		int maxAttempts {5};

		while (dataReady == 0 && maxAttempts--)
		{	
			if (VL53L1X_CheckForDataReady (m_deviceAddress, &dataReady) < 0)
			{
				break;
			}

			if (dataReady)
			{
				VL53L1X_Result_t result {0};
				if (VL53L1X_GetResult (m_deviceAddress, &result) < 0)
				{
					break;
				}

				onSuccess (result.Distance);
			}
			else
			{
				VL53L1_WaitMs (m_deviceAddress, 20);
			}
		}

		VL53L1X_ClearInterrupt (m_deviceAddress);
		VL53L1X_StopRanging (m_deviceAddress);
	}
}


stiHResult ToFSensor::distanceModeSet (DistanceMode mode)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_deviceInitialized)
	{
		return stiRESULT_DEVICE_OPEN_FAILED;
	}

	auto distanceMode = static_cast<uint16_t>(mode);
	if (VL53L1X_SetDistanceMode (m_deviceAddress, distanceMode) == 0)
	{
		m_distanceMode = distanceMode;
		return stiRESULT_SUCCESS;
	}

	return stiRESULT_DRIVER_ERROR;
}


stiHResult ToFSensor::timingBudgetSet (int ms)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_deviceInitialized)
	{
		return stiRESULT_DEVICE_OPEN_FAILED;
	}

	if (!timingBudgetAllowed (ms))
	{
		return stiRESULT_INVALID_PARAMETER;
	}
	
	if (ms > m_interMeasurement)
	{
		auto hResult = interMeasurementSet (ms);
		if (hResult != stiRESULT_SUCCESS)
		{
			return hResult;
		}
	}

	if (VL53L1X_SetTimingBudgetInMs (m_deviceAddress, ms) == 0)
	{
		m_timingBudget = ms;
		return stiRESULT_SUCCESS;
	}

	return stiRESULT_DRIVER_ERROR;
}


stiHResult ToFSensor::interMeasurementSet (int ms)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_deviceInitialized)
	{
		return stiRESULT_DEVICE_OPEN_FAILED;
	}

	if (ms < m_timingBudget)
	{
		return stiRESULT_INVALID_PARAMETER;
	}

	if (VL53L1X_SetInterMeasurementInMs (m_deviceAddress, ms) == 0)
	{
		m_interMeasurement = ms;
		return stiRESULT_SUCCESS;
	}

	return stiRESULT_DRIVER_ERROR;
}


bool ToFSensor::timingBudgetAllowed (int ms)
{
	if (m_distanceMode == 2 && ms == 15) // 15ms is only available in short distance mode
	{
		return false;
	}

	// Allowable timing budgets per ST Micro ULD documentation
	return	ms ==  15 || ms ==  20 ||
			ms ==  33 || ms ==  50 ||
			ms == 100 || ms == 200 ||
			ms == 500;
}
