#include "stiSVX.h"
#include "GStreamerElement.h"


#if GST_VERSION_MAJOR >= 1
static gboolean foreachElementPad (GstElement *element, GstPad *pad, gpointer userData)
{
	auto callback = static_cast<std::function<bool(const GStreamerPad &)> *>(userData);

	return (*callback) (GStreamerPad(pad, true));
}
#endif

bool GStreamerElement::foreachPad (std::function<bool(const GStreamerPad &)> callback) const
{
#if GST_VERSION_MAJOR >= 1
	return gst_element_foreach_pad (get (), foreachElementPad, &callback);
#else
	return false;
#endif
}

bool GStreamerElement::foreachProperty (
	std::function<bool(const GParamSpec &param, const GValue &propertyValue)> callback,
	bool nonDefaultOnly) const
{
	guint numProperties {0};
	auto propertySpecs = g_object_class_list_properties(
		G_OBJECT_GET_CLASS(get ()),
		&numProperties);

	if (propertySpecs)
	{
		for (guint i = 0; i < numProperties; i++)
		{
			auto paramSpec = propertySpecs[i];

			if (paramSpec && (paramSpec->flags & G_PARAM_READABLE))
			{
				GValue propertyValue {};

				g_value_init(&propertyValue, paramSpec->value_type);

				g_object_get_property(G_OBJECT(get ()), paramSpec->name, &propertyValue);

				if (!nonDefaultOnly || !g_param_value_defaults(paramSpec, &propertyValue))
				{
					callback (*paramSpec, propertyValue);
				}
			}
		}

		g_free(propertySpecs);
	}

	return true;
}


Poco::DynamicStruct GStreamerElement::asJson () const
{
	Poco::DynamicStruct e;

	e["class"] = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(get ()));
	e["name"] = nameGet ();

	auto path  = gst_object_get_path_string(GST_OBJECT(get()));

	if (path)
	{
		e["path"] = std::string(path);

		g_free(path);
	}

	Poco::JSON::Array pads;

	foreachPad(
		[&pads] (const GStreamerPad &pad)
		{
			pads.add (pad.asJson ());

			return true;
		});

	e["pads"] = pads;

	Poco::JSON::Array properties;

	// Retrieve the properties that have non-default values
	foreachProperty(
		[&properties] (const GParamSpec &paramSpec, const GValue &propertyValue)
		{
			properties.add (getPropertyAsJson (paramSpec, propertyValue));

			return true;
		},
		true);

	e["properties"] = properties;

	return e;
}


std::string GStreamerElement::propertyInfoGet (
	const std::string &name) const
{
	auto paramSpec = g_object_class_find_property(G_OBJECT_GET_CLASS(get ()), name.c_str());

	if (paramSpec) {
		return g_param_spec_get_blurb(paramSpec);
	}

	return {};
}

#if GST_VERSION_MAJOR >= 1
template <typename T>
bool convertToInteger(
	const std::string &str,
	T *output)
{
	if (std::is_signed<T>::value)
	{
		gint64 value;
		GError *error {nullptr};
		gint64 min {G_MININT64};
		gint64 max {G_MAXINT64};

		if (std::is_same<T, gint>::value)
		{
			min = G_MININT;
			max = G_MAXINT;
		}
		else if (std::is_same<T, glong>::value)
		{
			min = G_MINLONG;
			max = G_MAXLONG;
		}

		if (g_ascii_string_to_signed (
			str.c_str(),
			10,
			min,
			max,
			&value,
			&error))
		{
			*output = static_cast<T>(value);
			return true;
		}
	}
	else
	{
		guint64 value;
		GError *error {nullptr};
		guint64 max {G_MAXUINT64};

		if (std::is_same<T, guint>::value)
		{
			max = G_MAXUINT;
		}
		else if (std::is_same<T, gulong>::value)
		{
			max = G_MAXULONG;
		}

		if (g_ascii_string_to_unsigned (
			str.c_str(),
			10,
			0,
			max,
			&value,
			&error))
		{
			*output = static_cast<T>(value);
			return true;
		}
	}

	return false;
}


void GStreamerElement::propertiesSet(
	const std::vector<std::pair<std::string, std::string>> &properties)
{
	stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);
	std::vector<const char *> propertyNames;
	std::vector<GValue> propertyValues;

	for (const auto &property: properties)
	{
		auto paramSpec = g_object_class_find_property(G_OBJECT_GET_CLASS(get ()), property.first.c_str());

		GValue propertyValue {};

		g_value_init(&propertyValue, paramSpec->value_type);

		if (g_value_type_transformable(G_TYPE_STRING, paramSpec->value_type))
		{
			GValue sourceValue {};
			g_value_init(&sourceValue, G_TYPE_STRING);
			g_value_set_static_string(&sourceValue, property.second.c_str());

			g_value_transform(&sourceValue, &propertyValue);

			propertyNames.push_back(property.first.c_str());
			propertyValues.push_back(propertyValue);
		}
		else
		{
			switch (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(&propertyValue)))
			{
				case G_TYPE_INT:
				{
					gint value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_int(&propertyValue, value);
					}

					break;
				}
				case G_TYPE_UINT:
				{
					guint value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_uint(&propertyValue, value);
					}

					break;
				}

				case G_TYPE_LONG:
				{
					glong value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_long(&propertyValue, value);
					}

					break;
				}

				case G_TYPE_ULONG:
				{
					gulong value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_ulong(&propertyValue, value);
					}

					break;
				}

				case G_TYPE_INT64:
				{
					gint64 value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_int64(&propertyValue, value);
					}
					break;
				}

				case G_TYPE_UINT64:
				{
					guint64 value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_uint64(&propertyValue, value);
					}

					break;
				}

				case G_TYPE_BOOLEAN:
				{
					g_value_set_boolean(&propertyValue, (property.second == "true"));
					break;
				}

				case G_TYPE_ENUM:
				{
					gint value {0};
					if (convertToInteger(property.second, &value))
					{
						g_value_set_enum(&propertyValue, value);
					}

					break;
				}

				case G_TYPE_BOXED:
				{
					if (G_VALUE_TYPE(&propertyValue) == GST_TYPE_CAPS)
					{
						auto caps = gst_caps_from_string(property.second.c_str());

						if (caps)
						{

							gst_value_set_caps(&propertyValue, caps);
						}
						else
						{
							stiTHROWMSG(stiRESULT_ERROR, "Could not convert string to caps: %s", property.second.c_str());
						}
					}
					else
					{
						stiTHROWMSG(stiRESULT_ERROR, "Unhandled boxed pointer");
					}

					break;
				}

				default:
					stiTHROWMSG(stiRESULT_ERROR, "Unhandled type: %d", G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(&propertyValue)));
			}

			propertyNames.push_back(property.first.c_str());
			propertyValues.push_back(propertyValue);
		}
	}

	g_object_setv(G_OBJECT(get()), propertyNames.size(), propertyNames.data(), propertyValues.data());

STI_BAIL:;
}
#endif

