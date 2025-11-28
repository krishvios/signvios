// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BluetoothDbusInterface.h"
#include <list>
#include <algorithm>
#include "CstiBluetoothInterface.h"
#include "GattCharacteristicInterface.h"

class SstiBluezDevice;


class SstiGattService : public BluetoothDbusInterface
{
public:
	SstiGattService(
			DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, GATT_SERVICE_INTERFACE)
	{
	}

	~SstiGattService() override
	{
		if (m_serviceProxy)
		{
			g_object_unref (m_serviceProxy);
			m_serviceProxy = nullptr;
		}
	}

	SstiGattService (const SstiGattService &other) = delete;
	SstiGattService (SstiGattService &&other) = delete;
	SstiGattService &operator= (const SstiGattService &other) = delete;
	SstiGattService &operator= (SstiGattService &&other) = delete;

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	void removed () override;

	std::shared_ptr<SstiBluezDevice> deviceInterfaceGet ();

	void characteristicAdd (
		std::shared_ptr<SstiGattCharacteristic> characteristic)
	{
		m_characteristics.push_back (characteristic);
	}

	void characteristicRemove (
		std::shared_ptr<SstiGattCharacteristic> characteristic)
	{
		m_characteristics.remove (characteristic);
	}

	std::shared_ptr<SstiGattCharacteristic> characteristicFind (
		const std::string &uuid)
	{
		auto charIter = std::find_if (m_characteristics.begin (), m_characteristics.end (),
						[&uuid](const std::shared_ptr<SstiGattCharacteristic> &characteristic){return characteristic->uuidGet () == uuid;});

		if (charIter == m_characteristics.end ())
		{
			return nullptr;
		}
		else
		{
			return *charIter;
		}
	}

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	std::string m_uuid;
	bool m_primary = false;
	std::string m_devicePath;
	std::list<std::shared_ptr<SstiGattCharacteristic>> m_characteristics;

	CstiBluetoothGattService1 *m_serviceProxy = nullptr;
};

