#include "ServiceInterface.h"
#include "stiTrace.h"

CstiSignal<std::shared_ptr<NetworkService>> NetworkService::addedSignal;
CstiSignal<std::shared_ptr<NetworkService>> NetworkService::serviceConnectedSignal;
CstiSignal<std::shared_ptr<NetworkService>, connectionError> NetworkService::serviceConnectionErrorSignal;
CstiSignal<std::shared_ptr<NetworkService>, bool> NetworkService::serviceDisconnectedSignal;
CstiSignal<std::shared_ptr<NetworkService>, bool> NetworkService::serviceOfflineStateChangedSignal;
CstiSignal<std::shared_ptr<NetworkService>> NetworkService::serviceOnlineStateChangedSignal;
CstiSignal<std::shared_ptr<NetworkService>, bool> NetworkService::propertySetFinishedSignal;
CstiSignal<> NetworkService::newWiredConnectedSignal;
CstiSignal<std::shared_ptr<NetworkService>, const std::string &> NetworkService::specificServiceGetSignal;

NetworkService::~NetworkService()
{

	m_nameServers.clear();
	m_timeServers.clear();
	m_domains.clear();

	if (m_serviceProxy)
	{
		g_signal_handler_disconnect(m_serviceProxy, m_serviceChangedSignalHandlerId);

		g_object_unref(m_serviceProxy);
		m_serviceProxy = nullptr;
	}

	while (!m_queuedProperties.empty ())
	{
		auto dict = m_queuedProperties.front ();
		m_queuedProperties.pop ();

		g_variant_unref (dict);
	}
}

void NetworkService::added(
	GVariant *properties)
{
	propertiesProcess (properties);

	csti_network_service_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
			CONNMAN_SERVICE,
			objectPathGet().c_str(),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::eventProxyNew, this));
}

void NetworkService::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_serviceProxy = csti_network_service_proxy_new_finish(res, dbusError.get());

	if (dbusError.valid())
	{
		vpe::trace("NetworkService::", __func__, ": failed to create proxy: ", dbusError.message(), "\n");
	}
	else
	{
		g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(m_serviceProxy), G_MAXINT);

		while (!m_queuedProperties.empty ())
		{
			auto dict = m_queuedProperties.front ();

			propertiesProcess (dict);

			m_queuedProperties.pop ();

			g_variant_unref (dict);
		}

		m_serviceChangedSignalHandlerId = g_signal_connect(
			m_serviceProxy,
			"property-changed",
			G_CALLBACK(serviceChangedCallback),
			this);

		addedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()));

		if (m_technologyType == technologyType::wired)
		{
			newWiredConnectedSignal.Emit();
		}

		/*
		 * when creating a new service if we start at ready, and we don't have all of the IPV4 information
		 * we need to enquire of the manager for the information.
		 */
		if ((m_progressState & STATE_READY) && ((m_progressState & HAVE_IPV4) != HAVE_IPV4))
		{
			specificServiceGetSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), objectPathGet());
		}
	}
}

void NetworkService::propertySet(
	const std::string &property,
	GVariant *value)
{
	stiASSERT (m_serviceProxy);

	csti_network_service_call_set_property(
		m_serviceProxy,
		property.c_str(),
		value,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::propertySetFinish, this, nullptr));
}

void NetworkService::propertySetFinish(
	GObject *proxy,
	GAsyncResult *result,
	gpointer userData)
{
	DbusError dbusError;

	bool success {true};

	stiASSERT (m_serviceProxy);

	csti_network_service_call_set_property_finish(
		m_serviceProxy,
		result,
		dbusError.get());

	if (dbusError.valid())
	{
		vpe::trace("NetworkService::", __func__, ": set property on ", m_serviceName, " failed: ", dbusError.message(), "\n");
		success = false;
	}

	propertySetFinishedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), success);
}

void NetworkService::propertyClear(
	const std::string &property)
{
	stiASSERT (m_serviceProxy);

	csti_network_service_call_clear_property(
		m_serviceProxy,
		property.c_str(),
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::propertyClearFinish, this, nullptr));
}

void NetworkService::propertyClearFinish(
	GObject *proxy,
	GAsyncResult *res,
	gpointer userData)
{
	DbusError dbusError;

	stiASSERT (m_serviceProxy);

	csti_network_service_call_clear_property_finish(
		m_serviceProxy,
		res,
		dbusError.get());

	if (dbusError.valid())
	{
		vpe::trace("NetworkService::", __func__, ": clear property on ", m_serviceName, " failed: ", dbusError.message(), "\n");
	}
}

