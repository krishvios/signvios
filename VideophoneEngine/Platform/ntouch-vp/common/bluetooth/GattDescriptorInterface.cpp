// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "GattDescriptorInterface.h"
#include "GattServiceInterface.h"
#include "GattCharacteristicInterface.h"
#include "DeviceInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "stiTrace.h"


std::shared_ptr<SstiGattCharacteristic> SstiGattDescriptor::characteristicInterfaceGet ()
{
	auto parentDbusObject = m_dbusObject->dbusConnectionGet ()->find (m_characteristicPath);

	if (parentDbusObject)
	{
		auto characteristicInterface = std::static_pointer_cast<SstiGattCharacteristic>(parentDbusObject->interfaceGet (GATT_CHARACTERISTIC_INTERFACE));

		if (characteristicInterface)
		{
			return characteristicInterface;
		}
		else
		{
			stiASSERTMSG (false, "No characteristic interface found at path %s\n", m_characteristicPath.c_str ());
		}
	}
	else
	{
		stiASSERTMSG (false, "No object found at path %s\n", m_characteristicPath.c_str ());
	}

	return nullptr;
}


/*
 *\ brief add a new descriptor
 *
 */
void SstiGattDescriptor::added (
	GVariant *properties)
{
	m_characteristicPath = stringVariantGet (properties, "Characteristic");
	m_uuid = stringVariantGet (properties, "UUID");

	GVariant *tmp = g_variant_lookup_value (properties, PropertyValue.c_str (), nullptr);
	if (tmp)
	{
		char c;
		GVariantIter variantIter;
		g_variant_iter_init (&variantIter, tmp);
		int size = g_variant_iter_n_children (&variantIter);
		std::vector<unsigned char> value (size);
		size = 0;
		while(g_variant_iter_loop (&variantIter, "y", &c))
		{
			value[size++] = c;
		}

		m_value = value;

		g_variant_unref (tmp);
		tmp = nullptr;
	}

	auto characteristicInterface = characteristicInterfaceGet ();

	if (characteristicInterface)
	{
		characteristicInterface->descriptorAdd (std::static_pointer_cast<SstiGattDescriptor>(shared_from_this ()));
	}
	else
	{
		stiASSERT (false);
	}

	csti_bluetooth_gatt_descriptor1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiGattDescriptor::eventProxyNew, this));
}


void SstiGattDescriptor::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace ("Object %s: Interface %s: Finished proxy creation\n", objectPathGet().c_str (), interfaceNameGet ().c_str ());
	);

	m_gattDescriptorProxy = csti_bluetooth_gatt_descriptor1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: error making characteristic proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		// Allow device to maintain Helios readiness by passing associated characteristic UUID
		auto characteristic = characteristicInterfaceGet ();
		if (characteristic)
		{
			auto service = characteristic->serviceInterfaceGet ();

			if (service)
			{
				auto device = service->deviceInterfaceGet ();

				if (device)
				{
					device->descriptorAdd (characteristic->uuidGet ());
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
		else
		{
			stiASSERT (false);
		}
	}
}


void SstiGattDescriptor::changed(
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	int lim = g_variant_n_children (changedProperties);
	for (int i = 0; i < lim; i++)
	{
		GVariant *dict = nullptr;
		GVariant *tmp = nullptr;
		const char *property = nullptr;

		dict = g_variant_get_child_value (changedProperties, i);
		tmp = g_variant_get_child_value (dict, 0);
		property = g_variant_get_string (tmp, nullptr);
		g_variant_unref (tmp);
		tmp = nullptr;

		if (property == PropertyValue)
		{
			char c;
			GVariantIter variantIter;
			tmp = g_variant_get_child_value (dict, 1);
			auto var = g_variant_get_variant (tmp);
			g_variant_iter_init (&variantIter, var);
			int size = g_variant_iter_n_children (&variantIter);
			std::vector<unsigned char> value (size);
			size = 0;
			while (g_variant_iter_loop (&variantIter, "y", &c))
			{
				value[size++] = c;
			}

			g_variant_unref (tmp);
			tmp = nullptr;

			g_variant_unref (var);
			var = nullptr;

			m_value = value;

#if 0
			auto characteristicInterface = characteristicInterfaceGet ();

			if (characteristicInterface)
			{
				auto serviceInterface = characteristicInterface->serviceInterfaceGet ();

				if (serviceInterface)
				{
					auto deviceInterface = serviceInterface->deviceInterfaceGet ();

					if (deviceInterface)
					{
						valueChangedSignal.Emit (deviceInterface->bluetoothDevice.address, m_uuid);
					}
				}
			}
#endif
		}

		g_variant_unref (dict);
		dict = nullptr;
	}
}


void SstiGattDescriptor::removed ()
{
	auto characteristicInterface = characteristicInterfaceGet ();

	if (characteristicInterface)
	{
		characteristicInterface->descriptorRemove (std::static_pointer_cast<SstiGattDescriptor>(shared_from_this ()));
	}
}
