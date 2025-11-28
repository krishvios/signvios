#ifndef CSTIWATCHDOG_H
#define CSTIWATCHDOG_H

/*!
 * \file CstiWatchDog.h
 * \brief The WatchDog class is a simple, cross-platform timer/watchdog
 *        implementation which is used in conjunction with CstiTimer.
 *        This class is a singleton, and runs its own thread, which loops
 *        indefinitely over its internal list of timers, or until Stop()
 *        is called.  The thread will wait on a condition variable until
 *        the next timer is ready to fire.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include <condition_variable>
#include <thread>
#include <chrono>
#include <mutex>
#include <list>

class CstiTimer;


class CstiWatchDog
{
public:
	~CstiWatchDog ();

	static CstiWatchDog &Instance ()
	{
		// This is thread-safe in c++11 according to the standard (§6.7 [stmt.dcl] p4)
		static CstiWatchDog m_instance;
		return m_instance;
	}

	// Don't allow copying, moving or assigning
	CstiWatchDog (CstiWatchDog const &) = delete;
	CstiWatchDog (CstiWatchDog &&) = delete;
	CstiWatchDog &operator= (CstiWatchDog const &) = delete;
	CstiWatchDog &operator= (CstiWatchDog &&) = delete;


public:
	// Start the thread and monitor the timers
	bool Start ();
	bool Stop ();

	// Returns true if the WatchDog loop is running
	bool IsRunning () const;

	// Adds the timer to the list of sorted timers, and returns
	// a unique id representing the timer so it can be cancelled
	uint32_t StartTimer (CstiTimer *timer);

	// Cancels an active timer
	bool CancelTimer (uint32_t timerId);

public:
	static constexpr const uint32_t INACTIVE_TIMER_ID = 0;

private:
	CstiWatchDog () = default;

	void MonitorTimersLoop ();

private:
	using TimerEntry = std::pair<std::chrono::steady_clock::time_point, CstiTimer*>;

	// Sorted timer list (first element will always be the first one to timeout)
	std::list<TimerEntry> m_timerList;

	bool m_initialized = false;
	bool m_shutdown = false;
	std::thread m_thread;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;

	// Incremented each time a timer is added
	uint32_t m_idCounter = INACTIVE_TIMER_ID;
};

#endif
