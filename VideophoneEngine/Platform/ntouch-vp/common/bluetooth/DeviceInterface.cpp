// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "DeviceInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "BluetoothDefs.h"
#include "stiTrace.h"


CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::addedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::removedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::pairingCompleteSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::pairingFailedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::pairingCanceledSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::pairingInputTimeoutSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::connectedSignal;
CstiSignal<> SstiBluezDevice::connectFailedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::disconnectedSignal;
//CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::pairedSignal;
CstiSignal<std::shared_ptr<SstiBluezDevice>> SstiBluezDevice::disconnectCompletedSignal;
CstiSignal<> SstiBluezDevice::disconnectFailedSignal;
CstiSignal<> SstiBluezDevice::aliasChangedSignal;


/*
 * \brief add a new device
 */
void SstiBluezDevice::added(
	GVariant *properties)
{
	bluetoothDevice.objectPath = objectPathGet();
	propertiesProcess (properties);

	// Set up bluez proxies and signal handlers
	csti_bluetooth_device1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiBluezDevice::eventProxyNew, this));
}


void SstiBluezDevice::eventProxyNew (
	GObject *proxy,
	GAsyncResult *pRes)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace ("Object %s: Interface %s: Finished proxy creation\n", objectPathGet().c_str (), interfaceNameGet ().c_str ());
	);

	m_deviceProxy = csti_bluetooth_device1_proxy_new_finish (pRes, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: Can't create property proxy for %s: %s\n", __func__, bluetoothDevice.name.c_str(), dbusError.message ().c_str ());
	}
	else
	{
		addedSignal.Emit(std::static_pointer_cast<SstiBluezDevice>(shared_from_this()));

		if ((isHelios && isHeliosReady ())
		 || (bluetoothDevice.name == HELIOS_DFU_TARGET_NAME && isHeliosDfuTargetReady ())
		 || (!isHelios && bluetoothDevice.name != HELIOS_DFU_TARGET_NAME && bluetoothDevice.trusted && bluetoothDevice.connected))
		{
			connectedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this ()));
		}
	}
}


/*
 * \brief remove a device
 */
void SstiBluezDevice::removed ()
{
	removedSignal.Emit(std::static_pointer_cast<SstiBluezDevice>(shared_from_this()));
}

/*
 *\ brief  determine if the bluetooth device is supported
 *
 * returns true if the device is supported, else false
 */
