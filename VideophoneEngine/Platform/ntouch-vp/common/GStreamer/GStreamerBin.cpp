#include "GStreamerBin.h"
#include "stiTools.h"
#include <fstream>


std::map<std::string, std::weak_ptr<GstElement>> GStreamerBin::m_pipelines;


std::vector<std::string> GStreamerBin::binsGet ()
{
	std::vector<std::string> pipelines;

	for (auto iter = m_pipelines.begin (); iter != m_pipelines.end ();)
	{
		auto element = (*iter).second.lock ();

		if (element)
		{
			pipelines.push_back((*iter).first);

			++iter;
		}
		else
		{
			// We couldn't get the lock so the shared pointer
			// must have been freed. Remove the entry from
			// the map.
			iter = m_pipelines.erase(iter);
		}
	}

	return pipelines;
}


bool GStreamerBin::empty () const
{
	auto iterator = gst_bin_iterate_elements (GST_BIN(get()));

#if GST_VERSION_MAJOR >= 1
	GValue value = G_VALUE_INIT;
	auto result = gst_iterator_next (iterator, &value);
	g_value_unset(&value);
#else
	gpointer value = nullptr;
	auto result = gst_iterator_next (iterator, &value);
	if (result == GST_ITERATOR_OK)
	{
		g_object_unref(value);
	}
#endif

	gst_iterator_free (iterator);

	return result != GST_ITERATOR_OK;
}


void GStreamerBin::writeGraph (const std::string &fileName)
{
	GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(get ()), GST_DEBUG_GRAPH_SHOW_ALL, fileName.c_str ());
}


std::string GStreamerBin::getGraph ()
{
	const std::string fileName {"temporaryGraph"};

	writeGraph (fileName);

	std::ifstream input(std::string{"/tmp/"} + fileName + ".dot");

	return {(std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>())};
}


GStreamerBin GStreamerBin::find (
	const std::string &name)
{
	auto iter = m_pipelines.find (name);

	if (iter != m_pipelines.end ())
	{
		auto element = (*iter).second.lock ();

		if (element)
		{
			return {element};
		}

		// We couldn't get the lock so the shared pointer
		// must have been freed. Remove the entry from
		// the map.
		m_pipelines.erase(iter);
	}

	return {};
}


struct GStreamerPathComponent
{
	GStreamerPathComponent (
		const std::string className,
		const std::string name,
		const std::string memberClassName,
		const std::string memberName)
	:
		className(className),
		name(name),
		memberClassName(memberClassName),
		memberName(memberName)
	{
	}

	std::string className;
	std::string name;
	std::string memberClassName;
	std::string memberName;
};


std::vector<GStreamerPathComponent> decomposePath (
	const std::string &path)
{
	std::vector<GStreamerPathComponent> components;

	auto pathParts = splitString(path, '/');

	for (auto &pathPart: pathParts)
	{
		if (!pathPart.empty()) {
			auto objectParts = splitString(pathPart, '.');
			auto classAndName = splitString(objectParts[0], ':');

			std::vector<std::string> memberClassAndName {{}, {}};
			if (objectParts.size() > 1)
			{
				memberClassAndName = splitString(objectParts[1], ':');
			}

			components.emplace_back(
				classAndName[0],
				classAndName[1],
				memberClassAndName[0],
				memberClassAndName[1]);
		}
	}

	return components;
}


GStreamerPad GStreamerBin::findPad (
	const std::string &path)
{
	GStreamerPad foundPad;
	auto components = decomposePath(path);

	auto bin = GStreamerBin::find (components[0].name);

	if (bin.get())
	{
		for (size_t i = 1; i < components.size(); ++i)
		{
			GStreamerElement foundElement;
			auto &component = components[i];
			bin.foreachElement([component, &foundElement](const GStreamerElement &element){
				if (element.get() && element.nameGet() == component.name)
				{
					foundElement = element;
					return false;
				}

				return true;
			});

			if (!foundElement.get())
			{
				break;
			}

			// If we are at the last path component
			// and we have an element then check to
			// see if the pad is present.
			if (i == components.size() - 1)
			{
				foundElement.foreachPad(
					[component, &foundPad] (const GStreamerPad &pad)
					{
						if (pad.nameGet() == component.memberName)
						{
							foundPad = pad;
							return false;
						}

						return true;
					});
			}
			else
			{
				bin = foundElement;

				if (!bin.get())
				{
					break;
				}
			}
		}
	}

	return foundPad;
}


GStreamerElement GStreamerBin::findElement (
	const std::string &path)
{
	GStreamerElement element;
	auto components = decomposePath(path);

	auto bin = GStreamerBin::find (components[0].name);

	if (bin.get())
	{
		for (size_t i = 1; i < components.size(); ++i)
		{
			GStreamerElement foundElement;
			auto &component = components[i];
			bin.foreachElement([component, &foundElement](const GStreamerElement &element){
				if (element.get() && element.nameGet() == component.name)
				{
					foundElement = element;
					return false;
				}

				return true;
			});

			if (!foundElement.get())
			{
				break;
			}

			// If we are at the last path component
			// and we have an element then check to
			// see if the pad is present.
			if (i == components.size() - 1)
			{
				element = foundElement;
			}
			else
			{
				bin = foundElement;

				if (!bin.get())
				{
					break;
				}
			}
		}
	}

	return element;
}


bool GStreamerBin::foreachElement (std::function<bool(const GStreamerElement &)> callback) const
{
#if GST_VERSION_MAJOR >= 1
	GstIterator *it = gst_bin_iterate_elements(GST_BIN(get ()));
	GValue item = G_VALUE_INIT;
	bool done = false;

	while (!done)
	{
		switch (gst_iterator_next (it, &item))
		{
			case GST_ITERATOR_OK:
			{
				auto object = g_value_get_object(&item);

				if (GST_IS_BIN(object))
				{
					done = !callback (GStreamerBin(GST_BIN(object), true, false));
				}
				else
				{
					done = !callback (GStreamerElement(static_cast<GstElement*>(object), true));
				}

				g_value_reset (&item);
				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;
			case GST_ITERATOR_ERROR:
			case GST_ITERATOR_DONE:
				done = true;
				break;
		}
	}

	g_value_unset (&item);
	gst_iterator_free (it);
#endif
	return true;
}


Poco::DynamicStruct GStreamerBin::asJson () const
{
	auto e = GStreamerElement::asJson();

	Poco::JSON::Array elements;

	foreachElement(
		[&elements] (const GStreamerElement &element)
		{
			elements.add (element.asJson ());

			return true;
		});

	e["elements"] = elements;

	return e;
}

