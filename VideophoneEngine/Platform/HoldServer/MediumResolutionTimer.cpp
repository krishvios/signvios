#include "MediumResolutionTimer.h"
#include "stiAssert.h"
using namespace Common;

MediumResolutionTimer::MediumResolutionTimer ()
{
	m_timerQueue = CreateTimerQueue ();
}

MediumResolutionTimer::~MediumResolutionTimer ()
{
	TimerCleanup ();
	if (!DeleteTimerQueue (m_timerQueue))
	{
		stiASSERTMSG (false, "Failed to delete medium resolution timer queue.");
	}
}

void MediumResolutionTimer::OneShot (WAITORTIMERCALLBACK callback, PVOID param, int duration)
{
	if (callback)
	{
		// TODO: Add client error handling if timer cleanup fails
		TimerCleanup ();

		// Set a timer to call the timer routine in 10 seconds.
		if (!CreateTimerQueueTimer (&m_timer, m_timerQueue, callback, param, duration, 0, 0))
		{
			stiASSERTMSG(false, "Failed to create MediumResolutionTimer.");
		}
	}
}

void MediumResolutionTimer::Periodic (WAITORTIMERCALLBACK callback, PVOID param, int period)
{
	if (callback)
	{
		TimerCleanup ();
		// Set a timer to call the timer routine in 10 seconds.
		if (!CreateTimerQueueTimer (&m_timer, m_timerQueue, callback, param, 0, period, 0))
		{
			stiASSERTMSG(false, "Failed to create MediumResolutionTimer.");
		}
	}
}

bool IsTimerDeleteSuccess(bool result)
{
	if (result != 0)
	{
		auto error = GetLastError();
		// IO Pending indicates that it will be deleted once the pending 
		// IO operation has been completed so we consider that "successful"
		return error != ERROR_IO_PENDING;
	}
	else
	{
		return true;
	}
}

void MediumResolutionTimer::TimerCleanup ()
{
	// If m_timer is already null, count that as success
	bool success = true; 
	if (nullptr != m_timer && nullptr != m_timerQueue)
	{
		auto result = DeleteTimerQueueTimer(m_timerQueue, m_timer, NULL);
		success = IsTimerDeleteSuccess(result);
		if (!success)
		{
			// Try again one more time as per microsoft's suggestion
			result = DeleteTimerQueueTimer(m_timerQueue, m_timer, NULL) != 0;
			success = IsTimerDeleteSuccess(result);
		}
	}

	if (success)
	{
		m_timer = nullptr;
	}
	stiASSERTMSG(success, "Failed to delete MediumResolutionTimer.");
}