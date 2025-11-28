#pragma once

/*!
 * \file ISignal.h
 * \brief  Interface for using CstiSignal with other interfaces.
 *
 * Usage Example:
 * To use it in an interface you must provide a function to get the signal.
 * virtual ISignal<int, int>& signalGet() = 0;
 *
 * In your concrete class you implement the function and return the CstiSignal member variable.
 * ISignal<int, int>& signalGet() override
 * {
 *		return m_signal;
 * }
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#include <functional>
#include <map>
#include <list>
#include <cstdint>
#if APPLICATION == APP_NTOUCH_VP3 ||  APPLICATION == APP_NTOUCH_VP4
#define ATTRIBUTES __attribute__ ((warn_unused_result))
#else
#define ATTRIBUTES
#endif

class CstiSignalConnection
{
	using DisconnectionFuncType = std::function<bool ()>;

public:

	using Collection = std::list<CstiSignalConnection>;

	/**
	 * Allow empty constructor so that connection can be constructed,
	 * and then move constructed
	 */
	CstiSignalConnection () = default;

	/**
	 * Would like to make this private, but CstiSignal needs access to it
	 * (TODO: use friend?)
	 */
	CstiSignalConnection (const DisconnectionFuncType &f)
		: m_func (f)
	{
	}

	// Move assignable
	CstiSignalConnection & operator=(CstiSignalConnection &&other)
	{
		// Be sure to disconnect from current signal
		// before assuming ownership of new connection
		Disconnect ();

		std::swap (m_func, other.m_func);
		return *this;
	}

	// Move Constructable
	CstiSignalConnection (CstiSignalConnection && other)
	{
		std::swap (m_func, other.m_func);
	}

	// But not copyable/assignable
	CstiSignalConnection (const CstiSignalConnection &) = delete;
	CstiSignalConnection & operator=(const CstiSignalConnection &) = delete;

	void Disconnect ()
	{
		if (m_func)
		{
			m_func ();
			m_func = nullptr;
		}
	}

	/**
	 * is this a valid connection?
	 */
	operator bool () const
	{
		return m_func != nullptr;
	}

	~CstiSignalConnection ()
	{
		Disconnect ();
	}

private:

	// Intended to call "Disconnect" on the signal
	// (empty/nullptr by default)
	DisconnectionFuncType m_func;
};

template<typename... Args>
class ISignal
{
public:
	using Func = std::function<void (Args...)>;
	using FuncMap = std::map<uint64_t, Func>;

	ISignal () = default;
	~ISignal () = default;
	ISignal (const ISignal &) = default;
	ISignal (ISignal &&) = default;
	ISignal &operator= (const ISignal &) = default;
	ISignal &operator= (ISignal &&) = default;

	virtual CstiSignalConnection ATTRIBUTES Connect (Func &&f) = 0;

	virtual void DisconnectAll () = 0;

	virtual void Emit (Args... args) = 0;
};
