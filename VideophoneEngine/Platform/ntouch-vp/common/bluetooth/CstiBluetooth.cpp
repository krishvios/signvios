// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "CstiBluetooth.h"
#include "CstiLogging.h"
#include "stiTrace.h"
#include <cstring>
#include <algorithm>
#include <functional>
#include <memory>
#include <iomanip>
#include "AdapterInterface.h"
#include "AgentManagerInterface.h"
#include "DeviceInterface.h"
#include "GattManagerInterface.h"
#include "GattServiceInterface.h"
#include "GattCharacteristicInterface.h"
#include "GattDescriptorInterface.h"
#include "DbusCall.h"
#include "DbusConnection.h"
#include "DbusError.h"
#include "IstiPlatform.h"
#include "stiRemoteLogEvent.h"
#include <array>

#define MAX_PAIRED_COUNT	7

/*!
 * \brief create a new bluetooth object
 */
CstiBluetooth::CstiBluetooth ()
:
	CstiEventQueue("CstiBluetooth"),
	m_scanTimer(15000, this),
	m_pairedListConnectTimer(10000, this),
	m_audio(nonstd::make_observer(this), BluetoothDefaultAdapterNameGet())
{
}

/*!
 * \brief destroy a bluetooth object
 */
CstiBluetooth::~CstiBluetooth()
{

	CstiEventQueue::StopEventLoop();
	BluetoothUninitialize ();
}


/*!
 * \brief initialize the bluetooth object
 *
 * \returns stiRESULT_SUCCESS or error code
 */
stiHResult CstiBluetooth::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Agent Manager Interface Signals
	//
	m_signalConnections.push_back (AgentManagerInterface::addedSignal.Connect (
		[this](std::shared_ptr<AgentManagerInterface> agentManagerInterface)
		{
			PostEvent ([this, agentManagerInterface]
			{
				eventAgentManagerInterfaceAdded (agentManagerInterface);
			});
		}));

	m_signalConnections.push_back (AgentManagerInterface::agentRegistrationFailedSignal.Connect (
		[this]()
		{
			PostEvent ([this]
			{
				eventAgentRegistrationFailed ();
			});
		}));

	//
	// Gatt Manager Interface Signals
	//
	m_signalConnections.push_back (GattManagerInterface::addedSignal.Connect (
		[this](std::shared_ptr<GattManagerInterface> gattManagerInterface)
		{
			gattManagerInterface->autoReconnectSetup (m_appObjectManagerServer);
		}));

	// Audio related platform signals
	m_signalConnections.push_back (IstiPlatform::InstanceGet()->callStateChangedSignalGet ().Connect(
		[this](const PlatformCallStateChange &stateChange)
		{
			PostEvent ([this, stateChange]
				{
					m_audio.callStateUpdate(stateChange);
				});
		}));

	deviceInterfaceSignalsConnect ();

	adapterInterfaceSignalsConnect ();

	BluetoothInitialize ();

//STI_BAIL:

	return (hResult);
}


