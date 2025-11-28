/*!
 * \file CFifo.h
 * \brief contains CFifo (blocking queue)
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "stiSVX.h"
#include "stiTrace.h"

#include <thread>
#include <mutex>
#include <condition_variable>

template <class T>
class CFifo
{
public:
	CFifo ();
	virtual ~CFifo ();

	virtual void Dump ();

	stiHResult Put (
		T *element);

	T *Get ();

	uint32_t CountGet ();

	/*!
	 * \brief Waits for a a buffer if one is not available.
	 *
	 * \param nTimeOut The number of seconds to wait before giving up.  A zero means to wait forever.
	 */
	T *WaitForBuffer (
		int nTimeOut);

private:

	std::list<T*> m_List;

	std::recursive_mutex m_mutexidProtection;
	std::condition_variable_any m_condBufferWait;

	// NOTE: even though m_List provides .size(), not all implementations are O(1)
	// Keep track of this here...
	uint32_t m_bufferCount = 0;
	bool m_waitingOnBuffer = false;
};


/*! \brief Constructor
 *
 * This is the default constructor for the CFifo class.
 *
 * \return None
 */
template <class T>
CFifo<T>::CFifo ()
{
	stiLOG_ENTRY_NAME (CFifo::CFifo);
}


/*! \brief Destructor
 *
 * This is the default destructor for the CFifo class.
 *
 * \return None
 */
template <class T>
CFifo<T>::~CFifo ()
{
	stiLOG_ENTRY_NAME (CFifo::~CFifo);
	m_condBufferWait.notify_all ();
}

/*!\brief Put a buffer elemnet into the fifo
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
template <class T>
stiHResult CFifo<T>::Put (
	T * element)	// Buffer to add to queue
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CFifo::Add);

	if (element)
	{
		// Take the mutex semaphore
		std::lock_guard<std::recursive_mutex> lock(m_mutexidProtection);

		m_List.push_back(element);

		++m_bufferCount;

		if (m_waitingOnBuffer)
		{
#ifdef DEBUG_BUFFER_WAIT
			stiTrace ("Signaling that a buffer has been returned.\n");
#endif
			m_condBufferWait.notify_all();
		}
	}

	return (hResult);
}

/*!\brief Get a buffer elemnet from the fifo
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
template <class T>
T *CFifo<T>::Get ()
{
	stiLOG_ENTRY_NAME (CFifo::Get);

	// Take the mutex semaphore
	std::lock_guard<std::recursive_mutex> lock(m_mutexidProtection);

	T * element = nullptr;

	if(!m_List.empty())
	{
		element = m_List.front();
		m_List.pop_front();

		--m_bufferCount;
	}

	return element;
}

/*!\brief  Returns the count of buffer elements in the fifo
 *
 *  \retval a value of the number of buffer elements in the fifo
 */
template <class T>
uint32_t CFifo<T>::CountGet ()
{
	uint32_t count = 0;

	stiLOG_ENTRY_NAME (CFifo::CountGet);

	// Take the mutex
	std::lock_guard<std::recursive_mutex> lock(m_mutexidProtection);

	count = m_bufferCount;

	return count;
}


/*!
 * \brief Waits for a a buffer if one is not available.
 *
 * \param nTimeOut The number of seconds to wait before giving up.  A zero means to wait forever.
 */
template <class T>
T *CFifo<T>::WaitForBuffer (
	int timeOut)
{
	T * element = nullptr;

	std::unique_lock<std::recursive_mutex> lock(m_mutexidProtection);

	if (m_bufferCount == 0)
	{
		m_waitingOnBuffer = true;

		for (;;)
		{
#ifdef DEBUG_BUFFER_WAIT
			stiTrace ("Waiting on a buffer to be returned.\n");
#endif
			std::cv_status result = m_condBufferWait.wait_for(lock, std::chrono::seconds(timeOut));

			if (std::cv_status::timeout == result)
			{
				break;
			}

			if (m_bufferCount != 0)
			{
#ifdef DEBUG_BUFFER_WAIT
				stiTrace ("A buffer has been returned.\n");
#endif
				break;
			}
		}

		m_waitingOnBuffer = false;
	}

	element = Get ();

	return element;
}

////////////////////////////////////////////////////////////////////////////////
//; CFifo::Dump
//
// Description: Dumps the contents of a packet queue
//
// Abstract:
//
// Returns:
//	none
//
/*!\brief  Dumps the contents of a fifo
 *
 *  \retval none
 */
template <class T>
void CFifo<T>::Dump ()
{
	stiLOG_ENTRY_NAME (CFifo::Dump);

#ifdef stiDEBUGGING_TOOLS
	// Take the mutex semaphore
	std::lock_guard<std::recursive_mutex> lock(m_mutexidProtection);

	stiTrace ("Packet List Begin\n");

	for (T * element : m_List)
	{
		stiTrace ("Packet %p\n", element);
	}

	stiTrace ("Packet List End\n");

#endif // stiDEBUGGING_TOOLS

} // end CFifo::Dump

