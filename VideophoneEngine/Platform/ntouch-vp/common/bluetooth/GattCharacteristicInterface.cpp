// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "GattCharacteristicInterface.h"
#include "BluetoothDefs.h"
#include "GattServiceInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "DeviceInterface.h"
#include "stiTrace.h"
#include <algorithm>

#define ASSERT_AFTER_THIS_MANY_QUEUED_PACKETS 100

CstiSignal<std::shared_ptr<SstiBluezDevice>, std::string, const std::vector<uint8_t> &> SstiGattCharacteristic::valueChangedSignal;
CstiSignal<std::string> SstiGattCharacteristic::readWriteFailedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>, std::string> SstiGattCharacteristic::readWriteCompletedSignal;
CstiSignal<std::string, std::string> SstiGattCharacteristic::startNotifyCompletedSignal;
CstiSignal<> SstiGattCharacteristic::startNotifyFailedSignal;


std::shared_ptr<SstiGattService> SstiGattCharacteristic::serviceInterfaceGet ()
{
	auto parentDbusObject = m_dbusObject->dbusConnectionGet ()->find (m_servicePath);

	if (parentDbusObject)
	{
		auto serviceInterface = std::static_pointer_cast<SstiGattService>(parentDbusObject->interfaceGet (GATT_SERVICE_INTERFACE));

		if (serviceInterface)
		{
			return serviceInterface;
		}
		else
		{
			stiASSERTMSG (false, "No service interface found at path %s\n", m_servicePath.c_str ());
		}
	}
	else
	{
		stiASSERTMSG (false, "No object found at path %s\n", m_servicePath.c_str ());
	}

	return nullptr;
}


/*
 *\ brief add a new characteristic
 *
 */
void SstiGattCharacteristic::added (
	GVariant *properties)
{
	m_servicePath = stringVariantGet(properties, "Service");
	m_uuid = stringVariantGet(properties, "UUID");

	GVariant *tmp = g_variant_lookup_value(properties, PropertyValue.c_str (), nullptr);
	if (tmp)
	{
		char c;
		GVariantIter variantIter;
		g_variant_iter_init(&variantIter, tmp);
		int size = g_variant_iter_n_children(&variantIter);
		std::vector<unsigned char> value(size);
		size = 0;
		while(g_variant_iter_loop(&variantIter, "y", &c))
		{
			value[size++] = c;
		}

		m_value = value;

		g_variant_unref(tmp);
		tmp = nullptr;
	}

	gsize size = 0;
	m_notifying = boolVariantGet(properties, PropertyNotifying);
	auto val = g_variant_lookup_value(properties, "Flags", nullptr);
	const char **vect = g_variant_get_strv(val, &size);
	for (gsize i = 0; i < size; i++)
	{
		m_flags.push_back(vect[i]);
	}

	g_free(vect);
	vect = nullptr;

	g_variant_unref(val);
	val = nullptr;

	auto serviceInterface = serviceInterfaceGet ();

	if (serviceInterface)
	{
		serviceInterface->characteristicAdd(std::static_pointer_cast<SstiGattCharacteristic>(shared_from_this ()));
	}
	else
	{
		stiASSERT(false);
	}

	csti_bluetooth_gatt_characteristic1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattCharacteristic::eventProxyNew, this));
}


void SstiGattCharacteristic::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace ("Object %s: Interface %s: Finished proxy creation\n", objectPathGet().c_str (), interfaceNameGet ().c_str ());
	);

	m_gattCharacteristicProxy = csti_bluetooth_gatt_characteristic1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: error making characteristic proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		// Allow device to maintain Helios readiness by passing characteristic UUID
		auto service = serviceInterfaceGet ();
		if (service)
		{
			auto device = service->deviceInterfaceGet ();
			if (device)
			{
				device->characteristicAdd (m_uuid);
			}
			else
			{
				stiASSERT (false);
			}
		}
		else
		{
			stiASSERT (false);
		}

		/*
		 * we will always want this characteristic to get notifications (it's the nordic/sorenson uart read characteristic)
		 */
		if (!m_notifying &&
			(m_uuid == PULSE_READ_CHARACTERISTIC_UUID ||
			 m_uuid == PULSE_AUTHENTICATE_CHARACTERISTIC_UUID))
		{
			notifyStart ();
		}
	}
}


