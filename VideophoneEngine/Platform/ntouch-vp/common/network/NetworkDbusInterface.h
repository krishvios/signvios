// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "DbusInterfaceBase.h"
#include "CstiNetworkConnmanInterface.h"
#include "CstiSignal.h"

#include "DbusError.h"
#include "DbusCall.h"

#include <string>
#include <memory>

#define CONNMAN_SERVICE           "net.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE CONNMAN_SERVICE ".Manager"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"
#define CONNMAN_TECHNOLOGY_INTERFACE CONNMAN_SERVICE ".Technology"
#define CONNMAN_AGENT_INTERFACE CONNMAN_SERVICE ".Agent"

#define DBUS_PROPERTIES_INTERFACE			"org.freedesktop.DBus.Properties"
#define DBUS_INTROSPECTABLE_INTERFACE		"org.freedesktop.DBus.Introspectable"


class NetworkDbusInterface: public DbusInterfaceBase
{
public:
	NetworkDbusInterface (
		DbusObject *dbusObject,
		const std::string &interfaceName)
	: DbusInterfaceBase(dbusObject, interfaceName)
	{
	}

	NetworkDbusInterface (const NetworkDbusInterface &other) = delete;
	NetworkDbusInterface (NetworkDbusInterface &&other) = delete;
	NetworkDbusInterface &operator= (const NetworkDbusInterface &other) = delete;
	NetworkDbusInterface &operator= (NetworkDbusInterface &&other) = delete;

	~NetworkDbusInterface () override = default;

	static std::shared_ptr<NetworkDbusInterface> interfaceMake (
		DbusObject *dbusObject,
		const std::string &interfaceName);

private:

};

