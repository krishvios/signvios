// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "BluetoothDbusInterface.h"
#include "PropertiesInterface.h"
#include "AdapterInterface.h"
#include "AgentManagerInterface.h"
#include "GattManagerInterface.h"
#include "DeviceInterface.h"
#include "GattCharacteristicInterface.h"
#include "GattDescriptorInterface.h"
#include "GattServiceInterface.h"
#include "stiSVX.h"
#include "stiTrace.h"

void BluetoothDbusInterface::propertySet (
	const std::string &property,
	GVariant *value)
{
	auto propertiesInterface = std::static_pointer_cast<PropertiesInterface>(m_dbusObject->interfaceGet (DBUS_PROPERTIES_INTERFACE));

	if (propertiesInterface)
	{
		propertiesInterface->propertySet (m_interfaceName, property, value);
	}
	else
	{
		stiASSERTMSG (false, "Dbus object %s does not have a properties interface\n", objectPathGet().c_str ());
	}
}


std::shared_ptr<BluetoothDbusInterface> BluetoothDbusInterface::interfaceMake (
	DbusObject *dbusObject,
	const std::string &interfaceName)
{
	std::shared_ptr<BluetoothDbusInterface> interface;

	if (interfaceName == BLUETOOTH_ADAPTER_INTERFACE)
	{
		interface = std::make_shared<AdapterInterface>(dbusObject);
	}
	else if (interfaceName == BLUETOOTH_AGENT_MANAGER_INTERFACE)
	{
		interface = std::make_shared<AgentManagerInterface>(dbusObject);
	}
	else if (interfaceName == GATT_MANAGER_INTERFACE)
	{
		interface = std::make_shared<GattManagerInterface>(dbusObject);
	}
	else if (interfaceName == BLUETOOTH_DEVICE_INTERFACE)
	{
		interface = std::make_shared<SstiBluezDevice> (dbusObject);
	}
	else if (interfaceName == GATT_SERVICE_INTERFACE)
	{
		interface = std::make_shared<SstiGattService>(dbusObject);
	}
	else if (interfaceName == GATT_CHARACTERISTIC_INTERFACE)
	{
		interface = std::make_shared<SstiGattCharacteristic> (dbusObject);
	}
	else if (interfaceName == GATT_DESCRIPTOR_INTERFACE)
	{
		interface = std::make_shared<SstiGattDescriptor> (dbusObject);
	}
	else if (interfaceName == DBUS_PROPERTIES_INTERFACE)
	{
		interface = std::make_shared<PropertiesInterface>(dbusObject);
	}
	else if (interfaceName == DBUS_INTROSPECTABLE_INTERFACE)
	{
		interface = std::make_shared<BluetoothDbusInterface>(dbusObject, interfaceName);
	}
	else
	{
		interface = std::make_shared<BluetoothDbusInterface>(dbusObject, interfaceName);

		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("Unhandled interface add: %s\n", interfaceName.c_str ());
		);
	}

	return interface;
}


