#pragma once

#include <gst/gst.h>
#include "stiError.h"
#include "stiDefs.h"


static void bufferDestructionCallback (gpointer mem)
{
	auto callback = static_cast<std::function<void()> *>(mem);

	if (callback)
	{
		(*callback) ();

		delete callback;
	}
}


class GStreamerBuffer
{
public:

	GStreamerBuffer ()
	{
		m_buffer = gst_buffer_new ();
	}

	GStreamerBuffer (
		size_t size)
	{
#if GST_VERSION_MAJOR >= 1
		m_buffer = gst_buffer_new_allocate (nullptr, size, nullptr);
#else
		m_buffer = gst_buffer_new_and_alloc (size);
#endif
	}


#if GST_VERSION_MAJOR >= 1
	GStreamerBuffer (
		gpointer data,
		gsize size,
		std::function<void()> callback)
	{
		if (callback)
		{
			m_buffer = gst_buffer_new_wrapped_full ((GstMemoryFlags)0, data, size, 0, size,
					reinterpret_cast<gpointer>(new std::function<void()>(callback)), bufferDestructionCallback);
		}
		else
		{
			m_buffer = gst_buffer_new_wrapped_full ((GstMemoryFlags)0, data, size, 0, size, nullptr, nullptr);
		}
	}
#else
	GStreamerBuffer (
		uint8_t *data,
		gsize size,
		std::function<void()> callback)
	{
		m_buffer = gst_buffer_new ();

		GST_BUFFER_DATA(m_buffer) = data;
		GST_BUFFER_SIZE(m_buffer) = size;

		if (callback)
		{
			GST_BUFFER_MALLOCDATA(m_buffer) = reinterpret_cast<guint8 *>(new std::function<void()>(callback));
			GST_BUFFER_FREE_FUNC (m_buffer) = bufferDestructionCallback;
		}
	}
#endif

#if GST_VERSION_MAJOR >= 1
	GStreamerBuffer (
		GstPadProbeInfo *info)
	{
		if ((info->type & GST_PAD_PROBE_TYPE_BUFFER) != 0)
		{
			m_buffer = static_cast<GstBuffer *>(info->data);
			gst_buffer_ref (m_buffer);
		}
	}
#endif

	GStreamerBuffer (
		GstBuffer *buffer,
		bool addRef)
	:
		m_buffer(buffer)
	{
		if (m_buffer && addRef)
		{
			gst_buffer_ref (m_buffer);
		}
	}

	~GStreamerBuffer ()
	{
		clear ();
	}

	GStreamerBuffer (const GStreamerBuffer &other)
	{
		if (other.m_buffer)
		{
			m_buffer = other.m_buffer;
			gst_buffer_ref (m_buffer);
		}
	}

	GStreamerBuffer (GStreamerBuffer &&other)
	{
		if (other.m_buffer)
		{
			m_buffer = other.m_buffer;
#if GST_VERSION_MAJOR >= 1
			other.unmap ();
#endif
			other.m_buffer = nullptr;
		}
	}

	GStreamerBuffer &operator = (const GStreamerBuffer &other)
	{
		if (this != &other)
		{
			clear ();

			if (other.m_buffer)
			{
				m_buffer = other.m_buffer;
				gst_buffer_ref (m_buffer);
			}
		}

		return *this;
	}

	GStreamerBuffer &operator = (GStreamerBuffer &&other)
	{
		if (this != &other)
		{
			clear ();

			if (other.m_buffer)
			{
				m_buffer = other.m_buffer;
				other.m_buffer = nullptr;
			}
		}

		return *this;
	}

	GstBuffer *get () const
	{
		return m_buffer;
	}

	uint8_t *dataGet ()
	{
		if (m_buffer)
		{
#if GST_VERSION_MAJOR >= 1
			map (GST_MAP_READ);

			return m_gstMapInfo.data;
#else
			return GST_BUFFER_DATA(m_buffer);
#endif
		}

		return nullptr;
	}

	size_t sizeGet ()
	{
		if (m_buffer)
		{
#if GST_VERSION_MAJOR >= 1
			map (GST_MAP_READ);

			return m_gstMapInfo.size;
#else
			return GST_BUFFER_SIZE(m_buffer);
#endif
		}

		return 0;
	}

