#include "EncodePipelineBase.h"


EncodePipelineBase::EncodePipelineBase ()
:
	MediaPipeline ("encode_capture_pipeline")
{
}


/*!\brief pulls a sample from the application sink and emits via signal
 *
 */
GstFlowReturn EncodePipelineBase::sampleFromAppSinkPull (
	GStreamerAppSink appSink)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (appSink.get () != nullptr)
	{
		auto sample = appSink.pullSample ();

		if (sample.get () != nullptr)
		{
			auto gstBuffer = sample.bufferGet ();

			stiASSERT (!gstBuffer.flagIsSet(GST_BUFFER_FLAG_GAP));

			GstStructure *pGstStructure = nullptr;
			const char *pszCapsName = nullptr;

			auto gstCaps = sample.capsGet ();
			stiTESTCOND (gstCaps.get () != nullptr, stiRESULT_ERROR);

			pGstStructure = gst_caps_get_structure (gstCaps.get (), 0);
			stiTESTCOND (pGstStructure != nullptr, stiRESULT_ERROR);

			pszCapsName = gst_structure_get_name (pGstStructure);
			stiTESTCOND (pszCapsName != nullptr, stiRESULT_ERROR);

			stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
				stiTrace ("EncodePipelineBase::sampleFromAppSinkPull: pszCapsName = %s\n", pszCapsName);
			);

			encodeBitstreamReceivedSignal.Emit (sample);
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		return GST_FLOW_ERROR;
	}

	return GST_FLOW_OK;
}


void EncodePipelineBase::displaySinkProbeAdd ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	auto gstPad  = displaySinkGet().staticPadGet ("sink");
	stiTESTCOND (gstPad.get () != nullptr, stiRESULT_ERROR);

	gstPad.addBufferProbe(
		[this](
			GStreamerPad gstPad,
			GStreamerBuffer gstBuffer)
		{
			frameCountIncrement ();

			if (m_frameCaptureCallback)
			{
				m_frameCaptureCallback (gstPad, gstBuffer);
				m_frameCaptureCallback = nullptr;
			}

			return true;
		});

STI_BAIL:;
}


void EncodePipelineBase::frameCaptureCallbackSet (
	std::function <void(const GStreamerPad &gstPad, GStreamerBuffer &gstBuffer)> callback)
{
	m_frameCaptureCallback = callback;
}


void EncodePipelineBase::frameCountIncrement ()
{
	std::lock_guard<std::mutex> lock(m_frameCountMutex);

	m_frameCount++;
	m_lastFrameTime = std::chrono::steady_clock::now ();
}


void EncodePipelineBase::frameCountGet (
	uint64_t *frameCount,
	std::chrono::steady_clock::time_point *lastFrameTime)
{
	std::lock_guard<std::mutex> lock(m_frameCountMutex);

	*frameCount = m_frameCount;

	if (lastFrameTime)
	{
		*lastFrameTime = m_lastFrameTime;
	}
}


void EncodePipelineBase::ptzRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	captureBinBaseGet ().cropRectangleSet (ptzRectangle);
}


float EncodePipelineBase::aspectRatioGet () const
{
	auto cropRectangle = captureBinBaseGet ().cropRectangleGet ();

	if (cropRectangle.vertZoom)
	{
		return (float)cropRectangle.horzZoom / (float)cropRectangle.vertZoom;
	}

	return 0;
}

stiHResult EncodePipelineBase::brightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase::brightnessGet (
	int *brightness) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessGet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessSet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase::saturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase::saturationGet (
	int *saturation) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationGet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationSet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

int EncodePipelineBase::maxCaptureWidthGet () const
{
	return captureBinBaseGet().maxCaptureWidthGet();
}


int EncodePipelineBase::maxCaptureHeightGet () const
{
	return captureBinBaseGet().maxCaptureHeightGet();
}