void CstiBluetooth::deviceInterfaceSignalsConnect ()
{
	//
	// Device Interface Signals
	//
	m_signalConnections.push_back (SstiBluezDevice::addedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			PostEvent ([this, deviceInterface]
			{
				eventDeviceInterfaceAdded (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::removedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			PostEvent ([this, deviceInterface]
			{
				eventDeviceInterfaceRemoved (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::pairingCompleteSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			PostEvent ([this, deviceInterface]
			{
				eventDevicePairingComplete (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::pairingFailedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			pairingFailedSignal.Emit ();

			PostEvent ([this, deviceInterface]
			{
				eventDevicePairingStatus (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::pairingCanceledSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			pairingCanceledSignal.Emit ();

			PostEvent ([this, deviceInterface]
			{
				eventDevicePairingStatus (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::pairingInputTimeoutSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			pairingInputTimeoutSignal.Emit ();

			PostEvent ([this, deviceInterface]
			{
				eventDevicePairingStatus (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::connectedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			PostEvent ([this, deviceInterface]
			{
				eventDeviceConnected (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::disconnectedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			PostEvent ([this, deviceInterface]
			{
				eventDeviceDisconnected (deviceInterface);
			});
		}));

	m_signalConnections.push_back (SstiBluezDevice::connectFailedSignal.Connect (
		[this]()
		{
			deviceConnectFailedSignal.Emit();
			if (!m_connectList.empty())
			{
				PostEvent([this]{connectPairedDevice();});
			}
		}));

	m_signalConnections.push_back (SstiBluezDevice::disconnectFailedSignal.Connect (
		[this]()
		{
			deviceDisconnectFailedSignal.Emit();
		}));

	m_signalConnections.push_back (SstiBluezDevice::disconnectCompletedSignal.Connect (
		[this](std::shared_ptr<SstiBluezDevice> deviceInterface)
		{
			deviceDisconnectedSignal.Emit(deviceInterface->bluetoothDevice);
		}));

	m_signalConnections.push_back(SstiBluezDevice::aliasChangedSignal.Connect (
		[this] ()
		{
			deviceAliasChangedSignal.Emit();
		}));
}


void CstiBluetooth::adapterInterfaceSignalsConnect ()
{
	//
	// Adapter Interface signals
	//
	m_signalConnections.push_back (AdapterInterface::addedSignal.Connect (
		[this](std::shared_ptr<AdapterInterface> adapterInterface)
		{
			PostEvent ([this, adapterInterface]
			{
				eventAdapterInterfaceAdded (adapterInterface);
			});
		}));

	m_signalConnections.push_back (AdapterInterface::removedSignal.Connect (
		[this](std::shared_ptr<AdapterInterface> adapterInterface)
		{
			PostEvent ([this, adapterInterface]
			{
				eventAdapterInterfaceRemoved (adapterInterface);
			});
		}));

	m_signalConnections.push_back (AdapterInterface::scanCompletedSignal.Connect (
		[this]()
		{
			scanCompleteSignal.Emit();
		}));

	m_signalConnections.push_back (AdapterInterface::scanFailedSignal.Connect (
		[this]()
		{
			PostEvent ([this]
			{
				eventAdapterScanFailed ();
			});
		}));

	m_signalConnections.push_back (AdapterInterface::removeDeviceCompletedSignal.Connect (
		[this]()
		{
			unpairCompleteSignal.Emit();
		}));

	m_signalConnections.push_back (AdapterInterface::removeDeviceFailedSignal.Connect (
		[this]()
		{
			unpairFailedSignal.Emit();
		}));

	m_signalConnections.push_back (AdapterInterface::poweredOnSignal.Connect (
		[this](std::shared_ptr<AdapterInterface> adapterInterface)
		{
			m_audio.HCIAudioEnable();
		}));
}

/*
 * initialize the bluetooth object
 */
stiHResult CstiBluetooth::BluetoothInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	DbusError dbusError;

	auto dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, dbusError.get ());
	stiTESTCOND (dbusConnection, stiRESULT_ERROR);

	m_dbusConnection = std::make_shared<DbusConnection> (dbusConnection);

	// Create application object manager server...used to export
	// application objects to dbus at /ntouch
	m_appObjectManagerServer = g_dbus_object_manager_server_new ("/ntouch");

	// Add the object manager server to the bus
	g_dbus_object_manager_server_set_connection (m_appObjectManagerServer, m_dbusConnection->connectionGet ());

	/*
	 * create object manager client
	 */
	m_objectManagerProxy = csti_bluetooth_object_manager_client_new_sync (
			m_dbusConnection->connectionGet (),
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			BLUETOOTH_MANAGER_PATH,
			nullptr,
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiTHROWMSG (stiRESULT_ERROR, "Can't create Bluetooth Object Manager proxy - %s\n", dbusError.message ().c_str ());
	}

	/*
	 * set up signal handling so that we can catch them later
	 */
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	m_objectAddedSignalHandlerId = g_signal_connect (
			m_objectManagerProxy,
			"object-added",
			G_CALLBACK (objectAddedCallback),
			this);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	m_objectRemovedSignalHandlerId = g_signal_connect (
			m_objectManagerProxy,
			"object-removed",
			G_CALLBACK (objectRemovedCallback),
			this);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	m_interfaceAddedSignalHandlerId = g_signal_connect (
			m_objectManagerProxy,
			"interface-added",
			G_CALLBACK (interfaceAddedCallback),
			this);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	m_interfaceRemovedSignalHandlerId = g_signal_connect (
			m_objectManagerProxy,
			"interface-removed",
			G_CALLBACK (interfaceRemovedCallback),
			this);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	m_interfacePropertyChangedId = g_signal_connect (
			m_objectManagerProxy,
			"interface-proxy-properties-changed",
			G_CALLBACK (interfacePropertiesChangedCallback),
			this);

	m_signalConnections.push_back (m_scanTimer.timeoutSignal.Connect (
			[this]() {
				ScanCancel();
			}));

	m_signalConnections.push_back(m_pairedListConnectTimer.timeoutSignal.Connect(
			[this]() {
				eventConnectPairedDevices();
			}));
				
STI_BAIL:

	return hResult;
}


/*
 * shut down the bluetooth object
 */
void CstiBluetooth::BluetoothUninitialize ()
{
	if (m_objectManagerProxy)
	{
		if (m_objectAddedSignalHandlerId)
		{
			g_signal_handler_disconnect (m_objectManagerProxy, m_objectAddedSignalHandlerId);
			m_objectAddedSignalHandlerId = 0;
		}

		if (m_objectRemovedSignalHandlerId)
		{
			g_signal_handler_disconnect (m_objectManagerProxy, m_objectRemovedSignalHandlerId);
			m_objectRemovedSignalHandlerId = 0;
		}

		if (m_interfaceAddedSignalHandlerId)
		{
			g_signal_handler_disconnect (m_objectManagerProxy, m_interfaceAddedSignalHandlerId);
			m_interfaceAddedSignalHandlerId = 0;
		}

		if (m_interfaceRemovedSignalHandlerId)
		{
			g_signal_handler_disconnect (m_objectManagerProxy, m_interfaceRemovedSignalHandlerId);
			m_interfaceRemovedSignalHandlerId = 0;
		}

		if (m_interfacePropertyChangedId)
		{
			g_signal_handler_disconnect (m_objectManagerProxy, m_interfacePropertyChangedId);
			m_interfacePropertyChangedId = 0;
		}

		g_object_unref(m_objectManagerProxy);
		m_objectManagerProxy = nullptr;
	}

	agentUnregister ();

	if (m_appObjectManagerServer)
	{
		g_object_unref (m_appObjectManagerServer);
		m_appObjectManagerServer = nullptr;
	}

	m_deviceList.clear();
}


void CstiBluetooth::eventManagedObjectsGet ()
{
	if (m_objectManagerProxy)
	{
		GList *managedObjectList = g_dbus_object_manager_get_objects (m_objectManagerProxy);

		// Object list is often not in order, i.e. characteristics are before
		// related service. Iterate through list and place in ordered map.
		std::map<std::string,GList*> objectMap;

		for (auto node = managedObjectList; node != nullptr; node = node->next)
		{
			objectMap.insert (std::pair <std::string, GList*> (
					g_dbus_object_get_object_path (static_cast<GDBusObject*>(node->data)),
					g_dbus_object_get_interfaces (static_cast<GDBusObject*>(node->data))));
		}

		for (auto &object : objectMap)
		{
			auto objectPath = object.first;
			auto dbusObject = m_dbusConnection->findOrCreate (this, objectPath);

			auto interfaceList = object.second;
			for (; interfaceList != nullptr; interfaceList = interfaceList->next)
			{
				interfaceAdd (dbusObject, static_cast<GDBusInterface*>(interfaceList->data));
			}
			g_list_free_full(object.second, g_object_unref);
		}
		g_list_free_full (managedObjectList, g_object_unref);
		m_pairedListConnectTimer.restart();
	}
	else
	{
		stiASSERT (false);
	}
}

/*!
 * brief on startup we need to attempt to connect all paired devices because when bluetoothd
 * starts on system start, it doesn't automatically reconnect any device that might be available
 */
void CstiBluetooth::eventConnectPairedDevices()
{
	PairedListGet(&m_connectList);
	connectPairedDevice();
}

void CstiBluetooth::connectPairedDevice()
{
	while (!m_connectList.empty())
	{
		BluetoothDevice curr = m_connectList.front();
		m_connectList.pop_front();

		if ((curr.paired && curr.trusted && !curr.connected) && (!m_isAudioConnected || !curr.isAudio()))
		{
			Connect(curr);
			break;
		}
	}
}


/*!
 * \brief returns a list of the devices we are already paired with
 */
stiHResult CstiBluetooth::PairedListGet(std::list<BluetoothDevice> *pPairedList)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	for (auto &device : m_deviceList)
	{
		if (device->isSupported && device->bluetoothDevice.paired)
		{
			pPairedList->push_back (device->bluetoothDevice);
		}
	}
	return stiRESULT_SUCCESS;
}

/*!
 * \brief returns a list of the last discovered remote devices
 */
stiHResult CstiBluetooth::ScannedListGet(std::list<BluetoothDevice> *pDiscoveredList)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	for (auto &device : m_deviceList)
	{
		if (device->isSupported && !device->bluetoothDevice.paired && device->isAdvertising)
		{
			pDiscoveredList->push_back (device->bluetoothDevice);
		}
	}

	return stiRESULT_SUCCESS;
}

/*!
 * \brief returns a list of all remote devices
 */
stiHResult CstiBluetooth::allDevicesListGet(std::list<BluetoothDevice> *deviceList)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	for (auto &device : m_deviceList)
	{
		deviceList->push_back (device->bluetoothDevice);
	}

	return stiRESULT_SUCCESS;
}

/*!
 * \brief returns a list of the last discovered remote devices
 */
stiHResult CstiBluetooth::ScannedListAdvertisingReset ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	for (auto &device : m_deviceList)
	{
		device->isAdvertising = false;
	}

	return stiRESULT_SUCCESS;
}

stiHResult CstiBluetooth::heliosDevicesGet(std::list<std::shared_ptr<SstiBluezDevice>> *list)
{
	if (list)
	{
		for (auto &device : m_deviceList)
		{
			if (device->bluetoothDevice.paired && device->bluetoothDevice.connected && device->isHelios)
			{
				list->push_back (device);
			}
		}
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiBluetooth::heliosPairedDevicesGet(std::list<std::shared_ptr<SstiBluezDevice>> *list)
{
	if (list)
	{
		for (auto &device : m_deviceList)
		{
			if (device->bluetoothDevice.paired && device->isHelios)
			{
				list->push_back (device);
			}
		}
	}

	return stiRESULT_SUCCESS;
}


void CstiBluetooth::objectAddedCallback (
	GDBusObjectManager *objectManager,
	GDBusObject *object,
	gpointer userData)
{
	auto self = static_cast<CstiBluetooth *>(userData);

	g_object_ref (object);

	self->PostEvent (std::bind (&CstiBluetooth::eventObjectAdded, self, object));
}


void CstiBluetooth::eventObjectAdded (GDBusObject *object)
{
	auto objectPath = g_dbus_object_get_object_path (object);
	auto dbusObject = m_dbusConnection->findOrCreate (this, objectPath);

	auto interfaceList = g_dbus_object_get_interfaces (object);
	for (; interfaceList != nullptr; interfaceList = interfaceList->next)
	{
		interfaceAdd (dbusObject, static_cast<GDBusInterface*>(interfaceList->data));
	}

	g_list_free_full (interfaceList, g_object_unref);

	g_object_unref (object);
}


void CstiBluetooth::objectRemovedCallback (
	GDBusObjectManager *objectManager,
	GDBusObject *object,
	gpointer userData)
{
	auto self = static_cast<CstiBluetooth *>(userData);

	g_object_ref (object);

	self->PostEvent (std::bind (&CstiBluetooth::eventObjectRemoved, self, object));
}


void CstiBluetooth::eventObjectRemoved (GDBusObject *object)
{
	auto objectPath = g_dbus_object_get_object_path (object);

	auto dbusObject = m_dbusConnection->find (objectPath);

	if (dbusObject)
	{
		auto interfaceList = g_dbus_object_get_interfaces (object);
		for (; interfaceList != nullptr; interfaceList = interfaceList->next)
		{
			auto interfaceInfo = g_dbus_interface_get_info (static_cast<GDBusInterface*>(interfaceList->data));
			if (interfaceInfo)
			{
				if (interfaceInfo->name)
				{
					dbusObject->interfaceRemoved (interfaceInfo->name);
				}
			}
		}

		g_list_free_full (interfaceList, g_object_unref);

		m_dbusConnection->remove(objectPath);
	}

	g_object_unref (object);
}


void CstiBluetooth::interfaceAddedCallback (
	GDBusObjectManager *objectManager,
	GDBusObject *object,
	GDBusInterface *interface,
	gpointer userData)
{
	auto self = static_cast<CstiBluetooth *>(userData);

	g_object_ref (object);
	g_object_ref (interface);

	self->PostEvent (std::bind (&CstiBluetooth::eventInterfaceAdded, self, object, interface));
}


void CstiBluetooth::eventInterfaceAdded (GDBusObject *object, GDBusInterface *interface)
{
	auto objectPath = g_dbus_object_get_object_path (object);
	auto dbusObject = m_dbusConnection->findOrCreate (this, objectPath);

	interfaceAdd (dbusObject, interface);

	g_object_unref (object);
	g_object_unref (interface);
}


/*!\brief Retrieve the paired helios device by MAC address
 *
 * \param address The MAC address of the Helios device
 */
std::shared_ptr<SstiBluezDevice> CstiBluetooth::pairedHeliosDeviceGet (
	const std::string &address)
{
	// Get a list of all current Helios devices
	std::list<std::shared_ptr<SstiBluezDevice>> heliosDevices;
	heliosPairedDevicesGet (&heliosDevices);

	// Find corresponding Helios device
	auto deviceIter = std::find_if (heliosDevices.begin (), heliosDevices.end (),
							[&address](std::shared_ptr<SstiBluezDevice> &device)
								{return device->bluetoothDevice.address == address;});

	// If found, add the DFU target name to the list of properties for this device
	if (deviceIter != heliosDevices.end ())
	{
		return *deviceIter;
	}

	return nullptr;
}


void CstiBluetooth::interfaceAdd (
	std::shared_ptr<DbusObject>dbusObject,
	GDBusInterface *interface)
{
	DbusError dbusError;

	auto interfaceInfo = g_dbus_interface_get_info (interface);

	if (interfaceInfo)
	{
		if (interfaceInfo->name)
		{
			auto interfaceProxy = g_dbus_proxy_new_sync (
					m_dbusConnection->connectionGet (),
					G_DBUS_PROXY_FLAGS_NONE,
					interfaceInfo,
					BLUETOOTH_SERVICE,
					dbusObject->objectPathGet ().c_str (),
					interfaceInfo->name,
					nullptr,
					dbusError.get ());

			if (dbusError.valid ())
			{
				vpe::trace("Could not create ", interfaceInfo->name, "interface proxy for object ", dbusObject->objectPathGet (), "\n");
			}

			GVariant *properties = nullptr;
			GVariantBuilder *builder = nullptr;
			builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));

			auto nameReceived = false;
			std::string deviceAddress;
			auto propertyInfo = interfaceInfo->properties;

			if (propertyInfo)
			{
				for (int i = 0; propertyInfo[i] != nullptr; i++)
				{
					GVariant *value = g_dbus_proxy_get_cached_property (interfaceProxy, propertyInfo[i]->name);
					if (value)
					{
						// The following two checks capture the address of a device interface and whether
						// or not a name was included in the property list. These are used solely to add
						// the DFU target name to a DFU target device because we sometimes don't get a name.
						if (strcmp (propertyInfo[i]->name, PropertyName.c_str ()) == 0)
						{
							nameReceived = true;
						}
						else if (strcmp (propertyInfo[i]->name, PropertyAddress.c_str ()) == 0)
						{
							deviceAddress = g_variant_get_string (value, nullptr);
						}

						g_variant_builder_add (builder, "{sv}", propertyInfo[i]->name, value);
						g_variant_unref (value);
					}
				}
			}

			// If this is a device interface and a name property was not received, check if this is a
			// DFU target by comparing its address to Helios devices and add the DFU target name if applicable
			if (strcmp (interfaceInfo->name, BLUETOOTH_DEVICE_INTERFACE) == 0 &&
				deviceAddress.length () == MAC_ADDR_LENGTH &&
				!nameReceived)
			{
				// Determine the address of a corresponding Helios device
				// Get last octet and subtract 1
				uint8_t lastOctet = (uint8_t)std::stoi (deviceAddress.substr (MAC_ADDR_LAST_OCTET, OCTET_CHARACTERS), nullptr, HEXADECIMAL) - 1;
				char lastOctetString[3];
				sprintf (lastOctetString, "%02X", lastOctet);

				// Replace the last octet with the newly calculated one
				deviceAddress.replace (MAC_ADDR_LAST_OCTET, OCTET_CHARACTERS, lastOctetString);

				auto heliosDevice = pairedHeliosDeviceGet (deviceAddress);

				if (heliosDevice)
				{
					g_variant_builder_add (builder, "{sv}", PropertyName.c_str (), g_variant_new_string (HELIOS_DFU_TARGET_NAME));
				}
			}

			properties = g_variant_new ("a{sv}", builder);
			g_variant_builder_unref (builder);
			builder = nullptr;

			dbusObject->interfaceAdded (interfaceInfo->name, properties, BluetoothDbusInterface::interfaceMake);

			g_variant_unref (properties);
			properties = nullptr;

			g_object_unref (interfaceProxy);
			interfaceProxy = nullptr;
		}
	}
}


void CstiBluetooth::interfaceRemovedCallback (
	GDBusObjectManager *objectManager,
	GDBusObject *object,
	GDBusInterface *interface,
	gpointer userData)
{
	auto self = static_cast<CstiBluetooth *>(userData);

	g_object_ref (object);
	g_object_ref (interface);

	self->PostEvent (std::bind (&CstiBluetooth::eventInterfaceRemoved, self, object, interface));
}


void CstiBluetooth::eventInterfaceRemoved (GDBusObject *object, GDBusInterface *interface)
{
	auto objectPath = g_dbus_object_get_object_path (object);

	auto dbusObject = m_dbusConnection->find (objectPath);

	if (dbusObject)
	{
		auto interfaceInfo = g_dbus_interface_get_info (interface);
		if (interfaceInfo)
		{
			if (interfaceInfo->name)
			{
				dbusObject->interfaceRemoved (interfaceInfo->name);
			}
		}
	}

	g_object_unref (object);
	g_object_unref (interface);
}


void CstiBluetooth::interfacePropertiesChangedCallback(
		GDBusObjectManagerClient *objectManager,
		GDBusObjectProxy *objectProxy,
		GDBusProxy *interfaceProxy,
		GVariant *changedProperties,
		gchar **invalidatedProperties,
		gpointer userData)
{
	auto self = static_cast<CstiBluetooth *>(userData);
	std::vector<std::string> invalidatedPropertiesCopy;

	auto objectPath = g_dbus_proxy_get_object_path (interfaceProxy);

	auto interfaceName = std::string (g_dbus_proxy_get_interface_name (interfaceProxy));

	// Make a copy of the invalidated properties so they can be freed before we process the event.
	if (invalidatedProperties)
	{
		for (int i = 0; invalidatedProperties[i]; i++)
		{
			invalidatedPropertiesCopy.push_back (invalidatedProperties[i]);
		}
	}

	g_variant_ref (changedProperties);

	self->PostEvent (std::bind (
						&CstiBluetooth::eventPropertyChanged,
						self,
						objectPath,
						interfaceName,
						changedProperties,
						invalidatedPropertiesCopy)
					);
}


void CstiBluetooth::eventAdapterInterfaceAdded (
	std::shared_ptr<AdapterInterface> adapterInterface)
{
	// Check to see if an adapter is already assigned. If it is, fire an assert. However,
	// regardless of it being set or not, go ahead and assign the new adapter to the
	// pointer in the bluetooth class.
	if (m_adapterInterface)
	{
		stiASSERTMSG (false, "Adapter interface already set\n");
	}

	m_adapterInterface = adapterInterface;
}


void CstiBluetooth::eventAdapterInterfaceRemoved (
	std::shared_ptr<AdapterInterface> adapterInterface)
{
	// If the adapter being removed is the one that the bluetooth adapter points
	// to then set the point to null.
	if (m_adapterInterface == adapterInterface)
	{
		m_adapterInterface = nullptr;
	}
}


void CstiBluetooth::eventAdapterScanFailed ()
{
	m_scanTimer.stop ();

	scanFailedSignal.Emit();
}


static void remoteEventLog (const std::string &reason,
	const BluetoothDevice &bluetoothDevice)
{
	// NOTE: would have also like to log version and serial number but it appears
	// not all devices have those fields populated
	IstiLogging *pLogging = IstiLogging::InstanceGet();

	pLogging->onBootUpLog("EventType=Bluetooth Reason=" + reason + 
											 " Name=" + bluetoothDevice.name + 
											 " Type=" + bluetoothDevice.typeString () + 
											 " Address=" + bluetoothDevice.address + "\n");
}

void CstiBluetooth::eventDeviceInterfaceAdded (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	m_deviceList.push_back (deviceInterface);
	deviceAddedSignal.Emit (deviceInterface->bluetoothDevice);

	// Log paired devices when building initial list of devices
	// This is for the case when a device has already been paired
	// and the phone is booting up and the device is used without being paired again
	// if the device was just now added, and it is already paired, let the user interface know
	// a new interface that's already paired should only happen on boot up
	if (deviceInterface->bluetoothDevice.paired)
	{
		pairingCompleteSignal.Emit();
		remoteEventLog ("PairedDeviceAdded", deviceInterface->bluetoothDevice);
	}
}


void CstiBluetooth::eventDeviceInterfaceRemoved (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	m_deviceList.remove (deviceInterface);

	if (deviceInterface->isSupported)
	{
		deviceRemovedSignal.Emit();
	}
}


void CstiBluetooth::eventDevicePairingComplete (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	m_pairingDevice.reset();

	deviceInterface->propertySet (PropertyTrusted, g_variant_new_variant(g_variant_new_boolean(TRUE)));
	Connect(deviceInterface->bluetoothDevice);

	pairingCompleteSignal.Emit();

	if (m_pairingState == CstiBluetooth::PairingState::Pairing)
	{
		m_pairingState = CstiBluetooth::PairingState::None;
	}

	// Keep track of when the user pairs a bluetooth device
	remoteEventLog ("DevicePaired", deviceInterface->bluetoothDevice);
}

void CstiBluetooth::eventDevicePairingStatus (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	m_pairingDevice.reset();

	if (m_pairingState == CstiBluetooth::PairingState::Pairing)
	{
		m_pairingState = CstiBluetooth::PairingState::None;
	}
}


void CstiBluetooth::eventDeviceConnected (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	if (deviceInterface->isSupported && deviceInterface->bluetoothDevice.isAudio())
	{
		m_isAudioConnected = true;
		m_audio.devicesUpdate();
	}

	deviceConnectedSignal.Emit (deviceInterface->bluetoothDevice);

	if (!m_connectList.empty())
	{
		connectPairedDevice();
	}
}


void CstiBluetooth::eventDeviceDisconnected (
	std::shared_ptr<SstiBluezDevice> deviceInterface)
{
	if (deviceInterface->bluetoothDevice.isAudio())
	{
		m_isAudioConnected = false;
	}
	deviceDisconnectedSignal.Emit (deviceInterface->bluetoothDevice);
}


stiHResult CstiBluetooth::deviceConnectByAddress (
	const std::string& deviceAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto device = deviceGetByAddress (deviceAddress);
	stiTESTCOND (device, stiRESULT_ERROR);

	PostEvent (std::bind (&CstiBluetooth::Connect, this, device->bluetoothDevice));

STI_BAIL:

	return hResult;
}


std::shared_ptr<SstiBluezDevice> CstiBluetooth::deviceGetByAddress (const std::string& deviceAddress)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto deviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
						[&deviceAddress](std::shared_ptr<SstiBluezDevice> &device)
							{return device->bluetoothDevice.address == deviceAddress;});

	if (deviceIter != m_deviceList.end ())
	{
		return *deviceIter;
	}

	return nullptr;
}


void CstiBluetooth::deviceConnectNewWithoutDiscovery (
	const std::string &deviceAddress,
	const std::string &addressType)
{
	stiASSERT (m_adapterInterface != nullptr);

	m_adapterInterface->connectDevice (deviceAddress, addressType);
}


void CstiBluetooth::eventPropertyChanged(
	const std::string objectPath,
	const std::string interfaceName,
	GVariant *changedProperties,
	const std::vector<std::string> invalidatedProperties)
{
	auto dbusObject = m_dbusConnection->findOrCreate (this, objectPath);

	stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
		vpe::trace("Object ", objectPath, ": Interface ", interfaceName, ": properties changed\n");
		propertiesDump (changedProperties, invalidatedProperties);
	);

	if (dbusObject)
	{
		dbusObject->interfaceChanged (interfaceName, changedProperties, invalidatedProperties);
	}
	else
	{
		stiASSERTMSG (false, "Dbus object not found: %s\n", objectPath.c_str ());
	}

	g_variant_unref(changedProperties);
}


/*!
 * \brief cancel an in progress scan
 */
stiHResult CstiBluetooth::ScanCancel ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("enter bluetooth scan cancel\n");
	);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	hResult = m_adapterInterface->scanCancel ();

	m_scanTimer.stop ();

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("returning from scan cancel\n");
	);
	
	return (hResult);
}


/*!
 * \brief Initiate a scan for remote devices
 */
stiHResult CstiBluetooth::Scan ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	ScannedListAdvertisingReset();

	stiTESTCOND (m_adapterInterface, stiRESULT_ERROR);

	hResult = m_adapterInterface->scanStart ();
	stiTESTRESULT ();

	m_scanTimer.restart();

STI_BAIL:

	return hResult;
}


/*
 * begin pairing with a device
 */
stiHResult CstiBluetooth::Pair (
	const BluetoothDevice& bluetoothDevice)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("enter Pair \n",
				"\tdevice = ", bluetoothDevice.name, "\n",
				"\taddress = ", bluetoothDevice.address, "\n",
				"\tobject = ", bluetoothDevice.objectPath, "\n",
				"\ticon = ", bluetoothDevice.icon, "\n",
				"\tpaired = ", bluetoothDevice.paired, "\n",
				"\tmajor = ", bluetoothDevice.majorDeviceClass, ", minor = ", bluetoothDevice.minorDeviceClass, "\n",
				"\tconnected = ", bluetoothDevice.connected, "\n",
				"\ttrusted = ", bluetoothDevice.trusted, "\n",
				"\tblocked = ", bluetoothDevice.blocked, "\n",
				"\tlegacy = ", bluetoothDevice.legacyPairing, "\n"
		);
	);

	if (m_pairingState != PairingState::None)
	{
		stiTHROWMSG (stiRESULT_ERROR, "Trying to start pairing when state = %d\n", m_pairingState);
	}

	{
		auto device = deviceGetByAddress (bluetoothDevice.address);

		if (!device)
		{
			stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "Couldn't find device %s in scanned list\n", bluetoothDevice.name.c_str());
		}

		m_pairingState = PairingState::Pairing;
		m_pairingDevice = device;
		PostEvent ([this]{eventPair ();});
	}

STI_BAIL:
	return(hResult);
}

nonstd::observer_ptr<vpe::BluetoothAudio> CstiBluetooth::AudioGet ()
{
	return nonstd::make_observer(&m_audio);
}

/*!
 * \brief get the default bluetooth adapter name
 */
std::string CstiBluetooth::BluetoothDefaultAdapterNameGet ()
{
	return "hci0";
}

/*
 * actually initiate the pairing
 */
void CstiBluetooth::eventPair ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	if (m_pairingDevice)
	{
		int pairedCount = std::count_if (m_deviceList.begin (), m_deviceList.end (),
										 [](const std::shared_ptr<SstiBluezDevice> &device){
										 return device->bluetoothDevice.paired;});

		// Check device count.  If over max paired count send max count error.
		if (pairedCount >= MAX_PAIRED_COUNT)
		{
			pairedMaxCountExceededSignal.Emit();

			// We aren't pairing this device so release it.
			m_pairingDevice.reset();

			// Reset the pairing state.
			m_pairingState = PairingState::None;

			stiTHROW(stiRESULT_ERROR);
		}

		if (m_pairingDevice->bluetoothDevice.isAudio())
		{
			// If we are pairing an audio device look for other audio devices and disconnected them.
			auto audioDeviceIter = std::find_if (m_deviceList.begin (), m_deviceList.end (),
												 [](const std::shared_ptr<SstiBluezDevice> &device){
												return device->bluetoothDevice.isAudio() && device->bluetoothDevice.connected;});

			if (audioDeviceIter != m_deviceList.end ())
			{
				Disconnect ((*audioDeviceIter)->bluetoothDevice);
			}
		}

		m_pairingDevice->pair ();
	}

STI_BAIL:

	return;
}


/*
 * we need the cancellation to happen synchronously.  don't want to wait
 * until the pairing is finished...
 */
stiHResult CstiBluetooth::PairCancel ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_pairingState != PairingState::Pairing)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiBluetooth::PairCancel: called while not pairing\n");
	}

	if (m_pairingDevice)
	{
		m_pairingDevice->pairCancel ();
	}

