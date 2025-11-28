/*!
 * \file CstiWatchDog.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiWatchDog.h"
#include "CstiTimer.h"
#include <iostream>
#include "stiOSTask.h"

CstiWatchDog::~CstiWatchDog ()
{
	Stop ();
}

// Start the thread and monitor the timers
bool CstiWatchDog::Start ()
{
	std::lock_guard<std::mutex> lock (m_mutex);

	if (!m_initialized)
	{
		m_shutdown = false;

		m_thread = std::thread (&CstiWatchDog::MonitorTimersLoop, this);
		if (m_thread.get_id () != std::thread::id ())
		{
			m_initialized = true;
		}
	}
	return m_initialized;
}


bool CstiWatchDog::Stop ()
{
	m_mutex.lock ();

	if (m_initialized && !m_shutdown)
	{
		m_shutdown = true;
		m_mutex.unlock ();
		m_condition.notify_all ();
		m_thread.join ();
		m_mutex.lock ();
	}

	// Remove all timers
	m_timerList.clear ();

	m_mutex.unlock ();
	return m_shutdown;
}


bool CstiWatchDog::IsRunning () const
{
	std::lock_guard<std::mutex> lock (m_mutex);
	return (m_initialized && !m_shutdown);
}


// Adds the timer to the list of sorted timers
uint32_t CstiWatchDog::StartTimer (CstiTimer *timer)
{
	std::lock_guard<std::mutex> lock (m_mutex);

	auto timeNow = std::chrono::steady_clock::now ();
	auto timeoutTime = timeNow + std::chrono::milliseconds (timer->TimeoutGet ());

	bool inserted = false;
	for (auto it = m_timerList.begin (); it != m_timerList.end (); it++)
	{
		// Insert new timer here if it will timeout before this timer
		if (timeoutTime < it->first)
		{
			m_timerList.insert (it, std::make_pair (timeoutTime, timer));
			inserted = true;
			break;
		}
	}

	// If there were no timers in the list that would expire after
	// this timer, then just add it to the end of the list.
	if (!inserted)
	{
		m_timerList.emplace_back (timeoutTime, timer);
	}

	// Notify the monitor loop that a new timer was added
	m_condition.notify_all ();

	auto id = timer->IdGet ();

	
	// Assign a new id if one doesn't already exist
	if (INACTIVE_TIMER_ID == id)
	{
		++m_idCounter;
		
		// If counter is invalid, increment again.
		if (INACTIVE_TIMER_ID == m_idCounter)
		{
			++m_idCounter;
		}
		
		id = m_idCounter;
	}
	
	return id;
}


// Cancel a running timer
bool CstiWatchDog::CancelTimer (uint32_t id)
{
	std::lock_guard<std::mutex> lock (m_mutex);

	auto removed = false;
	for (auto it = m_timerList.begin (); it != m_timerList.end (); it++)
	{
		CstiTimer *t = it->second;
		if (t->IdGet () == id)
		{
			m_timerList.erase (it);
			removed = true;
			break;
		}
	}
	return removed;
}


void CstiWatchDog::MonitorTimersLoop ()
{
	std::unique_lock<std::mutex> lock (m_mutex);

	stiOSTaskSetCurrentThreadName("CstiWatchDog");

	while (!m_shutdown)
	{
		// If there are no timers, wait here until one is added
		if (m_timerList.empty ())
		{
			m_condition.wait (lock);
			// Continue to protect against spurious wakeup
			continue;
		}


		auto timeNow = std::chrono::steady_clock::now ();

		auto &head = m_timerList.front ();

		// Has a timer timed-out?
		if (timeNow >= head.first)
		{
			// Fire off the event signal
			CstiTimer *t = head.second;


			// Remove the timer from the list
			m_timerList.pop_front ();

			// Signal the timeout
			lock.unlock();
			(*t) ();
			// If the event is to be repeated, call StartTimer again
			if (t->TypeGet () == CstiTimer::Type::Repeat && !t->TimeoutLimitReached ())
			{
				StartTimer (t);
			}
			else // Stop the timer so IsActive will return false.
			{
				t->Stop ();
			}
			lock.lock ();
		}

		// No timers have expired, so wait until the next one is scheduled to expire
		// or a new timer is added to the mix
		else
		{
			auto duration = std::chrono::duration_cast<std::chrono::microseconds> (head.first - timeNow);
			m_condition.wait_for (lock, duration);
		}
	}
}
