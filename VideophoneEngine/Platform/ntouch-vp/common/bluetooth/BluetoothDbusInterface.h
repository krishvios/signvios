// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "DbusInterfaceBase.h"

#define BLUETOOTH_SERVICE					"org.bluez"
#define BLUETOOTH_DEVICE_INTERFACE			BLUETOOTH_SERVICE ".Device1"
#define BLUETOOTH_AGENT_INTERFACE			BLUETOOTH_SERVICE ".Agent1"
#define BLUETOOTH_AGENT_MANAGER_INTERFACE	BLUETOOTH_SERVICE ".AgentManager1"
#define BLUETOOTH_ADAPTER_INTERFACE			BLUETOOTH_SERVICE ".Adapter1"
#define GATT_MANAGER_INTERFACE				BLUETOOTH_SERVICE ".GattManager1"
#define GATT_SERVICE_INTERFACE				BLUETOOTH_SERVICE ".GattService1"
#define GATT_CHARACTERISTIC_INTERFACE		BLUETOOTH_SERVICE ".GattCharacteristic1"
#define GATT_DESCRIPTOR_INTERFACE			BLUETOOTH_SERVICE ".GattDescriptor1"
#define DBUS_PROPERTIES_INTERFACE			"org.freedesktop.DBus.Properties"
#define DBUS_INTROSPECTABLE_INTERFACE		"org.freedesktop.DBus.Introspectable"


class BluetoothDbusInterface: public DbusInterfaceBase
{
public:

	BluetoothDbusInterface (
		DbusObject *dbusObject,
		const std::string &interfaceName)
	: DbusInterfaceBase(dbusObject, interfaceName)
	{
	}

	BluetoothDbusInterface (const BluetoothDbusInterface &other) = delete;
	BluetoothDbusInterface (BluetoothDbusInterface &&other) = delete;
	BluetoothDbusInterface &operator= (const BluetoothDbusInterface &other) = delete;
	BluetoothDbusInterface &operator= (BluetoothDbusInterface &&other) = delete;

	~BluetoothDbusInterface () override = default;

	static std::shared_ptr<BluetoothDbusInterface> interfaceMake (
		DbusObject *dbusObject,
		const std::string &interfaceName);

	void propertySet (
		const std::string &property,
		GVariant *value);

private:

};