STI_BAIL:

	return (hResult);
}


/*
 * remove a currently paired device.
 */
stiHResult CstiBluetooth::PairedDeviceRemove(BluetoothDevice bluetoothDevice)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto device = deviceGetByAddress (bluetoothDevice.address);

	if (!device)
	{
		stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "Trying to remove unpaired device %s\n", bluetoothDevice.name.c_str());
	}

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("Removing paired device ", device->bluetoothDevice.name, "\n");
	);

	m_adapterInterface->removeDevice (device->bluetoothDevice.objectPath);

	// Track when user unpairs a device
	remoteEventLog ("DeviceUnpaired", device->bluetoothDevice);

STI_BAIL:

	return (hResult);
}


/*
 *\ brief - initiate an event to connect a previously paired device
 */
stiHResult CstiBluetooth::Connect (
	BluetoothDevice bluetoothDevice)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace(__func__, ": enter: device = ", bluetoothDevice.address, "\n");
	);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto device = deviceGetByAddress (bluetoothDevice.address);

	if (!device ||
		(!device->bluetoothDevice.paired && device->bluetoothDevice.name != HELIOS_DFU_TARGET_NAME))
	{
		stiTHROWMSG (stiRESULT_ERROR, "Trying to connect unpaired device %s\n", bluetoothDevice.name.c_str());
	}

	if (!device->m_connecting)
	{
		device->connect ();
	}

