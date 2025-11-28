// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <gio/gio.h>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include "CstiSignal.h"
#include "DbusConnection.h"
#include <functional>

class CstiEventQueue;
class DbusInterfaceBase;

/*!\brief The DBUS Object class
 *
 */
class DbusObject
{
public:

	DbusObject (
		std::string path,
		std::shared_ptr<DbusConnection> dbusConnection,
		CstiEventQueue *eventQueue)
	:
		m_objectPath (path),
		m_dbusConnection (dbusConnection),
		m_eventQueue (eventQueue)
	{
	};

	DbusObject (const DbusObject &other) = delete;
	DbusObject (DbusObject &&other) = delete;
	DbusObject &operator= (const DbusObject &other) = delete;
	DbusObject &operator= (DbusObject &&other) = delete;

	~DbusObject () = default;

	std::shared_ptr<DbusInterfaceBase> interfaceGet (
		const std::string &interfaceName);

	void interfaceAdded (
		const std::string &interfaceName,
		GVariant *dict,
		std::function<std::shared_ptr<DbusInterfaceBase>(DbusObject *dbusObject, const std::string &interfaceName)> interfaceMake);

	void interfaceChanged (
		const std::string &interfaceName,
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties);

	void interfaceRemoved (
		const std::string &interfaceName);

	std::string objectPathGet ()
	{
		return m_objectPath;
	}

	std::shared_ptr<DbusConnection> dbusConnectionGet ()
	{
		return m_dbusConnection.lock ();
	}

	CstiEventQueue *eventQueueGet ()
	{
		return m_eventQueue;
	}

private:

	std::string m_objectPath;
	std::weak_ptr<DbusConnection> m_dbusConnection;
	CstiEventQueue *m_eventQueue {nullptr};

	std::map<std::string, std::shared_ptr<DbusInterfaceBase>> m_interfaceMap;
};


void propertiesDump (
	GVariant *properties,
	const std::vector<std::string> &invalidatedProperties);

