/*!
 * \file CstiEventQueue.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiEventQueue.h"
#include "stiTrace.h"
#include "stiOSNet.h"  // for stiOSSelect
#include "stiError.h"
#include "stiDefs.h"
#include "IstiPlatform.h"  // for RestartSystem
#include "nonstd/observer_ptr.h"
#include <iostream>

static std::mutex eventQueueListLock;
static std::list<std::weak_ptr<TimeAccumulator>> eventQueueList;
static CstiEventQueue sharedEventQueue("SharedEventQueue");


CstiEventQueue::CstiEventQueue (
	const std::string &taskName)
	: m_taskName (taskName)
{
	auto result = stiOSSignalCreate (&m_signal);
	auto hResult = static_cast<stiHResult>(result);

	stiTESTCONDMSG (hResult == stiRESULT_SUCCESS, stiRESULT_ERROR, "failed to create signal fd");

	// Keep track of the highest-value fd for select()'s sake
	m_maxFd = stiOSSignalDescriptor (m_signal);

	m_accumulator = std::make_shared<TimeAccumulator>(m_taskName);

STI_BAIL:
	return;
}


CstiEventQueue::~CstiEventQueue ()
{
	StopEventLoop ();
	stiOSSignalClose (&m_signal);
}


/*! \brief Starts the event loop in a new thread
 *
 *  \retval true if success
 */
bool CstiEventQueue::StartEventLoop ()
{
	// Do nothing if the event loop is already running
	std::lock_guard<std::mutex> lock (m_mutex);

	if (m_shutdown)
	{
		// Reset the count for max # of events backed up in the queue
		m_highWaterMark = 0;

		m_queueDepthLastAssertTime = std::chrono::steady_clock::now();

		// Launch a thread to run eventLoop
		m_thread = std::thread (&CstiEventQueue::eventLoop, this);

		if (m_thread.get_id () != std::thread::id ())
		{
			m_shutdown = false;

			// Register the task so stiOSTaskIDSelf() works properly
			// This has to be called from the thread that we are registering
			m_taskId = stiOSTaskRegister (
			               m_thread.native_handle (),
			               m_taskName);

			std::unique_lock<std::mutex> lock(eventQueueListLock);
			eventQueueList.push_back(m_accumulator);
		}
	}

	return !m_shutdown;
}


/*! \brief Stop processing events and cleanup thread
 */
void CstiEventQueue::StopEventLoop ()
{
	std::unique_lock<std::mutex> lock (m_mutex);

	// Is the event loop running?
	if (!m_shutdown)
	{
		m_shutdown = true;
		stiOSSignalSet (m_signal);

		lock.unlock ();

		if (m_thread.joinable ())
		{
			m_thread.join ();
		}

		stiOSTaskUnregister (m_taskId);
		m_taskId = nullptr;

		// Remove the accumulator from the list of accumulators.
		std::unique_lock<std::mutex> lock(eventQueueListLock);
		auto it = std::find_if (eventQueueList.begin (), eventQueueList.end (),
			[this](std::weak_ptr<TimeAccumulator> a)
			{
				auto accum = a.lock ();

				return (accum && accum.get () == m_accumulator.get ());
			});
		
		if (it != eventQueueList.end() )
		{
			eventQueueList.erase (it);
		}
	}
}


/*! \brief Safely add an event to the event queue
 */
void CstiEventQueue::PostEvent (
	const EventHandler &event)
{
	std::lock_guard<std::mutex> queueLock (m_queueMutex);
	m_events.push (event);

	// Update the watermark for queue depth (max depth seen so far)
	if (m_events.size() > m_highWaterMark)
	{
		m_highWaterMark = m_events.size();

		stiDEBUG_TOOL (g_stiMessageQueueDebug,
			stiTrace ("High-water mark in event queue %s: %u\n",
				m_taskName.c_str(),
				m_highWaterMark);
		);
	}

	// Cause some noise if queue depth is excessively deep
	if (m_events.size() > m_queueDepthThreshold)
	{
		auto now = std::chrono::steady_clock::now();
		auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - m_queueDepthLastAssertTime).count();
		if (sec > 5)
		{
			stiASSERTMSG(false, "%s: queue depth (%u) is excessively larger than expected",
					m_taskName.c_str(),
					m_events.size());

			m_queueDepthLastAssertTime = now;
		}
	}

	stiOSSignalSet (m_signal);
}


/*! \brief Add the file descriptor to the list of FDs that are selected on
 *         handler will be called when the fd can be read from
 */
bool CstiEventQueue::FileDescriptorAttach (
	int fd,
	const EventHandler &handler)
{
	bool ret = true;

	if (fd < 0
#ifndef WIN32
		|| fd > FD_SETSIZE
#endif
		)
	{
		ret = false;
	}
	else
	{
		std::lock_guard<std::mutex> lock (m_mutex);

		m_fdHandlerMap[fd] = handler;

		if (fd > m_maxFd)
		{
			m_maxFd = fd;
		}

		stiOSSignalSet (m_signal);
	}

	return ret;
}


