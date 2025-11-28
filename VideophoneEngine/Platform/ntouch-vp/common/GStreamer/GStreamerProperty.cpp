#include "GStreamerProperty.h"
#include "stiTrace.h"
#include <tuple>
#include <sstream>

std::tuple<std::string, std::string> getTypeAndValueStrings (
	const GValue &propertyValue)
{
	std::string value = "<unknown type>";
	std::string type = g_type_name(G_VALUE_TYPE(&propertyValue));

	switch (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(&propertyValue)))
	{
		case G_TYPE_STRING:
		{
			auto stringValue = g_value_get_string (&propertyValue);

			if (stringValue == nullptr)
			{
				value = "<null>";
			}
			else {
				value = stringValue;
			}

			type = "String";

			break;
		}

		case G_TYPE_BOOLEAN:

			value = g_value_get_boolean (&propertyValue) ? "true" : "false";

			type = "Boolean";

			break;

		case G_TYPE_ULONG:

			value = std::to_string(g_value_get_ulong (&propertyValue));

			type = "ULong";

			break;

		case G_TYPE_LONG:

			value = std::to_string(g_value_get_long (&propertyValue));

			type = "Long";

			break;

		case G_TYPE_UINT:

			value = std::to_string(g_value_get_uint (&propertyValue));

			type = "UInteger";

			break;

		case G_TYPE_INT:

			value = std::to_string(g_value_get_int (&propertyValue));

			type = "Integer";

			break;

		case G_TYPE_UINT64:

			value = std::to_string(g_value_get_uint64 (&propertyValue));

			type = "UInteger64";

			break;

		case G_TYPE_INT64:

			value = std::to_string(g_value_get_int64 (&propertyValue));

			type = "Integer64";

			break;

		case G_TYPE_FLOAT:

			value = std::to_string(g_value_get_float (&propertyValue));

			type = "Float";

			break;

		case G_TYPE_DOUBLE:

			value = std::to_string(g_value_get_double (&propertyValue));

			type = "Double";

			break;

		case G_TYPE_ENUM:

			value = std::to_string(g_value_get_enum(&propertyValue));

			type = "Enum";

			break;

		case G_TYPE_FLAGS:
		{
			value = "<flags>";

			break;
		}

		case G_TYPE_OBJECT:
		{
			value = "<object>";

			auto object = g_value_get_object(&propertyValue);

			if (object)
			{
				std::stringstream ss;
				ss << object;
				value = ss.str();

				auto typeName = G_OBJECT_TYPE_NAME(object);

				if (typeName)
				{
					type = typeName;
				}

			}

			break;
		}

		case G_TYPE_BOXED:
		{
			if (G_VALUE_TYPE(&propertyValue) == GST_TYPE_CAPS)
			{
				const GstCaps *caps = gst_value_get_caps(&propertyValue);

				if (caps == nullptr)
				{
					value = "NULL";
				}
				else
				{
					if ( gst_caps_is_any( caps ))
					{
						value = "ANY";
					}
					else if (gst_caps_is_empty( caps ))
					{
						value = "EMPTY";
					}
					else
					{
						auto capsString = gst_caps_to_string (caps);

						if (capsString)
						{
							value = capsString;
						}
					}
				}

			}
			else
			{
				value = "<boxed pointer>";
			}

			break;
		}

		case G_TYPE_POINTER:
		{
			value = "<pointer>";

			break;
		}

		case G_TYPE_VARIANT:

			value = "<variant>";

			break;

		case G_TYPE_PARAM:

			value = "<param>";

			break;

		default:

			if (G_VALUE_TYPE(&propertyValue) == GST_TYPE_FRACTION)
			{
				value = "<fraction>";
			}

			break;
	}

	return std::make_tuple(type, value);
}


Poco::DynamicStruct getPropertyAsJson (
	const GParamSpec &paramSpec,
	const GValue &propertyValue)
{
	Poco::DynamicStruct p;

	auto name = g_param_spec_get_name (const_cast<GParamSpec *>(&paramSpec));

	if (name == nullptr)
	{
		p["name"] = "<empty>";
	}
	else
	{
		p["name"] = name;
	}

	p["writable"] = (paramSpec.flags & G_PARAM_WRITABLE);

	std::tie(p["type"], p["value"]) = getTypeAndValueStrings(propertyValue);

	return p;
}

