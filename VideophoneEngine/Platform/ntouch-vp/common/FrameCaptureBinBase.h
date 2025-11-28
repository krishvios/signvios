#pragma once

#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "CstiSignal.h"
#include "GStreamerAppSink.h"
#include "GStreamerBin.h"
#include "GStreamerAppSource.h"
#include "GStreamerElementFactory.h"
#include "GStreamerPad.h"
#include "IstiVideoInput.h"
#include "IstiVideoOutput.h"
#include "CstiEventQueue.h"
#include "CropScaleBinBase.h"

class FrameCaptureBinBase : public CstiEventQueue, public GStreamerBin
{
public:
	FrameCaptureBinBase();
	~FrameCaptureBinBase();

	FrameCaptureBinBase (const FrameCaptureBinBase &other) = delete;
	FrameCaptureBinBase (FrameCaptureBinBase &&other) = delete;
	FrameCaptureBinBase &operator= (const FrameCaptureBinBase &other) = delete;
	FrameCaptureBinBase &operator= (FrameCaptureBinBase &&other) = delete;

	stiHResult startup ();

	virtual void create ();

	stiHResult frameCaptureBinStart ();

	virtual void frameCapture (
		const GStreamerPad &gstPad,
		GStreamerBuffer &gstBuffer,
		bool crop,
		SstiImageCaptureSize imageSize);

	void displayModeSet (const EDisplayModes dispMode)
	{
		m_eDisplayMode = dispMode;
	}

	void displaySizeSet (const unsigned int columns,
						 const unsigned int rows)
	{
		m_columns = columns;
		m_rows = rows;
	}

	virtual int maxCaptureWidthGet () const = 0;
	virtual int maxCaptureHeightGet () const = 0;

	CstiSignal<GStreamerBuffer &> frameCaptureReceivedSignal;

protected:

	// Gstreamer elements
	GStreamerElementFactory m_msdkFactory;
	GStreamerAppSource  m_gstElementFrameCaptureAppSrc;
	GStreamerElement m_gstElementFrameCaptureJpegEnc;
	GStreamerAppSink m_gstElementFrameCaptureAppSink;

	gulong m_unFrameCaptureNewBufferSignalHandler {0};

	EDisplayModes m_eDisplayMode = eDISPLAY_MODE_UNKNOWN;
	unsigned int m_columns {0};
	unsigned int m_rows {0};

	static GstFlowReturn frameCaptureAppSinkGstBufferPut (
		GstElement* pGstElement,
		gpointer pUser);

	virtual CropScaleBinBase* cropScaleBinBaseGet () = 0;

	virtual GStreamerElement jpegEncoderGet ();

private:
	void eventFrameCaptureBinReset();

#if 0
	static GstPadProbeReturn debugProbeBufferFunction (
		GstPad * gstPad,
		GstPadProbeInfo *info,
		gpointer userData);

	static GstPadProbeReturn debugProbeEventFunction (
		GstPad * gstPad,
		GstPadProbeInfo *info,
		gpointer userData);
#endif

};
