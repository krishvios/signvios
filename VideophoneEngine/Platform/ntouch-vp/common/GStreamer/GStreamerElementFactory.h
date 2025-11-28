#pragma once

#include <gst/gst.h>

class GStreamerElementFactory
{
public:

	GStreamerElementFactory () = default;

	GStreamerElementFactory (const std::string &name)
	:
		m_factory (gst_element_factory_find(name.c_str ()))
	{
	}
	
	~GStreamerElementFactory ()
	{
		clear ();
	}
	
	GStreamerElementFactory (const GStreamerElementFactory &other) = delete;
	GStreamerElementFactory (GStreamerElementFactory &&other) = delete;

	GStreamerElementFactory &operator= (const GStreamerElementFactory &other)
	{
		{
			if (&other != this)
			{
				clear ();

				m_factory = other.m_factory;
				gst_object_ref (m_factory);
			}

			return *this;
		}
	}

	GStreamerElementFactory &operator= (GStreamerElementFactory &&other)
	{
		if (&other != this)
		{
			clear ();

			m_factory = other.m_factory;
			other.m_factory = nullptr;
		}

		return *this;
	}

	GStreamerElement createElement ()
	{
		return {get ()};
	}

	GStreamerElement createElement (const std::string &name)
	{
		return {get (), name};
	}
	
	void pluginRankSet (int rank)
	{
		gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(get ()), rank);
	}
	
	GstElementFactory *get ()
	{
		return m_factory;
	}
	
	void clear ()
	{
		if (m_factory)
		{
			gst_object_unref (m_factory);
			m_factory = nullptr;
		}
	}

private:
	
	GstElementFactory *m_factory {nullptr};
};
