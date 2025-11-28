// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BluetoothDbusInterface.h"
#include "stiSVX.h"
#include "CstiBluetoothInterface.h"
#include "CstiSignal.h"


class GattManagerInterface: public BluetoothDbusInterface
{
public:

	GattManagerInterface (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, GATT_MANAGER_INTERFACE)
	{
	}

	~GattManagerInterface () override
	{
		if (m_gattManagerProxy)
		{
			g_object_unref (m_gattManagerProxy);
			m_gattManagerProxy = nullptr;
		}
	}

	GattManagerInterface (const GattManagerInterface &other) = delete;
	GattManagerInterface (GattManagerInterface &&other) = delete;
	GattManagerInterface &operator= (const GattManagerInterface &other) = delete;
	GattManagerInterface &operator= (GattManagerInterface &&other) = delete;

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	static CstiSignal<std::shared_ptr<GattManagerInterface>> addedSignal;

	stiHResult autoReconnectSetup (
		GDBusObjectManagerServer *objectManagerServer);

private:

	void eventGattManagerAdd (
		GObject *proxy,
		GAsyncResult *res);

	void eventRegisterApplication (
		GObject *proxy,
		GAsyncResult *res);

	static gboolean ReleaseProfile (
		CstiBluetoothGattProfile1 *proxy,
		GDBusMethodInvocation *pInvocation,
		gpointer);

	CstiBluetoothGattManager1 *m_gattManagerProxy = nullptr;
};