STI_BAIL:

	return (hResult);
}


/*
 *\ brief - initiate an event to disconnect a previously paired device
 */
stiHResult CstiBluetooth::Disconnect (
	BluetoothDevice bluetoothDevice)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace(__func__, ": enter: device = ", bluetoothDevice.address, "\n");
	);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto device = deviceGetByAddress (bluetoothDevice.address);

	if (!device || !device->bluetoothDevice.connected)
	{
		stiTHROWMSG (stiRESULT_ERROR, "Trying to disconnect an already disconnected device %s\n", bluetoothDevice.name.c_str());
	}

	if (device->bluetoothDevice.connected)
	{
		device->disconnect ();
	}

STI_BAIL:

	return (hResult);
}


std::shared_ptr<SstiGattCharacteristic> CstiBluetooth::characteristicFromConnectedDeviceGet(
	const std::shared_ptr<SstiBluezDevice> &device,
	const std::string &uuid)
{
	if (!device->bluetoothDevice.connected)
	{
		vpe::trace("Trying to get characteristic from disconnected device ",  device->bluetoothDevice.name, "\n");
	}
	else
	{
		return device->characteristicFind (uuid);
	}

	// If we get here, a characteristic with the given uuid was not found
	return nullptr;
}

stiHResult CstiBluetooth::gattCharacteristicRead (
	const std::shared_ptr<SstiBluezDevice> &device,
	const std::string uuid)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::shared_ptr<SstiGattCharacteristic> characteristic = nullptr;

	characteristic = characteristicFromConnectedDeviceGet(device, uuid);

	if (characteristic == nullptr)
	{
		stiTHROWMSG (stiRESULT_ERROR, "%s: uuid %s not found in device %s\n", __func__, uuid.c_str(), device->bluetoothDevice.name.c_str());
	}

	characteristic->read (device);

