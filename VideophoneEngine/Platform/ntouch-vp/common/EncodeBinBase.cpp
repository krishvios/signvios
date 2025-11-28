#include "EncodeBinBase.h"

#include <gst/video/video.h>
#include <vector>
#include <string>
#include <sstream>
#include "GStreamerCaps.h"
#include "GStreamerPad.h"
#include "GStreamerElementFactory.h"
#include "stiError.h"
#include "stiTools.h"

EncodeBinBase::EncodeBinBase()
:
	GStreamerBin ("encode_leg")
{
}

void EncodeBinBase::create (
	const CaptureBinBase2 &captureBin)
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;
	GstPadLinkReturn linkResult = GST_PAD_LINK_OK;
	GStreamerPad sinkPad;

	m_pCaptureBin = &captureBin;
	
	// Create Elements

	m_encoderPreScaleFilter = GStreamerElement ("capsfilter", "encode-pre-scale-filter");
	m_encoderPreScaleFilter.propertySet("caps", encodeLegInputCapsGet ());

	elementAdd(m_encoderPreScaleFilter);
	
	m_encoderPostScaleFilter = GStreamerElement ("capsfilter", "encode-post-scale-filter");
	m_encoderPostScaleFilter.propertySet("caps", inputCapsGet ());

	elementAdd(m_encoderPostScaleFilter);

	m_bitstreamFilter = GStreamerElement ("capsfilter", "bitstream-filter");
	m_bitstreamFilter.propertySet("caps", outputCapsGet ());

	elementAdd(m_bitstreamFilter);

	// Create the output selector.
	m_outputSelector = GStreamerElement ("output-selector", "output_Selector");
	stiTESTCOND(m_outputSelector.get (), stiRESULT_ERROR);

	m_outputSelector.propertySet ("pad-negotiation-mode", 0);

	// Get the selector src pads.
	m_outputPadAppSink = m_outputSelector.requestPadGet("src_%u");
	stiTESTCOND(m_outputPadAppSink.get(), stiRESULT_ERROR);

	m_outputPadRecordPipeline = m_outputSelector.requestPadGet("src_%u");
	stiTESTCOND(m_outputPadRecordPipeline.get(), stiRESULT_ERROR);

	m_appSink = GStreamerAppSink("encode_appsink_element");
	stiTESTCOND(m_appSink.get() != nullptr, stiRESULT_ERROR);

	m_appSink.propertySet ("sync", TRUE);
	m_appSink.propertySet ("async", FALSE);
	m_appSink.propertySet ("qos", FALSE);
	m_appSink.propertySet ("enable-last-sample", FALSE);

	// Add elements to the bin
	elementAdd (m_outputSelector);
	elementAdd (m_appSink);

	m_encoderScale = videoConvertElementGet();
	stiTESTCOND(m_encoderScale.get (), stiRESULT_ERROR);

	elementAdd (m_encoderScale);

	// Create a sink pad for the bin
	sinkPad = m_encoderPreScaleFilter.staticPadGet ("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	// Using a probe, drop buffers if not
	// encoding to minimize cpu utilization.
	sinkPad.addBufferProbe(
		[this](
			GStreamerPad gstPad,
			GStreamerBuffer gstBuffer)
		{
			return m_encoding;
		});

	result = ghostPadAdd ("sink", sinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	// Link
	result = m_encoderPreScaleFilter.link (m_encoderScale);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = m_encoderScale.link (m_encoderPostScaleFilter);
	stiTESTCOND(result, stiRESULT_ERROR);
	
	result = m_bitstreamFilter.link (m_outputSelector);
	stiTESTCOND(result, stiRESULT_ERROR);

	sinkPad = m_appSink.staticPadGet("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	linkResult = m_outputPadAppSink.link(sinkPad);
	stiTESTCOND(linkResult == GST_PAD_LINK_OK, stiRESULT_ERROR);

	// Add the file encoder pipeline.
	m_recordBin = recordBinCreate();
	stiTESTCOND (m_recordBin != nullptr, stiRESULT_ERROR);
	stiTESTCOND (m_recordBin->get (), stiRESULT_ERROR);

	elementAdd(*m_recordBin);
	sinkPad = m_recordBin->staticPadGet("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	linkResult = m_outputPadRecordPipeline.link(sinkPad);
	stiTESTCOND(linkResult == GST_PAD_LINK_OK, stiRESULT_ERROR);

	hResult = addEncoderElement ();
	stiTESTRESULT ();

	// Set the active output.
	m_outputSelector.propertySet ("active-pad", m_outputPadAppSink);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}

void EncodeBinBase::encodingStart ()
{
	if (m_fileRecord)
	{
		m_outputSelector.propertySet ("active-pad", m_outputPadRecordPipeline);

		m_recordBin->recordStart();
	}
	else
	{
		m_outputSelector.propertySet ("active-pad", m_outputPadAppSink);
	}

	keyframeEventSend ();

	m_encoding = true;
}

void EncodeBinBase::encodingStop()
{
	m_encoding = false;

	if (m_fileRecord)
	{
		m_outputSelector.propertySet ("active-pad", m_outputPadAppSink);

		m_recordBin->recordStop();

		// Clear the fileRecord settings.
		fileRecordSet(false);
	}

	// Clear the pipeline.
	auto convertElement = elementGetByName("encode_mfxvpp_convert_element");
	if (convertElement.get())
	{
		convertElement.eventSend(gst_event_new_eos());
	}
}


const char* EncodeBinBase::mediaTypeGet()
{
	switch (m_codec)
	{
		case estiVIDEO_H263:
			return "video/x-h263";

		default:
		case estiVIDEO_H264:
			return "video/x-h264";

		case estiVIDEO_H265:
			return "video/x-h265";
	}
}

void EncodeBinBase::codecSet (
	EstiVideoCodec codec,
	EstiVideoProfile profile,
	unsigned int constraints,
	unsigned int level,
	EstiPacketizationScheme packetizationMode)
{
	if (m_codec != codec ||
		m_profile != profile ||
		m_constraints != constraints ||
		m_level != level ||
		m_packetizationMode != packetizationMode)
	{
		m_codec = codec;
		m_profile = profile;
		m_constraints = constraints;
		m_level = level;
		m_packetizationMode = packetizationMode;

		if (m_encoderElement.get ())
		{
			m_encoderPostScaleFilter.staticPadGet("src").block (
				[this]()
				{
					elementRemove (m_encoderElement);
					m_encoderElement.stateSet(GST_STATE_NULL);

					if (m_bitstreamFilter.get ())
					{
						m_bitstreamFilter.propertySet("caps", outputCapsGet ());
					}

					addEncoderElement();

					gst_element_sync_state_with_parent (m_encoderElement.get ());
				});
		}
	}
}


stiHResult EncodeBinBase::addEncoderElement ()
{
	stiHResult hResult {stiRESULT_SUCCESS};
	auto result = false;

	m_encoderElement = encoderGet ();
	stiTESTCOND (m_encoderElement.get(), stiRESULT_ERROR);

	m_encoderElement.propertySet ("bitrate", m_bitrateKbps);

	elementAdd (m_encoderElement);

	result = m_encoderPostScaleFilter.link (m_encoderElement);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = m_encoderElement.link (m_bitstreamFilter);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


void EncodeBinBase::bitrateSet (
	unsigned kbps)
{
	if (m_bitrateKbps != kbps)
	{
		m_bitrateKbps = kbps;
		m_encoderElement.propertySet("bitrate", m_bitrateKbps);
	}
}

void EncodeBinBase::encoderFilterUpdate ()
{
	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
		vpe::trace ("EncodeBinBase::encoderFilterUpdate: encode width = ", m_width, ", encode height = ", m_height, "\n");
	);
	
	if (m_encoderPreScaleFilter.get ())
	{
		m_encoderPreScaleFilter.propertySet("caps", encodeLegInputCapsGet ());
	}
	
	if (m_encoderPostScaleFilter.get ())
	{
		m_encoderPostScaleFilter.propertySet("caps", inputCapsGet ());
	}
}

void EncodeBinBase::sizeSet (
	unsigned width,
	unsigned height)
{
	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
		vpe::trace ("EncodeBinBase::sizeSet: encode width = ", width, ", encode height = ", height, "\n");
	);
	
	if (m_width != width || m_height != height)
	{
		m_width = width;
		m_height = height;
		encoderFilterUpdate ();
	}
}

void EncodeBinBase::framerateSet (
	unsigned fps)
{
	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
		vpe::trace ("EncodeBinBase::framerateSet: m_framerate = ", m_framerate, "\n");
	);
	
	if (m_framerate != fps)
	{
		m_framerate = fps;
		encoderFilterUpdate ();
	}
}

void EncodeBinBase::newBufferCallbackSet(
	std::function<void(GStreamerAppSink appSink)> func)
{
	m_appSink.newBufferCallbackSet (func);
}


std::unique_ptr<RecordBinBase> EncodeBinBase::recordBinCreate ()
{
	auto pipeline = sci::make_unique<RecordBinBase>();

	pipeline->create ();

	return pipeline;
}


void EncodeBinBase::recordBinInitialize(
	bool recordFastStartFile)
{
	m_recordBin->initialize(recordFastStartFile);
}
