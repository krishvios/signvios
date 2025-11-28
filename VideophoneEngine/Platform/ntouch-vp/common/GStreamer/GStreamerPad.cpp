#include "stiSVX.h"
#include "GStreamerPad.h"
#include "GStreamerElement.h"
#include "GStreamerSample.h"
#include "GStreamerBuffer.h"
#include "GStreamerEvent.h"
#include "stiTrace.h"
#include <map>
#include <string>

#if GST_VERSION_MAJOR >= 1

static GstPadProbeReturn blockCallback (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	auto callback = static_cast<BlockCallbackFunction *>(userData);

	if (callback)
	{
		(*callback) ();
	}

	return GST_PAD_PROBE_REMOVE;
}


static GstPadProbeReturn probeCallback0 (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	GstPadProbeReturn result {GST_PAD_PROBE_OK};
	auto callback = static_cast<ProbeCallbackFunction0 *>(userData);

	if (callback)
	{
		result = (*callback) ();
	}

	return result;
}


static GstPadProbeReturn probeCallback (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	GstPadProbeReturn result {GST_PAD_PROBE_OK};
	auto callback = static_cast<ProbeCallbackFunction *>(userData);

	if (callback)
	{
		result = (*callback) (GStreamerPad(pad, true), info);
	}

	return result;
}


static void blockDestroyedCallback (
	gpointer userData)
{
	auto callback = static_cast<BlockCallbackFunction *>(userData);

	delete callback;
}


static void probeDestroyedCallback0 (
	gpointer userData)
{
	auto callback = static_cast<ProbeCallbackFunction0 *>(userData);

	delete callback;
}


static void probeDestroyedCallback (
	gpointer userData)
{
	auto callback = static_cast<ProbeCallbackFunction *>(userData);

	delete callback;
}
#endif


#if GST_VERSION_MAJOR >= 1
static GstPadProbeReturn bufferProbeCallback0 (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	bool result {true};
	auto callback = static_cast<BufferProbeCallbackFunction0 *>(userData);

	if (callback)
	{
		result = (*callback) ();
	}

	if (result)
	{
		return GST_PAD_PROBE_OK;
	}

	return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn bufferProbeCallback (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	bool result {true};
	auto callback = static_cast<BufferProbeCallbackFunction *>(userData);

	if (callback)
	{
		GStreamerBuffer gstBuffer(info);

		result = (*callback) (GStreamerPad(pad, true), gstBuffer);
	}

	if (result)
	{
		return GST_PAD_PROBE_OK;
	}

	return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn eventProbeCallback (
	GstPad *pad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	bool result {true};
	auto callback = static_cast<EventProbeCallbackFunction *>(userData);

	if (callback)
	{
		auto event = gst_pad_probe_info_get_event(info);
		result = (*callback) (GStreamerEvent(event, true));
	}

	if (result)
	{
		return GST_PAD_PROBE_OK;
	}

	return GST_PAD_PROBE_DROP;
}
#else
static bool bufferProbeCallback0 (
	GstPad *pad,
	GstMiniObject *gstMiniObject,
	gpointer userData)
{
	bool result {true};
	auto callback = static_cast<BufferProbeCallbackFunction0 *>(userData);

	if (callback)
	{
		GStreamerSample gstSample(gstMiniObject);
		auto gstBuffer = gstSample.bufferGet ();

		result = (*callback) ();
	}

	return result;
}

static bool bufferProbeCallback (
	GstPad *pad,
	GstMiniObject *gstMiniObject,
	gpointer userData)
{
	bool result {true};
	auto callback = static_cast<BufferProbeCallbackFunction *>(userData);

	if (callback)
	{
		GStreamerSample gstSample(gstMiniObject);
		auto gstBuffer = gstSample.bufferGet ();

		result = (*callback) (GStreamerPad(pad, true), gstBuffer);
	}

	return result;
}
#endif

template<class T>
static void probeDestroyedCallback (
	gpointer userData)
{
	auto callback = static_cast<T *>(userData);

	delete callback;
}

void destroyNotify(gpointer data)
{
	auto map = static_cast<std::map<std::string, gulong> *>(data);
	delete map;
}

std::map<std::string, gulong> *GStreamerPad::getProbeMap() const
{
	auto gobject = G_OBJECT(get());

	auto data = g_object_get_data(gobject, "probes");

	return static_cast<std::map<std::string, gulong> *>(data);
}

void GStreamerPad::saveProbe(
	const std::string &name,
	gulong probeId)
{
	auto map = getProbeMap();

	if (!map)
	{
		map = new std::map<std::string, gulong>;
		g_object_set_data_full( G_OBJECT(get()), "probes", map, destroyNotify);
	}

	(*map)[name] = probeId;
}

gulong GStreamerPad::addBufferProbe (
	BufferProbeCallbackFunction0 callback)
{
#if GST_VERSION_MAJOR < 1
	return gst_pad_add_buffer_probe_full (
		get (),
		(GCallback)bufferProbeCallback0,
		reinterpret_cast<gpointer>(new BufferProbeCallbackFunction0(callback)),
		probeDestroyedCallback<BufferProbeCallbackFunction0>);
#else
	return gst_pad_add_probe (
			get (),
			static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH),
			bufferProbeCallback0,
			reinterpret_cast<gpointer>(new BufferProbeCallbackFunction0(callback)),
			probeDestroyedCallback<BufferProbeCallbackFunction0>);
#endif
}

gulong GStreamerPad::addEventProbe (
	EventProbeCallbackFunction callback)
{
#if GST_VERSION_MAJOR >= 1
	return gst_pad_add_probe (
			get (),
			static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_EVENT_BOTH),
			eventProbeCallback,
			reinterpret_cast<gpointer>(new EventProbeCallbackFunction(callback)),
			probeDestroyedCallback<EventProbeCallbackFunction>);
#else
	return 0;
#endif
}


