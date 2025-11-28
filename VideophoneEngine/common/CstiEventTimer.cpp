/*!
 * \file CstiEventTimer.cpp
 * \brief See CstiEventTimer.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiEventQueue.h"
#include "CstiEventTimer.h"

namespace vpe
{


/*!
 * \brief Constructor
 * \param timeoutMs - time in milliseconds before timer fires
 * \param eventQueue - the event loop thread to schedule the timer on
 */
EventTimer::EventTimer (
    int timeoutMs,
    CstiEventQueue *eventQueue)
:
    m_eventQueue(eventQueue),
    m_timeoutMs(timeoutMs),
    m_id (CstiEventQueue::INVALID_TIMER_ID)
{
}


/*!
 * \brief Destructor
 */
EventTimer::~EventTimer ()
{
    stop();
}


/*!
 * \brief Sets a new timeout value for the timer (in milliseconds)
 */
void EventTimer::timeoutSet (
    int timeoutMs)
{
	auto wasRunning = stop ();
    m_timeoutMs = timeoutMs;
    if (wasRunning) {
		start ();
    }
}


/*!
 * \brief Returns the timeout value (milliseconds)
 */
int EventTimer::timeoutGet() const
{
	return m_timeoutMs;
}


/*!
 * \brief Returns the unique id representing the timer
 */
uint32_t EventTimer::idGet() const
{
	return m_id;
}


/*!
 * \brief Returns true if the timer is currently scheduled to fire
 */
bool EventTimer::isActive() const
{
	return (m_id != CstiEventQueue::INVALID_TIMER_ID);
}


/*!
 * \brief Schedules the timer on the event loop
 */
bool EventTimer::start ()
{
    if (isActive()) {
        return false;
    }

	m_id = m_eventQueue->startTimer (this);
    return true;
}


/*!
 * \brief Cancels the timer if it is scheduled already
 */
bool EventTimer::stop ()
{
    if (!isActive()) {
        return false;
    }

	m_eventQueue->cancelTimer (m_id);
	m_id = CstiEventQueue::INVALID_TIMER_ID;
    return true;
}


/*!
 * \brief Stops then starts the timer again
 */
void EventTimer::restart ()
{
    stop ();
    start ();
}


/*!
 * \brief Causes the timer to fire its signal
 */
void EventTimer::operator() ()
{
	timeoutSignal.Emit ();
}


} // namespace vpe