/*!\brief Checks to see if we have enough information for the connection process to move an idle state
 * 
 */
void NetworkService::serviceStateCheck ()
{
	stiDEBUG_TOOL (g_stiNetworkDebug,
		stiTrace ("NetworkDebug: %s: State: 0x%x -> 0x%x\n", m_serviceName.c_str(), m_lastProgressState, m_progressState);
	);

	if (m_progressState != m_lastProgressState)
	{
		//
		// If we transitioned from an online state to a non-online state then
		// indicate that we are disconnected.
		//
		if ((m_lastProgressState & STATE_ONLINE) == STATE_ONLINE)
		{
			if ((m_progressState & STATE_ONLINE) != STATE_ONLINE)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
						vpe::trace("NetworkDebug: ", m_serviceName, ": transitioned to offline\n");
				);

				serviceOfflineStateChangedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), m_blocked);
				m_blocked = false;
			}
		}
		else
		{
			if ((m_progressState & STATE_ONLINE) == STATE_ONLINE)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
						vpe::trace("NetworkDebug: ", m_serviceName, ": transitioned to online\n");
				);

				serviceOnlineStateChangedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()));
			}
		}

		m_lastProgressState = m_progressState;
	}
}

void NetworkService::stateSet(
	std::string state)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkDebug: ", m_serviceName, ": State moves to ", state, "\n");
	);

	if (state == stateReady)
	{
		m_serviceState = estiServiceReady;
		m_progressState |= STATE_READY;
	}
	else if (state == stateAssociation)
	{
		m_serviceState = estiServiceAssociation;
		m_progressState &= ~STATE_READY;
	}
	else if (state == stateConfiguration)
	{
		m_serviceState = estiServiceConfiguration;
		m_progressState &= ~STATE_READY;
	}
	else if (state == stateOnline)
	{
		m_serviceState = estiServiceOnline;
		m_progressState |= STATE_READY;
	}
	else if (state == stateDisconnect)
	{
		m_serviceState = estiServiceDisconnect;
		m_progressState = STATE_NONE;
	}
	else if (state == stateIdle)
	{
		m_serviceState = estiServiceIdle;
		m_progressState = STATE_NONE;
	}
	else if (state == stateFailure)
	{
		m_serviceState = estiServiceFailure;
		m_progressState = STATE_NONE;
	}
}

void NetworkService::typeSet(std::string type)
{
/*
 * i've seen both Ethernet and ethernet for this.....
 */
	if (type == propertyEthernet || type == "ethernet")
	{
		m_technologyType = technologyType::wired;
	}
	else if (type == propertyWifi)
	{
		m_technologyType = technologyType::wireless;
	}
	else if (type == propertyBluetooth)				// we don't really care about this
	{
		m_technologyType = technologyType::bluetooth;
	}
	else
	{
		vpe::trace("NetworkService::", __func__, ": got unknown technologyType ", type, "\n");
	}
}

void NetworkService::securitySet(std::string security)
{
	if (security == propertySecurityNone)
	{
		m_securityType = estiWIRELESS_OPEN;	// no security
	}
	else if (security == propertySecurityEap)
	{
		m_securityType = estiWIRELESS_EAP;	// enterprise
	}
	else if (security == propertySecurityWep)
	{
		m_securityType = estiWIRELESS_WEP;	// we don't support this
	}
	else
	{
		m_securityType = estiWIRELESS_WPA;		// covers psk and wpa
	}
}

void NetworkService::stringListGet(GVariant *values, std::vector<std::string> *list)
{

	GVariant *variant = values;
	GVariant *var2 = nullptr;
	GVariantIter *iter = nullptr;
	gchar *str;

	if (g_variant_is_of_type(values, G_VARIANT_TYPE_VARIANT))
	{
		var2 = g_variant_get_variant(values);
		variant = var2;
	}

	list->clear();
	g_variant_get(variant, "as", &iter);

	if (iter)
	{
		while (g_variant_iter_loop(iter, "s", &str))
		{
			if (str && strlen(str))
			{
				list->push_back(str);
			}
			else
			{
				break;
			}
		}
		g_variant_iter_free(iter);
		iter = nullptr;
	}

	if (var2)
	{
		g_variant_unref (var2);
		var2 = nullptr;
	}
}