void SstiGattCharacteristic::changed(
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	int lim = g_variant_n_children(changedProperties);
	for (int i = 0; i < lim; i++)
	{
		GVariant *dict = g_variant_get_child_value(changedProperties, i);
		GVariant *tmp = g_variant_get_child_value(dict, 0);
		const char *property = g_variant_get_string(tmp, nullptr);
		g_variant_unref(tmp);
		tmp = nullptr;

		if (property == PropertyNotifying)
		{
			tmp = g_variant_get_child_value(dict, 1);
			auto var = g_variant_get_variant(tmp);
			m_notifying = g_variant_get_boolean(var);
			g_variant_unref(tmp);
			tmp = nullptr;
			g_variant_unref(var);
			var = nullptr;
		}
		else if (property == PropertyValue)
		{
			char c;
			GVariantIter variantIter;
			tmp = g_variant_get_child_value(dict, 1);
			auto var = g_variant_get_variant(tmp);
			g_variant_iter_init(&variantIter, var);
			int size = g_variant_iter_n_children(&variantIter);
			std::vector<unsigned char> value(size);
			size = 0;
			while(g_variant_iter_loop(&variantIter, "y", &c))
			{
				value[size++] = c;
			}

			g_variant_unref(tmp);
			tmp = nullptr;

			g_variant_unref(var);
			var = nullptr;

			m_value = value;

			auto serviceInterface = serviceInterfaceGet ();

			if (serviceInterface)
			{
				auto deviceInterface = serviceInterface->deviceInterfaceGet ();

				if (deviceInterface)
				{
					valueChangedSignal.Emit (deviceInterface, m_uuid, m_value);
				}
			}
		}
		g_variant_unref(dict);
		dict = nullptr;
	}
}


void SstiGattCharacteristic::removed ()
{
	auto serviceInterface = serviceInterfaceGet ();

	if (serviceInterface)
	{
		serviceInterface->characteristicRemove (std::static_pointer_cast<SstiGattCharacteristic>(shared_from_this ()));
	}
}


void SstiGattCharacteristic::read (
	const std::shared_ptr<SstiBluezDevice> &device)
{
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	GVariant *dict = g_variant_new("a{sv}", builder);
	g_variant_builder_unref(builder);
	builder = nullptr;

	stiASSERT (m_gattCharacteristicProxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_read_value(
			m_gattCharacteristicProxy,
			dict,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattCharacteristic::eventReadFinish, this, device));
}