bool SstiBluezDevice::deviceSupported()
{
	bool returnValue = false;

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		stiTrace("Device Supported - major = %d, minor = %d\n", bluetoothDevice.majorDeviceClass, bluetoothDevice.minorDeviceClass);
	);

	switch (bluetoothDevice.majorDeviceClass)
	{
		case BluetoothDevice::eAudioVideo:
			switch (bluetoothDevice.minorDeviceClass)
			{
				case BluetoothDevice::eWearableHeadset:
				case BluetoothDevice::eHeadphones:
				case BluetoothDevice::eLoudspeaker:
				case BluetoothDevice::eHandsFreeDevice:
					returnValue = true;
					break;
				default:
					break;
			}
			break;
		case BluetoothDevice::ePeripheral:
			switch (bluetoothDevice.minorDeviceClass)
			{
				case BluetoothDevice::eKeyboard:
				case BluetoothDevice::eComboDevice:
					returnValue = true;
					break;
				default:
					break;
			}
			break;
		case BluetoothDevice::eMiscellaneous:
		case BluetoothDevice::eComputer:
		case BluetoothDevice::ePhone:
		case BluetoothDevice::eLanNAP:
		case BluetoothDevice::eImaging:
		case BluetoothDevice::eWearable:
		case BluetoothDevice::eToy:
		case BluetoothDevice::eHealth:
		case BluetoothDevice::eUncategorizedMajor:
			break;
	}

	if (!returnValue && !bluetoothDevice.uuids.empty())
	{
		bool mayBeHelios = false;
		for (auto &uuid : bluetoothDevice.uuids)
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
				stiTrace("checking uuid %s\n", uuid.c_str());
			);

			long device = strtol(uuid.substr(4,4).c_str(), nullptr, 16);

			// TODO: audio devices can support a2dp and HSP and HFP
			// TODO: need to be able to represent this?  It can be an ordering problem...

			switch (device)
			{					// https://www.bluetooth.com/specifications/assigned-numbers/service-discovery
				case 0x0011:	// Human Interface Device Profile (HID)
				case 0x1124:	// Human Interface Device (HID) -  NOTE: Used as both Service Class Identifier and Profile Identifier.
					returnValue = true;
					break;
				case 0x1812:	// hid device    https://www.bluetooth.com/specifications/gatt/services
					returnValue = true;
					mayBeHelios = true;
					break;
				case 0x110b:	// Advanced Audio Distribution Profile (A2DP)
				case 0x1203:	// generic audio
					bluetoothDevice.majorDeviceClass = BluetoothDevice::eAudioVideo;
					bluetoothDevice.minorDeviceClass = BluetoothDevice::eLoudspeaker;
					returnValue = true;
					break;
				case 0x1108:	// headset
				case 0x1112:	// Headset Profile (HSP)
				case 0x1131:	// headset
					bluetoothDevice.majorDeviceClass = BluetoothDevice::eAudioVideo;
					bluetoothDevice.minorDeviceClass = BluetoothDevice::eWearableHeadset;
					returnValue = true;
					break;
				case 0x111e:	// Hands-free Profile (HFP)
				case 0x111f:	// Hands-free Profile (HFP)
					bluetoothDevice.majorDeviceClass = BluetoothDevice::eAudioVideo;
					bluetoothDevice.minorDeviceClass = BluetoothDevice::eHandsFreeDevice;
					returnValue = true;
					break;
			}

			if (returnValue && !mayBeHelios)
			{
				break;
			}
			else if (uuid == PULSE_SERVICE_UUID)
			{
				isHelios = true;
				returnValue = true;
				bluetoothDevice.autoReconnect = true;
				break;
			}
		}
	}

	isSupported = returnValue;

	return returnValue;
}


void SstiBluezDevice::propertiesProcess (
	GVariant *changedProperties)
{
	int limit = g_variant_n_children(changedProperties);

	for (int i = 0; i < limit; i++)
	{
		GVariant *dict = nullptr;
		GVariant *tmp = nullptr;
		const char *property = nullptr;

		dict = g_variant_get_child_value(changedProperties, i);
		tmp = g_variant_get_child_value(dict, 0);
		property = g_variant_get_string(tmp, nullptr);
		g_variant_unref(tmp);
		tmp = nullptr;

		auto booleanPropertyGet = [&dict](){

										GVariant *variant = g_variant_get_child_value (dict, 1);
										auto var = g_variant_get_variant (variant);
										auto property = g_variant_get_boolean (var);
										g_variant_unref (variant);
										variant = nullptr;
										g_variant_unref (var);
										var = nullptr;
										return property;
									};

		if (property == PropertyPaired)
		{
			bluetoothDevice.paired = booleanPropertyGet ();
		}
		else if (property == PropertyConnected)
		{
			bluetoothDevice.connected = booleanPropertyGet ();

			if (bluetoothDevice.connected)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Device connected\n");
				);
			}
			else
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace ("Device disconnected\n");
				);

				disconnectedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this ()));
			}
		}
