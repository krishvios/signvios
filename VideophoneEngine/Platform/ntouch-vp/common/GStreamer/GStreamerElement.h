#pragma once

#include "GStreamerPad.h"
#include "GStreamerEncodingVideoProfile.h"
#include "GStreamerProperty.h"
#include "stiTools.h"
#include <gst/gst.h>
#if GST_VERSION_MAJOR >= 1
#include <gst/video/video.h>
#endif
#include <memory>
#include <functional>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>

class GStreamerElement
{
	std::function<void (GstElement *)> GstElementDeleter =
		[](GstElement *p)
		{
			gst_object_unref (p);
		};

public:

	using GstElementType = std::shared_ptr<GstElement>;

	GStreamerElement () = default;

	GStreamerElement (
		const std::string &factoryName)
	{
		auto element = gst_element_factory_make(factoryName.c_str (), nullptr);
		gst_object_ref_sink (element);

		set (element);
	}

	GStreamerElement (
		const std::string &factoryName,
		const std::string &name)
	{
		auto element = gst_element_factory_make(factoryName.c_str (), name.c_str ());
		gst_object_ref_sink (element);

		set (element);
	}

	GStreamerElement (
		GstElement *gstElement,
		bool addRef)
	{
		set(gstElement);

		if (addRef && m_element)
		{
			gst_object_ref (m_element.get ());
		}
	}
	
	GStreamerElement (
		GstElementType gstElement)
	:
		m_element(gstElement)
	{
	}

	GStreamerElement (
		GstElementFactory *gstElementFactory)
	{
		auto element = gst_element_factory_create(gstElementFactory, nullptr);
		gst_object_ref_sink (element);

		set (element);
	}

	GStreamerElement (
		GstElementFactory *gstElementFactory,
		const std::string &name)
	{
		auto element = gst_element_factory_create(gstElementFactory, name.c_str ());
		gst_object_ref_sink (element);

		set (element);
	}

	GStreamerElement (
		GstObject *gstObject)
	:
		m_element(GstElementType(GST_ELEMENT_CAST(gstObject), GstElementDeleter))
	{
	}

	GStreamerElement (const GStreamerElement &other) = default;
	GStreamerElement (GStreamerElement &&other) = default;
	GStreamerElement &operator= (const GStreamerElement &other) = default;
	GStreamerElement &operator= (GStreamerElement &&other) = default;

	virtual ~GStreamerElement () = default;

	template<typename T>
	void propertySet (
		const std::string &property,
		T value)
	{
		g_object_set (m_element.get (), property.c_str (), value, NULL);
	}

	void propertySet (
		const std::string &property,
		const GStreamerCaps &caps)
	{
		g_object_set (m_element.get (), property.c_str (), caps.get (), NULL);
	}

	void propertySet (
		const std::string &property,
		GStreamerElement &element)
	{
		g_object_set (m_element.get (), property.c_str (), element.get (), NULL);
	}

	void propertySet (
		const std::string &property,
		GStreamerPad &pad)
	{
		g_object_set (m_element.get (), property.c_str (), pad.get (), NULL);
	}

	void propertySet (
		const std::string &property,
		GStreamerEncodingVideoProfile &profile)
	{
		g_object_set (m_element.get (), property.c_str (), profile.get (), NULL);
	}

	void propertySet (
		const std::string &property,
		const std::string &value)
	{
		g_object_set (m_element.get (), property.c_str (), value.c_str (), NULL);
	}

	GStreamerPad staticPadGet (
		const std::string &name)
	{
		return { gst_element_get_static_pad (m_element.get (), name.c_str ()), false };
	}

	GStreamerPad requestPadGet (
		const std::string &name)
	{
		auto padTemplate = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (m_element.get ()), name.c_str ());

		if (padTemplate != nullptr)
		{
			return { gst_element_request_pad (m_element.get (), padTemplate, nullptr, nullptr), false };
		}

		return {};
	}

	gboolean ghostPadAdd (
		const std::string &name,
		GStreamerPad &pad)
	{
		return gst_element_add_pad(m_element.get (), gst_ghost_pad_new(name.c_str (), pad.get()));
	}

	gboolean ghostPadAddActive (
		const std::string &name,
		GStreamerPad &pad)
	{
		auto activePad = gst_ghost_pad_new(name.c_str (), pad.get());
		gst_pad_set_active(activePad, true);
		return gst_element_add_pad(m_element.get (), activePad);
	}

	bool link (
		GStreamerElement &element)
	{
		return gst_element_link (m_element.get (), element.get ());
	}

	bool linkPads (
		const std::string &padName,
		GStreamerElement &other,
		const std::string &otherPadName)
	{
		return gst_element_link_pads (m_element.get (), padName.c_str (), other.get (), otherPadName.c_str ());
	}

	void unlink (
		GStreamerElement &element)
	{
		gst_element_unlink(m_element.get(), element.get());
	}

	virtual GstStateChangeReturn stateSet (GstState state)
	{
		return gst_element_set_state (m_element.get (), state);
	}

	GstStateChangeReturn stateGet (GstState *state, GstState *pending, GstClockTime timeout)
	{
		return gst_element_get_state (m_element.get (), state, pending, timeout);
	}

	bool syncStateWithParent()
	{
		return gst_element_sync_state_with_parent(m_element.get ());
	}

	gboolean eventSend (
		GstEvent *event)
	{
		return gst_element_send_event (m_element.get (), event);
	}

	virtual gboolean keyframeEventSend ()
	{
		auto structure = gst_structure_new ("GstForceKeyUnit", "all-headers", G_TYPE_BOOLEAN, TRUE, NULL);

		if (structure)
		{
			auto event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, structure);

			if (event)
			{
				return eventSend (event);
			}
		}

		return false;
	}

	std::string nameGet () const
	{
		auto name = gst_element_get_name (m_element.get ());

		if (name)
		{
			std::string result {name};
			g_free (name);

			return result;
		}

		return {};
	}

	bool foreachPad (std::function<bool(const GStreamerPad &)> callback) const;

	bool foreachProperty (
		std::function<bool(const GParamSpec &param, const GValue &propertyValue)> callback,
		bool nonDefaultOnly) const;

	std::string propertyInfoGet (
		const std::string &name) const;

	void propertiesSet(
		const std::vector<std::pair<std::string, std::string>> &properties);

	virtual Poco::DynamicStruct asJson () const;

	GstElement *get () const
	{
		return m_element.get ();
	}

	virtual void clear ()
	{
		m_element = nullptr;
	}

	virtual void discard ()
	{
		// Ref the object so that when the shared pointer is freed
		// it doesn't cause the object to be deleted.
		gst_object_ref (m_element.get ());
		m_element = nullptr;
	}

	GstClock *clockGet()
	{
		return gst_element_get_clock(m_element.get());
	}

protected:

	void set (GstElement *element)
	{
		m_element = GstElementType (element, GstElementDeleter);
	}

	std::weak_ptr<GstElement> getWeakPtr ()
	{
		return m_element;
	}

private:

	GstElementType m_element;
};