void NetworkService::ipv4Parse(GVariant *value)
{
	GVariant *dict = value;
	GVariant *dict2 = nullptr;
	GVariantIter *iter = nullptr;
	GVariant *variant = nullptr;

	std::string tmpStr;

	if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT))
	{
		dict2 = g_variant_get_variant(value);
		dict = dict2;
	}

	iter = g_variant_iter_new(dict);
	while((variant = g_variant_iter_next_value(iter)))
	{
		GVariant *tmpVariant = nullptr;
		std::string name;

		tmpVariant = g_variant_get_child_value(variant, 0);
		name = g_variant_get_string(tmpVariant, nullptr);
		g_variant_unref(tmpVariant);
		tmpVariant = nullptr;

		tmpVariant = g_variant_get_child_value(variant, 1);

		if (name == propertyMethod)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv4.method = tmpStr;

			if (tmpStr == methodDhcp)
			{
				m_useDhcp = true;
			}
			else if (tmpStr == methodManual)
			{
				m_useDhcp = false;
			}
			else if (tmpStr == methodOff)
			{
				m_useDhcp = false;
			}
			m_progressState |= STATE_METHOD;

			g_variant_unref (var);
			var = nullptr;

			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": got method ", tmpStr, ", adding STATE_METHOD\n");
			);
		}
		else if (name == propertyAddress)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv4.address = tmpStr;
			m_progressState |= STATE_ADDRESS;

			g_variant_unref (var);
			var = nullptr;

			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": got Address: ", tmpStr, ", adding STATE_ADDRESS\n");
			);
		}
		else if (name == propertyIpv4Netmask)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv4.netmask = tmpStr;
			m_progressState |= STATE_NETMASK;

			g_variant_unref (var);
			var = nullptr;

			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": got Netmask: ", tmpStr, ", adding STATE_NETMASK\n");
			);

		}
		else if (name == propertyGateway)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv4.gateway = tmpStr;
			m_progressState |= STATE_GATEWAY;

			g_variant_unref (var);
			var = nullptr;

			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": got Gateway: ", tmpStr, ", adding STATE_GATEWAY\n");
			);
		}

		g_variant_unref(tmpVariant);
		tmpVariant = nullptr;

		g_variant_unref(variant);
		variant = nullptr;
	}

	g_variant_iter_free(iter);
	iter = nullptr;

	if (dict2)
	{
		g_variant_unref(dict2);
		dict2 = nullptr;
	}
}

void NetworkService::ipv6Parse(GVariant *values)
{
	GVariantIter *iter;
	GVariant *dict;
	GVariant *variant = values;
	GVariant *variant2 = nullptr;

	std::string tmpStr;

	if (g_variant_is_of_type(values, G_VARIANT_TYPE_VARIANT))
	{
		variant2 = g_variant_get_variant(values);
		variant = variant2;
	}

	iter = g_variant_iter_new(variant);

	if (variant2)
	{
		g_variant_unref(variant2);
		variant2 = nullptr;
	}

/*
 * i don't know why, but the original would only set any of the state bits only if it was alreay in the ready state
 */
	while((dict = g_variant_iter_next_value(iter)))
	{
		std::string name;
		GVariant *tmpVariant = nullptr;

		tmpVariant = g_variant_get_child_value(dict, 0);
		name = g_variant_get_string(tmpVariant, nullptr);
		g_variant_unref(tmpVariant);
		tmpVariant = nullptr;

		tmpVariant = g_variant_get_child_value(dict, 1);

		if (name == propertyMethod)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv6.method = tmpStr;
			if (tmpStr == methodAuto)
			{
				m_useDhcp = true;
			}
			else if (tmpStr == methodManual)
			{
				m_useDhcp = false;
			}
			else if (tmpStr == method6to4)
			{
				m_useDhcp = true;				// this method can't be set manually
			}
			else if (tmpStr == methodOff)
			{
				m_useDhcp = false;
			}
			if (m_progressState & STATE_READY)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					vpe::trace("NetworkDebug: adding STATE_METHOD\n");
				);
            	m_progressState |= STATE_METHOD;
			}

			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyIpv6PrefixLength)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ipv6.prefixLength = g_variant_get_byte(var);
			if (m_progressState & STATE_READY)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					vpe::trace("NetworkDebug: adding STATE_NETMASK\n");
				);
				m_progressState |= STATE_NETMASK;
			}

			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyGateway)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ipv6.gateway = g_variant_get_string(var, nullptr);
			if (m_progressState & STATE_READY)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					vpe::trace("NetworkDebug: adding STATE_GATEWAY\n");
				);
				m_progressState |= STATE_GATEWAY;
			}

			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyAddress)
		{
			auto var = g_variant_get_variant(tmpVariant);
			tmpStr = g_variant_get_string(var, nullptr);
			m_ipv6.address = tmpStr;

			if (m_progressState & STATE_READY)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					vpe::trace("NetworkDebug: adding STATE_ADDRESS\n");
				);
				m_progressState |= STATE_ADDRESS;
			}

			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyIpv6Privacy)
		{
			auto var = g_variant_get_variant(tmpVariant);

			m_ipv6.privacy = g_variant_get_string(var, nullptr);

			g_variant_unref (var);
			var = nullptr;
		}

		g_variant_unref (tmpVariant);
		tmpVariant = nullptr;

		g_variant_unref (dict);
		dict = nullptr;
	}

	g_variant_iter_free(iter);
	iter = nullptr;
}

