// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiDefs.h"
#include "CstiBluetoothInterface.h"
#include "BluetoothDbusInterface.h"
#include "CstiSignal.h"


class AgentManagerInterface: public BluetoothDbusInterface
{
public:

	AgentManagerInterface (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, BLUETOOTH_AGENT_MANAGER_INTERFACE)
	{
	}

	~AgentManagerInterface () override
	{
		if (m_agentManagerProxy)
		{
			g_object_unref (m_agentManagerProxy);
			m_agentManagerProxy = nullptr;
		}
	}

	AgentManagerInterface (const AgentManagerInterface &other) = delete;
	AgentManagerInterface (AgentManagerInterface &&other) = delete;
	AgentManagerInterface &operator= (const AgentManagerInterface &other) = delete;
	AgentManagerInterface &operator= (AgentManagerInterface &&other) = delete;

	void added(
		GVariant *properties) override;

	void agentRegister (
		const std::string &agentPath);

	static CstiSignal<std::shared_ptr<AgentManagerInterface>> addedSignal;
	static CstiSignal<> agentRegistrationCompletedSignal;
	static CstiSignal<> agentRegistrationFailedSignal;

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	void eventRegisterAgent (
		GObject *proxy,
		GAsyncResult *res);

	CstiBluetoothAgentManager1 *m_agentManagerProxy = nullptr;
};

