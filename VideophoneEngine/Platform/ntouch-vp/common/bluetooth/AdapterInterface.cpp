// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "AdapterInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "IstiBluetooth.h"
#include "stiTrace.h"


CstiSignal<std::shared_ptr<AdapterInterface>> AdapterInterface::addedSignal;
CstiSignal<std::shared_ptr<AdapterInterface>> AdapterInterface::removedSignal;
CstiSignal<std::shared_ptr<AdapterInterface>> AdapterInterface::poweredOnSignal;
CstiSignal<> AdapterInterface::scanCompletedSignal;
CstiSignal<> AdapterInterface::scanFailedSignal;
CstiSignal<> AdapterInterface::removeDeviceCompletedSignal;
CstiSignal<> AdapterInterface::removeDeviceFailedSignal;
CstiSignal<std::string> AdapterInterface::connectDeviceSucceededSignal;
CstiSignal<std::string> AdapterInterface::connectDeviceFailedSignal;


void AdapterInterface::added(
	GVariant *properties)
{
	m_powered = boolVariantGet(properties, "Powered");

	csti_bluetooth_adapter1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AdapterInterface::eventProxyNew, this));
}


void AdapterInterface::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_adapterProxy = csti_bluetooth_adapter1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: Can't get default adapter proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		addedSignal.Emit (std::static_pointer_cast<AdapterInterface>(shared_from_this ()));
	}
}


/*!
 *\ brief catch the property changed signal and if the discovery has ended,
 *\ and signal the condition that the scan is waiting on.
 */
void AdapterInterface::changed(
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	static const std::string Discovering = "Discovering";
	static const std::string Powered = "Powered";
	int lim;

	lim = g_variant_n_children(changedProperties);
	for (int i = 0; i < lim; i++)
	{
		GVariant *dict = nullptr;
		GVariant *tmp = nullptr;
		const char *property = nullptr;

		dict = g_variant_get_child_value(changedProperties, i);
		tmp = g_variant_get_child_value(dict, 0);

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("%s: \n", __func__, tmp);
		);

		property = g_variant_get_string(tmp, nullptr);
		g_variant_unref(tmp);
		tmp = nullptr;

		if (property == Discovering)
		{
			tmp = g_variant_get_child_value(dict, 1);
			auto var = g_variant_get_variant(tmp);
			bool value = g_variant_get_boolean(var);
			g_variant_unref(tmp);
			tmp = nullptr;

			if (!value)
			{
				if (m_scanning)
				{
					stiDEBUG_TOOL(g_stiBluetoothDebug,
						stiTrace("%s: Setting m_scanning to false\n", __func__);
					);

					m_scanning = false;

					scanCompletedSignal.Emit ();
				}
			}
			else
			{
				m_scanning = true;
			}

			g_variant_unref (var);
			var = nullptr;
		}
		else if (property == Powered)
		{
			tmp = g_variant_get_child_value(dict, 1);
			auto var = g_variant_get_variant(tmp);
			m_powered = g_variant_get_boolean(var);
			g_variant_unref(tmp);
			tmp = nullptr;

			if(m_powered)
			{
				poweredOnSignal.Emit(std::static_pointer_cast<AdapterInterface>(shared_from_this()));
			}

			g_variant_unref (var);
			var = nullptr;
		}
		g_variant_unref(dict);
		dict = nullptr;
	}
}


void AdapterInterface::removed ()
{
	removedSignal.Emit (std::static_pointer_cast<AdapterInterface>(shared_from_this ()));
}


stiHResult AdapterInterface::scanCancel ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCONDMSG (m_scanning, stiRESULT_ERROR, "not in a scanning state\n");

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("calling scan cancel\n");
	);

	stiASSERT (m_adapterProxy != nullptr);

	csti_bluetooth_adapter1_call_stop_discovery (
			m_adapterProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AdapterInterface::eventScanCancelFinish, this));

STI_BAIL:

	return hResult;
}


void AdapterInterface::eventScanCancelFinish (
	GObject *pProxy,
	GAsyncResult *pRes)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("scanCancelFinish\n");
	);

	stiASSERT (m_adapterProxy != nullptr);

	csti_bluetooth_adapter1_call_stop_discovery_finish (
			m_adapterProxy,
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "Error Canceling scan: %s\n", dbusError.message ().c_str ());
	}
}


stiHResult AdapterInterface::scanStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_scanning)
	{
		stiTHROWMSG (stiRESULT_ERROR, "Starting scan when already scanning\n");
	}

	stiASSERT (m_adapterProxy != nullptr);

	csti_bluetooth_adapter1_call_start_discovery (
			m_adapterProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AdapterInterface::eventScanStartFinish, this));

STI_BAIL:

	return (hResult);
}


void AdapterInterface::eventScanStartFinish (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiASSERT (m_adapterProxy != nullptr);

	csti_bluetooth_adapter1_call_start_discovery_finish(
			m_adapterProxy,
			res,
			dbusError.get ());

	if (dbusError.valid ())
	{
		scanFailedSignal.Emit ();

		stiASSERTMSG (false, "Failed starting scan: %s\n", dbusError.message ().c_str ());
	}
}


void AdapterInterface::removeDevice (
	const std::string &objectPath)
{
	stiASSERT (m_adapterProxy != nullptr);

	csti_bluetooth_adapter1_call_remove_device(
			m_adapterProxy,
			objectPath.c_str(),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AdapterInterface::eventRemoveDevice, this));
}


/*
 * clean up from removing a device
 */
void AdapterInterface::eventRemoveDevice (
	GObject *object,
	GAsyncResult *pRes)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto proxy = reinterpret_cast<CstiBluetoothAdapter1 *>(object);
	DbusError dbusError;

	stiASSERT (proxy != nullptr);

	csti_bluetooth_adapter1_call_remove_device_finish (
			proxy,
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "eventRemoveDevice: error: %s\n", dbusError.message().c_str ());

		removeDeviceFailedSignal.Emit ();
	}
	else
	{
		removeDeviceCompletedSignal.Emit ();
	}
}


void AdapterInterface::connectDevice (
	const std::string &deviceAddress,
	const std::string &addressType)
{
	stiASSERT (m_adapterProxy != nullptr);

	GVariant *properties = nullptr;
	GVariantBuilder *builder = nullptr;

	builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	g_variant_builder_add (builder, "{sv}", "Address", g_variant_new ("s", deviceAddress.c_str ()));
	if (!addressType.empty ())
	{
		g_variant_builder_add (builder, "{sv}", "AddressType", g_variant_new ("s", addressType.c_str ()));
	}
	properties = g_variant_new("a{sv}", builder);
	g_variant_builder_unref (builder);
	builder = nullptr;

	csti_bluetooth_adapter1_call_connect_device (
			m_adapterProxy,
			properties,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AdapterInterface::eventConnectDevice, this, deviceAddress));
}


void AdapterInterface::eventConnectDevice (
	GObject *proxy,
	GAsyncResult *pRes,
	const std::string &deviceAddress)
{
	DbusError dbusError;

	stiASSERT (m_adapterProxy != nullptr);

	gchar *devicePath = nullptr;

	csti_bluetooth_adapter1_call_connect_device_finish (
			m_adapterProxy,
			&devicePath,
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		connectDeviceFailedSignal.Emit (deviceAddress);
	}
	else
	{
		connectDeviceSucceededSignal.Emit (deviceAddress);
	}
}