STI_BAIL:

	return(hResult);
}


stiHResult CstiBluetooth::gattCharacteristicWrite (
	const std::shared_ptr<SstiBluezDevice> &device,
	const std::string uuid,
	const char *value,
	const int len)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	//DbusError dbusError;
	
	std::shared_ptr<SstiGattCharacteristic> characteristic = nullptr;

	characteristic = characteristicFromConnectedDeviceGet (device, uuid);
	
	if (characteristic == nullptr)
	{
		stiTHROWMSG (stiRESULT_ERROR, "%s: uuid %s not found in device %s\n", __func__, uuid.c_str(), device->bluetoothDevice.name.c_str());
	}

	characteristic->write (device, value, len);

STI_BAIL:

	return (hResult);
}


/*
 * called to set whether or not you want notifications of when
 * this characteristic changes
 */
stiHResult CstiBluetooth::gattCharacteristicNotifySet(
	const std::shared_ptr<SstiBluezDevice> &device,
	const std::string uuid,
	bool value)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::shared_ptr<SstiGattCharacteristic> characteristic = nullptr;

	characteristic = characteristicFromConnectedDeviceGet(device, uuid);
	if (characteristic == nullptr)
	{
		stiTHROWMSG (stiRESULT_ERROR, "%s: uuid %s not found in device %s\n", __func__, uuid.c_str(), device->bluetoothDevice.name.c_str());
	}

	if (value)
	{
		if (characteristic->notifyingGet ())
		{
			// Already notifying
			SstiGattCharacteristic::startNotifyCompletedSignal.Emit (device->bluetoothDevice.address, characteristic->uuidGet ());
		}
		else
		{
			characteristic->notifyStart ();
		}
	}
	else
	{
		if (!characteristic->notifyingGet ())
		{
			// Already not notifying
			//gattAlertSignal.Emit (eStopNotifyComplete, device->bluetoothDevice.address, characteristic->uuidGet ());
		}
		else
		{
			characteristic->notifyStop ();
		}
	}

