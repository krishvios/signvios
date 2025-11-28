#pragma once

#include "stiError.h"
#include "stiDefs.h"
#include "GStreamerBuffer.h"
#include "GStreamerSample.h"
#include "GStreamerElement.h"

#include <gst/app/gstappsink.h>
#include <memory>
#include <functional>

class GStreamerAppSink : public GStreamerElement
{
public:

	GStreamerAppSink () = default;

	GStreamerAppSink (
		const char *name)
	:
		GStreamerElement ("appsink", name)
	{
		m_callbacks = std::make_shared<Callbacks>();
	}

	GStreamerAppSink (
		GstAppSink *appSink,
		bool addRef)
	{
		if (GST_IS_APP_SINK (appSink))  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast, modernize-use-auto)
		{
			set (GST_ELEMENT(appSink)); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

			if (addRef)
			{
				gst_object_ref (get ());
			}
		}

		m_callbacks = std::make_shared<Callbacks>();
	}

	GStreamerAppSink (const GStreamerAppSink &other) = default;

	GStreamerAppSink (GStreamerAppSink &&other)
	:
		GStreamerElement (other),
		m_callbacks (std::move(other.m_callbacks))
	{
	}

	GStreamerAppSink &operator = (const GStreamerAppSink &other) = delete;

	GStreamerAppSink &operator = (GStreamerAppSink &&other)
	{
		if (this != &other)
		{
			GStreamerElement::operator=(other);
			m_callbacks = std::move(other.m_callbacks);
		}

		return *this;
	}

	~GStreamerAppSink () override
	{
		clear ();
	}

	static GstFlowReturn newPrerollCallback (
		GstAppSink *appSink,
		gpointer userData)
	{
		auto self = static_cast<GStreamerAppSink *>(userData);

		if (self->m_callbacks && self->m_callbacks->m_newPreroll)
		{
			self->m_callbacks->m_newPreroll (*self);
		}

		return GST_FLOW_OK;
	}

	static GstFlowReturn newBufferCallback (
		GstAppSink *appSink,
		gpointer userData)
	{
		auto self = static_cast<GStreamerAppSink *>(userData);

		if (self->m_callbacks && self->m_callbacks->m_newBuffer)
		{
			self->m_callbacks->m_newBuffer (*self);
		}

		return GST_FLOW_OK;
	}

	void newPrerollCallbackSet (
		std::function<void (GStreamerAppSink appSink)> func)
	{
		m_callbacks->m_newPreroll = func;

		m_callbacks->m_gstCallbacks.new_preroll = newPrerollCallback;

		gst_app_sink_set_callbacks(get (), &m_callbacks->m_gstCallbacks, this, nullptr);
	}

	void newBufferCallbackSet (
		std::function<void (GStreamerAppSink appSink)> func)
	{
		m_callbacks->m_newBuffer = func;

#if GST_VERSION_MAJOR >= 1
		m_callbacks->m_gstCallbacks.new_sample = newBufferCallback;
#else
		m_callbacks->m_gstCallbacks.new_buffer = newBufferCallback;
#endif
		gst_app_sink_set_callbacks(get (), &m_callbacks->m_gstCallbacks, this, nullptr);
	}

	bool operator == (const GStreamerAppSink &other) const
	{
		return get () == other.get ();
	}

	GstAppSink *get () const
	{
		return GST_APP_SINK(GStreamerElement::get ()); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	}

	GstElement *elementGet () const
	{
		return GStreamerElement::get ();
	}

	GStreamerSample pullSample ()
	{
#if GST_VERSION_MAJOR >= 1
		return { gst_app_sink_pull_sample (get ()), false }; // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
#else
		return { gst_app_sink_pull_buffer (get ()), false }; // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
#endif
	}

	GStreamerSample pullPreroll ()
	{
		return { gst_app_sink_pull_preroll (get ()), false }; // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	}

	void clear () override
	{
		m_callbacks = nullptr;

		GStreamerElement::clear ();
	}

private:

	struct Callbacks
	{
		GstAppSinkCallbacks m_gstCallbacks {};
		std::function<void (GStreamerAppSink appSink)> m_newPreroll;
		std::function<void (GStreamerAppSink appSink)> m_newBuffer;
	};

	std::shared_ptr<Callbacks> m_callbacks;
};
