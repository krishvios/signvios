#include "DbusConnection.h"
#include "DbusObject.h"


DbusConnection::DbusConnection (
	GDBusConnection *dbusConnection)
:
	m_dbusConnection (dbusConnection)
{
}


DbusConnection::~DbusConnection ()
{
	g_clear_object(&m_dbusConnection);
}


std::shared_ptr<DbusObject> DbusConnection::find (
	const std::string &objectPath)
{
	auto iter = m_dbusObjectMap.find(objectPath);

	if (iter != m_dbusObjectMap.end ())
	{
		return iter->second;
	}
	else
	{
		return nullptr;
	}
}


std::shared_ptr<DbusObject> DbusConnection::findOrCreate (
	CstiEventQueue *eventQueue,
	const std::string &objectPath)
{
	auto dbusObject = find (objectPath);

	if (dbusObject == nullptr)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("Object %s: Adding object\n", objectPath.c_str ());
		);

		dbusObject = std::make_shared<DbusObject>(objectPath, shared_from_this(), eventQueue);

		m_dbusObjectMap.insert(std::pair<std::string, std::shared_ptr<DbusObject>>(objectPath, dbusObject));
	}

	return dbusObject;
}


void DbusConnection::remove (
	const std::string &objectPath)
{
	auto iter = m_dbusObjectMap.find (objectPath);

	if (iter != m_dbusObjectMap.end ())
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("Object %s: Removing object\n", objectPath.c_str ());
		);

		m_dbusObjectMap.erase (iter);
	}
	else
	{
		stiASSERTMSG (false, "Dbus object not found at %s\n", objectPath.c_str ());
	}
}