void GStreamerPad::removeProbe(
	const std::string &name)
{
#if GST_VERSION_MAJOR >= 1
	auto map = getProbeMap();

	if (map)
	{
		auto iter = (*map).find(name);

		if (iter != (*map).end())
		{
			gst_pad_remove_probe(get(), iter->second);

			(*map).erase(iter);
		}
	}
#endif
}

gulong GStreamerPad::addBufferProbe (
	BufferProbeCallbackFunction callback)
{
#if GST_VERSION_MAJOR < 1
	return gst_pad_add_buffer_probe_full (
		get (),
		(GCallback)bufferProbeCallback,
		reinterpret_cast<gpointer>(new BufferProbeCallbackFunction(callback)),
		probeDestroyedCallback<BufferProbeCallbackFunction>);
#else
	return gst_pad_add_probe (
			get (),
			static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH),
			bufferProbeCallback,
			reinterpret_cast<gpointer>(new BufferProbeCallbackFunction(callback)),
			probeDestroyedCallback<BufferProbeCallbackFunction>);
#endif
}

#if GST_VERSION_MAJOR >= 1
gulong GStreamerPad::addProbe (
	GstPadProbeType mask,
	ProbeCallbackFunction0 callback)
{
	return gst_pad_add_probe (get (), mask,
			probeCallback0,
			reinterpret_cast<gpointer>(new ProbeCallbackFunction0(callback)),
			probeDestroyedCallback0);
}

gulong GStreamerPad::addProbe (
	GstPadProbeType mask,
	ProbeCallbackFunction callback)
{
	return gst_pad_add_probe (get (), mask,
			probeCallback,
			reinterpret_cast<gpointer>(new ProbeCallbackFunction(callback)),
			probeDestroyedCallback);
}

// Blocks the pad, executes the callback and then unblocks the pad
void GStreamerPad::block (
	BlockCallbackFunction callback)
{
	gst_pad_add_probe (get (), GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
			blockCallback,
			reinterpret_cast<gpointer>(new BlockCallbackFunction(callback)),
			blockDestroyedCallback);
}

