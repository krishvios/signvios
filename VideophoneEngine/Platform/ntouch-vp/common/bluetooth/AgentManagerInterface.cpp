// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "AgentManagerInterface.h"
#include "DbusCall.h"
#include "DbusError.h"
#include "IstiBluetooth.h"
#include "stiTrace.h"


CstiSignal<std::shared_ptr<AgentManagerInterface>> AgentManagerInterface::addedSignal;
CstiSignal<> AgentManagerInterface::agentRegistrationCompletedSignal;
CstiSignal<> AgentManagerInterface::agentRegistrationFailedSignal;


void AgentManagerInterface::added(
	GVariant *properties)
{
	csti_bluetooth_agent_manager1_proxy_new (
			m_dbusObject->dbusConnectionGet ()->connectionGet (),
			G_DBUS_PROXY_FLAGS_NONE,
			BLUETOOTH_SERVICE,
			objectPathGet().c_str (),
			nullptr,
			DbusCall::finish,
			DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AgentManagerInterface::eventProxyNew, this));
}


void AgentManagerInterface::eventProxyNew (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;

	m_agentManagerProxy = csti_bluetooth_agent_manager1_proxy_new_finish (res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "%s: Can't get default agent manager proxy: %s\n", __func__, dbusError.message ().c_str ());
	}
	else
	{
		addedSignal.Emit (std::static_pointer_cast<AgentManagerInterface>(shared_from_this()));
	}
}


void AgentManagerInterface::agentRegister (
	const std::string &agentPath)
{
	csti_bluetooth_agent_manager1_call_register_agent (
		m_agentManagerProxy,
		agentPath.c_str (),
		"KeyboardDisplay",
		nullptr,
		DbusCall::finish,
		DbusCall::bind2 (m_dbusObject->eventQueueGet (), &AgentManagerInterface::eventRegisterAgent, this));
}


void AgentManagerInterface::eventRegisterAgent (
	GObject *proxy,
	GAsyncResult *res)
{
	DbusError dbusError;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto agentManager = reinterpret_cast<CstiBluetoothAgentManager1 *>(proxy);

	csti_bluetooth_agent_manager1_call_register_agent_finish (agentManager, res, dbusError.get ());

	if (dbusError.valid ())
	{
		stiASSERTMSG (false, "Can't register agent: %s\n", dbusError.message ().c_str ());
		agentRegistrationFailedSignal.Emit ();
	}
	else
	{
		agentRegistrationCompletedSignal.Emit ();
	}
}


