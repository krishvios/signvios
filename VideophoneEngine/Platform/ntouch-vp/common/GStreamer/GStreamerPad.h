#pragma once

#include <gst/gst.h>
#include <gst/gstpad.h>
#include "GStreamerCaps.h"
#include "GStreamerEvent.h"
#include <functional>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>

class GStreamerBuffer;
class GStreamerPad;

#if GST_VERSION_MAJOR >= 1
using BlockCallbackFunction = std::function<void()>;
using ProbeCallbackFunction0 = std::function<GstPadProbeReturn()>;
using ProbeCallbackFunction = std::function<GstPadProbeReturn(GStreamerPad pad, GstPadProbeInfo *info)>;
#endif
using BufferProbeCallbackFunction0 = std::function<bool()>;
using BufferProbeCallbackFunction = std::function<bool(GStreamerPad pad, GStreamerBuffer gstBuffer)>;
using EventProbeCallbackFunction = std::function<bool(const GStreamerEvent &event)>;

class GStreamerPad
{
public:

	GStreamerPad () = default;

	GStreamerPad (
		GstPad *gstPad,
		bool addRef)
	:
		m_pad (gstPad)
	{
		if (addRef && m_pad)
		{
			gst_object_ref(m_pad);
		}
	}

	GStreamerPad (const GStreamerPad &other)
	{
		m_pad = other.m_pad;

		if (m_pad)
		{
			gst_object_ref(m_pad);
		}
	}

	GStreamerPad (GStreamerPad &&other)
	{
		// No need to ref or unref as ownership is being transferred
		m_pad = other.m_pad;
		other.m_pad = nullptr;
	}

	GStreamerPad &operator= (const GStreamerPad &other)
	{
		if (this != &other)
		{
			clear ();

			m_pad = other.m_pad;

			if (m_pad)
			{
				gst_object_ref(m_pad);
			}
		}

		return *this;
	}

	GStreamerPad &operator= (GStreamerPad &&other)
	{
		if (this != &other)
		{
			clear ();

			// No need to ref or unref as ownership is being transferred
			m_pad = other.m_pad;
			other.m_pad = nullptr;
		}

		return *this;
	}

	~GStreamerPad ()
	{
		clear ();
	}

	GStreamerCaps currentCapsGet () const
	{
#if GST_VERSION_MAJOR >= 1
		return { gst_pad_get_current_caps (m_pad), false };
#else
		return { gst_pad_get_negotiated_caps (m_pad), false };
#endif
	}

	gboolean padPushEvent(
		GstEvent *event )
	{
		return gst_pad_push_event(m_pad, event);
	}

	GstPadLinkReturn link (
		GStreamerPad &pad)
	{
		return gst_pad_link (m_pad, pad.get ());
	}

	gboolean unlink(
		GStreamerPad &pad)
	{
		if (gst_pad_get_direction(m_pad) == GST_PAD_SRC)
		{
			return gst_pad_unlink (m_pad, pad.get ());
		}

		return gst_pad_unlink (pad.get(), m_pad);
	}

	std::string nameGet () const
	{
		auto name = gst_pad_get_name (m_pad);

		if (name)
		{
			std::string result {name};
			g_free (name);

			return result;
		}

		return {};
	}

	template<class T>
	gulong addBufferProbe (
		const std::string &name,
		T callback)
	{
		auto probeId = addBufferProbe(callback);

		saveProbe(name, probeId);

		return probeId;
	}

	gulong addBufferProbe (
			BufferProbeCallbackFunction0 callback);

	gulong addBufferProbe (
			BufferProbeCallbackFunction callback);

	template<class T>
	gulong addEventProbe (
		const std::string &name,
		T callback)
	{
		auto probeId = addEventProbe(callback);

		saveProbe(name, probeId);

		return probeId;
	}

	gulong addEventProbe (
		EventProbeCallbackFunction callback);

	void removeProbe(
		const std::string &name);

#if GST_VERSION_MAJOR >= 1
	gulong addProbe (
		GstPadProbeType mask,
		ProbeCallbackFunction0 callback);

	gulong addProbe (
		GstPadProbeType mask,
		ProbeCallbackFunction callback);

	// Blocks the pad, executes the callback and then unblocks the pad
	void block (
		BlockCallbackFunction callback);

	void enableDebugging(GstPadProbeType type);
#endif

	Poco::DynamicStruct asJson () const;

	GstPad *get () const
	{
		return m_pad;
	}

	void clear ()
	{
		if (m_pad)
		{
			gst_object_unref(m_pad);
			m_pad = nullptr;
		}
	}

private:

	std::map<std::string, gulong> *getProbeMap() const;

	void saveProbe(
		const std::string &name,
		gulong probeId);

	GstPad *m_pad {nullptr};
};