std::string outputEvent(GstEvent *event)
{
	std::stringstream message;

	message << GST_EVENT_TYPE_NAME(event);

	switch (GST_EVENT_TYPE(event))
	{
		case GST_EVENT_QOS:
		{
			GstQOSType type;
			gdouble proportion;
			GstClockTimeDiff diff;
			GstClockTime timestamp;

			gst_event_parse_qos(event, &type, &proportion, &diff, &timestamp);

			switch (type)
			{
				case GST_QOS_TYPE_OVERFLOW:
					message << ": overflow";
					break;
				case GST_QOS_TYPE_UNDERFLOW:
					message << ": underflow";
					break;
				case GST_QOS_TYPE_THROTTLE:
					message << ": throttle";
					break;
			}
			break;
		}

		case GST_EVENT_LATENCY:
		{
			GstClockTime latency;
			gst_event_parse_latency(event, &latency);

			message << ": " << latency;

			break;
		}

		default:
			break;
	}

	return message.str();
}


// Enables debug output on a pad
//
// Supports the following probe types:
//
//  GST_PAD_PROBE_TYPE_BUFFER
//  GST_PAD_PROBE_TYPE_BUFFER_LIST
//
//  GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM
//  GST_PAD_PROBE_TYPE_EVENT_UPSTREAM
//  GST_PAD_PROBE_TYPE_EVENT_BOTH
//
//  GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM
//  GST_PAD_PROBE_TYPE_QUERY_UPSTREAM
//  GST_PAD_PROBE_TYPE_QUERY_BOTH
//
//  GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM
//  GST_PAD_PROBE_TYPE_DATA_UPSTREAM
//  GST_PAD_PROBE_TYPE_DATA_BOTH
//
//  GST_PAD_PROBE_TYPE_ALL_BOTH
//
void GStreamerPad::enableDebugging(GstPadProbeType type)
{
	auto callback = [](GStreamerPad pad, GstPadProbeInfo *info)
		{
			GStreamerElement parent(gst_pad_get_parent(pad.get()));
			std::stringstream message;

			message << parent.nameGet() << ": " << GST_PAD_NAME(pad.get());

			auto type = GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_ALL_BOTH;

			switch (type)
			{
				case GST_PAD_PROBE_TYPE_BUFFER:
					message << ": buffer";
					break;

				case GST_PAD_PROBE_TYPE_BUFFER_LIST:
					message << ": buffer list";
					break;

				case GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM:
					message << ": downstream event: " <<  outputEvent(GST_EVENT(GST_PAD_PROBE_INFO_DATA (info)));
					break;

				case GST_PAD_PROBE_TYPE_EVENT_UPSTREAM:
					message << ": upstream event: " <<  outputEvent(GST_EVENT(GST_PAD_PROBE_INFO_DATA (info)));
					break;

				case GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM:
					message << ": downstream query: " <<  GST_QUERY_TYPE_NAME(GST_PAD_PROBE_INFO_DATA (info));
					break;

				case GST_PAD_PROBE_TYPE_QUERY_UPSTREAM:
					message << ": upstream query: " <<  GST_QUERY_TYPE_NAME(GST_PAD_PROBE_INFO_DATA (info));
					break;

				default:
					message << ": unknown type: " << type;
			}

			message << ", flow: " << gst_flow_get_name(gst_pad_get_last_flow_return(pad.get()));

			vpe::trace(message.str(), "\n");

			return GST_PAD_PROBE_OK;
		};

	if ((type & ~GST_PAD_PROBE_TYPE_ALL_BOTH) == 0)
	{
		gst_pad_add_probe(
			get(),
			type,
			probeCallback,
			reinterpret_cast<gpointer>(new ProbeCallbackFunction(callback)),
			probeDestroyedCallback);
	}
	else
	{
		stiASSERTMSG(false, "Probe type not supported: %d", type);
	}
}

#endif

