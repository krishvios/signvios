#pragma once

#include <string>
#include <gst/gst.h>
#include "GStreamerStructure.h"

class GStreamerCaps
{
public:

	GStreamerCaps () = default;

	GStreamerCaps (
		GstCaps *caps,
		bool addRef)
	:
		m_caps (caps)
	{
		if (addRef)
		{
			gst_caps_ref(m_caps);
		}
	}

	GStreamerCaps (const GStreamerCaps &other) = delete;

	GStreamerCaps (GStreamerCaps &&other)
	{
		if (other.m_caps)
		{
			m_caps = other.m_caps;
			other.m_caps = nullptr;
		}
	}

	GStreamerCaps &operator = (const GStreamerCaps &other) = delete;

	GStreamerCaps &operator = (GStreamerCaps &&other)
	{
		if (this != &other)
		{
			clear ();

			if (other.m_caps)
			{
				m_caps = other.m_caps;
				other.m_caps = nullptr;
			}
		}

		return *this;
	}

	GStreamerCaps (
		const std::string &mediaType,
		const char *fieldName,
		...)
	{
#if GST_VERSION_MAJOR >= 1
		m_caps = gst_caps_new_empty_simple (mediaType.c_str ());

		va_list argp;
		va_start(argp, fieldName);
		gst_caps_set_simple_valist (m_caps, fieldName, argp);
		va_end(argp);
#else
		m_caps = gst_caps_new_empty ();

		va_list argp;
		va_start(argp, fieldName);
		auto gstStructure = gst_structure_new_valist (mediaType.c_str (), fieldName, argp);
		va_end(argp);

		gst_caps_append_structure (m_caps, gstStructure);
#endif
	}

	GStreamerCaps (
		const std::string &capsString)
	{
		m_caps = gst_caps_from_string (capsString.c_str ());
	}

	~GStreamerCaps ()
	{
		clear ();
	}

	GStreamerStructure structureGet (unsigned int index)
	{
		return {gst_caps_get_structure (get (), 0)};
	}

	GstCaps *get () const
	{
		return m_caps;
	}

	void clear ()
	{
		if (m_caps)
		{
			gst_caps_unref (m_caps);
			m_caps = nullptr;
		}
	}

private:

	GstCaps *m_caps {nullptr};
};
