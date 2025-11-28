#pragma once

#include <gst/gst.h>
#include <Poco/JSON/Parser.h>

class GStreamerEvent
{
public:

	GStreamerEvent () = default;

	GStreamerEvent (
		GstEvent *event,
		bool addRef)
	:
		m_event(event)
	{
		if (addRef)
		{
			gst_event_ref(m_event);
		}
	}

	GStreamerEvent (const GStreamerEvent &other)
	:
		m_event(other.m_event)
	{
		gst_event_ref(m_event);
	}

	GStreamerEvent (GStreamerEvent &&other) = delete;
	GStreamerEvent &operator= (const GStreamerEvent &other) = delete;
	GStreamerEvent &operator= (GStreamerEvent &&other) = delete;

	~GStreamerEvent ()
	{
		gst_event_unref(m_event);
	}

	Poco::DynamicStruct asJson () const;

private:

	GstEvent *m_event {nullptr};
};
