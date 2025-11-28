// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BluetoothDbusInterface.h"
#include <list>
#include "CstiBluetoothInterface.h"

class SstiGattCharacteristic;


class SstiGattDescriptor : public BluetoothDbusInterface
{
public:
	SstiGattDescriptor (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, GATT_DESCRIPTOR_INTERFACE)
	{
	}

	SstiGattDescriptor (const SstiGattDescriptor &other) = delete;
	SstiGattDescriptor (SstiGattDescriptor &&other) = delete;
	SstiGattDescriptor &operator= (const SstiGattDescriptor &other) = delete;
	SstiGattDescriptor &operator= (SstiGattDescriptor &&other) = delete;

	~SstiGattDescriptor () override
	{
		if (m_gattDescriptorProxy)
		{
			g_object_unref (m_gattDescriptorProxy);
			m_gattDescriptorProxy = nullptr;
		}
	}

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	void removed () override;

	std::shared_ptr<SstiGattCharacteristic> characteristicInterfaceGet ();

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	std::string m_uuid;
	std::string m_characteristicPath;
	std::vector<unsigned char> m_value;
	std::list<std::string> m_flags;

	CstiBluetoothGattDescriptor1 *m_gattDescriptorProxy {nullptr};
};