	void merge (
		GStreamerBuffer &other)
	{
#if GST_VERSION_MAJOR >= 1
		gst_buffer_append (m_buffer, other.get ());
#else
		m_buffer = gst_buffer_merge (m_buffer, other.get ());
#endif
	}

	stiHResult dataCopy (
		GStreamerBuffer &other)
	{
		return dataCopy (other.dataGet (), other.sizeGet ());
	}

	stiHResult dataCopy (
		uint8_t *data,
		unsigned int length)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiTESTCOND (m_buffer != nullptr, stiRESULT_ERROR);

		{
#if GST_VERSION_MAJOR >= 1
			map (GST_MAP_WRITE);
			memcpy (m_gstMapInfo.data, data, length);
			unmap ();
#else
			memcpy (GST_BUFFER_DATA(m_buffer), data, length);
#endif
		}

	STI_BAIL:
		return hResult;
	}

	void metaDataCopy (
		GStreamerBuffer &other)
	{
		if (m_buffer)
		{
#if GST_VERSION_MAJOR >= 1
			gst_buffer_copy_into (m_buffer, other.get (), GST_BUFFER_COPY_META, 0, -1);
#else
			gst_buffer_copy_metadata (m_buffer, other.get (), GST_BUFFER_COPY_ALL);
#endif
		}
		else
		{
			stiASSERT(false);
		}
	}

	void flagSet (
		int flag)
	{
		if (m_buffer)
		{
			GST_BUFFER_FLAG_SET (m_buffer, flag); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		}
		else
		{
			stiASSERT(false);
		}
	}

	void flagUnset (
		int flag)
	{
		if (m_buffer)
		{
			GST_BUFFER_FLAG_UNSET (m_buffer, flag); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		}
		else
		{
			stiASSERT(false);
		}
	}

	int flagsGet ()
	{
		if (m_buffer)
		{
			return GST_BUFFER_FLAGS (m_buffer); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		}

		stiASSERT(false);
		return 0;
	}

	bool flagIsSet (int flag) const
	{
		if (m_buffer)
		{
			return GST_BUFFER_FLAG_IS_SET (m_buffer, flag); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		}

		stiASSERT(false);
		return false;
	}

	uint64_t timestampGet ()
	{
		if (m_buffer)
		{
			return GST_BUFFER_TIMESTAMP(m_buffer); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		}

		stiASSERT (false);
		return 0;
	}

	GstClockTime durationGet ()
	{
		if (m_buffer)
		{
			return GST_BUFFER_DURATION (m_buffer);
		}

		stiASSERT (false);
		return 0;
	}

	int offsetGet ()
	{
		if (m_buffer)
		{
			return GST_BUFFER_OFFSET (m_buffer);
		}

		stiASSERT(false);
		return 0;
	}

	int offsetEndGet ()
	{
		if (m_buffer)
		{
			return GST_BUFFER_OFFSET_END (m_buffer);
		}

		stiASSERT(false);
		return 0;
	}

	void clear ()
	{
		if (m_buffer)
		{
#if GST_VERSION_MAJOR >= 1
			unmap ();
#endif

			gst_buffer_unref (m_buffer);
			m_buffer = nullptr;
		}
	}

	void discard ()
	{
		m_buffer = nullptr;
	}

private:

	GstBuffer *m_buffer {nullptr};

#if GST_VERSION_MAJOR >= 1
	void map (
		GstMapFlags flags)
	{
		if (!m_mapped)
		{
			gst_buffer_map (m_buffer, &m_gstMapInfo, flags);
			m_mapped = true;
		}
	}
#endif

#if GST_VERSION_MAJOR >= 1
	void unmap ()
	{
		if (m_mapped)
		{
			gst_buffer_unmap (m_buffer, &m_gstMapInfo);
			m_mapped = false;
		}
	}
#endif

#if GST_VERSION_MAJOR >= 1
	bool m_mapped {false};
	GstMapInfo m_gstMapInfo {};
#endif
};
