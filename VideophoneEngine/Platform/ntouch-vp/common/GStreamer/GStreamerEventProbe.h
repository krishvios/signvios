#pragma once

#include "GStreamerSample.h"
#include "GStreamerPad.h"
#include "stiTools.h"

#include <gst/gstpad.h>


class GStreamerEventProbe
{
public:

	GStreamerEventProbe () = default;

	template<typename F, typename... Args>
	GStreamerEventProbe (
		GStreamerPad &gstPad,
		F &&func,
		Args... args)
	:
		m_gstPad (gstPad)
	{
		if (m_gstPad.get () != nullptr)
		{
			m_callback = std::bind (func, args..., m_gstPad, std::placeholders::_1);

			m_probe = sci::make_unique<GStreamerEventProbe *>(this);

#if GST_VERSION_MAJOR < 1
			m_probeId = gst_pad_add_event_probe (m_gstPad.get (), (GCallback)eventProbeCallback, m_probe.get ());
#else
			m_probeId = gst_pad_add_probe (m_gstPad.get (),
												 static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
												 eventProbeCallback, m_probe.get (), nullptr);
#endif
		}
	}

	GStreamerEventProbe (const GStreamerEventProbe &other) = delete;

	GStreamerEventProbe (GStreamerEventProbe &&other)
	:
		m_gstPad (std::move(other.m_gstPad))
	{
		m_callback = std::move(other.m_callback);

		m_probe = std::move(other.m_probe);
		*m_probe = this;

		m_probeId = other.m_probeId;
		other.m_probeId = 0;
	}

	GStreamerEventProbe &operator= (const GStreamerEventProbe &other) = delete;

	GStreamerEventProbe &operator= (GStreamerEventProbe &&other)
	{
		if (this != &other)
		{
			// If the gaining object already has a probe and/or pad then
			// we need to remove probe and release the pad.
			if (m_probeId != 0)
			{
	#if GST_VERSION_MAJOR < 1
				gst_pad_remove_buffer_probe (m_gstPad.get (), m_probeId);
	#else
				gst_pad_remove_probe (m_gstPad.get (), m_probeId);
	#endif
				m_probeId = 0;
			}

			m_callback = std::move(other.m_callback);
			m_probe = std::move(other.m_probe);
			*m_probe = this;

			// No need to add or remove a reference here
			// since we are just moving the data from one
			// object to another (net gain of zero refs).
			m_gstPad = std::move(other.m_gstPad);

			m_probeId = other.m_probeId;
			other.m_probeId = 0;
		}

		return *this;
	}

	~GStreamerEventProbe ()
	{
		if (m_probeId != 0)
		{
#if GST_VERSION_MAJOR < 1
			gst_pad_remove_buffer_probe (m_gstPad.get (), m_probeId);
#else
			gst_pad_remove_probe (m_gstPad.get (), m_probeId);
#endif
			m_probeId = 0;
		}
	}

private:

#if GST_VERSION_MAJOR < 1
	static gboolean eventProbeCallback (
		GstPad *gstPad,
		GstMiniObject *gstMiniObject,
		gpointer userData)
	{
		GStreamerSample gstSample(gstMiniObject);
		auto gstBuffer = gstSample.bufferGet ();

		callbackProcess (userData, gstBuffer);

		return true;
	}
#else
	static GstPadProbeReturn eventProbeCallback (
		GstPad *pad,
		GstPadProbeInfo *info,
		gpointer userData)
	{
		auto *event = gst_pad_probe_info_get_event (info);

		if (event)
		{
			callbackProcess (userData, event);
		}

		return GST_PAD_PROBE_OK;
	}
#endif

	static void callbackProcess (
		gpointer userData,
		GstEvent *gstEvent)
	{
		if (gstEvent != nullptr)
		{
			auto self = *static_cast<GStreamerEventProbe **>(userData);

			if (self && self->m_probeId != 0 && self->m_callback != nullptr)
			{
				self->m_callback (gstEvent);
			}
		}
	}

	std::unique_ptr<GStreamerEventProbe *> m_probe;
	GStreamerPad m_gstPad;
	gulong m_probeId {0};
	std::function<void(GstEvent *)> m_callback {nullptr};
};
