// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "DbusObject.h"

class DbusInterfaceBase: public std::enable_shared_from_this<DbusInterfaceBase>
{
public:
	DbusInterfaceBase () = delete;
	DbusInterfaceBase (
		DbusObject *dbusObject,
		const std::string &interfaceName)
	:
		m_dbusObject (dbusObject),
		m_interfaceName (interfaceName)
	{
	}

	DbusInterfaceBase (const DbusInterfaceBase &other) = delete;
	DbusInterfaceBase (DbusInterfaceBase &&other) = delete;
	DbusInterfaceBase &operator= (const DbusInterfaceBase &other) = delete;
	DbusInterfaceBase &operator= (DbusInterfaceBase &&other) = delete;

	virtual ~DbusInterfaceBase () = default;

	void interfaceMake (
		DbusObject *dbusObject,
		const std::string &interfaceName) {}

	virtual void added (
		GVariant *properties) {}

	virtual void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) {}

	virtual void removed () {}

	std::string objectPathGet ()
	{
		return m_dbusObject->objectPathGet ();
	}

	std::string interfaceNameGet ()
	{
		return m_interfaceName;
	}

protected:

	DbusObject *m_dbusObject {nullptr};
	std::string m_interfaceName;

private:

};


/*
 * get a named boolean value from a dict
 */
bool boolVariantGet(GVariant *dict, const std::string &propertyName);
std::string stringVariantGet(GVariant *dict, const std::string &propertyName);
