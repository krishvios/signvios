// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiDefs.h"
#include "CstiBluetoothInterface.h"
#include "BluetoothDbusInterface.h"
#include "CstiSignal.h"


class AdapterInterface: public BluetoothDbusInterface
{
public:

	AdapterInterface (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, BLUETOOTH_ADAPTER_INTERFACE)
	{
	}

	~AdapterInterface () override
	{
		if (m_adapterProxy)
		{
			g_object_unref (m_adapterProxy);
			m_adapterProxy = nullptr;
		}
	}

	AdapterInterface (const AdapterInterface &other) = delete;
	AdapterInterface (AdapterInterface &&other) = delete;
	AdapterInterface &operator= (const AdapterInterface &other) = delete;
	AdapterInterface &operator= (AdapterInterface &&other) = delete;

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	void removed () override;

	stiHResult scanStart ();

	stiHResult scanCancel ();

	void removeDevice (
		const std::string &objectPath);

	void connectDevice (
		const std::string &deviceAddress,
		const std::string &addressType);

	static CstiSignal<std::shared_ptr<AdapterInterface>> addedSignal;
	static CstiSignal<std::shared_ptr<AdapterInterface>> removedSignal;
	static CstiSignal<std::shared_ptr<AdapterInterface>> poweredOnSignal;
	static CstiSignal<> scanCompletedSignal;
	static CstiSignal<> scanFailedSignal;
	static CstiSignal<> removeDeviceCompletedSignal;
	static CstiSignal<> removeDeviceFailedSignal;
	static CstiSignal<std::string> connectDeviceSucceededSignal;
	static CstiSignal<std::string> connectDeviceFailedSignal;

	bool poweredGet () const
	{
		return m_powered;
	}

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	void eventScanStartFinish (
		GObject *proxy,
		GAsyncResult *pRes);

	void eventScanCancelFinish (
		GObject *proxy,
		GAsyncResult *pRes);

	void eventRemoveDevice (
		GObject *proxy,
		GAsyncResult *pRes);

	void eventConnectDevice (
		GObject *proxy,
		GAsyncResult *pRes,
		const std::string &deviceAddress);

	bool m_powered = false;
	bool m_scanning = false;

	CstiBluetoothAdapter1 *m_adapterProxy = nullptr;
};

