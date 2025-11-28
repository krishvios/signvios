
#include "NetworkManagerInterface.h"

CstiSignal<std::shared_ptr<NetworkManagerInterface>> NetworkManagerInterface::addedSignal;

NetworkManagerInterface::~NetworkManagerInterface()
{
	if (m_networkManagerProxy)
	{
		g_signal_handler_disconnect(m_networkManagerProxy, m_propertyChangedSignalHandlerId);
		g_signal_handler_disconnect(m_networkManagerProxy, m_technologyAddedSignalHandlerId);
		g_signal_handler_disconnect(m_networkManagerProxy, m_technologyRemovedSignalHandlerId);
		g_signal_handler_disconnect(m_networkManagerProxy, m_servicesChangedSignalHandlerId);

		g_object_unref(m_networkManagerProxy);
		m_networkManagerProxy = nullptr;
	}
}

void NetworkManagerInterface::added(
	GVariant */*properties*/)
{
	csti_network_manager_proxy_new (
				m_dbusObject->dbusConnectionGet ()->connectionGet (),
				G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
				CONNMAN_SERVICE,
				CONNMAN_MANAGER_PATH,
				nullptr,
				DbusCall::finish,
				DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkManagerInterface::eventProxyNew, this));
}

void NetworkManagerInterface::eventProxyNew(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_networkManagerProxy = csti_network_manager_proxy_new_finish(res, dbusError.get());

	if (dbusError.valid())
	{
			// throw some error here
	}
	else
	{
		addedSignal.Emit(std::static_pointer_cast<NetworkManagerInterface>(shared_from_this()));

		m_propertyChangedSignalHandlerId = g_signal_connect (
			m_networkManagerProxy,
			"property-changed",
			G_CALLBACK(propertyChangedCallback),
			this);
		m_technologyAddedSignalHandlerId = g_signal_connect(
			m_networkManagerProxy,
			"technology-added",
			G_CALLBACK(technologyAddedCallback),
			this);
		m_technologyRemovedSignalHandlerId = g_signal_connect(
			m_networkManagerProxy,
			"technology-removed",
			G_CALLBACK(technologyRemovedCallback),
			this);
		m_servicesChangedSignalHandlerId = g_signal_connect(
			m_networkManagerProxy,
			"services-changed",
			G_CALLBACK(servicesChangedCallback),
			this);

		csti_network_manager_call_get_properties (
			m_networkManagerProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2(m_dbusObject->eventQueueGet (),	&NetworkManagerInterface::eventGetProperties, this));
	}
}


void NetworkManagerInterface::propertyChangedCallback(
	CstiNetworkManager *proxy,
	const gchar *tmpName,
	GVariant *value,
	gpointer userData)
{
	auto self = (NetworkManagerInterface *)userData;

	g_variant_ref (value);
	std::string name(tmpName);

	self->m_dbusObject->eventQueueGet ()->PostEvent (
		[self, proxy, name, value]()
		{
			self->eventPropertyChanged (proxy, name, value);

			g_variant_unref (value);
		});
}


void NetworkManagerInterface::eventPropertyChanged (
	CstiNetworkManager *proxy,
	const std::string &name,
	GVariant *value)
{
	if (name == propertyState)
	{
		auto var = g_variant_get_variant(value);
		m_managerState = g_variant_get_string(var, nullptr);
		g_variant_unref (var);
	}
	else if (name == propertyOfflineMode)
	{
		auto var = g_variant_get_variant(value);
		m_offlineMode = g_variant_get_boolean(var);
		g_variant_unref (var);
	}
	else if (name == propertySessionMode)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkManagerInterface::%s: Got deprecated propertyChanged %s\n", __func__, name.c_str ());
		);
	}
	else
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkManagerInterface::%s: Got unknown propertyChanged %s\n", __func__, name.c_str ());
		);
	}
}

void NetworkManagerInterface::technologyAddedCallback(
	CstiNetworkManager *proxy,
	gchar *objectPath,
	GVariant *properties,
	gpointer userData)
{
	auto self = (NetworkManagerInterface *)userData;
	std::string pathCopy = objectPath;

	self->technologyAddedSignal.Emit(pathCopy, properties);
}

void NetworkManagerInterface::technologyRemovedCallback(
	CstiNetworkManager *proxy,
	gchar *objectPath,
	gpointer userData)
{
	auto self = (NetworkManagerInterface *)userData;
	std::string pathCopy = objectPath;
	self->technologyRemovedSignal.Emit(pathCopy);
}

void NetworkManagerInterface::servicesChangedCallback(
	CstiNetworkManagerProxy *proxy,
	GVariant *changed,
	gchar **removed,
	gpointer userData)
{
	auto self = (NetworkManagerInterface *)userData;
	std::vector<std::string> removedCopy;

	if (removed)
	{
		for (int i = 0; removed[i]; i++)
		{
			removedCopy.push_back(removed[i]);
		}
	}

	self->servicesChangedSignal.Emit(changed, removedCopy);
}