void SstiGattCharacteristic::eventReadFinish (
	GObject *proxy,
	GAsyncResult *res,
	const std::shared_ptr<SstiBluezDevice> &device)
{
	DbusError dbusError;
	GVariant *outValue = nullptr;
	GVariantIter variantIter;
	int size;
	guchar c;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::string characteristicUuid = csti_bluetooth_gatt_characteristic1_get_uuid (reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(proxy));

	stiASSERT (proxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_read_value_finish (
			reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(proxy), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			&outValue,
			res,
			dbusError.get ());

	if (dbusError.valid ())
	{
		readWriteFailedSignal.Emit (characteristicUuid);
		stiASSERTMSG (false, "%s: failed read: %s\n", __func__, dbusError.message().c_str ());
	}
	else
	{
		stiASSERT (proxy != nullptr);

		g_variant_iter_init(&variantIter, outValue);
		size = g_variant_iter_n_children(&variantIter);
		std::vector<unsigned char> finalValue(size);
		size = 0;
		while(g_variant_iter_loop(&variantIter, "y", &c))
		{
			finalValue[size++] = c;
		}

		m_value = finalValue;

		readWriteCompletedSignal.Emit (device, characteristicUuid);
	}
}


void SstiGattCharacteristic::write (
	const std::shared_ptr<SstiBluezDevice> &device,
	const char *value,
	const int len)
{
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

	for (int i = 0; i < len; i++)
	{
		g_variant_builder_add(builder, "y", value[i]);
	}

	GVariant *argValue = g_variant_new("ay", builder);
	g_variant_builder_unref(builder);
	builder = nullptr;

	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	GVariant *dict = g_variant_new("a{sv}", builder);
	g_variant_builder_unref(builder);
	builder = nullptr;

	stiASSERT (m_gattCharacteristicProxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_write_value(
			m_gattCharacteristicProxy,
			argValue,
			dict,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattCharacteristic::eventWriteFinish, this, device));
}


void SstiGattCharacteristic::eventWriteFinish (
	GObject *object,
	GAsyncResult *res,
	const std::shared_ptr<SstiBluezDevice> &device)
{
	DbusError dbusError;
	auto proxy = reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(object); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	stiASSERT (proxy != nullptr);

	std::string characteristicUuid = csti_bluetooth_gatt_characteristic1_get_uuid (proxy);

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace("%s: enter: object name: %s\n", __func__, device->bluetoothDevice.name.c_str());
	);

	csti_bluetooth_gatt_characteristic1_call_write_value_finish (
			proxy,
			res,
			dbusError.get ());

	if (dbusError.valid ())
	{
		readWriteFailedSignal.Emit (characteristicUuid);
		stiASSERTMSG (false, "%s: failed during write: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		readWriteCompletedSignal.Emit (device, characteristicUuid);
	}
}


stiHResult SstiGattCharacteristic::WriteQueuePacketsRemove (
	const std::string value)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::mutex> lock (m_writingMutex);

	stiTESTCOND (!value.empty(), stiRESULT_ERROR);

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace("WriteQueuePacketsRemove: packets before remove %d\n", m_writePacketQueue.size ());
	);

	m_writePacketQueue.erase (std::remove_if (m_writePacketQueue.begin (),
												m_writePacketQueue.end (),
												[value](std::string& packet)
												{return ((packet.find (value) != std::string::npos));}),
												m_writePacketQueue.end ());

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace("WriteQueuePacketsRemove: packets after remove %d\n", m_writePacketQueue.size ());
	);
STI_BAIL:

	return (hResult);
}


void SstiGattCharacteristic::WriteFinish (
	GObject *object,
	GAsyncResult *res,
	gpointer UserData)
{
	DbusError dbusError;
	bool writingAgain = false;
	auto proxy = reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(object); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	auto characteristic = static_cast<SstiGattCharacteristic*>(UserData);

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace("WriteFinish: service: %s\n", characteristic->m_servicePath.c_str());
	);

	std::lock_guard<std::mutex> lock (characteristic->m_writingMutex);

	stiASSERT (proxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_write_value_finish (
			proxy,
			res,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: failed during write: %s\n", __func__, dbusError.message ().c_str ());
	}
	else if (!characteristic->m_writePacketQueue.empty ())
	{
		std::string packet = characteristic->m_writePacketQueue.front ();
		characteristic->m_writePacketQueue.pop_front ();

		GVariantBuilder *builder;
		GVariant *argValue = nullptr;
		GVariant *dict = nullptr;

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("WriteFinish: writing a queued packet to service %s: value %s\n",
				characteristic->m_servicePath.c_str(), packet.c_str());
		);

		builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

		for (int i = 0; i < (int)packet.length () + 1; i++)
		{
			g_variant_builder_add (builder, "y", packet[i]);
		}

		argValue = g_variant_new ("ay", builder);
		g_variant_builder_unref (builder);
		builder = nullptr;

		builder = g_variant_builder_new (G_VARIANT_TYPE("a{sv}"));
		dict = g_variant_new ("a{sv}", builder);
		g_variant_builder_unref (builder);
		builder = nullptr;

		//should already be true
		writingAgain = true;

		stiASSERT (characteristic->m_gattCharacteristicProxy != nullptr);

		// Async write
		csti_bluetooth_gatt_characteristic1_call_write_value (
				characteristic->m_gattCharacteristicProxy,
				argValue,
				dict,
				nullptr,
				WriteFinish,
				characteristic);
	}

	if (!writingAgain)
	{
		characteristic->m_writing = false;
	}
}


