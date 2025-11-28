// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#include "DbusObject.h"
#include "DbusInterfaceBase.h"
#include "stiTrace.h"

void valuePrint (
	GVariant *tmp2,
	int indent,
	bool *newline)
{
	std::string indentStr (indent, '\t');

	if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_VARIANT)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto var = g_variant_get_variant(tmp2);

		valuePrint (var, indent, newline);

		g_variant_unref(var);
		tmp2 = nullptr;
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_BOOLEAN)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		if (g_variant_get_boolean (tmp2))
		{
			stiTrace ("true");
		}
		else
		{
			stiTrace ("false");
		}
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_STRING) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	 || g_variant_is_of_type (tmp2, G_VARIANT_TYPE_OBJECT_PATH)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		const char *value = g_variant_get_string(tmp2, nullptr);

		stiTrace ("%s", value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_INT16)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto value = g_variant_get_int16 (tmp2);

		stiTrace ("%d (0x%04x)", value, value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_UINT16)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto value = g_variant_get_uint16 (tmp2);

		stiTrace ("%d (0x%04x)", value, (uint16_t)value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_INT32)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto value = g_variant_get_int32 (tmp2);

		stiTrace ("%d (0x%08x)", value, value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_UINT32)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto value = g_variant_get_uint32 (tmp2);

		stiTrace ("%d (0x%08x)", value, (uint32_t)value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_BYTE)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		auto value = g_variant_get_byte (tmp2);

		stiTrace ("%d (0x%02x)", value, value);
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_STRING_ARRAY)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		const gchar **strings = nullptr;
		gsize size = 0;

		strings = g_variant_get_strv(tmp2, &size);

		stiTrace ("\n");

		for (gsize i = 0; i < size; i++)
		{
			stiTrace ("%s%s\n", indentStr.c_str (), strings[i]);
		}

		g_free (strings);
		strings = nullptr;

		*newline = true;
	}
	else if (g_variant_is_of_type (tmp2, G_VARIANT_TYPE_OBJECT_PATH_ARRAY)) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		const gchar **strings = nullptr;
		gsize size = 0;

		strings = g_variant_get_objv(tmp2, &size);

		stiTrace ("\n");

		for (gsize i = 0; i < size; i++)
		{
			stiTrace ("%s%s\n", indentStr.c_str (), strings[i]);
		}

		g_free(strings);
		strings = nullptr;

		*newline = true;
	}
	else if ((g_variant_is_of_type (tmp2, G_VARIANT_TYPE_BYTESTRING))) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		gsize size = 0;
		auto value = (const gint8 *)g_variant_get_fixed_array (tmp2, &size, sizeof (gint8)); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

		if (size > 0)
		{
			for (gsize i = 0; i < size; i++)
			{
				stiTrace ("0x%02x ", value[i]);
			}
		}
	}
	else if ((g_variant_is_of_type (tmp2, G_VARIANT_TYPE_DICTIONARY))) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	{
		stiTrace ("\n");

		int lim1 = g_variant_n_children(tmp2);
		for (int i = 0; i < lim1; i++)
		{
			auto entry = g_variant_get_child_value(tmp2, i);

			stiTrace ("%s", indentStr.c_str ());

			int lim2 = g_variant_n_children(entry);
			for (int j = 0; j < lim2; j++)
			{
				auto value1 = g_variant_get_child_value(entry, j);

				// Since this is a dictionary, delimit the key from the the
				// rest with a colon while delimit the remaining values with
				// a comma.
				if (j == 1)
				{
					stiTrace (": ");
				}
				else if (j > 1)
				{
					stiTrace (", ");
				}

				valuePrint (value1, indent + 1, newline);

				g_variant_unref (value1);
				value1 = nullptr;
			}

			stiTrace ("\n");

			g_variant_unref (entry);
			entry = nullptr;

			*newline = true;
		}
	}
	else
	{
		stiTrace ("<Unknown: %s>", g_variant_get_type_string(tmp2));
	}
}


void propertiesDump (
	GVariant *properties,
	const std::vector<std::string> &invalidatedProperties)
{
	int lim = g_variant_n_children(properties);
	for (int i = 0; i < lim; i++)
	{
		GVariant *dict = g_variant_get_child_value(properties, i);
		GVariant *tmp = g_variant_get_child_value(dict, 0);
		const char *property = g_variant_get_string(tmp, nullptr);

		stiTrace ("\tproperty: %s: ", property);

		auto tmp1 = g_variant_get_child_value (dict, 1);

		bool newline = false;

		valuePrint (tmp1, 2, &newline);

		if (!newline)
		{
			stiTrace ("\n");
		}

		g_variant_unref(tmp1);
		tmp1 = nullptr;
		g_variant_unref(tmp);
		tmp = nullptr;
		g_variant_unref (dict);
		dict = nullptr;
	}

	for (auto &property: invalidatedProperties)
	{
		stiTrace ("\tinvalidated property: %s\n", property.c_str ());
	}
}


std::shared_ptr<DbusInterfaceBase> DbusObject::interfaceGet (
	const std::string &interfaceName)
{
	auto iter = m_interfaceMap.find (interfaceName);

	if (iter != m_interfaceMap.end ())
	{
		return iter->second;
	}
	else
	{
		stiASSERTMSG (false, "Interface not found: %s\n", interfaceName.c_str ());
		return nullptr;
	}
}


void DbusObject::interfaceAdded (
	const std::string &interfaceName,
	GVariant *dict,
	std::function<std::shared_ptr<DbusInterfaceBase>(DbusObject *dbusObject, const std::string &interfaceName)> interfaceMake)
{
	auto iter = m_interfaceMap.find (interfaceName);

	if (iter == m_interfaceMap.end ())
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
			stiTrace ("Object %s: Adding interface %s\n", m_objectPath.c_str(), interfaceName.c_str ());
			// below line causes crashes
			//propertiesDump(dict, std::vector<std::string>());
		);

		auto interface = interfaceMake (this, interfaceName);

		if (interface)
		{
			interface->added (dict);
		}

		m_interfaceMap.insert (std::pair<std::string, std::shared_ptr<DbusInterfaceBase>>(interfaceName, interface));
	}
	else
	{
		stiASSERTMSG (false, "%s interface already added for %s\n", interfaceName.c_str (), m_objectPath.c_str ());
	}
}

void DbusObject::interfaceChanged(
	const std::string &interfaceName,
	GVariant *changedProperties,
	const std::vector<std::string> &invalidatedProperties)
{
	auto dbusInterface = interfaceGet (interfaceName);

	if (dbusInterface)
	{
		dbusInterface->changed (changedProperties, invalidatedProperties);
	}
	else
	{
		stiASSERTMSG (false, "%s interface not found on object %s\n", interfaceName.c_str (), m_objectPath.c_str ());
	}
}


void DbusObject::interfaceRemoved (
	const std::string &interfaceName)
{
	auto iter = m_interfaceMap.find (interfaceName);

	if (iter != m_interfaceMap.end ())
	{
		// Interfaces that have a null pointer are not errors. They are interfaces we don't care about.
		if (iter->second)
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug > 1,
				stiTrace("Object %s: Removing interface %s\n", m_objectPath.c_str(), interfaceName.c_str ());
			);

			iter->second->removed ();
		}

		m_interfaceMap.erase (iter);
	}
	else
	{
		stiASSERTMSG (false, "Failed to remove %s interface from %s\n", interfaceName.c_str (), m_objectPath.c_str ());
	}
}
