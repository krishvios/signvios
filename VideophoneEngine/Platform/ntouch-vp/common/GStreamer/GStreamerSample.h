#pragma once

#include "GStreamerBuffer.h"
#include "GStreamerCaps.h"

#include <gst/gst.h>


#if GST_VERSION_MAJOR < 1
using GstSample = GstBuffer;

inline void gst_sample_ref (GstBuffer *buffer)
{
	gst_buffer_ref (buffer);
}

inline void gst_sample_unref (GstBuffer *buffer)
{
	gst_buffer_unref (buffer);
}

inline GstBuffer *gst_sample_get_buffer (GstSample *sample)
{
	return sample;
}
#endif


class GStreamerSample
{
public:

	GStreamerSample ()
	{
#if GST_VERSION_MAJOR >= 1
		m_buffer = gst_buffer_new ();
		m_sample = gst_sample_new (m_buffer, nullptr, nullptr, nullptr);
#else
		m_sample = gst_buffer_new ();
#endif
	}

	GStreamerSample (
		GstSample *sample,
		bool addRef)
	:
		m_sample(sample)
	{
		if (m_sample && addRef)
		{
			gst_sample_ref (m_sample);
		}
	}

	GStreamerSample (
		GStreamerCaps &gstCaps)
	{
#if GST_VERSION_MAJOR >= 1
		m_buffer = gst_buffer_new ();
		m_sample = gst_sample_new (m_buffer, gstCaps.get (), nullptr, nullptr);
#else
		m_sample = gst_buffer_new ();
		gst_buffer_set_caps (m_sample, gstCaps.get ());
#endif
	}

#if GST_VERSION_MAJOR < 1
	GStreamerSample (
		GstMiniObject *miniObject)
	{
		m_sample = GST_BUFFER(miniObject);
		gst_sample_ref (m_sample);
	}
#endif

	~GStreamerSample ()
	{
		clear ();
	}

	GStreamerSample (const GStreamerSample &other)
	{
		if (other.m_sample)
		{
			m_sample = other.m_sample;
			gst_sample_ref (m_sample);
		}
	}

	GStreamerSample &operator = (const GStreamerSample &other)
	{
		if (this != &other)
		{
			clear ();

			if (other.m_sample)
			{
				m_sample = other.m_sample;
				gst_sample_ref (m_sample);
			}
		}

		return *this;
	}

	GStreamerSample (GStreamerSample &&other)
	{
		if (other.m_sample)
		{
			m_sample = other.m_sample;
			other.m_sample = nullptr;
		}

#if GST_VERSION_MAJOR >= 1
		if (other.m_buffer)
		{
			m_buffer = other.m_buffer;
			other.m_buffer = nullptr;
		}
#endif
	}

	GStreamerSample &operator = (GStreamerSample &&other)
	{
		if (this != &other)
		{
			clear ();

			if (other.m_sample)
			{
				m_sample = other.m_sample;
				other.m_sample = nullptr;
			}

#if GST_VERSION_MAJOR >= 1
			if (other.m_buffer)
			{
				m_buffer = other.m_buffer;
				other.m_buffer = nullptr;
			}
#endif
		}

		return *this;
	}

	GstSample *get () const
	{
		return m_sample;
	}

	GStreamerBuffer bufferGet () const
	{
		if (m_sample)
		{
			return { gst_sample_get_buffer (m_sample), true };
		}

		// There is no sample so return a null buffer
		return { nullptr, false };
	}

	GStreamerCaps capsGet ()
	{
#if GST_VERSION_MAJOR >= 1
		// gst_sample_get_caps *does not* transfer ownership so
		// we need to have GStreamerCaps add a reference.
		return { gst_sample_get_caps (m_sample), true };
#else
		// gst_sample_get_caps *does* transfer ownership so
		// we don't need to have GStreamerCaps add a reference.
		return { gst_buffer_get_caps (m_sample), false };
#endif
	}

	void clear ()
	{
		if (m_sample)
		{
			gst_sample_unref (m_sample);
			m_sample = nullptr;
		}

#if GST_VERSION_MAJOR >= 1
		if (m_buffer)
		{
			gst_buffer_unref (m_buffer);
			m_buffer = nullptr;
		}
#endif
	}

	void discard ()
	{
		m_sample = nullptr;
#if GST_VERSION_MAJOR >= 1
		m_buffer = nullptr;
#endif
	}

private:

	GstSample *m_sample {nullptr};
#if GST_VERSION_MAJOR >= 1
	GstBuffer *m_buffer {nullptr};
#endif
};
