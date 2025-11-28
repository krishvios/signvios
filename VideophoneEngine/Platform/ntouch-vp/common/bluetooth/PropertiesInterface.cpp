// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "PropertiesInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "stiTrace.h"



PropertiesInterface::PropertiesInterface (
	DbusObject *dbusObject)
:
	BluetoothDbusInterface (dbusObject, DBUS_PROPERTIES_INTERFACE)
{
}


PropertiesInterface::~PropertiesInterface ()
{
	if (m_propertyProxy)
	{
		g_object_unref(m_propertyProxy);
		m_propertyProxy = nullptr;
	}
}


void PropertiesInterface::added (
	GVariant *properties)
{
	csti_bluetooth_org_freedesktop_dbus_properties_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &PropertiesInterface::eventPropertiesProxyNew, this));
}


void PropertiesInterface::eventPropertiesProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace ("Object %s: Interface %s: Finished proxy creation\n", objectPathGet().c_str (), interfaceNameGet ().c_str ());
	);

	m_propertyProxy = csti_bluetooth_org_freedesktop_dbus_properties_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: Can't create property proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
}


void PropertiesInterface::propertySet (
	const std::string &interfaceName,
	const std::string &property,
	GVariant *value)
{
	auto proxy = (CstiBluetoothOrgFreedesktopDBusProperties*)m_propertyProxy;
	if (proxy)
	{
		csti_bluetooth_org_freedesktop_dbus_properties_call_set (
				proxy,
				interfaceName.c_str (),
				property.c_str (),
				value,
				nullptr,
				DbusCall::finish,
				DbusCall::bind2 (m_dbusObject->eventQueueGet (), &PropertiesInterface::eventPropertySet, this, interfaceName, property));
	}
	else
	{
		stiASSERTMSG (false, "Dbus object %s does not have a proxy for the properties interface\n", objectPathGet().c_str ());
	}
}


void PropertiesInterface::eventPropertySet (
	GObject *proxy,
	GAsyncResult *res,
	const std::string &interfaceName,
	const std::string &property)
{
	DbusError dbusError;
	auto propertiesProxy = reinterpret_cast<CstiBluetoothOrgFreedesktopDBusProperties *>(proxy); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	stiASSERT (propertiesProxy != nullptr);

	csti_bluetooth_org_freedesktop_dbus_properties_call_set_finish (propertiesProxy, res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "Interface %s: Can't set property %s: %s\n", interfaceName.c_str (), property.c_str (), dbusError.message ().c_str ());
	}
}


/*
 * get a named boolean value from a dict
 */
bool boolVariantGet(GVariant *dict, const std::string &propertyName)
{
	bool retVal = false;

	GVariant *variant = nullptr;
	variant = g_variant_lookup_value(dict, propertyName.c_str (), nullptr);

	if (variant)
	{
		retVal = g_variant_get_boolean(variant);
		g_variant_unref(variant);
		variant = nullptr;
	}
	return(retVal);
}

/*
 * get a named string from a dict
 */
std::string stringVariantGet(GVariant *dict, const std::string &propertyName)
{
	std::string retVal;
	GVariant *variant = nullptr;

	variant = g_variant_lookup_value(dict, propertyName.c_str (), nullptr);

	if (variant)
	{
		retVal = g_variant_get_string(variant, nullptr);
		g_variant_unref(variant);
		variant = nullptr;
	}

	return retVal;
}