STI_BAIL:

	return(hResult);
}


/*
 *\ brief - start this event loop
 */
stiHResult CstiBluetooth::Startup ()
{
	stiLOG_ENTRY_NAME (CstiBluetooth::Startup);

	CstiEventQueue::StartEventLoop();

	PostEvent ([this]{eventManagedObjectsGet ();});
	return (stiRESULT_SUCCESS);
}


/*
 *\ brief - let someone set the pincode, and then unblock the dbus callback
 *          that is waiting for the pincode
 */
stiHResult CstiBluetooth::PincodeSet (
	const char *pszPincode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	switch (m_pairingAuthenticationType)
	{
		case PairingType::Passkey:

			if (strlen(pszPincode))
			{
				uint32_t passkey = strtoul(pszPincode, nullptr, 10);
				g_dbus_method_invocation_return_value(m_pairingInvocation, g_variant_new("(u)", passkey));
			}
			else
			{
				stiASSERTMSG(false, "Passkey has no length\n");
				g_dbus_method_invocation_return_dbus_error(m_pairingInvocation,
						"org.bluez.Error.Canceled",
						"invalid PassKey");
			}

			break;

		case PairingType::PinCode:

			if (strlen(pszPincode))
			{
				g_dbus_method_invocation_return_value(m_pairingInvocation, g_variant_new("(s)", pszPincode));
			}
			else
			{
				stiASSERTMSG (false, "Pincode has no length\n");
				g_dbus_method_invocation_return_dbus_error(m_pairingInvocation,
						"org.bluez.Error.Canceled",
						"invalid PinCode");
			}

			break;

		case PairingType::Unknown:

			stiTHROWMSG (stiRESULT_ERROR, "Setting pincode when not pairing\n");

			break;
	}

STI_BAIL:
	return (hResult);
}

