#pragma once

#include <gio/gio.h>
#include <map>
#include <memory>
#include "stiTrace.h"

class DbusObject;
class CstiEventQueue;

class DbusConnection: public std::enable_shared_from_this<DbusConnection>
{
public:

	DbusConnection (
		GDBusConnection *dbusConnection);

	DbusConnection (const DbusConnection &other) = delete;
	DbusConnection (DbusConnection &&other) = delete;
	DbusConnection &operator= (const DbusConnection &other) = delete;
	DbusConnection &operator= (DbusConnection &&other) = delete;

	~DbusConnection ();

	GDBusConnection *connectionGet ()
	{
		return m_dbusConnection;
	}

	std::shared_ptr<DbusObject> find (
		const std::string &objectPath);

	std::shared_ptr<DbusObject> findOrCreate (
		CstiEventQueue *eventQueue,
		const std::string &objectPath);

	void remove (
		const std::string &objectPath);

private:

	GDBusConnection *m_dbusConnection {nullptr};

	std::map<std::string, std::shared_ptr<DbusObject>> m_dbusObjectMap;

};
