/*!
 * \file CstiTimer.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiTimer.h"

CstiTimer::CstiTimer ()
	:
	CstiTimer (0)
{
}

CstiTimer::CstiTimer (
	int timeoutMs)
	:
	m_timeoutMs (timeoutMs)
{
}


CstiTimer::~CstiTimer ()
{
	Stop ();
}


void CstiTimer::TimeoutSet (
	int timeoutMs)
{
	auto wasRunning = Stop ();

	m_timeoutMs = timeoutMs;

	if (wasRunning)
	{
		Start ();
	}
}


void CstiTimer::TypeSet (
	Type type)
{
	Stop ();

	m_type = type;
}


void CstiTimer::RepeatLimitSet (
	int limit)
{
	auto wasRunning = Stop ();

	m_repeatLimit = limit;

	if (wasRunning)
	{
		Start ();
	}
}


// Start the timer!
bool CstiTimer::Start ()
{
	if (IsActive())
	{
		return false;
	}
	
	++m_startCounter;	
	m_id = CstiWatchDog::Instance().StartTimer (this);
	return true;
}


// Stop the timer
// Returns true if the timer was active, false otherwise
bool CstiTimer::Stop ()
{
	if (!IsActive())
	{
		return false;
	}

	CstiWatchDog::Instance().CancelTimer (m_id);
	m_id = CstiWatchDog::INACTIVE_TIMER_ID;
	m_repeatCount = 0;

	return true;
}


// Restart the timer (useful for true watchdog timers)
void CstiTimer::Restart ()
{
	Stop ();
	Start ();
}


// Triggers the event
void CstiTimer::operator() ()
{
	++m_repeatCount;
	timeoutEvent.Emit ();
}


bool CstiTimer::TimeoutLimitReached () const
{
	if (m_type == Type::Repeat && m_repeatLimit > 0)
	{
		return (m_repeatCount >= m_repeatLimit);
	}
	return false;
}