// we will only ever use alias from now on.  From the bluez5 documentation:
// "In case no alias is set, it will return the remote device name."
// so alias will either return the remote device name, or the name we set it to.
//

		else if (property == PropertyName)
		{
#if 0
			tmp = g_variant_get_child_value(dict, 1);
			gsize length;
			auto var = g_variant_get_variant(tmp);
			std::string name = g_variant_get_string(var, &length);
			g_variant_unref(tmp);
			tmp = nullptr;

			if (length)
			{
				bluetoothDevice.name = name;
			}

			g_variant_unref (var);
			var = nullptr;
#endif
			defaultName = stringVariantGet (changedProperties, PropertyName);
		}
		else if (property == PropertyUUIDs)
		{
			std::list<std::string> uuidList;
			const gchar **uuids = nullptr;
			gsize size = 0;
			tmp = g_variant_get_child_value(dict, 1);
			auto var = g_variant_get_variant(tmp);
			uuids = g_variant_get_strv(var, &size);
			g_variant_unref(tmp);
			tmp = nullptr;

			for (gsize i = 0; i < size; i++)
			{
				uuidList.push_back(uuids[i]);
			}

			g_free(uuids);
			uuids = nullptr;

			g_variant_unref(var);
			var = nullptr;

			bluetoothDevice.uuids.clear();
			bluetoothDevice.uuids = uuidList;
		}
		else if (property == PropertyTrusted)
		{
			bluetoothDevice.trusted = booleanPropertyGet ();
		}
		else if (property == PropertyServicesResolved)
		{
			servicesResolved = booleanPropertyGet ();
		}
		else if (property == PropertyAlias)
		{
			bluetoothDevice.name = stringVariantGet (changedProperties, PropertyAlias);
			if (m_aliasChange)
			{
				aliasChangedSignal.Emit();
				m_aliasChange = false;
			}
		}
		else if (property == PropertyAddress)
		{
			bluetoothDevice.address = stringVariantGet(changedProperties, PropertyAddress);
		}
		else if (property == PropertyIcon)
		{
			bluetoothDevice.icon = stringVariantGet(changedProperties, PropertyIcon);
		}
		else if (property == PropertyClass)
		{
			GVariant *tmpVariant = g_variant_lookup_value(changedProperties, PropertyClass.c_str (), nullptr);

			if (tmpVariant)
			{
				bluetoothDevice.majorDeviceClass = static_cast<BluetoothDevice::EBluetoothMajorClass>(
					(g_variant_get_uint32(tmpVariant) & 0x01F00) >> 8
					);
				bluetoothDevice.minorDeviceClass = static_cast<int>((g_variant_get_uint32(tmpVariant) & 0x000FF) >> 2);
				g_variant_unref (tmpVariant);
				tmpVariant = nullptr;
			}
		}
		else if (property == PropertyBlocked)
		{
			bluetoothDevice.blocked =  booleanPropertyGet ();
		}
		else if (property == PropertyRSSI)
		{
			isAdvertising = true;
		}
		else
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug > 0,
				stiTrace ("Object %s: Interface %s: Unhandled property: %s\n", objectPathGet ().c_str (), interfaceNameGet ().c_str (), property);
			);
		}

		g_variant_unref(dict);
		dict = nullptr;
	}

	// if the device is not yet marked as supported, check again since we read new property values
	if (!isSupported)
	{
		deviceSupported();
	}
}


/*
 * catch information for when a device has changed
 */
void SstiBluezDevice::changed(
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	bool trustedPrevious = bluetoothDevice.trusted;
	bool servicesResolvedPrevious = servicesResolved;
	bool connectedPrevious = bluetoothDevice.connected;

	propertiesProcess (changedProperties);

	if (
	 // If one of the three properties, trusted, servicesResolved or connected, have changed
	 // AND this is a Helios device or a Helios DFU target then consider
	 // whether or not the device is connected.
	 ((trustedPrevious != bluetoothDevice.trusted
	 || servicesResolvedPrevious != servicesResolved
	 || connectedPrevious != bluetoothDevice.connected)
	 && ((isHelios && isHeliosReady ())
	 || (bluetoothDevice.name == HELIOS_DFU_TARGET_NAME && isHeliosDfuTargetReady ())))

	 // For any other device, if one of the two properties, trusted or connected, have changed
	 // then consider whether or not the device is connected
	 ||
	 ((trustedPrevious != bluetoothDevice.trusted
	 || connectedPrevious != bluetoothDevice.connected)
	 && (!isHelios && bluetoothDevice.name != HELIOS_DFU_TARGET_NAME && bluetoothDevice.trusted && bluetoothDevice.connected)))
	{
		connectedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this ()));
	}
}


void SstiBluezDevice::pair ()
{
	stiASSERT (m_deviceProxy != nullptr);

	csti_bluetooth_device1_call_pair (
			m_deviceProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiBluezDevice::eventPairFinish, this));
}


/*
 *\ brief the paring has completed, finish setting up the device
 */
