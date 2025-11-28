// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiEventQueue.h"


/*!\brief Class for handling DBus callbacks.
 *
 */
class DbusCall
{
public:

	template<typename F, typename T, typename... Args>
	static DbusCall *bind2 (
		CstiEventQueue *taskObject,
		F &&func,
		T &&thisPointer,
		Args... args)
	{
		return new DbusCall (taskObject, func, thisPointer, args...);
	}

	/*
	 * call appropriate clean up routine at the end of a dbus call
	 */
	static void finish (
		GObject *proxy,
		GAsyncResult *res,
		gpointer UserData)
	{
		auto self = static_cast<DbusCall *>(UserData);

		g_object_ref (proxy);
		g_object_ref (res);

		self->m_eventQueue->PostEvent (std::bind(&DbusCall::eventFinish, proxy, res, self->m_method));

		delete self;
		self = nullptr;
	}

private:

	template<typename F, typename T, typename... Args>
	DbusCall (
		CstiEventQueue *taskObject,
		F &&func,
		T &&thisPointer,
		Args... args)
	:
		m_eventQueue(taskObject)
	{
		m_method = std::bind(func, thisPointer, std::placeholders::_1, std::placeholders::_2, args...);
	}

	static void eventFinish (
		GObject *proxy,
		GAsyncResult *res,
		std::function<void(GObject *pProxy, GAsyncResult *pRes)> method)
	{
		method (proxy, res);

		if (res)
		{
			g_object_unref(res);
			res = nullptr;
		}

		if (proxy)
		{
			g_object_unref(proxy);
			proxy = nullptr;
		}
	}

	std::function<void(GObject *pProxy, GAsyncResult *pRes)> m_method;
	CstiEventQueue *m_eventQueue = nullptr;
};
