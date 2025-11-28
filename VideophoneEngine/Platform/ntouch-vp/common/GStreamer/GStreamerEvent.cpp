#include "GStreamerEvent.h"

Poco::DynamicStruct GStreamerEvent::asJson () const
{
	Poco::DynamicStruct e;

#if GST_VERSION_MAJOR >= 1
	auto type = gst_event_type_get_name(GST_EVENT_TYPE(m_event));

	if (type)
	{
		e["type"] = std::string(type);
	}
	else
	{
		e["type"] = "unknown";
	}

	switch (GST_EVENT_TYPE(m_event))
	{
		case GST_EVENT_QOS:
		{
			GstQOSType type;
			gdouble proportion;
			GstClockTimeDiff diff;
			GstClockTime timestamp;

			gst_event_parse_qos(m_event, &type, &proportion, &diff, &timestamp);

			std::string typeName;
			switch (type)
			{
				case 0:
					typeName = "Underflow";
					break;
				case 1:
					typeName = "Overflow";
					break;
				case 2:
					typeName = "Throttle";
					break;
			}

			e["qosType"] = typeName;
			e["proportion"] = proportion;
			e["diff"] = diff;
			//e["timestamp"] = timestamp;

			break;
		}

		case GST_EVENT_LATENCY:
		{
			GstClockTime latency;

			gst_event_parse_latency(m_event, &latency);

			e["latency"] = latency;

			break;
		}

		default:
			break;
	}
#endif

	return e;
}