void NetworkService::ethernetParse(GVariant *values)
{
	GVariantIter *iter;
	GVariant *dict;
	GVariant *variant = values;
	GVariant *variant2 = nullptr;

	if (g_variant_is_of_type(values, G_VARIANT_TYPE_VARIANT))
	{
		variant2 = g_variant_get_variant(values);
		variant = variant2;
	}
	iter = g_variant_iter_new(variant);

	if (variant2)
	{
		g_variant_unref(variant2);
		variant2 = nullptr;
	}

	while((dict = g_variant_iter_next_value(iter)))
	{
		std::string name;
		GVariant *tmpVariant = nullptr;

		tmpVariant = g_variant_get_child_value(dict, 0);
		name = g_variant_get_string(tmpVariant, nullptr);
		g_variant_unref(tmpVariant);
		tmpVariant = nullptr;
		tmpVariant = g_variant_get_child_value(dict, 1);

		if (name == propertyMethod)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ethernet.method = g_variant_get_string(var, nullptr);
			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyEthernetInterface)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ethernet.interfaceName = g_variant_get_string(var, nullptr);
			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyEthernetMtu)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ethernet.mtu = g_variant_get_uint16(var);
			g_variant_unref (var);
			var = nullptr;
		}
		else if (name == propertyAddress)
		{
			auto var = g_variant_get_variant(tmpVariant);
			m_ethernet.address = g_variant_get_string(var, nullptr);
			g_variant_unref (var);
			var = nullptr;
		}
		g_variant_unref(tmpVariant);
		tmpVariant = nullptr;

		g_variant_unref (dict);
		dict = nullptr;
	}
	g_variant_iter_free(iter);
	iter = nullptr;
}

void NetworkService::serviceChangedCallback(
	CstiNetworkService *proxy,
	const gchar *tmpName,
	GVariant *value,
	gpointer userData)
{
	auto self = (NetworkService *)userData;

	g_variant_ref (value);
	std::string name(tmpName);
	self->m_dbusObject->eventQueueGet ()->PostEvent (
		[self, proxy, name, value]()
		{
			self->eventServiceChanged (proxy, name, value);

			g_variant_unref (value);
		});
}