void NetworkManagerInterface::eventGetTechnologies(
	GObject *proxy,
	GAsyncResult *res)
{
	GVariant *technologies = nullptr;
	DbusError dbusError;

	csti_network_manager_call_get_technologies_finish(
		m_networkManagerProxy,
		&technologies,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		// something here that technologies failed;
	}
	else
	{
		technologiesRetrievedSignal.Emit(technologies);
	}
}

void NetworkManagerInterface::eventGetServices(
	GObject *proxy,
	GAsyncResult *res)
{
	GVariant *services;
	DbusError dbusError;

	csti_network_manager_call_get_services_finish(
		m_networkManagerProxy,
		&services,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		// something here
	}
	else
	{
		servicesRetrievedSignal.Emit(services);
	}
}

void NetworkManagerInterface::agentRegister(
	const std::string path)
{
	csti_network_manager_call_register_agent(
		m_networkManagerProxy,
		path.c_str(),
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (),	&NetworkManagerInterface::eventAgentRegistered, this));
}

void NetworkManagerInterface::eventAgentRegistered(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;
	auto manager = (CstiNetworkManager *)proxy;

	csti_network_manager_call_register_agent_finish(manager, res, dbusError.get());
	if (dbusError.valid())
	{
		stiASSERTMSG(false, "Can't register network agent: %s\n", dbusError.message().c_str());
		agentRegisterationFailedSignal.Emit();
	}
	else
	{
		csti_network_manager_call_get_technologies(
			m_networkManagerProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &NetworkManagerInterface::eventGetTechnologies, this));

		csti_network_manager_call_get_services(
			m_networkManagerProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &NetworkManagerInterface::eventGetServices, this));

		agentRegisteredSignal.Emit();
	}
}

void NetworkManagerInterface::eventGetProperties(
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;
	GVariant *properties;

	csti_network_manager_call_get_properties_finish(
		m_networkManagerProxy,
		&properties,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		stiTrace("NetworkManagerInterface::%s: can't retrieve properties: %s\n", __func__, dbusError.message().c_str());
		stiASSERT(false);
	}
	else
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
			propertyChangedCallback(nullptr, property, tmp, this);
			g_variant_unref(tmp);
			tmp = nullptr;
			g_variant_unref(dict);
			dict = nullptr;
		}
		g_variant_unref(properties);
		properties = nullptr;
	}
}

void NetworkManagerInterface::servicesGet()
{
		csti_network_manager_call_get_services(
			m_networkManagerProxy,
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &NetworkManagerInterface::getServicesFinish, this));
}

void NetworkManagerInterface::getServicesFinish(
	GObject *proxy,
	GAsyncResult *res)
{
	GVariant *services;
	DbusError dbusError;

	csti_network_manager_call_get_services_finish(
		m_networkManagerProxy,
		&services,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		servicesScannedSignal.Emit(nullptr);
	}
	else
	{
		servicesScannedSignal.Emit(services);
	}
}

void NetworkManagerInterface::specificServiceGet(const std::string &objectPath)
{
	csti_network_manager_call_get_services(
		m_networkManagerProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet(), &NetworkManagerInterface::specificServiceGetFinish, this, objectPath));
}

void NetworkManagerInterface::specificServiceGetFinish(
	GObject *proxy,
	GAsyncResult *res,
	const std::string &serviceObjectPath)
{
	GVariant *services;
	DbusError dbusError;

	csti_network_manager_call_get_services_finish(m_networkManagerProxy,
		&services,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		stiASSERTMSG(false, "specificServiceGet failed: %s\n", dbusError.message().c_str());
	}
	else
	{
		
		GVariantIter *variantIter = nullptr;
		GVariant *variant = nullptr;

		variantIter = g_variant_iter_new(services);

		while ((variant = g_variant_iter_next_value(variantIter)))
		{
			auto tmpVariant = g_variant_get_child_value(variant, 0);
			auto objectPath = g_variant_get_string(tmpVariant, nullptr);
			g_variant_unref(tmpVariant);
			tmpVariant = nullptr;

			if (objectPath == serviceObjectPath)
			{
				auto dict = g_variant_get_child_value(variant, 1);
				g_variant_ref(dict);
				specificServiceFoundSignal.Emit(objectPath, dict);
				g_variant_unref(dict);
				g_variant_unref(variant);
				break;
			}
			g_variant_unref(variant);
			variant = nullptr;
		}

		if (variantIter)
		{
			g_variant_iter_free(variantIter);
			variantIter = nullptr;
		}
	}
}