#if GST_VERSION_MAJOR >= 1
using capsFilterAndMapInplaceLambda = std::function<bool (GstCapsFeatures *, GstStructure *)>;
gboolean capsFilterAndMapInplaceCallback (GstCapsFeatures *features, GstStructure *structure, gpointer userData)
{
	auto callback = static_cast<capsFilterAndMapInplaceLambda *>(userData);

	if (callback)
	{
		(*callback)(features, structure);
	}

	return true;
}

void capsFilterAndMapInPlace(GstCaps *caps, capsFilterAndMapInplaceLambda foreach)
{
	gst_caps_foreach(caps, capsFilterAndMapInplaceCallback,
		&foreach);
}


using structureForEachLambda = std::function<bool (GQuark, const GValue *)>;
gboolean structureForeachCallback(GQuark fieldId, const GValue *value, gpointer userData)
{
	auto callback = static_cast<structureForEachLambda *>(userData);

	if (callback)
	{
		(*callback)(fieldId, value);
	}

	return true;
}

void structureForEach(GstStructure *structure, structureForEachLambda foreach)
{
	gst_structure_foreach(structure, structureForeachCallback,
		&foreach);
}
#endif

Poco::DynamicStruct GStreamerPad::asJson () const
{
	Poco::DynamicStruct e;

#if GST_VERSION_MAJOR >= 1
	e["class"] = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(get ()));
	e["name"] = nameGet ();

	auto path  = gst_object_get_path_string(GST_OBJECT(get()));

	if (path)
	{
		e["path"] = std::string(path);

		g_free(path);
	}

	auto direction = gst_pad_get_direction (get ());

	switch (direction)
	{
		case GST_PAD_UNKNOWN:

			e["direction"] = "unknown";

			break;

		case GST_PAD_SRC:

			e["direction"] = "src";

			break;

		case GST_PAD_SINK:

			e["direction"] = "sink";

			break;
	}

	auto peer = gst_pad_get_peer (get ());

	if (peer)
	{
		auto peerPath  = gst_object_get_path_string(GST_OBJECT(peer));

		if (peerPath)
		{
			e["peerPath"] = std::string(peerPath);

			g_free(peerPath);
		}

		gst_object_unref(peer);
	}

	e["ghost"] = static_cast<bool>(GST_IS_GHOST_PAD(get()));

	if (e["ghost"])
	{
		auto target = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(get()));

		if (target)
		{
			auto targetPath = gst_object_get_path_string(GST_OBJECT(target));

			if (targetPath)
			{
				e["targetPath"] = std::string(targetPath);

				g_free(targetPath);
			}

			gst_object_unref(target);
		}
	}

	auto map = getProbeMap();

	if (map)
	{
		std::vector<std::string> probes;

		for (const auto &entry: *map)
		{
			probes.push_back(entry.first);
		}

		e["probes"] = probes;
	}

	auto caps = gst_pad_get_current_caps(get());

	if (caps)
	{
		Poco::DynamicStruct currentCaps;

		capsFilterAndMapInPlace(caps,
			[&currentCaps](GstCapsFeatures *features, GstStructure *structure)
			{
				auto name = gst_structure_get_name(structure);
				currentCaps["name"] = std::string(name);

				if (features)
				{
					std::vector<std::string> f;
					auto count = gst_caps_features_get_size(features);

					for (guint i = 0; i < count; i++)
					{
						f.push_back(gst_caps_features_get_nth(features, i));
					}

					currentCaps["features"] = f;
				}

				std::vector<Poco::DynamicStruct> values;

				structureForEach(structure,
					[&values](GQuark fieldId, const GValue *value)
					{
						auto name = g_quark_to_string(fieldId);
						//auto type = g_quark_to_string(g_type_qname(gst_structure_get_field_type (structure, name)));
						auto type = gst_value_serialize(value);

						Poco::DynamicStruct v;

						v["name"] = std::string(name);
						v["value"] = std::string(type);

						values.push_back(v);

						return true;
					});

				currentCaps["values"] = values;

				return true;
			});

		e["currentCaps"] = currentCaps;
	}
#endif

	return e;
}

