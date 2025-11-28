// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#include "NetworkDbusInterface.h"
#include "NetworkManagerInterface.h"
#include "ServiceInterface.h"
#include "TechnologyInterface.h"
#include "stiSVX.h"
#include "stiTrace.h"


std::shared_ptr<NetworkDbusInterface> NetworkDbusInterface::interfaceMake (
	DbusObject *dbusObject,
	const std::string &interfaceName)
{

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDbusInterface::%s: name = %s, objectPath = %s\n", __func__, interfaceName.c_str(), dbusObject->objectPathGet ().c_str());
	);

	std::shared_ptr<NetworkDbusInterface> interface;

	if (interfaceName == CONNMAN_MANAGER_INTERFACE)
	{
		interface = std::make_shared<NetworkManagerInterface>(dbusObject);
	}
	else if (interfaceName == CONNMAN_SERVICE_INTERFACE )
	{
		interface = std::make_shared<NetworkService>(dbusObject);
	}
	else if (interfaceName == CONNMAN_TECHNOLOGY_INTERFACE)
	{
		interface = std::make_shared<NetworkTechnology>(dbusObject);
	}
//	else if (interfaceName == CONNMAN_AGENT_INTERFACE)
//	{
//		interface = std::make_shared<ConnmanAgent> (dbusObject);
//	}
	else if (interfaceName == DBUS_INTROSPECTABLE_INTERFACE)
	{
		interface = std::make_shared<NetworkDbusInterface>(dbusObject, interfaceName);
	}
	else
	{
		interface = std::make_shared<NetworkDbusInterface>(dbusObject, interfaceName);

		stiDEBUG_TOOL(g_stiNetworkDebug > 1,
			stiTrace ("Unhandled interface add: %s\n", interfaceName.c_str ());
		);
	}

	return interface;
}