stiHResult SstiGattCharacteristic::Write (
	const std::string value)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::mutex> lock (m_writingMutex);

	stiTESTCOND (!value.empty(), stiRESULT_ERROR);

	if (m_writing)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("Write: queueinq to service: %s: uuid %s: value %s\n",
				m_servicePath.c_str (), m_uuid.c_str (), value.c_str ());
		);

		m_writePacketQueue.push_back (value);

		if (m_writePacketQueue.size () > ASSERT_AFTER_THIS_MANY_QUEUED_PACKETS)
		{
			stiASSERTMSG (false, "Warning: gatt write queue over %d to service %s: uuid %s\n",
						  ASSERT_AFTER_THIS_MANY_QUEUED_PACKETS,
						  m_servicePath.c_str (),
						  m_uuid.c_str());
		}
	}
	else
	{
		GVariantBuilder *builder;
		GVariant *argValue = nullptr;
		GVariant *dict = nullptr;

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("Write: writing to service: %s: uuid %s: value %s\n",
				m_servicePath.c_str (), m_uuid.c_str (), value.c_str ());
		);

		if (!m_writePacketQueue.empty())
		{
			stiASSERTMSG (false, "Warning: gatt write queue is not empty on service %s: uuid %s\n",
						  m_servicePath.c_str (),
						  m_uuid.c_str());
		}

		builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

		for (int i = 0; i < (int)value.length () + 1; i++)
		{
			g_variant_builder_add (builder, "y", value[i]);
		}

		argValue = g_variant_new("ay", builder);
		g_variant_builder_unref(builder);
		builder = nullptr;

		builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
		dict = g_variant_new("a{sv}", builder);
		g_variant_builder_unref(builder);
		builder = nullptr;

		m_writing = true;

		stiASSERT (m_gattCharacteristicProxy != nullptr);

		// Async write
		csti_bluetooth_gatt_characteristic1_call_write_value (
				m_gattCharacteristicProxy,
				argValue,
				dict,
				nullptr,
				WriteFinish,
				this);
	}

STI_BAIL:

	m_writeSequenceNumber++;

	return (hResult);

}


void SstiGattCharacteristic::notifyStart ()
{
	stiASSERT (m_gattCharacteristicProxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_start_notify (
			m_gattCharacteristicProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattCharacteristic::eventStartNotifyFinish, this));
}


void SstiGattCharacteristic::eventStartNotifyFinish(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiASSERT (proxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_start_notify_finish (
			reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(proxy), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			res,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: failed: %s\n", __func__, dbusError.message ().c_str ());
		startNotifyFailedSignal.Emit ();
	}
	else
	{
		auto serviceInterface = serviceInterfaceGet();

		if (serviceInterface)
		{
			auto deviceInterface = serviceInterface->deviceInterfaceGet ();

			if (deviceInterface)
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				std::string characteristicUuid = csti_bluetooth_gatt_characteristic1_get_uuid (reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(proxy));
				startNotifyCompletedSignal.Emit (deviceInterface->bluetoothDevice.address, characteristicUuid);
			}
			else
			{
				stiASSERT (false);
			}
		}
		else
		{
			stiASSERT (false);
		}
	}
}


void SstiGattCharacteristic::notifyStop ()
{
	stiASSERT (m_gattCharacteristicProxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_stop_notify(
			m_gattCharacteristicProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattCharacteristic::eventStopNotifyFinish, this));
	}


void SstiGattCharacteristic::eventStopNotifyFinish(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiASSERT (proxy != nullptr);

	csti_bluetooth_gatt_characteristic1_call_stop_notify_finish (
		reinterpret_cast<CstiBluetoothGattCharacteristic1 *>(proxy), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		res,
		dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: failed: %s\n", __func__, dbusError.message().c_str ());
	}
}

