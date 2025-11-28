#pragma once


#include "GStreamerBin.h"
#include <gst/gst.h>


class GStreamerPipeline : public GStreamerBin
{
public:

	GStreamerPipeline () = default;

	GStreamerPipeline (
		const std::string &name,
		bool addToPipelines = true)
	{
		create (name, addToPipelines);
	}
		
	GStreamerPipeline (
		GstElement *gstElement,
		bool addRef,
		bool addToPipelines = true)
	:
		GStreamerBin (GST_BIN(gstElement), addRef, addToPipelines)
	{
	}

	GStreamerPipeline (const GStreamerPipeline &other) = default;
	GStreamerPipeline (GStreamerPipeline &&other) = default;
	GStreamerPipeline &operator= (const GStreamerPipeline &other) = default;
	GStreamerPipeline &operator= (GStreamerPipeline &&other) = default;

	~GStreamerPipeline () override = default;

	void create (
		const std::string &name,
		bool addToPipelines = true)
	{
		auto element = gst_pipeline_new (name.c_str ());

		set (element);
		m_busWatches.clear ();

		if (addToPipelines)
		{
			m_pipelines[name] = getWeakPtr ();
		}
	}

	void busWatchAdd (
		GstBusFunc func,
		gpointer userData)
	{
		auto bus = gst_pipeline_get_bus (GST_PIPELINE(get ()));

		auto watch = gst_bus_add_watch (bus, func, userData);

		m_busWatches.push_back (std::make_shared<BusWatch>(watch));

		gst_object_unref (bus);
	}

private:

	class BusWatch
	{
	public:
		BusWatch (guint watch)
		:
			m_watch (watch)
		{
		}

		~BusWatch ()
		{
			g_source_remove (m_watch);
		}

		BusWatch (const BusWatch &other) = default;
		BusWatch (BusWatch &&other) = default;
		BusWatch &operator= (const BusWatch &other) = default;
		BusWatch &operator= (BusWatch &&other) = default;

	private:

		guint m_watch;
	};

	std::vector<std::shared_ptr<BusWatch>> m_busWatches;
};

