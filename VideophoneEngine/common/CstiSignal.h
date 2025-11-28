#pragma once

/*!
 * \file CstiSignal.h
 * \brief  A simple, fast, type-safe, thread-safe signal implementation
 *         using variadic templates.  This is much more powerful than
 *         simply using callback function pointers, because it's generic,
 *         allows for more than one listener when an event occurs,
 *         and generally makes it easier to decouple components cleanly.
 *
 * Usage Example:
 *   // Create a signal with a specific callback signature
 *   // This one says the function must take two integers as parameters.
 *   CstiSignal<int, int> s;
 *
 *   // You can connect to static functions, either global or static members
 *   auto conn = s.Connect(Foo);
 *
 *   // You can also use lambdas
 *   auto conn = s.Connect([&t](int a, int b){ ... });
 *
 *   // Or std::bind (this one is no longer recommended unless you really need it)
 *   // bind can be useful if you need to partially apply some parameters up front
 *   //   (Note that _1,_2... are in std::placeholder namespace)
 *   // and are required for std::bind to return the correct function signature
 *   auto conn = s.Connect(std::bind(&MyClass::Foo, &obj, _1, _2));
 *
 *   // All connected functions will be called when emitted
 *   s.Emit(5, 10);
 *
 * - Shawn Badger 2016
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#include <cstdint>
#include "ISignal.h"
#include <memory>
#include <mutex>

template<typename... Args>
class CstiSignal : public ISignal<Args...>
{
	// Shared state so signals can be copied (required by SWIG on Android)
	struct State
	{
		State() = default;
		~State() = default;

		State (const State &other) = default;
		State (State &&other) = default;
		State &operator= (const State &other) = default;
		State &operator= (State &&other) = default;

		typename ISignal<Args...>::FuncMap funcMap;
		std::recursive_mutex mutex;
		uint64_t lastId = 0;
	};

public:
	CstiSignal ()
		: m_state(std::make_shared<State>())
	{
	}

	~CstiSignal () = default;
	CstiSignal (const CstiSignal &) = default;
	CstiSignal (CstiSignal &&) = default;
	CstiSignal &operator= (const CstiSignal &) = default;
	CstiSignal &operator= (CstiSignal &&) = default;

	CstiSignalConnection ATTRIBUTES Connect (typename ISignal<Args...>::Func &&f) override
	{
		std::lock_guard<std::recursive_mutex> lock(m_state->mutex);
		m_state->funcMap.insert (std::make_pair (++m_state->lastId, std::forward<typename ISignal<Args...>::Func> (f)));

		// NOTE: since CstiSignals are copyable with ref counted State, copy signal into bound function
		// Otherwise, if connections outlive signals, the "this" pointer is invalidated
		return { std::bind(&CstiSignal::Disconnect, *this, m_state->lastId) };
	}

	void DisconnectAll () override
	{
		std::lock_guard<std::recursive_mutex> lock(m_state->mutex);
		m_state->funcMap.clear ();
	}

	void Emit (
		Args... args) override
	{
		std::lock_guard<std::recursive_mutex> lock(m_state->mutex);
		for (auto it : m_state->funcMap)
		{
			it.second (args...);
		}
	}

private:

	bool Disconnect (
		uint64_t id)
	{
		std::lock_guard<std::recursive_mutex> lock(m_state->mutex);
		if (m_state->funcMap.count (id) > 0)
		{
			m_state->funcMap.erase (id);
			return true;
		}
		return false;
	}

	std::shared_ptr<State> m_state;
};
