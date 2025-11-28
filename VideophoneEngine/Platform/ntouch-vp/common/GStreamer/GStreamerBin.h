#pragma once

#include "GStreamerElement.h"
#include <gst/gst.h>
#include <map>
#include <vector>
#include <memory>
#include <tuple>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>

class Element
{
public:

	std::string m_name;
	std::string m_factoryName;
};


class GStreamerBin : public GStreamerElement
{
public:

	GStreamerBin () = default;

	GStreamerBin (
		const std::string &name,
		bool addToPipelines = true)
	{
		create (name, addToPipelines);
	}

	GStreamerBin (
		GstBin *gstBin,
		bool addRef,
		bool addToPipelines = true)
	:
		GStreamerElement (GST_ELEMENT(gstBin), addRef)
	{
		if (addToPipelines) {
			pipelinesAdd();
		}
	}

	GStreamerBin (
		GstElementType gstBin,
		bool addToPipelines = true)
	:
		GStreamerElement (gstBin)
	{
		if (addToPipelines) {
			pipelinesAdd();
		}
	}

	GStreamerBin (const GStreamerBin &other) = default;
	GStreamerBin (GStreamerBin &&other) = default;
	GStreamerBin &operator= (const GStreamerBin &other) = default;
	GStreamerBin &operator= (GStreamerBin &&other) = default;

	GStreamerBin &operator= (const GStreamerElement &other)
	{
		if (this != &other)
		{
			clear ();

			if (GST_IS_BIN(other.get()))
			{
				GStreamerElement::operator=(other);
			}
		}

		return *this;
	}

	~GStreamerBin () override = default;

	void create (
		const std::string &name,
		bool addToPipelines = true)
	{
		auto element = gst_bin_new(name.c_str ());
		gst_object_ref_sink (element);

		set (element);

		if (addToPipelines) {
			pipelinesAdd();
		}
	}

	void pipelinesAdd()
	{
		auto name = nameGet();

		if (!name.empty())
		{
			m_pipelines[name] = getWeakPtr ();
		}
	}

	void elementAdd (GStreamerElement &element)
	{
		gst_bin_add (GST_BIN(get ()), element.get ());
	}

	void elementRemove (
		GStreamerElement &element)
	{
		gst_bin_remove(GST_BIN(get ()), element.get ());
	}

	GStreamerElement elementGetByName (const std::string &name)
	{
		GStreamerElement returnElement;
		
		auto element = gst_bin_get_by_name (GST_BIN(get ()), name.c_str ());
		if (element)
		{
			returnElement = GStreamerElement (element, false);
		}

		return returnElement;
	}

	bool empty () const;

	bool foreachElement (std::function<bool(const GStreamerElement &)> callback) const;

	Poco::DynamicStruct asJson () const override;

	static GStreamerBin find (const std::string &name);
	static GStreamerPad findPad (const std::string &path);
	static GStreamerElement findElement (const std::string &path);

	void writeGraph (const std::string &fileName);
	std::string getGraph ();

	static std::vector<std::string> binsGet ();

protected:

	static std::map<std::string, std::weak_ptr<GstElement>> m_pipelines;
};
