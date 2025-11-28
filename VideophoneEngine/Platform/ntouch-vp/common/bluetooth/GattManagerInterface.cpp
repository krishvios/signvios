// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "GattManagerInterface.h"

#include "DbusCall.h"
#include "DbusError.h"
#include "BluetoothDefs.h"
#include "stiTrace.h"

const char* autoReconnectUUIDs[] = {PULSE_SERVICE_UUID, nullptr};

CstiSignal<std::shared_ptr<GattManagerInterface>> GattManagerInterface::addedSignal;


/*
 *\ brief add the gatt manager
 *
 */
void GattManagerInterface::added (
	GVariant *properties)
{
	// Get Gatt Manager proxy
	csti_bluetooth_gatt_manager1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &GattManagerInterface::eventGattManagerAdd, this));
}


void GattManagerInterface::eventGattManagerAdd (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_gattManagerProxy = csti_bluetooth_gatt_manager1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: failed getting gatt manager proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		addedSignal.Emit (std::static_pointer_cast<GattManagerInterface>(shared_from_this()));
	}
}



void GattManagerInterface::changed (
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
}


/*
 *\ brief - gatt profile was released.  Available to do cleanup
 */
gboolean GattManagerInterface::ReleaseProfile (
	CstiBluetoothGattProfile1 *proxy,
	GDBusMethodInvocation *pInvocation,
	gpointer)
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("Received gatt profile release message\n");
	);

	g_dbus_method_invocation_return_value (pInvocation, nullptr);

	return TRUE;
}


stiHResult GattManagerInterface::autoReconnectSetup (
	GDBusObjectManagerServer *objectManagerServer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiBluetoothObjectSkeleton *appSkeleton = nullptr;
	CstiBluetoothGattProfile1 *profile = nullptr;
	GVariant *options = nullptr;
	GVariantBuilder *builder = nullptr;

	// Create application object
	appSkeleton = csti_bluetooth_object_skeleton_new ("/ntouch/app");
	stiTESTCOND (appSkeleton, stiRESULT_ERROR);

	// Create Gatt Profile interface to be added to application object
	profile = csti_bluetooth_gatt_profile1_skeleton_new ();

	// Set gatt profile uuid on the profile
	csti_bluetooth_gatt_profile1_set_uuids (
			profile,
			(const char* const*)autoReconnectUUIDs);

	// Set "Release" method signal handler on the profile
	g_signal_connect (profile, "handle-release", G_CALLBACK (ReleaseProfile), this); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

	// Add the gatt profile interface to the application object
	csti_bluetooth_object_skeleton_set_gatt_profile1 (
			appSkeleton,
			profile);

	// Add the object to the bus
	g_dbus_object_manager_server_export (objectManagerServer, G_DBUS_OBJECT_SKELETON (appSkeleton)); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

	// Now call RegisterApplication after a little setup
	// Create map of options for RegisterApplication call (empty as of now)
	builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	options = g_variant_new("a{sv}", builder);
	g_variant_builder_unref (builder);
	builder = nullptr;

	stiASSERT (m_gattManagerProxy != nullptr);

	// Call RegisterApplication on the Gatt Manager
	csti_bluetooth_gatt_manager1_call_register_application (
			m_gattManagerProxy,
			"/ntouch",
			options,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &GattManagerInterface::eventRegisterApplication, this));

STI_BAIL:

	return hResult;
}


void GattManagerInterface::eventRegisterApplication (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;
	auto gattManagerProxy = reinterpret_cast<CstiBluetoothGattManager1 *>(proxy); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	stiASSERT (gattManagerProxy != nullptr);

	csti_bluetooth_gatt_manager1_call_register_application_finish (gattManagerProxy, res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "Can't register application - %s\n", dbusError.message ().c_str ());
	}
}