void SstiBluezDevice::eventPairFinish (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;
	auto device = std::static_pointer_cast<SstiBluezDevice>(shared_from_this());

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("eventPairFinish: enter\n");
	);

	// Remove device from the scanned list whether pairing succeeded or failed
	isAdvertising = false;

	stiASSERT (m_deviceProxy != nullptr);

	csti_bluetooth_device1_call_pair_finish (
		m_deviceProxy,
		res,
		dbusError.get ());

	if (dbusError.valid ())
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("error return message = %s\n", dbusError.message ().c_str ());
		);

		if (strstr(dbusError.message ().c_str (), "Authentication Canceled"))
		{
			stiTrace("canceled during CreatePairedDevice: %s\n", dbusError.message ().c_str ());
			pairingCanceledSignal.Emit (device);
		}
		else if (strstr(dbusError.message ().c_str (), "Timeout was reached"))
		{
			pairingInputTimeoutSignal.Emit (device);
		}
		else
		{
			stiTrace("other pairing error: %s\n", dbusError.message ().c_str ());
			pairingFailedSignal.Emit (device);
		}
	}
	else
	{
		pairingCompleteSignal.Emit(device);
	}
}


void SstiBluezDevice::pairCancel ()
{
	stiASSERT (m_deviceProxy != nullptr);

	csti_bluetooth_device1_call_cancel_pairing (
			m_deviceProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiBluezDevice::eventPairCancel, this));
}


/*
 *\ brief - dbus had finished initiating the paring cancellation
 */
void SstiBluezDevice::eventPairCancel(
	GObject *proxy,
	GAsyncResult *pRes)
{
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("enter eventPairCancel calling finish\n");
	);

	stiASSERT (proxy != nullptr);

	csti_bluetooth_device1_call_cancel_pairing_finish (
			reinterpret_cast<CstiBluetoothDevice1 *>(proxy), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "Error canceling pair: %s\n", dbusError.message ().c_str ());
	}
}



void SstiBluezDevice::connect ()
{
	m_connecting = true;

	stiASSERT (m_deviceProxy != nullptr);

	csti_bluetooth_device1_call_connect(
		m_deviceProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiBluezDevice::eventConnectFinish, this));
}


/*
 *\ brief - finish setting up the connection
 */
void SstiBluezDevice::eventConnectFinish (
	GObject *proxy,
	GAsyncResult *pRes)
{
	auto deviceProxy = reinterpret_cast<CstiBluetoothDevice1 *>(proxy); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("enter eventConnectFinish\n");
	);

	m_connecting = false;

	stiASSERT (deviceProxy != nullptr);

	csti_bluetooth_device1_call_connect_finish (
			deviceProxy,
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("EventConnectFinish: error %s\n", dbusError.message ().c_str ());
		);

		connectFailedSignal.Emit ();
	}
}


void SstiBluezDevice::disconnect ()
{
	stiASSERT (m_deviceProxy != nullptr);

	csti_bluetooth_device1_call_disconnect(
			m_deviceProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &SstiBluezDevice::eventDisconnectFinish, this));
}


/*
 *\ brief - finish disconnecting the device
 */
void SstiBluezDevice::eventDisconnectFinish (
	GObject *proxy,
	GAsyncResult *pRes)
{
	auto *deviceProxy = reinterpret_cast<CstiBluetoothDevice1 *>(proxy); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	DbusError dbusError;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("enter eventDisconnectFinish\n");
	);

	stiASSERT (deviceProxy != nullptr);

	csti_bluetooth_device1_call_disconnect_finish (
			deviceProxy,
			pRes,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG(false, "EventDisconnectFinish: error %s\n", dbusError.message ().c_str ());

		disconnectFailedSignal.Emit ();
	}
	else
	{
		bluetoothDevice.connected = false;

		disconnectCompletedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this()));
	}
}

void SstiBluezDevice::rename(
	std::string newName)
{
	stiASSERT (m_deviceProxy != nullptr);

	m_aliasChange = true;
	propertySet(PropertyAlias, g_variant_new_variant(g_variant_new_string(newName.c_str())));
}
