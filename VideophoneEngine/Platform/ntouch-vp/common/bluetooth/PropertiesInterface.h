// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiBluetoothInterface.h"
#include "BluetoothDbusInterface.h"
#include "CstiSignal.h"


class PropertiesInterface : public BluetoothDbusInterface
{
public:

	PropertiesInterface (
		DbusObject *dbusObject);

	PropertiesInterface (const PropertiesInterface &other) = delete;
	PropertiesInterface (PropertiesInterface &&other) = delete;
	PropertiesInterface &operator= (const PropertiesInterface &other) = delete;
	PropertiesInterface &operator= (PropertiesInterface &&other) = delete;

	~PropertiesInterface () override;

	void added (
		GVariant *properties) override;

	void propertySet (
		const std::string &interfaceName,
		const std::string &property,
		GVariant *value);

private:

	void eventPropertiesProxyNew (
		GObject *proxy,
		GAsyncResult *res);

	void eventPropertySet (
		GObject *proxy,
		GAsyncResult *res,
		const std::string &interfaceName,
		const std::string &property);

	CstiBluetoothOrgFreedesktopDBusProperties *m_propertyProxy = nullptr;
};

