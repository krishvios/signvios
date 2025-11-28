#ifndef CSTIEVENTQUEUE_H
#define CSTIEVENTQUEUE_H

/*!
 * \file CstiEventQueue.h
 * \brief Type-safe event queue implementation that can also monitor file descriptors
 *
 * Basic Usage:
 *
 *   class MyTask : public EventQueue
 *   { ... }
 *
 *   MyTask task;
 *
 *   // Spawns a thread and starts the event loop
 *   task.StartEventLoop ();
 *
 *   task.FileDescriptorAdd (fd, []{ std::cout << "Can read from fd"; });
 *
 *   // Schedules MyTask::SomeHandler to be run with params p1 and p2
 *   task.EventPost (
 *       std::bind(&MyTask::SomeHandler, &myobj, p1, p2));
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <list>
#include <map>
#include <future>
#include "stiOSSignal.h"
#include "stiOSTask.h"
#include "CstiSignal.h"
#include "CstiEventTimer.h"
#include "TimeAccumulator.h"

class CstiEventQueue
{
public:
	using EventHandler = std::function<void()>;
	using TimerEntry = std::pair<std::chrono::steady_clock::time_point, vpe::EventTimer*>;

	static constexpr uint32_t INVALID_TIMER_ID = 0;

public:
	explicit CstiEventQueue (
		const std::string &name);

	virtual ~CstiEventQueue ();

	CstiEventQueue (const CstiEventQueue &other) = delete;
	CstiEventQueue (CstiEventQueue &&other) = delete;
	CstiEventQueue &operator = (const CstiEventQueue &other) = delete;
	CstiEventQueue &operator = (CstiEventQueue &&other) = delete;

	// Start the event loop in a thread
	bool StartEventLoop ();

	// Kill the event loop and cleanup the thread
	void StopEventLoop ();

	// Add an event to the queue
	void PostEvent (
		const EventHandler &handler);

	template<typename T>
	std::shared_ptr<std::promise<T>> execute (const std::function<T ()>& func)
	{
		auto p = std::make_shared<std::promise<T>> ();
		PostEvent ([func, p]() {
			p->set_value (func ());
		});
		return p;
	}

	template<typename T>
	T executeAndWait (const std::function<T ()>& func)
	{
		auto p = execute (func);
		auto f = p->get_future ();
		f.wait ();
		return f.get ();
	}

	// Start monitoring a file descriptor, and call handler when it can be read from
	bool FileDescriptorAttach (
		int fd,
		const EventHandler &handler);

	// Stop monitoring a file descriptor
	void FileDescriptorDetach (
		int fd);

	// Schedule a timer to fire from the event loop
	uint32_t startTimer (
	    vpe::EventTimer *timer);

	// Cancel a scheduled timer, so it won't fire
	bool cancelTimer (
	    uint32_t id);

	// Set a custom warning threshold for queue depth
	void QueueDepthThresholdSet (uint8_t threshold);

	// Will return false if the current thread does not match m_thread
	bool CurrentThreadIsMine ();

	// Returns true if the event loop is running
	bool eventLoopIsRunning () const
	{
		std::lock_guard<std::mutex> memberLock (m_mutex);
		return !m_shutdown;
	}

	// Connects a CstiSignal to an event that gets posted on the queue.
	template<typename... ArgTypes, typename FuncType>
	uint64_t signalEventConnect(CstiSignal<ArgTypes...> &sig, FuncType &&func)
	{
		return sig.Connect([this, func](ArgTypes&&... args)
		{
			std::function<void()> fn = std::bind(func, std::forward<ArgTypes>(args)...);

			PostEvent(fn);
		});
	}

	static CstiEventQueue& sharedEventQueueGet ();

public:
	// This signal will be the first thing to run in the newly created
	// thread, just before the event loop
	CstiSignal<> startedSignal;

	// This signal will trigger just before the event loop is shut down.
	// Any cleanup code that needs to run in the event loop thread can
	// simply connect to this signal
	CstiSignal<> shutdownSignal;

	// These signals are fired whenever a timer is started/cancelled
	// This is useful for components that override the default event
	// loop (ie SipManager).  <id, timeoutMs>
	CstiSignal<uint32_t, uint32_t> timerStarted;
	CstiSignal<uint32_t> timerCancelled;

	// Manually lock the event loop so it doesn't process any events
	// This does not prevent events from being posted to the queue
	// NOTE:  Use std::lock_guard instead for RAII benefits
	void Lock () const
	{
		m_execMutex.lock ();
	}

	// Manually unlock the event loop so it starts processing events again
	// NOTE:  Use std::lock_guard instead for RAII benefits
	void Unlock () const
	{
		m_execMutex.unlock ();
	}

protected:

	// Returns the file descriptor that gets signaled when an event is posted
	int queueFileDescriptorGet ();

	// Processes one event from the queue (if present)
	void processNextEvent ();

	// Processes one expired timer from the list of scheduled timers (if any)
	uint32_t processNextTimer ();

	// Gets the time in milliseconds before the next timer is scheduled to fire
	int32_t nextTimerTimeoutGet ();

	// The event loop that runs in the new thread
	virtual void eventLoop ();

protected:
	// This mutex is locked during the execution of handlers
	// It's purpose is to allow tasks to manually obtain the
	// lock in order to make a member function synchronous,
	// rather than having it run from the event queue.  The
	// reason we use a separate lock is to ensure that events
	// can still get posted without blocking, even though those
	// events won't actually get executed until this is unlocked
	mutable std::recursive_mutex m_execMutex;

	std::string m_taskName;

private:
	void DetachHandlers ();

private:
	static constexpr const uint8_t DEFAULT_QUEUE_DEPTH_THRESHOLD = 32;

private:
	std::queue<EventHandler> m_events;
	stiTaskID m_taskId = nullptr;
	bool m_shutdown = true;

	std::thread m_thread;
	std::mutex m_queueMutex;  // protects the queue & thresholds
	mutable std::mutex m_mutex;       // protects all other members

	// Attached file descriptors and their handlers
	std::map<int, EventHandler> m_fdHandlerMap;

	// File descriptors that are scheduled to be detached
	std::list<int> m_fdDetachList;

	// Event timers scheduled to fire
	std::list<TimerEntry> m_timerList;
	uint32_t m_timerIdIncrementor = INVALID_TIMER_ID;

	// Highest-value file descriptor we are selecting on (used by select())
	int m_maxFd = -1;

	// Internal pipe to notify select() of newly posted events
	STI_OS_SIGNAL_TYPE m_signal{};

	// Keeps track of the highest number of events ever backed up in the queue (for debugging)
	uint32_t m_highWaterMark = 0;

	// If the queue backs up to this threshold, make some noise
	uint8_t m_queueDepthThreshold = DEFAULT_QUEUE_DEPTH_THRESHOLD;

	// When the queue backs up, we want to limit how fast we spam asserts
	std::chrono::time_point<std::chrono::steady_clock> m_queueDepthLastAssertTime;

	std::shared_ptr<TimeAccumulator> m_accumulator;
};

// Retrieves accumulators for all running event queues.
std::vector<TimeAccumulator::Stats> eventQueueStatsGet ();

#endif //CSTIEVENTQUEUE_H
