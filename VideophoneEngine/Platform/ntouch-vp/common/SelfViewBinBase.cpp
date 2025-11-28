#include "SelfViewBinBase.h"

#include "stiError.h"
#include "GStreamerPad.h"
#include "GStreamerElementFactory.h"
#include "stiTrace.h"

#undef QML_GL_SINK
#if APPLICATION == APP_NTOUCH_VP2
#define QML_GL_SINK "qmlglsink"
#else
#define QML_GL_SINK "qml6glsink"
#endif

SelfViewBinBase::SelfViewBinBase()
:
	GStreamerBin("selfview_bin")
{

}

void SelfViewBinBase::create(
	void *qquickItem)
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;
	GStreamerElement qmlSink;
	GStreamerElement displaySink;
	GStreamerElement qmlHackConvert;
	GStreamerElement qmlHackFilter;
	GStreamerPad svSinkPad;
	GStreamerCaps gstCaps;

	GStreamerElementFactory qmlglsinkFactory (QML_GL_SINK);
	GStreamerElementFactory imxConvertFactory ("imxvideoconvert_g2d");

	if (imxConvertFactory.get ())
	{
		// TODO: Fix this ASAP. This convert only exists to work around a bug in qml6sink.
		// Without it the self view stream hangs after the first few frames. See #VP-1362.
		qmlHackConvert = imxConvertFactory.createElement("qml-hack-convert");
		stiTESTCOND(qmlHackConvert.get (), stiRESULT_ERROR);

		elementAdd (qmlHackConvert);

		qmlHackFilter = GStreamerElement ("capsfilter", "qml-hack-filter");
		stiTESTCOND(qmlHackFilter.get (), stiRESULT_ERROR);

		gstCaps = GStreamerCaps ("video/x-raw,format=RGBx");
		stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);

		qmlHackFilter.propertySet ("caps", gstCaps);

		elementAdd (qmlHackFilter);
	}

	if (qmlglsinkFactory.get())
	{
		stiTESTCOND (qquickItem, stiRESULT_ERROR);

		qmlSink = GStreamerElement (QML_GL_SINK, "qmlglsink_element");
		stiTESTCOND(qmlSink.get (), stiRESULT_ERROR);

		displaySink = GStreamerElement ("glsinkbin", "glsinkbin_element");
		stiTESTCOND(displaySink.get (), stiRESULT_ERROR);

		// Set attributes
		qmlSink.propertySet ("widget", qquickItem);
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

	if (imxConvertFactory.get ())
	{
		result = qmlHackConvert.link(qmlHackFilter);
		stiTESTCOND (result, stiRESULT_ERROR);

		result = qmlHackFilter.link (displaySink);
		stiTESTCOND(result, stiRESULT_ERROR);

		svSinkPad = qmlHackConvert.staticPadGet ("sink");
		stiTESTCOND(svSinkPad.get(), stiRESULT_ERROR);
	}
	else
	{
		svSinkPad = displaySink.staticPadGet ("sink");
		stiTESTCOND(svSinkPad.get(), stiRESULT_ERROR);
	}

	result = ghostPadAdd ("sink", svSinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear ();
	}
}