void NetworkService::eventServiceChanged(
	CstiNetworkService *proxy,
	const std::string &name,
	GVariant *value)
{
	std::string tmpStr;

	if (name == propertyState)
	{
		auto var = g_variant_get_variant(value);
		tmpStr = g_variant_get_string(var, nullptr);

		stateSet(tmpStr);
		serviceStateCheck();
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyError)
	{
		auto var = g_variant_get_variant(value);
		tmpStr = g_variant_get_string(var, nullptr);

		if (tmpStr == "blocked")
		{
			blockHandle();
		}
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyName)
	{
		auto var = g_variant_get_variant(value);
		m_serviceName = g_variant_get_string(var, nullptr);

		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyType)
	{
		auto var = g_variant_get_variant(value);
		tmpStr = g_variant_get_string(var, nullptr);

		typeSet(tmpStr);
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertySecurity)			// we should only receive this if it's wireless
	{
		const gchar **vector = nullptr;
		gsize size = 0;
		auto var = g_variant_get_variant(value);
		vector = g_variant_get_strv(var, &size);
		if (vector && *vector)
		{
			tmpStr = *vector;
			securitySet(tmpStr);
		}
		else
		{
			m_securityType = estiWIRELESS_OPEN;	// no security
		}

		g_free(vector);
		vector = nullptr;

		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyStrength)
	{
		auto var = g_variant_get_variant(value);
		m_signalStrength = g_variant_get_byte(var);
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyFavorite)
	{
		auto var = g_variant_get_variant(value);
		m_favorite = g_variant_get_boolean(var);
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyImmutable)
	{
		auto var = g_variant_get_variant(value);
		m_immutable = g_variant_get_boolean(var);
		g_variant_unref(var);
		var = nullptr;
	}
	else if (name == propertyAutoConnect)
	{
		auto var = g_variant_get_variant(value);
		m_autoConnect = g_variant_get_boolean(var);
		g_variant_unref(var);
		var = nullptr;
		stiDEBUG_TOOL(g_stiNetworkDebug,
			vpe::trace("NetworkDebug: ", m_serviceName, ": setting autoConnect to ", m_autoConnect, "\n");
		);
	}
	else if (name == propertyRoaming)
	{
	}
	else if (name == propertyNameServers)
	{
		stringListGet(value, &m_nameServers);

		if (!m_nameServers.empty())
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": adding STATE_NAMESERVERS\n");
			);
			m_progressState |= STATE_NAMESERVERS;
		}

		serviceStateCheck();
	}
	else if (name == propertyNameServersConfig)
	{
	}
	else if (name == propertyTimeServers || name == propertyTimeServersConfig)
	{
		stringListGet(value, &m_timeServers);

		if (!m_timeServers.empty())
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": adding STATE_TIMESERVERS\n");
			);
			m_progressState |= STATE_TIMESERVERS;
		}
		serviceStateCheck();
	}
	else if (name == propertyDomains || name == propertyDomainsConfig)
	{
		stringListGet(value, &m_domains);

		if (!m_domains.empty())
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", m_serviceName, ": adding STATE_DOMAINS\n");
			);
			m_progressState |= STATE_DOMAINS;
		}
		serviceStateCheck();
	}
	else if (name == propertyIpv4)
	{
		ipv4Parse(value);
		serviceStateCheck();
	}
	else if (name == propertyIpv6)
	{
		ipv6Parse(value);
		serviceStateCheck();
	}
	else if (name == propertyEthernet)
	{
		ethernetParse(value);
	}
}

void NetworkService::propertiesProcess(GVariant *properties)
{
	if (m_serviceProxy == nullptr)
	{
		g_variant_ref (properties);
		m_queuedProperties.push (properties);
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
			std::string name(property);
			eventServiceChanged(m_serviceProxy, name, tmp);
			g_variant_unref(tmp);
			tmp = nullptr;
			g_variant_unref(dict);
			dict = nullptr;
		}
	}
}

void NetworkService::serviceConnect()
{
	stiASSERT (m_serviceProxy);
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkService::", __func__, ": enter\n");
	);

	csti_network_service_call_connect(
		m_serviceProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::serviceConnectFinish, this, nullptr));
}

void NetworkService::serviceConnectFinish(
	GObject *proxy,
	GAsyncResult *result,
	gpointer userData)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkService::", __func__, ": enter\n");
	);

	DbusError dbusError;
	connectionError error = connectionError::none;

	csti_network_service_call_connect_finish(
		m_serviceProxy,
		result,
		dbusError.get());

	if (dbusError.valid())
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			vpe::trace("NetworkService::", __func__, ": Failed to connect service ", m_serviceName, ": ", dbusError.message(), "\n");
		);

		if (dbusError.message().find("AlreadyConnected") == std::string::npos)
		{
			if (dbusError.message().find("Input/output") != std::string::npos)
			{
				error = connectionError::ioError;
			}
			else if (dbusError.message().find("InProgress") != std::string::npos)
			{
				error = connectionError::inProgress;
			}
			else
			{
				error = connectionError::other;
			}
		}
		else
		{
			error = connectionError::alreadyConnected;
		}

		serviceConnectionErrorSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), error);
	}

	if (error == connectionError::none)
	{
		serviceConnectedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()));
	}
}

void NetworkService::serviceDisconnect()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkService::", __func__, ": enter\n");
	);

	csti_network_service_call_disconnect(
		m_serviceProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::serviceDisconnectFinish, this, nullptr));
}

void NetworkService::serviceDisconnectFinish(
	GObject *proxy,
	GAsyncResult *result,
	gpointer userData)
{

	DbusError dbusError;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkService::", __func__, ": enter\n");
	);

	csti_network_service_call_disconnect_finish(
		m_serviceProxy,
		result,
		dbusError.get());

	if (dbusError.valid()
	 && dbusError.message().find("NotConnected") == std::string::npos)
	{
		vpe::trace("NetworkService::", __func__, ": Failed to disconnect service ", m_serviceName, ": ", dbusError.message(), "\n");
		serviceDisconnectedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), false);
	}
	else
	{
		serviceDisconnectedSignal.Emit(std::static_pointer_cast<NetworkService>(shared_from_this()), true);
	}
}

