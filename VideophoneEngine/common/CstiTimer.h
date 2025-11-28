#ifndef CSTITIMER_H
#define CSTITIMER_H

/*!
 * \file CstiTimer.h
 * \brief CstiTimer is a simple class that represents a timer,
 *        and in conjuction with CstiWatchDog, is a cross-platform
 *        replacement for the current stiOSWdStart implementations.
 *
 * 		CstiTimer t(2000);       // 2 seconds
 * 		t.SetParams(128, 256);   // optional parameters
 * 		t.timeoutEvent.connect(
 * 			[this](int p1, int p2) {
 * 				std::cout << "Time-out event ocurred\n";
 * 			});
 *
 * 		t.Start();
 *
 * There are three basic behaviors of timers.
 * 	- Those that repeat indefinitely until cancelled manually (Type::Repeat)
 * 	- Those that repeat a specific number of times (Type::Repeat with limit set - see RepeatLimitSet())
 * 	- Those that fire only once (Type::Single)
 *
 * If you need to move a timer around, it should be heap allocated.  If
 * the timer will stay put until destructed, then allocating it on the
 * stack is preferred.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiWatchDog.h"
#include "CstiSignal.h"

class CstiTimer
{
public:
	enum class Type
	{
		Single,
		Repeat,
	};

public:
	CstiTimer ();

	CstiTimer (
		int timeoutMs);

	~CstiTimer ();

	// Don't allow timers to be copied, as this would break things
	CstiTimer (
		const CstiTimer &rhs) = delete;

	CstiTimer (
		CstiTimer &&rhs) = delete;

	CstiTimer &operator= (
		const CstiTimer &) = delete;

	CstiTimer &operator= (
		CstiTimer &&) = delete;

	// Sets the timeout interval (milliseconds)
	void TimeoutSet (
		int timeoutMs);

	// Sets the type of the timer
	void TypeSet (
		Type type);

	// Sets the number of times the timer should be allowed to timeout
	// before stopping
	void RepeatLimitSet (
		int limit);

	//void ParamsSet(int param1, int param2=0);

	// Start the timer!
	bool Start ();

	// Stop the timer
	bool Stop ();

	// Restart the timer (useful for true watchdog timers)
	void Restart ();

	// Getters
	int TimeoutGet () const
	{
		return m_timeoutMs;
	}

	CstiTimer::Type TypeGet () const
	{
		return m_type;
	}

	uint32_t IdGet () const
	{
		return m_id;
	}

	bool IsActive () const
	{
		return (CstiWatchDog::INACTIVE_TIMER_ID != m_id);
	}
	
	// Returns how many times have we started this timer.
	uint32_t startCountGet () const
	{
		return m_startCounter;
	}

	// Triggers the event
	void operator() ();

	bool TimeoutLimitReached () const;

public:
	CstiSignal<> timeoutEvent;

private:
	int m_timeoutMs = 0;
	Type m_type = Type::Single;
	int m_repeatCount = 0;
	int m_repeatLimit = -1;  // -1 means no limit
	uint32_t m_id = CstiWatchDog::INACTIVE_TIMER_ID;
	uint32_t m_startCounter = 0;
};

#endif