/*
 *\ brief - we are receiving a pincode from d-bus to display.  save it and
 *          notify the user interface
 */
gboolean CstiBluetooth::DisplayPincodeMessage(CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	const gchar *ObjectPath,
	const gchar *Pincode,
	gpointer UserData)
{
	auto self = static_cast<CstiBluetooth *>(UserData);

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("BluetoothDisplayPincode - ", ObjectPath, ", ", Pincode, "\n");
	);

	std::lock_guard<std::recursive_mutex> lock (self->m_execMutex);
  
	std::string pincode = Pincode;
	self->displayPincodeSignal.Emit(pincode);

	g_dbus_method_invocation_return_value(pInvocation, nullptr);

	return (TRUE);
}

/*
 *\ brief - we are receiving a passkey to display from d-bus.  save it (in the pincode)
 *          and let the user interface know
 */
gboolean CstiBluetooth::DisplayPasskeyMessage (
	CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	const gchar *ObjectPath,
	guint32 Passkey,
	guint16 entered,
	gpointer UserData)
{
	auto self = static_cast<CstiBluetooth *>(UserData);

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("BluetoothDisplayPasskey - ", ObjectPath, ", ", Passkey, " - entered = ", entered, "\n");
	);
	
	std::lock_guard<std::recursive_mutex> lock (self->m_execMutex);

	if (entered == 0)
	{
		std::stringstream stream;
		stream << std::setfill('0') << std::setw(6) << Passkey;
		self->displayPincodeSignal.Emit(stream.str());
	}
	
	g_dbus_method_invocation_return_value(pInvocation, nullptr);
	
	return (TRUE);
}

/*
 *\ brief - d-bus would like a pincode from us.  notify the user interface of this
 *          then wait until the pincode is retrieved.
 */
gboolean CstiBluetooth::RequestPincodeMessage (
	CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	const gchar *ObjectPath,
	gpointer UserData)
{
	auto self = static_cast<CstiBluetooth *>(UserData);
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("BluetoothRequestPincode - ", ObjectPath, "\n");
	);

	std::lock_guard<std::recursive_mutex> lock (self->m_execMutex);

	self->m_pairingInvocation = pInvocation;
	self->m_pairingAuthenticationType = PairingType::PinCode;

	self->requestPincodeSignal.Emit();

	return (TRUE);
}

/*
 *\ brief - d-bus would like a passkey from us.  notify the user interface of this
 *          then wait until the passkey is retrieved.
 */
gboolean CstiBluetooth::RequestPasskeyMessage (
	CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	const gchar *ObjectPath,
	gpointer UserData)
{
	auto self = static_cast<CstiBluetooth *>(UserData);
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("got BluetoothRequestPasskey - ", ObjectPath, "\n");
	);

	std::lock_guard<std::recursive_mutex> lock (self->m_execMutex);

	self->m_pairingInvocation = pInvocation;
	self->m_pairingAuthenticationType = PairingType::Passkey;

	self->requestPincodeSignal.Emit();

	return (TRUE);
}

/*
 *\ brief - request a confirmation message.  I've not seen one of these before...
 */
gboolean CstiBluetooth::RequestConfirmationMessage(CstiBluetoothAgent1 *,
		GDBusMethodInvocation *pInvocation,
		const gchar *ObjectPath,
		guint Passkey,
		gpointer UserData)
{
	stiASSERTMSG (false, "Got handle-request-confirmation: %s - %d\n", ObjectPath, Passkey);

	g_dbus_method_invocation_return_value(pInvocation, nullptr);
	return (TRUE);
}

/*
 *\ brief - request an authorization message.  i've never seen one of these before
 */
gboolean CstiBluetooth::RequestAuthorizationMessage(CstiBluetoothAgent1 *,
		GDBusMethodInvocation *pInvocation,
		const gchar *ObjectPath,
		gpointer UserData)
{
	stiASSERTMSG (false, "Got handle-request-authorization: object - %s\n", ObjectPath);

	g_dbus_method_invocation_return_value(pInvocation, nullptr);
	return (TRUE);
}

/*
 *\ brief - we've cancelled something.  can use to do cleanup
 */
gboolean CstiBluetooth::CancelMessage(CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	gpointer)
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("Got BluetoothCancelMessage\n");
	);
	g_dbus_method_invocation_return_value(pInvocation, nullptr);
	return (TRUE);
}

/*
 *\ brief - something has finished/cancelled.  available to do cleanup
 */
