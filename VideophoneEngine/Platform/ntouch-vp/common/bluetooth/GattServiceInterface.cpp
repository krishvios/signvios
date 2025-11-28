// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "GattServiceInterface.h"
#include "DeviceInterface.h"
#include "DbusCall.h"
#include "DbusError.h"


/*
 *\ brief add a new service
 *
 */
void SstiGattService::added(
	GVariant *properties)
{
	m_devicePath = stringVariantGet(properties, "Device");
	m_uuid = stringVariantGet(properties, "UUID");
	m_primary = boolVariantGet(properties, "Primary");

	auto deviceInterface = deviceInterfaceGet ();

	if (deviceInterface)
	{
		deviceInterface->serviceAdd (std::static_pointer_cast<SstiGattService>(shared_from_this ()));
	}
	else
	{
		stiASSERT (false);
	}

	csti_bluetooth_gatt_service1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattService::eventProxyNew, this));
}


void SstiGattService::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_serviceProxy = csti_bluetooth_gatt_service1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: Can't create property proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
}


void SstiGattService::removed ()
{
	auto deviceInterface = deviceInterfaceGet ();

	if (deviceInterface)
	{
		deviceInterface->serviceRemove (std::static_pointer_cast<SstiGattService>(shared_from_this ()));
	}
}


void SstiGattService::changed(
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	m_primary = boolVariantGet (changedProperties, "Primary");
}


std::shared_ptr<SstiBluezDevice> SstiGattService::deviceInterfaceGet ()
{
	auto parentDbusObject = m_dbusObject->dbusConnectionGet ()->find (m_devicePath);

	if (parentDbusObject)
	{
		auto deviceInterface = std::static_pointer_cast<SstiBluezDevice>(parentDbusObject->interfaceGet (BLUETOOTH_DEVICE_INTERFACE));

		if (deviceInterface)
		{
			return deviceInterface;
		}
		else
		{
			stiASSERTMSG (false, "No device interface found at path %s\n", m_devicePath.c_str ());
		}
	}
	else
	{
		stiASSERTMSG (false, "No object found at path %s\n", m_devicePath.c_str ());
	}

	return nullptr;
}
