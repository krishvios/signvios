
#include "TechnologyInterface.h"
#include "stiTrace.h"

CstiSignal<std::shared_ptr<NetworkTechnology>> NetworkTechnology::addedSignal;
CstiSignal<std::shared_ptr<NetworkTechnology>> NetworkTechnology::powerChangedSignal;
CstiSignal<bool> NetworkTechnology::scanFinishedSignal;

NetworkTechnology::~NetworkTechnology()
{
	if (m_technologyProxy)
	{
		g_signal_handler_disconnect(m_technologyProxy, m_technologyChangedSignalHandlerId);

		g_object_unref(m_technologyProxy);
		m_technologyProxy = nullptr;
	}
}
void NetworkTechnology::added(
	GVariant *properties)
{
	g_variant_ref (properties);

	csti_network_technology_proxy_new (
				m_dbusObject->dbusConnectionGet ()->connectionGet (),
				G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
				CONNMAN_SERVICE,
				objectPathGet().c_str(),
				nullptr,
				DbusCall::finish,
				DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkTechnology::eventProxyNew, this, properties));
}

void NetworkTechnology::eventProxyNew(
	GObject *proxy,
	GAsyncResult *res,
	GVariant *properties)
{
	DbusError dbusError;

	m_technologyProxy = csti_network_technology_proxy_new_finish(res, dbusError.get());

	if (dbusError.valid())
	{
		stiTrace("NetworkTechnology::%s: failed to create new proxy: %s\n", __func__, dbusError.message().c_str());
	}
	else
	{
		propertiesProcess (properties);

		addedSignal.Emit(std::static_pointer_cast<NetworkTechnology>(shared_from_this()));

		g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(m_technologyProxy), G_MAXINT);
		m_technologyChangedSignalHandlerId = g_signal_connect(
				m_technologyProxy,
				"property-changed",
				G_CALLBACK(&NetworkTechnology::technologyChangedCallback),
				this);
	}

	g_variant_unref(properties);
}

void NetworkTechnology::technologyChangedCallback(
	CstiNetworkTechnology *proxy,
	const gchar *tmpName,
	GVariant *value,
	gpointer userData)
{
	auto self = (NetworkTechnology *)userData;

	g_variant_ref (value);
	std::string name(tmpName);

	self->m_dbusObject->eventQueueGet ()->PostEvent (
		[self, proxy, name, value]()
		{
			self->eventTechnologyChanged (proxy, name, value);

			g_variant_unref (value);
		});
}


void NetworkTechnology::eventTechnologyChanged (
	CstiNetworkTechnology *proxy,
	const std::string &name,
	GVariant *value)
{
	if (name == propertyPowered)
	{
		auto var = g_variant_get_variant(value);
		m_powered = g_variant_get_boolean(var);
		g_variant_unref (var);
		var = nullptr;
		powerChangedSignal.Emit(std::static_pointer_cast<NetworkTechnology>(shared_from_this()));
	}
	else if (name == propertyConnected)
	{
		auto var = g_variant_get_variant(value);
		m_connected = g_variant_get_boolean(var);
		g_variant_unref (var);
		var = nullptr;
	}
	else if (name == propertyName)
	{
		auto var = g_variant_get_variant(value);
		m_technologyName = g_variant_get_string(var, nullptr);
		g_variant_unref (var);
		var = nullptr;
	}
	else if (name == propertyType)
	{
		auto var = g_variant_get_variant(value);
		std::string tmpStr = g_variant_get_string(var, nullptr);
		if (tmpStr == typeWifi)
		{
			m_technologyType = technologyType::wireless;
		}
		else if (tmpStr == typeEthernet)
		{
			m_technologyType = technologyType::wired;
		}
		else if (tmpStr == typeBluetooth)
		{
			m_technologyType = technologyType::bluetooth;
		}
		else
		{
			m_technologyType = technologyType::unknown;
		}
		g_variant_unref (var);
		var = nullptr;
	}
}

void NetworkTechnology::propertiesProcess(GVariant *properties)
{
	int limit = g_variant_n_children(properties);

	for (int i = 0; i < limit; i++)
	{
		GVariant *dict = nullptr;
		GVariant *tmp = nullptr;
		const gchar *property = nullptr;

		dict = g_variant_get_child_value(properties, i);
		tmp = g_variant_get_child_value(dict, 0);
		property = g_variant_get_string(tmp, nullptr);
		g_variant_unref(tmp);
		tmp = nullptr;

		tmp = g_variant_get_child_value(dict, 1);
		std::string name(property);
		eventTechnologyChanged(m_technologyProxy, name, tmp);
		g_variant_unref(tmp);
		tmp = nullptr;
		g_variant_unref(dict);
		dict = nullptr;
	}
}

void NetworkTechnology::scanStart()
{
	// i've seen scans that didn't end.  don't let that happen.
	g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(m_technologyProxy), 25000);
	csti_network_technology_call_scan(
		m_technologyProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkTechnology::scanFinish, this));
}

void NetworkTechnology::scanFinish(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

//	stiASSERT(proxy != nullptr);

	csti_network_technology_call_scan_finish(
		m_technologyProxy,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		stiTrace("NetworkTechnology::%s: scanning failed: %s\n", __func__, dbusError.message().c_str());
		scanFinishedSignal.Emit(false);
	}
	else
	{
		scanFinishedSignal.Emit(true);
	}
// reset the timeout
	g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(m_technologyProxy), G_MAXINT);
}

void NetworkTechnology::propertySet(
	const std::string &property,
	GVariant *value)
{
	csti_network_technology_call_set_property (
		m_technologyProxy,
		property.c_str(),
		value,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkTechnology::propertySetFinish, this, nullptr));
}

void NetworkTechnology::propertySetFinish(
	GObject *proxy,
	GAsyncResult *result,
	gpointer userData)
{
	DbusError dbusError;

	csti_network_technology_call_set_property_finish(
		(CstiNetworkTechnology *)proxy,
		result,
		dbusError.get());

	if (dbusError.valid())
	{
		stiTrace("NetworkTechnology::%s: set property failed: %s\n", __func__, dbusError.message().c_str());
	}
	
}
