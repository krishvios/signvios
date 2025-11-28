#include "DisplaySinkBin.h"
#include "stiError.h"
#include "GStreamerPad.h"
#include "GStreamerElementFactory.h"

#undef QML_GL_SINK
#if APPLICATION == APP_NTOUCH_VP2
#define QML_GL_SINK "qmlglsink"
#else
#define QML_GL_SINK "qml6glsink"
#endif

DisplaySinkBin::DisplaySinkBin(
	void *qquickItem, bool mirrored)
:
	GStreamerBin("display_sink_bin")
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;
	GStreamerElement queue;
	GStreamerElement qmlSink;
	GStreamerElement displaySink;
	GStreamerPad queueSinkPad;

	GStreamerElementFactory qmlglsinkFactory (QML_GL_SINK);

	queue = GStreamerElement ("queue", "display_sink_queue");
	stiTESTCOND(queue.get (), stiRESULT_ERROR);

	elementAdd (queue);

	// Create Elements
	if (qmlglsinkFactory.get())
	{
		stiTESTCOND (qquickItem, stiRESULT_ERROR);

		qmlSink = GStreamerElement (QML_GL_SINK, "display_qmlglsink_element");
		stiTESTCOND(qmlSink.get (), stiRESULT_ERROR);

		qmlSink.propertySet ("widget", qquickItem);

		displaySink = GStreamerElement ("glsinkbin", "display_glsinkbin_element");
		stiTESTCOND(displaySink.get (), stiRESULT_ERROR);

		// Set attributes
		displaySink.propertySet ("sink", qmlSink);

		displaySink.propertySet ("force-aspect-ratio", TRUE);
		displaySink.propertySet ("sync", FALSE);
	}
	else
	{
		displaySink = GStreamerElement ("fakesink", "fake_sink_element");
		stiTESTCOND(displaySink.get (), stiRESULT_ERROR);
	}

	elementAdd (displaySink);

	result = queue.link(displaySink);
	stiTESTCOND (result, stiRESULT_ERROR);

	// Create a sink pad for the bin
	queueSinkPad = queue.staticPadGet ("sink");
	stiTESTCOND(queueSinkPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("sink", queueSinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear ();
	}
}
