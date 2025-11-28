#pragma once

#include "GStreamerBuffer.h"
#include "GStreamerElement.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>


class GStreamerAppSource : public GStreamerElement
{
public:

	GStreamerAppSource () = default;

	GStreamerAppSource (
		const char *name)
	:
		GStreamerElement ("appsrc", name)
	{
		m_callbacks = std::make_shared<Callbacks>();
	}

	GStreamerAppSource (const GStreamerAppSource &other)
	:
		GStreamerElement (other),
		m_callbacks(other.m_callbacks)
	{
	}

	GStreamerAppSource (GStreamerAppSource &&other)
	:
		GStreamerElement (other),
		m_callbacks (other.m_callbacks)
	{
		other.m_callbacks = nullptr;
	}

	GStreamerAppSource &operator= (const GStreamerAppSource &other)
	{
		if (this != &other)
		{
			GStreamerElement::operator=(other);
			m_callbacks = other.m_callbacks;
		}

		return *this;
	}

	GStreamerAppSource &operator= (GStreamerAppSource &&other)
	{
		if (this != &other)
		{
			GStreamerElement::operator=(other);

			m_callbacks = other.m_callbacks;
			other.m_callbacks = nullptr;
		}

		return *this;
	}

	~GStreamerAppSource () override = default;

	GstFlowReturn pushBuffer (
		GStreamerBuffer &gstBuffer)
	{
		GstFlowReturn ret = gst_app_src_push_buffer (GST_APP_SRC(get ()), gstBuffer.get ());

		// Ownership fully transfers to gst_app_src_push_buffer. We need to discard the ownership
		// we had.
		if (ret == GST_FLOW_OK)
		{
			gstBuffer.discard ();
		}

		return ret;
	}

	void streamTypeSet (
		GstAppStreamType type)
	{
		gst_app_src_set_stream_type(GST_APP_SRC(get ()), type);
	}

	void sizeSet (
		gint64 size)
	{
		gst_app_src_set_size (GST_APP_SRC(get ()), size);
	}

	void capsSet (
		GStreamerCaps &caps)
	{
		gst_app_src_set_caps(GST_APP_SRC(get ()), caps.get ());
	}

	static void needDataCallback (
		GstAppSrc *appSrc,
		guint length,
		gpointer userData)
	{
		auto self = static_cast<GStreamerAppSource *>(userData);

		if (self->m_callbacks && self->m_callbacks->m_needDataCallback)
		{
			self->m_callbacks->m_needDataCallback (*self);
		}
	}

	void needDataCallbackSet (
		std::function<void (GStreamerAppSource &)> func)
	{
		m_callbacks->m_needDataCallback = func;

		m_callbacks->m_gstCallbacks.need_data = needDataCallback;

		gst_app_src_set_callbacks(GST_APP_SRC(get ()), &m_callbacks->m_gstCallbacks, this, nullptr);
	}

	static void enoughDataCallback (
		GstAppSrc *appSrc, gpointer userData)
	{
		auto self = static_cast<GStreamerAppSource *>(userData);

		if (self->m_callbacks && self->m_callbacks->m_enoughDataCallback)
		{
			self->m_callbacks->m_enoughDataCallback (*self);
		}
	}

	void enoughDataCallbackSet (
		std::function<void (GStreamerAppSource &)> func)
	{
		m_callbacks->m_enoughDataCallback = func;

		m_callbacks->m_gstCallbacks.enough_data = enoughDataCallback;

		gst_app_src_set_callbacks(GST_APP_SRC(get ()), &m_callbacks->m_gstCallbacks, this, nullptr);
	}

	void clear () override
	{
		m_callbacks = nullptr;

		GStreamerElement::clear ();
	}

private:

	struct Callbacks
	{
		std::function<void (GStreamerAppSource &)> m_needDataCallback;
		std::function<void (GStreamerAppSource &)> m_enoughDataCallback;
		GstAppSrcCallbacks m_gstCallbacks {};
	};

	std::shared_ptr<Callbacks> m_callbacks;
};