gboolean CstiBluetooth::ReleaseMessage(CstiBluetoothAgent1 *,
	GDBusMethodInvocation *pInvocation,
	gpointer)
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("Got BluetoothRelease message\n");
	);
	g_dbus_method_invocation_return_value(pInvocation, nullptr);
	return (TRUE);
}


/*
 *\ brief - register our functions for the agent callbacks
 */
void CstiBluetooth::eventAgentManagerInterfaceAdded(
	std::shared_ptr<AgentManagerInterface> agentManagerInterface)
{
	DbusError dbusError;
	std::string agentPath = "/org/bluez/Agent1_" + std::to_string(getpid());

	m_agentProxy = csti_bluetooth_agent1_skeleton_new();


	m_unDisplayPasskeySignalHandlerId = g_signal_connect(m_agentProxy, "handle-display-passkey", G_CALLBACK(DisplayPasskeyMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unDisplayPinCodeSignalHandlerId = g_signal_connect(m_agentProxy, "handle-display-pin-code", G_CALLBACK(DisplayPincodeMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unReleaseSignalHandlerId = g_signal_connect(m_agentProxy, "handle-release", G_CALLBACK(ReleaseMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unCancelSignalHandlerId = g_signal_connect(m_agentProxy, "handle-cancel", G_CALLBACK(CancelMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unAuthorizeSignalHandlerId = g_signal_connect(m_agentProxy, "handle-request-authorization", G_CALLBACK(RequestAuthorizationMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unRequsetConfirmationSignalHandlerId = g_signal_connect(m_agentProxy, "handle-request-confirmation", G_CALLBACK(RequestConfirmationMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unReqeuestPasskeySignalHandlerId = g_signal_connect(m_agentProxy, "handle-request-passkey", G_CALLBACK(RequestPasskeyMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	m_unRequsetPinCodeSignalHandlerId = g_signal_connect(m_agentProxy, "handle-request-pin-code", G_CALLBACK(RequestPincodeMessage), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

	g_dbus_interface_skeleton_export(
			G_DBUS_INTERFACE_SKELETON(m_agentProxy), //NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
			m_dbusConnection->connectionGet (),
			agentPath.c_str (),
			dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (estiFALSE, "Can't export skeleton: %s\n", dbusError.message ().c_str ());
	}
	else
	{
		agentManagerInterface->agentRegister (agentPath);
	}
}


void CstiBluetooth::eventAgentRegistrationFailed ()
{
	agentUnregister ();
}


/*
 *\ brief - closing down.  unregister our agent callbacks.
 */
void CstiBluetooth::agentUnregister ()
{
	if (m_agentProxy)
	{
		g_signal_handler_disconnect(m_agentProxy, m_unDisplayPasskeySignalHandlerId);
		m_unDisplayPasskeySignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unDisplayPinCodeSignalHandlerId);
		m_unDisplayPinCodeSignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unReleaseSignalHandlerId);
		m_unReleaseSignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unCancelSignalHandlerId);
		m_unCancelSignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unAuthorizeSignalHandlerId);
		m_unAuthorizeSignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unRequsetConfirmationSignalHandlerId);
		m_unRequsetConfirmationSignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unReqeuestPasskeySignalHandlerId);
		m_unReqeuestPasskeySignalHandlerId = 0;

		g_signal_handler_disconnect(m_agentProxy, m_unRequsetPinCodeSignalHandlerId);
		m_unRequsetPinCodeSignalHandlerId = 0;

		g_object_unref (m_agentProxy);
		m_agentProxy = nullptr;
	}
}

bool CstiBluetooth::Powered ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	return (m_adapterInterface->poweredGet ());
}

/*
 * turn on/off the bluetooth adapter
 */
stiHResult CstiBluetooth::Enable (
	bool bEnable)
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("enter bluetooth enable - ", bEnable, "\n");
	);

	if (bEnable)
	{
		if (system("rfkill unblock bluetooth"))
		{
			stiASSERTMSG (false, "error: rfkill unblock bluetooth");
		}

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
		ControllerInitialize ();
#endif
		
	}
	else
	{
		if (system("rfkill block bluetooth"))
		{
			stiASSERTMSG (false, "error: rfkill block bluetooth");
		}
		
		ScannedListAdvertisingReset ();
	}

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("exit bluetooth enable\n");
	);

	return (stiRESULT_SUCCESS);
}

stiHResult CstiBluetooth::PairedDevicesLog ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::list<BluetoothDevice> pairedDevices;

	hResult = PairedListGet(&pairedDevices);
	stiTESTRESULT();

	for (const auto &bluetoothDevice : pairedDevices)
	{
		remoteEventLog ("PairedDeviceAdded", bluetoothDevice);
	}


STI_BAIL:

	return hResult;
}

stiHResult CstiBluetooth::DeviceRename(
	const BluetoothDevice& device,
	const std::string newName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto dev = deviceGetByAddress(device.address);
	stiTESTCONDMSG (dev, stiRESULT_ERROR, "%s: device not found: %s\n", __func__, device.name.c_str());

	if (newName == "" && dev->bluetoothDevice.name == dev->defaultName)
	{
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

	dev->rename(newName);

STI_BAIL:
	return hResult;
}

/*
 * when we reset connman when forgetting a wifi service, the bluetooth adapter goes down and up.
 * so.... we'll need to mark all of the paired devices as disconnected so that we can re-connect
 * them when we power the adapter back up.  even though we still have all of the proxies and info
 * for them
 */
void CstiBluetooth::pairedDevicesReset()
{
	for (auto device : m_deviceList)
	{
		device->bluetoothDevice.connected = false;
	}
}

stiHResult CstiBluetooth::ControllerInitialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::string controllerName = execCommand("hciconfig | perl -ne '/([hci]+[0-9]):/ && print $1'");
	std::string command = "hciconfig " + controllerName + " pscan";
	
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		vpe::trace("Host Controller Interface: ", controllerName, "\n");
	);
	
	if (system(command.c_str()) < 0)
	{
		stiTHROW(stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

std::string CstiBluetooth::execCommand (const char* cmd) 
{
	std::array<char, 128> buffer;
	std::string result;
	struct file_deleter
	{
		void operator()(FILE* d) const
		{
			pclose(d);
		}
	};

	std::unique_ptr<FILE, file_deleter> pipe(popen(cmd, "r"));
	
	if (!pipe)
	{
		stiASSERTMSG (estiFALSE, "popen() command failed.\n");
	}
	
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	
	return result;
}