/*! \brief Remove the file descriptor from the list of FDs that are selected on
 *         so we are no longer monitoring it.
 */
void CstiEventQueue::FileDescriptorDetach (
	int fd)
{
	std::lock_guard<std::mutex> lock (m_mutex);
	if (m_fdHandlerMap.count(fd) > 0)
	{
		m_fdDetachList.push_back(fd);
	}
	stiOSSignalSet (m_signal);
}


/*!
 * \brief Schedules the given timer onto the event loop.
 *        Return a unique id representing the timer
 */
uint32_t CstiEventQueue::startTimer (
    vpe::EventTimer *timer)
{
	std::lock_guard<std::mutex> lock (m_mutex);

	auto timeNow = std::chrono::steady_clock::now();
	auto timeoutTime = timeNow + std::chrono::milliseconds (timer->timeoutGet());

	bool inserted = false;
	for (auto it = m_timerList.begin(); it != m_timerList.end(); it++)
	{
		// Insert new timer here if it will timeout before this timer
		if (timeoutTime < it->first)
		{
			m_timerList.insert (it, std::make_pair(timeoutTime, timer));
			inserted = true;
			break;
		}
	}

	// If there were no timers in the list that would expire after
	// this timer, then just ad it to the end of the list
	if (!inserted)
	{
		m_timerList.emplace_back(timeoutTime, timer);
	}

	// In case the counter wraps back to the invalid id
	if (++m_timerIdIncrementor == INVALID_TIMER_ID) {
		++m_timerIdIncrementor;
	}

	// Signal the event loop that a new timer was added
	stiOSSignalSet (m_signal);

	timerStarted.Emit (m_timerIdIncrementor, timer->timeoutGet());

	return m_timerIdIncrementor;
}


/*!
 * \brief Cancels the currently scheduled timer, represented by id
 *        Returns true if the timer was cancelled successfully, false otherwise
 */
bool CstiEventQueue::cancelTimer (
    uint32_t id)
{
	std::lock_guard<std::mutex> lock (m_mutex);

	auto removed = false;
	for (auto it = m_timerList.begin(); it != m_timerList.end(); it++)
	{
		auto *t = it->second;
		if (t->idGet() == id)
		{
			timerCancelled.Emit (id);
			m_timerList.erase (it);
			removed = true;
			break;
		}
	}

	return removed;
}


/*! \brief Sets a custom warning threshold for queue depth
 */
void CstiEventQueue::QueueDepthThresholdSet (uint8_t threshold)
{
	std::lock_guard<std::mutex> lock (m_queueMutex);
	m_queueDepthThreshold = threshold;
}


/*! \brief Detects if the thread calling this function is the
 *         same as the one running this object's event loop
 *
 *  \retval true if the caller thread is the same as the event loop thread
 */
bool CstiEventQueue::CurrentThreadIsMine ()
{
	std::lock_guard<std::mutex> lock (m_mutex);
	return m_shutdown? true: (std::this_thread::get_id () == m_thread.get_id ());
}

CstiEventQueue& CstiEventQueue::sharedEventQueueGet ()
{
	return sharedEventQueue;
}


/*!
 * \brief Gets the file descriptor that is signaled when an event is posted
 * \return int
 */
int CstiEventQueue::queueFileDescriptorGet ()
{
	return stiOSSignalDescriptor (m_signal);
}


/*!
 * \brief Processes one event from the queue (if present)
 */
void CstiEventQueue::processNextEvent ()
{
	std::unique_lock<std::mutex> queueLock (m_queueMutex);

	// Grab an event from the queue, if there is one available
	if (!m_events.empty())
	{
		m_accumulator->increment("events");

		EventHandler handler = m_events.front ();
		m_events.pop ();

		queueLock.unlock ();

		// Run the event
		Lock ();
		handler ();
		Unlock ();
	}

	// Decrement the signal counter so we don't spin when there's
	// nothing in the queue
	stiOSSignalClear(m_signal);
}


/*!
 * \brief Process the next expired timer.  Returns the ID of the timer that was
 *        processed, or INVALID_TIMER_ID if none were processed.
 */
uint32_t CstiEventQueue::processNextTimer ()
{
	std::unique_lock<std::mutex> lock (m_mutex);

	// If there are no timers scheduled, do nothing
	if (!m_timerList.empty())
	{
		auto timeNow = std::chrono::steady_clock::now ();
		auto &head = m_timerList.front ();

		// Has the timer reached its timeout?
		if (timeNow >= head.first)
		{
			m_accumulator->increment("timerEvents");
			
			auto *timer = head.second;
			auto timerId = timer->idGet ();
			m_timerList.pop_front ();
			lock.unlock ();

			timer->stop ();

			// Fire the signal
			Lock ();
			(*timer)();
			Unlock ();

			return timerId;
		}
	}

	return CstiEventQueue::INVALID_TIMER_ID;
}


/*!
 * \brief Returns the number of milliseconds until the next timer
 *        is scheduled to fire, or 0 if a timer is already past due.
 *        Will return -1 if there are no timers scheduled.
 */
