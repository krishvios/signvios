/*!
 * \file CstiEventTimer.h
 * \brief CstiEventTimer is a simple timer class that can be scheduled onto
 *        any CstiEventQueue, such that when it fires, the handler will be
 *        run inside that event queue's thread.
 *
 * 		vpe::EventTimer timer(sipManager, 2000);  // 2 seconds
 * 		timer.timeoutSignal.connect(
 * 			[this] {
 * 				std::cout << "Time-out event ocurred\n";
 * 			});
 *
 * 		timer.start();
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "CstiSignal.h"

class CstiEventQueue;



namespace vpe
{


class EventTimer
{
public:
	EventTimer(int timeoutMs, CstiEventQueue *eventLoop);
	EventTimer(const EventTimer &rhs) = delete;
	EventTimer(EventTimer &&rhs) = delete;
	EventTimer &operator=(const EventTimer &) = delete;
	EventTimer &operator=(EventTimer &&) = delete;

	~EventTimer();

	bool start();
	bool stop();
	void restart();

	void timeoutSet(int ms);
	int timeoutGet() const;

	uint32_t idGet() const;
	bool isActive() const;

	void operator()();

public:
	CstiSignal<> timeoutSignal;

private:
	CstiEventQueue *m_eventQueue;
    int m_timeoutMs = 0;
	uint32_t m_id = 0;
};


} // namespace vpe
