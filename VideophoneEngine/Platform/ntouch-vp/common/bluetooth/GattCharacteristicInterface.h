// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BluetoothDbusInterface.h"
#include <deque>
#include <mutex>
#include "stiSVX.h"
#include "CstiBluetoothInterface.h"
#include "CstiSignal.h"

class SstiBluezDevice;
class SstiGattService;
class SstiGattDescriptor;


class SstiGattCharacteristic : public BluetoothDbusInterface
{
public:
	SstiGattCharacteristic (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, GATT_CHARACTERISTIC_INTERFACE)
	{
	}

	~SstiGattCharacteristic () override
	{
		if (m_gattCharacteristicProxy)
		{
			g_object_unref (m_gattCharacteristicProxy);
			m_gattCharacteristicProxy = nullptr;
		}
	}

	SstiGattCharacteristic (const SstiGattCharacteristic &other) = delete;
	SstiGattCharacteristic (SstiGattCharacteristic &&other) = delete;
	SstiGattCharacteristic &operator= (const SstiGattCharacteristic &other) = delete;
	SstiGattCharacteristic &operator= (SstiGattCharacteristic &&other) = delete;

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	void removed () override;

	static CstiSignal<std::shared_ptr<SstiBluezDevice>, std::string, const std::vector<uint8_t> &> valueChangedSignal;
	static CstiSignal<std::string> readWriteFailedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>, std::string> readWriteCompletedSignal;
	static CstiSignal<std::string, std::string> startNotifyCompletedSignal;
	static CstiSignal<> startNotifyFailedSignal;

	inline int WriteSequenceNumberGet () {
		return m_writeSequenceNumber;
	};

	stiHResult WriteQueuePacketsRemove (
		std::string value);

	stiHResult Write (
		std::string value);

	void read (
		const std::shared_ptr<SstiBluezDevice> &device);

	void write (
		const std::shared_ptr<SstiBluezDevice> &device,
		const char *value,
		int len);

	void notifyStart ();

	void notifyStop ();

	std::shared_ptr<SstiGattService> serviceInterfaceGet ();

	inline std::string uuidGet () const
	{
		return m_uuid;
	}

	std::vector<unsigned char> valueGet () const
	{
		return m_value;
	}

	bool notifyingGet () const
	{
		return m_notifying;
	}

	void descriptorAdd (
		std::shared_ptr<SstiGattDescriptor> descriptor)
	{
		m_descriptors.push_back (descriptor);
	}

	void descriptorRemove (
		std::shared_ptr<SstiGattDescriptor> descriptor)
	{
		m_descriptors.remove (descriptor);
	}

	std::string servicePathGet () const
	{
		return m_servicePath;
	}

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	void eventReadFinish (
		GObject *proxy,
		GAsyncResult *res,
		const std::shared_ptr<SstiBluezDevice> &device);

	static void WriteFinish (
		GObject *object,
		GAsyncResult *res,
		gpointer UserData);

	void eventWriteFinish (
		GObject *object,
		GAsyncResult *res,
		const std::shared_ptr<SstiBluezDevice> &device);

	void eventStartNotifyFinish(
		GObject *proxy,
		GAsyncResult *res);

	void eventStopNotifyFinish(
		GObject *proxy,
		GAsyncResult *res);

	std::string m_uuid;
	std::string m_servicePath;
	std::vector<unsigned char> m_value;
	bool m_notifying = false;
	std::list<std::string> m_flags;
	std::list<std::shared_ptr<SstiGattDescriptor>> m_descriptors;

	std::deque<std::string> m_writePacketQueue;
	int m_writeSequenceNumber = 0;
	bool m_writing = false;
	std::mutex m_writingMutex;

	CstiBluetoothGattCharacteristic1 *m_gattCharacteristicProxy = nullptr;
};