connectionError NetworkService::synchronousDisconnect()
{
	DbusError dbusError;
	connectionError ret = connectionError::none;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkDebug: ", __func__, ": enter\n");
	);

	m_cancelled = true;

	csti_network_service_call_disconnect_sync(
		m_serviceProxy,
		nullptr,
		dbusError.get());

	if (dbusError.valid())
	{
		if (dbusError.message().find("NotConnected") != std::string::npos)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: cancel returns not connected\n");
			);
			ret = connectionError::notConnected;
		}
		else
		{
			vpe::trace("NetworkService::", __func__, ": ", dbusError.message(), "\n");
			ret = connectionError::other;
		}
	}
	return ret;
}


void NetworkService::enableDHCP ()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkDebug: ", __func__ , ": enter\n");
	);
	m_progressState &= ~(HAVE_IPV4 | STATE_NAMESERVERS);
	serviceStateCheck ();

	const char *nullServers [3] = { "", "", "" };

	propertySet ( "Nameservers.Configuration",
			g_variant_new_variant(g_variant_new_strv(nullServers, 3)));

	auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "Method", g_variant_new_string("dhcp"));
	g_variant_builder_add(builder, "{sv}", "Address", g_variant_new_string(""));
	g_variant_builder_add(builder, "{sv}", "Netmask", g_variant_new_string(""));
	g_variant_builder_add(builder, "{sv}", "Gateway", g_variant_new_string(""));

	propertySet( "IPv4.Configuration",
			g_variant_new_variant(g_variant_builder_end(builder)));

	g_variant_builder_unref(builder);
	builder = nullptr;
}


void NetworkService::disableDHCP(
	const SstiNetworkSettings &settings)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkDebug: ", __func__, ": enter\n");
	);
	m_progressState &= ~(HAVE_IPV4 | STATE_NAMESERVERS);
	serviceStateCheck ();

	auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "Method", g_variant_new_string("manual"));

	if (!settings.IPv4IP.empty ())
	{
		g_variant_builder_add(builder, "{sv}", "Address", g_variant_new_string(settings.IPv4IP.c_str()));
	}

	if (!settings.IPv4SubnetMask.empty ())
	{
		g_variant_builder_add(builder, "{sv}", "Netmask", g_variant_new_string(settings.IPv4SubnetMask.c_str()));
	}

	if (!settings.IPv4NetGatewayIP.empty ())
	{
		g_variant_builder_add(builder, "{sv}", "Gateway", g_variant_new_string(settings.IPv4NetGatewayIP.c_str()));
	}

	propertySet("IPv4.Configuration", g_variant_new_variant(g_variant_builder_end(builder)));

	g_variant_builder_unref(builder);
	builder = nullptr;

	int size = 0;
	const char *nameservers[3]{};

	if (!settings.IPv4DNS1.empty())
	{
		nameservers[size++] = settings.IPv4DNS1.c_str();
	}

	if (!settings.IPv4DNS2.empty())
	{
		nameservers[size++] = settings.IPv4DNS2.c_str();
	}

	if (size > 0)
	{
		propertySet( "Nameservers.Configuration",
				g_variant_new_variant(g_variant_new_strv(nameservers, size)));
	}
}

void NetworkService::serviceRemove()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkDebug: ", __func__, ": removing ", m_serviceName, "\n");
	);

	csti_network_service_call_remove(
		m_serviceProxy,
		nullptr,
		DbusCall::finish,
		DbusCall::bind2(m_dbusObject->eventQueueGet (), &NetworkService::serviceRemoveFinish, this, nullptr));
}

void NetworkService::serviceRemoveFinish(
	GObject *proxy,
	GAsyncResult *result,
	gpointer userData)
{
	DbusError dbusError;

	csti_network_service_call_remove_finish(
		m_serviceProxy,
		result,
		dbusError.get());

	if (dbusError.valid())
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			vpe::trace("NetworkService::", __func__, ": Failed to remove service ", m_serviceName, ": ", dbusError.message(), "\n");
		);
	}
}

void NetworkService::blockHandle()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace("NetworkService::", __func__, ": enter\n");
	);
	m_blocked = true;
	synchronousDisconnect();
	m_cancelled = false;		// synchronousDisconnect sets this
	propertyClear("Error");
	serviceRemove();
}