int32_t CstiEventQueue::nextTimerTimeoutGet ()
{
	std::lock_guard<std::mutex> lock (m_mutex);

	int32_t nextTimeoutMs = -1;

	// Calculate when the next timer is scheduled to fire
	if (!m_timerList.empty())
	{
		auto &entry = m_timerList.front ();
		auto nextTimeout = entry.first;
		auto now = std::chrono::steady_clock::now ();
		nextTimeoutMs = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(nextTimeout - now).count ());

		// If less than zero, the next timer has already expired
		if (nextTimeoutMs < 0)
		{
			nextTimeoutMs = 0;
		}
	}

	return nextTimeoutMs;
}


/*! \brief Loops over the queue of events and processes them as they are added
 *         until m_shutdown is set to true.  The function is run in its own
 *         thread and is the "event loop".
 */
void CstiEventQueue::eventLoop ()
{
	try
	{

	std::unique_lock<std::mutex> memberLock (m_mutex);

	stiOSTaskSetCurrentThreadName(m_taskName);

	// Allows any initialization that may need to be run in this thread
	// prior to any events being processed
	startedSignal.Emit ();

	auto eventFd = stiOSSignalDescriptor (m_signal);
	fd_set readFds;
	timeval tv { 10, 0 };
	timeval tv1 = tv;

	while (!m_shutdown)
	{
		stiFD_ZERO (&readFds);
		stiFD_SET (eventFd, &readFds);

		// Set other fds we are monitoring
		for (auto &keyval : m_fdHandlerMap)
		{
			stiFD_SET (keyval.first, &readFds);
		}

		auto nFd = m_maxFd+1;

		memberLock.unlock ();

		m_accumulator->timeStart ("idle");

		stiOSSelect (nFd, &readFds, nullptr, nullptr, &tv1);

		m_accumulator->timeStart ("execution");

		memberLock.lock ();

		// Reset select timeout back to default
		tv1 = tv;

		// If set, we were signaled that an event was added,
		// or a new file descriptor is being monitored
		if (stiFD_ISSET (eventFd, &readFds))
		{
			memberLock.unlock ();
			processNextEvent ();
			memberLock.lock ();
		}

		// Now check each fd we are monitoring
		for (auto &keyval : m_fdHandlerMap)
		{
			if (stiFD_ISSET (keyval.first, &readFds))
			{
				// Call the handler associated with this fd
				const EventHandler &handler = keyval.second;

				memberLock.unlock ();

				Lock ();
				m_accumulator->increment("fdEvents");
				handler ();
				Unlock ();

				memberLock.lock ();
			}
		}

		// Now detach any file descriptors that were set through
		// FileDescriptorDetach().  We do this in a separate step
		// so that all handlers can safely call FileDescriptorDetach
		// without deadlocking or modifying the fdHandlerMap while we
		// are looping over it
		if (!m_fdDetachList.empty())
		{
			DetachHandlers ();
		}

		// Process an expired timer (if any)
		memberLock.unlock ();
		processNextTimer ();
		auto nextTimeoutMs = nextTimerTimeoutGet ();
		memberLock.lock ();

		// Update the timeout for select() so it expires when the next timer is ready to fire
		if (nextTimeoutMs >= 0)
		{
			int sec = nextTimeoutMs / 1000;
			int usec = (nextTimeoutMs - (sec*1000)) * 1000;
			tv1.tv_sec = sec;
			tv1.tv_usec = usec;
		}
	}

	// Allow any cleanup code that needs to run in this thread
	shutdownSignal.Emit ();

	}
	catch (const std::exception &e)
	{
		stiTrace ("CstiEventQueue::eventLoop: ***UNHANDLED EXCEPTION*** (Thread: %s) - %s\n", m_taskName.c_str(), e.what());
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}
	catch (...)
	{
		stiTrace ("CstiEventQueue::eventLoop: ***UNHANDLED EXCEPTION*** (Thread: %s) - Unknown\n");
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}

	m_accumulator->timeStop ();
}


/*!
 * \brief Helper method to remove file descriptors from the fdHandlerMap
 *        that exist in the fdDetachList, and update m_maxFd accordingly
 */
void CstiEventQueue::DetachHandlers ()
{
	for (auto fd : m_fdDetachList)
	{
		if (m_fdHandlerMap.count (fd) > 0)
		{
			m_fdHandlerMap.erase (fd);
		}
	}
	m_fdDetachList.clear ();

	// Update m_maxFd so select doesn't have to work harder than necessary
	m_maxFd = stiOSSignalDescriptor (m_signal);

	for (auto &entry : m_fdHandlerMap)
	{
		if (entry.first > m_maxFd)
		{
			m_maxFd = entry.first;
		}
	}
}

// Retrieves accumulators for all running event queues.
std::vector<TimeAccumulator::Stats> eventQueueStatsGet ()
{
	std::vector<TimeAccumulator::Stats> stats;

	std::unique_lock<std::mutex> lock(eventQueueListLock);

	for (auto &accumulator: eventQueueList)
	{
		auto accum = accumulator.lock ();

		if (accum)
		{
			stats.push_back(accum->statsGet ());
		}
	}

	return stats;
}
